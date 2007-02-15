/*
 * This file is part of the Sofia-SIP package
 *
 * Copyright (C) 2006 Nokia Corporation.
 *
 * Contact: Pekka Pessi <pekka.pessi@nokia.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */

/**@CFILE nua_session.c
 * @brief SIP session handling
 *
 * @author Pekka Pessi <Pekka.Pessi@nokia.com>
 *
 * @date Created: Wed Mar  8 16:17:27 EET 2006 ppessi
 */

#include "config.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include <assert.h>

#include <sofia-sip/string0.h>
#include <sofia-sip/sip_protos.h>
#include <sofia-sip/sip_status.h>
#include <sofia-sip/sip_util.h>
#include <sofia-sip/su_uniqueid.h>

#define NTA_INCOMING_MAGIC_T struct nua_server_request
#define NTA_OUTGOING_MAGIC_T struct nua_client_request
#define NTA_RELIABLE_MAGIC_T struct nua_handle_s

#include "nua_stack.h"
#include <sofia-sip/soa.h>

#ifndef SDP_H
typedef struct sdp_session_s sdp_session_t;
#endif

/* ---------------------------------------------------------------------- */

/** @enum nua_callstate

The states for SIP session established with INVITE.

Initially the call states follow the state of the INVITE transaction. If the
initial INVITE transaction fails, the call is terminated. The status codes
401 and 407 are an exception: if the client (on the left side in the diagram
below) receives them, it enters in #nua_callstate_authenticating state.

If a re-INVITE transaction fails, the result depends on the status code in
failure. The call can return to the ready state, be terminated immediately,
or be terminated gracefully. The proper action to take is determined with
sip_response_terminates_dialog().			   

@sa @ref nua_call_model, #nua_i_state, nua_invite(), #nua_i_invite
							   
@par Session State Diagram				   
							   
@code							   
  			 +----------+			   
  			 |          |---------------------+
  			 |   Init   |                     |
  			 |          |----------+          |
  			 +----------+          |          |
  			  |        |           |          |
                 --/INVITE|        |INVITE/100 |          |
                          V        V           |          |
     		+----------+      +----------+ |          |
       +--------|          |      |          | |          |
       |  18X +-| Calling  |      | Received | |INVITE/   |
       |   /- | |          |      |          | |  /18X    |
       |      V +----------+      +----------+ V          |
       |   +----------+ |           |     |  +----------+ |
       |---|          | |2XX     -/ |  -/ |  |          | |
       |   | Proceed- | | /-     2XX|  18X|  |  Early   | |INVITE/
       |   |   ing    | |           |     +->|          | |  /200
       |   +----------+ V           V        +----------+ |
       |     |  +----------+      +----------+   | -/     |
       |  2XX|  |          |      |          |<--+ 2XX    |
       |   /-|  | Complet- |      | Complete |<-----------+
       |     +->|   ing    |      |          |------+
       |        +----------+      +----------+      |
       |                  |        |      |         |
       |401,407/     -/ACK|        |ACK/- |timeout/ |
       | /ACK             V        V      | /BYE    |
       |                 +----------+     |         |
       |                 |          |     |         |
       |              +--|  Ready   |     |         |
       |              |  |          |     |         |
       |              |  +----------+     |         |
       |              |       |           |         |
       |         BYE/ |       |-/BYE      |         |BYE/
       V         /200 |       V           |         |/200
  +----------+        |  +----------+     |         |
  |          |        |  |          |     |         |
  |Authentic-|        |  | Terminat-|<----+         |
  |  ating   |        |  |   ing    |               |
  +----------+        |  +----------+               |
                      |       |                     |
                      |       |[23456]XX/-          |
                      |       V                     |
                      |  +----------+               |
                      |  |          |               |
                      +->|Terminated|<--------------+
                         |          |
                         +----------+
                              | 
                              V
                         +----------+
        		 |          |
                         |   Init   |
			 |          |
          		 +----------+
@endcode			      
*/			      
			      
/* ---------------------------------------------------------------------- */
/* Session event usage */

/** Session-related state */
typedef struct nua_session_usage
{
  enum nua_callstate ss_state;		/**< Session status (enum nua_callstate) */
  
  unsigned        ss_100rel:1;	        /**< Use 100rel, send 183 */
  unsigned        ss_alerting:1;	/**< 180 is sent/received */
  
  unsigned        ss_update_needed:2;	/**< Send an UPDATE (do O/A if > 1) */

  unsigned        ss_precondition:1;	/**< Precondition required */

  unsigned        ss_timer_set:1;       /**< We have active session timer. */

  unsigned        ss_reporting:1;       /**< True if reporting state */
  unsigned        : 0;
  
  unsigned        ss_session_timer;	/**< Value of Session-Expires (delta) */
  unsigned        ss_min_se;		/**< Minimum session expires */
  enum nua_session_refresher ss_refresher; /**< none, local or remote */

  char const     *ss_oa_recv, *ss_oa_sent;
  char const     *ss_reason;	        /**< Reason for termination. */
} nua_session_usage_t;

static char const *nua_session_usage_name(nua_dialog_usage_t const *du);
static int nua_session_usage_add(nua_handle_t *nh,
				 nua_dialog_state_t *ds,
				 nua_dialog_usage_t *du);
static void nua_session_usage_remove(nua_handle_t *nh,
				     nua_dialog_state_t *ds,
				     nua_dialog_usage_t *du);
static void nua_session_usage_refresh(nua_owner_t *,
				      nua_dialog_state_t *,
				      nua_dialog_usage_t *,
				      sip_time_t now);
static int nua_session_usage_shutdown(nua_owner_t *,
				      nua_dialog_state_t *,
				      nua_dialog_usage_t *);

static nua_usage_class const nua_session_usage[1] = {
  {
    sizeof (nua_session_usage_t),
    sizeof nua_session_usage,
    nua_session_usage_add,
    nua_session_usage_remove,
    nua_session_usage_name,
    NULL,
    nua_session_usage_refresh,
    nua_session_usage_shutdown
  }};

static char const *nua_session_usage_name(nua_dialog_usage_t const *du)
{
  return "session";
}

static
int nua_session_usage_add(nua_handle_t *nh,
			   nua_dialog_state_t *ds,
			   nua_dialog_usage_t *du)
{
  if (ds->ds_has_session)
    return -1;
  ds->ds_has_session = 1;

  return 0;
}

static
void nua_session_usage_remove(nua_handle_t *nh,
			       nua_dialog_state_t *ds,
			       nua_dialog_usage_t *du)
{
  nua_session_usage_t *ss = nua_dialog_usage_private(du);

  ds->ds_has_session = 0;
  
  (void)ss;
}

static
nua_dialog_usage_t *nua_dialog_usage_for_session(nua_dialog_state_t const *ds)
{
  if (ds == ((nua_handle_t *)NULL)->nh_ds)
    return NULL;

  return nua_dialog_usage_get(ds, nua_session_usage, NULL);
}

static
nua_session_usage_t *nua_session_usage_get(nua_dialog_state_t const *ds)
{
  nua_dialog_usage_t *du;

  if (ds == ((nua_handle_t *)NULL)->nh_ds)
    return NULL;

  du = nua_dialog_usage_get(ds, nua_session_usage, NULL);

  return (nua_session_usage_t *)nua_dialog_usage_private(du);
}

/** Zap the session associated with the handle */
static
void nua_session_usage_destroy(nua_handle_t *nh,
			       nua_session_usage_t *ss)
{
  nh->nh_has_invite = 0;
  nh->nh_active_call = 0;
  nh->nh_hold_remote = 0;

  if (nh->nh_soa)
    soa_destroy(nh->nh_soa), nh->nh_soa = NULL;

  /* Remove usage */
  nua_dialog_usage_remove(nh, nh->nh_ds, nua_dialog_usage_public(ss));

  SU_DEBUG_5(("nua: terminated session %p\n", (void *)nh));
}

/* ======================================================================== */
/* INVITE and call (session) processing */

int nua_stack_prack(nua_t *nua, nua_handle_t *nh, nua_event_t e,
		    tagi_t const *tags);

static void session_timer_preferences(nua_session_usage_t *ss,
				      unsigned expires,
				      unsigned min_se,
				      enum nua_session_refresher refresher);

static int session_timer_is_supported(nua_handle_t const *nh);

static int prefer_session_timer(nua_handle_t const *nh);

static int use_session_timer(nua_session_usage_t *ss, int uas, int always,
			     msg_t *msg, sip_t *);
static int init_session_timer(nua_session_usage_t *ss, sip_t const *, int refresher);
static void set_session_timer(nua_session_usage_t *ss);

static int session_timer_check_restart(nua_client_request_t *cr,
				       int status, char const *phrase,
				       sip_t const *sip);

static int nh_referral_check(nua_handle_t *nh, tagi_t const *tags);
static void nh_referral_respond(nua_handle_t *,
				int status, char const *phrase);

static void signal_call_state_change(nua_handle_t *nh,
				      nua_session_usage_t *ss,
				      int status, char const *phrase,
				      enum nua_callstate next_state);

static
int session_get_description(sip_t const *sip,
			    char const **return_sdp,
			    size_t *return_len);

static
int session_include_description(soa_session_t *soa,
				int session,
				msg_t *msg,
				sip_t *sip);

static
int session_make_description(su_home_t *home,
			     soa_session_t *soa,
			     int session,
			     sip_content_disposition_t **return_cd,
			     sip_content_type_t **return_ct,
			     sip_payload_t **return_pl);

static
int nua_server_retry_after(nua_server_request_t *sr,
			   int status, char const *phrase,
			   int min, int max);

/**@fn void nua_invite(nua_handle_t *nh, tag_type_t tag, tag_value_t value, ...);
 *
 * Place a call using SIP INVITE method. 
 *
 * Incomplete call can be hung-up with nua_cancel(). Complete or incomplete
 * calls can be hung-up with nua_bye().
 *
 * Optionally 
 * - uses early media if NUTAG_EARLY_MEDIA() tag is used with non zero-value
 * - media parameters can be set by SOA tags
 * - nua_invite() can be used to change status of an existing call: 
 *   - #SOATAG_HOLD tag can be used to list the media that will be put on hold,
 *     the value "*" sets all the media beloginging to the session on hold
 *
 * @param nh              Pointer to operation handle
 * @param tag, value, ... List of tagged parameters
 *
 * @return 
 *    nothing
 *
 * @par Related Tags:
 *    NUTAG_URL() \n
 *    Tags of nua_set_hparams() \n
 *    NUTAG_INCLUDE_EXTRA_SDP() \n
 *    SOATAG_HOLD(), SOATAG_AF(), SOATAG_ADDRESS(),
 *    SOATAG_RTP_SELECT(), SOATAG_RTP_SORT(), SOATAG_RTP_MISMATCH(), 
 *    SOATAG_AUDIO_AUX(), \n
 *    SOATAG_USER_SDP() or SOATAG_USER_SDP_STR() \n
 *    See use of tags in <sip_tag.h> below
 *
 * @par Events:
 *    #nua_r_invite \n
 *    #nua_i_state (#nua_i_active, #nua_i_terminated) \n
 *    #nua_i_media_error \n
 *    #nua_i_fork \n
 *
 * @par Populating SIP Request Message with Tagged Arguments
 * The tagged arguments can be used to pass values for any SIP headers to
 * the stack. When the INVITE message (or any other SIP message) is created,
 * the tagged values saved with nua_handle() are used first, next the tagged
 * values given with the operation (nua_invite()) are added.
 *
 * @par
 * When multiple tags for the same header are specified, the behaviour
 * depends on the header type. If only a single header field can be included
 * in a SIP message, the latest non-NULL value is used, e.g., @Subject. 
 * However, if the SIP header can consist of multiple lines or header fields
 * separated by comma, e.g., @Accept, all the tagged
 * values are concatenated.
 * 
 * @par
 * However, if a tag value is #SIP_NONE (-1 casted as a void pointer), the
 * values from previous tags are ignored.
 *
 * @par
 * Next, values previously set with nua_set_params() or nua_set_hparams()
 * are used: @Allow, @Supported, @Organization, and @UserAgent headers are
 * added to the request if they are not already set.
 *
 * @par
 * Now, the target URI for the request needs to be determined.
 *
 * @par
 * For initial INVITE requests, values from tags are used. If NUTAG_URL() is
 * given, it is used as target URI. Otherwise, if SIPTAG_TO() is given, it
 * is used as target URI. If neither is given, the complete request line
 * already specified using SIPTAG_REQUEST() or SIPTAG_REQUEST_STR() is used. 
 * If none of the tags above are given, an internal error is returned to the
 * application. At this point, the target URI is stored in the request line,
 * together with method name ("INVITE") and protocol version ("SIP/2.0"). 
 * The initial dialog information is also created: @CallID, @CSeq headers
 * are generated, if they do not exist, and tag is added to @From header.
 *
 * @par
 * For in-dialog INVITE (re-INVITE), the request URI is taken from the
 * @Contact header received from the remote party during the dialog
 * establishment. Also, the @CallID and @CSeq headers and @From and @To tags
 * are generated based on the dialog information and added to the request. 
 * If the dialog has a route (set by @RecordRoute headers), it is added to
 * the request, too.
 *
 * @par
 * @MaxForwards header (with default value set by NTATAG_MAX_FORWARDS()) is
 * also added now, if it does not exist.
 * 
 * @par
 * Next, the stack generates a @Contact header for the request (Unless the
 * application already gave a @Contact header or it does not want to use
 * @Contact and indicates that by including SIPTAG_CONTACT(NULL) or
 * SIPTAG_CONTACT(SIP_NONE) in the tagged parameters.) If the application
 * has registered the URI in @From header, the @Contact header used with
 * registration is used. Otherwise, the @Contact header is generated from the
 * local IP address and port number.
 *
 * @par
 * For the initial INVITE requests, @ServiceRoute set received from
 * the registrar is also added to the request message.
 *
 * @par
 * The INVITE request message created by nua_invite() operation is saved as
 * a template for automatic re-INVITE requests sent by the session timer
 * ("timer") feature (see NUTAG_SESSION_TIMER() for more details). Please
 * note that the template message is not used when ACK, PRACK, UPDATE or
 * INFO requests are created (however, these requests will include
 * dialog-specific headers like @To, @From, and @CallID as well as
 * preference headers @Allow, @Supported, @UserAgent, @Organization).
 *
 * @par SDP Handling
 * The initial nua_invite() creates a @ref soa_session_t "soa media session"
 * unless NUTAG_MEDIA_ENABLE(0) has been given. The SDP description of the
 * @ref soa_session_t "soa media session" is included in the INVITE request
 * as message body. 
 *
 * @par
 * The SDP in a 1XX or 2XX response message is interpreted as an answer,
 * given to the @ref soa_session_t "soa media session" object for
 * processing.
 *
 * @bug If the INVITE request already contains a message body, SDP is not
 * added. Also, if the response contains a multipart body, it is not parsed.
 *
 * @par Authentication
 * The INVITE request may need authentication. Each proxy or server
 * requiring authentication can respond with 401 or 407 response. The
 * nua_authenticate() operation stores authentication information (username
 * and password) to the handle, and stack tries to authenticate all the rest
 * of the requests (e.g., PRACK, ACK, UPDATE, re-INVITE, BYE) using same
 * username and password.
 *
 * @sa @ref nua_call_model, #nua_r_invite, #nua_i_state, \n
 *     nua_handle_has_active_call() \n
 *     nua_handle_has_call_on_hold()\n
 *     nua_handle_has_invite() \n
 *     nua_authenticate() \n
 *     nua_prack() \n
 *     nua_update() \n
 *     nua_info() \n 
 *     nua_cancel() \n
 *     nua_bye() \n
 *     #nua_i_invite, nua_respond()
 */

/* Tags not implemented
 *    NUTAG_REFER_PAUSE() \n
 */

static int nua_invite_client_init(nua_client_request_t *cr, 
				  msg_t *msg, sip_t *sip,
				  tagi_t const *tags);
static int nua_invite_client_request(nua_client_request_t *cr,
				     msg_t *msg, sip_t *sip,
				     tagi_t const *tags);
static int nua_invite_client_preliminary(nua_client_request_t *cr,
					 int status, char const *phrase,
					 sip_t const *sip);
static int nua_invite_client_response(nua_client_request_t *cr,
				      int status, char const *phrase,
				      sip_t const *sip);
static int nua_session_client_response(nua_client_request_t *cr,
				       int status, char const *phrase,
				       sip_t const *sip);
static int nua_invite_client_report(nua_client_request_t *cr,
				    int status, char const *phrase,
				    sip_t const *sip,
				    nta_outgoing_t *orq,
				    tagi_t const *tags);

static int nua_invite_client_ack(nua_client_request_t *cr, tagi_t const *tags);
static int nua_invite_client_ack_msg(nua_client_request_t *cr, 
				     msg_t *msg, sip_t *sip,
				     tagi_t const *tags);

static int nua_invite_client_deinit(nua_client_request_t *cr);

nua_client_methods_t const nua_invite_client_methods = {
  SIP_METHOD_INVITE,
  0,
  { 
    /* create_dialog */ 1,
    /* in_dialog */ 1,
    /* target refresh */ 1
  },
  NULL,
  nua_invite_client_init,
  nua_invite_client_request,
  session_timer_check_restart,
  nua_invite_client_response,
  nua_invite_client_preliminary,
  nua_invite_client_report,
  nua_invite_client_deinit
};

extern nua_client_methods_t const nua_bye_client_methods;
extern nua_client_methods_t const nua_cancel_client_methods;
extern nua_client_methods_t const nua_info_client_methods;
extern nua_client_methods_t const nua_update_client_methods;
extern nua_client_methods_t const nua_prack_client_methods;

int nua_stack_invite(nua_t *nua, nua_handle_t *nh, nua_event_t e,
		     tagi_t const *tags)
{
  return nua_client_create(nh, e, &nua_invite_client_methods, tags);
}

static int nua_invite_client_init(nua_client_request_t *cr, 
				  msg_t *msg, sip_t *sip,
				  tagi_t const *tags)
{
  nua_handle_t *nh = cr->cr_owner;
  nua_dialog_usage_t *du;

  cr->cr_usage = du = nua_dialog_usage_for_session(nh->nh_ds);
  
  if (nh_is_special(nh) || 
      nua_stack_set_handle_special(nh, nh_has_invite, nua_i_error))
    return nua_client_return(cr, 900, "Invalid handle for INVITE", msg);
  else if (nh_referral_check(nh, tags) < 0)
    return nua_client_return(cr, 900, "Invalid referral", msg);

  if (!du)
    du = nua_dialog_usage_add(nh, nh->nh_ds, nua_session_usage, NULL);
  if (!du)
    return -1;

  if (nua_client_bind(cr, du) < 0)
    return nua_client_return(cr, 900, "INVITE already in progress", msg);

  session_timer_preferences(nua_dialog_usage_private(du), 
			    NH_PGET(nh, session_timer),
			    NH_PGET(nh, min_se),
			    NH_PGET(nh, refresher));

  return 0;
}

static int nua_invite_client_request(nua_client_request_t *cr,
				     msg_t *msg, sip_t *sip,
				     tagi_t const *tags)
{
  nua_handle_t *nh = cr->cr_owner;
  nua_dialog_usage_t *du = cr->cr_usage;
  nua_session_usage_t *ss = nua_dialog_usage_private(du);
  int offer_sent = 0, retval;
  sip_time_t invite_timeout;

  if (du == NULL)		/* Call terminated */ 
    return nua_client_return(cr, SIP_481_NO_TRANSACTION, msg);

  assert(ss);

  invite_timeout = NH_PGET(nh, invite_timeout);
  if (invite_timeout == 0)
    invite_timeout = UINT_MAX;
  /* Cancel if we don't get response within timeout*/
  nua_dialog_usage_set_expires(du, invite_timeout);
  nua_dialog_usage_set_refresh(du, 0);

  /* Add session timer headers */
  if (session_timer_is_supported(nh))
    use_session_timer(ss, 0, prefer_session_timer(nh), msg, sip);

  ss->ss_100rel = NH_PGET(nh, early_media);
  ss->ss_precondition = sip_has_feature(sip->sip_require, "precondition");
  if (ss->ss_precondition)
    ss->ss_update_needed = ss->ss_100rel = 1;

  if (nh->nh_soa) {
    soa_init_offer_answer(nh->nh_soa);

    if (sip->sip_payload)
      offer_sent = 0;		/* XXX - kludge */
    else if (soa_generate_offer(nh->nh_soa, 0, NULL) < 0)
      return -1;
    else
      offer_sent = 1;
  }

  if (offer_sent > 0 &&
      session_include_description(nh->nh_soa, 1, msg, sip) < 0)
    return nua_client_return(cr, 900, "Internal media error", msg);

  if (nh->nh_soa &&
      NH_PGET(nh, media_features) &&
      !nua_dialog_is_established(nh->nh_ds) &&
      !sip->sip_accept_contact && !sip->sip_reject_contact) {
    sip_accept_contact_t ac[1];
    sip_accept_contact_init(ac);

    ac->cp_params = (msg_param_t *)
      soa_media_features(nh->nh_soa, 1, msg_home(msg));

    if (ac->cp_params) {
      msg_header_replace_param(msg_home(msg), ac->cp_common, "explicit");
      sip_add_dup(msg, sip, (sip_header_t *)ac);
    }
  }

  retval = nua_base_client_trequest(cr, msg, sip,
				    NTATAG_REL100(ss->ss_100rel),
				    TAG_NEXT(tags));
  if (retval == 0) {
    cr->cr_offer_sent = offer_sent;
    ss->ss_oa_sent = offer_sent ? "offer" : NULL;

    if (!cr->cr_restarting)
      signal_call_state_change(nh, ss, 0, "INVITE sent", 
			       nua_callstate_calling);
  }

  return retval;
}

static int nua_invite_client_response(nua_client_request_t *cr,
				      int status, char const *phrase,
				      sip_t const *sip)
{
  nua_handle_t *nh = cr->cr_owner;
  nua_dialog_usage_t *du = cr->cr_usage;
  nua_session_usage_t *ss = nua_dialog_usage_private(du);

  if (ss == NULL || sip == NULL) {
    /* Xyzzy */
  }
  else if (status < 300) {
    du->du_ready = 1;

    init_session_timer(ss, sip, NH_PGET(nh, refresher));
    set_session_timer(ss);
  }
  
  return nua_session_client_response(cr, status, phrase, sip);
}

static int nua_invite_client_preliminary(nua_client_request_t *cr,
					 int status, char const *phrase,
					 sip_t const *sip)
{
  nua_handle_t *nh = cr->cr_owner;
  nua_dialog_usage_t *du = cr->cr_usage;
  nua_session_usage_t *ss = nua_dialog_usage_private(du);

  assert(sip); assert(ss);

  if (ss && sip && sip->sip_rseq) {
    /* Handle 100rel responses */
    sip_rseq_t *rseq = sip->sip_rseq;

    /* Establish early dialog - we should fork here */
    if (!nua_dialog_is_established(nh->nh_ds)) {
      nta_outgoing_t *tagged;

      nua_dialog_uac_route(nh, nh->nh_ds, sip, 1);
      nua_dialog_store_peer_info(nh, nh->nh_ds, sip);
      
      /* Tag the INVITE request */
      tagged = nta_outgoing_tagged(cr->cr_orq,
				   nua_client_orq_response, cr,
				   sip->sip_to->a_tag, sip->sip_rseq);
      if (tagged) {
	nta_outgoing_destroy(cr->cr_orq), cr->cr_orq = tagged;
      }
      else {
	cr->cr_graceful = 1;
	ss->ss_reason = "SIP;cause=500;text=\"Cannot Create Early Dialog\"";
      }
    }
  
    if (!rseq) {
      SU_DEBUG_5(("nua(%p): 100rel missing RSeq\n", (void *)nh));
    }
    else if (nta_outgoing_rseq(cr->cr_orq) > rseq->rs_response) {
      SU_DEBUG_5(("nua(%p): 100rel bad RSeq %u (got %u)\n", (void *)nh, 
		  (unsigned)rseq->rs_response,
		  nta_outgoing_rseq(cr->cr_orq)));
      return 1;    /* Do not send event */
    }
    else if (nta_outgoing_setrseq(cr->cr_orq, rseq->rs_response) < 0) {
      SU_DEBUG_1(("nua(%p): cannot set RSeq %u\n", (void *)nh, 
		  (unsigned)rseq->rs_response));
      cr->cr_graceful = 1;
      ss->ss_reason = "SIP;cause=400;text=\"Bad RSeq\"";
    }
  }

  return nua_session_client_response(cr, status, phrase, sip);
}

/** Process response to a session request (INVITE, PRACK, UPDATE) */
static int nua_session_client_response(nua_client_request_t *cr,
				       int status, char const *phrase,
				       sip_t const *sip)
{
  nua_handle_t *nh = cr->cr_owner;
  nua_dialog_usage_t *du = cr->cr_usage;
  nua_session_usage_t *ss = nua_dialog_usage_private(du);

  char const *sdp = NULL;
  size_t len;
  char const *received = NULL;

#define LOG3(m) \
  SU_DEBUG_3(("nua(%p): %s: %s %s in %u %s\n", \
	      (void *)nh, cr->cr_method_name, (m),		\
	      received ? received : "SDP", status, phrase))
#define LOG5(m) \
  SU_DEBUG_5(("nua(%p): %s: %s %s in %u %s\n", \
	      (void *)nh, cr->cr_method_name, (m), received, status, phrase))

  if (nh->nh_soa == NULL || !ss || !sip || 300 <= status)
    /* Xyzzy */;
  else if (!session_get_description(sip, &sdp, &len))
    /* No SDP */;
  else if (cr->cr_answer_recv) {
    /* Ignore spurious answers after completing O/A */
    LOG3("ignoring duplicate");
    sdp = NULL;
  }
  else if (cr->cr_offer_sent) {
    /* case 1: incoming answer */
    cr->cr_answer_recv = status;
    received = "answer";

    if (soa_set_remote_sdp(nh->nh_soa, NULL, sdp, len) < 0) {
      LOG3("error parsing SDP");
      sdp = NULL;
      cr->cr_graceful = 1;
      ss->ss_reason = "SIP;cause=400;text=\"Malformed Session Description\"";
    }
    else if (soa_process_answer(nh->nh_soa, NULL) < 0) {
      LOG5("error processing SDP");
      /* XXX */
      sdp = NULL;
    }
    else if (soa_activate(nh->nh_soa, NULL) < 0)
      /* XXX - what about errors? */
      LOG3("error activating media after");
    else
      LOG5("processed SDP");
  }
  else if (cr->cr_method != sip_method_invite) {
    /* If non-invite request did not have offer, ignore SDP in response */
    LOG3("ignoring extra");
    sdp = NULL;
  }
  else {
    /* case 2: answer to our offer */
    cr->cr_offer_recv = 1, cr->cr_answer_sent = 0;
    received = "offer";

    if (soa_set_remote_sdp(nh->nh_soa, NULL, sdp, len) < 0) {
      LOG3("error parsing SDP");
      sdp = NULL;
      cr->cr_graceful = 1;
      ss->ss_reason = "SIP;cause=400;text=\"Malformed Session Description\"";
    }
    else 
      LOG5("got SDP");
  }

  if (ss && received)
    ss->ss_oa_recv = received;

  if (sdp)
    return nua_base_client_tresponse(cr, status, phrase, sip,
				     NH_REMOTE_MEDIA_TAGS(1, nh->nh_soa),
				     TAG_END());
  else
    return nua_base_client_response(cr, status, phrase, sip, NULL);
}

static int nua_invite_client_report(nua_client_request_t *cr,
				    int status, char const *phrase,
				    sip_t const *sip,
				    nta_outgoing_t *orq,
				    tagi_t const *tags)
{
  nua_handle_t *nh = cr->cr_owner;
  nua_dialog_usage_t *du = cr->cr_usage;
  nua_session_usage_t *ss = nua_dialog_usage_private(du);
  unsigned next_state;
  int error;

  nh_referral_respond(nh, status, phrase);

  nua_stack_event(nh->nh_nua, nh, 
		  nta_outgoing_getresponse(orq),
		  cr->cr_event,
		  status, phrase,
		  tags);

  if (orq != cr->cr_orq && status != 100)
    return 1;

  ss->ss_reporting = 1;

  if (ss == NULL) {
    next_state = nua_callstate_terminated;
  }
  else if (status == 100) {
    next_state = nua_callstate_calling;
  }
  else if (status < 300 && cr->cr_graceful) {
    next_state = nua_callstate_terminating;
    if (200 <= status) {
      nua_invite_client_ack(cr, NULL);
    }
  }
  else if (status < 200) {
    next_state = nua_callstate_proceeding;
    if (sip && sip->sip_rseq) {
      sip_rack_t rack[1];

      sip_rack_init(rack);
      rack->ra_response    = sip->sip_rseq->rs_response;
      rack->ra_cseq        = sip->sip_cseq->cs_seq;
      rack->ra_method      = sip->sip_cseq->cs_method;
      rack->ra_method_name = sip->sip_cseq->cs_method_name;

      error = nua_client_tcreate(nh, nua_r_prack, &nua_prack_client_methods, 
				 SIPTAG_RACK(rack),
				 TAG_END());
      if (error < 0) {
	cr->cr_graceful = 1;
	next_state = nua_callstate_terminating;
      }
    }
  }
  else if (status < 300) {
    next_state = nua_callstate_completing;
  }
  else if (cr->cr_terminated) {
    next_state = nua_callstate_terminated;
  }
  else if (cr->cr_graceful && ss->ss_state >= nua_callstate_completing) {
    next_state = nua_callstate_terminating;
  }
  else {
    next_state = nua_callstate_init;
  }

  if (next_state == nua_callstate_calling) {
    if (sip && sip->sip_status && sip->sip_status->st_status == 100) {
      ss->ss_reporting = 0;
      return 1;
    }
  }

  if (next_state == nua_callstate_completing) {
    if (NH_PGET(nh, auto_ack) ||
	/* Auto-ACK response to re-INVITE unless auto_ack is set to 0 */
	(ss->ss_state == nua_callstate_ready &&
	 !NH_PISSET(nh, auto_ack))) {

      if (nua_invite_client_ack(cr, NULL) > 0)
	next_state = nua_callstate_ready;
      else
	next_state = nua_callstate_terminating;
    }
  }

  if (next_state == nua_callstate_terminating) {
    /* Send BYE or CANCEL */
    /* XXX - Forking - send BYE to early dialog?? */
    if (ss->ss_state > nua_callstate_proceeding || status >= 200)
      error = nua_client_create(nh, nua_r_bye, &nua_bye_client_methods, NULL);
    else
      error = nua_client_create(nh, nua_r_cancel, 
				&nua_cancel_client_methods, tags);

    if (error) {
      next_state = nua_callstate_terminated;
      cr->cr_terminated = 1;
    }
    cr->cr_graceful = 0;
  }

  ss->ss_reporting = 0;

  signal_call_state_change(nh, ss, status, phrase, next_state);

  return 1;
}

/**@fn void nua_ack(nua_handle_t *nh, tag_type_t tag, tag_value_t value, ...);
 *
 * Acknowledge a succesful response to INVITE request.
 *
 * Acknowledge a successful response (200..299) to INVITE request with the
 * SIP ACK request message. This function is needed only if NUTAG_AUTOACK()
 * parameter has been cleared.
 *
 * @param nh              Pointer to operation handle
 * @param tag, value, ... List of tagged parameters
 *
 * @return 
 *    nothing
 *
 * @par Related Tags:
 *    Tags in <sip_tag.h>
 *
 * @par Events:
 *    #nua_i_media_error \n
 *    #nua_i_state  (#nua_i_active, #nua_i_terminating, #nua_i_terminated) 
 *
 * @sa NUTAG_AUTOACK(), @ref nua_call_model, #nua_i_state
 */

int nua_stack_ack(nua_t *nua, nua_handle_t *nh, nua_event_t e,
		  tagi_t const *tags)
{
  nua_dialog_usage_t *du = nua_dialog_usage_for_session(nh->nh_ds);
  nua_session_usage_t *ss = nua_dialog_usage_private(du);

  if (!du || 
      !du->du_cr || 
      du->du_cr->cr_orq == NULL || 
      du->du_cr->cr_status < 200) {
    UA_EVENT2(nua_i_error, 900, "No response to ACK");
    return 1;
  }

  if (tags) {
    nua_stack_set_params(nua, nh, nua_i_error, tags);
    if (nh->nh_soa)
      soa_set_params(nh->nh_soa, TAG_NEXT(tags));
  }

  if (nua_invite_client_ack(du->du_cr, tags) < 0) {
    int error;
    ss->ss_reason = "SIP;cause=500;text=\"Internal Error\"";
    ss->ss_reporting = 1;	/* We report state here if BYE fails */
    error = nua_client_create(nh, nua_r_bye, &nua_bye_client_methods, NULL);
    ss->ss_reporting = 0;
    signal_call_state_change(nh, ss, 500, "Internal Error", 
			     error 
			     ? nua_callstate_terminated
			     : nua_callstate_terminating);
  }

  return 0;
}

/** Send ACK, destroy INVITE transaction.
 *
 *  @retval 1 if successful
 *  @retval < 0 if an error occurred
 */
static
int nua_invite_client_ack(nua_client_request_t *cr, tagi_t const *tags)
{
  nua_handle_t *nh = cr->cr_owner;
  nua_dialog_state_t *ds = nh->nh_ds;

  msg_t *msg;
  sip_t *sip;
  int error = -1;
  sip_authorization_t *wa;
  sip_proxy_authorization_t *pa;
  sip_cseq_t *cseq;

  assert(ds->ds_leg);
  assert(cr->cr_orq);

  msg = nta_outgoing_getrequest(cr->cr_orq);
  sip = sip_object(msg);  
  if (!msg)
    return -1;
  
  wa = sip_authorization(sip);
  pa = sip_proxy_authorization(sip);
  
  msg_destroy(msg);

  msg = nta_msg_create(nh->nh_nua->nua_nta, 0);
  sip = sip_object(msg);  
  if (!msg)
    return -1;

  cseq = sip_cseq_create(msg_home(msg), cr->cr_seq, SIP_METHOD_ACK);

  if (!cseq)
    ;
  else if (nh->nh_tags && sip_add_tl(msg, sip, TAG_NEXT(nh->nh_tags)) < 0)
    ;
  else if (tags && sip_add_tl(msg, sip, TAG_NEXT(tags)) < 0)
    ;
  else if (wa && sip_add_dup(msg, sip, (sip_header_t *)wa) < 0)
    ;
  else if (pa && sip_add_dup(msg, sip, (sip_header_t *)pa) < 0)
    ;
  else if (sip_header_insert(msg, sip, (sip_header_t *)cseq) < 0)
    ;
  else if (nta_msg_request_complete(msg, ds->ds_leg, SIP_METHOD_ACK, NULL) < 0)
    ;
  else
    error = nua_invite_client_ack_msg(cr, msg, sip, tags);

  nta_outgoing_destroy(cr->cr_orq), cr->cr_orq = NULL;
  
  if (error == -1)
    msg_destroy(msg);

  return error;
}

/** Send ACK, destroy INVITE transaction.
 *
 *  @retval 1 if successful
 *  @retval -2 if an error occurred
 */
static
int nua_invite_client_ack_msg(nua_client_request_t *cr,
			      msg_t *msg, sip_t *sip,
			      tagi_t const *tags)
{
  nua_handle_t *nh = cr->cr_owner;
  nua_dialog_usage_t *du = cr->cr_usage;
  nua_session_usage_t *ss = nua_dialog_usage_private(du);

  nta_outgoing_t *ack;
  int status = 200;
  char const *phrase = "OK", *reason = NULL;

  /* Remove extra headers */
  while (sip->sip_allow)
    sip_header_remove(msg, sip, (sip_header_t*)sip->sip_allow);
  while (sip->sip_priority)
    sip_header_remove(msg, sip, (sip_header_t*)sip->sip_priority);
  while (sip->sip_proxy_require)
    sip_header_remove(msg, sip, (sip_header_t*)sip->sip_proxy_require);
  while (sip->sip_require)
    sip_header_remove(msg, sip, (sip_header_t*)sip->sip_require);
  while (sip->sip_subject)
    sip_header_remove(msg, sip, (sip_header_t*)sip->sip_subject);
  while (sip->sip_supported)
    sip_header_remove(msg, sip, (sip_header_t*)sip->sip_supported);

  if (!nh->nh_soa)
    ;
  else if (cr->cr_offer_recv && !cr->cr_answer_sent) {
    if (soa_generate_answer(nh->nh_soa, NULL) < 0 ||
	session_include_description(nh->nh_soa, 1, msg, sip) < 0) {
      status = 900, phrase = "Internal media error";
      reason = "SIP;cause=500;text=\"Internal media error\"";
      /* reason = soa_error_as_sip_reason(nh->nh_soa); */
    }
    else {
      cr->cr_answer_sent = 1;
      soa_activate(nh->nh_soa, NULL);
      /* signal that O/A round is complete */
      ss->ss_oa_sent = "answer";
    }

    if (!reason &&
	/* ss->ss_offer_sent && !ss->ss_answer_recv */
	!soa_is_complete(nh->nh_soa)) {
      /* No SDP answer in 2XX response -> terminate call */
      status = 988, phrase = "Incomplete offer/answer";
      reason = "SIP;cause=488;text=\"Incomplete offer/answer\"";
    }
  }

  if ((ack = nta_outgoing_mcreate(nh->nh_nua->nua_nta, NULL, NULL, NULL,
				  msg,
				  SIPTAG_END(),
				  TAG_NEXT(tags)))) {
    nta_outgoing_destroy(ack);	/* TR engine keeps this around for T2 */
  }
  else if (!reason) {
    status = 900, phrase = "Cannot send ACK";
    reason = "SIP;cause=500;text=\"Internal Error\"";
  }

  if (ss) {
    if (reason)
      ss->ss_reason = reason;

    if (!ss->ss_reporting && status < 300)
      signal_call_state_change(nh, ss, status, phrase, nua_callstate_ready);
  }
  
  return status < 300 ? 1 : -2;
}

/** Deinitialize client request */
static int nua_invite_client_deinit(nua_client_request_t *cr)
{
  if (cr->cr_orq == NULL)
    /* Xyzzy */;
  else if (cr->cr_status < 200)
    nta_outgoing_cancel(cr->cr_orq);
  else
    nua_invite_client_ack(cr, NULL);

  return 0;
}

/**@fn void nua_cancel(nua_handle_t *nh, tag_type_t tag, tag_value_t value, ...);
 *
 * Cancel an INVITE operation 
 *
 * @param nh              Pointer to operation handle
 * @param tag, value, ... List of tagged parameters
 *
 * @return 
 *    nothing
 *
 * @par Related Tags:
 *    Tags in <sip_tag.h>
 *
 * @par Events:
 *    #nua_r_cancel, #nua_i_state  (#nua_i_active, #nua_i_terminated)
 *
 * @sa @ref nua_call_model, nua_invite(), #nua_i_cancel
 */

static int nua_cancel_client_request(nua_client_request_t *cr,
				     msg_t *msg, sip_t *sip,
				     tagi_t const *tags);

nua_client_methods_t const nua_cancel_client_methods = {
  SIP_METHOD_CANCEL,
  0,
  { 
    /* create_dialog */ 0,
    /* in_dialog */ 1,
    /* target refresh */ 0
  },
  NULL,
  NULL,
  nua_cancel_client_request,
  /* nua_cancel_client_check_restart */ NULL,
  /* nua_cancel_client_response */ NULL
};

int nua_stack_cancel(nua_t *nua, nua_handle_t *nh, nua_event_t e,
		     tagi_t const *tags)
{
  return nua_client_create(nh, e, &nua_cancel_client_methods, tags);
}

static int nua_cancel_client_request(nua_client_request_t *cr,
				     msg_t *msg, sip_t *sip,
				     tagi_t const *tags)
{
  nua_handle_t *nh = cr->cr_owner;
  nua_dialog_usage_t *du = nua_dialog_usage_for_session(nh->nh_ds);

  if (!du || !du->du_cr || !du->du_cr->cr_orq || 
      nta_outgoing_status(du->du_cr->cr_orq) >= 200) {
    return nua_client_return(cr, 481, "No transaction to CANCEL", msg);
  }

  cr->cr_orq = nta_outgoing_tcancel(du->du_cr->cr_orq,
				    nua_client_orq_response, cr,
				    TAG_NEXT(tags));

  return cr->cr_orq ? 0 : -1;
}

/** @NUA_EVENT nua_r_cancel
 *
 * Answer to outgoing CANCEL.
 *
 * The CANCEL may be sent explicitly by nua_cancel() or implicitly by NUA
 * state machine.
 *
 * @param status response status code 
 * @param phrase a short textual description of @a status code
 * @param nh     operation handle associated with the call
 * @param hmagic application context associated with the call
 * @param sip    response to CANCEL request or NULL upon an error
 *               (status code is in @a status and 
 *                descriptive message in @a phrase parameters)
 * @param tags   empty
 *
 * @sa nua_cancel(), @ref nua_uac_call_model, #nua_r_invite, nua_invite(),
 * #nua_i_state
 *
 * @END_NUA_EVENT
 */

static void nua_session_usage_refresh(nua_handle_t *nh,
				      nua_dialog_state_t *ds,
				      nua_dialog_usage_t *du,
				      sip_time_t now)
{
  nua_session_usage_t *ss = nua_dialog_usage_private(du);
  nua_client_request_t const *cr = du->du_cr;
  nua_server_request_t const *sr;

  assert(cr);

  if (ss->ss_state >= nua_callstate_terminating || 
      /* No INVITE template */
      cr == NULL || 
      /* INVITE is in progress or being authenticated */
      cr->cr_orq || cr->cr_challenged)
    return;

  /* UPDATE in progress or being authenticated */
  for (cr = ds->ds_cr; cr; cr = cr->cr_next) 
    if (cr->cr_method == sip_method_update)
      return;

  /* INVITE or UPDATE in progress or being authenticated */
  for (sr = ds->ds_sr; sr; sr = sr->sr_next)
    if (sr->sr_usage == du && 
	(sr->sr_method == sip_method_invite || 
	 sr->sr_method == sip_method_update))
      return;

  if (!ss->ss_refresher) {
    if (du->du_expires == 0 || now < du->du_expires)
      /* Refresh contact & route set using re-INVITE */
      nua_client_resend_request(du->du_cr, 0, NULL);
    else {
      ss->ss_reason = "SIP;cause=408;text=\"Session timeout\""; 
      nua_stack_bye(nh->nh_nua, nh, nua_r_bye, NULL);
    }
  }
  else if (NH_PGET(nh, update_refresh)) {
    nua_stack_update(nh->nh_nua, nh, nua_r_update, NULL);
  }
  else {
    nua_client_resend_request(du->du_cr, 0, NULL);
  }
}

/** @interal Shut down session usage. 
 *
 * @retval >0  shutdown done
 * @retval 0   shutdown in progress
 * @retval <0  try again later
 */
static int nua_session_usage_shutdown(nua_handle_t *nh,
				      nua_dialog_state_t *ds,
				      nua_dialog_usage_t *du)
{
  nua_session_usage_t *ss = nua_dialog_usage_private(du);
  nua_server_request_t *sr, *sr_next;
  nua_client_request_t *cri;

  assert(ss == nua_session_usage_get(nh->nh_ds));

  /* Zap server-side transactions */
  for (sr = ds->ds_sr; sr; sr = sr_next) {
    sr_next = sr->sr_next;
    if (sr->sr_usage == du) {
      assert(sr->sr_usage == du);
      sr->sr_usage = NULL;

      if (nua_server_request_is_pending(sr)) {
	SR_STATUS1(sr, SIP_480_TEMPORARILY_UNAVAILABLE);
	nua_server_respond(sr, NULL);
	if (nua_server_report(sr) >= 2)
	  return 480;
      }
      else
	nua_server_request_destroy(sr);
    }
  }

  cri = du->du_cr;

  switch (ss->ss_state) {
  case nua_callstate_calling:
  case nua_callstate_proceeding:
    return nua_client_create(nh, nua_r_cancel, &nua_cancel_client_methods, NULL);

  case nua_callstate_completing:
  case nua_callstate_ready:
  case nua_callstate_completed:
    if (cri && cri->cr_orq) {
      if (cri->cr_status < 200)
	nua_client_create(nh, nua_r_cancel, &nua_cancel_client_methods, NULL);
      else if (cri->cr_status < 300)
	nua_invite_client_ack(cri, NULL);
    }
    if (nua_client_create(nh, nua_r_bye, &nua_bye_client_methods, NULL) != 0)
      break;
    return 0;

  case nua_callstate_terminating:
  case nua_callstate_terminated: /* XXX */
    return 0;

  default:
    break;
  }
  
  nua_dialog_usage_remove(nh, ds, du);

  return 200;
}

/**@fn void nua_prack(nua_handle_t *nh, tag_type_t tag, tag_value_t value, ...);
 * Send a PRACK request. 
 *
 * PRACK is used to acknowledge receipt of 100rel responses. See @RFC3262.
 *
 * @param nh              Pointer to operation handle
 * @param tag, value, ... List of tagged parameters
 *
 * @return 
 *    nothing
 *
 * @par Related Tags:
 *    Tags in <sofia-sip/soa_tag.h>, <sofia-sip/sip_tag.h>.
 *
 * @par Events:
 *    #nua_r_prack
 */

/** @NUA_EVENT nua_r_prack
 *
 * Response to an outgoing @b PRACK request. PRACK request is used to
 * acknowledge reliable preliminary responses and it is usually sent
 * automatically by the nua stack.
 *
 * @param status response status code
 *               (if the request is retried, @a status is 100, the @a
 *               sip->sip_status->st_status contain the real status code
 *               from the response message, e.g., 302, 401, or 407)
 * @param phrase a short textual description of @a status code
 * @param nh     operation handle associated with the call
 * @param hmagic application context associated with the call
 * @param sip    response to @b PRACK or NULL upon an error
 *               (status code is in @a status and 
 *                descriptive message in @a phrase parameters)
 * @param tags   empty
 *
 * @sa nua_prack(), #nua_i_prack, @RFC3262
 *
 * @END_NUA_EVENT
 */

static int nua_prack_client_init(nua_client_request_t *cr, 
				 msg_t *msg, sip_t *sip,
				 tagi_t const *tags);
static int nua_prack_client_request(nua_client_request_t *cr,
				    msg_t *msg, sip_t *sip,
				    tagi_t const *tags);
static int nua_prack_client_response(nua_client_request_t *cr,
				     int status, char const *phrase,
				     sip_t const *sip);
static int nua_prack_client_report(nua_client_request_t *cr,
				   int status, char const *phrase,
				   sip_t const *sip,
				   nta_outgoing_t *orq,
				   tagi_t const *tags);

nua_client_methods_t const nua_prack_client_methods = {
  SIP_METHOD_PRACK,
  0,
  { 
    /* create_dialog */ 0,
    /* in_dialog */ 1,
    /* target refresh */ 0
  },
  NULL,
  nua_prack_client_init,
  nua_prack_client_request,
  /* nua_prack_client_check_restart */ NULL,
  nua_prack_client_response,
  NULL,
  nua_prack_client_report
};

int nua_stack_prack(nua_t *nua, nua_handle_t *nh, nua_event_t e,
		     tagi_t const *tags)
{
  return nua_client_create(nh, e, &nua_prack_client_methods, tags);
}

static int nua_prack_client_init(nua_client_request_t *cr, 
				 msg_t *msg, sip_t *sip,
				 tagi_t const *tags)
{
  nua_handle_t *nh = cr->cr_owner;
  nua_dialog_usage_t *du = nua_dialog_usage_for_session(nh->nh_ds);

  cr->cr_usage = du;

  return 0;
}

static int nua_prack_client_request(nua_client_request_t *cr,
				    msg_t *msg, sip_t *sip,
				    tagi_t const *tags)
{
  nua_handle_t *nh = cr->cr_owner;
  nua_dialog_usage_t *du = cr->cr_usage;
  nua_session_usage_t *ss = nua_dialog_usage_private(du);
  nua_client_request_t *cri;
  int offer_sent = 0, answer_sent = 0, retval;
  int status = 0; char const *phrase = "PRACK Sent";
  uint32_t rseq = 0;

  if (du == NULL)		/* Call terminated */
    return nua_client_return(cr, SIP_481_NO_TRANSACTION, msg);
  assert(ss);

  cri = du->du_cr;

  if (sip->sip_rack)
    rseq = sip->sip_rack->ra_response;

  if (nh->nh_soa == NULL)
    /* It is up to application to handle SDP */;
  else if (sip->sip_payload)
    /* XXX - we should just do MIME in session_include_description() */;
  else if (cri->cr_offer_recv && !cri->cr_answer_sent) {
    if (soa_generate_answer(nh->nh_soa, NULL) < 0 ||
	session_include_description(nh->nh_soa, 1, msg, sip) < 0) {
      status = soa_error_as_sip_response(nh->nh_soa, &phrase);
      SU_DEBUG_3(("nua(%p): local response to PRACK: %d %s\n",
		  (void *)nh, status, phrase));
      nua_stack_event(nh->nh_nua, nh, NULL,
		      nua_i_media_error, status, phrase,
		      NULL);
      return nua_client_return(cr, status, phrase, msg);
    }
    else {
      answer_sent = 1;
      soa_activate(nh->nh_soa, NULL);
    }
  }
  /* When 100rel response status was 183 fake support for preconditions */
  else if (cr->cr_auto && cri->cr_status == 183 && ss->ss_precondition) {
    if (soa_generate_offer(nh->nh_soa, 0, NULL) < 0 ||
	session_include_description(nh->nh_soa, 1, msg, sip) < 0) {
      status = soa_error_as_sip_response(nh->nh_soa, &phrase);
      SU_DEBUG_3(("nua(%p): PRACK offer: %d %s\n", (void *)nh,
		  status, phrase));
      nua_stack_event(nh->nh_nua, nh, NULL,
		      nua_i_media_error, status, phrase, NULL);
      return nua_client_return(cr, status, phrase, msg);
    }
    else {
      offer_sent = 1;
    }
  }

  retval = nua_base_client_request(cr, msg, sip, NULL);

  if (retval == 0) {
    cr->cr_offer_sent = offer_sent;
    cr->cr_answer_sent = answer_sent;

    if (!cr->cr_restarting) {
      if (offer_sent) 
	ss->ss_oa_sent = "offer";
      else if (answer_sent)
	ss->ss_oa_sent = "answer";

      if (!ss->ss_reporting)
	signal_call_state_change(nh, ss, status, phrase, ss->ss_state);
    }
  }

  return retval;
}

static int nua_prack_client_response(nua_client_request_t *cr,
				     int status, char const *phrase,
				     sip_t const *sip)
{
  /* XXX - fatal error cases? */

  return nua_session_client_response(cr, status, phrase, sip);
}

static int nua_prack_client_report(nua_client_request_t *cr,
				   int status, char const *phrase,
				   sip_t const *sip,
				   nta_outgoing_t *orq,
				   tagi_t const *tags)
{
  nua_handle_t *nh = cr->cr_owner;
  nua_session_usage_t *ss = nua_dialog_usage_private(cr->cr_usage);

  nua_stack_event(nh->nh_nua, nh, 
		  nta_outgoing_getresponse(orq),
		  cr->cr_event,
		  status, phrase,
		  tags);

  if (!ss || orq != cr->cr_orq || cr->cr_terminated || cr->cr_graceful)
    return 1;

  if (cr->cr_offer_sent)
    signal_call_state_change(nh, ss, status, phrase, ss->ss_state);

  if (ss->ss_update_needed && 200 <= status && status < 300)
    nua_client_create(nh, nua_r_update, &nua_update_client_methods, NULL);
  
  return 1;
}

/* ---------------------------------------------------------------------- */
/* UAS side of INVITE */

/** @NUA_EVENT nua_i_invite
 *
 * Indication of incoming call or re-INVITE request. 
 *
 * @param status statuscode of response sent automatically by stack
 * @param phrase a short textual description of @a status code
 * @param nh     operation handle associated with this call
 *               (maybe created for this call)
 * @param hmagic application context associated with this call
 *               (maybe NULL if call handle was created for this call)
 * @param sip    incoming INVITE request
 * @param tags   SOATAG_ACTIVE_AUDIO(), SOATAG_ACTIVE_VIDEO()
 * 
 * @par Responding to INVITE with nua_respond()
 *
 * If @a status in #nua_i_invite event is below 200, the application should
 * accept or reject the call with nua_respond(). See the @ref nua_call_model
 * for the detailed explanation of various options in call processing at
 * server end.
 *
 * The @b INVITE request takes care of session setup using SDP Offer-Answer
 * negotiation as specified in @RFC3264 (updated in @RFC3262 section 5,
 * @RFC3311, and @RFC3312). The Offer-Answer can be taken care by
 * application (if NUTAG_MEDIA_ENABLE(0) parameter has been set) or by the
 * built-in SDP Offer/Answer engine @soa (by default and when
 * NUTAG_MEDIA_ENABLE(1) parameter has been set). When @soa is enabled, it
 * will take care of parsing the SDP, negotiating the media and codecs, and
 * including the SDP in the SIP message bodies as required by the
 * Offer-Answer model.
 *
 * When @soa is enabled, the SDP in the incoming INVITE is parsed and feed
 * to a #soa_session_t object. The #nua_i_state event sent to the
 * application immediately after #nua_i_invite will contain the parsing
 * results in SOATAG_REMOTE_SDP() and SOATAG_REMOTE_SDP_STR() tags.
 * 
 * Note that currently the parser within @nua does not handle MIME
 * multipart. The SDP Offer/Answer engine can get confused if the SDP offer
 * is included in a MIME multipart, therefore such an @b INVITE is rejected
 * with <i>415 Unsupported Media Type</i> error response: the client is
 * expected to retry the INVITE without MIME multipart content.
 *
 * If the call is to be accepted, the application should include the SDP in
 * the 2XX response. If @soa is not disabled with NUTAG_MEDIA_ENABLE(0), the
 * SDP should be included in the SOATAG_USER_SDP() or SOATAG_USER_SDP_STR()
 * parameter given to nua_respond(). If it is disabled, the SDP should be
 * included in the response message using SIPTAG_PAYLOAD() or
 * SIPTAG_PAYLOAD_STR(). Also, the @ContentType should be set using
 * SIPTAG_CONTENT_TYPE() or SIPTAG_CONTENT_TYPE_STR().
 *
 * @par Preliminary Responses and 100rel
 *
 * Call progress can be signaled with preliminary responses (with status
 * code in the range 101..199). It is possible to conclude the SDP
 * Offer-Answer negotiation using preliminary responses, too. If
 * NUTAG_EARLY_ANSWER(1), SOATAG_USER_SDP() or SOATAG_USER_SDP_STR()
 * parameter is included with in a preliminary nua_response(), the SDP
 * answer is generated and sent with the preliminary responses, too.
 *
 * The preliminary responses are sent reliably if feature tag "100rel" is
 * included in the @Require header of the response or if
 * NUTAG_EARLY_MEDIA(1) parameter has been given. The reliably delivery of
 * preliminary responses mean that a sequence number is included in the
 * @RSeq header in the response message and the response message is resent
 * until the client responds with a @b PRACK request with matching sequence
 * number in @RAck header.
 *
 * Note that only the "183" response is sent reliably if the
 * NUTAG_ONLY183_100REL(1) parameter has been given. The reliable
 * preliminary responses are acknowledged with @b PRACK request sent by the
 * client.
 *
 * Note if the SDP offer-answer is completed with the reliable preliminary
 * responses, the is no need to include SDP in 200 OK response (or other 2XX
 * response). However, it the tag NUTAG_INCLUDE_EXTRA_SDP(1) is included
 * with nua_respond(), a copy of the SDP answer generated earlier by @soa is
 * included as the message body.
 *
 * @sa nua_respond(), @ref nua_uas_call_model, #nua_i_state,
 * NUTAG_MEDIA_ENABLE(), SOATAG_USER_SDP(), SOATAG_USER_SDP_STR(),
 * @RFC3262, NUTAG_EARLY_ANSWER(), NUTAG_EARLY_MEDIA(), 
 * NUTAG_ONLY183_100REL(), 
 * NUTAG_INCLUDE_EXTRA_SDP(),
 * #nua_i_prack, #nua_i_update, nua_update(),
 * nua_invite(), #nua_r_invite
 *
 * @par Third Party Call Control
 *
 * When so called 2rd party call control is used, the initial @b INVITE may
 * not contain SDP offer. In that case, the offer is sent by the recipient
 * of the @b INVITE request (User-Agent Server, UAS). The SDP sent in 2XX
 * response (or in a preliminary reliable response) is considered as an
 * offer, and the answer will be included in the @b ACK request sent by the
 * UAC (or @b PRACK in case of preliminary reliable response).
 *
 * @sa @ref nua_3pcc_call_model
 *
 * @END_NUA_EVENT
 */

static int nua_invite_server_init(nua_server_request_t *sr);
static int nua_session_server_init(nua_server_request_t *sr);
static int nua_invite_server_preprocess(nua_server_request_t *sr);
static int nua_invite_server_respond(nua_server_request_t *sr, tagi_t const *);
static int nua_invite_server_is_100rel(nua_server_request_t *, tagi_t const *);
static int nua_invite_server_report(nua_server_request_t *sr, tagi_t const *);

static int
  process_ack_or_cancel(nua_server_request_t *, nta_incoming_t *, 
			sip_t const *),
  process_ack(nua_server_request_t *, nta_incoming_t *, sip_t const *),
  process_cancel(nua_server_request_t *, nta_incoming_t *, sip_t const *),
  process_timeout(nua_server_request_t *, nta_incoming_t *),
  process_prack(nua_handle_t *nh,
		nta_reliable_t *rel,
		nta_incoming_t *irq,
		sip_t const *sip);

nua_server_methods_t const nua_invite_server_methods = 
  {
    SIP_METHOD_INVITE,
    nua_i_invite,		/* Event */
    { 
      1,			/* Create dialog */
      0,			/* Initial request */
      1,			/* Target refresh request  */
      1,			/* Add Contact */
    },
    nua_invite_server_init,
    nua_invite_server_preprocess,
    nua_base_server_params,
    nua_invite_server_respond,
    nua_invite_server_report,
  };


/** @internal Preprocess incoming invite - sure we have a valid request. 
 * 
 * @return 0 if request is valid, or error statuscode otherwise
 */
static int
nua_invite_server_init(nua_server_request_t *sr)
{
  nua_handle_t *nh = sr->sr_owner;
  nua_t *nua = nh->nh_nua;

  if (!NUA_PGET(nua, nh, invite_enable))
    return SR_STATUS1(sr, SIP_403_FORBIDDEN);

  if (nua_session_server_init(sr))
    return sr->sr_status;
    
  if (sr->sr_usage) {
    /* Existing session - check for overlap and glare */ 

    nua_server_request_t const *sr0;
    nua_client_request_t const *cr;

    for (sr0 = nh->nh_ds->ds_sr; sr0; sr0 = sr0->sr_next) {
      /* Final response have not been sent to previous INVITE */
      if (sr0->sr_method == sip_method_invite && 
	  nua_server_request_is_pending(sr0))
	break;
      /* Or we have sent offer but have not received answer */
      if (sr->sr_sdp && sr0->sr_offer_sent && !sr0->sr_answer_recv)
	break;
      /* Or we have received request with offer but not sent answer */
      if (sr->sr_sdp && sr0->sr_offer_recv && !sr0->sr_answer_sent)
	break;
    }
    
    if (sr0)
      /* Overlapping invites - RFC 3261 14.2 */
      return nua_server_retry_after(sr, 500, "Overlapping Requests", 0, 10);

    for (cr = nh->nh_ds->ds_cr; cr; cr = cr->cr_next) {
      if (cr->cr_usage == sr->sr_usage && cr->cr_orq && cr->cr_offer_sent)
	/* Glare - RFC 3261 14.2 and RFC 3311 section 5.2 */
	return SR_STATUS1(sr, SIP_491_REQUEST_PENDING);
    }
  }

  return 0;
}

/** Initialize session server request.
 *
 * Ensure that the request is valid.
 */
static int
nua_session_server_init(nua_server_request_t *sr)
{
  nua_handle_t *nh = sr->sr_owner;
  nua_t *nua = nh->nh_nua;

  msg_t *msg = sr->sr_response.msg;
  sip_t *sip = sr->sr_response.sip;

  sip_t const *request = sr->sr_request.sip;

  unsigned min = NH_PGET(nh, min_se);

  if (!sr->sr_initial)
    sr->sr_usage = nua_dialog_usage_get(nh->nh_ds, nua_session_usage, NULL);

  if (sr->sr_method != sip_method_invite && sr->sr_usage == NULL) {
    /* UPDATE/PRACK sent within an existing dialog? */
    return SR_STATUS(sr, 481, "Call Does Not Exist");
  }

  if (nh->nh_soa) {
    sip_accept_t *a = nua->nua_invite_accept;

    /* XXX - soa should know what it supports */
    sip_add_dup(msg, sip, (sip_header_t *)a);

    /* Make sure caller uses application/sdp without compression */
    if (nta_check_session_content(NULL, request, a, TAG_END())) {
      sip_add_make(msg, sip, sip_accept_encoding_class, "");
      return SR_STATUS1(sr, SIP_415_UNSUPPORTED_MEDIA);
    }

    /* Make sure caller accepts application/sdp */
    if (nta_check_accept(NULL, request, a, NULL, TAG_END())) {
      sip_add_make(msg, sip, sip_accept_encoding_class, "");
      return SR_STATUS1(sr, SIP_406_NOT_ACCEPTABLE);
    }
  }

  if (request->sip_session_expires &&
      nta_check_session_expires(NULL, request, min, TAG_END())) {
    sip_min_se_t *min_se, min_se0[1];

    min_se = sip_min_se_init(min_se0);
    min_se->min_delta = min;
    
    if (request->sip_min_se && request->sip_min_se->min_delta > min)
      min_se = request->sip_min_se;

    sip_add_dup(msg, sip, (sip_header_t *)min_se);
    
    return SR_STATUS1(sr, SIP_422_SESSION_TIMER_TOO_SMALL);
  }

  session_get_description(sr->sr_request.sip, &sr->sr_sdp, &sr->sr_sdp_len);

  return 0;
}

/** Preprocess INVITE.
 *
 * This is called after a handle has been created for an incoming INVITE.
 */
int nua_invite_server_preprocess(nua_server_request_t *sr)
{
  nua_handle_t *nh = sr->sr_owner;
  nua_dialog_state_t *ds = nh->nh_ds;
  nua_session_usage_t *ss;

  sip_t const *request = sr->sr_request.sip;

  assert(sr->sr_status == 100);
  assert(nh != nh->nh_nua->nua_dhandle);

  if (sr->sr_status > 100)
    return sr->sr_status;

  if (nh->nh_soa) {
    soa_init_offer_answer(nh->nh_soa);

    if (sr->sr_sdp) {
      if (soa_set_remote_sdp(nh->nh_soa, NULL,
			     sr->sr_sdp, sr->sr_sdp_len) < 0) {
	SU_DEBUG_5(("nua(%p): %s server: error parsing SDP\n", (void *)nh,
		    "INVITE"));
	return SR_STATUS(sr, 400, "Bad Session Description");
      }
      else
	sr->sr_offer_recv = 1;
    }
  }

  /* Add the session usage */
  if (sr->sr_usage == NULL) {
    sr->sr_usage = nua_dialog_usage_add(nh, ds, nua_session_usage, NULL);
    if (sr->sr_usage == NULL)
      return SR_STATUS1(sr, SIP_500_INTERNAL_SERVER_ERROR);
  }

  ss = nua_dialog_usage_private(sr->sr_usage);

  if (sr->sr_offer_recv)
    ss->ss_oa_recv = "offer";

  ss->ss_100rel = NH_PGET(nh, early_media);
  ss->ss_precondition = sip_has_feature(request->sip_require, "precondition");
  if (ss->ss_precondition)
    ss->ss_100rel = 1;

  session_timer_preferences(ss, 
			    NH_PGET(nh, session_timer),
			    NH_PGET(nh, min_se),
			    NH_PGET(nh, refresher));

  /* Session Timer negotiation */
  if (sip_has_supported(NH_PGET(nh, supported), "timer"))
    init_session_timer(ss, request, ss->ss_refresher);

  assert(ss->ss_state >= nua_callstate_ready ||
	 ss->ss_state == nua_callstate_init);

  if (NH_PGET(nh, auto_answer) ||
      /* Auto-answer to re-INVITE unless auto_answer is set to 0 on handle */
      (ss->ss_state == nua_callstate_ready &&
       /* Auto-answer requires enabled media (soa). 
	* XXX - if the re-INVITE modifies the media we should not auto-answer.
	*/
       nh->nh_soa &&
       !NH_PISSET(nh, auto_answer))) {
    SR_STATUS1(sr, SIP_200_OK);
  }
  else if (NH_PGET(nh, auto_alert)) {
    if (ss->ss_100rel &&
	(sip_has_feature(request->sip_supported, "100rel") ||
	 sip_has_feature(request->sip_require, "100rel"))) {
      SR_STATUS1(sr, SIP_183_SESSION_PROGRESS);
    }
    else {
      SR_STATUS1(sr, SIP_180_RINGING);
    }
  }

  return 0;
}


/** @internal Respond to an INVITE request.
 *
 */
static
int nua_invite_server_respond(nua_server_request_t *sr, tagi_t const *tags)
{
  nua_handle_t *nh = sr->sr_owner;
  nua_dialog_usage_t *du = sr->sr_usage;
  nua_session_usage_t *ss = nua_dialog_usage_private(du);
  msg_t *msg = sr->sr_response.msg; 
  sip_t *sip = sr->sr_response.sip; 

  int reliable = 0, offer = 0, answer = 0, early_answer = 0, extra = 0;

  enter;

  if (du == NULL) {
    if (sr->sr_status < 300)
      sr_status(sr, SIP_500_INTERNAL_SERVER_ERROR);
    return nua_base_server_respond(sr, tags);
  }

  if (nua_invite_server_is_100rel(sr, tags)) {
    reliable = 1, early_answer = 1;
  }
  else if (!nh->nh_soa || sr->sr_status >= 300) {
    
  }
  else if (tags && 100 < sr->sr_status && sr->sr_status < 200 && 
      !NHP_ISSET(nh->nh_prefs, early_answer)) {
    sdp_session_t const *user_sdp = NULL;
    char const *user_sdp_str = NULL;

    tl_gets(tags,
	    SOATAG_USER_SDP_REF(user_sdp),
	    SOATAG_USER_SDP_STR_REF(user_sdp_str),
	    TAG_END());

    early_answer = user_sdp || user_sdp_str;
  }
  else {
    early_answer = NH_PGET(nh, early_answer);
  }

  if (!nh->nh_soa) {
    /* Xyzzy */
  }
  else if (sr->sr_status >= 300) {
    soa_clear_remote_sdp(nh->nh_soa);
  }
  else if (sr->sr_offer_sent && !sr->sr_answer_recv)
    /* Wait for answer */;
  else if (sr->sr_offer_recv && sr->sr_answer_sent > 1) {
    /* We have sent answer */
    /* ...  but we may want to send it again */
    tagi_t const *t = tl_find_last(tags, nutag_include_extra_sdp);
    extra = t && t->t_value;
  }
  else if (sr->sr_offer_recv && !sr->sr_answer_sent && early_answer) {
    /* Generate answer */ 
    if (soa_generate_answer(nh->nh_soa, NULL) >= 0) {
      answer = 1;
      soa_activate(nh->nh_soa, NULL);
      /* signal that O/A answer sent (answer to invite) */
    }
    else if (sr->sr_status >= 200) {
      sip_warning_t *warning = NULL;
      int wcode;
      char const *text;
      char const *host = "invalid.";
      
      sr->sr_status = soa_error_as_sip_response(nh->nh_soa, &sr->sr_phrase);
      
      wcode = soa_get_warning(nh->nh_soa, &text);
      
      if (wcode) {
	if (sip->sip_contact)
	  host = sip->sip_contact->m_url->url_host;
	warning = sip_warning_format(msg_home(msg), "%u %s \"%s\"",
				     wcode, host, text);
	sip_header_insert(msg, sip, (sip_header_t *)warning);
      }
    }
    else {
      /* 1xx - we don't have to send answer */
    }
  }
  else if (sr->sr_offer_recv && sr->sr_answer_sent == 1 && early_answer) {
    /* The answer was sent unreliably, keep sending it */
    answer = 1;
  }
  else if (!sr->sr_offer_recv && !sr->sr_offer_sent && reliable) {
    /* Generate offer */
    if (soa_generate_offer(nh->nh_soa, 0, NULL) < 0)
      sr->sr_status = soa_error_as_sip_response(nh->nh_soa, &sr->sr_phrase);
    else
      offer = 1;
  }

  if (sr->sr_status < 300 && (offer || answer || extra)) {
    if (session_include_description(nh->nh_soa, 1, msg, sip) < 0)
      SR_STATUS1(sr, SIP_500_INTERNAL_SERVER_ERROR);
    else if (offer)
      sr->sr_offer_sent = 1, ss->ss_oa_sent = "offer";
    else if (answer)
      sr->sr_answer_sent = 1 + reliable, ss->ss_oa_sent = "answer";
  }

  if (reliable && sr->sr_status < 200) {
    sr->sr_response.msg = NULL, sr->sr_response.sip = NULL;
    if (nta_reliable_mreply(sr->sr_irq, process_prack, nh, msg) == NULL)
      return -1;
    return 0;
  }

  if (ss->ss_refresher && 200 <= sr->sr_status && sr->sr_status < 300)
    if (session_timer_is_supported(nh))
      use_session_timer(ss, 1, 1, msg, sip);

  return nua_base_server_respond(sr, tags);  
}

/** Check if the response should be sent reliably.
 * XXX - use tags to indicate when to use reliable responses ???
 */
static
int nua_invite_server_is_100rel(nua_server_request_t *sr, tagi_t const *tags)
{
  nua_handle_t *nh = sr->sr_owner;
  sip_t const *sip = sr->sr_response.sip;
  sip_require_t *require = sr->sr_request.sip->sip_require;
  sip_supported_t *supported = sr->sr_request.sip->sip_supported;

  if (sr->sr_status >= 200)
    return 1;
  else if (sr->sr_status == 100)
    return 0;

  if (sip_has_feature(sip->sip_require, "100rel"))
    return 1;

  if (require == NULL && supported == NULL)
    return 0;

  if (sip_has_feature(require, "100rel"))
    return 1;
  if (!sip_has_feature(supported, "100rel"))
    return 0;
  if (sr->sr_status == 183)
    return 1;

  if (NH_PGET(nh, early_media) && !NH_PGET(nh, only183_100rel))
    return 1;

  if (sip_has_feature(require, "precondition")) {
    if (!NH_PGET(nh, only183_100rel))
      return 1;
    if (sr->sr_offer_recv && !sr->sr_answer_sent)
      return 1;
  }

  return 0;
}


int nua_invite_server_report(nua_server_request_t *sr, tagi_t const *tags)
{
  nua_handle_t *nh = sr->sr_owner;
  nua_dialog_usage_t *du = sr->sr_usage;
  nua_session_usage_t *ss = nua_dialog_usage_private(sr->sr_usage);
  int initial = sr->sr_initial && !sr->sr_event;
  int application = sr->sr_application;
  int status = sr->sr_status; char const *phrase = sr->sr_phrase;
  int retval;

  if (!sr->sr_event && status < 300) {	/* Not reported yet */
    nta_incoming_bind(sr->sr_irq, process_ack_or_cancel, sr);
  }

  retval = nua_base_server_report(sr, tags), sr = NULL; /* destroys sr */
  
  if (retval >= 2 || ss == NULL) {
    /* Session has been terminated. */ 
    if (!initial)
      signal_call_state_change(nh, NULL, status, phrase,
			       nua_callstate_terminated);
    return retval;
  }

  assert(ss);
  assert(ss->ss_state != nua_callstate_calling);
  assert(ss->ss_state != nua_callstate_proceeding);

  /* Update session state */
  if (status < 300 || application != 0)
    signal_call_state_change(nh, ss, status, phrase,
			     status >= 300
			     ? nua_callstate_init
			     : status >= 200
			     ? nua_callstate_completed
			     : status > 100
			     ? nua_callstate_early
			     : nua_callstate_received);

  if (status == 180)
    ss->ss_alerting = 1;
  else if (status >= 200)
    ss->ss_alerting = 0;

  if (200 <= status && status < 300) {
     du->du_ready = 1;
  }
  else if (300 <= status) {
    if (nh->nh_soa)
      soa_init_offer_answer(nh->nh_soa);
  }

  if (ss->ss_state == nua_callstate_init) {
    assert(status >= 300);
    nua_session_usage_destroy(nh, ss);
  }

  return retval;
}

/** @internal Process ACK or CANCEL or timeout (no ACK) for incoming INVITE */
static
int process_ack_or_cancel(nua_server_request_t *sr,
			  nta_incoming_t *irq,
			  sip_t const *sip)
{
  enter;

  assert(sr->sr_usage);
  assert(sr->sr_usage->du_class == nua_session_usage);

  if (sip && sip->sip_request->rq_method == sip_method_ack)
    return process_ack(sr, irq, sip);
  else if (sip && sip->sip_request->rq_method == sip_method_cancel)
    return process_cancel(sr, irq, sip);
  else
    return process_timeout(sr, irq);
}

/** @NUA_EVENT nua_i_ack
 *
 * Final response to INVITE has been acknowledged by UAC with ACK. 
 * 
 * @note This event is only sent after 2XX response.
 *
 * @param nh     operation handle associated with the call
 * @param hmagic application context associated with the call
 * @param sip    incoming ACK request
 * @param tags   empty
 *
 * @sa #nua_i_invite, #nua_i_state, @ref nua_uas_call_model, nua_ack()
 * 
 * @END_NUA_EVENT
 */

int process_ack(nua_server_request_t *sr,
		nta_incoming_t *irq,
		sip_t const *sip)
{
  nua_handle_t *nh = sr->sr_owner;
  nua_session_usage_t *ss = nua_dialog_usage_private(sr->sr_usage);
  msg_t *msg = nta_incoming_getrequest_ackcancel(irq);
  char const *recv = NULL;

  if (ss == NULL)
    return 0;

  if (nh->nh_soa && sr->sr_offer_sent && !sr->sr_answer_recv) {
    char const *sdp;
    size_t len;
    int error;

    if (!session_get_description(sip, &sdp, &len) ||
	!(recv = "answer") ||
	soa_set_remote_sdp(nh->nh_soa, NULL, sdp, len) < 0 ||
	soa_process_answer(nh->nh_soa, NULL) < 0 ||
	soa_activate(nh->nh_soa, NULL)) {
      int status; char const *phrase, *reason;

      status = soa_error_as_sip_response(nh->nh_soa, &phrase);
      reason = soa_error_as_sip_reason(nh->nh_soa);

      nua_stack_event(nh->nh_nua, nh, msg,
		      nua_i_ack, status, phrase, NULL);
      nua_stack_event(nh->nh_nua, nh, NULL,
		      nua_i_media_error, status, phrase, NULL);

      assert(ss->ss_oa_recv == NULL);

      ss->ss_oa_recv = recv;

      ss->ss_reporting = 1;	/* We report state here if BYE fails */
      error = nua_client_create(nh, nua_r_bye, &nua_bye_client_methods, NULL);
      ss->ss_reporting = 0;

      signal_call_state_change(nh, ss, 488, "Offer-Answer Error",
			       error
			       ? nua_callstate_terminated
			       : nua_callstate_terminating);

      return 0;
    }
  }

  soa_clear_remote_sdp(nh->nh_soa);
  nua_stack_event(nh->nh_nua, nh, msg, nua_i_ack, SIP_200_OK, NULL);
  signal_call_state_change(nh, ss, 200, "OK", nua_callstate_ready);
  set_session_timer(ss);

  nua_server_request_destroy(sr);

  return 0;
}

/** @NUA_EVENT nua_i_cancel
 *
 * Incoming INVITE has been cancelled by the client.
 *
 * @param status status code of response to CANCEL sent automatically by stack
 * @param phrase a short textual description of @a status code
 * @param nh     operation handle associated with the call
 * @param hmagic application context associated with the call
 * @param sip    incoming CANCEL request
 * @param tags   empty
 *
 * @sa @ref nua_uas_call_model, nua_cancel(), #nua_i_invite, #nua_i_state
 *
 * @END_NUA_EVENT
 */

/* CANCEL  */
static
int process_cancel(nua_server_request_t *sr,
		   nta_incoming_t *irq,
		   sip_t const *sip)
{
  nua_handle_t *nh = sr->sr_owner;
  nua_session_usage_t *ss = nua_dialog_usage_private(sr->sr_usage);
  msg_t *cancel = nta_incoming_getrequest_ackcancel(irq);

  if (nta_incoming_status(irq) < 200 && nua_server_request_is_pending(sr) &&
	  ss && (ss == nua_session_usage_get(nh->nh_ds))) {
    nua_stack_event(nh->nh_nua, nh, cancel, nua_i_cancel, SIP_200_OK, NULL);

  SR_STATUS1(sr, SIP_487_REQUEST_TERMINATED);

    nua_server_respond(sr, NULL);
    nua_server_report(sr);
  }

  return 0;
}

/* Timeout (no ACK or PRACK received) */
static
int process_timeout(nua_server_request_t *sr,
		    nta_incoming_t *irq)
{
  nua_handle_t *nh = sr->sr_owner;
  nua_session_usage_t *ss = nua_dialog_usage_private(sr->sr_usage);
  char const *phrase = "ACK Timeout";
  char const *reason = "SIP;cause=408;text=\"ACK Timeout\"";
  int error;

  assert(ss); assert(ss == nua_session_usage_get(nh->nh_ds));

  if (nua_server_request_is_pending(sr)) {
    phrase = "PRACK Timeout";
    reason = "SIP;cause=504;text=\"PRACK Timeout\"";
  }

  nua_stack_event(nh->nh_nua, nh, 0, nua_i_error, 408, phrase, NULL);

  if (nua_server_request_is_pending(sr)) {
    /* PRACK timeout */
    SR_STATUS1(sr, SIP_504_GATEWAY_TIME_OUT);
    nua_server_trespond(sr, 
			SIPTAG_REASON_STR(reason),
			TAG_END());
    if (nua_server_report(sr) >= 2)
      return 0;			/* Done */
    sr = NULL;
  }

  /* send BYE, too, if 200 OK (or 183 to re-INVITE) timeouts  */
  ss->ss_reason = reason;

  ss->ss_reporting = 1;		/* We report state here if BYE fails */
  error = nua_client_create(nh, nua_r_bye, &nua_bye_client_methods, NULL);
  ss->ss_reporting = 0;

  signal_call_state_change(nh, ss, 0, phrase,
			   error
			   ? nua_callstate_terminated
			   : nua_callstate_terminating);

  if (sr)
    nua_server_request_destroy(sr);

  return 0;
}


/** @NUA_EVENT nua_i_prack
 *
 * Incoming PRACK request. PRACK request is used to acknowledge reliable
 * preliminary responses and it is usually sent automatically by the nua
 * stack.
 *
 * @param status status code of response sent automatically by stack
 * @param phrase a short textual description of @a status code
 * @param nh     operation handle associated with the call
 * @param hmagic application context associated with the call
 * @param sip    incoming PRACK request
 * @param tags   empty
 *
 * @sa nua_prack(), #nua_r_prack, @RFC3262, NUTAG_EARLY_MEDIA()
 * 
 * @END_NUA_EVENT
 */

int nua_prack_server_init(nua_server_request_t *sr);
int nua_prack_server_preprocess(nua_server_request_t *sr);
int nua_prack_server_respond(nua_server_request_t *sr, tagi_t const *tags);
int nua_prack_server_report(nua_server_request_t *sr, tagi_t const *tags);

nua_server_methods_t const nua_prack_server_methods = 
  {
    SIP_METHOD_PRACK,
    nua_i_prack,		/* Event */
    { 
      0,			/* Do not create dialog */
      1,			/* In-dialog request */
      1,			/* Target refresh request  */
      1,			/* Add Contact */
    },
    nua_prack_server_init,
    nua_prack_server_preprocess,
    nua_base_server_params,
    nua_prack_server_respond,
    nua_prack_server_report,
  };

/** @internal Process reliable response PRACK or (timeout from 100rel) */
static int process_prack(nua_handle_t *nh,
			 nta_reliable_t *rel,
			 nta_incoming_t *irq,
			 sip_t const *sip)
{
  nua_dialog_state_t *ds = nh->nh_ds;
  nua_dialog_usage_t *du;
  nua_server_request_t *sr;

  nta_reliable_destroy(rel);
  if (irq == NULL)  
    /* Final response interrupted 100rel, we did not actually receive PRACK */
    return 200;

  if (!nh->nh_ds->ds_leg)
    return 481;

  du = nua_dialog_usage_for_session(ds);

  for (sr = ds->ds_sr; sr; sr = sr->sr_next) {
    if (sr->sr_method == sip_method_invite && sr->sr_usage == du)
      break;
  }

  if (!nua_server_request_is_pending(sr)) /* There is no INVITE */
    return 481;

  if (sip == NULL) {
    /* 100rel timeout */
    SR_STATUS(sr, 504, "Reliable Response Timeout");
    nua_stack_event(nh->nh_nua, nh, NULL, nua_i_error,
		    sr->sr_status, sr->sr_phrase,
		    NULL);
    nua_server_trespond(sr,
			SIPTAG_REASON_STR("SIP;cause=504;"
					  "text=\"PRACK Timeout\""),
			TAG_END());
    nua_server_report(sr);
    return 504;
  }

  nta_incoming_bind(irq, NULL, (void *)sr);

  return nua_stack_process_request(nh, nh->nh_ds->ds_leg, irq, sip);
}


int nua_prack_server_init(nua_server_request_t *sr)
{
  nua_handle_t *nh = sr->sr_owner;
  nua_server_request_t *sri = nta_incoming_magic(sr->sr_irq, NULL);

  if (sri == NULL)
    return SR_STATUS(sr, 481, "No Such Preliminary Response");
  
  if (nua_session_server_init(sr))
    return sr->sr_status;

  if (sr->sr_sdp) {
    nua_session_usage_t *ss = nua_dialog_usage_private(sr->sr_usage);

    /* XXX - check for overlap? */
    
    if (sri->sr_offer_sent)
      sr->sr_answer_recv = 1, ss->ss_oa_recv = "answer";
    else 
      sr->sr_offer_recv = 1, ss->ss_oa_recv = "offer";

    if (nh->nh_soa &&
	soa_set_remote_sdp(nh->nh_soa, NULL, sr->sr_sdp, sr->sr_sdp_len) < 0) {
      SU_DEBUG_5(("nua(%p): %s server: error parsing %s\n", (void *)nh,
		  "PRACK", "offer"));
      return 
	sr->sr_status = soa_error_as_sip_response(nh->nh_soa, &sr->sr_phrase);
    }
  }

  return 0;
}

int nua_prack_server_preprocess(nua_server_request_t *sr)
{
  return sr_status(sr, SIP_200_OK); /* For now */
}

int nua_prack_server_respond(nua_server_request_t *sr, tagi_t const *tags)
{
  nua_handle_t *nh = sr->sr_owner;

  if (sr->sr_status < 200 || 300 <= sr->sr_status) 
    return nua_base_server_respond(sr, tags);

  if (nh->nh_soa && sr->sr_sdp) {
    nua_session_usage_t *ss = nua_dialog_usage_private(sr->sr_usage);
    msg_t *msg = sr->sr_response.msg;
    sip_t *sip = sr->sr_response.sip;

    if ((sr->sr_offer_recv && soa_generate_answer(nh->nh_soa, NULL) < 0) ||
	(sr->sr_answer_recv && soa_process_answer(nh->nh_soa, NULL) < 0)) {
      SU_DEBUG_5(("nua(%p): %s server: %s %s\n", 
		  (void *)nh, "PRACK", 
		  "error processing",
		  sr->sr_offer_recv ? "offer" : "answer"));
      sr->sr_status = soa_error_as_sip_response(nh->nh_soa, &sr->sr_phrase);
    }
    else if (sr->sr_offer_recv) {
      if (session_include_description(nh->nh_soa, 1, msg, sip) < 0)
	sr_status(sr, SIP_500_INTERNAL_SERVER_ERROR);
      else
      sr->sr_answer_sent = 1, ss->ss_oa_sent = "answer";
    }
  }

  return nua_base_server_respond(sr, tags);
}

int nua_prack_server_report(nua_server_request_t *sr, tagi_t const *tags)
{
  nua_handle_t *nh = sr->sr_owner;
  nua_session_usage_t *ss = nua_dialog_usage_private(sr->sr_usage);
  int retval = nua_base_server_report(sr, tags); /* destroys sr */

  if (retval >= 2 || ss == NULL) {
    signal_call_state_change(nh, NULL,
			     sr->sr_status, sr->sr_phrase, 
			     nua_callstate_terminated);
    return retval;
  }

  if (sr->sr_offer_recv || sr->sr_answer_sent) {
    /* signal offer received, answer sent */
    signal_call_state_change(nh, ss,
			     sr->sr_status, sr->sr_phrase, 
			     ss->ss_state);
    soa_activate(nh->nh_soa, NULL);
  }

  if (200 <= sr->sr_status && sr->sr_status < 300
      && ss->ss_state < nua_callstate_ready
      && !ss->ss_alerting
      && !ss->ss_precondition
      && NH_PGET(nh, auto_alert))  {
    nua_server_request_t *sri;
    
    for (sri = nh->nh_ds->ds_sr; sri; sri = sri->sr_next)
      if (sri->sr_method == sip_method_invite && 
	  nua_server_request_is_pending(sri))
	break;

    if (sri) {
      SR_STATUS1(sri, SIP_180_RINGING);
      nua_server_respond(sri, NULL);
      nua_server_report(sri);
    }
  }

  return retval;
}

/* ---------------------------------------------------------------------- */
/* Session timer - RFC 4028 */

static int session_timer_is_supported(nua_handle_t const *nh)
{
  /* Is timer feature supported? */
  return sip_has_supported(NH_PGET(nh, supported), "timer");
}

static int prefer_session_timer(nua_handle_t const *nh)
{
  return 
    NH_PGET(nh, refresher) != nua_no_refresher || 
    NH_PGET(nh, session_timer) != 0;
}

/* Initialize session timer */ 
static
void session_timer_preferences(nua_session_usage_t *ss,
			       unsigned expires,
			       unsigned min_se,
			       enum nua_session_refresher refresher)
{
  if (expires < min_se)
    expires = min_se;
  if (refresher && expires == 0)
    expires = 3600;

  ss->ss_min_se = min_se;
  ss->ss_session_timer = expires;
  ss->ss_refresher = refresher;
}


/** Add timer featuretag and Session-Expires/Min-SE headers */
static int
use_session_timer(nua_session_usage_t *ss, int uas, int always,
		  msg_t *msg, sip_t *sip)
{
  sip_min_se_t min_se[1];
  sip_session_expires_t session_expires[1];

  static sip_param_t const x_params_uac[] = {"refresher=uac", NULL};
  static sip_param_t const x_params_uas[] = {"refresher=uas", NULL};

  /* Session-Expires timer */
  if (ss->ss_refresher == nua_no_refresher && !always)
    return 0;

  sip_min_se_init(min_se)->min_delta = ss->ss_min_se;
  sip_session_expires_init(session_expires)->x_delta = ss->ss_session_timer;

  if (ss->ss_refresher == nua_remote_refresher)
    session_expires->x_params = uas ? x_params_uac : x_params_uas;
  else if (ss->ss_refresher == nua_local_refresher)
    session_expires->x_params = uas ? x_params_uas : x_params_uac;

  sip_add_tl(msg, sip,
	     TAG_IF(ss->ss_session_timer,
		    SIPTAG_SESSION_EXPIRES(session_expires)),
	     TAG_IF(ss->ss_min_se != 0
		    /* Min-SE: 0 is optional with initial INVITE */
		    || ss->ss_state != nua_callstate_init,
		    SIPTAG_MIN_SE(min_se)),
	     TAG_IF(ss->ss_refresher == nua_remote_refresher,
		    SIPTAG_REQUIRE_STR("timer")),
	     TAG_END());

  return 1;
}

static int
init_session_timer(nua_session_usage_t *ss,
		   sip_t const *sip,
		   int refresher)
{
  int server;

  /* Session timer is not needed */
  if (!sip->sip_session_expires) {
    if (!sip_has_supported(sip->sip_supported, "timer"))
      ss->ss_refresher = nua_local_refresher;
    return 0;
  }

  ss->ss_refresher = nua_no_refresher;
  ss->ss_session_timer = sip->sip_session_expires->x_delta;

  if (sip->sip_min_se != NULL
      && sip->sip_min_se->min_delta > ss->ss_min_se)
    ss->ss_min_se = sip->sip_min_se->min_delta;

  server = sip->sip_request != NULL;

  if (!sip_has_supported(sip->sip_supported, "timer"))
    ss->ss_refresher = nua_local_refresher;
  else if (!str0casecmp("uac", sip->sip_session_expires->x_refresher))
    ss->ss_refresher = server ? nua_remote_refresher : nua_local_refresher;
  else if (!str0casecmp("uas", sip->sip_session_expires->x_refresher))
    ss->ss_refresher = server ? nua_local_refresher : nua_remote_refresher;
  else if (!server)
    return 0;			/* XXX */
  /* User preferences */
  else if (refresher == nua_local_refresher)
    ss->ss_refresher = nua_local_refresher;
  else
    ss->ss_refresher = nua_remote_refresher;

  SU_DEBUG_7(("nua session: session expires in %u refreshed by %s (%s %s)\n",
	      ss->ss_session_timer,
	      ss->ss_refresher == nua_local_refresher ? "local" : "remote",
	      server ? sip->sip_request->rq_method_name : "response to",
	      server ? "request" : sip->sip_cseq->cs_method_name));

  return 1;
}

static int session_timer_check_restart(nua_client_request_t *cr,
				       int status, char const *phrase,
				       sip_t const *sip)
{
  if (cr->cr_usage && status == 422) {
    nua_session_usage_t *ss = nua_dialog_usage_private(cr->cr_usage);

    if (sip->sip_min_se && ss->ss_min_se < sip->sip_min_se->min_delta)
      ss->ss_min_se = sip->sip_min_se->min_delta;
    if (ss->ss_min_se > ss->ss_session_timer)
      ss->ss_session_timer = ss->ss_min_se;
  
    return nua_client_restart(cr, 100, "Re-Negotiating Session Timer");
  }

  return nua_base_client_check_restart(cr, status, phrase, sip);
}

static void
set_session_timer(nua_session_usage_t *ss)
{
  nua_dialog_usage_t *du = nua_dialog_usage_public(ss);

  if (ss == NULL)
    return;

  if (ss->ss_refresher == nua_local_refresher) {
    ss->ss_timer_set = 1;
    nua_dialog_usage_set_expires(du, ss->ss_session_timer);
  }
  else if (ss->ss_refresher == nua_remote_refresher) {
    ss->ss_timer_set = 1;
    nua_dialog_usage_set_expires(du, ss->ss_session_timer + 32);
    nua_dialog_usage_reset_refresh(du);
  }
  else {
    ss->ss_timer_set = 0;
    nua_dialog_usage_set_expires(du, UINT_MAX);
    nua_dialog_usage_reset_refresh(du);
  }
}

static inline int
is_session_timer_set(nua_session_usage_t *ss)
{
  return ss->ss_timer_set;
}

/* ---------------------------------------------------------------------- */
/* Automatic notifications from a referral */

static int
nh_referral_check(nua_handle_t *nh, tagi_t const *tags)
{
  sip_event_t const *event = NULL;
  int pause = 1;
  struct nua_referral *ref = nh->nh_referral;
  nua_handle_t *ref_handle = ref->ref_handle;

  if (!ref_handle
      &&
      tl_gets(tags,
	      NUTAG_NOTIFY_REFER_REF(ref_handle),
	      NUTAG_REFER_EVENT_REF(event),
	      NUTAG_REFER_PAUSE_REF(pause),
	      TAG_END()) == 0
      &&
      tl_gets(nh->nh_tags,
	      NUTAG_NOTIFY_REFER_REF(ref_handle),
	      NUTAG_REFER_EVENT_REF(event),
	      NUTAG_REFER_PAUSE_REF(pause),
	      TAG_END()) == 0)
    return 0;

  if (!ref_handle)
    return 0;

  /* Remove nh_referral and nh_notevent */
  tl_tremove(nh->nh_tags,
	     NUTAG_NOTIFY_REFER(ref_handle),
	     TAG_IF(event, NUTAG_REFER_EVENT(event)),
	     TAG_END());

  if (event)
    ref->ref_event = sip_event_dup(nh->nh_home, event);

  if (!nh_validate(nh->nh_nua, ref_handle)) {
    SU_DEBUG_3(("nua: invalid NOTIFY_REFER handle\n"));
    return -1;
  }
  else if (!ref->ref_event) {
    SU_DEBUG_3(("nua: NOTIFY event missing\n"));
    return -1;
  }

  if (ref_handle != ref->ref_handle) {
    if (ref->ref_handle)
      nua_handle_unref(ref->ref_handle);
    ref->ref_handle = nua_handle_ref(ref_handle);
  }

#if 0
  if (pause) {
    /* Pause media on REFER handle */
    nmedia_pause(nua, ref_handle->nh_nm, NULL);
  }
#endif

  return 0;
}


static void
nh_referral_respond(nua_handle_t *nh, int status, char const *phrase)
{
  char payload[128];
  char const *substate;
  struct nua_referral *ref = nh->nh_referral;

  if (!nh_validate(nh->nh_nua, ref->ref_handle)) {
    if (ref) {
      if (ref->ref_handle)
	SU_DEBUG_1(("nh_handle_referral: stale referral handle %p\n",
		    (void *)ref->ref_handle));
      ref->ref_handle = NULL;
    }
    return;
  }

  /* XXX - we should have a policy here whether to send 101..199 */

  assert(ref->ref_event);

  if (status >= 300)
    status = 503, phrase = sip_503_Service_unavailable;

  snprintf(payload, sizeof(payload), "SIP/2.0 %03u %s\r\n", status, phrase);

  if (status < 200)
    substate = "active";
  else
    substate = "terminated ;reason=noresource";

  nua_stack_post_signal(ref->ref_handle,
			nua_r_notify,
			SIPTAG_EVENT(ref->ref_event),
			SIPTAG_SUBSCRIPTION_STATE_STR(substate),
			SIPTAG_CONTENT_TYPE_STR("message/sipfrag"),
			SIPTAG_PAYLOAD_STR(payload),
			TAG_END());

  if (status < 200)
    return;

  su_free(nh->nh_home, ref->ref_event), ref->ref_event = NULL;

  nua_handle_unref(ref->ref_handle), ref->ref_handle = NULL;
}

/* ======================================================================== */
/* INFO */

/**@fn void nua_info(nua_handle_t *nh, tag_type_t tag, tag_value_t value, ...);
 *
 * Send an INFO request. 
 *
 * INFO is used to send call related information like DTMF 
 * digit input events. See @RFC2976.
 *
 * @param nh              Pointer to operation handle
 * @param tag, value, ... List of tagged parameters
 *
 * @return 
 *    nothing
 *
 * @par Related Tags:
 *    Tags in <sip_tag.h>.
 *
 * @par Events:
 *    #nua_r_info
 *
 * @sa #nua_i_info
 */

static int nua_info_client_init(nua_client_request_t *cr, 
				msg_t *msg, sip_t *sip,
				tagi_t const *tags);

static int nua_info_client_request(nua_client_request_t *cr,
				   msg_t *msg, sip_t *sip,
				   tagi_t const *tags);

nua_client_methods_t const nua_info_client_methods = {
  SIP_METHOD_INFO,
  0,
  { 
    /* create_dialog */ 0,
    /* in_dialog */ 1,
    /* target refresh */ 0
  },
  /*nua_info_client_template*/ NULL,
  nua_info_client_init,
  nua_info_client_request,
  /*nua_info_client_check_restart*/ NULL,
  /*nua_info_client_response*/ NULL
};

int
nua_stack_info(nua_t *nua, nua_handle_t *nh, nua_event_t e, tagi_t const *tags)
{
  return nua_client_create(nh, e, &nua_info_client_methods, tags);
}

static int nua_info_client_init(nua_client_request_t *cr, 
				msg_t *msg, sip_t *sip,
				tagi_t const *tags)
{
  nua_handle_t *nh = cr->cr_owner;
  nua_dialog_usage_t *du = nua_dialog_usage_for_session(nh->nh_ds);
  nua_session_usage_t *ss = nua_dialog_usage_private(du);

  if (!ss || ss->ss_state >= nua_callstate_terminating)
    return nua_client_return(cr, 900, "Invalid handle for INFO", msg);

  cr->cr_usage = du;

  return 0;
}

static int nua_info_client_request(nua_client_request_t *cr,
				   msg_t *msg, sip_t *sip,
				   tagi_t const *tags)
{
  if (cr->cr_usage == NULL)
    return nua_client_return(cr, SIP_481_NO_TRANSACTION, msg);
  else
    return nua_base_client_request(cr, msg, sip, tags);
}

/** @NUA_EVENT nua_r_info
 *
 * Response to an outgoing @b INFO request.
 *
 * @param status response status code
 *               (if the request is retried, @a status is 100, the @a
 *               sip->sip_status->st_status contain the real status code
 *               from the response message, e.g., 302, 401, or 407)
 * @param phrase a short textual description of @a status code
 * @param nh     operation handle associated with the call
 * @param hmagic application context associated with the call
 * @param sip    response to @b INFO or NULL upon an error
 *               (status code is in @a status and 
 *                descriptive message in @a phrase parameters)
 * @param tags   empty
 *
 * @sa nua_info(), #nua_i_info, @RFC2976
 *
 * @END_NUA_EVENT
 */

/** @NUA_EVENT nua_i_info
 *
 * Incoming session INFO request.
 *
 * @param status statuscode of response sent automatically by stack
 * @param phrase a short textual description of @a status code
 * @param nh     operation handle associated with the call
 * @param hmagic application context associated with the call
 * @param sip    incoming INFO request
 * @param tags   empty
 *
 * @sa nua_info(), #nua_r_info, @RFC2976
 * 
 * @END_NUA_EVENT
 */

nua_server_methods_t const nua_info_server_methods = 
  {
    SIP_METHOD_INFO,
    nua_i_info,			/* Event */
    { 
      0,			/* Do not create dialog */
      1,			/* In-dialog request */
      0,			/* Not a target refresh request  */
      0,			/* Do not add Contact */
    },
    nua_base_server_init,
    nua_base_server_preprocess,
    nua_base_server_params,
    nua_base_server_respond,
    nua_base_server_report,
  };

/* ======================================================================== */
/* UPDATE */

/**@fn void nua_update(nua_handle_t *nh, tag_type_t tag, tag_value_t value, ...);
 *
 * Update a session. 
 * 
 * Update a session using SIP UPDATE method. See @RFC3311.
 *
 * Update method can be used when the session has been established with
 * INVITE. It's mainly used during the session establishment when
 * preconditions are used (@RFC3312). It can be also used during the call if
 * no user input is needed for offer/answer negotiation.
 *
 * @param nh              Pointer to operation handle
 * @param tag, value, ... List of tagged parameters
 *
 * @return 
 *    nothing
 *
 * @par Related Tags:
 *    same as nua_invite()
 *
 * @par Events:
 *    #nua_r_update \n
 *    #nua_i_state (#nua_i_active, #nua_i_terminated)\n
 *    #nua_i_media_error \n
 *
 * @sa @ref nua_call_model, @RFC3311, nua_update(), #nua_i_update
 */

static int nua_update_client_init(nua_client_request_t *cr, 
				  msg_t *msg, sip_t *sip,
				  tagi_t const *tags);
static int nua_update_client_request(nua_client_request_t *cr,
				     msg_t *msg, sip_t *sip,
				     tagi_t const *tags);
static int nua_update_client_response(nua_client_request_t *cr,
				      int status, char const *phrase,
				      sip_t const *sip);
static int nua_update_client_report(nua_client_request_t *cr,
				    int status, char const *phrase,
				    sip_t const *sip,
				    nta_outgoing_t *orq,
				    tagi_t const *tags);

nua_client_methods_t const nua_update_client_methods = {
  SIP_METHOD_UPDATE,
  0,				/* size of private data */
  { 
    /* create_dialog */ 0,
    /* in_dialog */ 1,
    /* target refresh */ 1
  },
  NULL,
  nua_update_client_init,
  nua_update_client_request,
  session_timer_check_restart,
  nua_update_client_response,
  NULL,
  nua_update_client_report
};

int nua_stack_update(nua_t *nua, nua_handle_t *nh, nua_event_t e,
		     tagi_t const *tags)
{
  return nua_client_create(nh, e, &nua_update_client_methods, tags);
}

static int nua_update_client_init(nua_client_request_t *cr, 
				  msg_t *msg, sip_t *sip,
				  tagi_t const *tags)
{
  nua_handle_t *nh = cr->cr_owner;
  nua_dialog_usage_t *du = nua_dialog_usage_for_session(nh->nh_ds);

  cr->cr_usage = du;

  return 0;
}

static int nua_update_client_request(nua_client_request_t *cr,
				     msg_t *msg, sip_t *sip,
				     tagi_t const *tags)
{
  nua_handle_t *nh = cr->cr_owner;
  nua_dialog_usage_t *du = cr->cr_usage;
  nua_session_usage_t *ss = nua_dialog_usage_private(du);
  nua_server_request_t *sr;
  nua_client_request_t *cri;
  int offer_sent = 0, retval;
  
  if (du == NULL)		/* Call terminated */
    return nua_client_return(cr, SIP_481_NO_TRANSACTION, msg);
  assert(ss);

  cri = du->du_cr;

  for (sr = nh->nh_ds->ds_sr; sr; sr = sr->sr_next)
    if ((sr->sr_offer_sent && !sr->sr_answer_recv) ||
	(sr->sr_offer_recv && !sr->sr_answer_sent))
      break;
    
  if (nh->nh_soa && !sip->sip_payload && 
      !sr &&
      !(cri && cri->cr_offer_sent && !cri->cr_answer_recv) &&
      !(cri && cri->cr_offer_recv && !cri->cr_answer_sent)) {
    soa_init_offer_answer(nh->nh_soa);

    if (soa_generate_offer(nh->nh_soa, 0, NULL) < 0 ||
	session_include_description(nh->nh_soa, 1, msg, sip) < 0) {
      if (ss->ss_state < nua_callstate_ready) {
	/* XXX - use soa_error_as_sip_reason(nh->nh_soa) */
	cr->cr_graceful = 1;
	ss->ss_reason = "SIP;cause=400;text=\"Local media failure\"";
      }
      return nua_client_return(cr, 900, "Local media failed", msg);
    }
    offer_sent = 1;
  }

  /* Add session timer headers */
  if (session_timer_is_supported(nh))
    use_session_timer(ss, 0, prefer_session_timer(nh), msg, sip);

  retval = nua_base_client_request(cr, msg, sip, NULL);

  if (retval == 0) {
    cr->cr_offer_sent = offer_sent;
    ss->ss_update_needed = 0;

    if (!cr->cr_restarting) {
      if (offer_sent)
	ss->ss_oa_sent = "offer";
      signal_call_state_change(nh, ss, 0, "UPDATE sent", ss->ss_state);
    }
  }

  return retval;
}

static int nua_update_client_response(nua_client_request_t *cr,
				      int status, char const *phrase,
				      sip_t const *sip)
{
  nua_handle_t *nh = cr->cr_owner;
  nua_dialog_usage_t *du = cr->cr_usage;
  nua_session_usage_t *ss = nua_dialog_usage_private(du);

  assert(200 <= status);

  if (ss && sip && status < 300) {
    if (is_session_timer_set(ss)) {
      init_session_timer(ss, sip, NH_PGET(nh, refresher));
      set_session_timer(ss);
    }
  }

  return nua_session_client_response(cr, status, phrase, sip);
}

/** @NUA_EVENT nua_r_update
 *
 * Answer to outgoing UPDATE.
 *
 * The UPDATE may be sent explicitly by nua_update() or
 * implicitly by NUA state machine.
 *
 * @param status response status code
 *               (if the request is retried, @a status is 100, the @a
 *               sip->sip_status->st_status contain the real status code
 *               from the response message, e.g., 302, 401, or 407)
 * @param phrase a short textual description of @a status code
 * @param nh     operation handle associated with the call
 * @param hmagic application context associated with the call
 * @param sip    response to UPDATE request or NULL upon an error
 *               (status code is in @a status and 
 *                descriptive message in @a phrase parameters)
 * @param tags   empty
 *
 * @sa @ref nua_call_model, @RFC3311, nua_update(), #nua_i_update
 *
 * @END_NUA_EVENT
 */

static int nua_update_client_report(nua_client_request_t *cr,
				    int status, char const *phrase,
				    sip_t const *sip,
				    nta_outgoing_t *orq,
				    tagi_t const *tags)
{
  nua_handle_t *nh = cr->cr_owner;
  nua_dialog_usage_t *du = cr->cr_usage;
  nua_session_usage_t *ss = nua_dialog_usage_private(du);

  nua_stack_event(nh->nh_nua, nh, 
		  nta_outgoing_getresponse(orq),
		  cr->cr_event,
		  status, phrase,
		  tags);

  if (!ss || orq != cr->cr_orq || 
      cr->cr_terminated || cr->cr_graceful || !cr->cr_offer_sent)
    return 1;

  signal_call_state_change(nh, ss, status, phrase, ss->ss_state);

  return 1;
}

/* ---------------------------------------------------------------------- */
/* UPDATE server */

int nua_update_server_init(nua_server_request_t *sr);
int nua_update_server_preprocess(nua_server_request_t *sr);
int nua_update_server_respond(nua_server_request_t *sr, tagi_t const *tags);
int nua_update_server_report(nua_server_request_t *, tagi_t const *);

nua_server_methods_t const nua_update_server_methods = 
  {
    SIP_METHOD_UPDATE,
    nua_i_update,		/* Event */
    { 
      0,			/* Do not create dialog */
      1,			/* In-dialog request */
      1,			/* Target refresh request  */
      1,			/* Add Contact */
    },
    nua_update_server_init,
    nua_update_server_preprocess,
    nua_base_server_params,
    nua_update_server_respond,
    nua_update_server_report,
  };

int nua_update_server_init(nua_server_request_t *sr)
{
  nua_handle_t *nh = sr->sr_owner;
  nua_session_usage_t *ss;

  sip_t const *request = sr->sr_request.sip;

  if (nua_session_server_init(sr))
    return sr->sr_status;

  ss = nua_dialog_usage_private(sr->sr_usage);

  /* Do session timer negotiation */
  if (request->sip_session_expires)
    init_session_timer(ss, request, NH_PGET(nh, refresher));

  if (sr->sr_sdp) {		/* Check for overlap */
    nua_client_request_t *cr;
    nua_server_request_t *sr0;
    int overlap = 0;

    /*
      A UAS that receives an UPDATE before it has generated a final
      response to a previous UPDATE on the same dialog MUST return a 500
      response to the new UPDATE, and MUST include a Retry-After header
      field with a randomly chosen value between 0 and 10 seconds.

      If an UPDATE is received that contains an offer, and the UAS has
      generated an offer (in an UPDATE, PRACK or INVITE) to which it has
      not yet received an answer, the UAS MUST reject the UPDATE with a 491
      response.  Similarly, if an UPDATE is received that contains an
      offer, and the UAS has received an offer (in an UPDATE, PRACK, or
      INVITE) to which it has not yet generated an answer, the UAS MUST
      reject the UPDATE with a 500 response, and MUST include a Retry-After
      header field with a randomly chosen value between 0 and 10 seconds.
    */
    for (cr = nh->nh_ds->ds_cr; cr; cr = cr->cr_next)
      if ((overlap = cr->cr_offer_sent && !cr->cr_answer_recv))
	break;

    if (!overlap)
      for (sr0 = nh->nh_ds->ds_sr; sr0; sr0 = sr0->sr_next)
	if ((overlap = sr0->sr_offer_recv && !sr0->sr_answer_sent))
	  break;

    if (overlap)
      return nua_server_retry_after(sr, 500, "Overlapping Offer/Answer", 1, 9);

    if (nh->nh_soa &&
	soa_set_remote_sdp(nh->nh_soa, NULL, sr->sr_sdp, sr->sr_sdp_len) < 0) {
      SU_DEBUG_5(("nua(%p): %s server: error parsing %s\n", (void *)nh,
		  "UPDATE", "offer"));
      return 
	sr->sr_status = soa_error_as_sip_response(nh->nh_soa, &sr->sr_phrase);
    }

    sr->sr_offer_recv = 1;
    ss->ss_oa_recv = "offer";
  }

  return 0;
}

int nua_update_server_preprocess(nua_server_request_t *sr)
{
  return sr_status(sr, SIP_200_OK); /* For now */
}

/** @internal Respond to an UPDATE request.
 *
 */
int nua_update_server_respond(nua_server_request_t *sr, tagi_t const *tags)
{
  nua_handle_t *nh = sr->sr_owner;
  nua_session_usage_t *ss = nua_dialog_usage_private(sr->sr_usage);
  soa_session_t *soa = nh->nh_soa;

  msg_t *msg = sr->sr_response.msg;
  sip_t *sip = sr->sr_response.sip;

  if (200 <= sr->sr_status && sr->sr_status < 300 && soa && sr->sr_sdp) {
    if (soa_generate_answer(nh->nh_soa, NULL) < 0) {
      SU_DEBUG_5(("nua(%p): %s server: %s %s\n", 
		  (void *)nh, "UPDATE", "error processing", "offer"));
      sr->sr_status = soa_error_as_sip_response(nh->nh_soa, &sr->sr_phrase);
    }
    else if (soa_activate(nh->nh_soa, NULL) < 0) {
      SU_DEBUG_5(("nua(%p): %s server: error activating media\n",
		  (void *)nh, "UPDATE"));
      /* XXX */
    }
    else if (session_include_description(nh->nh_soa, 1, msg, sip) < 0) {
      sr_status(sr, SIP_500_INTERNAL_SERVER_ERROR);
    }
    else
      sr->sr_answer_sent = 1, ss->ss_oa_sent = "answer";
  }

  if (ss->ss_refresher && 200 <= sr->sr_status && sr->sr_status < 300)
    if (session_timer_is_supported(nh)) {
      use_session_timer(ss, 1, 1, msg, sip);
      set_session_timer(ss);	/* XXX */
    }

  return nua_base_server_respond(sr, tags);
}

/** @NUA_EVENT nua_i_update
 *
 * @brief Incoming session UPDATE request.
 *
 * @param status statuscode of response sent automatically by stack
 * @param phrase a short textual description of @a status code
 * @param nh     operation handle associated with the call
 * @param hmagic application context associated with the call
 * @param sip    incoming UPDATE request
 * @param tags   empty
 *
 * @sa nua_update(), #nua_r_update, #nua_i_state
 *
 * @END_NUA_EVENT
 */

int nua_update_server_report(nua_server_request_t *sr, tagi_t const *tags)
{
  nua_handle_t *nh = sr->sr_owner;
  nua_dialog_usage_t *du = sr->sr_usage;
  nua_session_usage_t *ss = nua_dialog_usage_private(du);
  int retval = nua_base_server_report(sr, tags); /* destroys sr */

  if (retval >= 2 || ss == NULL) {
    signal_call_state_change(nh, NULL,
			     sr->sr_status, sr->sr_phrase, 
			     nua_callstate_terminated);
    return retval;
  }

  if (sr->sr_offer_recv || sr->sr_answer_sent)
    /* signal offer received, answer sent */
    signal_call_state_change(nh, ss,
			     sr->sr_status, sr->sr_phrase, 
			     ss->ss_state);

  if (200 <= sr->sr_status && sr->sr_status < 300
      && ss->ss_state < nua_callstate_ready
      && ss->ss_precondition 
      && !ss->ss_alerting
      && NH_PGET(nh, auto_alert))  {
    nua_server_request_t *sr;
    
    for (sr = nh->nh_ds->ds_sr; sr; sr = sr->sr_next)
      if (sr->sr_method == sip_method_invite && 
	  nua_server_request_is_pending(sr))
	break;

    if (sr) {
      SR_STATUS1(sr, SIP_180_RINGING);
      nua_server_respond(sr, NULL);
      nua_server_report(sr);
      return retval;
    }
  }

  return retval;
}

/* ======================================================================== */
/* BYE */

/**@fn void nua_bye(nua_handle_t *nh, tag_type_t tag, tag_value_t value, ...);
 *
 * Hangdown a call.
 *
 * Hangdown a call using SIP BYE method. Also the media session 
 * associated with the call is terminated. 
 *
 * @param nh              Pointer to operation handle
 * @param tag, value, ... List of tagged parameters
 *
 * @return 
 *    nothing
 *
 * @par Related Tags:
 *    none
 *
 * @par Events:
 *    #nua_r_bye \n
 *    #nua_i_media_error
 */

static int nua_bye_client_init(nua_client_request_t *cr, 
			       msg_t *msg, sip_t *sip,
			       tagi_t const *tags);
static int nua_bye_client_request(nua_client_request_t *cr,
				  msg_t *msg, sip_t *sip,
				  tagi_t const *tags);
static int nua_bye_client_report(nua_client_request_t *cr,
				 int status, char const *phrase,
				 sip_t const *sip,
				 nta_outgoing_t *orq,
				 tagi_t const *tags);

nua_client_methods_t const nua_bye_client_methods = {
  SIP_METHOD_BYE,
  0,
  { 
    /* create_dialog */ 0,
    /* in_dialog */ 1,
    /* target refresh */ 0
  },
  NULL,
  nua_bye_client_init,
  nua_bye_client_request,
  /*nua_bye_client_check_restart*/ NULL,
  /*nua_bye_client_response*/ NULL,
  /*nua_bye_client_preliminary*/ NULL,
  nua_bye_client_report
};

int
nua_stack_bye(nua_t *nua, nua_handle_t *nh, nua_event_t e, tagi_t const *tags)
{
  nua_session_usage_t *ss = nua_session_usage_get(nh->nh_ds);

  if (ss && 
      nua_callstate_calling <= ss->ss_state &&
      ss->ss_state <= nua_callstate_proceeding)
    return nua_client_create(nh, e, &nua_cancel_client_methods, tags);
  else
    return nua_client_create(nh, e, &nua_bye_client_methods, tags);
}

static int nua_bye_client_init(nua_client_request_t *cr, 
			       msg_t *msg, sip_t *sip,
			       tagi_t const *tags)
{
  nua_handle_t *nh = cr->cr_owner;
  nua_dialog_usage_t *du = nua_dialog_usage_for_session(nh->nh_ds);
  nua_session_usage_t *ss = nua_dialog_usage_private(du);

  if (!ss || (ss->ss_state >= nua_callstate_terminating && !cr->cr_auto))
    return nua_client_return(cr, 900, "Invalid handle for BYE", msg);

  if (!cr->cr_auto)
    /* Implicit state transition by nua_bye() */
    ss->ss_state = nua_callstate_terminating;

  if (nh->nh_soa)
    soa_terminate(nh->nh_soa, 0);
  cr->cr_usage = du;

  return 0;
}

static int nua_bye_client_request(nua_client_request_t *cr,
				  msg_t *msg, sip_t *sip,
				  tagi_t const *tags)
{
  nua_dialog_usage_t *du = cr->cr_usage;
  nua_session_usage_t *ss;
  char const *reason = NULL;

  if (du == NULL)
    return nua_client_return(cr, SIP_481_NO_TRANSACTION, msg);

  ss = nua_dialog_usage_private(du);
  reason = ss->ss_reason;

  return nua_base_client_trequest(cr, msg, sip,
				  SIPTAG_REASON_STR(reason),
				  TAG_NEXT(tags));
}

/** @NUA_EVENT nua_r_bye
 *
 * Answer to outgoing BYE.
 *
 * The BYE may be sent explicitly by nua_bye() or implicitly by NUA state
 * machine.
 *
 * @param status response status code
 *               (if the request is retried, @a status is 100, the @a
 *               sip->sip_status->st_status contain the real status code
 *               from the response message, e.g., 302, 401, or 407)
 * @param phrase a short textual description of @a status code
 * @param nh     operation handle associated with the call
 * @param hmagic application context associated with the call
 * @param sip    response to BYE request or NULL upon an error
 *               (status code is in @a status and 
 *                descriptive message in @a phrase parameters)
 * @param tags   empty
 *
 * @sa nua_bye(), @ref nua_call_model, #nua_i_state, #nua_r_invite()
 * 
 * @END_NUA_EVENT
 */

static int nua_bye_client_report(nua_client_request_t *cr,
				 int status, char const *phrase,
				 sip_t const *sip,
				 nta_outgoing_t *orq,
				 tagi_t const *tags)
{
  nua_handle_t *nh = cr->cr_owner;
  nua_dialog_usage_t *du = cr->cr_usage;

  nua_stack_event(nh->nh_nua, nh, 
		  nta_outgoing_getresponse(orq),
		  cr->cr_event,
		  status, phrase,
		  tags);

  if (du == NULL) {
    /* No more session */
  }
  else if (status < 200) {
    /* Preliminary */
  }
  else {
    nua_session_usage_t *ss = nua_dialog_usage_private(du);

    signal_call_state_change(nh, ss, status, "to BYE", 
			     nua_callstate_terminated);

    if (ss && !ss->ss_reporting && !nua_client_is_queued(du->du_cr)) {
      /* Do not destroy session usage while INVITE is alive */
      nua_session_usage_destroy(nh, ss);
    }
  }

  return 1;
}

/** @NUA_EVENT nua_i_bye
 *
 * Incoming BYE request, call hangup.
 *
 * @param status statuscode of response sent automatically by stack
 * @param phrase a short textual description of @a status code
 * @param nh     operation handle associated with the call
 * @param hmagic application context associated with the call
 * @param sip    pointer to BYE request
 * @param tags   empty
 *
 * @sa @ref nua_call_model, #nua_i_state, nua_bye(), nua_bye(), #nua_r_cancel
 *
 * @END_NUA_EVENT
 */

int nua_bye_server_init(nua_server_request_t *sr);
int nua_bye_server_report(nua_server_request_t *sr, tagi_t const *tags);

nua_server_methods_t const nua_bye_server_methods = 
  {
    SIP_METHOD_BYE,
    nua_i_bye,			/* Event */
    { 
      0,			/* Do not create dialog */
      1,			/* In-dialog request */
      0,			/* Not a target refresh request  */
      0,			/* Do not add Contact */
    },
    nua_bye_server_init,
    nua_base_server_preprocess,
    nua_base_server_params,
    nua_base_server_respond,
    nua_bye_server_report,
  };


int nua_bye_server_init(nua_server_request_t *sr)
{
  nua_handle_t *nh = sr->sr_owner;
  nua_dialog_usage_t *du = nua_dialog_usage_for_session(nh->nh_ds);

  sr->sr_terminating = 1;

  if (du)
    sr->sr_usage = du;
  else
    return SR_STATUS(sr, 481, "No Such Call");

  return 0;
}

int nua_bye_server_report(nua_server_request_t *sr, tagi_t const *tags)
{
  nua_handle_t *nh = sr->sr_owner;
  nua_session_usage_t *ss = nua_dialog_usage_private(sr->sr_usage);
  int early = 0, retval;

  if (sr->sr_status < 200)
    return nua_base_server_report(sr, tags);

  if (ss) {
    nua_server_request_t *sr0 = NULL, *sr_next;
    char const *phrase;

    early = ss->ss_state < nua_callstate_ready;
    phrase = early ? "Early Session Terminated" : "Session Terminated";
    
    for (sr0 = nh->nh_ds->ds_sr; sr0; sr0 = sr_next) {
      sr_next = sr0->sr_next;

      if (sr == sr0 || sr0->sr_usage != sr->sr_usage)
	continue;

      if (nua_server_request_is_pending(sr0)) {
	SR_STATUS(sr0, 487, phrase);
	nua_server_respond(sr0, NULL);
      }
      nua_server_request_destroy(sr0);
    }
  }

  retval = nua_base_server_report(sr, tags);

  assert(2 <= retval && retval < 4);

  if (ss)
    signal_call_state_change(nh, NULL, 200,
			     early ? "Received early BYE" : "Received BYE",
			     nua_callstate_terminated);

  return retval;
}

/* ---------------------------------------------------------------------- */

/**
 * Delivers call state changed event to the nua client. @internal
 *
 * @param nh call handle
 * @param status status code
 * @param tr_event SIP transaction event triggering this change
 * @param oa_recv Received SDP
 * @param oa_sent Sent SDP
 */

static void signal_call_state_change(nua_handle_t *nh,
				     nua_session_usage_t *ss,
				     int status, char const *phrase,
				     enum nua_callstate next_state)
{
  enum nua_callstate ss_state = nua_callstate_init;

  sdp_session_t const *remote_sdp = NULL;
  char const *remote_sdp_str = NULL;
  sdp_session_t const *local_sdp = NULL;
  char const *local_sdp_str = NULL;
  char const *oa_recv = NULL;
  char const *oa_sent = NULL;

  int offer_recv = 0, answer_recv = 0, offer_sent = 0, answer_sent = 0;

  if (ss && ss->ss_reporting)
    return;

  if (ss) {
    ss_state = ss->ss_state;
    oa_recv = ss->ss_oa_recv, ss->ss_oa_recv = NULL;
    oa_sent = ss->ss_oa_sent, ss->ss_oa_sent = NULL;
  }

  if (ss_state < nua_callstate_ready || next_state > nua_callstate_ready)
    SU_DEBUG_5(("nua(%p): call state changed: %s -> %s%s%s%s%s\n",
		(void *)nh, nua_callstate_name(ss_state),
		nua_callstate_name(next_state),
		oa_recv ? ", received " : "", oa_recv ? oa_recv : "",
		oa_sent && oa_recv ? ", and sent " :
		oa_sent ? ", sent " : "", oa_sent ? oa_sent : ""));
  else
    SU_DEBUG_5(("nua(%p): ready call updated: %s%s%s%s%s\n",
		(void *)nh, nua_callstate_name(next_state),
		oa_recv ? " received " : "", oa_recv ? oa_recv : "",
		oa_sent && oa_recv ? ", sent " :
		oa_sent ? " sent " : "", oa_sent ? oa_sent : ""));

  if (next_state == nua_callstate_terminating &&
      ss_state >= nua_callstate_terminating)
    return;

  if (oa_recv) {
    soa_get_remote_sdp(nh->nh_soa, &remote_sdp, &remote_sdp_str, 0);
    offer_recv = strcasecmp(oa_recv, "offer") == 0;
    answer_recv = strcasecmp(oa_recv, "answer") == 0;
  }

  if (oa_sent) {
    soa_get_local_sdp(nh->nh_soa, &local_sdp, &local_sdp_str, 0);
    offer_sent = strcasecmp(oa_sent, "offer") == 0;
    answer_sent = strcasecmp(oa_sent, "answer") == 0;
  }

  if (answer_recv || answer_sent) {
    /* Update nh_hold_remote */

    char const *held;

    soa_get_params(nh->nh_soa, SOATAG_HOLD_REF(held), TAG_END());

    nh->nh_hold_remote = held && strlen(held) > 0;
  }

  if (ss) {
    /* Update state variables */
    if (next_state == nua_callstate_init) {
      if (ss_state < nua_callstate_ready)
	ss->ss_state = next_state;
      else
	/* Do not change state - we are ready, terminating, or terminated */
	next_state = ss_state;
    }
    else if (next_state > ss_state)
      ss->ss_state = next_state;
  }

  if (next_state == nua_callstate_init) 
    next_state = nua_callstate_terminated;

  if (ss && ss->ss_state == nua_callstate_ready)
    nh->nh_active_call = 1;
  else if (next_state == nua_callstate_terminated)
    nh->nh_active_call = 0;

  /* Send events */
  if (phrase == NULL)
    phrase = "Call state";

/** @NUA_EVENT nua_i_state
 *
 * @brief Call state has changed.
 *
 * This event will be sent whenever the call state changes. 
 *
 * In addition to basic changes of session status indicated with enum
 * ::nua_callstate, the @RFC3264 SDP Offer/Answer negotiation status is also
 * included if it is enabled (by default or with NUTAG_MEDIA_ENABLE(1)). The
 * received remote SDP is included in tag SOATAG_REMOTE_SDP(). The tags
 * NUTAG_OFFER_RECV() or NUTAG_ANSWER_RECV() indicate whether the remote SDP
 * was an offer or an answer. The SDP negotiation result is included in the
 * tags SOATAG_LOCAL_SDP() and SOATAG_LOCAL_SDP_STR() and tags
 * NUTAG_OFFER_SENT() or NUTAG_ANSWER_SENT() indicate whether the local SDP
 * was an offer or answer.
 *
 * SOATAG_ACTIVE_AUDIO() and SOATAG_ACTIVE_VIDEO() are informational tags
 * used to indicate what is the status of audio or video.
 *
 * Note that #nua_i_state also covers call establisment events
 * (#nua_i_active) and termination (#nua_i_terminated).
 *
 * @param status protocol status code \n
 *               (always present)
 * @param phrase short description of status code \n
 *               (always present)
 * @param nh     operation handle associated with the call
 * @param hmagic application context associated with the call
 * @param sip    NULL
 * @param tags   NUTAG_CALLSTATE(), 
 *               SOATAG_LOCAL_SDP(), SOATAG_LOCAL_SDP_STR(),
 *               NUTAG_OFFER_SENT(), NUTAG_ANSWER_SENT(),
 *               SOATAG_REMOTE_SDP(), SOATAG_REMOTE_SDP_STR(),
 *               NUTAG_OFFER_RECV(), NUTAG_ANSWER_RECV(),
 *               SOATAG_ACTIVE_AUDIO(), SOATAG_ACTIVE_VIDEO(),
 *               SOATAG_ACTIVE_IMAGE(), SOATAG_ACTIVE_CHAT().
 *
 * @sa @ref nua_call_model, #nua_i_active, #nua_i_terminated,
 * nua_invite(), #nua_r_invite, #nua_i_invite, nua_respond(), 
 * NUTAG_AUTOALERT(), NUTAG_AUTOANSWER(), NUTAG_EARLY_MEDIA(),
 * NUTAG_EARLY_ANSWER(), NUTAG_INCLUDE_EXTRA_SDP(),
 * nua_ack(), NUTAG_AUTOACK(), nua_bye(), #nua_r_bye, #nua_i_bye,
 * nua_cancel(), #nua_r_cancel, #nua_i_cancel,
 * nua_prack(), #nua_r_prack, #nua_i_prack,
 * nua_update(), #nua_r_update, #nua_i_update
 *
 * @END_NUA_EVENT
 */

  nua_stack_tevent(nh->nh_nua, nh, NULL, nua_i_state,
		   status, phrase,
		   NUTAG_CALLSTATE(next_state),
		   NH_ACTIVE_MEDIA_TAGS(1, nh->nh_soa),
		   /* NUTAG_SOA_SESSION(nh->nh_soa), */
		   TAG_IF(offer_recv, NUTAG_OFFER_RECV(offer_recv)),
		   TAG_IF(answer_recv, NUTAG_ANSWER_RECV(answer_recv)),
		   TAG_IF(offer_sent, NUTAG_OFFER_SENT(offer_sent)),
		   TAG_IF(answer_sent, NUTAG_ANSWER_SENT(answer_sent)),
		   TAG_IF(oa_recv, SOATAG_REMOTE_SDP(remote_sdp)),
		   TAG_IF(oa_recv, SOATAG_REMOTE_SDP_STR(remote_sdp_str)),
		   TAG_IF(oa_sent, SOATAG_LOCAL_SDP(local_sdp)),
		   TAG_IF(oa_sent, SOATAG_LOCAL_SDP_STR(local_sdp_str)),
		   TAG_END());

/** @NUA_EVENT nua_i_active
 *
 * A call has been activated.
 *
 * This event will be sent after a succesful response to the initial
 * INVITE has been received and the media has been activated.
 *
 * @param nh     operation handle associated with the call
 * @param hmagic application context associated with the call
 * @param sip    NULL
 * @param tags   SOATAG_ACTIVE_AUDIO(), SOATAG_ACTIVE_VIDEO(),
 *               SOATAG_ACTIVE_IMAGE(), SOATAG_ACTIVE_CHAT().
 *
 * @deprecated Use #nua_i_state instead.
 *
 * @sa @ref nua_call_model, #nua_i_state, #nua_i_terminated, 
 * #nua_i_invite
 *
 * @END_NUA_EVENT
 */

  if (next_state == nua_callstate_ready && ss_state <= nua_callstate_ready) {
    nua_stack_tevent(nh->nh_nua, nh, NULL, nua_i_active, status, "Call active",
		     NH_ACTIVE_MEDIA_TAGS(1, nh->nh_soa),
		     /* NUTAG_SOA_SESSION(nh->nh_soa), */
		     TAG_END());
  }

/** @NUA_EVENT nua_i_terminated
 *
 * A call has been terminated.
 *
 * This event will be sent after a call has been terminated. A call is
 * terminated, when
 * 1) an error response (300..599) is sent to an incoming initial INVITE
 * 2) a reliable response (200..299 or reliable preliminary response) to
 *    an incoming initial INVITE is not acknowledged with ACK or PRACK
 * 3) BYE is received or sent
 *
 * @param nh     operation handle associated with the call
 * @param hmagic application context associated with the call
 * @param sip    NULL
 * @param tags   empty
 *
 * @deprecated Use #nua_i_state instead.
 *
 * @sa @ref nua_call_model, #nua_i_state, #nua_i_active, #nua_i_bye,
 * #nua_i_invite
 *
 * @END_NUA_EVENT
 */

  else if (next_state == nua_callstate_terminated) {
    nua_stack_event(nh->nh_nua, nh, NULL,
		    nua_i_terminated, status, phrase,
		    NULL);
  }
}

/* ======================================================================== */

static
int nua_server_retry_after(nua_server_request_t *sr,
			   int status, char const *phrase,
			   int min, int max)
{
  sip_retry_after_t af[1];

  sip_retry_after_init(af);
  af->af_delta = (unsigned)su_randint(min, max);
  af->af_comment = phrase;

  sip_add_dup(sr->sr_response.msg, sr->sr_response.sip, (sip_header_t *)af);

  return sr_status(sr, status, phrase);
}

/* ======================================================================== */

/** Get SDP from a SIP message */
static
int session_get_description(sip_t const *sip,
			    char const **return_sdp,
			    size_t *return_len)
{
  sip_payload_t const *pl = sip->sip_payload;
  sip_content_type_t const *ct = sip->sip_content_type;
  int matching_content_type = 0;

  if (pl == NULL)
    return 0;
  else if (pl->pl_len == 0 || pl->pl_data == NULL)
    return 0;
  else if (ct == NULL)
    /* Be bug-compatible with our old gateways */
    SU_DEBUG_3(("nua: no %s, assuming %s\n",
		"Content-Type", SDP_MIME_TYPE));
  else if (ct->c_type == NULL)
    SU_DEBUG_3(("nua: empty %s, assuming %s\n",
		"Content-Type", SDP_MIME_TYPE));
  else if (strcasecmp(ct->c_type, SDP_MIME_TYPE)) {
    SU_DEBUG_5(("nua: unknown %s: %s\n", "Content-Type", ct->c_type));
    return 0;
  }
  else
    matching_content_type = 1;

  if (pl == NULL)
    return 0;

  if (!matching_content_type) {
    /* Make sure we got SDP */
    if (pl->pl_len < 3 || strncasecmp(pl->pl_data, "v=0", 3))
      return 0;
  }

  *return_sdp = pl->pl_data;
  *return_len = pl->pl_len;

  return 1;
}

/** Insert SDP into SIP message */
static
int session_include_description(soa_session_t *soa,
				int session,
				msg_t *msg,
				sip_t *sip)
{
  su_home_t *home = msg_home(msg);

  sip_content_disposition_t *cd = NULL;
  sip_content_type_t *ct = NULL;
  sip_payload_t *pl = NULL;

  int retval;

  if (!soa)
    return 0;

  retval = session_make_description(home, soa, session, &cd, &ct, &pl);

  if (retval <= 0)
    return retval;

  if ((cd && sip_header_insert(msg, sip, (sip_header_t *)cd) < 0) ||
      sip_header_insert(msg, sip, (sip_header_t *)ct) < 0 ||
      sip_header_insert(msg, sip, (sip_header_t *)pl) < 0)
    return -1;

  return retval;
}

/** Generate SDP headers */
static
int session_make_description(su_home_t *home,
			     soa_session_t *soa,
			     int session,
			     sip_content_disposition_t **return_cd,
			     sip_content_type_t **return_ct,
			     sip_payload_t **return_pl)
{
  char const *sdp;
  isize_t len;
  int retval;

  if (!soa)
    return 0;

  if (session)
    retval = soa_get_local_sdp(soa, 0, &sdp, &len);
  else
    retval = soa_get_capability_sdp(soa, 0, &sdp, &len);

  if (retval > 0) {
    *return_pl = sip_payload_create(home, sdp, len);
    *return_ct = sip_content_type_make(home, SDP_MIME_TYPE);
    if (session)
      *return_cd = sip_content_disposition_make(home, "session");
    else
      *return_cd = NULL;

    if (!*return_pl || !*return_cd)
      return -1;

    if (session && !*return_cd)
      return -1;
  }

  return retval;
}

/** @NUA_EVENT nua_i_options
 *
 * Incoming OPTIONS request. The user-agent should respond to an OPTIONS
 * request with the same statuscode as it would respond to an INVITE
 * request.
 *
 * Stack responds automatically to OPTIONS request unless OPTIONS is
 * included in the set of application methods, set by NUTAG_APPL_METHOD().
 *
 * The OPTIONS request does not create a dialog. Currently the processing
 * of incoming OPTIONS creates a new handle for each incoming request which
 * is not assiciated with an existing dialog. If the handle @a nh is not
 * bound, you should probably destroy it after responding to the OPTIONS
 * request.
 *
 * @param status status code of response sent automatically by stack
 * @param phrase a short textual description of @a status code
 * @param nh     operation handle associated with the OPTIONS request
 * @param hmagic application context associated with the call
 *               (NULL if outside session)
 * @param sip    incoming OPTIONS request
 * @param tags   empty
 *
 * @sa nua_respond(), nua_options(), #nua_r_options, @RFC3261 section 11.2
 *
 * @END_NUA_EVENT
 */

int nua_options_server_respond(nua_server_request_t *sr, tagi_t const *tags);

nua_server_methods_t const nua_options_server_methods = 
  {
    SIP_METHOD_OPTIONS,
    nua_i_options,		/* Event */
    { 
      0,			/* Do not create dialog */
      0,			/* Initial request */
      0,			/* Not a target refresh request  */
      1,			/* Add Contact */
    },
    nua_base_server_init,
    nua_base_server_preprocess,
    nua_base_server_params,
    nua_options_server_respond,
    nua_base_server_report,
  };

/** @internal Respond to an OPTIONS request.
 *
 */
int nua_options_server_respond(nua_server_request_t *sr, tagi_t const *tags)
{
  nua_handle_t *nh = sr->sr_owner;
  nua_t *nua = nh->nh_nua;

  if (200 <= sr->sr_status && sr->sr_status < 300) {
    msg_t *msg = sr->sr_response.msg;
    sip_t *sip = sr->sr_response.sip;

    sip_add_tl(msg, sip, SIPTAG_ACCEPT_STR(SDP_MIME_TYPE), TAG_END());

    if (!sip->sip_payload) {	/* XXX - do MIME multipart? */
      soa_session_t *soa = nh->nh_soa;

      if (soa == NULL)
	soa = nua->nua_dhandle->nh_soa;

      session_include_description(soa, 0, msg, sip);
    }
  }

  return nua_base_server_respond(sr, tags);
}


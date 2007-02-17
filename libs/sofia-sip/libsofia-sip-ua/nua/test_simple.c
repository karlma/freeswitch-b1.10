/*
 * This file is part of the Sofia-SIP package
 *
 * Copyright (C) 2005 Nokia Corporation.
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

/**@CFILE test_nua_simple.c
 * @brief NUA-11: Test SIMPLE methods: MESSAGE, PUBLISH and SUBSCRIBE/NOTIFY.
 *
 * @author Pekka Pessi <Pekka.Pessi@nokia.com>
 * @author Martti Mela <Martti Mela@nokia.com>
 *
 * @date Created: Wed Aug 17 12:12:12 EEST 2005 ppessi
 */

#include "config.h"

#include "test_nua.h"
#include <sofia-sip/su_tag_class.h>

#if HAVE_FUNC
#elif HAVE_FUNCTION
#define __func__ __FUNCTION__
#else
#define __func__ "test_simple"
#endif

int test_message(struct context *ctx)
{
  BEGIN();

  struct endpoint *a = &ctx->a,  *b = &ctx->b;
  struct call *a_call = a->call, *b_call = b->call;
  struct event *e;
  sip_t const *sip;
  url_t url[1];

/* Message test

   A			B
   |-------MESSAGE----->|
   |<-------200---------|
   |			|

*/
  if (print_headings)
    printf("TEST NUA-11.1: MESSAGE\n");

  if (ctx->proxy_tests)
    *url = *b->to->a_url;
  else
    *url = *b->contact->m_url;

  /* Test that query part is included in request sent to B */
  url->url_headers = "organization=United%20Testers";

  TEST_1(a_call->nh = nua_handle(a->nua, a_call, TAG_END()));

  MESSAGE(a, a_call, a_call->nh,
	  NUTAG_URL(url),
	  SIPTAG_SUBJECT_STR("NUA-11.1"),
	  SIPTAG_CONTENT_TYPE_STR("text/plain"),
	  SIPTAG_PAYLOAD_STR("Hello hellO!\n"),
	  TAG_END());

  run_ab_until(ctx, -1, save_until_final_response, -1, save_until_received);

  /* Client events:
     nua_message(), nua_r_message
  */
  TEST_1(e = a->events->head); TEST_E(e->data->e_event, nua_r_message);
  TEST(e->data->e_status, 200);
  TEST_1(!e->next);

  /*
   Server events:
   nua_i_message
  */
  TEST_1(e = b->events->head); TEST_E(e->data->e_event, nua_i_message);
  TEST(e->data->e_status, 200);
  TEST_1(sip = sip_object(e->data->e_msg));
  TEST_1(sip->sip_subject && sip->sip_subject->g_string);
  TEST_S(sip->sip_subject->g_string, "NUA-11.1");
  TEST_1(sip->sip_organization);
  TEST_S(sip->sip_organization->g_string, "United Testers");
  TEST_1(!e->next);

  free_events_in_list(ctx, a->events);
  nua_handle_destroy(a_call->nh), a_call->nh = NULL;

  free_events_in_list(ctx, b->events);
  nua_handle_destroy(b_call->nh), b_call->nh = NULL;

  if (print_headings)
    printf("TEST NUA-11.1: PASSED\n");


/* Message test

   A
   |-------MESSAGE--\
   |<---------------/
   |--------200-----\
   |<---------------/
   |

*/
  if (print_headings)
    printf("TEST NUA-11.2: MESSAGE to myself\n");

  TEST_1(a_call->nh = nua_handle(a->nua, a_call, SIPTAG_TO(a->to), TAG_END()));

  MESSAGE(a, a_call, a_call->nh,
	  /* We cannot reach us by using our contact! */
	  NUTAG_URL(!ctx->p && !ctx->proxy_tests ? a->contact->m_url : NULL),
	  SIPTAG_SUBJECT_STR("NUA-11.1b"),
	  SIPTAG_CONTENT_TYPE_STR("text/plain"),
	  SIPTAG_PAYLOAD_STR("Hello hellO!\n"),
	  TAG_END());

  run_a_until(ctx, -1, save_until_final_response);

  /* Events:
     nua_message(), nua_i_message, nua_r_message
  */
  TEST_1(e = a->events->head); TEST_E(e->data->e_event, nua_i_message);
  TEST(e->data->e_status, 200);
  TEST_1(sip = sip_object(e->data->e_msg));
  TEST_1(sip->sip_subject && sip->sip_subject->g_string);
  TEST_S(sip->sip_subject->g_string, "NUA-11.1b");
  TEST_1(e = e->next); TEST_E(e->data->e_event, nua_r_message);
  TEST(e->data->e_status, 200);
  TEST_1(!e->next);

  free_events_in_list(ctx, a->events);
  nua_handle_destroy(a_call->nh), a_call->nh = NULL;

  if (print_headings)
    printf("TEST NUA-11.2: PASSED\n");

  END();
}

int respond_with_etag(CONDITION_PARAMS)
{
  msg_t *with = nua_current_request(nua);

  if (!(check_handle(ep, call, nh, SIP_500_INTERNAL_SERVER_ERROR)))
    return 0;

  save_event_in_list(ctx, event, ep, call);

  switch (event) {
    char const *etag;
  case nua_i_publish:
    etag = sip->sip_if_match ? sip->sip_if_match->g_value : NULL;
    if (sip->sip_if_match && (etag == NULL || strcmp(etag, "tagtag"))) {
      RESPOND(ep, call, nh, SIP_412_PRECONDITION_FAILED,
	      NUTAG_WITH(with),
	      TAG_END());
    } 
    else {
	RESPOND(ep, call, nh, SIP_200_OK,
		NUTAG_WITH(with),
		SIPTAG_ETAG_STR("tagtag"),
		SIPTAG_EXPIRES_STR("3600"),
		SIPTAG_EXPIRES(sip->sip_expires),	/* overrides 3600 */
		TAG_END());
    }
    return 1;
  default:
    return 0;
  }
}

int test_publish(struct context *ctx)
{
  BEGIN();

  struct endpoint *a = &ctx->a,  *b = &ctx->b;
  struct call *a_call = a->call, *b_call = b->call;
  struct event *e;
  sip_t const *sip;


/* PUBLISH test

   A			B
   |-------PUBLISH----->|
   |<-------405---------| (method not allowed by default)
   |			|
   |-------PUBLISH----->|
   |<-------501---------| (no events allowed)
   |			|
   |-------PUBLISH----->|
   |<-------489---------| (event not allowed by default)
   |			|
   |-------PUBLISH----->|
   |<-------200---------| (event allowed, responded)
   |			|
   |-----un-PUBLISH---->|
   |<-------200---------| (event allowed, responded)

*/
  if (print_headings)
    printf("TEST NUA-11.3: PUBLISH\n");

  TEST_1(a_call->nh = nua_handle(a->nua, a_call, SIPTAG_TO(b->to), TAG_END()));

  PUBLISH(a, a_call, a_call->nh,
	  TAG_IF(!ctx->proxy_tests, NUTAG_URL(b->contact->m_url)),
	  SIPTAG_EVENT_STR("presence"),
	  SIPTAG_CONTENT_TYPE_STR("text/urllist"),
	  SIPTAG_PAYLOAD_STR("sip:example.com\n"),
	  TAG_END());

  run_ab_until(ctx, -1, save_until_final_response, -1, NULL);

  /* Client events:
     nua_publish(), nua_r_publish
  */
  TEST_1(e = a->events->head); TEST_E(e->data->e_event, nua_r_publish);
  TEST(e->data->e_status, 405);
  TEST_1(!e->next);

  free_events_in_list(ctx, a->events);
  nua_handle_destroy(a_call->nh), a_call->nh = NULL;

  free_events_in_list(ctx, b->events);
  nua_handle_destroy(b_call->nh), b_call->nh = NULL;

  nua_set_params(b->nua, NUTAG_ALLOW("PUBLISH"), TAG_END());

  run_b_until(ctx, nua_r_set_params, until_final_response);

  TEST_1(a_call->nh = nua_handle(a->nua, a_call, SIPTAG_TO(b->to), TAG_END()));

  PUBLISH(a, a_call, a_call->nh,
	  TAG_IF(!ctx->proxy_tests, NUTAG_URL(b->contact->m_url)),
	  SIPTAG_EVENT_STR("presence"),
	  SIPTAG_CONTENT_TYPE_STR("text/urllist"),
	  SIPTAG_PAYLOAD_STR("sip:example.com\n"),
	  TAG_END());

  run_a_until(ctx, -1, save_until_final_response);

  /* Client events:
     nua_publish(), nua_r_publish
  */
  TEST_1(e = a->events->head); TEST_E(e->data->e_event, nua_r_publish);
  TEST(e->data->e_status, 501);	/* Not implemented */
  TEST_1(!e->next);

  free_events_in_list(ctx, a->events);
  nua_handle_destroy(a_call->nh), a_call->nh = NULL;

  /* Allow presence event */

  nua_set_params(b->nua, NUTAG_ALLOW_EVENTS("presence"), TAG_END());

  run_b_until(ctx, nua_r_set_params, until_final_response);

  TEST_1(a_call->nh = nua_handle(a->nua, a_call, SIPTAG_TO(b->to), TAG_END()));

  PUBLISH(a, a_call, a_call->nh,
	  TAG_IF(!ctx->proxy_tests, NUTAG_URL(b->contact->m_url)),
	  SIPTAG_EVENT_STR("reg"),
	  SIPTAG_CONTENT_TYPE_STR("text/urllist"),
	  SIPTAG_PAYLOAD_STR("sip:example.com\n"),
	  TAG_END());

  run_a_until(ctx, -1, save_until_final_response);

  /* Client events:
     nua_publish(), nua_r_publish
  */
  TEST_1(e = a->events->head); TEST_E(e->data->e_event, nua_r_publish);
  TEST(e->data->e_status, 489);	/* Bad Event */
  TEST_1(!e->next);

  free_events_in_list(ctx, a->events);
  nua_handle_destroy(a_call->nh), a_call->nh = NULL;

  TEST_1(a_call->nh = nua_handle(a->nua, a_call, SIPTAG_TO(b->to), TAG_END()));

  PUBLISH(a, a_call, a_call->nh,
	  TAG_IF(!ctx->proxy_tests, NUTAG_URL(b->contact->m_url)),
	  SIPTAG_EVENT_STR("presence"),
	  SIPTAG_CONTENT_TYPE_STR("text/urllist"),
	  SIPTAG_PAYLOAD_STR("sip:example.com\n"),
	  TAG_END());

  run_ab_until(ctx, -1, save_until_final_response, -1, respond_with_etag);

  /* Client events:
     nua_publish(), nua_r_publish
  */
  TEST_1(e = a->events->head); TEST_E(e->data->e_event, nua_r_publish);
  TEST(e->data->e_status, 200);
  TEST_1(sip = sip_object(e->data->e_msg));
  TEST_1(sip->sip_etag);
  TEST_S(sip->sip_etag->g_string, "tagtag");
  TEST_1(!e->next);

  /*
   Server events:
   nua_i_publish
  */
  TEST_1(e = b->events->head); TEST_E(e->data->e_event, nua_i_publish);
  TEST(e->data->e_status, 100);
  TEST_1(!e->next);

  free_events_in_list(ctx, a->events);
  free_events_in_list(ctx, b->events);
  nua_handle_destroy(b_call->nh), b_call->nh = NULL;

  UNPUBLISH(a, a_call, a_call->nh, TAG_END());

  run_ab_until(ctx, -1, save_until_final_response, -1, respond_with_etag);

  /* Client events:
     nua_publish(), nua_r_publish
  */
  TEST_1(e = a->events->head); TEST_E(e->data->e_event, nua_r_unpublish);
  TEST(e->data->e_status, 200);
  TEST_1(!e->next);

  /*
   Server events:
   nua_i_publish
  */
  TEST_1(e = b->events->head); TEST_E(e->data->e_event, nua_i_publish);
  TEST(e->data->e_status, 100);
  TEST_1(!e->next);

  free_events_in_list(ctx, a->events);
  free_events_in_list(ctx, b->events);

  nua_handle_destroy(a_call->nh), a_call->nh = NULL;
  nua_handle_destroy(b_call->nh), b_call->nh = NULL;

  if (print_headings)
    printf("TEST NUA-11.3: PASSED\n");
  END();
}

static char const presence_open[] =
    "<?xml version='1.0' encoding='UTF-8'?>\n"
    "<presence xmlns='urn:ietf:params:xml:ns:cpim-pidf' \n"
    "   entity='pres:bob@example.org'>\n"
    "  <tuple id='ksac9udshce'>\n"
    "    <status><basic>open</basic></status>\n"
    "    <contact priority='1.0'>sip:bob@example.org</contact>\n"
    "  </tuple>\n"
    "</presence>\n";

static char const presence_closed[] =
    "<?xml version='1.0' encoding='UTF-8'?>\n"
    "<presence xmlns='urn:ietf:params:xml:ns:cpim-pidf' \n"
    "   entity='pres:bob@example.org'>\n"
    "  <tuple id='ksac9udshce'>\n"
    "    <status><basic>closed</basic></status>\n"
    "  </tuple>\n"
    "</presence>\n";


int accept_and_notify(CONDITION_PARAMS)
{
  msg_t *with = nua_current_request(nua);

  if (!(check_handle(ep, call, nh, SIP_500_INTERNAL_SERVER_ERROR)))
    return 0;

  save_event_in_list(ctx, event, ep, call);

  switch (event) {
  case nua_i_subscribe:
    if (status < 200) {
      RESPOND(ep, call, nh, SIP_202_ACCEPTED,
	      NUTAG_WITH(with),
	      SIPTAG_EXPIRES_STR("360"),
	      TAG_END());
      NOTIFY(ep, call, nh, 
	     SIPTAG_EVENT(sip->sip_event),
	     SIPTAG_CONTENT_TYPE_STR("application/pidf+xml"),
	     SIPTAG_PAYLOAD_STR(presence_closed),
	     NUTAG_SUBSTATE(nua_substate_pending),
	     TAG_END());
    }
    return 0;

  case nua_r_notify:
    return status >= 200;

  default:
    return 0;
  }
}

extern int save_until_notified_and_responded(CONDITION_PARAMS);
extern int save_until_notified(CONDITION_PARAMS);

int test_subscribe_notify(struct context *ctx)
{
  BEGIN();

  struct endpoint *a = &ctx->a,  *b = &ctx->b;
  struct call *a_call = a->call, *b_call = b->call;
  struct event *e;
  sip_t const *sip;
  tagi_t const *n_tags, *r_tags;

  if (print_headings)
    printf("TEST NUA-11.4: establishing subscription without notifier\n");

  TEST_1(a_call->nh = nua_handle(a->nua, a_call, SIPTAG_TO(b->to), TAG_END()));

  SUBSCRIBE(a, a_call, a_call->nh, NUTAG_URL(b->contact->m_url),
	    SIPTAG_EVENT_STR("presence"),
	    SIPTAG_ACCEPT_STR("application/xpidf, application/pidf+xml"),
	    TAG_END());

  run_ab_until(ctx, -1, save_until_notified_and_responded,
	       -1, accept_and_notify);

  /* Client events:
     nua_subscribe(), nua_i_notify/nua_r_subscribe
  */
  TEST_1(e = a->events->head);
  if (e->data->e_event == nua_i_notify) {
    TEST_E(e->data->e_event, nua_i_notify);
    TEST_1(sip = sip_object(e->data->e_msg));
    n_tags = e->data->e_tags;
    TEST_1(e = e->next); TEST_E(e->data->e_event, nua_r_subscribe);
    r_tags = e->data->e_tags;
    TEST_1(tl_find(r_tags, nutag_substate));
    TEST(tl_find(r_tags, nutag_substate)->t_value, nua_substate_pending);
  }
  else {
    TEST_E(e->data->e_event, nua_r_subscribe);
    TEST(e->data->e_status, 202);
    r_tags = e->data->e_tags;
    TEST_1(tl_find(r_tags, nutag_substate));
    TEST(tl_find(r_tags, nutag_substate)->t_value, nua_substate_embryonic);
    TEST_1(e = e->next); TEST_E(e->data->e_event, nua_i_notify);
    TEST_1(sip = sip_object(e->data->e_msg));
    n_tags = e->data->e_tags;
  }
  TEST_1(sip->sip_event); TEST_S(sip->sip_event->o_type, "presence");
  TEST_1(sip->sip_content_type);
  TEST_S(sip->sip_content_type->c_type, "application/pidf+xml");
  TEST_1(sip->sip_subscription_state);
  TEST_S(sip->sip_subscription_state->ss_substate, "pending");
  TEST_1(sip->sip_subscription_state->ss_expires);
  TEST_1(tl_find(n_tags, nutag_substate));
  TEST(tl_find(n_tags, nutag_substate)->t_value, nua_substate_pending);
  TEST_1(!e->next);
  free_events_in_list(ctx, a->events);

  /* Server events: nua_i_subscribe, nua_r_notify */
  TEST_1(e = b->events->head);
  TEST_E(e->data->e_event, nua_i_subscribe);
  TEST_E(e->data->e_status, 100);
  TEST_1(sip = sip_object(e->data->e_msg));

  TEST_1(e = e->next); TEST_E(e->data->e_event, nua_r_notify);
  r_tags = e->data->e_tags;
  TEST_1(tl_find(r_tags, nutag_substate));
  TEST(tl_find(r_tags, nutag_substate)->t_value, nua_substate_pending);

  free_events_in_list(ctx, b->events);

  if (print_headings)
    printf("TEST NUA-11.4: PASSED\n");

  /* ---------------------------------------------------------------------- */

/* NOTIFY with updated content

   A			B
   |                    |
   |<------NOTIFY-------|
   |-------200 OK------>|
   |                    |
*/
  if (print_headings)
    printf("TEST NUA-11.5: send NOTIFY\n");

  /* Update presence data */

  NOTIFY(b, b_call, b_call->nh,
	 NUTAG_SUBSTATE(nua_substate_active),
	 SIPTAG_EVENT_STR("presence"),
	 SIPTAG_CONTENT_TYPE_STR("application/pidf+xml"),
	 SIPTAG_PAYLOAD_STR(presence_open),
	 TAG_END());

  run_ab_until(ctx, -1, save_until_notified,
	       -1, save_until_final_response);

  /* subscriber events:
     nua_i_notify
  */
  TEST_1(e = a->events->head); TEST_E(e->data->e_event, nua_i_notify);
  TEST_1(sip = sip_object(e->data->e_msg));
  TEST_1(sip->sip_event); TEST_S(sip->sip_event->o_type, "presence");
  TEST_1(sip->sip_content_type);
  TEST_S(sip->sip_content_type->c_type, "application/pidf+xml");
  TEST_1(sip->sip_subscription_state);
  TEST_S(sip->sip_subscription_state->ss_substate, "active");
  TEST_1(sip->sip_subscription_state->ss_expires);
  n_tags = e->data->e_tags;
  TEST_1(tl_find(n_tags, nutag_substate));
  TEST(tl_find(n_tags, nutag_substate)->t_value, nua_substate_active);
  TEST_1(!e->next);
  free_events_in_list(ctx, a->events);

  /* Notifier events: nua_r_notify */
  TEST_1(e = b->events->head); TEST_E(e->data->e_event, nua_r_notify);
  r_tags = e->data->e_tags;
  TEST_1(tl_find(r_tags, nutag_substate));
  TEST(tl_find(r_tags, nutag_substate)->t_value, nua_substate_active);

  free_events_in_list(ctx, b->events);

  if (print_headings)
    printf("TEST NUA-11.5: PASSED\n");

  /* ---------------------------------------------------------------------- */

/* un-SUBSCRIBE

   A			B
   |                    |
   |------SUBSCRIBE---->|
   |<--------202--------|
   |<------NOTIFY-------|
   |-------200 OK------>|
   |                    |
*/
  if (print_headings)
    printf("TEST NUA-11.6: un-SUBSCRIBE\n");

  UNSUBSCRIBE(a, a_call, a_call->nh, TAG_END());

  run_ab_until(ctx, -1, save_until_final_response,
	       -1, save_until_final_response);

  /* Client events:
     nua_unsubscribe(), nua_i_notify/nua_r_unsubscribe
  */
  TEST_1(e = a->events->head);
  if (e->data->e_event == nua_i_notify) {
    TEST_E(e->data->e_event, nua_i_notify);
    n_tags = e->data->e_tags;
    TEST_1(sip = sip_object(e->data->e_msg));
    TEST_1(sip->sip_event);
    TEST_1(sip->sip_subscription_state);
    TEST_S(sip->sip_subscription_state->ss_substate, "terminated");
    TEST_1(!sip->sip_subscription_state->ss_expires);
    TEST_1(tl_find(n_tags, nutag_substate));
    TEST(tl_find(n_tags, nutag_substate)->t_value, nua_substate_terminated);
    TEST_1(e = e->next); TEST_E(e->data->e_event, nua_r_unsubscribe);
    TEST(e->data->e_status, 200);
    r_tags = e->data->e_tags;
    TEST_1(tl_find(r_tags, nutag_substate));
    TEST(tl_find(r_tags, nutag_substate)->t_value, nua_substate_terminated);
  }
  else {
    TEST_E(e->data->e_event, nua_r_unsubscribe);
    TEST(e->data->e_status, 200);
    r_tags = e->data->e_tags;
    TEST_1(tl_find(r_tags, nutag_substate));
    TEST(tl_find(r_tags, nutag_substate)->t_value, nua_substate_terminated);
  }
  TEST_1(!e->next);
  free_events_in_list(ctx, a->events);

  /* Notifier events: nua_r_notify */
  TEST_1(e = b->events->head);
  TEST_E(e->data->e_event, nua_i_subscribe);
  TEST(e->data->e_status, 200);
  TEST_1(sip = sip_object(e->data->e_msg));
  TEST_1(tl_find(e->data->e_tags, nutag_substate));
  TEST(tl_find(e->data->e_tags, nutag_substate)->t_value, 
       nua_substate_terminated);
  TEST_1(e = e->next); TEST_E(e->data->e_event, nua_r_notify);
  TEST_1(e->data->e_status >= 200);
  r_tags = e->data->e_tags;
  TEST_1(tl_find(r_tags, nutag_substate));
  TEST(tl_find(r_tags, nutag_substate)->t_value, nua_substate_terminated);

  free_events_in_list(ctx, b->events);

  nua_handle_destroy(a_call->nh), a_call->nh = NULL;
  nua_handle_destroy(b_call->nh), b_call->nh = NULL;

  if (print_headings)
    printf("TEST NUA-11.6: PASSED\n");

  END();
}

/* ======================================================================== */
/* Test simple methods: MESSAGE, PUBLISH, SUBSCRIBE/NOTIFY */

int test_simple(struct context *ctx)
{
  return
    test_message(ctx)
    || test_publish(ctx)
    || test_subscribe_notify(ctx)
    ;
}

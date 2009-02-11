/*
 * This file is part of the Sofia-SIP package
 *
 * Copyright (C) 2008 Nokia Corporation.
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

/**@CFILE check_events.c
 *
 * @brief NUA module tests for SIP events
 *
 * @author Pekka Pessi <Pekka.Pessi@nokia.com>
 *
 * @copyright (C) 2008 Nokia Corporation.
 */

#include "config.h"

#include "check_nua.h"

#include "test_s2.h"

#include <sofia-sip/sip_status.h>
#include <sofia-sip/sip_header.h>
#include <sofia-sip/soa.h>
#include <sofia-sip/su_tagarg.h>
#include <sofia-sip/su_string.h>
#include <sofia-sip/su_tag_io.h>

#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* define XXX as 1 in order to see all failing test cases */
#ifndef XXX
#define XXX (0)
#endif

/* ====================================================================== */

static nua_t *nua;
static soa_session_t *soa = NULL;
static struct dialog *dialog = NULL;

#define CRLF "\r\n"

void s2_dialog_setup(void)
{
  nua = s2_nua_setup("simple",
		     SIPTAG_ORGANIZATION_STR("Pussy Galore's Flying Circus"),
                     NUTAG_OUTBOUND("no-options-keepalive, no-validate"),
		     TAG_END());

  soa = soa_create(NULL, s2->root, NULL);

  fail_if(!soa);

  soa_set_params(soa,
		 SOATAG_USER_SDP_STR("m=audio 5008 RTP/AVP 8 0" CRLF
				     "m=video 5010 RTP/AVP 34" CRLF),
		 TAG_END());

  dialog = su_home_new(sizeof *dialog); fail_if(!dialog);

  s2_register_setup();
}

void s2_dialog_teardown(void)
{
  s2_teardown_started("simple");

  s2_register_teardown();

  nua_shutdown(nua);

  fail_unless(s2_check_event(nua_r_shutdown, 200));

  s2_nua_teardown();
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

static char const *event_type = "presence";
static char const *event_mime_type = "application/pidf+xml";
static char const *event_state = presence_open;
static char const *subscription_state = "active;expires=600";

static struct event *
respond_to_subscribe(struct message *subscribe,
		     nua_event_t expect_event,
		     enum nua_substate expect_substate,
		     int status, char const *phrase,
		     tag_type_t tag, tag_value_t value, ...)
{
  struct event *event;
  ta_list ta;

  ta_start(ta, tag, value);
  s2_respond_to(subscribe, dialog, status, phrase,
		ta_tags(ta));
  ta_end(ta);

  event = s2_wait_for_event(expect_event, status); fail_if(!event);
  fail_unless(s2_check_substate(event, expect_substate));
  return event;
}

static struct event *
notify_to_nua(enum nua_substate expect_substate,
	      tag_type_t tag, tag_value_t value, ...)
{
  struct event *event;
  struct message *response;
  ta_list ta;

  ta_start(ta, tag, value);
  fail_if(s2_request_to(dialog, SIP_METHOD_NOTIFY, NULL,
			SIPTAG_CONTENT_TYPE_STR(event_mime_type),
			SIPTAG_PAYLOAD_STR(event_state),
			ta_tags(ta)));
  ta_end(ta);

  response = s2_wait_for_response(200, SIP_METHOD_NOTIFY);
  fail_if(!response);
  s2_free_message(response);

  event = s2_wait_for_event(nua_i_notify, 200); fail_if(!event);
  fail_unless(s2_check_substate(event, expect_substate));

  return event;
}

static int send_notify_before_response = 0;

static struct event *
subscription_by_nua(nua_handle_t *nh,
		    enum nua_substate current,
		    tag_type_t tag, tag_value_t value, ...)
{
  struct message *subscribe;
  struct event *notify, *event;
  ta_list ta;
  enum nua_substate substate = nua_substate_active;
  char const *substate_str = subscription_state;
  char const *expires = "600";

  subscribe = s2_wait_for_request(SIP_METHOD_SUBSCRIBE);
  if (event_type)
    fail_if(!subscribe->sip->sip_event ||
	    strcmp(event_type, subscribe->sip->sip_event->o_type));

  if (subscribe->sip->sip_expires && subscribe->sip->sip_expires->ex_delta == 0) {
    substate = nua_substate_terminated;
    substate_str = "terminated;reason=timeout";
    expires = "0";
  }

  ta_start(ta, tag, value);

  if (send_notify_before_response) {
    s2_save_uas_dialog(dialog, subscribe->sip);
    notify = notify_to_nua(substate,
			   SIPTAG_EVENT(subscribe->sip->sip_event),
			   SIPTAG_SUBSCRIPTION_STATE_STR(substate_str),
			   ta_tags(ta));
    event = respond_to_subscribe(subscribe, nua_r_subscribe, substate,
				 SIP_200_OK,
				 SIPTAG_EXPIRES_STR(expires),
				 TAG_END());
    s2_free_event(event);
  }
  else {
    event = respond_to_subscribe(subscribe, nua_r_subscribe, current,
				 SIP_202_ACCEPTED,
				 SIPTAG_EXPIRES_STR(expires),
				 TAG_END());
    s2_free_event(event);
    notify = notify_to_nua(substate,
			   SIPTAG_EVENT(subscribe->sip->sip_event),
			   SIPTAG_SUBSCRIPTION_STATE_STR(substate_str),
			   ta_tags(ta));
  }

  s2_free_message(subscribe);

  return notify;
}

static void
unsubscribe_by_nua(nua_handle_t *nh, tag_type_t tag, tag_value_t value, ...)
{
  struct message *subscribe, *response;
  struct event *event;

  nua_unsubscribe(nh, TAG_END());
  subscribe = s2_wait_for_request(SIP_METHOD_SUBSCRIBE);

  s2_respond_to(subscribe, dialog, SIP_200_OK, SIPTAG_EXPIRES_STR("0"), TAG_END());

  event = s2_wait_for_event(nua_r_unsubscribe, 200); fail_if(!event);
  fail_unless(s2_check_substate(event, nua_substate_active));
  s2_free_event(event);

  fail_if(s2_request_to(dialog, SIP_METHOD_NOTIFY, NULL,
			SIPTAG_EVENT(subscribe->sip->sip_event),
			SIPTAG_SUBSCRIPTION_STATE_STR("terminated;reason=tiemout"),
			SIPTAG_CONTENT_TYPE_STR(event_mime_type),
			SIPTAG_PAYLOAD_STR(event_state),
			TAG_END()));

  event = s2_wait_for_event(nua_i_notify, 200); fail_if(!event);
  fail_unless(s2_check_substate(event, nua_substate_terminated));
  s2_free_event(event);

  response = s2_wait_for_response(200, SIP_METHOD_NOTIFY);
  fail_if(!response);
  s2_free_message(response); s2_free_message(subscribe);
}

/* ====================================================================== */
/* 6 - Subscribe/notify */

START_TEST(subscribe_6_1_1)
{
  nua_handle_t *nh;
  struct event *notify;
  s2_case("6.1.1", "Basic subscription",
	  "NUA sends SUBSCRIBE, waits for NOTIFY, sends un-SUBSCRIBE");

  nh = nua_handle(nua, NULL, SIPTAG_TO(s2->local), TAG_END());
  nua_subscribe(nh, SIPTAG_EVENT_STR(event_type), TAG_END());
  notify = subscription_by_nua(nh, nua_substate_embryonic, TAG_END());
  s2_free_event(notify);
  unsubscribe_by_nua(nh, TAG_END());
  nua_handle_destroy(nh);
}
END_TEST

START_TEST(subscribe_6_1_2)
{
  nua_handle_t *nh;
  struct message *subscribe, *response;
  struct event *notify, *event;

  s2_case("6.1.2", "Basic subscription with refresh",
	  "NUA sends SUBSCRIBE, waits for NOTIFY, "
	  "sends re-SUBSCRIBE, waits for NOTIFY, "
	  "sends un-SUBSCRIBE");

  send_notify_before_response = 1;

  nh = nua_handle(nua, NULL, SIPTAG_TO(s2->local), TAG_END());
  nua_subscribe(nh, SIPTAG_EVENT_STR(event_type), TAG_END());
  notify = subscription_by_nua(nh, nua_substate_embryonic, TAG_END());
  s2_free_event(notify);

  /* Wait for refresh */
  s2_fast_forward(600, s2->root);;
  subscribe = s2_wait_for_request(SIP_METHOD_SUBSCRIBE);
  s2_respond_to(subscribe, dialog, SIP_200_OK,
		SIPTAG_EXPIRES_STR("600"),
		TAG_END());

  event = s2_wait_for_event(nua_r_subscribe, 200); fail_if(!event);
  fail_unless(s2_check_substate(event, nua_substate_active));
  s2_free_event(event);

  fail_if(s2_request_to(dialog, SIP_METHOD_NOTIFY, NULL,
			SIPTAG_EVENT(subscribe->sip->sip_event),
			SIPTAG_SUBSCRIPTION_STATE_STR("active;expires=600"),
			SIPTAG_CONTENT_TYPE_STR(event_mime_type),
			SIPTAG_PAYLOAD_STR(event_state),
			TAG_END()));
  event = s2_wait_for_event(nua_i_notify, 200); fail_if(!event);
  fail_unless(s2_check_substate(event, nua_substate_active));
  s2_free_event(event);
  response = s2_wait_for_response(200, SIP_METHOD_NOTIFY);
  fail_if(!response);
  s2_free_message(response);

  unsubscribe_by_nua(nh, TAG_END());

  nua_handle_destroy(nh);
}
END_TEST

START_TEST(subscribe_6_1_3)
{
  nua_handle_t *nh;
  struct message *response;
  struct event *notify, *event;

  s2_case("6.1.3", "Subscription terminated by notifier",
	  "NUA sends SUBSCRIBE, waits for NOTIFY, "
	  "gets NOTIFY terminating the subscription,");

  nh = nua_handle(nua, NULL, SIPTAG_TO(s2->local), TAG_END());
  nua_subscribe(nh, SIPTAG_EVENT_STR(event_type), TAG_END());
  notify = subscription_by_nua(nh, nua_substate_embryonic, TAG_END());
  s2_free_event(notify);

  fail_if(s2_request_to(dialog, SIP_METHOD_NOTIFY, NULL,
			SIPTAG_EVENT_STR(event_type),
			SIPTAG_SUBSCRIPTION_STATE_STR("terminated;reason=noresource"),
			TAG_END()));
  event = s2_wait_for_event(nua_i_notify, 200); fail_if(!event);
  fail_unless(s2_check_substate(event, nua_substate_terminated));
  s2_free_event(event);
  response = s2_wait_for_response(200, SIP_METHOD_NOTIFY);
  fail_if(!response);
  s2_free_message(response);

  nua_handle_destroy(nh);
}
END_TEST

START_TEST(subscribe_6_1_4)
{
  nua_handle_t *nh;
  struct message *response;
  struct event *notify, *event;

  s2_case("6.1.4", "Subscription terminated by notifier, re-established",
	  "NUA sends SUBSCRIBE, waits for NOTIFY, "
	  "gets NOTIFY terminating the subscription,");

  nh = nua_handle(nua, NULL, SIPTAG_TO(s2->local), TAG_END());
  nua_subscribe(nh, SIPTAG_EVENT_STR(event_type), TAG_END());
  notify = subscription_by_nua(nh, nua_substate_embryonic, TAG_END());
  s2_free_event(notify);

  fail_if(s2_request_to(dialog, SIP_METHOD_NOTIFY, NULL,
			SIPTAG_EVENT_STR(event_type),
			SIPTAG_SUBSCRIPTION_STATE_STR("terminated;reason=deactivated"),
			TAG_END()));
  event = s2_wait_for_event(nua_i_notify, 200); fail_if(!event);
  fail_unless(s2_check_substate(event, nua_substate_embryonic));
  s2_free_event(event);
  response = s2_wait_for_response(200, SIP_METHOD_NOTIFY);
  fail_if(!response);
  s2_free_message(response);

  su_home_unref((void *)dialog), dialog = su_home_new(sizeof *dialog); fail_if(!dialog);

  s2_fast_forward(5, s2->root);;
  /* nua re-establishes the subscription */
  notify = subscription_by_nua(nh, nua_substate_embryonic, TAG_END());
  s2_free_event(notify);

  /* Unsubscribe with nua_subscribe() Expires: 0 */
  nua_subscribe(nh, SIPTAG_EVENT_STR(event_type), SIPTAG_EXPIRES_STR("0"), TAG_END());
  notify = subscription_by_nua(nh, nua_substate_active, TAG_END());
  s2_free_event(notify);

  nua_handle_destroy(nh);
}
END_TEST

TCase *subscribe_tcase(void)
{
  TCase *tc = tcase_create("6.1 - Basic SUBSCRIBE_");
  tcase_add_checked_fixture(tc, s2_dialog_setup, s2_dialog_teardown);
  {
    tcase_add_test(tc, subscribe_6_1_1);
    tcase_add_test(tc, subscribe_6_1_2);
    tcase_add_test(tc, subscribe_6_1_3);
    tcase_add_test(tc, subscribe_6_1_4);
  }
  return tc;
}

START_TEST(fetch_6_2_1)
{
  nua_handle_t *nh;
  struct event *notify;

  s2_case("6.2.1", "Event fetch - NOTIFY after 202",
	  "NUA sends SUBSCRIBE with Expires 0, waits for NOTIFY");

  nh = nua_handle(nua, NULL, SIPTAG_TO(s2->local), TAG_END());
  nua_subscribe(nh, SIPTAG_EVENT_STR(event_type), SIPTAG_EXPIRES_STR("0"), TAG_END());
  notify = subscription_by_nua(nh, nua_substate_embryonic, TAG_END());
  s2_check_substate(notify, nua_substate_terminated);
  s2_free_event(notify);
  nua_handle_destroy(nh);
}
END_TEST

START_TEST(fetch_6_2_2)
{
  nua_handle_t *nh;
  struct event *notify;

  s2_case("6.2.2", "Event fetch - NOTIFY before 200",
	  "NUA sends SUBSCRIBE with Expires 0, waits for NOTIFY");

  send_notify_before_response = 1;

  nh = nua_handle(nua, NULL, SIPTAG_TO(s2->local), TAG_END());
  nua_subscribe(nh, SIPTAG_EVENT_STR(event_type), SIPTAG_EXPIRES_STR("0"), TAG_END());
  notify = subscription_by_nua(nh, nua_substate_embryonic, TAG_END());
  s2_check_substate(notify, nua_substate_terminated);
  s2_free_event(notify);
  nua_handle_destroy(nh);
}
END_TEST

START_TEST(fetch_6_2_3)
{
  nua_handle_t *nh;
  struct message *subscribe;
  struct event *event;

  s2_case("6.2.3", "Event fetch - no NOTIFY",
	  "NUA sends SUBSCRIBE with Expires 0, waits for NOTIFY, times out");

  nh = nua_handle(nua, NULL, SIPTAG_TO(s2->local), TAG_END());
  nua_subscribe(nh, SIPTAG_EVENT_STR(event_type), SIPTAG_EXPIRES_STR("0"), TAG_END());
  subscribe = s2_wait_for_request(SIP_METHOD_SUBSCRIBE);
  s2_respond_to(subscribe, dialog, SIP_202_ACCEPTED,
		SIPTAG_EXPIRES_STR("0"), TAG_END());
  s2_free_message(subscribe);

  event = s2_wait_for_event(nua_r_subscribe, 202); fail_if(!event);
  fail_unless(s2_check_substate(event, nua_substate_embryonic));
  s2_free_event(event);

  s2_fast_forward(600, s2->root);;

  event = s2_wait_for_event(nua_i_notify, 408); fail_if(!event);
  fail_unless(s2_check_substate(event, nua_substate_terminated));
  s2_free_event(event);

  nua_handle_destroy(nh);
}
END_TEST


TCase *fetch_tcase(void)
{
  TCase *tc = tcase_create("6.2 - Event fetch");
  tcase_add_checked_fixture(tc, s2_dialog_setup, s2_dialog_teardown);
  {
    tcase_add_test(tc, fetch_6_2_1);
    tcase_add_test(tc, fetch_6_2_2);
    tcase_add_test(tc, fetch_6_2_3);
  }
  return tc;
}

/* ====================================================================== */

/* Test case template */

START_TEST(empty)
{
  s2_case("0.0.0", "Empty test case",
	  "Detailed explanation for empty test case.");

  tport_set_params(s2->master, TPTAG_LOG(1), TAG_END());
  s2_setup_logs(7);
  s2_setup_logs(0);
  tport_set_params(s2->master, TPTAG_LOG(0), TAG_END());
}

END_TEST

static TCase *empty_tcase(void)
{
  TCase *tc = tcase_create("0 - Empty");
  tcase_add_checked_fixture(tc, s2_dialog_setup, s2_dialog_teardown);
  tcase_add_test(tc, empty);

  return tc;
}

/* ====================================================================== */

void check_simple_cases(Suite *suite)
{
  suite_add_tcase(suite, subscribe_tcase());
  suite_add_tcase(suite, fetch_tcase());

  if (0)			/* Template */
    suite_add_tcase(suite, empty_tcase());
}


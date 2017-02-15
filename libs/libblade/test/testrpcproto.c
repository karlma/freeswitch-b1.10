#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-function"


#include <blade_rpcproto.h>

#pragma GCC optimize ("O0")


ks_pool_t *pool;
ks_thread_pool_t *tpool;


static ks_thread_t *threads[10];

static char idbuffer[51];



static enum jrpc_status_t  process_widget(cJSON *msg, cJSON **response)
{
    printf("entering process_widget\n");

    char *b0 = cJSON_PrintUnformatted(msg);
    printf("Request: %s\n", b0);
    ks_pool_free(pool, &b0);

    cJSON *resp = cJSON_CreateObject();
    cJSON_AddNumberToObject(resp, "code", 199);

    //ks_rpcmessageid_t msgid = ks_rpcmessage_create_response(msg, &resp, response);
	ks_rpcmessageid_t msgid = blade_rpc_create_response(msg, &resp, response);

    char *b1 = cJSON_PrintUnformatted(*response);   //(*response);
    printf("Response: msgid %d\n%s\n", msgid, b1);
    ks_pool_free(pool, &b1);

    printf("exiting process_wombat\n");

    return JRPC_SEND;
}


static enum jrpc_status_t  process_widget_response(cJSON *request, cJSON **msg)
{
	printf("entering process_widget_response\n");
	printf("exiting process_widget_response\n");
	return JRPC_PASS;
}



static enum jrpc_status_t  process_wombat(cJSON *msg, cJSON **replyP)
{
	printf("entering process_wombat\n");
	
	char *b0 = cJSON_PrintUnformatted(msg);
	printf("\nRequest: %s\n\n", b0);
	ks_pool_free(pool, &b0);

	cJSON *result = cJSON_CreateObject();
	cJSON_AddNumberToObject(result, "code", 99);
	cJSON *response;

//    ks_rpcmessageid_t msgid = ks_rpcmessage_create_response(msg, &result, &response);
	ks_rpcmessageid_t msgid = blade_rpc_create_response(msg, &result, &response);

	cJSON *response_copy = cJSON_Duplicate(response, 1);
    blade_rpc_process_jsonmessage(response_copy);

    if (msgid != 0) {
	    char *b1 = cJSON_PrintUnformatted(response);   //(*response);
	    printf("\nResponse: msgid %d\n%s\n\n", msgid, b1);
        blade_rpc_write_json(response);
		ks_pool_free(pool, &b1);
    }
    else {
        printf("process_wombat: unable to create response \n");
        return JRPC_ERROR;
    }

	blade_rpc_fields_t *r_fields;

	char *r_method;
	char *r_namespace;
	char *r_version;
	uint32_t r_id;

	ks_status_t s1 = blade_rpc_parse_message(msg, &r_namespace, &r_method, &r_version, &r_id, &r_fields);

	if (s1 == KS_STATUS_FAIL) {
		printf("process_wombat:  blade_rpc_parse_message failed\n");
		return JRPC_ERROR;
	}
	
	printf("\nprocess_wombat:  blade_rpc_parse_message namespace %s,  method %s, id %d,  version %s,  to %s, from %s,  token %s\n\n",
											r_namespace, r_method, r_id, r_version,
											r_fields->to, r_fields->from, r_fields->token);

	cJSON *parms2 = NULL;

    char to[] = "tony@freeswitch.com/laptop?transport=wss&host=server1.freeswitch.com&port=1234";
    char from[] = "colm@freeswitch.com/laptop?transport=wss&host=server2.freeswitch.com&port=4321";
    char token[] = "abcdefhgjojklmnopqrst";

    blade_rpc_fields_t  fields;
    fields.to = to;
    fields.from = from;
    fields.token = token;

//	msgid = ks_rpcmessage_create_request("app1", "widget", &parms2, replyP);
	msgid = blade_rpc_create_request(r_namespace, r_method, &fields, NULL, replyP);

	if (!msgid) {
		printf("process wombat:  create of next request failed\n");
		return  JRPC_ERROR;
	}

	b0 = cJSON_PrintUnformatted(*replyP);
	
	if (!b0) {	
        printf("process wombat:  create of next request cannot be formatted\n");
        return  JRPC_ERROR;
    }


	printf("\nprocess wombat: next request\n%s\n\n", b0);    
	

    printf("\n\nexiting process_wombat with a reply to send\n");

	return JRPC_SEND; 
}

static enum jrpc_status_t  process_wombat_prerequest(cJSON *request, cJSON **msg)
{
    printf("entering process_wombat_prerequest\n");
    printf("exiting process_wombat_prerequest\n");
    return JRPC_SEND;
}

static enum jrpc_status_t  process_wombat_postrequest(cJSON *request, cJSON **msg)
{
    printf("entering process_wombat_postrequest\n");
    printf("exiting process_wombat_postrequest\n");
    return JRPC_PASS;
}



static enum jrpc_status_t  process_wombat_response(cJSON *request, cJSON **msg)
{
	printf("entering process_wombat_response\n");
	printf("exiting process_wombat_response\n");
	return JRPC_PASS;
}

static enum jrpc_status_t  process_wombat_preresponse(cJSON *request, cJSON **msg)
{

    printf("entering process_wombat_preresponse\n");

	cJSON *response = NULL;
	cJSON *result = NULL;

	cJSON *parms2 = NULL;

	//ks_rpcmessageid_t msgid = ks_rpcmessage_create_request("app1", "widget", &parms2, msg);	

    printf("exiting process_wombat_preresponse\n");
    return JRPC_SEND;
}

static enum jrpc_status_t  process_wombat_postresponse(cJSON *request, cJSON **msg)
{
    printf("entering process_postwombat_response\n");
    printf("exiting process_postwombat_response\n");
    return JRPC_PASS;
}




static enum jrpc_status_t  process_badbunny( cJSON *msg, cJSON **response)
{
    printf("entering process_badbunny\n");

    char *b0 = cJSON_PrintUnformatted(msg);
    printf("\nRequest: %s\n\n", b0);
    ks_pool_free(pool, &b0);

    cJSON *respvalue;

    ks_rpcmessageid_t msgid = ks_rpcmessage_create_errorresponse(msg, &respvalue, response);

    char *b2 = cJSON_PrintUnformatted(*response);
    printf("\nRequest: msgid %d\n%s\n\n", msgid, b2);
    ks_pool_free(pool, &b2);

	//cJSON *respvalue = cJSON_CreateNumber(1);
    	

    char *b1 = cJSON_PrintUnformatted(*response);   //(*response);
    printf("\nResponse: %s\n\n", b1);
    ks_pool_free(pool, &b1);

    printf("exiting process_badbunny\n");


    return JRPC_SEND;
}


void test01()
{
	printf("**** testrpcmessages - test01 start\n"); fflush(stdout);

	blade_rpc_declare_template("temp1", "1.0");

	blade_rpc_register_template_function("temp1", "widget", process_widget, process_widget_response); 

    blade_rpc_declare_namespace("app1", "1.0");

	blade_rpc_register_function("app1", "wombat", process_wombat, process_wombat_response); 

    blade_rpc_register_custom_request_function("app1", "wombat", process_wombat_prerequest, process_wombat_postresponse);
    blade_rpc_register_custom_response_function("app1", "wombat", process_wombat_preresponse, process_wombat_postresponse);


	/* message 1 */
	/* --------- */
    cJSON* request1 = NULL;
    cJSON* parms1   = NULL;

	printf("\n\n\n - test01 message1 - basic message\n\n\n");

	ks_rpcmessageid_t msgid = ks_rpcmessage_create_request("app1", "wombat", &parms1, &request1);
	if (msgid == 0) {
		printf("test01.1: unable to create message 1\n");
		return;
	}	

	if (!request1) {
		printf("test01.1: No json returned from create request 1\n");
		return;
	}

    cJSON_AddStringToObject(parms1, "hello", "cruel world");

	char *pdata = cJSON_PrintUnformatted(request1);

	if (!pdata) {
		printf("test01.1: unable to parse cJSON object\n");
		return;
	}

	printf("test01 request:\n%s\n", pdata);


	blade_rpc_process_jsonmessage(request1);

	cJSON_Delete(request1);

	ks_pool_free(pool, &pdata);

	printf("\ntest01.1 complete\n");

	
	/* message 2 */
	/* --------- */

    printf("\n\n\n test01 - message2 - test inherit\n\n\n");

	blade_rpc_inherit_template("app1", "temp1");

    cJSON* request2 = NULL;
    cJSON* parms2   = cJSON_CreateObject();

    cJSON_AddStringToObject(parms2, "hello2", "cruel world once again");

    msgid = ks_rpcmessage_create_request("app1", "temp1.widget", &parms2, &request2); 
	if (msgid == 0) {
		printf("test01.2: failed to create a wombat\n");
		return;
	}

    if (!request2) {
        printf("test01.2: No json returned from create request 1\n");
        return;
    }

    pdata = cJSON_PrintUnformatted(request2);

	if (!pdata) {
		printf("test01.2: unable to parse cJSON object\n");
		return;
	}

    printf("\ntest01 request:\n%s\n\n\n", pdata);

    blade_rpc_process_jsonmessage(request2);

    cJSON_Delete(request2);
    ks_pool_free(pool, &pdata);

    printf("\ntest01.2 complete\n");

	return;

}

void test02()
{
	printf("**** testrpcmessages - test02 start\n"); fflush(stdout);

    blade_rpc_declare_namespace("app2", "1.0");

    blade_rpc_register_function("app2", "wombat", process_wombat, process_wombat_response);

    blade_rpc_inherit_template("app2", "temp1");

    blade_rpc_register_custom_request_function("app2", "wombat", process_wombat_prerequest, process_wombat_postresponse);
    blade_rpc_register_custom_response_function("app2", "wombat", process_wombat_preresponse, process_wombat_postresponse);

    blade_rpc_register_function("app2", "bunny", process_badbunny, NULL);

	char to[] = "tony@freeswitch.com/laptop?transport=wss&host=server1.freeswitch.com&port=1234";
	char from[] = "colm@freeswitch.com/laptop?transport=wss&host=server2.freeswitch.com&port=4321";
	char token[] = "abcdefhgjojklmnopqrst";

	blade_rpc_fields_t  fields;
	fields.to = to;
	fields.from = from;
	fields.token = token;


	/* test the 4 different ways to handle param messages */

	cJSON *params1 = NULL;
	cJSON *request1;

	ks_rpcmessageid_t msgid =  blade_rpc_create_request("app2", "temp1.widget2", &fields, &params1, &request1); 

	if (!msgid) {
		 printf("test02.1: create_request failed\n");
		return;
	}

    cJSON_AddStringToObject(params1, "hello", "cruel world");

    char *pdata = cJSON_PrintUnformatted(request1);

    if (!pdata) {
        printf("test02.1: unable to parse cJSON object\n");
        return;
    }

	printf("\ntest02.1 request:\n\n%s\n\n\n", pdata);

	printf("\n\n -----------------------------------------\n\n");

	ks_status_t s1 = blade_rpc_process_jsonmessage(request1);
	if (s1 == KS_STATUS_FAIL) {
		printf("test02.1:  process request1 failed\n");
		return;
	}
 
	printf(" -----------------------------------------\n\n\n\n");

	ks_pool_free(pool, &pdata);

	cJSON *reply1 = NULL;
	cJSON *response1 = NULL;

	ks_rpcmessageid_t msgid2 = blade_rpc_create_response(request1, &reply1, &response1);

	if (!msgid2) {
		printf("test02.1: create_response failed\n");
		return;
	}

	cJSON_AddNumberToObject(reply1, "code", 10);
	cJSON_AddStringToObject(reply1, "farewell", "cruel server");

	pdata = cJSON_PrintUnformatted(response1);

	if (!pdata) {
        printf("test02.1: unable to parse cJSON response object\n");
        return;
	}

    printf("\ntest02.1 response:\n\n%s\n\n\n", pdata);

    printf("\n\n -----------------------------------------\n\n");

    s1 = blade_rpc_process_jsonmessage(response1);
    if (s1 == KS_STATUS_FAIL) {
        printf("test02.1:  process request1 failed\n");
        return;
    }

    printf(" -----------------------------------------\n\n\n\n");


	ks_pool_free(pool, &pdata);

	printf("****  testrpcmessages - test02 finished\n"); fflush(stdout);

	return;
}


void test02a()
{
	printf("**** testrpcmessages - test02a start\n"); fflush(stdout);

	char to[] = "tony@freeswitch.com/laptop?transport=wss&host=server1.freeswitch.com&port=1234";
	char from[] = "colm@freeswitch.com/laptop?transport=wss&host=server2.freeswitch.com&port=4321";
	char token[] = "abcdefhgjojklmnopqrst";

	blade_rpc_fields_t  fields;
	fields.to = to;
	fields.from = from;
	fields.token = token;


	/* test the 4 different ways to handle param messages */

	cJSON *request1;

	ks_rpcmessageid_t msgid =  blade_rpc_create_request("app2", "wombat", &fields, NULL, &request1);

	if (!msgid) {
		printf("test02.1: create_request failed\n");
		return;
	}

	char *pdata = cJSON_PrintUnformatted(request1);

	if (!pdata) {
		printf("test02.1: unable to parse cJSON object\n");
		return;
	}

	printf("\ntest02.1 request:\n\n%s\n\n\n", pdata);

    printf("\n\n -----------------------------------------\n\n");

    ks_status_t s1 = blade_rpc_process_jsonmessage(request1);
    if (s1 == KS_STATUS_FAIL) {
        printf("test02.1:  process request1 failed\n");
        return;
    }

    printf(" -----------------------------------------\n\n\n\n");



	
	ks_pool_free(pool, &pdata);

	cJSON *response1 = NULL;

	ks_rpcmessageid_t msgid2 = blade_rpc_create_response(request1, NULL, &response1);

	if (!msgid2) {
		printf("test02.1: create_response failed\n");
		return;
	}

	pdata = cJSON_PrintUnformatted(response1);

	printf("\ntest02.1 response:\n\n%s\n\n\n", pdata);

	ks_pool_free(pool, &pdata);

	printf("****  testrpcmessages - test02a finished\n\n\n"); fflush(stdout);

	return;
}


void test02b()
{
    printf("**** testrpcmessages - test02b start\n"); fflush(stdout);

    char to[] = "tony@freeswitch.com/laptop?transport=wss&host=server1.freeswitch.com&port=1234";
    char from[] = "colm@freeswitch.com/laptop?transport=wss&host=server2.freeswitch.com&port=4321";
    char token[] = "abcdefhgjojklmnopqrst";

    blade_rpc_fields_t  fields;
    fields.to = to;
    fields.from = from;
    fields.token = token;


    /* test the 4 different ways to handle param messages */

    cJSON *params1 = cJSON_CreateNumber(4321);
    cJSON *request1;

    ks_rpcmessageid_t msgid =  blade_rpc_create_request("app2", "temp1.widget", &fields, &params1, &request1);

    if (!msgid) {
         printf("test02.1: create_request failed\n");
        return;
    }

    char *pdata = cJSON_PrintUnformatted(request1);

    if (!pdata) {
        printf("test02.1: unable to parse cJSON object\n");
        return;
    }

    printf("\ntest02.1 request:\n\n%s\n\n\n", pdata);
	
	    ks_pool_free(pool, &pdata);

    cJSON *reply1 = cJSON_CreateString("successful");
    cJSON *response1 = NULL;

    ks_rpcmessageid_t msgid2 = blade_rpc_create_response(request1, &reply1, &response1);

    if (!msgid2) {
        printf("test02.1: create_response failed\n");
        return;
    }

     pdata = cJSON_PrintUnformatted(response1);

    printf("\ntest02.1 response:\n\n%s\n\n\n", pdata);

    ks_pool_free(pool, &pdata);

    printf("****  testrpcmessages - test02b finished\n"); fflush(stdout);

    return;
}


void test02c()	
{

    printf("**** testrpcmessages - test02c start\n"); fflush(stdout);

    char to[] = "tony@freeswitch.com/laptop?transport=wss&host=server1.freeswitch.com&port=1234";
    char from[] = "colm@freeswitch.com/laptop?transport=wss&host=server2.freeswitch.com&port=4321";
    char token[] = "abcdefhgjojklmnopqrst";

    blade_rpc_fields_t  fields;
    fields.to    = to;
    fields.from  = from;
    fields.token = token;


    /* test the 4 different ways to handle param messages */

    cJSON *params1 = cJSON_CreateObject();

	cJSON_AddStringToObject(params1, "string1", "here is a string");
	cJSON_AddNumberToObject(params1, "number1", 4242);

    cJSON *request1;

    ks_rpcmessageid_t msgid =  blade_rpc_create_request("app2", "bunny", &fields, &params1, &request1);

    if (!msgid) {
         printf("test02.1: create_request failed\n");
        return;
    }

    cJSON_AddStringToObject(params1, "hello", "cruel world");

    char *pdata = cJSON_PrintUnformatted(request1);

    if (!pdata) {
        printf("test02.1: unable to parse cJSON object\n");
        return;
    }

    printf("\ntest02.1 request:\n\n%s\n\n\n", pdata);
	
	    ks_pool_free(pool, &pdata);


    cJSON *reply1 = cJSON_CreateObject();
    cJSON_AddNumberToObject(reply1, "code", 10);
    cJSON_AddStringToObject(reply1, "farewell", "cruel server");


    cJSON *response1 = NULL;

    ks_rpcmessageid_t msgid2 = blade_rpc_create_response(request1, &reply1, &response1);

    if (!msgid2) {
        printf("test02.1: create_response failed\n");
        return;
    }


     pdata = cJSON_PrintUnformatted(response1);

    printf("\ntest02.1 response:\n\n%s\n\n\n", pdata);

    ks_pool_free(pool, &pdata);

    printf("****  testrpcmessages - test02c finished\n"); fflush(stdout);

    return;
}
















/* test06  */
/* ------  */

static void *testnodelocking_ex1(ks_thread_t *thread, void *data)
{
	return NULL;
}

static void *testnodelocking_ex2(ks_thread_t *thread, void *data)
{
	return NULL;
}


void test06()
{
	printf("**** testmessages - test06 start\n"); fflush(stdout);

	ks_thread_t *t0;
	ks_thread_create(&t0, testnodelocking_ex1, NULL, pool);

	ks_thread_t *t1;
	ks_thread_create(&t1, testnodelocking_ex2, NULL, pool);

	ks_thread_join(t1);
	ks_thread_join(t0);

	printf("\n\n* **testmessages - test06 -- threads complete\n\n"); fflush(stdout);

	printf("**** testmessages - test06 start\n"); fflush(stdout);

	return;
}



int main(int argc, char *argv[]) {

	printf("testmessages - start\n");

	int tests[100];
	if (argc == 0) {
		tests[0] = 1;
		tests[1] = 2;
		tests[2] = 3;
		tests[3] = 4;
		tests[4] = 5;
	}
	else {
		for(int tix=1; tix<100 && tix<argc; ++tix) {
			long i = strtol(argv[tix], NULL, 0);
			tests[tix] = i;
		}
	}

	ks_init();
	ks_global_set_default_logger(7);


	ks_status_t status;

	status = ks_pool_open(&pool);

	blade_rpc_init(pool);

    blade_rpc_declare_template("temp1", "1.0");

    blade_rpc_register_template_function("temp1", "widget", process_widget, process_widget_response);
    blade_rpc_register_template_function("temp1", "widget2", process_widget, process_widget_response);
    blade_rpc_register_template_function("temp1", "widget3", process_widget, process_widget_response);


	for (int tix=0; tix<argc; ++tix) {


		if (tests[tix] == 1) {
			test01();
			continue;
		}

		if (tests[tix] == 2) {
			test02();
			printf("\n\n");
			test02a();
            printf("\n\n");
			test02b();
            printf("\n\n");
			test02c();
            printf("\n\n");
			continue;
		}

	}

	return 0;
}

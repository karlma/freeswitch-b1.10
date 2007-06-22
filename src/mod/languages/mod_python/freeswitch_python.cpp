#include "freeswitch_python.h"

#define sanity_check(x) do { if (!session) { switch_log_printf(SWITCH_CHANNEL_LOG,SWITCH_LOG_ERROR, "session is not initalized\n"); return x;}} while(0)
#define init_vars() do { caller_profile.source = "mod_python"; swapstate = S_SWAPPED_IN; } while(0)

PySession::PySession() : CoreSession()
{
	init_vars();
}

PySession::PySession(char *uuid) : CoreSession(uuid)
{
	init_vars();
}

PySession::PySession(switch_core_session_t *new_session) : CoreSession(new_session)
{
	init_vars();
}


void PySession::setDTMFCallback(PyObject *pyfunc, char *funcargs)
{
    sanity_check();

    if (!PyCallable_Check(pyfunc)) {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "DTMF function is not a python function.\n");
	return;
    }
    Py_XINCREF(pyfunc);  
    CoreSession::setDTMFCallback((void *) pyfunc, funcargs);


}

void PySession::setHangupHook(PyObject *pyfunc) {

    if (!PyCallable_Check(pyfunc)) {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "Hangup hook is not a python function.\n");
	return;
    }

    // without this Py_XINCREF, there will be segfaults.  basically the python
    // interpreter will not know that it should not GC this object.
    // callback example: http://docs.python.org/ext/callingPython.html
    Py_XINCREF(pyfunc);  
    CoreSession::setHangupHook((void *) pyfunc);

}


void PySession::check_hangup_hook() {
   PyObject *func;
   PyObject *result;
   char *resultStr;
   bool did_swap_in = false;

   switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "check_hangup_hook called\n");

   if (!session) {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "No valid session\n");
	return;
   }

   // The did_swap_in boolean was added to fix the following problem:
   // Design flaw - we swap in threadstate based on the assumption that thread state 
   // is currently _swapped out_ when this hangup hook is called.  However, nothing known to 
   // guarantee that, and  if thread state is already swapped in when this is invoked, 
   // bad things will happen.
   switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "check hangup hook end_allow_threads\n");
   did_swap_in = end_allow_threads();
 
   if (on_hangup == NULL) {
       switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "on_hangup is null\n");
       return;
   }

   func = (PyObject *) on_hangup;

   // TODO: to match js implementation, should pass the _python_ PySession 
   // object instance wrapping this C++ PySession instance. but how do we do that?
   // for now, pass the uuid since its better than nothing
   PyObject* func_arg = Py_BuildValue("(s)", uuid);

   result = PyEval_CallObject(func, func_arg);
   Py_XDECREF(func_arg);

   if (result) {
       resultStr = (char *) PyString_AsString(result);
       // currently just ignore the result
   }
   else {
       switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Failed to call python hangup callback\n");
       PyErr_Print();
       PyErr_Clear();
   }

   switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "check hangup hook begin_allow_threads\n");
   if (did_swap_in) {
       begin_allow_threads();
   }

   Py_XDECREF(result);

}

switch_status_t PySession::run_dtmf_callback(void *input, 
					     switch_input_type_t itype) {

   PyObject *func, *arglist;
   PyObject *pyresult;
   char *resultStr;
   char *funcargs;
   switch_file_handle_t *fh = NULL;	
   bool did_swap_in = false;
 
   switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "run_dtmf_callback\n");	


   if (!cb_state.function) {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "cb_state->function is null\n");	
	return SWITCH_STATUS_FALSE;
   }

   func = (PyObject *) cb_state.function;
   if (!func) {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "cb_state->function is null\n");	
	return SWITCH_STATUS_FALSE;
   }
   else {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "cb_state->function is NOT null\n");	
   }
   if (!PyCallable_Check(func)) {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "function not callable\n");	
	return SWITCH_STATUS_FALSE;
   }

   funcargs = (char *) cb_state.funcargs;

   arglist = Py_BuildValue("(sis)", input, itype, funcargs);
   if (!arglist) {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "error building arglist");	
	return SWITCH_STATUS_FALSE;
   }

   switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "run_dtmf_callback end_allow_threads\n");
   did_swap_in = end_allow_threads();
	
   pyresult = PyEval_CallObject(func, arglist);    
   

   Py_XDECREF(arglist);                           // Trash arglist
   if (pyresult && pyresult != Py_None) {                       
       resultStr = (char *) PyString_AsString(pyresult);
       switch_status_t cbresult = process_callback_result(resultStr, &cb_state, session);
       return cbresult;
   }
   else {
       switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Error calling python callback\n");
       PyErr_Print();
       PyErr_Clear();
   }

   switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "run_dtmf_callback begin_allow_threads\n");
   if (did_swap_in) {
       begin_allow_threads();
   }

   Py_XDECREF(pyresult);

   return SWITCH_STATUS_SUCCESS;	

}

bool PySession::begin_allow_threads(void) { 
    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "PySession::begin_allow_threads() called\n");

    // swap out threadstate and store in instance variable 
    switch_channel_t *channel = switch_core_session_get_channel(session);
    PyThreadState *swapin_tstate = (PyThreadState *) switch_channel_get_private(channel, "SwapInThreadState");
    // so lets assume the thread state was swapped in when the python script was started,
    // therefore swapin_tstate will be NULL (because there is nothing to swap in, since its 
    // _already_ swapped in.)
    if (swapin_tstate == NULL) {
	// currently swapped in
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Threadstate swap-out!\n");	
	swapin_tstate = PyEval_SaveThread();
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "swapin_tstate: %p\n", swapin_tstate);	
	// give future swapper-inners something to actually swap in
	switch_channel_set_private(channel, "SwapInThreadState", (void *) swapin_tstate); 
	cb_state.threadState = threadState;  // TODO: get rid of this
	args.buf = &cb_state;     
	ap = &args;
	return true;

    }
    else {
	// currently swapped out
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "Threadstate already swapd-out! Skipping\n");	
	return false;
    }

}

bool PySession::end_allow_threads(void) { 
    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "PySession::end_allow_threads() called\n");
    // swap in threadstate from instance variable saved earlier
    switch_channel_t *channel = switch_core_session_get_channel(session);
    PyThreadState *swapin_tstate = (PyThreadState *) switch_channel_get_private(channel, "SwapInThreadState");
    if (swapin_tstate == NULL) {
	// currently swapped in
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "Threadstate double swap-in! Skipping\n");	
	return false;
    }
    else {
	// currently swapped out
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Threadstate swap-in!\n");	
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "swapin_tstate: %p\n", swapin_tstate);	
	PyEval_RestoreThread(swapin_tstate);
	// dont give any swapper-inners the opportunity to do a double swap
	switch_channel_set_private(channel, "SwapInThreadState", NULL);
	return true;
    }


}

void PySession::hangup(char *cause) {


    // since we INCREF'd this function pointer earlier (so the py gc didnt reclaim it)
    // we have to DECREF it, or else the PySession dtor will never get called and
    // a zombie channel will be left over using up resources
    
    if (cb_state.function != NULL) {
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "xdecref on cb_state_function\n");	
	PyObject * func = (PyObject *) cb_state.function;
	Py_XDECREF(func);
    }
    else {
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "cb_state.function is null\n");	
    }
    

    CoreSession::hangup(cause);

}


PySession::~PySession() {
    // Should we do any cleanup here?
    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "PySession::~PySession started\n");

    if (on_hangup) {
	PyObject * func = (PyObject *) on_hangup;
	Py_XDECREF(func);
    }

    
    if (cb_state.function != NULL) {
	PyObject * func = (PyObject *) cb_state.function;
	Py_XDECREF(func);
    }
    
    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "PySession::~PySession finished\n");
    
}











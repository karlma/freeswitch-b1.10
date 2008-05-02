/* ----------------------------------------------------------------------------
 * This file was automatically generated by SWIG (http://www.swig.org).
 * Version 1.3.35
 *
 * Do not make changes to this file unless you know what you are doing--modify
 * the SWIG interface file instead.
 * ----------------------------------------------------------------------------- */

package org.freeswitch.swig;

class freeswitchJNI {
  public final static native void consoleLog(String jarg1, String jarg2);
  public final static native void consoleCleanLog(String jarg1);
  public final static native long new_IVRMenu(long jarg1, IVRMenu jarg1_, String jarg2, String jarg3, String jarg4, String jarg5, String jarg6, String jarg7, String jarg8, int jarg9, int jarg10, int jarg11, int jarg12, int jarg13);
  public final static native void delete_IVRMenu(long jarg1);
  public final static native void IVRMenu_bindAction(long jarg1, IVRMenu jarg1_, String jarg2, String jarg3, String jarg4);
  public final static native void IVRMenu_execute(long jarg1, IVRMenu jarg1_, long jarg2, CoreSession jarg2_, String jarg3);
  public final static native long new_API();
  public final static native void delete_API(long jarg1);
  public final static native String API_execute(long jarg1, API jarg1_, String jarg2, String jarg3);
  public final static native String API_executeString(long jarg1, API jarg1_, String jarg2);
  public final static native void input_callback_state_t_function_set(long jarg1, input_callback_state_t jarg1_, long jarg2);
  public final static native long input_callback_state_t_function_get(long jarg1, input_callback_state_t jarg1_);
  public final static native void input_callback_state_t_threadState_set(long jarg1, input_callback_state_t jarg1_, long jarg2);
  public final static native long input_callback_state_t_threadState_get(long jarg1, input_callback_state_t jarg1_);
  public final static native void input_callback_state_t_extra_set(long jarg1, input_callback_state_t jarg1_, long jarg2);
  public final static native long input_callback_state_t_extra_get(long jarg1, input_callback_state_t jarg1_);
  public final static native void input_callback_state_t_funcargs_set(long jarg1, input_callback_state_t jarg1_, String jarg2);
  public final static native String input_callback_state_t_funcargs_get(long jarg1, input_callback_state_t jarg1_);
  public final static native long new_input_callback_state_t();
  public final static native void delete_input_callback_state_t(long jarg1);
  public final static native int S_HUP_get();
  public final static native int S_FREE_get();
  public final static native int S_RDLOCK_get();
  public final static native long new_Stream__SWIG_0();
  public final static native long new_Stream__SWIG_1(long jarg1);
  public final static native void delete_Stream(long jarg1);
  public final static native void Stream_write(long jarg1, Stream jarg1_, String jarg2);
  public final static native String Stream_get_data(long jarg1, Stream jarg1_);
  public final static native void Event_event_set(long jarg1, Event jarg1_, long jarg2);
  public final static native long Event_event_get(long jarg1, Event jarg1_);
  public final static native void Event_serialized_string_set(long jarg1, Event jarg1_, String jarg2);
  public final static native String Event_serialized_string_get(long jarg1, Event jarg1_);
  public final static native void Event_mine_set(long jarg1, Event jarg1_, int jarg2);
  public final static native int Event_mine_get(long jarg1, Event jarg1_);
  public final static native long new_Event__SWIG_0(String jarg1, String jarg2);
  public final static native long new_Event__SWIG_1(String jarg1);
  public final static native long new_Event__SWIG_2(long jarg1, int jarg2);
  public final static native long new_Event__SWIG_3(long jarg1);
  public final static native void delete_Event(long jarg1);
  public final static native String Event_serialize__SWIG_0(long jarg1, Event jarg1_, String jarg2);
  public final static native String Event_serialize__SWIG_1(long jarg1, Event jarg1_);
  public final static native boolean Event_setPriority__SWIG_0(long jarg1, Event jarg1_, long jarg2);
  public final static native boolean Event_setPriority__SWIG_1(long jarg1, Event jarg1_);
  public final static native String Event_getHeader(long jarg1, Event jarg1_, String jarg2);
  public final static native String Event_getBody(long jarg1, Event jarg1_);
  public final static native String Event_getType(long jarg1, Event jarg1_);
  public final static native boolean Event_addBody(long jarg1, Event jarg1_, String jarg2);
  public final static native boolean Event_addHeader(long jarg1, Event jarg1_, String jarg2, String jarg3);
  public final static native boolean Event_delHeader(long jarg1, Event jarg1_, String jarg2);
  public final static native boolean Event_fire(long jarg1, Event jarg1_);
  public final static native void delete_CoreSession(long jarg1);
  public final static native void CoreSession_session_set(long jarg1, CoreSession jarg1_, long jarg2);
  public final static native long CoreSession_session_get(long jarg1, CoreSession jarg1_);
  public final static native void CoreSession_channel_set(long jarg1, CoreSession jarg1_, long jarg2);
  public final static native long CoreSession_channel_get(long jarg1, CoreSession jarg1_);
  public final static native void CoreSession_flags_set(long jarg1, CoreSession jarg1_, long jarg2);
  public final static native long CoreSession_flags_get(long jarg1, CoreSession jarg1_);
  public final static native void CoreSession_allocated_set(long jarg1, CoreSession jarg1_, int jarg2);
  public final static native int CoreSession_allocated_get(long jarg1, CoreSession jarg1_);
  public final static native void CoreSession_cb_state_set(long jarg1, CoreSession jarg1_, long jarg2, input_callback_state_t jarg2_);
  public final static native long CoreSession_cb_state_get(long jarg1, CoreSession jarg1_);
  public final static native void CoreSession_hook_state_set(long jarg1, CoreSession jarg1_, long jarg2);
  public final static native long CoreSession_hook_state_get(long jarg1, CoreSession jarg1_);
  public final static native int CoreSession_answer(long jarg1, CoreSession jarg1_);
  public final static native int CoreSession_preAnswer(long jarg1, CoreSession jarg1_);
  public final static native void CoreSession_hangup__SWIG_0(long jarg1, CoreSession jarg1_, String jarg2);
  public final static native void CoreSession_hangup__SWIG_1(long jarg1, CoreSession jarg1_);
  public final static native void CoreSession_setVariable(long jarg1, CoreSession jarg1_, String jarg2, String jarg3);
  public final static native void CoreSession_setPrivate(long jarg1, CoreSession jarg1_, String jarg2, long jarg3);
  public final static native long CoreSession_getPrivate(long jarg1, CoreSession jarg1_, String jarg2);
  public final static native String CoreSession_getVariable(long jarg1, CoreSession jarg1_, String jarg2);
  public final static native int CoreSession_recordFile__SWIG_0(long jarg1, CoreSession jarg1_, String jarg2, int jarg3, int jarg4, int jarg5);
  public final static native int CoreSession_recordFile__SWIG_1(long jarg1, CoreSession jarg1_, String jarg2, int jarg3, int jarg4);
  public final static native int CoreSession_recordFile__SWIG_2(long jarg1, CoreSession jarg1_, String jarg2, int jarg3);
  public final static native int CoreSession_recordFile__SWIG_3(long jarg1, CoreSession jarg1_, String jarg2);
  public final static native void CoreSession_setCallerData(long jarg1, CoreSession jarg1_, String jarg2, String jarg3);
  public final static native int CoreSession_originate__SWIG_0(long jarg1, CoreSession jarg1_, long jarg2, CoreSession jarg2_, String jarg3, int jarg4);
  public final static native int CoreSession_originate__SWIG_1(long jarg1, CoreSession jarg1_, long jarg2, CoreSession jarg2_, String jarg3);
  public final static native void CoreSession_setDTMFCallback(long jarg1, CoreSession jarg1_, long jarg2, String jarg3);
  public final static native int CoreSession_speak(long jarg1, CoreSession jarg1_, String jarg2);
  public final static native void CoreSession_set_tts_parms(long jarg1, CoreSession jarg1_, String jarg2, String jarg3);
  public final static native int CoreSession_collectDigits(long jarg1, CoreSession jarg1_, int jarg2);
  public final static native String CoreSession_getDigits(long jarg1, CoreSession jarg1_, int jarg2, String jarg3, int jarg4);
  public final static native int CoreSession_transfer(long jarg1, CoreSession jarg1_, String jarg2, String jarg3, String jarg4);
  public final static native String CoreSession_read(long jarg1, CoreSession jarg1_, int jarg2, int jarg3, String jarg4, int jarg5, String jarg6);
  public final static native String CoreSession_playAndGetDigits(long jarg1, CoreSession jarg1_, int jarg2, int jarg3, int jarg4, int jarg5, String jarg6, String jarg7, String jarg8, String jarg9);
  public final static native int CoreSession_streamFile__SWIG_0(long jarg1, CoreSession jarg1_, String jarg2, int jarg3);
  public final static native int CoreSession_streamFile__SWIG_1(long jarg1, CoreSession jarg1_, String jarg2);
  public final static native int CoreSession_flushEvents(long jarg1, CoreSession jarg1_);
  public final static native int CoreSession_flushDigits(long jarg1, CoreSession jarg1_);
  public final static native int CoreSession_setAutoHangup(long jarg1, CoreSession jarg1_, boolean jarg2);
  public final static native void CoreSession_setHangupHook(long jarg1, CoreSession jarg1_, long jarg2);
  public final static native boolean CoreSession_ready(long jarg1, CoreSession jarg1_);
  public final static native void CoreSession_execute(long jarg1, CoreSession jarg1_, String jarg2, String jarg3);
  public final static native void CoreSession_sendEvent(long jarg1, CoreSession jarg1_, long jarg2, Event jarg2_);
  public final static native boolean CoreSession_begin_allow_threads(long jarg1, CoreSession jarg1_);
  public final static native boolean CoreSession_end_allow_threads(long jarg1, CoreSession jarg1_);
  public final static native String CoreSession_get_uuid(long jarg1, CoreSession jarg1_);
  public final static native long CoreSession_get_cb_args(long jarg1, CoreSession jarg1_);
  public final static native void CoreSession_check_hangup_hook(long jarg1, CoreSession jarg1_);
  public final static native long CoreSession_run_dtmf_callback(long jarg1, CoreSession jarg1_, long jarg2, long jarg3);
  public final static native void console_log(String jarg1, String jarg2);
  public final static native void console_clean_log(String jarg1);
  public final static native void bridge(long jarg1, CoreSession jarg1_, long jarg2, CoreSession jarg2_);
  public final static native long hanguphook(long jarg1);
  public final static native long dtmf_callback(long jarg1, long jarg2, long jarg3, long jarg4, long jarg5);
  public final static native void javaVM_set(long jarg1);
  public final static native long javaVM_get();
  public final static native long new_JavaSession__SWIG_0();
  public final static native long new_JavaSession__SWIG_1(String jarg1);
  public final static native long new_JavaSession__SWIG_2(long jarg1);
  public final static native void delete_JavaSession(long jarg1);
  public final static native boolean JavaSession_begin_allow_threads(long jarg1, JavaSession jarg1_);
  public final static native boolean JavaSession_end_allow_threads(long jarg1, JavaSession jarg1_);
  public final static native void JavaSession_setDTMFCallback(long jarg1, JavaSession jarg1_, org.freeswitch.DTMFCallback jarg2, String jarg3);
  public final static native void JavaSession_setHangupHook(long jarg1, JavaSession jarg1_, org.freeswitch.HangupHook jarg2);
  public final static native void JavaSession_check_hangup_hook(long jarg1, JavaSession jarg1_);
  public final static native long JavaSession_run_dtmf_callback(long jarg1, JavaSession jarg1_, long jarg2, long jarg3);
  public final static native long SWIGJavaSessionUpcast(long jarg1);
}

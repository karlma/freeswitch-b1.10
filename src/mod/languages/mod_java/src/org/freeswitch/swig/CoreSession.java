/* ----------------------------------------------------------------------------
 * This file was automatically generated by SWIG (http://www.swig.org).
 * Version 2.0.12
 *
 * Do not make changes to this file unless you know what you are doing--modify
 * the SWIG interface file instead.
 * ----------------------------------------------------------------------------- */

package org.freeswitch.swig;

public class CoreSession {
  private long swigCPtr;
  protected boolean swigCMemOwn;

  protected CoreSession(long cPtr, boolean cMemoryOwn) {
    swigCMemOwn = cMemoryOwn;
    swigCPtr = cPtr;
  }

  protected static long getCPtr(CoreSession obj) {
    return (obj == null) ? 0 : obj.swigCPtr;
  }

  protected void finalize() {
    delete();
  }

  public synchronized void delete() {
    if (swigCPtr != 0) {
      if (swigCMemOwn) {
        swigCMemOwn = false;
        freeswitchJNI.delete_CoreSession(swigCPtr);
      }
      swigCPtr = 0;
    }
  }

  public void setSession(SWIGTYPE_p_switch_core_session_t value) {
    freeswitchJNI.CoreSession_session_set(swigCPtr, this, SWIGTYPE_p_switch_core_session_t.getCPtr(value));
  }

  public SWIGTYPE_p_switch_core_session_t getSession() {
    long cPtr = freeswitchJNI.CoreSession_session_get(swigCPtr, this);
    return (cPtr == 0) ? null : new SWIGTYPE_p_switch_core_session_t(cPtr, false);
  }

  public void setChannel(SWIGTYPE_p_switch_channel_t value) {
    freeswitchJNI.CoreSession_channel_set(swigCPtr, this, SWIGTYPE_p_switch_channel_t.getCPtr(value));
  }

  public SWIGTYPE_p_switch_channel_t getChannel() {
    long cPtr = freeswitchJNI.CoreSession_channel_get(swigCPtr, this);
    return (cPtr == 0) ? null : new SWIGTYPE_p_switch_channel_t(cPtr, false);
  }

  public void setFlags(long value) {
    freeswitchJNI.CoreSession_flags_set(swigCPtr, this, value);
  }

  public long getFlags() {
    return freeswitchJNI.CoreSession_flags_get(swigCPtr, this);
  }

  public void setAllocated(int value) {
    freeswitchJNI.CoreSession_allocated_set(swigCPtr, this, value);
  }

  public int getAllocated() {
    return freeswitchJNI.CoreSession_allocated_get(swigCPtr, this);
  }

  public void setCb_state(input_callback_state_t value) {
    freeswitchJNI.CoreSession_cb_state_set(swigCPtr, this, input_callback_state_t.getCPtr(value), value);
  }

  public input_callback_state_t getCb_state() {
    long cPtr = freeswitchJNI.CoreSession_cb_state_get(swigCPtr, this);
    return (cPtr == 0) ? null : new input_callback_state_t(cPtr, false);
  }

  public void setHook_state(SWIGTYPE_p_switch_channel_state_t value) {
    freeswitchJNI.CoreSession_hook_state_set(swigCPtr, this, SWIGTYPE_p_switch_channel_state_t.getCPtr(value));
  }

  public SWIGTYPE_p_switch_channel_state_t getHook_state() {
    return new SWIGTYPE_p_switch_channel_state_t(freeswitchJNI.CoreSession_hook_state_get(swigCPtr, this), true);
  }

  public void setCause(SWIGTYPE_p_switch_call_cause_t value) {
    freeswitchJNI.CoreSession_cause_set(swigCPtr, this, SWIGTYPE_p_switch_call_cause_t.getCPtr(value));
  }

  public SWIGTYPE_p_switch_call_cause_t getCause() {
    return new SWIGTYPE_p_switch_call_cause_t(freeswitchJNI.CoreSession_cause_get(swigCPtr, this), true);
  }

  public void setUuid(String value) {
    freeswitchJNI.CoreSession_uuid_set(swigCPtr, this, value);
  }

  public String getUuid() {
    return freeswitchJNI.CoreSession_uuid_get(swigCPtr, this);
  }

  public void setTts_name(String value) {
    freeswitchJNI.CoreSession_tts_name_set(swigCPtr, this, value);
  }

  public String getTts_name() {
    return freeswitchJNI.CoreSession_tts_name_get(swigCPtr, this);
  }

  public void setVoice_name(String value) {
    freeswitchJNI.CoreSession_voice_name_set(swigCPtr, this, value);
  }

  public String getVoice_name() {
    return freeswitchJNI.CoreSession_voice_name_get(swigCPtr, this);
  }

  public int insertFile(String file, String insert_file, int sample_point) {
    return freeswitchJNI.CoreSession_insertFile(swigCPtr, this, file, insert_file, sample_point);
  }

  public int answer() {
    return freeswitchJNI.CoreSession_answer(swigCPtr, this);
  }

  public int print(String txt) {
    return freeswitchJNI.CoreSession_print(swigCPtr, this, txt);
  }

  public int preAnswer() {
    return freeswitchJNI.CoreSession_preAnswer(swigCPtr, this);
  }

  public void hangup(String cause) {
    freeswitchJNI.CoreSession_hangup(swigCPtr, this, cause);
  }

  public void hangupState() {
    freeswitchJNI.CoreSession_hangupState(swigCPtr, this);
  }

  public void setVariable(String var, String val) {
    freeswitchJNI.CoreSession_setVariable(swigCPtr, this, var, val);
  }

  public void setPrivate(String var, SWIGTYPE_p_void val) {
    freeswitchJNI.CoreSession_setPrivate(swigCPtr, this, var, SWIGTYPE_p_void.getCPtr(val));
  }

  public SWIGTYPE_p_void getPrivate(String var) {
    long cPtr = freeswitchJNI.CoreSession_getPrivate(swigCPtr, this, var);
    return (cPtr == 0) ? null : new SWIGTYPE_p_void(cPtr, false);
  }

  public String getVariable(String var) {
    return freeswitchJNI.CoreSession_getVariable(swigCPtr, this, var);
  }

  public SWIGTYPE_p_switch_status_t process_callback_result(String result) {
    return new SWIGTYPE_p_switch_status_t(freeswitchJNI.CoreSession_process_callback_result(swigCPtr, this, result), true);
  }

  public void say(String tosay, String module_name, String say_type, String say_method, String say_gender) {
    freeswitchJNI.CoreSession_say(swigCPtr, this, tosay, module_name, say_type, say_method, say_gender);
  }

  public void sayPhrase(String phrase_name, String phrase_data, String phrase_lang) {
    freeswitchJNI.CoreSession_sayPhrase(swigCPtr, this, phrase_name, phrase_data, phrase_lang);
  }

  public String hangupCause() {
    return freeswitchJNI.CoreSession_hangupCause(swigCPtr, this);
  }

  public String getState() {
    return freeswitchJNI.CoreSession_getState(swigCPtr, this);
  }

  public int recordFile(String file_name, int time_limit, int silence_threshold, int silence_hits) {
    return freeswitchJNI.CoreSession_recordFile(swigCPtr, this, file_name, time_limit, silence_threshold, silence_hits);
  }

  protected int originate(CoreSession a_leg_session, String dest, int timeout, SWIGTYPE_p_switch_state_handler_table_t handlers) {
    return freeswitchJNI.CoreSession_originate(swigCPtr, this, CoreSession.getCPtr(a_leg_session), a_leg_session, dest, timeout, SWIGTYPE_p_switch_state_handler_table_t.getCPtr(handlers));
  }

  public void destroy() {
    freeswitchJNI.CoreSession_destroy(swigCPtr, this);
  }

  public void setDTMFCallback(SWIGTYPE_p_void cbfunc, String funcargs) {
    freeswitchJNI.CoreSession_setDTMFCallback(swigCPtr, this, SWIGTYPE_p_void.getCPtr(cbfunc), funcargs);
  }

  public int speak(String text) {
    return freeswitchJNI.CoreSession_speak(swigCPtr, this, text);
  }

  public void set_tts_parms(String tts_name, String voice_name) {
    freeswitchJNI.CoreSession_set_tts_parms(swigCPtr, this, tts_name, voice_name);
  }

  public void set_tts_params(String tts_name, String voice_name) {
    freeswitchJNI.CoreSession_set_tts_params(swigCPtr, this, tts_name, voice_name);
  }

  public int collectDigits(int abs_timeout) {
    return freeswitchJNI.CoreSession_collectDigits__SWIG_0(swigCPtr, this, abs_timeout);
  }

  public int collectDigits(int digit_timeout, int abs_timeout) {
    return freeswitchJNI.CoreSession_collectDigits__SWIG_1(swigCPtr, this, digit_timeout, abs_timeout);
  }

  public String getDigits(int maxdigits, String terminators, int timeout) {
    return freeswitchJNI.CoreSession_getDigits__SWIG_0(swigCPtr, this, maxdigits, terminators, timeout);
  }

  public String getDigits(int maxdigits, String terminators, int timeout, int interdigit) {
    return freeswitchJNI.CoreSession_getDigits__SWIG_1(swigCPtr, this, maxdigits, terminators, timeout, interdigit);
  }

  public String getDigits(int maxdigits, String terminators, int timeout, int interdigit, int abstimeout) {
    return freeswitchJNI.CoreSession_getDigits__SWIG_2(swigCPtr, this, maxdigits, terminators, timeout, interdigit, abstimeout);
  }

  public int transfer(String extension, String dialplan, String context) {
    return freeswitchJNI.CoreSession_transfer(swigCPtr, this, extension, dialplan, context);
  }

  public String read(int min_digits, int max_digits, String prompt_audio_file, int timeout, String valid_terminators, int digit_timeout) {
    return freeswitchJNI.CoreSession_read(swigCPtr, this, min_digits, max_digits, prompt_audio_file, timeout, valid_terminators, digit_timeout);
  }

  public String playAndGetDigits(int min_digits, int max_digits, int max_tries, int timeout, String terminators, String audio_files, String bad_input_audio_files, String digits_regex, String var_name, int digit_timeout, String transfer_on_failure) {
    return freeswitchJNI.CoreSession_playAndGetDigits(swigCPtr, this, min_digits, max_digits, max_tries, timeout, terminators, audio_files, bad_input_audio_files, digits_regex, var_name, digit_timeout, transfer_on_failure);
  }

  public int streamFile(String file, int starting_sample_count) {
    return freeswitchJNI.CoreSession_streamFile(swigCPtr, this, file, starting_sample_count);
  }

  public int sleep(int ms, int sync) {
    return freeswitchJNI.CoreSession_sleep(swigCPtr, this, ms, sync);
  }

  public int flushEvents() {
    return freeswitchJNI.CoreSession_flushEvents(swigCPtr, this);
  }

  public int flushDigits() {
    return freeswitchJNI.CoreSession_flushDigits(swigCPtr, this);
  }

  public int setAutoHangup(boolean val) {
    return freeswitchJNI.CoreSession_setAutoHangup(swigCPtr, this, val);
  }

  public void setHangupHook(SWIGTYPE_p_void hangup_func) {
    freeswitchJNI.CoreSession_setHangupHook(swigCPtr, this, SWIGTYPE_p_void.getCPtr(hangup_func));
  }

  public boolean ready() {
    return freeswitchJNI.CoreSession_ready(swigCPtr, this);
  }

  public boolean bridged() {
    return freeswitchJNI.CoreSession_bridged(swigCPtr, this);
  }

  public boolean answered() {
    return freeswitchJNI.CoreSession_answered(swigCPtr, this);
  }

  public boolean mediaReady() {
    return freeswitchJNI.CoreSession_mediaReady(swigCPtr, this);
  }

  public void waitForAnswer(CoreSession calling_session) {
    freeswitchJNI.CoreSession_waitForAnswer(swigCPtr, this, CoreSession.getCPtr(calling_session), calling_session);
  }

  public void execute(String app, String data) {
    freeswitchJNI.CoreSession_execute(swigCPtr, this, app, data);
  }

  public void sendEvent(Event sendME) {
    freeswitchJNI.CoreSession_sendEvent(swigCPtr, this, Event.getCPtr(sendME), sendME);
  }

  public void setEventData(Event e) {
    freeswitchJNI.CoreSession_setEventData(swigCPtr, this, Event.getCPtr(e), e);
  }

  public String getXMLCDR() {
    return freeswitchJNI.CoreSession_getXMLCDR(swigCPtr, this);
  }

  public boolean begin_allow_threads() {
    return freeswitchJNI.CoreSession_begin_allow_threads(swigCPtr, this);
  }

  public boolean end_allow_threads() {
    return freeswitchJNI.CoreSession_end_allow_threads(swigCPtr, this);
  }

  public String get_uuid() {
    return freeswitchJNI.CoreSession_get_uuid(swigCPtr, this);
  }

  public SWIGTYPE_p_switch_input_args_t get_cb_args() {
    return new SWIGTYPE_p_switch_input_args_t(freeswitchJNI.CoreSession_get_cb_args(swigCPtr, this), false);
  }

  public void check_hangup_hook() {
    freeswitchJNI.CoreSession_check_hangup_hook(swigCPtr, this);
  }

  public SWIGTYPE_p_switch_status_t run_dtmf_callback(SWIGTYPE_p_void input, SWIGTYPE_p_switch_input_type_t itype) {
    return new SWIGTYPE_p_switch_status_t(freeswitchJNI.CoreSession_run_dtmf_callback(swigCPtr, this, SWIGTYPE_p_void.getCPtr(input), SWIGTYPE_p_switch_input_type_t.getCPtr(itype)), true);
  }

  public void consoleLog(String level_str, String msg) {
    freeswitchJNI.CoreSession_consoleLog(swigCPtr, this, level_str, msg);
  }

  public void consoleLog2(String level_str, String file, String func, int line, String msg) {
    freeswitchJNI.CoreSession_consoleLog2(swigCPtr, this, level_str, file, func, line, msg);
  }

}

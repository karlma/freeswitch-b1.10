/* ----------------------------------------------------------------------------
 * This file was automatically generated by SWIG (http://www.swig.org).
 * Version 1.3.35
 *
 * Do not make changes to this file unless you know what you are doing--modify
 * the SWIG interface file instead.
 * ----------------------------------------------------------------------------- */

namespace FreeSWITCH.Native {

using System;
using System.Runtime.InteropServices;

public class CoreSession : IDisposable {
  private HandleRef swigCPtr;
  protected bool swigCMemOwn;

  internal CoreSession(IntPtr cPtr, bool cMemoryOwn) {
    swigCMemOwn = cMemoryOwn;
    swigCPtr = new HandleRef(this, cPtr);
  }

  internal static HandleRef getCPtr(CoreSession obj) {
    return (obj == null) ? new HandleRef(null, IntPtr.Zero) : obj.swigCPtr;
  }

  ~CoreSession() {
    Dispose();
  }

  public virtual void Dispose() {
    lock(this) {
      if(swigCPtr.Handle != IntPtr.Zero && swigCMemOwn) {
        swigCMemOwn = false;
        freeswitchPINVOKE.delete_CoreSession(swigCPtr);
      }
      swigCPtr = new HandleRef(null, IntPtr.Zero);
      GC.SuppressFinalize(this);
    }
  }

  public SWIGTYPE_p_switch_core_session InternalSession {
    set {
      freeswitchPINVOKE.CoreSession_InternalSession_set(swigCPtr, SWIGTYPE_p_switch_core_session.getCPtr(value));
    } 
    get {
      IntPtr cPtr = freeswitchPINVOKE.CoreSession_InternalSession_get(swigCPtr);
      SWIGTYPE_p_switch_core_session ret = (cPtr == IntPtr.Zero) ? null : new SWIGTYPE_p_switch_core_session(cPtr, false);
      return ret;
    } 
  }

  public SWIGTYPE_p_switch_channel channel {
    set {
      freeswitchPINVOKE.CoreSession_channel_set(swigCPtr, SWIGTYPE_p_switch_channel.getCPtr(value));
    } 
    get {
      IntPtr cPtr = freeswitchPINVOKE.CoreSession_channel_get(swigCPtr);
      SWIGTYPE_p_switch_channel ret = (cPtr == IntPtr.Zero) ? null : new SWIGTYPE_p_switch_channel(cPtr, false);
      return ret;
    } 
  }

  public uint flags {
    set {
      freeswitchPINVOKE.CoreSession_flags_set(swigCPtr, value);
    } 
    get {
      uint ret = freeswitchPINVOKE.CoreSession_flags_get(swigCPtr);
      return ret;
    } 
  }

  public int allocated {
    set {
      freeswitchPINVOKE.CoreSession_allocated_set(swigCPtr, value);
    } 
    get {
      int ret = freeswitchPINVOKE.CoreSession_allocated_get(swigCPtr);
      return ret;
    } 
  }

  public input_callback_state_t cb_state {
    set {
      freeswitchPINVOKE.CoreSession_cb_state_set(swigCPtr, input_callback_state_t.getCPtr(value));
    } 
    get {
      IntPtr cPtr = freeswitchPINVOKE.CoreSession_cb_state_get(swigCPtr);
      input_callback_state_t ret = (cPtr == IntPtr.Zero) ? null : new input_callback_state_t(cPtr, false);
      return ret;
    } 
  }

  public switch_channel_state_t HookState {
    set {
      freeswitchPINVOKE.CoreSession_HookState_set(swigCPtr, (int)value);
    } 
    get {
      switch_channel_state_t ret = (switch_channel_state_t)freeswitchPINVOKE.CoreSession_HookState_get(swigCPtr);
      return ret;
    } 
  }

  public int Answer() {
    int ret = freeswitchPINVOKE.CoreSession_Answer(swigCPtr);
    return ret;
  }

  public int preAnswer() {
    int ret = freeswitchPINVOKE.CoreSession_preAnswer(swigCPtr);
    return ret;
  }

  public void Hangup(string cause) {
    freeswitchPINVOKE.CoreSession_Hangup(swigCPtr, cause);
  }

  public void SetVariable(string var, string val) {
    freeswitchPINVOKE.CoreSession_SetVariable(swigCPtr, var, val);
  }

  public void SetPrivate(string var, SWIGTYPE_p_void val) {
    freeswitchPINVOKE.CoreSession_SetPrivate(swigCPtr, var, SWIGTYPE_p_void.getCPtr(val));
  }

  public SWIGTYPE_p_void GetPrivate(string var) {
    IntPtr cPtr = freeswitchPINVOKE.CoreSession_GetPrivate(swigCPtr, var);
    SWIGTYPE_p_void ret = (cPtr == IntPtr.Zero) ? null : new SWIGTYPE_p_void(cPtr, false);
    return ret;
  }

  public string GetVariable(string var) {
    string ret = freeswitchPINVOKE.CoreSession_GetVariable(swigCPtr, var);
    return ret;
  }

  public void Say(string tosay, string module_name, string say_type, string say_method) {
    freeswitchPINVOKE.CoreSession_Say(swigCPtr, tosay, module_name, say_type, say_method);
  }

  public void SayPhrase(string phrase_name, string phrase_data, string phrase_lang) {
    freeswitchPINVOKE.CoreSession_SayPhrase(swigCPtr, phrase_name, phrase_data, phrase_lang);
  }

  public int RecordFile(string file_name, int max_len, int silence_threshold, int silence_secs) {
    int ret = freeswitchPINVOKE.CoreSession_RecordFile(swigCPtr, file_name, max_len, silence_threshold, silence_secs);
    return ret;
  }

  public void SetCallerData(string var, string val) {
    freeswitchPINVOKE.CoreSession_SetCallerData(swigCPtr, var, val);
  }

  public int Originate(CoreSession a_leg_session, string dest, int timeout) {
    int ret = freeswitchPINVOKE.CoreSession_Originate(swigCPtr, CoreSession.getCPtr(a_leg_session), dest, timeout);
    return ret;
  }

  public int Speak(string text) {
    int ret = freeswitchPINVOKE.CoreSession_Speak(swigCPtr, text);
    return ret;
  }

  public void SetTtsParameters(string tts_name, string voice_name) {
    freeswitchPINVOKE.CoreSession_SetTtsParameters(swigCPtr, tts_name, voice_name);
  }

  public int CollectDigits(int timeout) {
    int ret = freeswitchPINVOKE.CoreSession_CollectDigits(swigCPtr, timeout);
    return ret;
  }

  public string GetDigits(int maxdigits, string terminators, int timeout) {
    string ret = freeswitchPINVOKE.CoreSession_GetDigits(swigCPtr, maxdigits, terminators, timeout);
    return ret;
  }

  public int Transfer(string extensions, string dialplan, string context) {
    int ret = freeswitchPINVOKE.CoreSession_Transfer(swigCPtr, extensions, dialplan, context);
    return ret;
  }

  public string read(int min_digits, int max_digits, string prompt_audio_file, int timeout, string valid_terminators) {
    string ret = freeswitchPINVOKE.CoreSession_read(swigCPtr, min_digits, max_digits, prompt_audio_file, timeout, valid_terminators);
    return ret;
  }

  public string PlayAndGetDigits(int min_digits, int max_digits, int max_tries, int timeout, string terminators, string audio_files, string bad_input_audio_files, string digits_regex) {
    string ret = freeswitchPINVOKE.CoreSession_PlayAndGetDigits(swigCPtr, min_digits, max_digits, max_tries, timeout, terminators, audio_files, bad_input_audio_files, digits_regex);
    return ret;
  }

  public int StreamFile(string file, int starting_sample_count) {
    int ret = freeswitchPINVOKE.CoreSession_StreamFile(swigCPtr, file, starting_sample_count);
    return ret;
  }

  public int flushEvents() {
    int ret = freeswitchPINVOKE.CoreSession_flushEvents(swigCPtr);
    return ret;
  }

  public int flushDigits() {
    int ret = freeswitchPINVOKE.CoreSession_flushDigits(swigCPtr);
    return ret;
  }

  public int SetAutoHangup(bool val) {
    int ret = freeswitchPINVOKE.CoreSession_SetAutoHangup(swigCPtr, val);
    return ret;
  }

  public bool Ready() {
    bool ret = freeswitchPINVOKE.CoreSession_Ready(swigCPtr);
    return ret;
  }

  public bool answered() {
    bool ret = freeswitchPINVOKE.CoreSession_answered(swigCPtr);
    return ret;
  }

  public bool mediaReady() {
    bool ret = freeswitchPINVOKE.CoreSession_mediaReady(swigCPtr);
    return ret;
  }

  public void waitForAnswer(CoreSession calling_session) {
    freeswitchPINVOKE.CoreSession_waitForAnswer(swigCPtr, CoreSession.getCPtr(calling_session));
  }

  public void Execute(string app, string data) {
    freeswitchPINVOKE.CoreSession_Execute(swigCPtr, app, data);
  }

  public void sendEvent(Event sendME) {
    freeswitchPINVOKE.CoreSession_sendEvent(swigCPtr, Event.getCPtr(sendME));
  }

  public void setEventData(Event e) {
    freeswitchPINVOKE.CoreSession_setEventData(swigCPtr, Event.getCPtr(e));
  }

  public string getXMLCDR() {
    string ret = freeswitchPINVOKE.CoreSession_getXMLCDR(swigCPtr);
    return ret;
  }

  public virtual bool begin_allow_threads() {
    bool ret = freeswitchPINVOKE.CoreSession_begin_allow_threads(swigCPtr);
    return ret;
  }

  public virtual bool end_allow_threads() {
    bool ret = freeswitchPINVOKE.CoreSession_end_allow_threads(swigCPtr);
    return ret;
  }

  public string GetUuid() {
    string ret = freeswitchPINVOKE.CoreSession_GetUuid(swigCPtr);
    return ret;
  }

  public switch_input_args_t get_cb_args() {
    switch_input_args_t ret = new switch_input_args_t(freeswitchPINVOKE.CoreSession_get_cb_args(swigCPtr), false);
    return ret;
  }

  public virtual void check_hangup_hook() {
    freeswitchPINVOKE.CoreSession_check_hangup_hook(swigCPtr);
  }

}

}

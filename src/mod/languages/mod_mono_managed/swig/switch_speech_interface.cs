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

public class switch_speech_interface : IDisposable {
  private HandleRef swigCPtr;
  protected bool swigCMemOwn;

  internal switch_speech_interface(IntPtr cPtr, bool cMemoryOwn) {
    swigCMemOwn = cMemoryOwn;
    swigCPtr = new HandleRef(this, cPtr);
  }

  internal static HandleRef getCPtr(switch_speech_interface obj) {
    return (obj == null) ? new HandleRef(null, IntPtr.Zero) : obj.swigCPtr;
  }

  ~switch_speech_interface() {
    Dispose();
  }

  public virtual void Dispose() {
    lock(this) {
      if(swigCPtr.Handle != IntPtr.Zero && swigCMemOwn) {
        swigCMemOwn = false;
        freeswitchPINVOKE.delete_switch_speech_interface(swigCPtr);
      }
      swigCPtr = new HandleRef(null, IntPtr.Zero);
      GC.SuppressFinalize(this);
    }
  }

  public string interface_name {
    set {
      freeswitchPINVOKE.switch_speech_interface_interface_name_set(swigCPtr, value);
    } 
    get {
      string ret = freeswitchPINVOKE.switch_speech_interface_interface_name_get(swigCPtr);
      return ret;
    } 
  }

  public SWIGTYPE_p_f_p_switch_speech_handle_p_q_const__char_int_p_unsigned_long__switch_status_t speech_open {
    set {
      freeswitchPINVOKE.switch_speech_interface_speech_open_set(swigCPtr, SWIGTYPE_p_f_p_switch_speech_handle_p_q_const__char_int_p_unsigned_long__switch_status_t.getCPtr(value));
    } 
    get {
      IntPtr cPtr = freeswitchPINVOKE.switch_speech_interface_speech_open_get(swigCPtr);
      SWIGTYPE_p_f_p_switch_speech_handle_p_q_const__char_int_p_unsigned_long__switch_status_t ret = (cPtr == IntPtr.Zero) ? null : new SWIGTYPE_p_f_p_switch_speech_handle_p_q_const__char_int_p_unsigned_long__switch_status_t(cPtr, false);
      return ret;
    } 
  }

  public SWIGTYPE_p_f_p_switch_speech_handle_p_unsigned_long__switch_status_t speech_close {
    set {
      freeswitchPINVOKE.switch_speech_interface_speech_close_set(swigCPtr, SWIGTYPE_p_f_p_switch_speech_handle_p_unsigned_long__switch_status_t.getCPtr(value));
    } 
    get {
      IntPtr cPtr = freeswitchPINVOKE.switch_speech_interface_speech_close_get(swigCPtr);
      SWIGTYPE_p_f_p_switch_speech_handle_p_unsigned_long__switch_status_t ret = (cPtr == IntPtr.Zero) ? null : new SWIGTYPE_p_f_p_switch_speech_handle_p_unsigned_long__switch_status_t(cPtr, false);
      return ret;
    } 
  }

  public SWIGTYPE_p_f_p_switch_speech_handle_p_char_p_unsigned_long__switch_status_t speech_feed_tts {
    set {
      freeswitchPINVOKE.switch_speech_interface_speech_feed_tts_set(swigCPtr, SWIGTYPE_p_f_p_switch_speech_handle_p_char_p_unsigned_long__switch_status_t.getCPtr(value));
    } 
    get {
      IntPtr cPtr = freeswitchPINVOKE.switch_speech_interface_speech_feed_tts_get(swigCPtr);
      SWIGTYPE_p_f_p_switch_speech_handle_p_char_p_unsigned_long__switch_status_t ret = (cPtr == IntPtr.Zero) ? null : new SWIGTYPE_p_f_p_switch_speech_handle_p_char_p_unsigned_long__switch_status_t(cPtr, false);
      return ret;
    } 
  }

  public SWIGTYPE_p_f_p_switch_speech_handle_p_void_p_switch_size_t_p_unsigned_long_p_unsigned_long__switch_status_t speech_read_tts {
    set {
      freeswitchPINVOKE.switch_speech_interface_speech_read_tts_set(swigCPtr, SWIGTYPE_p_f_p_switch_speech_handle_p_void_p_switch_size_t_p_unsigned_long_p_unsigned_long__switch_status_t.getCPtr(value));
    } 
    get {
      IntPtr cPtr = freeswitchPINVOKE.switch_speech_interface_speech_read_tts_get(swigCPtr);
      SWIGTYPE_p_f_p_switch_speech_handle_p_void_p_switch_size_t_p_unsigned_long_p_unsigned_long__switch_status_t ret = (cPtr == IntPtr.Zero) ? null : new SWIGTYPE_p_f_p_switch_speech_handle_p_void_p_switch_size_t_p_unsigned_long_p_unsigned_long__switch_status_t(cPtr, false);
      return ret;
    } 
  }

  public SWIGTYPE_p_f_p_switch_speech_handle__void speech_flush_tts {
    set {
      freeswitchPINVOKE.switch_speech_interface_speech_flush_tts_set(swigCPtr, SWIGTYPE_p_f_p_switch_speech_handle__void.getCPtr(value));
    } 
    get {
      IntPtr cPtr = freeswitchPINVOKE.switch_speech_interface_speech_flush_tts_get(swigCPtr);
      SWIGTYPE_p_f_p_switch_speech_handle__void ret = (cPtr == IntPtr.Zero) ? null : new SWIGTYPE_p_f_p_switch_speech_handle__void(cPtr, false);
      return ret;
    } 
  }

  public SWIGTYPE_p_f_p_switch_speech_handle_p_char_p_q_const__char__void speech_text_param_tts {
    set {
      freeswitchPINVOKE.switch_speech_interface_speech_text_param_tts_set(swigCPtr, SWIGTYPE_p_f_p_switch_speech_handle_p_char_p_q_const__char__void.getCPtr(value));
    } 
    get {
      IntPtr cPtr = freeswitchPINVOKE.switch_speech_interface_speech_text_param_tts_get(swigCPtr);
      SWIGTYPE_p_f_p_switch_speech_handle_p_char_p_q_const__char__void ret = (cPtr == IntPtr.Zero) ? null : new SWIGTYPE_p_f_p_switch_speech_handle_p_char_p_q_const__char__void(cPtr, false);
      return ret;
    } 
  }

  public SWIGTYPE_p_f_p_switch_speech_handle_p_char_int__void speech_numeric_param_tts {
    set {
      freeswitchPINVOKE.switch_speech_interface_speech_numeric_param_tts_set(swigCPtr, SWIGTYPE_p_f_p_switch_speech_handle_p_char_int__void.getCPtr(value));
    } 
    get {
      IntPtr cPtr = freeswitchPINVOKE.switch_speech_interface_speech_numeric_param_tts_get(swigCPtr);
      SWIGTYPE_p_f_p_switch_speech_handle_p_char_int__void ret = (cPtr == IntPtr.Zero) ? null : new SWIGTYPE_p_f_p_switch_speech_handle_p_char_int__void(cPtr, false);
      return ret;
    } 
  }

  public SWIGTYPE_p_f_p_switch_speech_handle_p_char_double__void speech_float_param_tts {
    set {
      freeswitchPINVOKE.switch_speech_interface_speech_float_param_tts_set(swigCPtr, SWIGTYPE_p_f_p_switch_speech_handle_p_char_double__void.getCPtr(value));
    } 
    get {
      IntPtr cPtr = freeswitchPINVOKE.switch_speech_interface_speech_float_param_tts_get(swigCPtr);
      SWIGTYPE_p_f_p_switch_speech_handle_p_char_double__void ret = (cPtr == IntPtr.Zero) ? null : new SWIGTYPE_p_f_p_switch_speech_handle_p_char_double__void(cPtr, false);
      return ret;
    } 
  }

  public switch_speech_interface next {
    set {
      freeswitchPINVOKE.switch_speech_interface_next_set(swigCPtr, switch_speech_interface.getCPtr(value));
    } 
    get {
      IntPtr cPtr = freeswitchPINVOKE.switch_speech_interface_next_get(swigCPtr);
      switch_speech_interface ret = (cPtr == IntPtr.Zero) ? null : new switch_speech_interface(cPtr, false);
      return ret;
    } 
  }

  public switch_speech_interface() : this(freeswitchPINVOKE.new_switch_speech_interface(), true) {
  }

}

}

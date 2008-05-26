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

public class switch_asr_interface : IDisposable {
  private HandleRef swigCPtr;
  protected bool swigCMemOwn;

  internal switch_asr_interface(IntPtr cPtr, bool cMemoryOwn) {
    swigCMemOwn = cMemoryOwn;
    swigCPtr = new HandleRef(this, cPtr);
  }

  internal static HandleRef getCPtr(switch_asr_interface obj) {
    return (obj == null) ? new HandleRef(null, IntPtr.Zero) : obj.swigCPtr;
  }

  ~switch_asr_interface() {
    Dispose();
  }

  public virtual void Dispose() {
    lock(this) {
      if(swigCPtr.Handle != IntPtr.Zero && swigCMemOwn) {
        swigCMemOwn = false;
        freeswitchPINVOKE.delete_switch_asr_interface(swigCPtr);
      }
      swigCPtr = new HandleRef(null, IntPtr.Zero);
      GC.SuppressFinalize(this);
    }
  }

  public string interface_name {
    set {
      freeswitchPINVOKE.switch_asr_interface_interface_name_set(swigCPtr, value);
    } 
    get {
      string ret = freeswitchPINVOKE.switch_asr_interface_interface_name_get(swigCPtr);
      return ret;
    } 
  }

  public SWIGTYPE_p_f_p_switch_asr_handle_p_q_const__char_int_p_q_const__char_p_enum_switch_asr_flag_t__switch_status_t asr_open {
    set {
      freeswitchPINVOKE.switch_asr_interface_asr_open_set(swigCPtr, SWIGTYPE_p_f_p_switch_asr_handle_p_q_const__char_int_p_q_const__char_p_enum_switch_asr_flag_t__switch_status_t.getCPtr(value));
    } 
    get {
      IntPtr cPtr = freeswitchPINVOKE.switch_asr_interface_asr_open_get(swigCPtr);
      SWIGTYPE_p_f_p_switch_asr_handle_p_q_const__char_int_p_q_const__char_p_enum_switch_asr_flag_t__switch_status_t ret = (cPtr == IntPtr.Zero) ? null : new SWIGTYPE_p_f_p_switch_asr_handle_p_q_const__char_int_p_q_const__char_p_enum_switch_asr_flag_t__switch_status_t(cPtr, false);
      return ret;
    } 
  }

  public SWIGTYPE_p_f_p_switch_asr_handle_p_q_const__char_p_q_const__char__switch_status_t asr_load_grammar {
    set {
      freeswitchPINVOKE.switch_asr_interface_asr_load_grammar_set(swigCPtr, SWIGTYPE_p_f_p_switch_asr_handle_p_q_const__char_p_q_const__char__switch_status_t.getCPtr(value));
    } 
    get {
      IntPtr cPtr = freeswitchPINVOKE.switch_asr_interface_asr_load_grammar_get(swigCPtr);
      SWIGTYPE_p_f_p_switch_asr_handle_p_q_const__char_p_q_const__char__switch_status_t ret = (cPtr == IntPtr.Zero) ? null : new SWIGTYPE_p_f_p_switch_asr_handle_p_q_const__char_p_q_const__char__switch_status_t(cPtr, false);
      return ret;
    } 
  }

  public SWIGTYPE_p_f_p_switch_asr_handle_p_q_const__char__switch_status_t asr_unload_grammar {
    set {
      freeswitchPINVOKE.switch_asr_interface_asr_unload_grammar_set(swigCPtr, SWIGTYPE_p_f_p_switch_asr_handle_p_q_const__char__switch_status_t.getCPtr(value));
    } 
    get {
      IntPtr cPtr = freeswitchPINVOKE.switch_asr_interface_asr_unload_grammar_get(swigCPtr);
      SWIGTYPE_p_f_p_switch_asr_handle_p_q_const__char__switch_status_t ret = (cPtr == IntPtr.Zero) ? null : new SWIGTYPE_p_f_p_switch_asr_handle_p_q_const__char__switch_status_t(cPtr, false);
      return ret;
    } 
  }

  public SWIGTYPE_p_f_p_switch_asr_handle_p_enum_switch_asr_flag_t__switch_status_t asr_close {
    set {
      freeswitchPINVOKE.switch_asr_interface_asr_close_set(swigCPtr, SWIGTYPE_p_f_p_switch_asr_handle_p_enum_switch_asr_flag_t__switch_status_t.getCPtr(value));
    } 
    get {
      IntPtr cPtr = freeswitchPINVOKE.switch_asr_interface_asr_close_get(swigCPtr);
      SWIGTYPE_p_f_p_switch_asr_handle_p_enum_switch_asr_flag_t__switch_status_t ret = (cPtr == IntPtr.Zero) ? null : new SWIGTYPE_p_f_p_switch_asr_handle_p_enum_switch_asr_flag_t__switch_status_t(cPtr, false);
      return ret;
    } 
  }

  public SWIGTYPE_p_f_p_switch_asr_handle_p_void_unsigned_int_p_enum_switch_asr_flag_t__switch_status_t asr_feed {
    set {
      freeswitchPINVOKE.switch_asr_interface_asr_feed_set(swigCPtr, SWIGTYPE_p_f_p_switch_asr_handle_p_void_unsigned_int_p_enum_switch_asr_flag_t__switch_status_t.getCPtr(value));
    } 
    get {
      IntPtr cPtr = freeswitchPINVOKE.switch_asr_interface_asr_feed_get(swigCPtr);
      SWIGTYPE_p_f_p_switch_asr_handle_p_void_unsigned_int_p_enum_switch_asr_flag_t__switch_status_t ret = (cPtr == IntPtr.Zero) ? null : new SWIGTYPE_p_f_p_switch_asr_handle_p_void_unsigned_int_p_enum_switch_asr_flag_t__switch_status_t(cPtr, false);
      return ret;
    } 
  }

  public SWIGTYPE_p_f_p_switch_asr_handle__switch_status_t asr_resume {
    set {
      freeswitchPINVOKE.switch_asr_interface_asr_resume_set(swigCPtr, SWIGTYPE_p_f_p_switch_asr_handle__switch_status_t.getCPtr(value));
    } 
    get {
      IntPtr cPtr = freeswitchPINVOKE.switch_asr_interface_asr_resume_get(swigCPtr);
      SWIGTYPE_p_f_p_switch_asr_handle__switch_status_t ret = (cPtr == IntPtr.Zero) ? null : new SWIGTYPE_p_f_p_switch_asr_handle__switch_status_t(cPtr, false);
      return ret;
    } 
  }

  public SWIGTYPE_p_f_p_switch_asr_handle__switch_status_t asr_pause {
    set {
      freeswitchPINVOKE.switch_asr_interface_asr_pause_set(swigCPtr, SWIGTYPE_p_f_p_switch_asr_handle__switch_status_t.getCPtr(value));
    } 
    get {
      IntPtr cPtr = freeswitchPINVOKE.switch_asr_interface_asr_pause_get(swigCPtr);
      SWIGTYPE_p_f_p_switch_asr_handle__switch_status_t ret = (cPtr == IntPtr.Zero) ? null : new SWIGTYPE_p_f_p_switch_asr_handle__switch_status_t(cPtr, false);
      return ret;
    } 
  }

  public SWIGTYPE_p_f_p_switch_asr_handle_p_enum_switch_asr_flag_t__switch_status_t asr_check_results {
    set {
      freeswitchPINVOKE.switch_asr_interface_asr_check_results_set(swigCPtr, SWIGTYPE_p_f_p_switch_asr_handle_p_enum_switch_asr_flag_t__switch_status_t.getCPtr(value));
    } 
    get {
      IntPtr cPtr = freeswitchPINVOKE.switch_asr_interface_asr_check_results_get(swigCPtr);
      SWIGTYPE_p_f_p_switch_asr_handle_p_enum_switch_asr_flag_t__switch_status_t ret = (cPtr == IntPtr.Zero) ? null : new SWIGTYPE_p_f_p_switch_asr_handle_p_enum_switch_asr_flag_t__switch_status_t(cPtr, false);
      return ret;
    } 
  }

  public SWIGTYPE_p_f_p_switch_asr_handle_p_p_char_p_enum_switch_asr_flag_t__switch_status_t asr_get_results {
    set {
      freeswitchPINVOKE.switch_asr_interface_asr_get_results_set(swigCPtr, SWIGTYPE_p_f_p_switch_asr_handle_p_p_char_p_enum_switch_asr_flag_t__switch_status_t.getCPtr(value));
    } 
    get {
      IntPtr cPtr = freeswitchPINVOKE.switch_asr_interface_asr_get_results_get(swigCPtr);
      SWIGTYPE_p_f_p_switch_asr_handle_p_p_char_p_enum_switch_asr_flag_t__switch_status_t ret = (cPtr == IntPtr.Zero) ? null : new SWIGTYPE_p_f_p_switch_asr_handle_p_p_char_p_enum_switch_asr_flag_t__switch_status_t(cPtr, false);
      return ret;
    } 
  }

  public switch_asr_interface next {
    set {
      freeswitchPINVOKE.switch_asr_interface_next_set(swigCPtr, switch_asr_interface.getCPtr(value));
    } 
    get {
      IntPtr cPtr = freeswitchPINVOKE.switch_asr_interface_next_get(swigCPtr);
      switch_asr_interface ret = (cPtr == IntPtr.Zero) ? null : new switch_asr_interface(cPtr, false);
      return ret;
    } 
  }

  public switch_asr_interface() : this(freeswitchPINVOKE.new_switch_asr_interface(), true) {
  }

}

}

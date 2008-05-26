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

public class switch_timer_interface : IDisposable {
  private HandleRef swigCPtr;
  protected bool swigCMemOwn;

  internal switch_timer_interface(IntPtr cPtr, bool cMemoryOwn) {
    swigCMemOwn = cMemoryOwn;
    swigCPtr = new HandleRef(this, cPtr);
  }

  internal static HandleRef getCPtr(switch_timer_interface obj) {
    return (obj == null) ? new HandleRef(null, IntPtr.Zero) : obj.swigCPtr;
  }

  ~switch_timer_interface() {
    Dispose();
  }

  public virtual void Dispose() {
    lock(this) {
      if(swigCPtr.Handle != IntPtr.Zero && swigCMemOwn) {
        swigCMemOwn = false;
        freeswitchPINVOKE.delete_switch_timer_interface(swigCPtr);
      }
      swigCPtr = new HandleRef(null, IntPtr.Zero);
      GC.SuppressFinalize(this);
    }
  }

  public string interface_name {
    set {
      freeswitchPINVOKE.switch_timer_interface_interface_name_set(swigCPtr, value);
    } 
    get {
      string ret = freeswitchPINVOKE.switch_timer_interface_interface_name_get(swigCPtr);
      return ret;
    } 
  }

  public SWIGTYPE_p_f_p_switch_timer__switch_status_t timer_init {
    set {
      freeswitchPINVOKE.switch_timer_interface_timer_init_set(swigCPtr, SWIGTYPE_p_f_p_switch_timer__switch_status_t.getCPtr(value));
    } 
    get {
      IntPtr cPtr = freeswitchPINVOKE.switch_timer_interface_timer_init_get(swigCPtr);
      SWIGTYPE_p_f_p_switch_timer__switch_status_t ret = (cPtr == IntPtr.Zero) ? null : new SWIGTYPE_p_f_p_switch_timer__switch_status_t(cPtr, false);
      return ret;
    } 
  }

  public SWIGTYPE_p_f_p_switch_timer__switch_status_t timer_next {
    set {
      freeswitchPINVOKE.switch_timer_interface_timer_next_set(swigCPtr, SWIGTYPE_p_f_p_switch_timer__switch_status_t.getCPtr(value));
    } 
    get {
      IntPtr cPtr = freeswitchPINVOKE.switch_timer_interface_timer_next_get(swigCPtr);
      SWIGTYPE_p_f_p_switch_timer__switch_status_t ret = (cPtr == IntPtr.Zero) ? null : new SWIGTYPE_p_f_p_switch_timer__switch_status_t(cPtr, false);
      return ret;
    } 
  }

  public SWIGTYPE_p_f_p_switch_timer__switch_status_t timer_step {
    set {
      freeswitchPINVOKE.switch_timer_interface_timer_step_set(swigCPtr, SWIGTYPE_p_f_p_switch_timer__switch_status_t.getCPtr(value));
    } 
    get {
      IntPtr cPtr = freeswitchPINVOKE.switch_timer_interface_timer_step_get(swigCPtr);
      SWIGTYPE_p_f_p_switch_timer__switch_status_t ret = (cPtr == IntPtr.Zero) ? null : new SWIGTYPE_p_f_p_switch_timer__switch_status_t(cPtr, false);
      return ret;
    } 
  }

  public SWIGTYPE_p_f_p_switch_timer__switch_status_t timer_sync {
    set {
      freeswitchPINVOKE.switch_timer_interface_timer_sync_set(swigCPtr, SWIGTYPE_p_f_p_switch_timer__switch_status_t.getCPtr(value));
    } 
    get {
      IntPtr cPtr = freeswitchPINVOKE.switch_timer_interface_timer_sync_get(swigCPtr);
      SWIGTYPE_p_f_p_switch_timer__switch_status_t ret = (cPtr == IntPtr.Zero) ? null : new SWIGTYPE_p_f_p_switch_timer__switch_status_t(cPtr, false);
      return ret;
    } 
  }

  public SWIGTYPE_p_f_p_switch_timer_enum_switch_bool_t__switch_status_t timer_check {
    set {
      freeswitchPINVOKE.switch_timer_interface_timer_check_set(swigCPtr, SWIGTYPE_p_f_p_switch_timer_enum_switch_bool_t__switch_status_t.getCPtr(value));
    } 
    get {
      IntPtr cPtr = freeswitchPINVOKE.switch_timer_interface_timer_check_get(swigCPtr);
      SWIGTYPE_p_f_p_switch_timer_enum_switch_bool_t__switch_status_t ret = (cPtr == IntPtr.Zero) ? null : new SWIGTYPE_p_f_p_switch_timer_enum_switch_bool_t__switch_status_t(cPtr, false);
      return ret;
    } 
  }

  public SWIGTYPE_p_f_p_switch_timer__switch_status_t timer_destroy {
    set {
      freeswitchPINVOKE.switch_timer_interface_timer_destroy_set(swigCPtr, SWIGTYPE_p_f_p_switch_timer__switch_status_t.getCPtr(value));
    } 
    get {
      IntPtr cPtr = freeswitchPINVOKE.switch_timer_interface_timer_destroy_get(swigCPtr);
      SWIGTYPE_p_f_p_switch_timer__switch_status_t ret = (cPtr == IntPtr.Zero) ? null : new SWIGTYPE_p_f_p_switch_timer__switch_status_t(cPtr, false);
      return ret;
    } 
  }

  public switch_timer_interface next {
    set {
      freeswitchPINVOKE.switch_timer_interface_next_set(swigCPtr, switch_timer_interface.getCPtr(value));
    } 
    get {
      IntPtr cPtr = freeswitchPINVOKE.switch_timer_interface_next_get(swigCPtr);
      switch_timer_interface ret = (cPtr == IntPtr.Zero) ? null : new switch_timer_interface(cPtr, false);
      return ret;
    } 
  }

  public switch_timer_interface() : this(freeswitchPINVOKE.new_switch_timer_interface(), true) {
  }

}

}

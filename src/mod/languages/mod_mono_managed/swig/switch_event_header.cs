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

public class switch_event_header : IDisposable {
  private HandleRef swigCPtr;
  protected bool swigCMemOwn;

  internal switch_event_header(IntPtr cPtr, bool cMemoryOwn) {
    swigCMemOwn = cMemoryOwn;
    swigCPtr = new HandleRef(this, cPtr);
  }

  internal static HandleRef getCPtr(switch_event_header obj) {
    return (obj == null) ? new HandleRef(null, IntPtr.Zero) : obj.swigCPtr;
  }

  ~switch_event_header() {
    Dispose();
  }

  public virtual void Dispose() {
    lock(this) {
      if(swigCPtr.Handle != IntPtr.Zero && swigCMemOwn) {
        swigCMemOwn = false;
        freeswitchPINVOKE.delete_switch_event_header(swigCPtr);
      }
      swigCPtr = new HandleRef(null, IntPtr.Zero);
      GC.SuppressFinalize(this);
    }
  }

  public string name {
    set {
      freeswitchPINVOKE.switch_event_header_name_set(swigCPtr, value);
    } 
    get {
      string ret = freeswitchPINVOKE.switch_event_header_name_get(swigCPtr);
      return ret;
    } 
  }

  public string value {
    set {
      freeswitchPINVOKE.switch_event_header_value_set(swigCPtr, value);
    } 
    get {
      string ret = freeswitchPINVOKE.switch_event_header_value_get(swigCPtr);
      return ret;
    } 
  }

  public switch_event_header next {
    set {
      freeswitchPINVOKE.switch_event_header_next_set(swigCPtr, switch_event_header.getCPtr(value));
    } 
    get {
      IntPtr cPtr = freeswitchPINVOKE.switch_event_header_next_get(swigCPtr);
      switch_event_header ret = (cPtr == IntPtr.Zero) ? null : new switch_event_header(cPtr, false);
      return ret;
    } 
  }

  public switch_event_header() : this(freeswitchPINVOKE.new_switch_event_header(), true) {
  }

}

}

/* ----------------------------------------------------------------------------
 * This file was automatically generated by SWIG (http://www.swig.org).
 * Version 1.3.36
 *
 * Do not make changes to this file unless you know what you are doing--modify
 * the SWIG interface file instead.
 * ----------------------------------------------------------------------------- */

namespace FreeSWITCH.Native {

using System;
using System.Runtime.InteropServices;

public class switch_rtp_hdr_t : IDisposable {
  private HandleRef swigCPtr;
  protected bool swigCMemOwn;

  internal switch_rtp_hdr_t(IntPtr cPtr, bool cMemoryOwn) {
    swigCMemOwn = cMemoryOwn;
    swigCPtr = new HandleRef(this, cPtr);
  }

  internal static HandleRef getCPtr(switch_rtp_hdr_t obj) {
    return (obj == null) ? new HandleRef(null, IntPtr.Zero) : obj.swigCPtr;
  }

  ~switch_rtp_hdr_t() {
    Dispose();
  }

  public virtual void Dispose() {
    lock(this) {
      if(swigCPtr.Handle != IntPtr.Zero && swigCMemOwn) {
        swigCMemOwn = false;
        freeswitchPINVOKE.delete_switch_rtp_hdr_t(swigCPtr);
      }
      swigCPtr = new HandleRef(null, IntPtr.Zero);
      GC.SuppressFinalize(this);
    }
  }

  public uint version {
    set {
      freeswitchPINVOKE.switch_rtp_hdr_t_version_set(swigCPtr, value);
    } 
    get {
      uint ret = freeswitchPINVOKE.switch_rtp_hdr_t_version_get(swigCPtr);
      return ret;
    } 
  }

  public uint p {
    set {
      freeswitchPINVOKE.switch_rtp_hdr_t_p_set(swigCPtr, value);
    } 
    get {
      uint ret = freeswitchPINVOKE.switch_rtp_hdr_t_p_get(swigCPtr);
      return ret;
    } 
  }

  public uint x {
    set {
      freeswitchPINVOKE.switch_rtp_hdr_t_x_set(swigCPtr, value);
    } 
    get {
      uint ret = freeswitchPINVOKE.switch_rtp_hdr_t_x_get(swigCPtr);
      return ret;
    } 
  }

  public uint cc {
    set {
      freeswitchPINVOKE.switch_rtp_hdr_t_cc_set(swigCPtr, value);
    } 
    get {
      uint ret = freeswitchPINVOKE.switch_rtp_hdr_t_cc_get(swigCPtr);
      return ret;
    } 
  }

  public uint m {
    set {
      freeswitchPINVOKE.switch_rtp_hdr_t_m_set(swigCPtr, value);
    } 
    get {
      uint ret = freeswitchPINVOKE.switch_rtp_hdr_t_m_get(swigCPtr);
      return ret;
    } 
  }

  public uint pt {
    set {
      freeswitchPINVOKE.switch_rtp_hdr_t_pt_set(swigCPtr, value);
    } 
    get {
      uint ret = freeswitchPINVOKE.switch_rtp_hdr_t_pt_get(swigCPtr);
      return ret;
    } 
  }

  public uint seq {
    set {
      freeswitchPINVOKE.switch_rtp_hdr_t_seq_set(swigCPtr, value);
    } 
    get {
      uint ret = freeswitchPINVOKE.switch_rtp_hdr_t_seq_get(swigCPtr);
      return ret;
    } 
  }

  public uint ts {
    set {
      freeswitchPINVOKE.switch_rtp_hdr_t_ts_set(swigCPtr, value);
    } 
    get {
      uint ret = freeswitchPINVOKE.switch_rtp_hdr_t_ts_get(swigCPtr);
      return ret;
    } 
  }

  public uint ssrc {
    set {
      freeswitchPINVOKE.switch_rtp_hdr_t_ssrc_set(swigCPtr, value);
    } 
    get {
      uint ret = freeswitchPINVOKE.switch_rtp_hdr_t_ssrc_get(swigCPtr);
      return ret;
    } 
  }

  public switch_rtp_hdr_t() : this(freeswitchPINVOKE.new_switch_rtp_hdr_t(), true) {
  }

}

}

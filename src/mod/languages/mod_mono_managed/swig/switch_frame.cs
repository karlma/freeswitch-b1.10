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

public class switch_frame : IDisposable {
  private HandleRef swigCPtr;
  protected bool swigCMemOwn;

  internal switch_frame(IntPtr cPtr, bool cMemoryOwn) {
    swigCMemOwn = cMemoryOwn;
    swigCPtr = new HandleRef(this, cPtr);
  }

  internal static HandleRef getCPtr(switch_frame obj) {
    return (obj == null) ? new HandleRef(null, IntPtr.Zero) : obj.swigCPtr;
  }

  ~switch_frame() {
    Dispose();
  }

  public virtual void Dispose() {
    lock(this) {
      if(swigCPtr.Handle != IntPtr.Zero && swigCMemOwn) {
        swigCMemOwn = false;
        freeswitchPINVOKE.delete_switch_frame(swigCPtr);
      }
      swigCPtr = new HandleRef(null, IntPtr.Zero);
      GC.SuppressFinalize(this);
    }
  }

  public switch_codec codec {
    set {
      freeswitchPINVOKE.switch_frame_codec_set(swigCPtr, switch_codec.getCPtr(value));
    } 
    get {
      IntPtr cPtr = freeswitchPINVOKE.switch_frame_codec_get(swigCPtr);
      switch_codec ret = (cPtr == IntPtr.Zero) ? null : new switch_codec(cPtr, false);
      return ret;
    } 
  }

  public string source {
    set {
      freeswitchPINVOKE.switch_frame_source_set(swigCPtr, value);
    } 
    get {
      string ret = freeswitchPINVOKE.switch_frame_source_get(swigCPtr);
      return ret;
    } 
  }

  public SWIGTYPE_p_void packet {
    set {
      freeswitchPINVOKE.switch_frame_packet_set(swigCPtr, SWIGTYPE_p_void.getCPtr(value));
    } 
    get {
      IntPtr cPtr = freeswitchPINVOKE.switch_frame_packet_get(swigCPtr);
      SWIGTYPE_p_void ret = (cPtr == IntPtr.Zero) ? null : new SWIGTYPE_p_void(cPtr, false);
      return ret;
    } 
  }

  public uint packetlen {
    set {
      freeswitchPINVOKE.switch_frame_packetlen_set(swigCPtr, value);
    } 
    get {
      uint ret = freeswitchPINVOKE.switch_frame_packetlen_get(swigCPtr);
      return ret;
    } 
  }

  public SWIGTYPE_p_void data {
    set {
      freeswitchPINVOKE.switch_frame_data_set(swigCPtr, SWIGTYPE_p_void.getCPtr(value));
    } 
    get {
      IntPtr cPtr = freeswitchPINVOKE.switch_frame_data_get(swigCPtr);
      SWIGTYPE_p_void ret = (cPtr == IntPtr.Zero) ? null : new SWIGTYPE_p_void(cPtr, false);
      return ret;
    } 
  }

  public uint datalen {
    set {
      freeswitchPINVOKE.switch_frame_datalen_set(swigCPtr, value);
    } 
    get {
      uint ret = freeswitchPINVOKE.switch_frame_datalen_get(swigCPtr);
      return ret;
    } 
  }

  public uint buflen {
    set {
      freeswitchPINVOKE.switch_frame_buflen_set(swigCPtr, value);
    } 
    get {
      uint ret = freeswitchPINVOKE.switch_frame_buflen_get(swigCPtr);
      return ret;
    } 
  }

  public uint samples {
    set {
      freeswitchPINVOKE.switch_frame_samples_set(swigCPtr, value);
    } 
    get {
      uint ret = freeswitchPINVOKE.switch_frame_samples_get(swigCPtr);
      return ret;
    } 
  }

  public uint rate {
    set {
      freeswitchPINVOKE.switch_frame_rate_set(swigCPtr, value);
    } 
    get {
      uint ret = freeswitchPINVOKE.switch_frame_rate_get(swigCPtr);
      return ret;
    } 
  }

  public byte payload {
    set {
      freeswitchPINVOKE.switch_frame_payload_set(swigCPtr, value);
    } 
    get {
      byte ret = freeswitchPINVOKE.switch_frame_payload_get(swigCPtr);
      return ret;
    } 
  }

  public SWIGTYPE_p_switch_size_t timestamp {
    set {
      freeswitchPINVOKE.switch_frame_timestamp_set(swigCPtr, SWIGTYPE_p_switch_size_t.getCPtr(value));
      if (freeswitchPINVOKE.SWIGPendingException.Pending) throw freeswitchPINVOKE.SWIGPendingException.Retrieve();
    } 
    get {
      SWIGTYPE_p_switch_size_t ret = new SWIGTYPE_p_switch_size_t(freeswitchPINVOKE.switch_frame_timestamp_get(swigCPtr), true);
      if (freeswitchPINVOKE.SWIGPendingException.Pending) throw freeswitchPINVOKE.SWIGPendingException.Retrieve();
      return ret;
    } 
  }

  public ushort seq {
    set {
      freeswitchPINVOKE.switch_frame_seq_set(swigCPtr, value);
    } 
    get {
      ushort ret = freeswitchPINVOKE.switch_frame_seq_get(swigCPtr);
      return ret;
    } 
  }

  public uint ssrc {
    set {
      freeswitchPINVOKE.switch_frame_ssrc_set(swigCPtr, value);
    } 
    get {
      uint ret = freeswitchPINVOKE.switch_frame_ssrc_get(swigCPtr);
      return ret;
    } 
  }

  public switch_bool_t m {
    set {
      freeswitchPINVOKE.switch_frame_m_set(swigCPtr, (int)value);
    } 
    get {
      switch_bool_t ret = (switch_bool_t)freeswitchPINVOKE.switch_frame_m_get(swigCPtr);
      return ret;
    } 
  }

  public uint flags {
    set {
      freeswitchPINVOKE.switch_frame_flags_set(swigCPtr, value);
    } 
    get {
      uint ret = freeswitchPINVOKE.switch_frame_flags_get(swigCPtr);
      return ret;
    } 
  }

  public switch_frame() : this(freeswitchPINVOKE.new_switch_frame(), true) {
  }

}

}

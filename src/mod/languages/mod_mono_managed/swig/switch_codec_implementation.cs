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

public class switch_codec_implementation : IDisposable {
  private HandleRef swigCPtr;
  protected bool swigCMemOwn;

  internal switch_codec_implementation(IntPtr cPtr, bool cMemoryOwn) {
    swigCMemOwn = cMemoryOwn;
    swigCPtr = new HandleRef(this, cPtr);
  }

  internal static HandleRef getCPtr(switch_codec_implementation obj) {
    return (obj == null) ? new HandleRef(null, IntPtr.Zero) : obj.swigCPtr;
  }

  ~switch_codec_implementation() {
    Dispose();
  }

  public virtual void Dispose() {
    lock(this) {
      if(swigCPtr.Handle != IntPtr.Zero && swigCMemOwn) {
        swigCMemOwn = false;
        freeswitchPINVOKE.delete_switch_codec_implementation(swigCPtr);
      }
      swigCPtr = new HandleRef(null, IntPtr.Zero);
      GC.SuppressFinalize(this);
    }
  }

  public switch_codec_type_t codec_type {
    set {
      freeswitchPINVOKE.switch_codec_implementation_codec_type_set(swigCPtr, (int)value);
    } 
    get {
      switch_codec_type_t ret = (switch_codec_type_t)freeswitchPINVOKE.switch_codec_implementation_codec_type_get(swigCPtr);
      return ret;
    } 
  }

  public byte ianacode {
    set {
      freeswitchPINVOKE.switch_codec_implementation_ianacode_set(swigCPtr, value);
    } 
    get {
      byte ret = freeswitchPINVOKE.switch_codec_implementation_ianacode_get(swigCPtr);
      return ret;
    } 
  }

  public string iananame {
    set {
      freeswitchPINVOKE.switch_codec_implementation_iananame_set(swigCPtr, value);
    } 
    get {
      string ret = freeswitchPINVOKE.switch_codec_implementation_iananame_get(swigCPtr);
      return ret;
    } 
  }

  public string fmtp {
    set {
      freeswitchPINVOKE.switch_codec_implementation_fmtp_set(swigCPtr, value);
    } 
    get {
      string ret = freeswitchPINVOKE.switch_codec_implementation_fmtp_get(swigCPtr);
      return ret;
    } 
  }

  public uint samples_per_second {
    set {
      freeswitchPINVOKE.switch_codec_implementation_samples_per_second_set(swigCPtr, value);
    } 
    get {
      uint ret = freeswitchPINVOKE.switch_codec_implementation_samples_per_second_get(swigCPtr);
      return ret;
    } 
  }

  public uint actual_samples_per_second {
    set {
      freeswitchPINVOKE.switch_codec_implementation_actual_samples_per_second_set(swigCPtr, value);
    } 
    get {
      uint ret = freeswitchPINVOKE.switch_codec_implementation_actual_samples_per_second_get(swigCPtr);
      return ret;
    } 
  }

  public int bits_per_second {
    set {
      freeswitchPINVOKE.switch_codec_implementation_bits_per_second_set(swigCPtr, value);
    } 
    get {
      int ret = freeswitchPINVOKE.switch_codec_implementation_bits_per_second_get(swigCPtr);
      return ret;
    } 
  }

  public int microseconds_per_frame {
    set {
      freeswitchPINVOKE.switch_codec_implementation_microseconds_per_frame_set(swigCPtr, value);
    } 
    get {
      int ret = freeswitchPINVOKE.switch_codec_implementation_microseconds_per_frame_get(swigCPtr);
      return ret;
    } 
  }

  public uint samples_per_frame {
    set {
      freeswitchPINVOKE.switch_codec_implementation_samples_per_frame_set(swigCPtr, value);
    } 
    get {
      uint ret = freeswitchPINVOKE.switch_codec_implementation_samples_per_frame_get(swigCPtr);
      return ret;
    } 
  }

  public uint bytes_per_frame {
    set {
      freeswitchPINVOKE.switch_codec_implementation_bytes_per_frame_set(swigCPtr, value);
    } 
    get {
      uint ret = freeswitchPINVOKE.switch_codec_implementation_bytes_per_frame_get(swigCPtr);
      return ret;
    } 
  }

  public uint encoded_bytes_per_frame {
    set {
      freeswitchPINVOKE.switch_codec_implementation_encoded_bytes_per_frame_set(swigCPtr, value);
    } 
    get {
      uint ret = freeswitchPINVOKE.switch_codec_implementation_encoded_bytes_per_frame_get(swigCPtr);
      return ret;
    } 
  }

  public byte number_of_channels {
    set {
      freeswitchPINVOKE.switch_codec_implementation_number_of_channels_set(swigCPtr, value);
    } 
    get {
      byte ret = freeswitchPINVOKE.switch_codec_implementation_number_of_channels_get(swigCPtr);
      return ret;
    } 
  }

  public int pref_frames_per_packet {
    set {
      freeswitchPINVOKE.switch_codec_implementation_pref_frames_per_packet_set(swigCPtr, value);
    } 
    get {
      int ret = freeswitchPINVOKE.switch_codec_implementation_pref_frames_per_packet_get(swigCPtr);
      return ret;
    } 
  }

  public int max_frames_per_packet {
    set {
      freeswitchPINVOKE.switch_codec_implementation_max_frames_per_packet_set(swigCPtr, value);
    } 
    get {
      int ret = freeswitchPINVOKE.switch_codec_implementation_max_frames_per_packet_get(swigCPtr);
      return ret;
    } 
  }

  public SWIGTYPE_p_f_p_switch_codec_unsigned_long_p_q_const__switch_codec_settings__switch_status_t init {
    set {
      freeswitchPINVOKE.switch_codec_implementation_init_set(swigCPtr, SWIGTYPE_p_f_p_switch_codec_unsigned_long_p_q_const__switch_codec_settings__switch_status_t.getCPtr(value));
    } 
    get {
      IntPtr cPtr = freeswitchPINVOKE.switch_codec_implementation_init_get(swigCPtr);
      SWIGTYPE_p_f_p_switch_codec_unsigned_long_p_q_const__switch_codec_settings__switch_status_t ret = (cPtr == IntPtr.Zero) ? null : new SWIGTYPE_p_f_p_switch_codec_unsigned_long_p_q_const__switch_codec_settings__switch_status_t(cPtr, false);
      return ret;
    } 
  }

  public SWIGTYPE_p_f_p_switch_codec_p_switch_codec_p_void_unsigned_long_unsigned_long_p_void_p_unsigned_long_p_unsigned_long_p_unsigned_int__switch_status_t encode {
    set {
      freeswitchPINVOKE.switch_codec_implementation_encode_set(swigCPtr, SWIGTYPE_p_f_p_switch_codec_p_switch_codec_p_void_unsigned_long_unsigned_long_p_void_p_unsigned_long_p_unsigned_long_p_unsigned_int__switch_status_t.getCPtr(value));
    } 
    get {
      IntPtr cPtr = freeswitchPINVOKE.switch_codec_implementation_encode_get(swigCPtr);
      SWIGTYPE_p_f_p_switch_codec_p_switch_codec_p_void_unsigned_long_unsigned_long_p_void_p_unsigned_long_p_unsigned_long_p_unsigned_int__switch_status_t ret = (cPtr == IntPtr.Zero) ? null : new SWIGTYPE_p_f_p_switch_codec_p_switch_codec_p_void_unsigned_long_unsigned_long_p_void_p_unsigned_long_p_unsigned_long_p_unsigned_int__switch_status_t(cPtr, false);
      return ret;
    } 
  }

  public SWIGTYPE_p_f_p_switch_codec_p_switch_codec_p_void_unsigned_long_unsigned_long_p_void_p_unsigned_long_p_unsigned_long_p_unsigned_int__switch_status_t decode {
    set {
      freeswitchPINVOKE.switch_codec_implementation_decode_set(swigCPtr, SWIGTYPE_p_f_p_switch_codec_p_switch_codec_p_void_unsigned_long_unsigned_long_p_void_p_unsigned_long_p_unsigned_long_p_unsigned_int__switch_status_t.getCPtr(value));
    } 
    get {
      IntPtr cPtr = freeswitchPINVOKE.switch_codec_implementation_decode_get(swigCPtr);
      SWIGTYPE_p_f_p_switch_codec_p_switch_codec_p_void_unsigned_long_unsigned_long_p_void_p_unsigned_long_p_unsigned_long_p_unsigned_int__switch_status_t ret = (cPtr == IntPtr.Zero) ? null : new SWIGTYPE_p_f_p_switch_codec_p_switch_codec_p_void_unsigned_long_unsigned_long_p_void_p_unsigned_long_p_unsigned_long_p_unsigned_int__switch_status_t(cPtr, false);
      return ret;
    } 
  }

  public SWIGTYPE_p_f_p_switch_codec__switch_status_t destroy {
    set {
      freeswitchPINVOKE.switch_codec_implementation_destroy_set(swigCPtr, SWIGTYPE_p_f_p_switch_codec__switch_status_t.getCPtr(value));
    } 
    get {
      IntPtr cPtr = freeswitchPINVOKE.switch_codec_implementation_destroy_get(swigCPtr);
      SWIGTYPE_p_f_p_switch_codec__switch_status_t ret = (cPtr == IntPtr.Zero) ? null : new SWIGTYPE_p_f_p_switch_codec__switch_status_t(cPtr, false);
      return ret;
    } 
  }

  public uint codec_id {
    set {
      freeswitchPINVOKE.switch_codec_implementation_codec_id_set(swigCPtr, value);
    } 
    get {
      uint ret = freeswitchPINVOKE.switch_codec_implementation_codec_id_get(swigCPtr);
      return ret;
    } 
  }

  public switch_codec_implementation next {
    set {
      freeswitchPINVOKE.switch_codec_implementation_next_set(swigCPtr, switch_codec_implementation.getCPtr(value));
    } 
    get {
      IntPtr cPtr = freeswitchPINVOKE.switch_codec_implementation_next_get(swigCPtr);
      switch_codec_implementation ret = (cPtr == IntPtr.Zero) ? null : new switch_codec_implementation(cPtr, false);
      return ret;
    } 
  }

  public switch_codec_implementation() : this(freeswitchPINVOKE.new_switch_codec_implementation(), true) {
  }

}

}

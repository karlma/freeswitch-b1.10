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

public class switch_file_handle : IDisposable {
  private HandleRef swigCPtr;
  protected bool swigCMemOwn;

  internal switch_file_handle(IntPtr cPtr, bool cMemoryOwn) {
    swigCMemOwn = cMemoryOwn;
    swigCPtr = new HandleRef(this, cPtr);
  }

  internal static HandleRef getCPtr(switch_file_handle obj) {
    return (obj == null) ? new HandleRef(null, IntPtr.Zero) : obj.swigCPtr;
  }

  ~switch_file_handle() {
    Dispose();
  }

  public virtual void Dispose() {
    lock(this) {
      if(swigCPtr.Handle != IntPtr.Zero && swigCMemOwn) {
        swigCMemOwn = false;
        freeswitchPINVOKE.delete_switch_file_handle(swigCPtr);
      }
      swigCPtr = new HandleRef(null, IntPtr.Zero);
      GC.SuppressFinalize(this);
    }
  }

  public switch_file_interface file_interface {
    set {
      freeswitchPINVOKE.switch_file_handle_file_interface_set(swigCPtr, switch_file_interface.getCPtr(value));
    } 
    get {
      IntPtr cPtr = freeswitchPINVOKE.switch_file_handle_file_interface_get(swigCPtr);
      switch_file_interface ret = (cPtr == IntPtr.Zero) ? null : new switch_file_interface(cPtr, false);
      return ret;
    } 
  }

  public uint flags {
    set {
      freeswitchPINVOKE.switch_file_handle_flags_set(swigCPtr, value);
    } 
    get {
      uint ret = freeswitchPINVOKE.switch_file_handle_flags_get(swigCPtr);
      return ret;
    } 
  }

  public SWIGTYPE_p_switch_file_t fd {
    set {
      freeswitchPINVOKE.switch_file_handle_fd_set(swigCPtr, SWIGTYPE_p_switch_file_t.getCPtr(value));
    } 
    get {
      IntPtr cPtr = freeswitchPINVOKE.switch_file_handle_fd_get(swigCPtr);
      SWIGTYPE_p_switch_file_t ret = (cPtr == IntPtr.Zero) ? null : new SWIGTYPE_p_switch_file_t(cPtr, false);
      return ret;
    } 
  }

  public uint samples {
    set {
      freeswitchPINVOKE.switch_file_handle_samples_set(swigCPtr, value);
    } 
    get {
      uint ret = freeswitchPINVOKE.switch_file_handle_samples_get(swigCPtr);
      return ret;
    } 
  }

  public uint samplerate {
    set {
      freeswitchPINVOKE.switch_file_handle_samplerate_set(swigCPtr, value);
    } 
    get {
      uint ret = freeswitchPINVOKE.switch_file_handle_samplerate_get(swigCPtr);
      return ret;
    } 
  }

  public uint native_rate {
    set {
      freeswitchPINVOKE.switch_file_handle_native_rate_set(swigCPtr, value);
    } 
    get {
      uint ret = freeswitchPINVOKE.switch_file_handle_native_rate_get(swigCPtr);
      return ret;
    } 
  }

  public byte channels {
    set {
      freeswitchPINVOKE.switch_file_handle_channels_set(swigCPtr, value);
    } 
    get {
      byte ret = freeswitchPINVOKE.switch_file_handle_channels_get(swigCPtr);
      return ret;
    } 
  }

  public uint format {
    set {
      freeswitchPINVOKE.switch_file_handle_format_set(swigCPtr, value);
    } 
    get {
      uint ret = freeswitchPINVOKE.switch_file_handle_format_get(swigCPtr);
      return ret;
    } 
  }

  public uint sections {
    set {
      freeswitchPINVOKE.switch_file_handle_sections_set(swigCPtr, value);
    } 
    get {
      uint ret = freeswitchPINVOKE.switch_file_handle_sections_get(swigCPtr);
      return ret;
    } 
  }

  public int seekable {
    set {
      freeswitchPINVOKE.switch_file_handle_seekable_set(swigCPtr, value);
    } 
    get {
      int ret = freeswitchPINVOKE.switch_file_handle_seekable_get(swigCPtr);
      return ret;
    } 
  }

  public SWIGTYPE_p_switch_size_t sample_count {
    set {
      freeswitchPINVOKE.switch_file_handle_sample_count_set(swigCPtr, SWIGTYPE_p_switch_size_t.getCPtr(value));
      if (freeswitchPINVOKE.SWIGPendingException.Pending) throw freeswitchPINVOKE.SWIGPendingException.Retrieve();
    } 
    get {
      SWIGTYPE_p_switch_size_t ret = new SWIGTYPE_p_switch_size_t(freeswitchPINVOKE.switch_file_handle_sample_count_get(swigCPtr), true);
      if (freeswitchPINVOKE.SWIGPendingException.Pending) throw freeswitchPINVOKE.SWIGPendingException.Retrieve();
      return ret;
    } 
  }

  public int speed {
    set {
      freeswitchPINVOKE.switch_file_handle_speed_set(swigCPtr, value);
    } 
    get {
      int ret = freeswitchPINVOKE.switch_file_handle_speed_get(swigCPtr);
      return ret;
    } 
  }

  public SWIGTYPE_p_apr_pool_t memory_pool {
    set {
      freeswitchPINVOKE.switch_file_handle_memory_pool_set(swigCPtr, SWIGTYPE_p_apr_pool_t.getCPtr(value));
    } 
    get {
      IntPtr cPtr = freeswitchPINVOKE.switch_file_handle_memory_pool_get(swigCPtr);
      SWIGTYPE_p_apr_pool_t ret = (cPtr == IntPtr.Zero) ? null : new SWIGTYPE_p_apr_pool_t(cPtr, false);
      return ret;
    } 
  }

  public uint prebuf {
    set {
      freeswitchPINVOKE.switch_file_handle_prebuf_set(swigCPtr, value);
    } 
    get {
      uint ret = freeswitchPINVOKE.switch_file_handle_prebuf_get(swigCPtr);
      return ret;
    } 
  }

  public uint interval {
    set {
      freeswitchPINVOKE.switch_file_handle_interval_set(swigCPtr, value);
    } 
    get {
      uint ret = freeswitchPINVOKE.switch_file_handle_interval_get(swigCPtr);
      return ret;
    } 
  }

  public SWIGTYPE_p_void private_info {
    set {
      freeswitchPINVOKE.switch_file_handle_private_info_set(swigCPtr, SWIGTYPE_p_void.getCPtr(value));
    } 
    get {
      IntPtr cPtr = freeswitchPINVOKE.switch_file_handle_private_info_get(swigCPtr);
      SWIGTYPE_p_void ret = (cPtr == IntPtr.Zero) ? null : new SWIGTYPE_p_void(cPtr, false);
      return ret;
    } 
  }

  public string handler {
    set {
      freeswitchPINVOKE.switch_file_handle_handler_set(swigCPtr, value);
    } 
    get {
      string ret = freeswitchPINVOKE.switch_file_handle_handler_get(swigCPtr);
      return ret;
    } 
  }

  public long pos {
    set {
      freeswitchPINVOKE.switch_file_handle_pos_set(swigCPtr, value);
    } 
    get {
      long ret = freeswitchPINVOKE.switch_file_handle_pos_get(swigCPtr);
      return ret;
    } 
  }

  public SWIGTYPE_p_switch_buffer audio_buffer {
    set {
      freeswitchPINVOKE.switch_file_handle_audio_buffer_set(swigCPtr, SWIGTYPE_p_switch_buffer.getCPtr(value));
    } 
    get {
      IntPtr cPtr = freeswitchPINVOKE.switch_file_handle_audio_buffer_get(swigCPtr);
      SWIGTYPE_p_switch_buffer ret = (cPtr == IntPtr.Zero) ? null : new SWIGTYPE_p_switch_buffer(cPtr, false);
      return ret;
    } 
  }

  public SWIGTYPE_p_switch_buffer sp_audio_buffer {
    set {
      freeswitchPINVOKE.switch_file_handle_sp_audio_buffer_set(swigCPtr, SWIGTYPE_p_switch_buffer.getCPtr(value));
    } 
    get {
      IntPtr cPtr = freeswitchPINVOKE.switch_file_handle_sp_audio_buffer_get(swigCPtr);
      SWIGTYPE_p_switch_buffer ret = (cPtr == IntPtr.Zero) ? null : new SWIGTYPE_p_switch_buffer(cPtr, false);
      return ret;
    } 
  }

  public uint thresh {
    set {
      freeswitchPINVOKE.switch_file_handle_thresh_set(swigCPtr, value);
    } 
    get {
      uint ret = freeswitchPINVOKE.switch_file_handle_thresh_get(swigCPtr);
      return ret;
    } 
  }

  public uint silence_hits {
    set {
      freeswitchPINVOKE.switch_file_handle_silence_hits_set(swigCPtr, value);
    } 
    get {
      uint ret = freeswitchPINVOKE.switch_file_handle_silence_hits_get(swigCPtr);
      return ret;
    } 
  }

  public uint offset_pos {
    set {
      freeswitchPINVOKE.switch_file_handle_offset_pos_set(swigCPtr, value);
    } 
    get {
      uint ret = freeswitchPINVOKE.switch_file_handle_offset_pos_get(swigCPtr);
      return ret;
    } 
  }

  public uint last_pos {
    set {
      freeswitchPINVOKE.switch_file_handle_last_pos_set(swigCPtr, value);
    } 
    get {
      uint ret = freeswitchPINVOKE.switch_file_handle_last_pos_get(swigCPtr);
      return ret;
    } 
  }

  public int vol {
    set {
      freeswitchPINVOKE.switch_file_handle_vol_set(swigCPtr, value);
    } 
    get {
      int ret = freeswitchPINVOKE.switch_file_handle_vol_get(swigCPtr);
      return ret;
    } 
  }

  public switch_audio_resampler_t resampler {
    set {
      freeswitchPINVOKE.switch_file_handle_resampler_set(swigCPtr, switch_audio_resampler_t.getCPtr(value));
    } 
    get {
      IntPtr cPtr = freeswitchPINVOKE.switch_file_handle_resampler_get(swigCPtr);
      switch_audio_resampler_t ret = (cPtr == IntPtr.Zero) ? null : new switch_audio_resampler_t(cPtr, false);
      return ret;
    } 
  }

  public SWIGTYPE_p_switch_buffer buffer {
    set {
      freeswitchPINVOKE.switch_file_handle_buffer_set(swigCPtr, SWIGTYPE_p_switch_buffer.getCPtr(value));
    } 
    get {
      IntPtr cPtr = freeswitchPINVOKE.switch_file_handle_buffer_get(swigCPtr);
      SWIGTYPE_p_switch_buffer ret = (cPtr == IntPtr.Zero) ? null : new SWIGTYPE_p_switch_buffer(cPtr, false);
      return ret;
    } 
  }

  public SWIGTYPE_p_unsigned_char dbuf {
    set {
      freeswitchPINVOKE.switch_file_handle_dbuf_set(swigCPtr, SWIGTYPE_p_unsigned_char.getCPtr(value));
    } 
    get {
      IntPtr cPtr = freeswitchPINVOKE.switch_file_handle_dbuf_get(swigCPtr);
      SWIGTYPE_p_unsigned_char ret = (cPtr == IntPtr.Zero) ? null : new SWIGTYPE_p_unsigned_char(cPtr, false);
      return ret;
    } 
  }

  public SWIGTYPE_p_switch_size_t dbuflen {
    set {
      freeswitchPINVOKE.switch_file_handle_dbuflen_set(swigCPtr, SWIGTYPE_p_switch_size_t.getCPtr(value));
      if (freeswitchPINVOKE.SWIGPendingException.Pending) throw freeswitchPINVOKE.SWIGPendingException.Retrieve();
    } 
    get {
      SWIGTYPE_p_switch_size_t ret = new SWIGTYPE_p_switch_size_t(freeswitchPINVOKE.switch_file_handle_dbuflen_get(swigCPtr), true);
      if (freeswitchPINVOKE.SWIGPendingException.Pending) throw freeswitchPINVOKE.SWIGPendingException.Retrieve();
      return ret;
    } 
  }

  public string file {
    set {
      freeswitchPINVOKE.switch_file_handle_file_set(swigCPtr, value);
    } 
    get {
      string ret = freeswitchPINVOKE.switch_file_handle_file_get(swigCPtr);
      return ret;
    } 
  }

  public string func {
    set {
      freeswitchPINVOKE.switch_file_handle_func_set(swigCPtr, value);
    } 
    get {
      string ret = freeswitchPINVOKE.switch_file_handle_func_get(swigCPtr);
      return ret;
    } 
  }

  public int line {
    set {
      freeswitchPINVOKE.switch_file_handle_line_set(swigCPtr, value);
    } 
    get {
      int ret = freeswitchPINVOKE.switch_file_handle_line_get(swigCPtr);
      return ret;
    } 
  }

  public switch_file_handle() : this(freeswitchPINVOKE.new_switch_file_handle(), true) {
  }

}

}

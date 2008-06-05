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

public class switch_io_routines : IDisposable {
  private HandleRef swigCPtr;
  protected bool swigCMemOwn;

  internal switch_io_routines(IntPtr cPtr, bool cMemoryOwn) {
    swigCMemOwn = cMemoryOwn;
    swigCPtr = new HandleRef(this, cPtr);
  }

  internal static HandleRef getCPtr(switch_io_routines obj) {
    return (obj == null) ? new HandleRef(null, IntPtr.Zero) : obj.swigCPtr;
  }

  ~switch_io_routines() {
    Dispose();
  }

  public virtual void Dispose() {
    lock(this) {
      if(swigCPtr.Handle != IntPtr.Zero && swigCMemOwn) {
        swigCMemOwn = false;
        freeswitchPINVOKE.delete_switch_io_routines(swigCPtr);
      }
      swigCPtr = new HandleRef(null, IntPtr.Zero);
      GC.SuppressFinalize(this);
    }
  }

  public SWIGTYPE_p_f_p_switch_core_session_p_switch_event_p_switch_caller_profile_p_p_switch_core_session_p_p_apr_pool_t_unsigned_long__switch_call_cause_t outgoing_channel {
    set {
      freeswitchPINVOKE.switch_io_routines_outgoing_channel_set(swigCPtr, SWIGTYPE_p_f_p_switch_core_session_p_switch_event_p_switch_caller_profile_p_p_switch_core_session_p_p_apr_pool_t_unsigned_long__switch_call_cause_t.getCPtr(value));
    } 
    get {
      IntPtr cPtr = freeswitchPINVOKE.switch_io_routines_outgoing_channel_get(swigCPtr);
      SWIGTYPE_p_f_p_switch_core_session_p_switch_event_p_switch_caller_profile_p_p_switch_core_session_p_p_apr_pool_t_unsigned_long__switch_call_cause_t ret = (cPtr == IntPtr.Zero) ? null : new SWIGTYPE_p_f_p_switch_core_session_p_switch_event_p_switch_caller_profile_p_p_switch_core_session_p_p_apr_pool_t_unsigned_long__switch_call_cause_t(cPtr, false);
      return ret;
    } 
  }

  public SWIGTYPE_p_f_p_switch_core_session_p_p_switch_frame_unsigned_long_int__switch_status_t read_frame {
    set {
      freeswitchPINVOKE.switch_io_routines_read_frame_set(swigCPtr, SWIGTYPE_p_f_p_switch_core_session_p_p_switch_frame_unsigned_long_int__switch_status_t.getCPtr(value));
    } 
    get {
      IntPtr cPtr = freeswitchPINVOKE.switch_io_routines_read_frame_get(swigCPtr);
      SWIGTYPE_p_f_p_switch_core_session_p_p_switch_frame_unsigned_long_int__switch_status_t ret = (cPtr == IntPtr.Zero) ? null : new SWIGTYPE_p_f_p_switch_core_session_p_p_switch_frame_unsigned_long_int__switch_status_t(cPtr, false);
      return ret;
    } 
  }

  public SWIGTYPE_p_f_p_switch_core_session_p_switch_frame_unsigned_long_int__switch_status_t write_frame {
    set {
      freeswitchPINVOKE.switch_io_routines_write_frame_set(swigCPtr, SWIGTYPE_p_f_p_switch_core_session_p_switch_frame_unsigned_long_int__switch_status_t.getCPtr(value));
    } 
    get {
      IntPtr cPtr = freeswitchPINVOKE.switch_io_routines_write_frame_get(swigCPtr);
      SWIGTYPE_p_f_p_switch_core_session_p_switch_frame_unsigned_long_int__switch_status_t ret = (cPtr == IntPtr.Zero) ? null : new SWIGTYPE_p_f_p_switch_core_session_p_switch_frame_unsigned_long_int__switch_status_t(cPtr, false);
      return ret;
    } 
  }

  public SWIGTYPE_p_f_p_switch_core_session_int__switch_status_t kill_channel {
    set {
      freeswitchPINVOKE.switch_io_routines_kill_channel_set(swigCPtr, SWIGTYPE_p_f_p_switch_core_session_int__switch_status_t.getCPtr(value));
    } 
    get {
      IntPtr cPtr = freeswitchPINVOKE.switch_io_routines_kill_channel_get(swigCPtr);
      SWIGTYPE_p_f_p_switch_core_session_int__switch_status_t ret = (cPtr == IntPtr.Zero) ? null : new SWIGTYPE_p_f_p_switch_core_session_int__switch_status_t(cPtr, false);
      return ret;
    } 
  }

  public SWIGTYPE_p_f_p_switch_core_session_p_q_const__switch_dtmf_t__switch_status_t send_dtmf {
    set {
      freeswitchPINVOKE.switch_io_routines_send_dtmf_set(swigCPtr, SWIGTYPE_p_f_p_switch_core_session_p_q_const__switch_dtmf_t__switch_status_t.getCPtr(value));
    } 
    get {
      IntPtr cPtr = freeswitchPINVOKE.switch_io_routines_send_dtmf_get(swigCPtr);
      SWIGTYPE_p_f_p_switch_core_session_p_q_const__switch_dtmf_t__switch_status_t ret = (cPtr == IntPtr.Zero) ? null : new SWIGTYPE_p_f_p_switch_core_session_p_q_const__switch_dtmf_t__switch_status_t(cPtr, false);
      return ret;
    } 
  }

  public SWIGTYPE_p_f_p_switch_core_session_p_switch_core_session_message__switch_status_t receive_message {
    set {
      freeswitchPINVOKE.switch_io_routines_receive_message_set(swigCPtr, SWIGTYPE_p_f_p_switch_core_session_p_switch_core_session_message__switch_status_t.getCPtr(value));
    } 
    get {
      IntPtr cPtr = freeswitchPINVOKE.switch_io_routines_receive_message_get(swigCPtr);
      SWIGTYPE_p_f_p_switch_core_session_p_switch_core_session_message__switch_status_t ret = (cPtr == IntPtr.Zero) ? null : new SWIGTYPE_p_f_p_switch_core_session_p_switch_core_session_message__switch_status_t(cPtr, false);
      return ret;
    } 
  }

  public SWIGTYPE_p_f_p_switch_core_session_p_switch_event__switch_status_t receive_event {
    set {
      freeswitchPINVOKE.switch_io_routines_receive_event_set(swigCPtr, SWIGTYPE_p_f_p_switch_core_session_p_switch_event__switch_status_t.getCPtr(value));
    } 
    get {
      IntPtr cPtr = freeswitchPINVOKE.switch_io_routines_receive_event_get(swigCPtr);
      SWIGTYPE_p_f_p_switch_core_session_p_switch_event__switch_status_t ret = (cPtr == IntPtr.Zero) ? null : new SWIGTYPE_p_f_p_switch_core_session_p_switch_event__switch_status_t(cPtr, false);
      return ret;
    } 
  }

  public SWIGTYPE_p_f_p_switch_core_session__switch_status_t state_change {
    set {
      freeswitchPINVOKE.switch_io_routines_state_change_set(swigCPtr, SWIGTYPE_p_f_p_switch_core_session__switch_status_t.getCPtr(value));
    } 
    get {
      IntPtr cPtr = freeswitchPINVOKE.switch_io_routines_state_change_get(swigCPtr);
      SWIGTYPE_p_f_p_switch_core_session__switch_status_t ret = (cPtr == IntPtr.Zero) ? null : new SWIGTYPE_p_f_p_switch_core_session__switch_status_t(cPtr, false);
      return ret;
    } 
  }

  public SWIGTYPE_p_f_p_switch_core_session_p_p_switch_frame_unsigned_long_int__switch_status_t read_video_frame {
    set {
      freeswitchPINVOKE.switch_io_routines_read_video_frame_set(swigCPtr, SWIGTYPE_p_f_p_switch_core_session_p_p_switch_frame_unsigned_long_int__switch_status_t.getCPtr(value));
    } 
    get {
      IntPtr cPtr = freeswitchPINVOKE.switch_io_routines_read_video_frame_get(swigCPtr);
      SWIGTYPE_p_f_p_switch_core_session_p_p_switch_frame_unsigned_long_int__switch_status_t ret = (cPtr == IntPtr.Zero) ? null : new SWIGTYPE_p_f_p_switch_core_session_p_p_switch_frame_unsigned_long_int__switch_status_t(cPtr, false);
      return ret;
    } 
  }

  public SWIGTYPE_p_f_p_switch_core_session_p_switch_frame_unsigned_long_int__switch_status_t write_video_frame {
    set {
      freeswitchPINVOKE.switch_io_routines_write_video_frame_set(swigCPtr, SWIGTYPE_p_f_p_switch_core_session_p_switch_frame_unsigned_long_int__switch_status_t.getCPtr(value));
    } 
    get {
      IntPtr cPtr = freeswitchPINVOKE.switch_io_routines_write_video_frame_get(swigCPtr);
      SWIGTYPE_p_f_p_switch_core_session_p_switch_frame_unsigned_long_int__switch_status_t ret = (cPtr == IntPtr.Zero) ? null : new SWIGTYPE_p_f_p_switch_core_session_p_switch_frame_unsigned_long_int__switch_status_t(cPtr, false);
      return ret;
    } 
  }

  public SWIGTYPE_p_f_p_p_switch_core_session_p_p_apr_pool_t_p_void__switch_call_cause_t resurrect_session {
    set {
      freeswitchPINVOKE.switch_io_routines_resurrect_session_set(swigCPtr, SWIGTYPE_p_f_p_p_switch_core_session_p_p_apr_pool_t_p_void__switch_call_cause_t.getCPtr(value));
    } 
    get {
      IntPtr cPtr = freeswitchPINVOKE.switch_io_routines_resurrect_session_get(swigCPtr);
      SWIGTYPE_p_f_p_p_switch_core_session_p_p_apr_pool_t_p_void__switch_call_cause_t ret = (cPtr == IntPtr.Zero) ? null : new SWIGTYPE_p_f_p_p_switch_core_session_p_p_apr_pool_t_p_void__switch_call_cause_t(cPtr, false);
      return ret;
    } 
  }

  public SWIGTYPE_p_p_void padding {
    set {
      freeswitchPINVOKE.switch_io_routines_padding_set(swigCPtr, SWIGTYPE_p_p_void.getCPtr(value));
    } 
    get {
      IntPtr cPtr = freeswitchPINVOKE.switch_io_routines_padding_get(swigCPtr);
      SWIGTYPE_p_p_void ret = (cPtr == IntPtr.Zero) ? null : new SWIGTYPE_p_p_void(cPtr, false);
      return ret;
    } 
  }

  public switch_io_routines() : this(freeswitchPINVOKE.new_switch_io_routines(), true) {
  }

}

}

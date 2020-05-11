/* ----------------------------------------------------------------------------
 * This file was automatically generated by SWIG (http://www.swig.org).
 * Version 3.0.12
 *
 * Do not make changes to this file unless you know what you are doing--modify
 * the SWIG interface file instead.
 * ----------------------------------------------------------------------------- */

package org.freeswitch.swig;

public class input_callback_state_t {
  private transient long swigCPtr;
  protected transient boolean swigCMemOwn;

  protected input_callback_state_t(long cPtr, boolean cMemoryOwn) {
    swigCMemOwn = cMemoryOwn;
    swigCPtr = cPtr;
  }

  protected static long getCPtr(input_callback_state_t obj) {
    return (obj == null) ? 0 : obj.swigCPtr;
  }

  protected void finalize() {
    delete();
  }

  public synchronized void delete() {
    if (swigCPtr != 0) {
      if (swigCMemOwn) {
        swigCMemOwn = false;
        freeswitchJNI.delete_input_callback_state_t(swigCPtr);
      }
      swigCPtr = 0;
    }
  }

  public void setFunction(SWIGTYPE_p_void value) {
    freeswitchJNI.input_callback_state_t_function_set(swigCPtr, this, SWIGTYPE_p_void.getCPtr(value));
  }

  public SWIGTYPE_p_void getFunction() {
    long cPtr = freeswitchJNI.input_callback_state_t_function_get(swigCPtr, this);
    return (cPtr == 0) ? null : new SWIGTYPE_p_void(cPtr, false);
  }

  public void setThreadState(SWIGTYPE_p_void value) {
    freeswitchJNI.input_callback_state_t_threadState_set(swigCPtr, this, SWIGTYPE_p_void.getCPtr(value));
  }

  public SWIGTYPE_p_void getThreadState() {
    long cPtr = freeswitchJNI.input_callback_state_t_threadState_get(swigCPtr, this);
    return (cPtr == 0) ? null : new SWIGTYPE_p_void(cPtr, false);
  }

  public void setExtra(SWIGTYPE_p_void value) {
    freeswitchJNI.input_callback_state_t_extra_set(swigCPtr, this, SWIGTYPE_p_void.getCPtr(value));
  }

  public SWIGTYPE_p_void getExtra() {
    long cPtr = freeswitchJNI.input_callback_state_t_extra_get(swigCPtr, this);
    return (cPtr == 0) ? null : new SWIGTYPE_p_void(cPtr, false);
  }

  public void setFuncargs(String value) {
    freeswitchJNI.input_callback_state_t_funcargs_set(swigCPtr, this, value);
  }

  public String getFuncargs() {
    return freeswitchJNI.input_callback_state_t_funcargs_get(swigCPtr, this);
  }

  public input_callback_state_t() {
    this(freeswitchJNI.new_input_callback_state_t(), true);
  }

}

/* ----------------------------------------------------------------------------
 * This file was automatically generated by SWIG (http://www.swig.org).
 * Version 3.0.10
 *
 * Do not make changes to this file unless you know what you are doing--modify
 * the SWIG interface file instead.
 * ----------------------------------------------------------------------------- */

package org.freeswitch.swig;

public class DTMF {
  private transient long swigCPtr;
  protected transient boolean swigCMemOwn;

  protected DTMF(long cPtr, boolean cMemoryOwn) {
    swigCMemOwn = cMemoryOwn;
    swigCPtr = cPtr;
  }

  protected static long getCPtr(DTMF obj) {
    return (obj == null) ? 0 : obj.swigCPtr;
  }

  protected void finalize() {
    delete();
  }

  public synchronized void delete() {
    if (swigCPtr != 0) {
      if (swigCMemOwn) {
        swigCMemOwn = false;
        freeswitchJNI.delete_DTMF(swigCPtr);
      }
      swigCPtr = 0;
    }
  }

  public void setDigit(char value) {
    freeswitchJNI.DTMF_digit_set(swigCPtr, this, value);
  }

  public char getDigit() {
    return freeswitchJNI.DTMF_digit_get(swigCPtr, this);
  }

  public void setDuration(SWIGTYPE_p_uint32_t value) {
    freeswitchJNI.DTMF_duration_set(swigCPtr, this, SWIGTYPE_p_uint32_t.getCPtr(value));
  }

  public SWIGTYPE_p_uint32_t getDuration() {
    return new SWIGTYPE_p_uint32_t(freeswitchJNI.DTMF_duration_get(swigCPtr, this), true);
  }

  public DTMF(char idigit, SWIGTYPE_p_uint32_t iduration) {
    this(freeswitchJNI.new_DTMF(idigit, SWIGTYPE_p_uint32_t.getCPtr(iduration)), true);
  }

}

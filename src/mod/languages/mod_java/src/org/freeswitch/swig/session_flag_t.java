/* ----------------------------------------------------------------------------
 * This file was automatically generated by SWIG (http://www.swig.org).
 * Version 3.0.12
 *
 * Do not make changes to this file unless you know what you are doing--modify
 * the SWIG interface file instead.
 * ----------------------------------------------------------------------------- */

package org.freeswitch.swig;

public enum session_flag_t {
  S_HUP(freeswitchJNI.S_HUP_get()),
  S_FREE(freeswitchJNI.S_FREE_get()),
  S_RDLOCK(freeswitchJNI.S_RDLOCK_get());

  public final int swigValue() {
    return swigValue;
  }

  public static session_flag_t swigToEnum(int swigValue) {
    session_flag_t[] swigValues = session_flag_t.class.getEnumConstants();
    if (swigValue < swigValues.length && swigValue >= 0 && swigValues[swigValue].swigValue == swigValue)
      return swigValues[swigValue];
    for (session_flag_t swigEnum : swigValues)
      if (swigEnum.swigValue == swigValue)
        return swigEnum;
    throw new IllegalArgumentException("No enum " + session_flag_t.class + " with value " + swigValue);
  }

  @SuppressWarnings("unused")
  private session_flag_t() {
    this.swigValue = SwigNext.next++;
  }

  @SuppressWarnings("unused")
  private session_flag_t(int swigValue) {
    this.swigValue = swigValue;
    SwigNext.next = swigValue+1;
  }

  @SuppressWarnings("unused")
  private session_flag_t(session_flag_t swigEnum) {
    this.swigValue = swigEnum.swigValue;
    SwigNext.next = this.swigValue+1;
  }

  private final int swigValue;

  private static class SwigNext {
    private static int next = 0;
  }
}


/* ----------------------------------------------------------------------------
 * This file was automatically generated by SWIG (http://www.swig.org).
 * Version 1.3.29
 *
 * Do not make changes to this file unless you know what you are doing--modify
 * the SWIG interface file instead.
 * ----------------------------------------------------------------------------- */

package org.freeswitch.swig;

public class Stream {
  private long swigCPtr;
  protected boolean swigCMemOwn;

  protected Stream(long cPtr, boolean cMemoryOwn) {
    swigCMemOwn = cMemoryOwn;
    swigCPtr = cPtr;
  }

  protected static long getCPtr(Stream obj) {
    return (obj == null) ? 0 : obj.swigCPtr;
  }

  protected void finalize() {
    delete();
  }

  public void delete() {
    if(swigCPtr != 0 && swigCMemOwn) {
      swigCMemOwn = false;
      freeswitchJNI.delete_Stream(swigCPtr);
    }
    swigCPtr = 0;
  }

  public Stream() {
    this(freeswitchJNI.new_Stream__SWIG_0(), true);
  }

  public Stream(SWIGTYPE_p_switch_stream_handle_t arg0) {
    this(freeswitchJNI.new_Stream__SWIG_1(SWIGTYPE_p_switch_stream_handle_t.getCPtr(arg0)), true);
  }

  public String read(SWIGTYPE_p_int len) {
    return freeswitchJNI.Stream_read(swigCPtr, SWIGTYPE_p_int.getCPtr(len));
  }

  public void write(String data) {
    freeswitchJNI.Stream_write(swigCPtr, data);
  }

  public void raw_write(String data, int len) {
    freeswitchJNI.Stream_raw_write(swigCPtr, data, len);
  }

  public String get_data() {
    return freeswitchJNI.Stream_get_data(swigCPtr);
  }

}

/* ----------------------------------------------------------------------------
 * This file was automatically generated by SWIG (http://www.swig.org).
 * Version 1.3.35
 *
 * Do not make changes to this file unless you know what you are doing--modify
 * the SWIG interface file instead.
 * ----------------------------------------------------------------------------- */

package org.freeswitch.swig;

public class IVRMenu {
  private long swigCPtr;
  protected boolean swigCMemOwn;

  protected IVRMenu(long cPtr, boolean cMemoryOwn) {
    swigCMemOwn = cMemoryOwn;
    swigCPtr = cPtr;
  }

  protected static long getCPtr(IVRMenu obj) {
    return (obj == null) ? 0 : obj.swigCPtr;
  }

  protected void finalize() {
    delete();
  }

  public synchronized void delete() {
    if(swigCPtr != 0 && swigCMemOwn) {
      swigCMemOwn = false;
      freeswitchJNI.delete_IVRMenu(swigCPtr);
    }
    swigCPtr = 0;
  }

  public IVRMenu(IVRMenu main, String name, String greeting_sound, String short_greeting_sound, String invalid_sound, String exit_sound, String transfer_sound, String confirm_macro, String confirm_key, String tts_engine, String tts_voice, int confirm_attempts, int inter_timeout, int digit_len, int timeout, int max_failures, int max_timeouts) {
    this(freeswitchJNI.new_IVRMenu(IVRMenu.getCPtr(main), main, name, greeting_sound, short_greeting_sound, invalid_sound, exit_sound, transfer_sound, confirm_macro, confirm_key, tts_engine, tts_voice, confirm_attempts, inter_timeout, digit_len, timeout, max_failures, max_timeouts), true);
  }

  public void bindAction(String action, String arg, String bind) {
    freeswitchJNI.IVRMenu_bindAction(swigCPtr, this, action, arg, bind);
  }

  public void execute(CoreSession session, String name) {
    freeswitchJNI.IVRMenu_execute(swigCPtr, this, CoreSession.getCPtr(session), session, name);
  }

}

/* ----------------------------------------------------------------------------
 * This file was automatically generated by SWIG (http://www.swig.org).
 * Version 1.3.35
 *
 * Do not make changes to this file unless you know what you are doing--modify
 * the SWIG interface file instead.
 * ----------------------------------------------------------------------------- */

package org.freeswitch.swig;

public class EventConsumer {
  private long swigCPtr;
  protected boolean swigCMemOwn;

  protected EventConsumer(long cPtr, boolean cMemoryOwn) {
    swigCMemOwn = cMemoryOwn;
    swigCPtr = cPtr;
  }

  protected static long getCPtr(EventConsumer obj) {
    return (obj == null) ? 0 : obj.swigCPtr;
  }

  protected void finalize() {
    delete();
  }

  public synchronized void delete() {
    if(swigCPtr != 0 && swigCMemOwn) {
      swigCMemOwn = false;
      freeswitchJNI.delete_EventConsumer(swigCPtr);
    }
    swigCPtr = 0;
  }

  public void setEvents(SWIGTYPE_p_switch_queue_t value) {
    freeswitchJNI.EventConsumer_events_set(swigCPtr, this, SWIGTYPE_p_switch_queue_t.getCPtr(value));
  }

  public SWIGTYPE_p_switch_queue_t getEvents() {
    long cPtr = freeswitchJNI.EventConsumer_events_get(swigCPtr, this);
    return (cPtr == 0) ? null : new SWIGTYPE_p_switch_queue_t(cPtr, false);
  }

  public void setE_event_id(SWIGTYPE_p_switch_event_types_t value) {
    freeswitchJNI.EventConsumer_e_event_id_set(swigCPtr, this, SWIGTYPE_p_switch_event_types_t.getCPtr(value));
  }

  public SWIGTYPE_p_switch_event_types_t getE_event_id() {
    return new SWIGTYPE_p_switch_event_types_t(freeswitchJNI.EventConsumer_e_event_id_get(swigCPtr, this), true);
  }

  public void setNode(SWIGTYPE_p_switch_event_node_t value) {
    freeswitchJNI.EventConsumer_node_set(swigCPtr, this, SWIGTYPE_p_switch_event_node_t.getCPtr(value));
  }

  public SWIGTYPE_p_switch_event_node_t getNode() {
    long cPtr = freeswitchJNI.EventConsumer_node_get(swigCPtr, this);
    return (cPtr == 0) ? null : new SWIGTYPE_p_switch_event_node_t(cPtr, false);
  }

  public void setE_callback(String value) {
    freeswitchJNI.EventConsumer_e_callback_set(swigCPtr, this, value);
  }

  public String getE_callback() {
    return freeswitchJNI.EventConsumer_e_callback_get(swigCPtr, this);
  }

  public void setE_subclass_name(String value) {
    freeswitchJNI.EventConsumer_e_subclass_name_set(swigCPtr, this, value);
  }

  public String getE_subclass_name() {
    return freeswitchJNI.EventConsumer_e_subclass_name_get(swigCPtr, this);
  }

  public void setE_cb_arg(String value) {
    freeswitchJNI.EventConsumer_e_cb_arg_set(swigCPtr, this, value);
  }

  public String getE_cb_arg() {
    return freeswitchJNI.EventConsumer_e_cb_arg_get(swigCPtr, this);
  }

  public EventConsumer(String event_name, String subclass_name) {
    this(freeswitchJNI.new_EventConsumer__SWIG_0(event_name, subclass_name), true);
  }

  public EventConsumer(String event_name) {
    this(freeswitchJNI.new_EventConsumer__SWIG_1(event_name), true);
  }

  public Event pop(int block) {
    long cPtr = freeswitchJNI.EventConsumer_pop__SWIG_0(swigCPtr, this, block);
    return (cPtr == 0) ? null : new Event(cPtr, false);
  }

  public Event pop() {
    long cPtr = freeswitchJNI.EventConsumer_pop__SWIG_1(swigCPtr, this);
    return (cPtr == 0) ? null : new Event(cPtr, false);
  }

}

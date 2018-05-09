//------------------------------------------------------------------------------
// <auto-generated />
//
// This file was automatically generated by SWIG (http://www.swig.org).
// Version 3.0.12
//
// Do not make changes to this file unless you know what you are doing--modify
// the SWIG interface file instead.
//------------------------------------------------------------------------------


public class ESLevent : global::System.IDisposable {
  private global::System.Runtime.InteropServices.HandleRef swigCPtr;
  protected bool swigCMemOwn;

  internal ESLevent(global::System.IntPtr cPtr, bool cMemoryOwn) {
    swigCMemOwn = cMemoryOwn;
    swigCPtr = new global::System.Runtime.InteropServices.HandleRef(this, cPtr);
  }

  internal static global::System.Runtime.InteropServices.HandleRef getCPtr(ESLevent obj) {
    return (obj == null) ? new global::System.Runtime.InteropServices.HandleRef(null, global::System.IntPtr.Zero) : obj.swigCPtr;
  }

  ~ESLevent() {
    Dispose();
  }

  public virtual void Dispose() {
    lock(this) {
      if (swigCPtr.Handle != global::System.IntPtr.Zero) {
        if (swigCMemOwn) {
          swigCMemOwn = false;
          ESLPINVOKE.delete_ESLevent(swigCPtr);
        }
        swigCPtr = new global::System.Runtime.InteropServices.HandleRef(null, global::System.IntPtr.Zero);
      }
      global::System.GC.SuppressFinalize(this);
    }
  }

  public SWIGTYPE_p_esl_event_t Event {
    set {
      ESLPINVOKE.ESLevent_Event_set(swigCPtr, SWIGTYPE_p_esl_event_t.getCPtr(value));
    } 
    get {
      global::System.IntPtr cPtr = ESLPINVOKE.ESLevent_Event_get(swigCPtr);
      SWIGTYPE_p_esl_event_t ret = (cPtr == global::System.IntPtr.Zero) ? null : new SWIGTYPE_p_esl_event_t(cPtr, false);
      return ret;
    } 
  }

  public string SerializedString {
    set {
      ESLPINVOKE.ESLevent_SerializedString_set(swigCPtr, value);
    } 
    get {
      string ret = ESLPINVOKE.ESLevent_SerializedString_get(swigCPtr);
      return ret;
    } 
  }

  public int Mine {
    set {
      ESLPINVOKE.ESLevent_Mine_set(swigCPtr, value);
    } 
    get {
      int ret = ESLPINVOKE.ESLevent_Mine_get(swigCPtr);
      return ret;
    } 
  }

  public ESLevent(string type, string subclass_name) : this(ESLPINVOKE.new_ESLevent__SWIG_0(type, subclass_name), true) {
  }

  public ESLevent(SWIGTYPE_p_esl_event_t wrap_me, int free_me) : this(ESLPINVOKE.new_ESLevent__SWIG_1(SWIGTYPE_p_esl_event_t.getCPtr(wrap_me), free_me), true) {
  }

  public ESLevent(ESLevent me) : this(ESLPINVOKE.new_ESLevent__SWIG_2(ESLevent.getCPtr(me)), true) {
  }

  public string Serialize(string format) {
    string ret = ESLPINVOKE.ESLevent_Serialize(swigCPtr, format);
    return ret;
  }

  public bool SetPriority(SWIGTYPE_p_esl_priority_t priority) {
    bool ret = ESLPINVOKE.ESLevent_SetPriority(swigCPtr, SWIGTYPE_p_esl_priority_t.getCPtr(priority));
    if (ESLPINVOKE.SWIGPendingException.Pending) throw ESLPINVOKE.SWIGPendingException.Retrieve();
    return ret;
  }

  public string GetHeader(string header_name, int idx) {
    string ret = ESLPINVOKE.ESLevent_GetHeader(swigCPtr, header_name, idx);
    return ret;
  }

  public string GetBody() {
    string ret = ESLPINVOKE.ESLevent_GetBody(swigCPtr);
    return ret;
  }

  public string getType() {
    string ret = ESLPINVOKE.ESLevent_getType(swigCPtr);
    return ret;
  }

  public bool AddBody(string value) {
    bool ret = ESLPINVOKE.ESLevent_AddBody(swigCPtr, value);
    return ret;
  }

  public bool AddHeader(string header_name, string value) {
    bool ret = ESLPINVOKE.ESLevent_AddHeader(swigCPtr, header_name, value);
    return ret;
  }

  public bool pushHeader(string header_name, string value) {
    bool ret = ESLPINVOKE.ESLevent_pushHeader(swigCPtr, header_name, value);
    return ret;
  }

  public bool unshiftHeader(string header_name, string value) {
    bool ret = ESLPINVOKE.ESLevent_unshiftHeader(swigCPtr, header_name, value);
    return ret;
  }

  public bool DelHeader(string header_name) {
    bool ret = ESLPINVOKE.ESLevent_DelHeader(swigCPtr, header_name);
    return ret;
  }

  public string FirstHeader() {
    string ret = ESLPINVOKE.ESLevent_FirstHeader(swigCPtr);
    return ret;
  }

  public string NextHeader() {
    string ret = ESLPINVOKE.ESLevent_NextHeader(swigCPtr);
    return ret;
  }

}

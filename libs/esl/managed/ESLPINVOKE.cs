/* ----------------------------------------------------------------------------
 * This file was automatically generated by SWIG (http://www.swig.org).
 * Version 1.3.35
 *
 * Do not make changes to this file unless you know what you are doing--modify
 * the SWIG interface file instead.
 * ----------------------------------------------------------------------------- */


using System;
using System.Runtime.InteropServices;

class ESLPINVOKE {

  protected class SWIGExceptionHelper {

    public delegate void ExceptionDelegate(string message);
    public delegate void ExceptionArgumentDelegate(string message, string paramName);

    static ExceptionDelegate applicationDelegate = new ExceptionDelegate(SetPendingApplicationException);
    static ExceptionDelegate arithmeticDelegate = new ExceptionDelegate(SetPendingArithmeticException);
    static ExceptionDelegate divideByZeroDelegate = new ExceptionDelegate(SetPendingDivideByZeroException);
    static ExceptionDelegate indexOutOfRangeDelegate = new ExceptionDelegate(SetPendingIndexOutOfRangeException);
    static ExceptionDelegate invalidCastDelegate = new ExceptionDelegate(SetPendingInvalidCastException);
    static ExceptionDelegate invalidOperationDelegate = new ExceptionDelegate(SetPendingInvalidOperationException);
    static ExceptionDelegate ioDelegate = new ExceptionDelegate(SetPendingIOException);
    static ExceptionDelegate nullReferenceDelegate = new ExceptionDelegate(SetPendingNullReferenceException);
    static ExceptionDelegate outOfMemoryDelegate = new ExceptionDelegate(SetPendingOutOfMemoryException);
    static ExceptionDelegate overflowDelegate = new ExceptionDelegate(SetPendingOverflowException);
    static ExceptionDelegate systemDelegate = new ExceptionDelegate(SetPendingSystemException);

    static ExceptionArgumentDelegate argumentDelegate = new ExceptionArgumentDelegate(SetPendingArgumentException);
    static ExceptionArgumentDelegate argumentNullDelegate = new ExceptionArgumentDelegate(SetPendingArgumentNullException);
    static ExceptionArgumentDelegate argumentOutOfRangeDelegate = new ExceptionArgumentDelegate(SetPendingArgumentOutOfRangeException);

    [DllImport("ESL", EntryPoint="SWIGRegisterExceptionCallbacks_ESL")]
    public static extern void SWIGRegisterExceptionCallbacks_ESL(
                                ExceptionDelegate applicationDelegate,
                                ExceptionDelegate arithmeticDelegate,
                                ExceptionDelegate divideByZeroDelegate, 
                                ExceptionDelegate indexOutOfRangeDelegate, 
                                ExceptionDelegate invalidCastDelegate,
                                ExceptionDelegate invalidOperationDelegate,
                                ExceptionDelegate ioDelegate,
                                ExceptionDelegate nullReferenceDelegate,
                                ExceptionDelegate outOfMemoryDelegate, 
                                ExceptionDelegate overflowDelegate, 
                                ExceptionDelegate systemExceptionDelegate);

    [DllImport("ESL", EntryPoint="SWIGRegisterExceptionArgumentCallbacks_ESL")]
    public static extern void SWIGRegisterExceptionCallbacksArgument_ESL(
                                ExceptionArgumentDelegate argumentDelegate,
                                ExceptionArgumentDelegate argumentNullDelegate,
                                ExceptionArgumentDelegate argumentOutOfRangeDelegate);

    static void SetPendingApplicationException(string message) {
      SWIGPendingException.Set(new System.ApplicationException(message, SWIGPendingException.Retrieve()));
    }
    static void SetPendingArithmeticException(string message) {
      SWIGPendingException.Set(new System.ArithmeticException(message, SWIGPendingException.Retrieve()));
    }
    static void SetPendingDivideByZeroException(string message) {
      SWIGPendingException.Set(new System.DivideByZeroException(message, SWIGPendingException.Retrieve()));
    }
    static void SetPendingIndexOutOfRangeException(string message) {
      SWIGPendingException.Set(new System.IndexOutOfRangeException(message, SWIGPendingException.Retrieve()));
    }
    static void SetPendingInvalidCastException(string message) {
      SWIGPendingException.Set(new System.InvalidCastException(message, SWIGPendingException.Retrieve()));
    }
    static void SetPendingInvalidOperationException(string message) {
      SWIGPendingException.Set(new System.InvalidOperationException(message, SWIGPendingException.Retrieve()));
    }
    static void SetPendingIOException(string message) {
      SWIGPendingException.Set(new System.IO.IOException(message, SWIGPendingException.Retrieve()));
    }
    static void SetPendingNullReferenceException(string message) {
      SWIGPendingException.Set(new System.NullReferenceException(message, SWIGPendingException.Retrieve()));
    }
    static void SetPendingOutOfMemoryException(string message) {
      SWIGPendingException.Set(new System.OutOfMemoryException(message, SWIGPendingException.Retrieve()));
    }
    static void SetPendingOverflowException(string message) {
      SWIGPendingException.Set(new System.OverflowException(message, SWIGPendingException.Retrieve()));
    }
    static void SetPendingSystemException(string message) {
      SWIGPendingException.Set(new System.SystemException(message, SWIGPendingException.Retrieve()));
    }

    static void SetPendingArgumentException(string message, string paramName) {
      SWIGPendingException.Set(new System.ArgumentException(message, paramName, SWIGPendingException.Retrieve()));
    }
    static void SetPendingArgumentNullException(string message, string paramName) {
      Exception e = SWIGPendingException.Retrieve();
      if (e != null) message = message + " Inner Exception: " + e.Message;
      SWIGPendingException.Set(new System.ArgumentNullException(paramName, message));
    }
    static void SetPendingArgumentOutOfRangeException(string message, string paramName) {
      Exception e = SWIGPendingException.Retrieve();
      if (e != null) message = message + " Inner Exception: " + e.Message;
      SWIGPendingException.Set(new System.ArgumentOutOfRangeException(paramName, message));
    }

    static SWIGExceptionHelper() {
      SWIGRegisterExceptionCallbacks_ESL(
                                applicationDelegate,
                                arithmeticDelegate,
                                divideByZeroDelegate,
                                indexOutOfRangeDelegate,
                                invalidCastDelegate,
                                invalidOperationDelegate,
                                ioDelegate,
                                nullReferenceDelegate,
                                outOfMemoryDelegate,
                                overflowDelegate,
                                systemDelegate);

      SWIGRegisterExceptionCallbacksArgument_ESL(
                                argumentDelegate,
                                argumentNullDelegate,
                                argumentOutOfRangeDelegate);
    }
  }

  protected static SWIGExceptionHelper swigExceptionHelper = new SWIGExceptionHelper();

  public class SWIGPendingException {
    [ThreadStatic]
    private static Exception pendingException = null;
    private static int numExceptionsPending = 0;

    public static bool Pending {
      get {
        bool pending = false;
        if (numExceptionsPending > 0)
          if (pendingException != null)
            pending = true;
        return pending;
      } 
    }

    public static void Set(Exception e) {
      if (pendingException != null)
        throw new ApplicationException("FATAL: An earlier pending exception from unmanaged code was missed and thus not thrown (" + pendingException.ToString() + ")", e);
      pendingException = e;
      lock(typeof(ESLPINVOKE)) {
        numExceptionsPending++;
      }
    }

    public static Exception Retrieve() {
      Exception e = null;
      if (numExceptionsPending > 0) {
        if (pendingException != null) {
          e = pendingException;
          pendingException = null;
          lock(typeof(ESLPINVOKE)) {
            numExceptionsPending--;
          }
        }
      }
      return e;
    }
  }


  protected class SWIGStringHelper {

    public delegate string SWIGStringDelegate(string message);
    static SWIGStringDelegate stringDelegate = new SWIGStringDelegate(CreateString);

    [DllImport("ESL", EntryPoint="SWIGRegisterStringCallback_ESL")]
    public static extern void SWIGRegisterStringCallback_ESL(SWIGStringDelegate stringDelegate);

    static string CreateString(string cString) {
      return cString;
    }

    static SWIGStringHelper() {
      SWIGRegisterStringCallback_ESL(stringDelegate);
    }
  }

  static protected SWIGStringHelper swigStringHelper = new SWIGStringHelper();


  [DllImport("ESL", EntryPoint="CSharp_ESLevent_event_set")]
  public static extern void ESLevent_event_set(HandleRef jarg1, HandleRef jarg2);

  [DllImport("ESL", EntryPoint="CSharp_ESLevent_event_get")]
  public static extern IntPtr ESLevent_event_get(HandleRef jarg1);

  [DllImport("ESL", EntryPoint="CSharp_ESLevent_serialized_string_set")]
  public static extern void ESLevent_serialized_string_set(HandleRef jarg1, string jarg2);

  [DllImport("ESL", EntryPoint="CSharp_ESLevent_serialized_string_get")]
  public static extern string ESLevent_serialized_string_get(HandleRef jarg1);

  [DllImport("ESL", EntryPoint="CSharp_ESLevent_mine_set")]
  public static extern void ESLevent_mine_set(HandleRef jarg1, int jarg2);

  [DllImport("ESL", EntryPoint="CSharp_ESLevent_mine_get")]
  public static extern int ESLevent_mine_get(HandleRef jarg1);

  [DllImport("ESL", EntryPoint="CSharp_new_ESLevent__SWIG_0")]
  public static extern IntPtr new_ESLevent__SWIG_0(string jarg1, string jarg2);

  [DllImport("ESL", EntryPoint="CSharp_new_ESLevent__SWIG_1")]
  public static extern IntPtr new_ESLevent__SWIG_1(HandleRef jarg1, int jarg2);

  [DllImport("ESL", EntryPoint="CSharp_new_ESLevent__SWIG_2")]
  public static extern IntPtr new_ESLevent__SWIG_2(HandleRef jarg1);

  [DllImport("ESL", EntryPoint="CSharp_delete_ESLevent")]
  public static extern void delete_ESLevent(HandleRef jarg1);

  [DllImport("ESL", EntryPoint="CSharp_ESLevent_serialize")]
  public static extern string ESLevent_serialize(HandleRef jarg1, string jarg2);

  [DllImport("ESL", EntryPoint="CSharp_ESLevent_setPriority")]
  public static extern bool ESLevent_setPriority(HandleRef jarg1, HandleRef jarg2);

  [DllImport("ESL", EntryPoint="CSharp_ESLevent_getHeader")]
  public static extern string ESLevent_getHeader(HandleRef jarg1, string jarg2);

  [DllImport("ESL", EntryPoint="CSharp_ESLevent_getBody")]
  public static extern string ESLevent_getBody(HandleRef jarg1);

  [DllImport("ESL", EntryPoint="CSharp_ESLevent_getType")]
  public static extern string ESLevent_getType(HandleRef jarg1);

  [DllImport("ESL", EntryPoint="CSharp_ESLevent_addBody")]
  public static extern bool ESLevent_addBody(HandleRef jarg1, string jarg2);

  [DllImport("ESL", EntryPoint="CSharp_ESLevent_addHeader")]
  public static extern bool ESLevent_addHeader(HandleRef jarg1, string jarg2, string jarg3);

  [DllImport("ESL", EntryPoint="CSharp_ESLevent_delHeader")]
  public static extern bool ESLevent_delHeader(HandleRef jarg1, string jarg2);

  [DllImport("ESL", EntryPoint="CSharp_ESLevent_firstHeader")]
  public static extern string ESLevent_firstHeader(HandleRef jarg1);

  [DllImport("ESL", EntryPoint="CSharp_ESLevent_nextHeader")]
  public static extern string ESLevent_nextHeader(HandleRef jarg1);

  [DllImport("ESL", EntryPoint="CSharp_new_ESLconnection__SWIG_0")]
  public static extern IntPtr new_ESLconnection__SWIG_0(string jarg1, string jarg2, string jarg3);

  [DllImport("ESL", EntryPoint="CSharp_new_ESLconnection__SWIG_1")]
  public static extern IntPtr new_ESLconnection__SWIG_1(int jarg1);

  [DllImport("ESL", EntryPoint="CSharp_delete_ESLconnection")]
  public static extern void delete_ESLconnection(HandleRef jarg1);

  [DllImport("ESL", EntryPoint="CSharp_ESLconnection_socketDescriptor")]
  public static extern int ESLconnection_socketDescriptor(HandleRef jarg1);

  [DllImport("ESL", EntryPoint="CSharp_ESLconnection_connected")]
  public static extern int ESLconnection_connected(HandleRef jarg1);

  [DllImport("ESL", EntryPoint="CSharp_ESLconnection_getInfo")]
  public static extern IntPtr ESLconnection_getInfo(HandleRef jarg1);

  [DllImport("ESL", EntryPoint="CSharp_ESLconnection_send")]
  public static extern int ESLconnection_send(HandleRef jarg1, string jarg2);

  [DllImport("ESL", EntryPoint="CSharp_ESLconnection_sendRecv")]
  public static extern IntPtr ESLconnection_sendRecv(HandleRef jarg1, string jarg2);

  [DllImport("ESL", EntryPoint="CSharp_ESLconnection_api")]
  public static extern IntPtr ESLconnection_api(HandleRef jarg1, string jarg2, string jarg3);

  [DllImport("ESL", EntryPoint="CSharp_ESLconnection_bgapi")]
  public static extern IntPtr ESLconnection_bgapi(HandleRef jarg1, string jarg2, string jarg3);

  [DllImport("ESL", EntryPoint="CSharp_ESLconnection_sendEvent")]
  public static extern int ESLconnection_sendEvent(HandleRef jarg1, HandleRef jarg2);

  [DllImport("ESL", EntryPoint="CSharp_ESLconnection_recvEvent")]
  public static extern IntPtr ESLconnection_recvEvent(HandleRef jarg1);

  [DllImport("ESL", EntryPoint="CSharp_ESLconnection_recvEventTimed")]
  public static extern IntPtr ESLconnection_recvEventTimed(HandleRef jarg1, int jarg2);

  [DllImport("ESL", EntryPoint="CSharp_ESLconnection_filter")]
  public static extern IntPtr ESLconnection_filter(HandleRef jarg1, string jarg2, string jarg3);

  [DllImport("ESL", EntryPoint="CSharp_ESLconnection_events")]
  public static extern int ESLconnection_events(HandleRef jarg1, string jarg2, string jarg3);

  [DllImport("ESL", EntryPoint="CSharp_ESLconnection_execute")]
  public static extern IntPtr ESLconnection_execute(HandleRef jarg1, string jarg2, string jarg3, string jarg4);

  [DllImport("ESL", EntryPoint="CSharp_ESLconnection_executeAsync")]
  public static extern IntPtr ESLconnection_executeAsync(HandleRef jarg1, string jarg2, string jarg3, string jarg4);

  [DllImport("ESL", EntryPoint="CSharp_ESLconnection_setAsyncExecute")]
  public static extern int ESLconnection_setAsyncExecute(HandleRef jarg1, string jarg2);

  [DllImport("ESL", EntryPoint="CSharp_ESLconnection_setEventLock")]
  public static extern int ESLconnection_setEventLock(HandleRef jarg1, string jarg2);

  [DllImport("ESL", EntryPoint="CSharp_ESLconnection_disconnect")]
  public static extern int ESLconnection_disconnect(HandleRef jarg1);

  [DllImport("ESL", EntryPoint="CSharp_eslSetLogLevel")]
  public static extern void eslSetLogLevel(int jarg1);
}

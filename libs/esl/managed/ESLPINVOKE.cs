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


  [DllImport("ESL", EntryPoint="CSharp_ESLevent_Event_set")]
  public static extern void ESLevent_Event_set(HandleRef jarg1, HandleRef jarg2);

  [DllImport("ESL", EntryPoint="CSharp_ESLevent_Event_get")]
  public static extern IntPtr ESLevent_Event_get(HandleRef jarg1);

  [DllImport("ESL", EntryPoint="CSharp_ESLevent_SerializedString_set")]
  public static extern void ESLevent_SerializedString_set(HandleRef jarg1, string jarg2);

  [DllImport("ESL", EntryPoint="CSharp_ESLevent_SerializedString_get")]
  public static extern string ESLevent_SerializedString_get(HandleRef jarg1);

  [DllImport("ESL", EntryPoint="CSharp_ESLevent_Mine_set")]
  public static extern void ESLevent_Mine_set(HandleRef jarg1, int jarg2);

  [DllImport("ESL", EntryPoint="CSharp_ESLevent_Mine_get")]
  public static extern int ESLevent_Mine_get(HandleRef jarg1);

  [DllImport("ESL", EntryPoint="CSharp_new_ESLevent__SWIG_0")]
  public static extern IntPtr new_ESLevent__SWIG_0(string jarg1, string jarg2);

  [DllImport("ESL", EntryPoint="CSharp_new_ESLevent__SWIG_1")]
  public static extern IntPtr new_ESLevent__SWIG_1(HandleRef jarg1, int jarg2);

  [DllImport("ESL", EntryPoint="CSharp_new_ESLevent__SWIG_2")]
  public static extern IntPtr new_ESLevent__SWIG_2(HandleRef jarg1);

  [DllImport("ESL", EntryPoint="CSharp_delete_ESLevent")]
  public static extern void delete_ESLevent(HandleRef jarg1);

  [DllImport("ESL", EntryPoint="CSharp_ESLevent_Serialize")]
  public static extern string ESLevent_Serialize(HandleRef jarg1, string jarg2);

  [DllImport("ESL", EntryPoint="CSharp_ESLevent_SetPriority")]
  public static extern bool ESLevent_SetPriority(HandleRef jarg1, HandleRef jarg2);

  [DllImport("ESL", EntryPoint="CSharp_ESLevent_GetHeader")]
  public static extern string ESLevent_GetHeader(HandleRef jarg1, string jarg2);

  [DllImport("ESL", EntryPoint="CSharp_ESLevent_GetBody")]
  public static extern string ESLevent_GetBody(HandleRef jarg1);

  [DllImport("ESL", EntryPoint="CSharp_ESLevent_getType")]
  public static extern string ESLevent_getType(HandleRef jarg1);

  [DllImport("ESL", EntryPoint="CSharp_ESLevent_AddBody")]
  public static extern bool ESLevent_AddBody(HandleRef jarg1, string jarg2);

  [DllImport("ESL", EntryPoint="CSharp_ESLevent_AddHeader")]
  public static extern bool ESLevent_AddHeader(HandleRef jarg1, string jarg2, string jarg3);

  [DllImport("ESL", EntryPoint="CSharp_ESLevent_DelHeader")]
  public static extern bool ESLevent_DelHeader(HandleRef jarg1, string jarg2);

  [DllImport("ESL", EntryPoint="CSharp_ESLevent_FirstHeader")]
  public static extern string ESLevent_FirstHeader(HandleRef jarg1);

  [DllImport("ESL", EntryPoint="CSharp_ESLevent_NextHeader")]
  public static extern string ESLevent_NextHeader(HandleRef jarg1);

  [DllImport("ESL", EntryPoint="CSharp_new_ESLconnection__SWIG_0")]
  public static extern IntPtr new_ESLconnection__SWIG_0(string jarg1, string jarg2, string jarg3, string jarg4);

  [DllImport("ESL", EntryPoint="CSharp_new_ESLconnection__SWIG_1")]
  public static extern IntPtr new_ESLconnection__SWIG_1(string jarg1, string jarg2, string jarg3);

  [DllImport("ESL", EntryPoint="CSharp_new_ESLconnection__SWIG_2")]
  public static extern IntPtr new_ESLconnection__SWIG_2(int jarg1);

  [DllImport("ESL", EntryPoint="CSharp_delete_ESLconnection")]
  public static extern void delete_ESLconnection(HandleRef jarg1);

  [DllImport("ESL", EntryPoint="CSharp_ESLconnection_SocketDescriptor")]
  public static extern int ESLconnection_SocketDescriptor(HandleRef jarg1);

  [DllImport("ESL", EntryPoint="CSharp_ESLconnection_Connected")]
  public static extern int ESLconnection_Connected(HandleRef jarg1);

  [DllImport("ESL", EntryPoint="CSharp_ESLconnection_GetInfo")]
  public static extern IntPtr ESLconnection_GetInfo(HandleRef jarg1);

  [DllImport("ESL", EntryPoint="CSharp_ESLconnection_Send")]
  public static extern int ESLconnection_Send(HandleRef jarg1, string jarg2);

  [DllImport("ESL", EntryPoint="CSharp_ESLconnection_SendRecv")]
  public static extern IntPtr ESLconnection_SendRecv(HandleRef jarg1, string jarg2);

  [DllImport("ESL", EntryPoint="CSharp_ESLconnection_Api")]
  public static extern IntPtr ESLconnection_Api(HandleRef jarg1, string jarg2, string jarg3);

  [DllImport("ESL", EntryPoint="CSharp_ESLconnection_Bgapi")]
  public static extern IntPtr ESLconnection_Bgapi(HandleRef jarg1, string jarg2, string jarg3, string jarg4);

  [DllImport("ESL", EntryPoint="CSharp_ESLconnection_SendEvent")]
  public static extern IntPtr ESLconnection_SendEvent(HandleRef jarg1, HandleRef jarg2);

  [DllImport("ESL", EntryPoint="CSharp_ESLconnection_RecvEvent")]
  public static extern IntPtr ESLconnection_RecvEvent(HandleRef jarg1);

  [DllImport("ESL", EntryPoint="CSharp_ESLconnection_RecvEventTimed")]
  public static extern IntPtr ESLconnection_RecvEventTimed(HandleRef jarg1, int jarg2);

  [DllImport("ESL", EntryPoint="CSharp_ESLconnection_Filter")]
  public static extern IntPtr ESLconnection_Filter(HandleRef jarg1, string jarg2, string jarg3);

  [DllImport("ESL", EntryPoint="CSharp_ESLconnection_Events")]
  public static extern int ESLconnection_Events(HandleRef jarg1, string jarg2, string jarg3);

  [DllImport("ESL", EntryPoint="CSharp_ESLconnection_Execute")]
  public static extern IntPtr ESLconnection_Execute(HandleRef jarg1, string jarg2, string jarg3, string jarg4);

  [DllImport("ESL", EntryPoint="CSharp_ESLconnection_ExecuteAsync")]
  public static extern IntPtr ESLconnection_ExecuteAsync(HandleRef jarg1, string jarg2, string jarg3, string jarg4);

  [DllImport("ESL", EntryPoint="CSharp_ESLconnection_SetAsyncExecute")]
  public static extern int ESLconnection_SetAsyncExecute(HandleRef jarg1, string jarg2);

  [DllImport("ESL", EntryPoint="CSharp_ESLconnection_SetEventLock")]
  public static extern int ESLconnection_SetEventLock(HandleRef jarg1, string jarg2);

  [DllImport("ESL", EntryPoint="CSharp_ESLconnection_Disconnect")]
  public static extern int ESLconnection_Disconnect(HandleRef jarg1);

  [DllImport("ESL", EntryPoint="CSharp_eslSetLogLevel")]
  public static extern void eslSetLogLevel(int jarg1);
}

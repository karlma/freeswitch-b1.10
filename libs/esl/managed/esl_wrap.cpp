/* ----------------------------------------------------------------------------
 * This file was automatically generated by SWIG (http://www.swig.org).
 * Version 1.3.40
 * 
 * This file is not intended to be easily readable and contains a number of 
 * coding conventions designed to improve portability and efficiency. Do not make
 * changes to this file unless you know what you are doing--modify the SWIG 
 * interface file instead. 
 * ----------------------------------------------------------------------------- */

#define SWIGCSHARP


#ifdef __cplusplus
/* SwigValueWrapper is described in swig.swg */
template<typename T> class SwigValueWrapper {
  struct SwigMovePointer {
    T *ptr;
    SwigMovePointer(T *p) : ptr(p) { }
    ~SwigMovePointer() { delete ptr; }
    SwigMovePointer& operator=(SwigMovePointer& rhs) { T* oldptr = ptr; ptr = 0; delete oldptr; ptr = rhs.ptr; rhs.ptr = 0; return *this; }
  } pointer;
  SwigValueWrapper& operator=(const SwigValueWrapper<T>& rhs);
  SwigValueWrapper(const SwigValueWrapper<T>& rhs);
public:
  SwigValueWrapper() : pointer(0) { }
  SwigValueWrapper& operator=(const T& t) { SwigMovePointer tmp(new T(t)); pointer = tmp; return *this; }
  operator T&() const { return *pointer.ptr; }
  T *operator&() { return pointer.ptr; }
};

template <typename T> T SwigValueInit() {
  return T();
}
#endif

/* -----------------------------------------------------------------------------
 *  This section contains generic SWIG labels for method/variable
 *  declarations/attributes, and other compiler dependent labels.
 * ----------------------------------------------------------------------------- */

/* template workaround for compilers that cannot correctly implement the C++ standard */
#ifndef SWIGTEMPLATEDISAMBIGUATOR
# if defined(__SUNPRO_CC) && (__SUNPRO_CC <= 0x560)
#  define SWIGTEMPLATEDISAMBIGUATOR template
# elif defined(__HP_aCC)
/* Needed even with `aCC -AA' when `aCC -V' reports HP ANSI C++ B3910B A.03.55 */
/* If we find a maximum version that requires this, the test would be __HP_aCC <= 35500 for A.03.55 */
#  define SWIGTEMPLATEDISAMBIGUATOR template
# else
#  define SWIGTEMPLATEDISAMBIGUATOR
# endif
#endif

/* inline attribute */
#ifndef SWIGINLINE
# if defined(__cplusplus) || (defined(__GNUC__) && !defined(__STRICT_ANSI__))
#   define SWIGINLINE inline
# else
#   define SWIGINLINE
# endif
#endif

/* attribute recognised by some compilers to avoid 'unused' warnings */
#ifndef SWIGUNUSED
# if defined(__GNUC__)
#   if !(defined(__cplusplus)) || (__GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4))
#     define SWIGUNUSED __attribute__ ((__unused__)) 
#   else
#     define SWIGUNUSED
#   endif
# elif defined(__ICC)
#   define SWIGUNUSED __attribute__ ((__unused__)) 
# else
#   define SWIGUNUSED 
# endif
#endif

#ifndef SWIG_MSC_UNSUPPRESS_4505
# if defined(_MSC_VER)
#   pragma warning(disable : 4505) /* unreferenced local function has been removed */
# endif 
#endif

#ifndef SWIGUNUSEDPARM
# ifdef __cplusplus
#   define SWIGUNUSEDPARM(p)
# else
#   define SWIGUNUSEDPARM(p) p SWIGUNUSED 
# endif
#endif

/* internal SWIG method */
#ifndef SWIGINTERN
# define SWIGINTERN static SWIGUNUSED
#endif

/* internal inline SWIG method */
#ifndef SWIGINTERNINLINE
# define SWIGINTERNINLINE SWIGINTERN SWIGINLINE
#endif

/* exporting methods */
#if (__GNUC__ >= 4) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4)
#  ifndef GCC_HASCLASSVISIBILITY
#    define GCC_HASCLASSVISIBILITY
#  endif
#endif

#ifndef SWIGEXPORT
# if defined(_WIN32) || defined(__WIN32__) || defined(__CYGWIN__)
#   if defined(STATIC_LINKED)
#     define SWIGEXPORT
#   else
#     define SWIGEXPORT __declspec(dllexport)
#   endif
# else
#   if defined(__GNUC__) && defined(GCC_HASCLASSVISIBILITY)
#     define SWIGEXPORT __attribute__ ((visibility("default")))
#   else
#     define SWIGEXPORT
#   endif
# endif
#endif

/* calling conventions for Windows */
#ifndef SWIGSTDCALL
# if defined(_WIN32) || defined(__WIN32__) || defined(__CYGWIN__)
#   define SWIGSTDCALL __stdcall
# else
#   define SWIGSTDCALL
# endif 
#endif

/* Deal with Microsoft's attempt at deprecating C standard runtime functions */
#if !defined(SWIG_NO_CRT_SECURE_NO_DEPRECATE) && defined(_MSC_VER) && !defined(_CRT_SECURE_NO_DEPRECATE)
# define _CRT_SECURE_NO_DEPRECATE
#endif

/* Deal with Microsoft's attempt at deprecating methods in the standard C++ library */
#if !defined(SWIG_NO_SCL_SECURE_NO_DEPRECATE) && defined(_MSC_VER) && !defined(_SCL_SECURE_NO_DEPRECATE)
# define _SCL_SECURE_NO_DEPRECATE
#endif



#include <stdlib.h>
#include <string.h>
#include <stdio.h>


/* Support for throwing C# exceptions from C/C++. There are two types: 
 * Exceptions that take a message and ArgumentExceptions that take a message and a parameter name. */
typedef enum {
  SWIG_CSharpApplicationException,
  SWIG_CSharpArithmeticException,
  SWIG_CSharpDivideByZeroException,
  SWIG_CSharpIndexOutOfRangeException,
  SWIG_CSharpInvalidCastException,
  SWIG_CSharpInvalidOperationException,
  SWIG_CSharpIOException,
  SWIG_CSharpNullReferenceException,
  SWIG_CSharpOutOfMemoryException,
  SWIG_CSharpOverflowException,
  SWIG_CSharpSystemException
} SWIG_CSharpExceptionCodes;

typedef enum {
  SWIG_CSharpArgumentException,
  SWIG_CSharpArgumentNullException,
  SWIG_CSharpArgumentOutOfRangeException
} SWIG_CSharpExceptionArgumentCodes;

typedef void (SWIGSTDCALL* SWIG_CSharpExceptionCallback_t)(const char *);
typedef void (SWIGSTDCALL* SWIG_CSharpExceptionArgumentCallback_t)(const char *, const char *);

typedef struct {
  SWIG_CSharpExceptionCodes code;
  SWIG_CSharpExceptionCallback_t callback;
} SWIG_CSharpException_t;

typedef struct {
  SWIG_CSharpExceptionArgumentCodes code;
  SWIG_CSharpExceptionArgumentCallback_t callback;
} SWIG_CSharpExceptionArgument_t;

static SWIG_CSharpException_t SWIG_csharp_exceptions[] = {
  { SWIG_CSharpApplicationException, NULL },
  { SWIG_CSharpArithmeticException, NULL },
  { SWIG_CSharpDivideByZeroException, NULL },
  { SWIG_CSharpIndexOutOfRangeException, NULL },
  { SWIG_CSharpInvalidCastException, NULL },
  { SWIG_CSharpInvalidOperationException, NULL },
  { SWIG_CSharpIOException, NULL },
  { SWIG_CSharpNullReferenceException, NULL },
  { SWIG_CSharpOutOfMemoryException, NULL },
  { SWIG_CSharpOverflowException, NULL },
  { SWIG_CSharpSystemException, NULL }
};

static SWIG_CSharpExceptionArgument_t SWIG_csharp_exceptions_argument[] = {
  { SWIG_CSharpArgumentException, NULL },
  { SWIG_CSharpArgumentNullException, NULL },
  { SWIG_CSharpArgumentOutOfRangeException, NULL }
};

static void SWIGUNUSED SWIG_CSharpSetPendingException(SWIG_CSharpExceptionCodes code, const char *msg) {
  SWIG_CSharpExceptionCallback_t callback = SWIG_csharp_exceptions[SWIG_CSharpApplicationException].callback;
  if ((size_t)code < sizeof(SWIG_csharp_exceptions)/sizeof(SWIG_CSharpException_t)) {
    callback = SWIG_csharp_exceptions[code].callback;
  }
  callback(msg);
}

static void SWIGUNUSED SWIG_CSharpSetPendingExceptionArgument(SWIG_CSharpExceptionArgumentCodes code, const char *msg, const char *param_name) {
  SWIG_CSharpExceptionArgumentCallback_t callback = SWIG_csharp_exceptions_argument[SWIG_CSharpArgumentException].callback;
  if ((size_t)code < sizeof(SWIG_csharp_exceptions_argument)/sizeof(SWIG_CSharpExceptionArgument_t)) {
    callback = SWIG_csharp_exceptions_argument[code].callback;
  }
  callback(msg, param_name);
}


#ifdef __cplusplus
extern "C" 
#endif
SWIGEXPORT void SWIGSTDCALL SWIGRegisterExceptionCallbacks_ESL(
                                                SWIG_CSharpExceptionCallback_t applicationCallback,
                                                SWIG_CSharpExceptionCallback_t arithmeticCallback,
                                                SWIG_CSharpExceptionCallback_t divideByZeroCallback, 
                                                SWIG_CSharpExceptionCallback_t indexOutOfRangeCallback, 
                                                SWIG_CSharpExceptionCallback_t invalidCastCallback,
                                                SWIG_CSharpExceptionCallback_t invalidOperationCallback,
                                                SWIG_CSharpExceptionCallback_t ioCallback,
                                                SWIG_CSharpExceptionCallback_t nullReferenceCallback,
                                                SWIG_CSharpExceptionCallback_t outOfMemoryCallback, 
                                                SWIG_CSharpExceptionCallback_t overflowCallback, 
                                                SWIG_CSharpExceptionCallback_t systemCallback) {
  SWIG_csharp_exceptions[SWIG_CSharpApplicationException].callback = applicationCallback;
  SWIG_csharp_exceptions[SWIG_CSharpArithmeticException].callback = arithmeticCallback;
  SWIG_csharp_exceptions[SWIG_CSharpDivideByZeroException].callback = divideByZeroCallback;
  SWIG_csharp_exceptions[SWIG_CSharpIndexOutOfRangeException].callback = indexOutOfRangeCallback;
  SWIG_csharp_exceptions[SWIG_CSharpInvalidCastException].callback = invalidCastCallback;
  SWIG_csharp_exceptions[SWIG_CSharpInvalidOperationException].callback = invalidOperationCallback;
  SWIG_csharp_exceptions[SWIG_CSharpIOException].callback = ioCallback;
  SWIG_csharp_exceptions[SWIG_CSharpNullReferenceException].callback = nullReferenceCallback;
  SWIG_csharp_exceptions[SWIG_CSharpOutOfMemoryException].callback = outOfMemoryCallback;
  SWIG_csharp_exceptions[SWIG_CSharpOverflowException].callback = overflowCallback;
  SWIG_csharp_exceptions[SWIG_CSharpSystemException].callback = systemCallback;
}

#ifdef __cplusplus
extern "C" 
#endif
SWIGEXPORT void SWIGSTDCALL SWIGRegisterExceptionArgumentCallbacks_ESL(
                                                SWIG_CSharpExceptionArgumentCallback_t argumentCallback,
                                                SWIG_CSharpExceptionArgumentCallback_t argumentNullCallback,
                                                SWIG_CSharpExceptionArgumentCallback_t argumentOutOfRangeCallback) {
  SWIG_csharp_exceptions_argument[SWIG_CSharpArgumentException].callback = argumentCallback;
  SWIG_csharp_exceptions_argument[SWIG_CSharpArgumentNullException].callback = argumentNullCallback;
  SWIG_csharp_exceptions_argument[SWIG_CSharpArgumentOutOfRangeException].callback = argumentOutOfRangeCallback;
}


/* Callback for returning strings to C# without leaking memory */
typedef char * (SWIGSTDCALL* SWIG_CSharpStringHelperCallback)(const char *);
static SWIG_CSharpStringHelperCallback SWIG_csharp_string_callback = NULL;


#ifdef __cplusplus
extern "C" 
#endif
SWIGEXPORT void SWIGSTDCALL SWIGRegisterStringCallback_ESL(SWIG_CSharpStringHelperCallback callback) {
  SWIG_csharp_string_callback = callback;
}


/* Contract support */

#define SWIG_contract_assert(nullreturn, expr, msg) if (!(expr)) {SWIG_CSharpSetPendingExceptionArgument(SWIG_CSharpArgumentOutOfRangeException, msg, ""); return nullreturn; } else


#include "esl.h"
#include "esl_oop.h"


#ifdef __cplusplus
extern "C" {
#endif

SWIGEXPORT void SWIGSTDCALL CSharp_ESLevent_Event_set(void * jarg1, void * jarg2) {
  ESLevent *arg1 = (ESLevent *) 0 ;
  esl_event_t *arg2 = (esl_event_t *) 0 ;
  
  arg1 = (ESLevent *)jarg1; 
  arg2 = (esl_event_t *)jarg2; 
  if (arg1) (arg1)->event = arg2;
}


SWIGEXPORT void * SWIGSTDCALL CSharp_ESLevent_Event_get(void * jarg1) {
  void * jresult ;
  ESLevent *arg1 = (ESLevent *) 0 ;
  esl_event_t *result = 0 ;
  
  arg1 = (ESLevent *)jarg1; 
  result = (esl_event_t *) ((arg1)->event);
  jresult = (void *)result; 
  return jresult;
}


SWIGEXPORT void SWIGSTDCALL CSharp_ESLevent_SerializedString_set(void * jarg1, char * jarg2) {
  ESLevent *arg1 = (ESLevent *) 0 ;
  char *arg2 = (char *) 0 ;
  
  arg1 = (ESLevent *)jarg1; 
  arg2 = (char *)jarg2; 
  {
    if (arg1->serialized_string) delete [] arg1->serialized_string;
    if (arg2) {
      arg1->serialized_string = (char *) (new char[strlen((const char *)arg2)+1]);
      strcpy((char *)arg1->serialized_string, (const char *)arg2);
    } else {
      arg1->serialized_string = 0;
    }
  }
}


SWIGEXPORT char * SWIGSTDCALL CSharp_ESLevent_SerializedString_get(void * jarg1) {
  char * jresult ;
  ESLevent *arg1 = (ESLevent *) 0 ;
  char *result = 0 ;
  
  arg1 = (ESLevent *)jarg1; 
  result = (char *) ((arg1)->serialized_string);
  jresult = SWIG_csharp_string_callback((const char *)result); 
  return jresult;
}


SWIGEXPORT void SWIGSTDCALL CSharp_ESLevent_Mine_set(void * jarg1, int jarg2) {
  ESLevent *arg1 = (ESLevent *) 0 ;
  int arg2 ;
  
  arg1 = (ESLevent *)jarg1; 
  arg2 = (int)jarg2; 
  if (arg1) (arg1)->mine = arg2;
}


SWIGEXPORT int SWIGSTDCALL CSharp_ESLevent_Mine_get(void * jarg1) {
  int jresult ;
  ESLevent *arg1 = (ESLevent *) 0 ;
  int result;
  
  arg1 = (ESLevent *)jarg1; 
  result = (int) ((arg1)->mine);
  jresult = result; 
  return jresult;
}


SWIGEXPORT void * SWIGSTDCALL CSharp_new_ESLevent__SWIG_0(char * jarg1, char * jarg2) {
  void * jresult ;
  char *arg1 = (char *) 0 ;
  char *arg2 = (char *) NULL ;
  ESLevent *result = 0 ;
  
  arg1 = (char *)jarg1; 
  arg2 = (char *)jarg2; 
  result = (ESLevent *)new ESLevent((char const *)arg1,(char const *)arg2);
  jresult = (void *)result; 
  return jresult;
}


SWIGEXPORT void * SWIGSTDCALL CSharp_new_ESLevent__SWIG_1(void * jarg1, int jarg2) {
  void * jresult ;
  esl_event_t *arg1 = (esl_event_t *) 0 ;
  int arg2 = (int) 0 ;
  ESLevent *result = 0 ;
  
  arg1 = (esl_event_t *)jarg1; 
  arg2 = (int)jarg2; 
  result = (ESLevent *)new ESLevent(arg1,arg2);
  jresult = (void *)result; 
  return jresult;
}


SWIGEXPORT void * SWIGSTDCALL CSharp_new_ESLevent__SWIG_2(void * jarg1) {
  void * jresult ;
  ESLevent *arg1 = (ESLevent *) 0 ;
  ESLevent *result = 0 ;
  
  arg1 = (ESLevent *)jarg1; 
  result = (ESLevent *)new ESLevent(arg1);
  jresult = (void *)result; 
  return jresult;
}


SWIGEXPORT void SWIGSTDCALL CSharp_delete_ESLevent(void * jarg1) {
  ESLevent *arg1 = (ESLevent *) 0 ;
  
  arg1 = (ESLevent *)jarg1; 
  delete arg1;
}


SWIGEXPORT char * SWIGSTDCALL CSharp_ESLevent_Serialize(void * jarg1, char * jarg2) {
  char * jresult ;
  ESLevent *arg1 = (ESLevent *) 0 ;
  char *arg2 = (char *) NULL ;
  char *result = 0 ;
  
  arg1 = (ESLevent *)jarg1; 
  arg2 = (char *)jarg2; 
  result = (char *)(arg1)->serialize((char const *)arg2);
  jresult = SWIG_csharp_string_callback((const char *)result); 
  return jresult;
}


SWIGEXPORT unsigned int SWIGSTDCALL CSharp_ESLevent_SetPriority(void * jarg1, void * jarg2) {
  unsigned int jresult ;
  ESLevent *arg1 = (ESLevent *) 0 ;
  esl_priority_t arg2 = (esl_priority_t) ESL_PRIORITY_NORMAL ;
  esl_priority_t *argp2 ;
  bool result;
  
  arg1 = (ESLevent *)jarg1; 
  argp2 = (esl_priority_t *)jarg2; 
  if (!argp2) {
    SWIG_CSharpSetPendingExceptionArgument(SWIG_CSharpArgumentNullException, "Attempt to dereference null esl_priority_t", 0);
    return 0;
  }
  arg2 = *argp2; 
  result = (bool)(arg1)->setPriority(arg2);
  jresult = result; 
  return jresult;
}


SWIGEXPORT char * SWIGSTDCALL CSharp_ESLevent_GetHeader(void * jarg1, char * jarg2) {
  char * jresult ;
  ESLevent *arg1 = (ESLevent *) 0 ;
  char *arg2 = (char *) 0 ;
  char *result = 0 ;
  
  arg1 = (ESLevent *)jarg1; 
  arg2 = (char *)jarg2; 
  result = (char *)(arg1)->getHeader((char const *)arg2);
  jresult = SWIG_csharp_string_callback((const char *)result); 
  return jresult;
}


SWIGEXPORT char * SWIGSTDCALL CSharp_ESLevent_GetBody(void * jarg1) {
  char * jresult ;
  ESLevent *arg1 = (ESLevent *) 0 ;
  char *result = 0 ;
  
  arg1 = (ESLevent *)jarg1; 
  result = (char *)(arg1)->getBody();
  jresult = SWIG_csharp_string_callback((const char *)result); 
  return jresult;
}


SWIGEXPORT char * SWIGSTDCALL CSharp_ESLevent_getType(void * jarg1) {
  char * jresult ;
  ESLevent *arg1 = (ESLevent *) 0 ;
  char *result = 0 ;
  
  arg1 = (ESLevent *)jarg1; 
  result = (char *)(arg1)->getType();
  jresult = SWIG_csharp_string_callback((const char *)result); 
  return jresult;
}


SWIGEXPORT unsigned int SWIGSTDCALL CSharp_ESLevent_AddBody(void * jarg1, char * jarg2) {
  unsigned int jresult ;
  ESLevent *arg1 = (ESLevent *) 0 ;
  char *arg2 = (char *) 0 ;
  bool result;
  
  arg1 = (ESLevent *)jarg1; 
  arg2 = (char *)jarg2; 
  result = (bool)(arg1)->addBody((char const *)arg2);
  jresult = result; 
  return jresult;
}


SWIGEXPORT unsigned int SWIGSTDCALL CSharp_ESLevent_AddHeader(void * jarg1, char * jarg2, char * jarg3) {
  unsigned int jresult ;
  ESLevent *arg1 = (ESLevent *) 0 ;
  char *arg2 = (char *) 0 ;
  char *arg3 = (char *) 0 ;
  bool result;
  
  arg1 = (ESLevent *)jarg1; 
  arg2 = (char *)jarg2; 
  arg3 = (char *)jarg3; 
  result = (bool)(arg1)->addHeader((char const *)arg2,(char const *)arg3);
  jresult = result; 
  return jresult;
}


SWIGEXPORT unsigned int SWIGSTDCALL CSharp_ESLevent_DelHeader(void * jarg1, char * jarg2) {
  unsigned int jresult ;
  ESLevent *arg1 = (ESLevent *) 0 ;
  char *arg2 = (char *) 0 ;
  bool result;
  
  arg1 = (ESLevent *)jarg1; 
  arg2 = (char *)jarg2; 
  result = (bool)(arg1)->delHeader((char const *)arg2);
  jresult = result; 
  return jresult;
}


SWIGEXPORT char * SWIGSTDCALL CSharp_ESLevent_FirstHeader(void * jarg1) {
  char * jresult ;
  ESLevent *arg1 = (ESLevent *) 0 ;
  char *result = 0 ;
  
  arg1 = (ESLevent *)jarg1; 
  result = (char *)(arg1)->firstHeader();
  jresult = SWIG_csharp_string_callback((const char *)result); 
  return jresult;
}


SWIGEXPORT char * SWIGSTDCALL CSharp_ESLevent_NextHeader(void * jarg1) {
  char * jresult ;
  ESLevent *arg1 = (ESLevent *) 0 ;
  char *result = 0 ;
  
  arg1 = (ESLevent *)jarg1; 
  result = (char *)(arg1)->nextHeader();
  jresult = SWIG_csharp_string_callback((const char *)result); 
  return jresult;
}


SWIGEXPORT void * SWIGSTDCALL CSharp_new_ESLconnection__SWIG_0(char * jarg1, char * jarg2, char * jarg3, char * jarg4) {
  void * jresult ;
  char *arg1 = (char *) 0 ;
  char *arg2 = (char *) 0 ;
  char *arg3 = (char *) 0 ;
  char *arg4 = (char *) 0 ;
  ESLconnection *result = 0 ;
  
  arg1 = (char *)jarg1; 
  arg2 = (char *)jarg2; 
  arg3 = (char *)jarg3; 
  arg4 = (char *)jarg4; 
  result = (ESLconnection *)new ESLconnection((char const *)arg1,(char const *)arg2,(char const *)arg3,(char const *)arg4);
  jresult = (void *)result; 
  return jresult;
}


SWIGEXPORT void * SWIGSTDCALL CSharp_new_ESLconnection__SWIG_1(char * jarg1, char * jarg2, char * jarg3) {
  void * jresult ;
  char *arg1 = (char *) 0 ;
  char *arg2 = (char *) 0 ;
  char *arg3 = (char *) 0 ;
  ESLconnection *result = 0 ;
  
  arg1 = (char *)jarg1; 
  arg2 = (char *)jarg2; 
  arg3 = (char *)jarg3; 
  result = (ESLconnection *)new ESLconnection((char const *)arg1,(char const *)arg2,(char const *)arg3);
  jresult = (void *)result; 
  return jresult;
}


SWIGEXPORT void * SWIGSTDCALL CSharp_new_ESLconnection__SWIG_2(int jarg1) {
  void * jresult ;
  int arg1 ;
  ESLconnection *result = 0 ;
  
  arg1 = (int)jarg1; 
  result = (ESLconnection *)new ESLconnection(arg1);
  jresult = (void *)result; 
  return jresult;
}


SWIGEXPORT void SWIGSTDCALL CSharp_delete_ESLconnection(void * jarg1) {
  ESLconnection *arg1 = (ESLconnection *) 0 ;
  
  arg1 = (ESLconnection *)jarg1; 
  delete arg1;
}


SWIGEXPORT int SWIGSTDCALL CSharp_ESLconnection_SocketDescriptor(void * jarg1) {
  int jresult ;
  ESLconnection *arg1 = (ESLconnection *) 0 ;
  int result;
  
  arg1 = (ESLconnection *)jarg1; 
  result = (int)(arg1)->socketDescriptor();
  jresult = result; 
  return jresult;
}


SWIGEXPORT int SWIGSTDCALL CSharp_ESLconnection_Connected(void * jarg1) {
  int jresult ;
  ESLconnection *arg1 = (ESLconnection *) 0 ;
  int result;
  
  arg1 = (ESLconnection *)jarg1; 
  result = (int)(arg1)->connected();
  jresult = result; 
  return jresult;
}


SWIGEXPORT void * SWIGSTDCALL CSharp_ESLconnection_GetInfo(void * jarg1) {
  void * jresult ;
  ESLconnection *arg1 = (ESLconnection *) 0 ;
  ESLevent *result = 0 ;
  
  arg1 = (ESLconnection *)jarg1; 
  result = (ESLevent *)(arg1)->getInfo();
  jresult = (void *)result; 
  return jresult;
}


SWIGEXPORT int SWIGSTDCALL CSharp_ESLconnection_Send(void * jarg1, char * jarg2) {
  int jresult ;
  ESLconnection *arg1 = (ESLconnection *) 0 ;
  char *arg2 = (char *) 0 ;
  int result;
  
  arg1 = (ESLconnection *)jarg1; 
  arg2 = (char *)jarg2; 
  result = (int)(arg1)->send((char const *)arg2);
  jresult = result; 
  return jresult;
}


SWIGEXPORT void * SWIGSTDCALL CSharp_ESLconnection_SendRecv(void * jarg1, char * jarg2) {
  void * jresult ;
  ESLconnection *arg1 = (ESLconnection *) 0 ;
  char *arg2 = (char *) 0 ;
  ESLevent *result = 0 ;
  
  arg1 = (ESLconnection *)jarg1; 
  arg2 = (char *)jarg2; 
  result = (ESLevent *)(arg1)->sendRecv((char const *)arg2);
  jresult = (void *)result; 
  return jresult;
}


SWIGEXPORT void * SWIGSTDCALL CSharp_ESLconnection_Api(void * jarg1, char * jarg2, char * jarg3) {
  void * jresult ;
  ESLconnection *arg1 = (ESLconnection *) 0 ;
  char *arg2 = (char *) 0 ;
  char *arg3 = (char *) NULL ;
  ESLevent *result = 0 ;
  
  arg1 = (ESLconnection *)jarg1; 
  arg2 = (char *)jarg2; 
  arg3 = (char *)jarg3; 
  result = (ESLevent *)(arg1)->api((char const *)arg2,(char const *)arg3);
  jresult = (void *)result; 
  return jresult;
}


SWIGEXPORT void * SWIGSTDCALL CSharp_ESLconnection_Bgapi(void * jarg1, char * jarg2, char * jarg3) {
  void * jresult ;
  ESLconnection *arg1 = (ESLconnection *) 0 ;
  char *arg2 = (char *) 0 ;
  char *arg3 = (char *) NULL ;
  ESLevent *result = 0 ;
  
  arg1 = (ESLconnection *)jarg1; 
  arg2 = (char *)jarg2; 
  arg3 = (char *)jarg3; 
  result = (ESLevent *)(arg1)->bgapi((char const *)arg2,(char const *)arg3);
  jresult = (void *)result; 
  return jresult;
}


SWIGEXPORT int SWIGSTDCALL CSharp_ESLconnection_SendEvent(void * jarg1, void * jarg2) {
  int jresult ;
  ESLconnection *arg1 = (ESLconnection *) 0 ;
  ESLevent *arg2 = (ESLevent *) 0 ;
  int result;
  
  arg1 = (ESLconnection *)jarg1; 
  arg2 = (ESLevent *)jarg2; 
  result = (int)(arg1)->sendEvent(arg2);
  jresult = result; 
  return jresult;
}


SWIGEXPORT void * SWIGSTDCALL CSharp_ESLconnection_RecvEvent(void * jarg1) {
  void * jresult ;
  ESLconnection *arg1 = (ESLconnection *) 0 ;
  ESLevent *result = 0 ;
  
  arg1 = (ESLconnection *)jarg1; 
  result = (ESLevent *)(arg1)->recvEvent();
  jresult = (void *)result; 
  return jresult;
}


SWIGEXPORT void * SWIGSTDCALL CSharp_ESLconnection_RecvEventTimed(void * jarg1, int jarg2) {
  void * jresult ;
  ESLconnection *arg1 = (ESLconnection *) 0 ;
  int arg2 ;
  ESLevent *result = 0 ;
  
  arg1 = (ESLconnection *)jarg1; 
  arg2 = (int)jarg2; 
  result = (ESLevent *)(arg1)->recvEventTimed(arg2);
  jresult = (void *)result; 
  return jresult;
}


SWIGEXPORT void * SWIGSTDCALL CSharp_ESLconnection_Filter(void * jarg1, char * jarg2, char * jarg3) {
  void * jresult ;
  ESLconnection *arg1 = (ESLconnection *) 0 ;
  char *arg2 = (char *) 0 ;
  char *arg3 = (char *) 0 ;
  ESLevent *result = 0 ;
  
  arg1 = (ESLconnection *)jarg1; 
  arg2 = (char *)jarg2; 
  arg3 = (char *)jarg3; 
  result = (ESLevent *)(arg1)->filter((char const *)arg2,(char const *)arg3);
  jresult = (void *)result; 
  return jresult;
}


SWIGEXPORT int SWIGSTDCALL CSharp_ESLconnection_Events(void * jarg1, char * jarg2, char * jarg3) {
  int jresult ;
  ESLconnection *arg1 = (ESLconnection *) 0 ;
  char *arg2 = (char *) 0 ;
  char *arg3 = (char *) 0 ;
  int result;
  
  arg1 = (ESLconnection *)jarg1; 
  arg2 = (char *)jarg2; 
  arg3 = (char *)jarg3; 
  result = (int)(arg1)->events((char const *)arg2,(char const *)arg3);
  jresult = result; 
  return jresult;
}


SWIGEXPORT void * SWIGSTDCALL CSharp_ESLconnection_Execute(void * jarg1, char * jarg2, char * jarg3, char * jarg4) {
  void * jresult ;
  ESLconnection *arg1 = (ESLconnection *) 0 ;
  char *arg2 = (char *) 0 ;
  char *arg3 = (char *) NULL ;
  char *arg4 = (char *) NULL ;
  ESLevent *result = 0 ;
  
  arg1 = (ESLconnection *)jarg1; 
  arg2 = (char *)jarg2; 
  arg3 = (char *)jarg3; 
  arg4 = (char *)jarg4; 
  result = (ESLevent *)(arg1)->execute((char const *)arg2,(char const *)arg3,(char const *)arg4);
  jresult = (void *)result; 
  return jresult;
}


SWIGEXPORT void * SWIGSTDCALL CSharp_ESLconnection_ExecuteAsync(void * jarg1, char * jarg2, char * jarg3, char * jarg4) {
  void * jresult ;
  ESLconnection *arg1 = (ESLconnection *) 0 ;
  char *arg2 = (char *) 0 ;
  char *arg3 = (char *) NULL ;
  char *arg4 = (char *) NULL ;
  ESLevent *result = 0 ;
  
  arg1 = (ESLconnection *)jarg1; 
  arg2 = (char *)jarg2; 
  arg3 = (char *)jarg3; 
  arg4 = (char *)jarg4; 
  result = (ESLevent *)(arg1)->executeAsync((char const *)arg2,(char const *)arg3,(char const *)arg4);
  jresult = (void *)result; 
  return jresult;
}


SWIGEXPORT int SWIGSTDCALL CSharp_ESLconnection_SetAsyncExecute(void * jarg1, char * jarg2) {
  int jresult ;
  ESLconnection *arg1 = (ESLconnection *) 0 ;
  char *arg2 = (char *) 0 ;
  int result;
  
  arg1 = (ESLconnection *)jarg1; 
  arg2 = (char *)jarg2; 
  result = (int)(arg1)->setAsyncExecute((char const *)arg2);
  jresult = result; 
  return jresult;
}


SWIGEXPORT int SWIGSTDCALL CSharp_ESLconnection_SetEventLock(void * jarg1, char * jarg2) {
  int jresult ;
  ESLconnection *arg1 = (ESLconnection *) 0 ;
  char *arg2 = (char *) 0 ;
  int result;
  
  arg1 = (ESLconnection *)jarg1; 
  arg2 = (char *)jarg2; 
  result = (int)(arg1)->setEventLock((char const *)arg2);
  jresult = result; 
  return jresult;
}


SWIGEXPORT int SWIGSTDCALL CSharp_ESLconnection_Disconnect(void * jarg1) {
  int jresult ;
  ESLconnection *arg1 = (ESLconnection *) 0 ;
  int result;
  
  arg1 = (ESLconnection *)jarg1; 
  result = (int)(arg1)->disconnect();
  jresult = result; 
  return jresult;
}


SWIGEXPORT void SWIGSTDCALL CSharp_eslSetLogLevel(int jarg1) {
  int arg1 ;
  
  arg1 = (int)jarg1; 
  eslSetLogLevel(arg1);
}


#ifdef __cplusplus
}
#endif


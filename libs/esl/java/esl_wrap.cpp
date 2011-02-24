/* ----------------------------------------------------------------------------
 * This file was automatically generated by SWIG (http://www.swig.org).
 * Version 1.3.35
 * 
 * This file is not intended to be easily readable and contains a number of 
 * coding conventions designed to improve portability and efficiency. Do not make
 * changes to this file unless you know what you are doing--modify the SWIG 
 * interface file instead. 
 * ----------------------------------------------------------------------------- */


#ifdef __cplusplus
template<typename T> class SwigValueWrapper {
    T *tt;
public:
    SwigValueWrapper() : tt(0) { }
    SwigValueWrapper(const SwigValueWrapper<T>& rhs) : tt(new T(*rhs.tt)) { }
    SwigValueWrapper(const T& t) : tt(new T(t)) { }
    ~SwigValueWrapper() { delete tt; } 
    SwigValueWrapper& operator=(const T& t) { delete tt; tt = new T(t); return *this; }
    operator T&() const { return *tt; }
    T *operator&() { return tt; }
private:
    SwigValueWrapper& operator=(const SwigValueWrapper<T>& rhs);
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



/* Fix for jlong on some versions of gcc on Windows */
#if defined(__GNUC__) && !defined(__INTELC__)
  typedef long long __int64;
#endif

/* Fix for jlong on 64-bit x86 Solaris */
#if defined(__x86_64)
# ifdef _LP64
#   undef _LP64
# endif
#endif

#include <jni.h>
#include <stdlib.h>
#include <string.h>


/* Support for throwing Java exceptions */
typedef enum {
  SWIG_JavaOutOfMemoryError = 1, 
  SWIG_JavaIOException, 
  SWIG_JavaRuntimeException, 
  SWIG_JavaIndexOutOfBoundsException,
  SWIG_JavaArithmeticException,
  SWIG_JavaIllegalArgumentException,
  SWIG_JavaNullPointerException,
  SWIG_JavaDirectorPureVirtual,
  SWIG_JavaUnknownError
} SWIG_JavaExceptionCodes;

typedef struct {
  SWIG_JavaExceptionCodes code;
  const char *java_exception;
} SWIG_JavaExceptions_t;


static void SWIGUNUSED SWIG_JavaThrowException(JNIEnv *jenv, SWIG_JavaExceptionCodes code, const char *msg) {
  jclass excep;
  static const SWIG_JavaExceptions_t java_exceptions[] = {
    { SWIG_JavaOutOfMemoryError, "java/lang/OutOfMemoryError" },
    { SWIG_JavaIOException, "java/io/IOException" },
    { SWIG_JavaRuntimeException, "java/lang/RuntimeException" },
    { SWIG_JavaIndexOutOfBoundsException, "java/lang/IndexOutOfBoundsException" },
    { SWIG_JavaArithmeticException, "java/lang/ArithmeticException" },
    { SWIG_JavaIllegalArgumentException, "java/lang/IllegalArgumentException" },
    { SWIG_JavaNullPointerException, "java/lang/NullPointerException" },
    { SWIG_JavaDirectorPureVirtual, "java/lang/RuntimeException" },
    { SWIG_JavaUnknownError,  "java/lang/UnknownError" },
    { (SWIG_JavaExceptionCodes)0,  "java/lang/UnknownError" } };
  const SWIG_JavaExceptions_t *except_ptr = java_exceptions;

  while (except_ptr->code != code && except_ptr->code)
    except_ptr++;

  jenv->ExceptionClear();
  excep = jenv->FindClass(except_ptr->java_exception);
  if (excep)
    jenv->ThrowNew(excep, msg);
}


/* Contract support */

#define SWIG_contract_assert(nullreturn, expr, msg) if (!(expr)) {SWIG_JavaThrowException(jenv, SWIG_JavaIllegalArgumentException, msg); return nullreturn; } else


#include "esl.h"
#include "esl_oop.h"


#ifdef __cplusplus
extern "C" {
#endif

SWIGEXPORT void JNICALL Java_org_freeswitch_esl_eslJNI_ESLevent_1event_1set(JNIEnv *jenv, jclass jcls, jlong jarg1, jobject jarg1_, jlong jarg2) {
  ESLevent *arg1 = (ESLevent *) 0 ;
  esl_event_t *arg2 = (esl_event_t *) 0 ;
  
  (void)jenv;
  (void)jcls;
  (void)jarg1_;
  arg1 = *(ESLevent **)&jarg1; 
  arg2 = *(esl_event_t **)&jarg2; 
  if (arg1) (arg1)->event = arg2;
  
}


SWIGEXPORT jlong JNICALL Java_org_freeswitch_esl_eslJNI_ESLevent_1event_1get(JNIEnv *jenv, jclass jcls, jlong jarg1, jobject jarg1_) {
  jlong jresult = 0 ;
  ESLevent *arg1 = (ESLevent *) 0 ;
  esl_event_t *result = 0 ;
  
  (void)jenv;
  (void)jcls;
  (void)jarg1_;
  arg1 = *(ESLevent **)&jarg1; 
  result = (esl_event_t *) ((arg1)->event);
  *(esl_event_t **)&jresult = result; 
  return jresult;
}


SWIGEXPORT void JNICALL Java_org_freeswitch_esl_eslJNI_ESLevent_1serialized_1string_1set(JNIEnv *jenv, jclass jcls, jlong jarg1, jobject jarg1_, jstring jarg2) {
  ESLevent *arg1 = (ESLevent *) 0 ;
  char *arg2 = (char *) 0 ;
  
  (void)jenv;
  (void)jcls;
  (void)jarg1_;
  arg1 = *(ESLevent **)&jarg1; 
  arg2 = 0;
  if (jarg2) {
    arg2 = (char *)jenv->GetStringUTFChars(jarg2, 0);
    if (!arg2) return ;
  }
  {
    if (arg1->serialized_string) delete [] arg1->serialized_string;
    if (arg2) {
      arg1->serialized_string = (char *) (new char[strlen((const char *)arg2)+1]);
      strcpy((char *)arg1->serialized_string, (const char *)arg2);
    } else {
      arg1->serialized_string = 0;
    }
  }
  if (arg2) jenv->ReleaseStringUTFChars(jarg2, (const char *)arg2);
}


SWIGEXPORT jstring JNICALL Java_org_freeswitch_esl_eslJNI_ESLevent_1serialized_1string_1get(JNIEnv *jenv, jclass jcls, jlong jarg1, jobject jarg1_) {
  jstring jresult = 0 ;
  ESLevent *arg1 = (ESLevent *) 0 ;
  char *result = 0 ;
  
  (void)jenv;
  (void)jcls;
  (void)jarg1_;
  arg1 = *(ESLevent **)&jarg1; 
  result = (char *) ((arg1)->serialized_string);
  if(result) jresult = jenv->NewStringUTF((const char *)result);
  return jresult;
}


SWIGEXPORT void JNICALL Java_org_freeswitch_esl_eslJNI_ESLevent_1mine_1set(JNIEnv *jenv, jclass jcls, jlong jarg1, jobject jarg1_, jint jarg2) {
  ESLevent *arg1 = (ESLevent *) 0 ;
  int arg2 ;
  
  (void)jenv;
  (void)jcls;
  (void)jarg1_;
  arg1 = *(ESLevent **)&jarg1; 
  arg2 = (int)jarg2; 
  if (arg1) (arg1)->mine = arg2;
  
}


SWIGEXPORT jint JNICALL Java_org_freeswitch_esl_eslJNI_ESLevent_1mine_1get(JNIEnv *jenv, jclass jcls, jlong jarg1, jobject jarg1_) {
  jint jresult = 0 ;
  ESLevent *arg1 = (ESLevent *) 0 ;
  int result;
  
  (void)jenv;
  (void)jcls;
  (void)jarg1_;
  arg1 = *(ESLevent **)&jarg1; 
  result = (int) ((arg1)->mine);
  jresult = (jint)result; 
  return jresult;
}


SWIGEXPORT jlong JNICALL Java_org_freeswitch_esl_eslJNI_new_1ESLevent_1_1SWIG_10(JNIEnv *jenv, jclass jcls, jstring jarg1, jstring jarg2) {
  jlong jresult = 0 ;
  char *arg1 = (char *) 0 ;
  char *arg2 = (char *) NULL ;
  ESLevent *result = 0 ;
  
  (void)jenv;
  (void)jcls;
  arg1 = 0;
  if (jarg1) {
    arg1 = (char *)jenv->GetStringUTFChars(jarg1, 0);
    if (!arg1) return 0;
  }
  arg2 = 0;
  if (jarg2) {
    arg2 = (char *)jenv->GetStringUTFChars(jarg2, 0);
    if (!arg2) return 0;
  }
  result = (ESLevent *)new ESLevent((char const *)arg1,(char const *)arg2);
  *(ESLevent **)&jresult = result; 
  if (arg1) jenv->ReleaseStringUTFChars(jarg1, (const char *)arg1);
  if (arg2) jenv->ReleaseStringUTFChars(jarg2, (const char *)arg2);
  return jresult;
}


SWIGEXPORT jlong JNICALL Java_org_freeswitch_esl_eslJNI_new_1ESLevent_1_1SWIG_11(JNIEnv *jenv, jclass jcls, jlong jarg1, jint jarg2) {
  jlong jresult = 0 ;
  esl_event_t *arg1 = (esl_event_t *) 0 ;
  int arg2 = (int) 0 ;
  ESLevent *result = 0 ;
  
  (void)jenv;
  (void)jcls;
  arg1 = *(esl_event_t **)&jarg1; 
  arg2 = (int)jarg2; 
  result = (ESLevent *)new ESLevent(arg1,arg2);
  *(ESLevent **)&jresult = result; 
  return jresult;
}


SWIGEXPORT jlong JNICALL Java_org_freeswitch_esl_eslJNI_new_1ESLevent_1_1SWIG_12(JNIEnv *jenv, jclass jcls, jlong jarg1, jobject jarg1_) {
  jlong jresult = 0 ;
  ESLevent *arg1 = (ESLevent *) 0 ;
  ESLevent *result = 0 ;
  
  (void)jenv;
  (void)jcls;
  (void)jarg1_;
  arg1 = *(ESLevent **)&jarg1; 
  result = (ESLevent *)new ESLevent(arg1);
  *(ESLevent **)&jresult = result; 
  return jresult;
}


SWIGEXPORT void JNICALL Java_org_freeswitch_esl_eslJNI_delete_1ESLevent(JNIEnv *jenv, jclass jcls, jlong jarg1) {
  ESLevent *arg1 = (ESLevent *) 0 ;
  
  (void)jenv;
  (void)jcls;
  arg1 = *(ESLevent **)&jarg1; 
  delete arg1;
  
}


SWIGEXPORT jstring JNICALL Java_org_freeswitch_esl_eslJNI_ESLevent_1serialize(JNIEnv *jenv, jclass jcls, jlong jarg1, jobject jarg1_, jstring jarg2) {
  jstring jresult = 0 ;
  ESLevent *arg1 = (ESLevent *) 0 ;
  char *arg2 = (char *) NULL ;
  char *result = 0 ;
  
  (void)jenv;
  (void)jcls;
  (void)jarg1_;
  arg1 = *(ESLevent **)&jarg1; 
  arg2 = 0;
  if (jarg2) {
    arg2 = (char *)jenv->GetStringUTFChars(jarg2, 0);
    if (!arg2) return 0;
  }
  result = (char *)(arg1)->serialize((char const *)arg2);
  if(result) jresult = jenv->NewStringUTF((const char *)result);
  if (arg2) jenv->ReleaseStringUTFChars(jarg2, (const char *)arg2);
  return jresult;
}


SWIGEXPORT jboolean JNICALL Java_org_freeswitch_esl_eslJNI_ESLevent_1setPriority(JNIEnv *jenv, jclass jcls, jlong jarg1, jobject jarg1_, jlong jarg2) {
  jboolean jresult = 0 ;
  ESLevent *arg1 = (ESLevent *) 0 ;
  esl_priority_t arg2 = (esl_priority_t) ESL_PRIORITY_NORMAL ;
  bool result;
  esl_priority_t *argp2 ;
  
  (void)jenv;
  (void)jcls;
  (void)jarg1_;
  arg1 = *(ESLevent **)&jarg1; 
  argp2 = *(esl_priority_t **)&jarg2; 
  if (!argp2) {
    SWIG_JavaThrowException(jenv, SWIG_JavaNullPointerException, "Attempt to dereference null esl_priority_t");
    return 0;
  }
  arg2 = *argp2; 
  result = (bool)(arg1)->setPriority(arg2);
  jresult = (jboolean)result; 
  return jresult;
}


SWIGEXPORT jstring JNICALL Java_org_freeswitch_esl_eslJNI_ESLevent_1getHeader(JNIEnv *jenv, jclass jcls, jlong jarg1, jobject jarg1_, jstring jarg2) {
  jstring jresult = 0 ;
  ESLevent *arg1 = (ESLevent *) 0 ;
  char *arg2 = (char *) 0 ;
  char *result = 0 ;
  
  (void)jenv;
  (void)jcls;
  (void)jarg1_;
  arg1 = *(ESLevent **)&jarg1; 
  arg2 = 0;
  if (jarg2) {
    arg2 = (char *)jenv->GetStringUTFChars(jarg2, 0);
    if (!arg2) return 0;
  }
  result = (char *)(arg1)->getHeader((char const *)arg2);
  if(result) jresult = jenv->NewStringUTF((const char *)result);
  if (arg2) jenv->ReleaseStringUTFChars(jarg2, (const char *)arg2);
  return jresult;
}


SWIGEXPORT jstring JNICALL Java_org_freeswitch_esl_eslJNI_ESLevent_1getBody(JNIEnv *jenv, jclass jcls, jlong jarg1, jobject jarg1_) {
  jstring jresult = 0 ;
  ESLevent *arg1 = (ESLevent *) 0 ;
  char *result = 0 ;
  
  (void)jenv;
  (void)jcls;
  (void)jarg1_;
  arg1 = *(ESLevent **)&jarg1; 
  result = (char *)(arg1)->getBody();
  if(result) jresult = jenv->NewStringUTF((const char *)result);
  return jresult;
}


SWIGEXPORT jstring JNICALL Java_org_freeswitch_esl_eslJNI_ESLevent_1getType(JNIEnv *jenv, jclass jcls, jlong jarg1, jobject jarg1_) {
  jstring jresult = 0 ;
  ESLevent *arg1 = (ESLevent *) 0 ;
  char *result = 0 ;
  
  (void)jenv;
  (void)jcls;
  (void)jarg1_;
  arg1 = *(ESLevent **)&jarg1; 
  result = (char *)(arg1)->getType();
  if(result) jresult = jenv->NewStringUTF((const char *)result);
  return jresult;
}


SWIGEXPORT jboolean JNICALL Java_org_freeswitch_esl_eslJNI_ESLevent_1addBody(JNIEnv *jenv, jclass jcls, jlong jarg1, jobject jarg1_, jstring jarg2) {
  jboolean jresult = 0 ;
  ESLevent *arg1 = (ESLevent *) 0 ;
  char *arg2 = (char *) 0 ;
  bool result;
  
  (void)jenv;
  (void)jcls;
  (void)jarg1_;
  arg1 = *(ESLevent **)&jarg1; 
  arg2 = 0;
  if (jarg2) {
    arg2 = (char *)jenv->GetStringUTFChars(jarg2, 0);
    if (!arg2) return 0;
  }
  result = (bool)(arg1)->addBody((char const *)arg2);
  jresult = (jboolean)result; 
  if (arg2) jenv->ReleaseStringUTFChars(jarg2, (const char *)arg2);
  return jresult;
}


SWIGEXPORT jboolean JNICALL Java_org_freeswitch_esl_eslJNI_ESLevent_1addHeader(JNIEnv *jenv, jclass jcls, jlong jarg1, jobject jarg1_, jstring jarg2, jstring jarg3) {
  jboolean jresult = 0 ;
  ESLevent *arg1 = (ESLevent *) 0 ;
  char *arg2 = (char *) 0 ;
  char *arg3 = (char *) 0 ;
  bool result;
  
  (void)jenv;
  (void)jcls;
  (void)jarg1_;
  arg1 = *(ESLevent **)&jarg1; 
  arg2 = 0;
  if (jarg2) {
    arg2 = (char *)jenv->GetStringUTFChars(jarg2, 0);
    if (!arg2) return 0;
  }
  arg3 = 0;
  if (jarg3) {
    arg3 = (char *)jenv->GetStringUTFChars(jarg3, 0);
    if (!arg3) return 0;
  }
  result = (bool)(arg1)->addHeader((char const *)arg2,(char const *)arg3);
  jresult = (jboolean)result; 
  if (arg2) jenv->ReleaseStringUTFChars(jarg2, (const char *)arg2);
  if (arg3) jenv->ReleaseStringUTFChars(jarg3, (const char *)arg3);
  return jresult;
}


SWIGEXPORT jboolean JNICALL Java_org_freeswitch_esl_eslJNI_ESLevent_1delHeader(JNIEnv *jenv, jclass jcls, jlong jarg1, jobject jarg1_, jstring jarg2) {
  jboolean jresult = 0 ;
  ESLevent *arg1 = (ESLevent *) 0 ;
  char *arg2 = (char *) 0 ;
  bool result;
  
  (void)jenv;
  (void)jcls;
  (void)jarg1_;
  arg1 = *(ESLevent **)&jarg1; 
  arg2 = 0;
  if (jarg2) {
    arg2 = (char *)jenv->GetStringUTFChars(jarg2, 0);
    if (!arg2) return 0;
  }
  result = (bool)(arg1)->delHeader((char const *)arg2);
  jresult = (jboolean)result; 
  if (arg2) jenv->ReleaseStringUTFChars(jarg2, (const char *)arg2);
  return jresult;
}


SWIGEXPORT jstring JNICALL Java_org_freeswitch_esl_eslJNI_ESLevent_1firstHeader(JNIEnv *jenv, jclass jcls, jlong jarg1, jobject jarg1_) {
  jstring jresult = 0 ;
  ESLevent *arg1 = (ESLevent *) 0 ;
  char *result = 0 ;
  
  (void)jenv;
  (void)jcls;
  (void)jarg1_;
  arg1 = *(ESLevent **)&jarg1; 
  result = (char *)(arg1)->firstHeader();
  if(result) jresult = jenv->NewStringUTF((const char *)result);
  return jresult;
}


SWIGEXPORT jstring JNICALL Java_org_freeswitch_esl_eslJNI_ESLevent_1nextHeader(JNIEnv *jenv, jclass jcls, jlong jarg1, jobject jarg1_) {
  jstring jresult = 0 ;
  ESLevent *arg1 = (ESLevent *) 0 ;
  char *result = 0 ;
  
  (void)jenv;
  (void)jcls;
  (void)jarg1_;
  arg1 = *(ESLevent **)&jarg1; 
  result = (char *)(arg1)->nextHeader();
  if(result) jresult = jenv->NewStringUTF((const char *)result);
  return jresult;
}


SWIGEXPORT jlong JNICALL Java_org_freeswitch_esl_eslJNI_new_1ESLconnection_1_1SWIG_10(JNIEnv *jenv, jclass jcls, jstring jarg1, jstring jarg2, jstring jarg3, jstring jarg4) {
  jlong jresult = 0 ;
  char *arg1 = (char *) 0 ;
  char *arg2 = (char *) 0 ;
  char *arg3 = (char *) 0 ;
  char *arg4 = (char *) 0 ;
  ESLconnection *result = 0 ;
  
  (void)jenv;
  (void)jcls;
  arg1 = 0;
  if (jarg1) {
    arg1 = (char *)jenv->GetStringUTFChars(jarg1, 0);
    if (!arg1) return 0;
  }
  arg2 = 0;
  if (jarg2) {
    arg2 = (char *)jenv->GetStringUTFChars(jarg2, 0);
    if (!arg2) return 0;
  }
  arg3 = 0;
  if (jarg3) {
    arg3 = (char *)jenv->GetStringUTFChars(jarg3, 0);
    if (!arg3) return 0;
  }
  arg4 = 0;
  if (jarg4) {
    arg4 = (char *)jenv->GetStringUTFChars(jarg4, 0);
    if (!arg4) return 0;
  }
  result = (ESLconnection *)new ESLconnection((char const *)arg1,(char const *)arg2,(char const *)arg3,(char const *)arg4);
  *(ESLconnection **)&jresult = result; 
  if (arg1) jenv->ReleaseStringUTFChars(jarg1, (const char *)arg1);
  if (arg2) jenv->ReleaseStringUTFChars(jarg2, (const char *)arg2);
  if (arg3) jenv->ReleaseStringUTFChars(jarg3, (const char *)arg3);
  if (arg4) jenv->ReleaseStringUTFChars(jarg4, (const char *)arg4);
  return jresult;
}


SWIGEXPORT jlong JNICALL Java_org_freeswitch_esl_eslJNI_new_1ESLconnection_1_1SWIG_11(JNIEnv *jenv, jclass jcls, jstring jarg1, jstring jarg2, jstring jarg3) {
  jlong jresult = 0 ;
  char *arg1 = (char *) 0 ;
  char *arg2 = (char *) 0 ;
  char *arg3 = (char *) 0 ;
  ESLconnection *result = 0 ;
  
  (void)jenv;
  (void)jcls;
  arg1 = 0;
  if (jarg1) {
    arg1 = (char *)jenv->GetStringUTFChars(jarg1, 0);
    if (!arg1) return 0;
  }
  arg2 = 0;
  if (jarg2) {
    arg2 = (char *)jenv->GetStringUTFChars(jarg2, 0);
    if (!arg2) return 0;
  }
  arg3 = 0;
  if (jarg3) {
    arg3 = (char *)jenv->GetStringUTFChars(jarg3, 0);
    if (!arg3) return 0;
  }
  result = (ESLconnection *)new ESLconnection((char const *)arg1,(char const *)arg2,(char const *)arg3);
  *(ESLconnection **)&jresult = result; 
  if (arg1) jenv->ReleaseStringUTFChars(jarg1, (const char *)arg1);
  if (arg2) jenv->ReleaseStringUTFChars(jarg2, (const char *)arg2);
  if (arg3) jenv->ReleaseStringUTFChars(jarg3, (const char *)arg3);
  return jresult;
}


SWIGEXPORT jlong JNICALL Java_org_freeswitch_esl_eslJNI_new_1ESLconnection_1_1SWIG_12(JNIEnv *jenv, jclass jcls, jint jarg1) {
  jlong jresult = 0 ;
  int arg1 ;
  ESLconnection *result = 0 ;
  
  (void)jenv;
  (void)jcls;
  arg1 = (int)jarg1; 
  result = (ESLconnection *)new ESLconnection(arg1);
  *(ESLconnection **)&jresult = result; 
  return jresult;
}


SWIGEXPORT void JNICALL Java_org_freeswitch_esl_eslJNI_delete_1ESLconnection(JNIEnv *jenv, jclass jcls, jlong jarg1) {
  ESLconnection *arg1 = (ESLconnection *) 0 ;
  
  (void)jenv;
  (void)jcls;
  arg1 = *(ESLconnection **)&jarg1; 
  delete arg1;
  
}


SWIGEXPORT jint JNICALL Java_org_freeswitch_esl_eslJNI_ESLconnection_1socketDescriptor(JNIEnv *jenv, jclass jcls, jlong jarg1, jobject jarg1_) {
  jint jresult = 0 ;
  ESLconnection *arg1 = (ESLconnection *) 0 ;
  int result;
  
  (void)jenv;
  (void)jcls;
  (void)jarg1_;
  arg1 = *(ESLconnection **)&jarg1; 
  result = (int)(arg1)->socketDescriptor();
  jresult = (jint)result; 
  return jresult;
}


SWIGEXPORT jint JNICALL Java_org_freeswitch_esl_eslJNI_ESLconnection_1connected(JNIEnv *jenv, jclass jcls, jlong jarg1, jobject jarg1_) {
  jint jresult = 0 ;
  ESLconnection *arg1 = (ESLconnection *) 0 ;
  int result;
  
  (void)jenv;
  (void)jcls;
  (void)jarg1_;
  arg1 = *(ESLconnection **)&jarg1; 
  result = (int)(arg1)->connected();
  jresult = (jint)result; 
  return jresult;
}


SWIGEXPORT jlong JNICALL Java_org_freeswitch_esl_eslJNI_ESLconnection_1getInfo(JNIEnv *jenv, jclass jcls, jlong jarg1, jobject jarg1_) {
  jlong jresult = 0 ;
  ESLconnection *arg1 = (ESLconnection *) 0 ;
  ESLevent *result = 0 ;
  
  (void)jenv;
  (void)jcls;
  (void)jarg1_;
  arg1 = *(ESLconnection **)&jarg1; 
  result = (ESLevent *)(arg1)->getInfo();
  *(ESLevent **)&jresult = result; 
  return jresult;
}


SWIGEXPORT jint JNICALL Java_org_freeswitch_esl_eslJNI_ESLconnection_1send(JNIEnv *jenv, jclass jcls, jlong jarg1, jobject jarg1_, jstring jarg2) {
  jint jresult = 0 ;
  ESLconnection *arg1 = (ESLconnection *) 0 ;
  char *arg2 = (char *) 0 ;
  int result;
  
  (void)jenv;
  (void)jcls;
  (void)jarg1_;
  arg1 = *(ESLconnection **)&jarg1; 
  arg2 = 0;
  if (jarg2) {
    arg2 = (char *)jenv->GetStringUTFChars(jarg2, 0);
    if (!arg2) return 0;
  }
  result = (int)(arg1)->send((char const *)arg2);
  jresult = (jint)result; 
  if (arg2) jenv->ReleaseStringUTFChars(jarg2, (const char *)arg2);
  return jresult;
}


SWIGEXPORT jlong JNICALL Java_org_freeswitch_esl_eslJNI_ESLconnection_1sendRecv(JNIEnv *jenv, jclass jcls, jlong jarg1, jobject jarg1_, jstring jarg2) {
  jlong jresult = 0 ;
  ESLconnection *arg1 = (ESLconnection *) 0 ;
  char *arg2 = (char *) 0 ;
  ESLevent *result = 0 ;
  
  (void)jenv;
  (void)jcls;
  (void)jarg1_;
  arg1 = *(ESLconnection **)&jarg1; 
  arg2 = 0;
  if (jarg2) {
    arg2 = (char *)jenv->GetStringUTFChars(jarg2, 0);
    if (!arg2) return 0;
  }
  result = (ESLevent *)(arg1)->sendRecv((char const *)arg2);
  *(ESLevent **)&jresult = result; 
  if (arg2) jenv->ReleaseStringUTFChars(jarg2, (const char *)arg2);
  return jresult;
}


SWIGEXPORT jlong JNICALL Java_org_freeswitch_esl_eslJNI_ESLconnection_1api(JNIEnv *jenv, jclass jcls, jlong jarg1, jobject jarg1_, jstring jarg2, jstring jarg3) {
  jlong jresult = 0 ;
  ESLconnection *arg1 = (ESLconnection *) 0 ;
  char *arg2 = (char *) 0 ;
  char *arg3 = (char *) NULL ;
  ESLevent *result = 0 ;
  
  (void)jenv;
  (void)jcls;
  (void)jarg1_;
  arg1 = *(ESLconnection **)&jarg1; 
  arg2 = 0;
  if (jarg2) {
    arg2 = (char *)jenv->GetStringUTFChars(jarg2, 0);
    if (!arg2) return 0;
  }
  arg3 = 0;
  if (jarg3) {
    arg3 = (char *)jenv->GetStringUTFChars(jarg3, 0);
    if (!arg3) return 0;
  }
  result = (ESLevent *)(arg1)->api((char const *)arg2,(char const *)arg3);
  *(ESLevent **)&jresult = result; 
  if (arg2) jenv->ReleaseStringUTFChars(jarg2, (const char *)arg2);
  if (arg3) jenv->ReleaseStringUTFChars(jarg3, (const char *)arg3);
  return jresult;
}


SWIGEXPORT jlong JNICALL Java_org_freeswitch_esl_eslJNI_ESLconnection_1bgapi(JNIEnv *jenv, jclass jcls, jlong jarg1, jobject jarg1_, jstring jarg2, jstring jarg3, jstring jarg4) {
  jlong jresult = 0 ;
  ESLconnection *arg1 = (ESLconnection *) 0 ;
  char *arg2 = (char *) 0 ;
  char *arg3 = (char *) NULL ;
  char *arg4 = (char *) NULL ;
  ESLevent *result = 0 ;
  
  (void)jenv;
  (void)jcls;
  (void)jarg1_;
  arg1 = *(ESLconnection **)&jarg1; 
  arg2 = 0;
  if (jarg2) {
    arg2 = (char *)jenv->GetStringUTFChars(jarg2, 0);
    if (!arg2) return 0;
  }
  arg3 = 0;
  if (jarg3) {
    arg3 = (char *)jenv->GetStringUTFChars(jarg3, 0);
    if (!arg3) return 0;
  }
  arg4 = 0;
  if (jarg4) {
    arg4 = (char *)jenv->GetStringUTFChars(jarg4, 0);
    if (!arg4) return 0;
  }
  result = (ESLevent *)(arg1)->bgapi((char const *)arg2,(char const *)arg3,(char const *)arg4);
  *(ESLevent **)&jresult = result; 
  if (arg2) jenv->ReleaseStringUTFChars(jarg2, (const char *)arg2);
  if (arg3) jenv->ReleaseStringUTFChars(jarg3, (const char *)arg3);
  if (arg4) jenv->ReleaseStringUTFChars(jarg4, (const char *)arg4);
  return jresult;
}


SWIGEXPORT jlong JNICALL Java_org_freeswitch_esl_eslJNI_ESLconnection_1sendEvent(JNIEnv *jenv, jclass jcls, jlong jarg1, jobject jarg1_, jlong jarg2, jobject jarg2_) {
  jlong jresult = 0 ;
  ESLconnection *arg1 = (ESLconnection *) 0 ;
  ESLevent *arg2 = (ESLevent *) 0 ;
  ESLevent *result = 0 ;
  
  (void)jenv;
  (void)jcls;
  (void)jarg1_;
  (void)jarg2_;
  arg1 = *(ESLconnection **)&jarg1; 
  arg2 = *(ESLevent **)&jarg2; 
  result = (ESLevent *)(arg1)->sendEvent(arg2);
  *(ESLevent **)&jresult = result; 
  return jresult;
}


SWIGEXPORT jlong JNICALL Java_org_freeswitch_esl_eslJNI_ESLconnection_1recvEvent(JNIEnv *jenv, jclass jcls, jlong jarg1, jobject jarg1_) {
  jlong jresult = 0 ;
  ESLconnection *arg1 = (ESLconnection *) 0 ;
  ESLevent *result = 0 ;
  
  (void)jenv;
  (void)jcls;
  (void)jarg1_;
  arg1 = *(ESLconnection **)&jarg1; 
  result = (ESLevent *)(arg1)->recvEvent();
  *(ESLevent **)&jresult = result; 
  return jresult;
}


SWIGEXPORT jlong JNICALL Java_org_freeswitch_esl_eslJNI_ESLconnection_1recvEventTimed(JNIEnv *jenv, jclass jcls, jlong jarg1, jobject jarg1_, jint jarg2) {
  jlong jresult = 0 ;
  ESLconnection *arg1 = (ESLconnection *) 0 ;
  int arg2 ;
  ESLevent *result = 0 ;
  
  (void)jenv;
  (void)jcls;
  (void)jarg1_;
  arg1 = *(ESLconnection **)&jarg1; 
  arg2 = (int)jarg2; 
  result = (ESLevent *)(arg1)->recvEventTimed(arg2);
  *(ESLevent **)&jresult = result; 
  return jresult;
}


SWIGEXPORT jlong JNICALL Java_org_freeswitch_esl_eslJNI_ESLconnection_1filter(JNIEnv *jenv, jclass jcls, jlong jarg1, jobject jarg1_, jstring jarg2, jstring jarg3) {
  jlong jresult = 0 ;
  ESLconnection *arg1 = (ESLconnection *) 0 ;
  char *arg2 = (char *) 0 ;
  char *arg3 = (char *) 0 ;
  ESLevent *result = 0 ;
  
  (void)jenv;
  (void)jcls;
  (void)jarg1_;
  arg1 = *(ESLconnection **)&jarg1; 
  arg2 = 0;
  if (jarg2) {
    arg2 = (char *)jenv->GetStringUTFChars(jarg2, 0);
    if (!arg2) return 0;
  }
  arg3 = 0;
  if (jarg3) {
    arg3 = (char *)jenv->GetStringUTFChars(jarg3, 0);
    if (!arg3) return 0;
  }
  result = (ESLevent *)(arg1)->filter((char const *)arg2,(char const *)arg3);
  *(ESLevent **)&jresult = result; 
  if (arg2) jenv->ReleaseStringUTFChars(jarg2, (const char *)arg2);
  if (arg3) jenv->ReleaseStringUTFChars(jarg3, (const char *)arg3);
  return jresult;
}


SWIGEXPORT jint JNICALL Java_org_freeswitch_esl_eslJNI_ESLconnection_1events(JNIEnv *jenv, jclass jcls, jlong jarg1, jobject jarg1_, jstring jarg2, jstring jarg3) {
  jint jresult = 0 ;
  ESLconnection *arg1 = (ESLconnection *) 0 ;
  char *arg2 = (char *) 0 ;
  char *arg3 = (char *) 0 ;
  int result;
  
  (void)jenv;
  (void)jcls;
  (void)jarg1_;
  arg1 = *(ESLconnection **)&jarg1; 
  arg2 = 0;
  if (jarg2) {
    arg2 = (char *)jenv->GetStringUTFChars(jarg2, 0);
    if (!arg2) return 0;
  }
  arg3 = 0;
  if (jarg3) {
    arg3 = (char *)jenv->GetStringUTFChars(jarg3, 0);
    if (!arg3) return 0;
  }
  result = (int)(arg1)->events((char const *)arg2,(char const *)arg3);
  jresult = (jint)result; 
  if (arg2) jenv->ReleaseStringUTFChars(jarg2, (const char *)arg2);
  if (arg3) jenv->ReleaseStringUTFChars(jarg3, (const char *)arg3);
  return jresult;
}


SWIGEXPORT jlong JNICALL Java_org_freeswitch_esl_eslJNI_ESLconnection_1execute(JNIEnv *jenv, jclass jcls, jlong jarg1, jobject jarg1_, jstring jarg2, jstring jarg3, jstring jarg4) {
  jlong jresult = 0 ;
  ESLconnection *arg1 = (ESLconnection *) 0 ;
  char *arg2 = (char *) 0 ;
  char *arg3 = (char *) NULL ;
  char *arg4 = (char *) NULL ;
  ESLevent *result = 0 ;
  
  (void)jenv;
  (void)jcls;
  (void)jarg1_;
  arg1 = *(ESLconnection **)&jarg1; 
  arg2 = 0;
  if (jarg2) {
    arg2 = (char *)jenv->GetStringUTFChars(jarg2, 0);
    if (!arg2) return 0;
  }
  arg3 = 0;
  if (jarg3) {
    arg3 = (char *)jenv->GetStringUTFChars(jarg3, 0);
    if (!arg3) return 0;
  }
  arg4 = 0;
  if (jarg4) {
    arg4 = (char *)jenv->GetStringUTFChars(jarg4, 0);
    if (!arg4) return 0;
  }
  result = (ESLevent *)(arg1)->execute((char const *)arg2,(char const *)arg3,(char const *)arg4);
  *(ESLevent **)&jresult = result; 
  if (arg2) jenv->ReleaseStringUTFChars(jarg2, (const char *)arg2);
  if (arg3) jenv->ReleaseStringUTFChars(jarg3, (const char *)arg3);
  if (arg4) jenv->ReleaseStringUTFChars(jarg4, (const char *)arg4);
  return jresult;
}


SWIGEXPORT jlong JNICALL Java_org_freeswitch_esl_eslJNI_ESLconnection_1executeAsync(JNIEnv *jenv, jclass jcls, jlong jarg1, jobject jarg1_, jstring jarg2, jstring jarg3, jstring jarg4) {
  jlong jresult = 0 ;
  ESLconnection *arg1 = (ESLconnection *) 0 ;
  char *arg2 = (char *) 0 ;
  char *arg3 = (char *) NULL ;
  char *arg4 = (char *) NULL ;
  ESLevent *result = 0 ;
  
  (void)jenv;
  (void)jcls;
  (void)jarg1_;
  arg1 = *(ESLconnection **)&jarg1; 
  arg2 = 0;
  if (jarg2) {
    arg2 = (char *)jenv->GetStringUTFChars(jarg2, 0);
    if (!arg2) return 0;
  }
  arg3 = 0;
  if (jarg3) {
    arg3 = (char *)jenv->GetStringUTFChars(jarg3, 0);
    if (!arg3) return 0;
  }
  arg4 = 0;
  if (jarg4) {
    arg4 = (char *)jenv->GetStringUTFChars(jarg4, 0);
    if (!arg4) return 0;
  }
  result = (ESLevent *)(arg1)->executeAsync((char const *)arg2,(char const *)arg3,(char const *)arg4);
  *(ESLevent **)&jresult = result; 
  if (arg2) jenv->ReleaseStringUTFChars(jarg2, (const char *)arg2);
  if (arg3) jenv->ReleaseStringUTFChars(jarg3, (const char *)arg3);
  if (arg4) jenv->ReleaseStringUTFChars(jarg4, (const char *)arg4);
  return jresult;
}


SWIGEXPORT jint JNICALL Java_org_freeswitch_esl_eslJNI_ESLconnection_1setAsyncExecute(JNIEnv *jenv, jclass jcls, jlong jarg1, jobject jarg1_, jstring jarg2) {
  jint jresult = 0 ;
  ESLconnection *arg1 = (ESLconnection *) 0 ;
  char *arg2 = (char *) 0 ;
  int result;
  
  (void)jenv;
  (void)jcls;
  (void)jarg1_;
  arg1 = *(ESLconnection **)&jarg1; 
  arg2 = 0;
  if (jarg2) {
    arg2 = (char *)jenv->GetStringUTFChars(jarg2, 0);
    if (!arg2) return 0;
  }
  result = (int)(arg1)->setAsyncExecute((char const *)arg2);
  jresult = (jint)result; 
  if (arg2) jenv->ReleaseStringUTFChars(jarg2, (const char *)arg2);
  return jresult;
}


SWIGEXPORT jint JNICALL Java_org_freeswitch_esl_eslJNI_ESLconnection_1setEventLock(JNIEnv *jenv, jclass jcls, jlong jarg1, jobject jarg1_, jstring jarg2) {
  jint jresult = 0 ;
  ESLconnection *arg1 = (ESLconnection *) 0 ;
  char *arg2 = (char *) 0 ;
  int result;
  
  (void)jenv;
  (void)jcls;
  (void)jarg1_;
  arg1 = *(ESLconnection **)&jarg1; 
  arg2 = 0;
  if (jarg2) {
    arg2 = (char *)jenv->GetStringUTFChars(jarg2, 0);
    if (!arg2) return 0;
  }
  result = (int)(arg1)->setEventLock((char const *)arg2);
  jresult = (jint)result; 
  if (arg2) jenv->ReleaseStringUTFChars(jarg2, (const char *)arg2);
  return jresult;
}


SWIGEXPORT jint JNICALL Java_org_freeswitch_esl_eslJNI_ESLconnection_1disconnect(JNIEnv *jenv, jclass jcls, jlong jarg1, jobject jarg1_) {
  jint jresult = 0 ;
  ESLconnection *arg1 = (ESLconnection *) 0 ;
  int result;
  
  (void)jenv;
  (void)jcls;
  (void)jarg1_;
  arg1 = *(ESLconnection **)&jarg1; 
  result = (int)(arg1)->disconnect();
  jresult = (jint)result; 
  return jresult;
}


SWIGEXPORT void JNICALL Java_org_freeswitch_esl_eslJNI_eslSetLogLevel(JNIEnv *jenv, jclass jcls, jint jarg1) {
  int arg1 ;
  
  (void)jenv;
  (void)jcls;
  arg1 = (int)jarg1; 
  eslSetLogLevel(arg1);
}


#ifdef __cplusplus
}
#endif


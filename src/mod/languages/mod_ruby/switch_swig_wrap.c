/* ----------------------------------------------------------------------------
 * This file was automatically generated by SWIG (http://www.swig.org).
 * Version 1.3.21
 * 
 * This file is not intended to be easily readable and contains a number of 
 * coding conventions designed to improve portability and efficiency. Do not make
 * changes to this file unless you know what you are doing--modify the SWIG 
 * interface file instead. 
 * ----------------------------------------------------------------------------- */

/* ruby.swg */
/* Implementation : RUBY */
#define SWIGRUBY 1

#include "ruby.h"

/* Flags for pointer conversion */
#define SWIG_POINTER_EXCEPTION     0x1
#define SWIG_POINTER_DISOWN        0x2

#define NUM2USHRT(n) (\
    (0 <= NUM2UINT(n) && NUM2UINT(n) <= USHRT_MAX)\
    ? (unsigned short) NUM2UINT(n) \
    : (rb_raise(rb_eArgError, "integer %d out of range of `unsigned short'",\
               NUM2UINT(n)), (short)0)\
)

#define NUM2SHRT(n) (\
    (SHRT_MIN <= NUM2INT(n) && NUM2INT(n) <= SHRT_MAX)\
    ? (short)NUM2INT(n)\
    : (rb_raise(rb_eArgError, "integer %d out of range of `short'",\
               NUM2INT(n)), (short)0)\
)

/* Ruby 1.7 defines NUM2LL(), LL2NUM() and ULL2NUM() macros */
#ifndef NUM2LL
#define NUM2LL(x) NUM2LONG((x))
#endif
#ifndef LL2NUM
#define LL2NUM(x) INT2NUM((long) (x))
#endif
#ifndef ULL2NUM
#define ULL2NUM(x) UINT2NUM((unsigned long) (x))
#endif

/* Ruby 1.7 doesn't (yet) define NUM2ULL() */
#ifndef NUM2ULL
#ifdef HAVE_LONG_LONG
#define NUM2ULL(x) rb_num2ull((x))
#else
#define NUM2ULL(x) NUM2ULONG(x)
#endif
#endif

/*
 * Need to be very careful about how these macros are defined, especially
 * when compiling C++ code or C code with an ANSI C compiler.
 *
 * VALUEFUNC(f) is a macro used to typecast a C function that implements
 * a Ruby method so that it can be passed as an argument to API functions
 * like rb_define_method() and rb_define_singleton_method().
 *
 * VOIDFUNC(f) is a macro used to typecast a C function that implements
 * either the "mark" or "free" stuff for a Ruby Data object, so that it
 * can be passed as an argument to API functions like Data_Wrap_Struct()
 * and Data_Make_Struct().
 */
 
#ifdef __cplusplus
#  ifndef RUBY_METHOD_FUNC /* These definitions should work for Ruby 1.4.6 */
#    define VALUEFUNC(f) ((VALUE (*)()) f)
#    define VOIDFUNC(f)  ((void (*)()) f)
#  else
#    ifndef ANYARGS /* These definitions should work for Ruby 1.6 */
#      define VALUEFUNC(f) ((VALUE (*)()) f)
#      define VOIDFUNC(f)  ((RUBY_DATA_FUNC) f)
#    else /* These definitions should work for Ruby 1.7 */
#      define VALUEFUNC(f) ((VALUE (*)(ANYARGS)) f)
#      define VOIDFUNC(f)  ((RUBY_DATA_FUNC) f)
#    endif
#  endif
#else
#  define VALUEFUNC(f) (f)
#  define VOIDFUNC(f) (f)
#endif

typedef struct {
  VALUE klass;
  VALUE mImpl;
  void  (*mark)(void *);
  void  (*destroy)(void *);
} swig_class;

/* Don't use for expressions have side effect */
#ifndef RB_STRING_VALUE
#define RB_STRING_VALUE(s) (TYPE(s) == T_STRING ? (s) : (*(volatile VALUE *)&(s) = rb_str_to_str(s)))
#endif
#ifndef StringValue
#define StringValue(s) RB_STRING_VALUE(s)
#endif
#ifndef StringValuePtr
#define StringValuePtr(s) RSTRING(RB_STRING_VALUE(s))->ptr
#endif
#ifndef StringValueLen
#define StringValueLen(s) RSTRING(RB_STRING_VALUE(s))->len
#endif
#ifndef SafeStringValue
#define SafeStringValue(v) do {\
    StringValue(v);\
    rb_check_safe_str(v);\
} while (0)
#endif

#ifndef HAVE_RB_DEFINE_ALLOC_FUNC
#define rb_define_alloc_func(klass, func) rb_define_singleton_method((klass), "new", VALUEFUNC((func)), -1)
#define rb_undef_alloc_func(klass) rb_undef_method(CLASS_OF((klass)), "new")
#endif

/* Contract support */

#define SWIG_contract_assert(expr, msg) if (!(expr)) { rb_raise(rb_eRuntimeError, (char *) msg ); } else


/*************************************************************** -*- c -*-
 * ruby/precommon.swg
 *
 * Rename all exported symbols from common.swg, to avoid symbol
 * clashes if multiple interpreters are included
 *
 ************************************************************************/

#define SWIG_TypeRegister    SWIG_Ruby_TypeRegister
#define SWIG_TypeCheck       SWIG_Ruby_TypeCheck
#define SWIG_TypeCast        SWIG_Ruby_TypeCast
#define SWIG_TypeDynamicCast SWIG_Ruby_TypeDynamicCast
#define SWIG_TypeName        SWIG_Ruby_TypeName
#define SWIG_TypeQuery       SWIG_Ruby_TypeQuery
#define SWIG_TypeClientData  SWIG_Ruby_TypeClientData
#define SWIG_PackData        SWIG_Ruby_PackData 
#define SWIG_UnpackData      SWIG_Ruby_UnpackData 

/* Also rename all exported symbols from rubydef.swig */

/* Common SWIG API */
#define SWIG_ConvertPtr(obj, pp, type, flags) \
  SWIG_Ruby_ConvertPtr(obj, pp, type, flags)
#define SWIG_NewPointerObj(p, type, flags) \
  SWIG_Ruby_NewPointerObj(p, type, flags)
#define SWIG_MustGetPtr(p, type, argnum, flags) \
  SWIG_Ruby_MustGetPtr(p, type, argnum, flags)

/* Ruby-specific SWIG API */

#define SWIG_InitRuntime() \
  SWIG_Ruby_InitRuntime()
#define SWIG_define_class(ty) \
  SWIG_Ruby_define_class(ty)
#define SWIG_NewClassInstance(value, ty) \
  SWIG_Ruby_NewClassInstance(value, ty)
#define SWIG_MangleStr(value) \
  SWIG_Ruby_MangleStr(value)
#define SWIG_CheckConvert(value, ty) \
  SWIG_Ruby_CheckConvert(value, ty)
#define SWIG_NewPackedObj(ptr, sz, ty) \
  SWIG_Ruby_NewPackedObj(ptr, sz, ty)
#define SWIG_ConvertPacked(obj, ptr, sz, ty, flags) \
  SWIG_Ruby_ConvertPacked(obj, ptr, sz, ty, flags)


/***********************************************************************
 * common.swg
 *
 *     This file contains generic SWIG runtime support for pointer
 *     type checking as well as a few commonly used macros to control
 *     external linkage.
 *
 * Author : David Beazley (beazley@cs.uchicago.edu)
 *
 * Copyright (c) 1999-2000, The University of Chicago
 * 
 * This file may be freely redistributed without license or fee provided
 * this copyright message remains intact.
 ************************************************************************/

#include <string.h>

#if defined(_WIN32) || defined(__WIN32__) || defined(__CYGWIN__)
#  if defined(_MSC_VER) || defined(__GNUC__)
#    if defined(STATIC_LINKED)
#      define SWIGEXPORT(a) a
#      define SWIGIMPORT(a) extern a
#    else
#      define SWIGEXPORT(a) __declspec(dllexport) a
#      define SWIGIMPORT(a) extern a
#    endif
#  else
#    if defined(__BORLANDC__)
#      define SWIGEXPORT(a) a _export
#      define SWIGIMPORT(a) a _export
#    else
#      define SWIGEXPORT(a) a
#      define SWIGIMPORT(a) a
#    endif
#  endif
#else
#  define SWIGEXPORT(a) a
#  define SWIGIMPORT(a) a
#endif

#ifdef SWIG_GLOBAL
#  define SWIGRUNTIME(a) SWIGEXPORT(a)
#else
#  define SWIGRUNTIME(a) static a
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef void *(*swig_converter_func)(void *);
typedef struct swig_type_info *(*swig_dycast_func)(void **);

typedef struct swig_type_info {
  const char             *name;
  swig_converter_func     converter;
  const char             *str;
  void                   *clientdata;
  swig_dycast_func        dcast;
  struct swig_type_info  *next;
  struct swig_type_info  *prev;
} swig_type_info;

#ifdef SWIG_NOINCLUDE

SWIGIMPORT(swig_type_info *) SWIG_TypeRegister(swig_type_info *);
SWIGIMPORT(swig_type_info *) SWIG_TypeCheck(char *c, swig_type_info *);
SWIGIMPORT(void *)           SWIG_TypeCast(swig_type_info *, void *);
SWIGIMPORT(swig_type_info *) SWIG_TypeDynamicCast(swig_type_info *, void **);
SWIGIMPORT(const char *)     SWIG_TypeName(const swig_type_info *);
SWIGIMPORT(swig_type_info *) SWIG_TypeQuery(const char *);
SWIGIMPORT(void)             SWIG_TypeClientData(swig_type_info *, void *);
SWIGIMPORT(char *)           SWIG_PackData(char *, void *, int);
SWIGIMPORT(char *)           SWIG_UnpackData(char *, void *, int);

#else

static swig_type_info *swig_type_list = 0;

/* Register a type mapping with the type-checking */
SWIGRUNTIME(swig_type_info *)
SWIG_TypeRegister(swig_type_info *ti) {
  swig_type_info *tc, *head, *ret, *next;
  /* Check to see if this type has already been registered */
  tc = swig_type_list;
  while (tc) {
    if (strcmp(tc->name, ti->name) == 0) {
      /* Already exists in the table.  Just add additional types to the list */
      if (tc->clientdata) ti->clientdata = tc->clientdata;
      head = tc;
      next = tc->next;
      goto l1;
    }
    tc = tc->prev;
  }
  head = ti;
  next = 0;

  /* Place in list */
  ti->prev = swig_type_list;
  swig_type_list = ti;

  /* Build linked lists */
  l1:
  ret = head;
  tc = ti + 1;
  /* Patch up the rest of the links */
  while (tc->name) {
    head->next = tc;
    tc->prev = head;
    head = tc;
    tc++;
  }
  if (next) next->prev = head;
  head->next = next;
  return ret;
}

/* Check the typename */
SWIGRUNTIME(swig_type_info *) 
SWIG_TypeCheck(char *c, swig_type_info *ty) {
  swig_type_info *s;
  if (!ty) return 0;        /* Void pointer */
  s = ty->next;             /* First element always just a name */
  do {
    if (strcmp(s->name,c) == 0) {
      if (s == ty->next) return s;
      /* Move s to the top of the linked list */
      s->prev->next = s->next;
      if (s->next) {
        s->next->prev = s->prev;
      }
      /* Insert s as second element in the list */
      s->next = ty->next;
      if (ty->next) ty->next->prev = s;
      ty->next = s;
      s->prev = ty;
      return s;
    }
    s = s->next;
  } while (s && (s != ty->next));
  return 0;
}

/* Cast a pointer up an inheritance hierarchy */
SWIGRUNTIME(void *) 
SWIG_TypeCast(swig_type_info *ty, void *ptr) {
  if ((!ty) || (!ty->converter)) return ptr;
  return (*ty->converter)(ptr);
}

/* Dynamic pointer casting. Down an inheritance hierarchy */
SWIGRUNTIME(swig_type_info *) 
SWIG_TypeDynamicCast(swig_type_info *ty, void **ptr) {
  swig_type_info *lastty = ty;
  if (!ty || !ty->dcast) return ty;
  while (ty && (ty->dcast)) {
    ty = (*ty->dcast)(ptr);
    if (ty) lastty = ty;
  }
  return lastty;
}

/* Return the name associated with this type */
SWIGRUNTIME(const char *)
SWIG_TypeName(const swig_type_info *ty) {
  return ty->name;
}

/* Search for a swig_type_info structure */
SWIGRUNTIME(swig_type_info *)
SWIG_TypeQuery(const char *name) {
  swig_type_info *ty = swig_type_list;
  while (ty) {
    if (ty->str && (strcmp(name,ty->str) == 0)) return ty;
    if (ty->name && (strcmp(name,ty->name) == 0)) return ty;
    ty = ty->prev;
  }
  return 0;
}

/* Set the clientdata field for a type */
SWIGRUNTIME(void)
SWIG_TypeClientData(swig_type_info *ti, void *clientdata) {
  swig_type_info *tc, *equiv;
  if (ti->clientdata == clientdata) return;
  ti->clientdata = clientdata;
  equiv = ti->next;
  while (equiv) {
    if (!equiv->converter) {
      tc = swig_type_list;
      while (tc) {
        if ((strcmp(tc->name, equiv->name) == 0))
          SWIG_TypeClientData(tc,clientdata);
        tc = tc->prev;
      }
    }
    equiv = equiv->next;
  }
}

/* Pack binary data into a string */
SWIGRUNTIME(char *)
SWIG_PackData(char *c, void *ptr, int sz) {
  static char hex[17] = "0123456789abcdef";
  int i;
  unsigned char *u = (unsigned char *) ptr;
  register unsigned char uu;
  for (i = 0; i < sz; i++,u++) {
    uu = *u;
    *(c++) = hex[(uu & 0xf0) >> 4];
    *(c++) = hex[uu & 0xf];
  }
  return c;
}

/* Unpack binary data from a string */
SWIGRUNTIME(char *)
SWIG_UnpackData(char *c, void *ptr, int sz) {
  register unsigned char uu = 0;
  register int d;
  unsigned char *u = (unsigned char *) ptr;
  int i;
  for (i = 0; i < sz; i++, u++) {
    d = *(c++);
    if ((d >= '0') && (d <= '9'))
      uu = ((d - '0') << 4);
    else if ((d >= 'a') && (d <= 'f'))
      uu = ((d - ('a'-10)) << 4);
    d = *(c++);
    if ((d >= '0') && (d <= '9'))
      uu |= (d - '0');
    else if ((d >= 'a') && (d <= 'f'))
      uu |= (d - ('a'-10));
    *u = uu;
  }
  return c;
}

#endif

#ifdef __cplusplus
}
#endif

/* rubydef.swg */
#ifdef __cplusplus
extern "C" {
#endif

static VALUE _mSWIG = Qnil;
static VALUE _cSWIG_Pointer = Qnil;

/* Initialize Ruby runtime support */
SWIGRUNTIME(void)
SWIG_Ruby_InitRuntime(void)
{
    if (_mSWIG == Qnil) {
        _mSWIG = rb_define_module("SWIG");
    }
}

/* Define Ruby class for C type */
SWIGRUNTIME(void)
SWIG_Ruby_define_class(swig_type_info *type)
{
    VALUE klass;
    char *klass_name = (char *) malloc(4 + strlen(type->name) + 1);
    sprintf(klass_name, "TYPE%s", type->name);
    if (NIL_P(_cSWIG_Pointer)) {
	_cSWIG_Pointer = rb_define_class_under(_mSWIG, "Pointer", rb_cObject);
	rb_undef_method(CLASS_OF(_cSWIG_Pointer), "new");
    }
    klass = rb_define_class_under(_mSWIG, klass_name, _cSWIG_Pointer);
    free((void *) klass_name);
}

/* Create a new pointer object */
SWIGRUNTIME(VALUE)
SWIG_Ruby_NewPointerObj(void *ptr, swig_type_info *type, int own)
{
    char *klass_name;
    swig_class *sklass;
    VALUE klass;
    VALUE obj;
    
    if (!ptr)
	return Qnil;
    
    if (type->clientdata) {
      sklass = (swig_class *) type->clientdata;
      obj = Data_Wrap_Struct(sklass->klass, VOIDFUNC(sklass->mark), (own ? VOIDFUNC(sklass->destroy) : 0), ptr);
    } else {
      klass_name = (char *) malloc(4 + strlen(type->name) + 1);
      sprintf(klass_name, "TYPE%s", type->name);
      klass = rb_const_get(_mSWIG, rb_intern(klass_name));
      free((void *) klass_name);
      obj = Data_Wrap_Struct(klass, 0, 0, ptr);
    }
    rb_iv_set(obj, "__swigtype__", rb_str_new2(type->name));
    return obj;
}

/* Create a new class instance (always owned) */
SWIGRUNTIME(VALUE)
SWIG_Ruby_NewClassInstance(VALUE klass, swig_type_info *type)
{
    VALUE obj;
    swig_class *sklass = (swig_class *) type->clientdata;
    obj = Data_Wrap_Struct(klass, VOIDFUNC(sklass->mark), VOIDFUNC(sklass->destroy), 0);
    rb_iv_set(obj, "__swigtype__", rb_str_new2(type->name));
    return obj;
}

/* Get type mangle from class name */
SWIGRUNTIME(char *)
SWIG_Ruby_MangleStr(VALUE obj)
{
  VALUE stype = rb_iv_get(obj, "__swigtype__");
  return StringValuePtr(stype);
}

/* Convert a pointer value */
SWIGRUNTIME(int)
SWIG_Ruby_ConvertPtr(VALUE obj, void **ptr, swig_type_info *ty, int flags)
{
  char *c;
  swig_type_info *tc;

  /* Grab the pointer */
  if (NIL_P(obj)) {
    *ptr = 0;
    return 0;
  } else {
    Data_Get_Struct(obj, void, *ptr);
  }
  
  /* Do type-checking if type info was provided */
  if (ty) {
    if (ty->clientdata) {
        if (rb_obj_is_kind_of(obj, ((swig_class *) (ty->clientdata))->klass)) {
          if (*ptr == 0)
            rb_raise(rb_eRuntimeError, "This %s already released", ty->str);
          return 0;
        }
    }
    if ((c = SWIG_MangleStr(obj)) == NULL) {
      if (flags & SWIG_POINTER_EXCEPTION)
        rb_raise(rb_eTypeError, "Expected %s", ty->str);
      else
        return -1;
    }
    tc = SWIG_TypeCheck(c, ty);
    if (!tc) {
      if (flags & SWIG_POINTER_EXCEPTION)
        rb_raise(rb_eTypeError, "Expected %s", ty->str);
      else
        return -1;
    }
    *ptr = SWIG_TypeCast(tc, *ptr);
  }
  return 0;
}

/* Convert a pointer value, signal an exception on a type mismatch */
SWIGRUNTIME(void *)
SWIG_Ruby_MustGetPtr(VALUE obj, swig_type_info *ty, int argnum, int flags)
{
  void *result;
  SWIG_ConvertPtr(obj, &result, ty, flags | SWIG_POINTER_EXCEPTION);
  return result;
}

/* Check convert */
SWIGRUNTIME(int)
SWIG_Ruby_CheckConvert(VALUE obj, swig_type_info *ty)
{
  char *c = SWIG_MangleStr(obj);
  if (!c)
    return 0;
  return SWIG_TypeCheck(c,ty) != 0;
}

SWIGRUNTIME(VALUE)
SWIG_Ruby_NewPackedObj(void *ptr, int sz, swig_type_info *type) {
  char result[1024];
  char *r = result;
  if ((2*sz + 1 + strlen(type->name)) > 1000) return 0;
  *(r++) = '_';
  r = SWIG_PackData(r, ptr, sz);
  strcpy(r, type->name);
  return rb_str_new2(result);
}

/* Convert a packed value value */
SWIGRUNTIME(void)
SWIG_Ruby_ConvertPacked(VALUE obj, void *ptr, int sz, swig_type_info *ty, int flags) {
  swig_type_info *tc;
  char  *c;

  if (TYPE(obj) != T_STRING) goto type_error;
  c = StringValuePtr(obj);
  /* Pointer values must start with leading underscore */
  if (*c != '_') goto type_error;
  c++;
  c = SWIG_UnpackData(c, ptr, sz);
  if (ty) {
    tc = SWIG_TypeCheck(c, ty);
    if (!tc) goto type_error;
  }
  return;

type_error:

  if (flags) {
    if (ty) {
      rb_raise(rb_eTypeError, "Type error. Expected %s", ty->name);
    } else {
      rb_raise(rb_eTypeError, "Expected a pointer");
    }
  }
}

#ifdef __cplusplus
}
#endif



/* -------- TYPES TABLE (BEGIN) -------- */

#define  SWIGTYPE_p_switch_channel_t swig_types[0] 
#define  SWIGTYPE_p_switch_file_handle_t swig_types[1] 
#define  SWIGTYPE_p_switch_core_session_t swig_types[2] 
#define  SWIGTYPE_p_p_switch_core_session_t swig_types[3] 
#define  SWIGTYPE_p_uint32_t swig_types[4] 
#define  SWIGTYPE_p_switch_input_callback_function_t swig_types[5] 
static swig_type_info *swig_types[7];

/* -------- TYPES TABLE (END) -------- */

#define SWIG_init    Init_freeswitch
#define SWIG_name    "Freeswitch"

static VALUE mFreeswitch;
extern void fs_core_set_globals(void);
extern int fs_core_init(char *);
extern int fs_core_destroy(void);
extern int fs_loadable_module_init(void);
extern int fs_loadable_module_shutdown(void);
extern int fs_console_loop(void);
extern void fs_consol_log(char *,char *);
extern void fs_consol_clean(char *);
extern switch_core_session_t *fs_core_session_locate(char *);
extern void fs_channel_answer(switch_core_session_t *);
extern void fs_channel_pre_answer(switch_core_session_t *);
extern void fs_channel_hangup(switch_core_session_t *,char *);
extern void fs_channel_set_variable(switch_core_session_t *,char *,char *);
extern void fs_channel_get_variable(switch_core_session_t *,char *);
extern void fs_channel_set_state(switch_core_session_t *,char *);
extern int fs_ivr_play_file(switch_core_session_t *,char *);
extern int fs_switch_ivr_record_file(switch_core_session_t *,switch_file_handle_t *,char *,switch_input_callback_function_t,void *,unsigned int,unsigned int);
extern int fs_switch_ivr_sleep(switch_core_session_t *,uint32_t);
extern int fs_ivr_play_file2(switch_core_session_t *,char *);
extern int fs_switch_ivr_collect_digits_callback(switch_core_session_t *,switch_input_callback_function_t,void *,unsigned int,unsigned int);
extern int fs_switch_ivr_collect_digits_count(switch_core_session_t *,char *,unsigned int,unsigned int,char const *,char *,unsigned int);
extern int fs_switch_ivr_originate(switch_core_session_t *,switch_core_session_t **,char *,uint32_t);
extern int fs_switch_ivr_session_transfer(switch_core_session_t *,char *,char *,char *);
extern int fs_switch_ivr_speak_text(switch_core_session_t *,char *,char *,uint32_t,char *);
extern char *fs_switch_channel_get_variable(switch_channel_t *,char *);
extern int fs_switch_channel_set_variable(switch_channel_t *,char *,char *);

#include "switch.h"

static VALUE
_wrap_fs_core_set_globals(int argc, VALUE *argv, VALUE self) {
    if ((argc < 0) || (argc > 0))
    rb_raise(rb_eArgError, "wrong # of arguments(%d for 0)",argc);
    fs_core_set_globals();
    
    return Qnil;
}


static VALUE
_wrap_fs_core_init(int argc, VALUE *argv, VALUE self) {
    char *arg1 ;
    int result;
    VALUE vresult = Qnil;
    
    if ((argc < 1) || (argc > 1))
    rb_raise(rb_eArgError, "wrong # of arguments(%d for 1)",argc);
    arg1 = StringValuePtr(argv[0]);
    result = (int)fs_core_init(arg1);
    
    vresult = INT2NUM(result);
    return vresult;
}


static VALUE
_wrap_fs_core_destroy(int argc, VALUE *argv, VALUE self) {
    int result;
    VALUE vresult = Qnil;
    
    if ((argc < 0) || (argc > 0))
    rb_raise(rb_eArgError, "wrong # of arguments(%d for 0)",argc);
    result = (int)fs_core_destroy();
    
    vresult = INT2NUM(result);
    return vresult;
}


static VALUE
_wrap_fs_loadable_module_init(int argc, VALUE *argv, VALUE self) {
    int result;
    VALUE vresult = Qnil;
    
    if ((argc < 0) || (argc > 0))
    rb_raise(rb_eArgError, "wrong # of arguments(%d for 0)",argc);
    result = (int)fs_loadable_module_init();
    
    vresult = INT2NUM(result);
    return vresult;
}


static VALUE
_wrap_fs_loadable_module_shutdown(int argc, VALUE *argv, VALUE self) {
    int result;
    VALUE vresult = Qnil;
    
    if ((argc < 0) || (argc > 0))
    rb_raise(rb_eArgError, "wrong # of arguments(%d for 0)",argc);
    result = (int)fs_loadable_module_shutdown();
    
    vresult = INT2NUM(result);
    return vresult;
}


static VALUE
_wrap_fs_console_loop(int argc, VALUE *argv, VALUE self) {
    int result;
    VALUE vresult = Qnil;
    
    if ((argc < 0) || (argc > 0))
    rb_raise(rb_eArgError, "wrong # of arguments(%d for 0)",argc);
    result = (int)fs_console_loop();
    
    vresult = INT2NUM(result);
    return vresult;
}


static VALUE
_wrap_fs_consol_log(int argc, VALUE *argv, VALUE self) {
    char *arg1 ;
    char *arg2 ;
    
    if ((argc < 2) || (argc > 2))
    rb_raise(rb_eArgError, "wrong # of arguments(%d for 2)",argc);
    arg1 = StringValuePtr(argv[0]);
    arg2 = StringValuePtr(argv[1]);
    fs_consol_log(arg1,arg2);
    
    return Qnil;
}


static VALUE
_wrap_fs_consol_clean(int argc, VALUE *argv, VALUE self) {
    char *arg1 ;
    
    if ((argc < 1) || (argc > 1))
    rb_raise(rb_eArgError, "wrong # of arguments(%d for 1)",argc);
    arg1 = StringValuePtr(argv[0]);
    fs_consol_clean(arg1);
    
    return Qnil;
}


static VALUE
_wrap_fs_core_session_locate(int argc, VALUE *argv, VALUE self) {
    char *arg1 ;
    switch_core_session_t *result;
    VALUE vresult = Qnil;
    
    if ((argc < 1) || (argc > 1))
    rb_raise(rb_eArgError, "wrong # of arguments(%d for 1)",argc);
    arg1 = StringValuePtr(argv[0]);
    result = (switch_core_session_t *)fs_core_session_locate(arg1);
    
    vresult = SWIG_NewPointerObj((void *) result, SWIGTYPE_p_switch_core_session_t,0);
    return vresult;
}


static VALUE
_wrap_fs_channel_answer(int argc, VALUE *argv, VALUE self) {
    switch_core_session_t *arg1 = (switch_core_session_t *) 0 ;
    
    if ((argc < 1) || (argc > 1))
    rb_raise(rb_eArgError, "wrong # of arguments(%d for 1)",argc);
    SWIG_ConvertPtr(argv[0], (void **) &arg1, SWIGTYPE_p_switch_core_session_t, 1);
    fs_channel_answer(arg1);
    
    return Qnil;
}


static VALUE
_wrap_fs_channel_pre_answer(int argc, VALUE *argv, VALUE self) {
    switch_core_session_t *arg1 = (switch_core_session_t *) 0 ;
    
    if ((argc < 1) || (argc > 1))
    rb_raise(rb_eArgError, "wrong # of arguments(%d for 1)",argc);
    SWIG_ConvertPtr(argv[0], (void **) &arg1, SWIGTYPE_p_switch_core_session_t, 1);
    fs_channel_pre_answer(arg1);
    
    return Qnil;
}


static VALUE
_wrap_fs_channel_hangup(int argc, VALUE *argv, VALUE self) {
    switch_core_session_t *arg1 = (switch_core_session_t *) 0 ;
    char *arg2 ;
    
    if ((argc < 2) || (argc > 2))
    rb_raise(rb_eArgError, "wrong # of arguments(%d for 2)",argc);
    SWIG_ConvertPtr(argv[0], (void **) &arg1, SWIGTYPE_p_switch_core_session_t, 1);
    arg2 = StringValuePtr(argv[1]);
    fs_channel_hangup(arg1,arg2);
    
    return Qnil;
}


static VALUE
_wrap_fs_channel_set_variable(int argc, VALUE *argv, VALUE self) {
    switch_core_session_t *arg1 = (switch_core_session_t *) 0 ;
    char *arg2 ;
    char *arg3 ;
    
    if ((argc < 3) || (argc > 3))
    rb_raise(rb_eArgError, "wrong # of arguments(%d for 3)",argc);
    SWIG_ConvertPtr(argv[0], (void **) &arg1, SWIGTYPE_p_switch_core_session_t, 1);
    arg2 = StringValuePtr(argv[1]);
    arg3 = StringValuePtr(argv[2]);
    fs_channel_set_variable(arg1,arg2,arg3);
    
    return Qnil;
}


static VALUE
_wrap_fs_channel_get_variable(int argc, VALUE *argv, VALUE self) {
    switch_core_session_t *arg1 = (switch_core_session_t *) 0 ;
    char *arg2 ;
    
    if ((argc < 2) || (argc > 2))
    rb_raise(rb_eArgError, "wrong # of arguments(%d for 2)",argc);
    SWIG_ConvertPtr(argv[0], (void **) &arg1, SWIGTYPE_p_switch_core_session_t, 1);
    arg2 = StringValuePtr(argv[1]);
    fs_channel_get_variable(arg1,arg2);
    
    return Qnil;
}


static VALUE
_wrap_fs_channel_set_state(int argc, VALUE *argv, VALUE self) {
    switch_core_session_t *arg1 = (switch_core_session_t *) 0 ;
    char *arg2 ;
    
    if ((argc < 2) || (argc > 2))
    rb_raise(rb_eArgError, "wrong # of arguments(%d for 2)",argc);
    SWIG_ConvertPtr(argv[0], (void **) &arg1, SWIGTYPE_p_switch_core_session_t, 1);
    arg2 = StringValuePtr(argv[1]);
    fs_channel_set_state(arg1,arg2);
    
    return Qnil;
}


static VALUE
_wrap_fs_ivr_play_file(int argc, VALUE *argv, VALUE self) {
    switch_core_session_t *arg1 = (switch_core_session_t *) 0 ;
    char *arg2 ;
    int result;
    VALUE vresult = Qnil;
    
    if ((argc < 2) || (argc > 2))
    rb_raise(rb_eArgError, "wrong # of arguments(%d for 2)",argc);
    SWIG_ConvertPtr(argv[0], (void **) &arg1, SWIGTYPE_p_switch_core_session_t, 1);
    arg2 = StringValuePtr(argv[1]);
    result = (int)fs_ivr_play_file(arg1,arg2);
    
    vresult = INT2NUM(result);
    return vresult;
}


static VALUE
_wrap_fs_switch_ivr_record_file(int argc, VALUE *argv, VALUE self) {
    switch_core_session_t *arg1 = (switch_core_session_t *) 0 ;
    switch_file_handle_t *arg2 = (switch_file_handle_t *) 0 ;
    char *arg3 ;
    switch_input_callback_function_t arg4 ;
    void *arg5 = (void *) 0 ;
    unsigned int arg6 ;
    unsigned int arg7 ;
    int result;
    VALUE vresult = Qnil;
    
    if ((argc < 7) || (argc > 7))
    rb_raise(rb_eArgError, "wrong # of arguments(%d for 7)",argc);
    SWIG_ConvertPtr(argv[0], (void **) &arg1, SWIGTYPE_p_switch_core_session_t, 1);
    SWIG_ConvertPtr(argv[1], (void **) &arg2, SWIGTYPE_p_switch_file_handle_t, 1);
    arg3 = StringValuePtr(argv[2]);
    {
        switch_input_callback_function_t * ptr;
        SWIG_ConvertPtr(argv[3], (void **) &ptr, SWIGTYPE_p_switch_input_callback_function_t, 1);
        if (ptr) arg4 = *ptr;
    }
    SWIG_ConvertPtr(argv[4], (void **) &arg5, 0, 1);
    arg6 = NUM2UINT(argv[5]);
    arg7 = NUM2UINT(argv[6]);
    result = (int)fs_switch_ivr_record_file(arg1,arg2,arg3,arg4,arg5,arg6,arg7);
    
    vresult = INT2NUM(result);
    return vresult;
}


static VALUE
_wrap_fs_switch_ivr_sleep(int argc, VALUE *argv, VALUE self) {
    switch_core_session_t *arg1 = (switch_core_session_t *) 0 ;
    uint32_t arg2 ;
    int result;
    VALUE vresult = Qnil;
    
    if ((argc < 2) || (argc > 2))
    rb_raise(rb_eArgError, "wrong # of arguments(%d for 2)",argc);
    SWIG_ConvertPtr(argv[0], (void **) &arg1, SWIGTYPE_p_switch_core_session_t, 1);
    {
        uint32_t * ptr;
        SWIG_ConvertPtr(argv[1], (void **) &ptr, SWIGTYPE_p_uint32_t, 1);
        if (ptr) arg2 = *ptr;
    }
    result = (int)fs_switch_ivr_sleep(arg1,arg2);
    
    vresult = INT2NUM(result);
    return vresult;
}


static VALUE
_wrap_fs_ivr_play_file2(int argc, VALUE *argv, VALUE self) {
    switch_core_session_t *arg1 = (switch_core_session_t *) 0 ;
    char *arg2 ;
    int result;
    VALUE vresult = Qnil;
    
    if ((argc < 2) || (argc > 2))
    rb_raise(rb_eArgError, "wrong # of arguments(%d for 2)",argc);
    SWIG_ConvertPtr(argv[0], (void **) &arg1, SWIGTYPE_p_switch_core_session_t, 1);
    arg2 = StringValuePtr(argv[1]);
    result = (int)fs_ivr_play_file2(arg1,arg2);
    
    vresult = INT2NUM(result);
    return vresult;
}


static VALUE
_wrap_fs_switch_ivr_collect_digits_callback(int argc, VALUE *argv, VALUE self) {
    switch_core_session_t *arg1 = (switch_core_session_t *) 0 ;
    switch_input_callback_function_t arg2 ;
    void *arg3 = (void *) 0 ;
    unsigned int arg4 ;
    unsigned int arg5 ;
    int result;
    VALUE vresult = Qnil;
    
    if ((argc < 5) || (argc > 5))
    rb_raise(rb_eArgError, "wrong # of arguments(%d for 5)",argc);
    SWIG_ConvertPtr(argv[0], (void **) &arg1, SWIGTYPE_p_switch_core_session_t, 1);
    {
        switch_input_callback_function_t * ptr;
        SWIG_ConvertPtr(argv[1], (void **) &ptr, SWIGTYPE_p_switch_input_callback_function_t, 1);
        if (ptr) arg2 = *ptr;
    }
    SWIG_ConvertPtr(argv[2], (void **) &arg3, 0, 1);
    arg4 = NUM2UINT(argv[3]);
    arg5 = NUM2UINT(argv[4]);
    result = (int)fs_switch_ivr_collect_digits_callback(arg1,arg2,arg3,arg4,arg5);
    
    vresult = INT2NUM(result);
    return vresult;
}


static VALUE
_wrap_fs_switch_ivr_collect_digits_count(int argc, VALUE *argv, VALUE self) {
    switch_core_session_t *arg1 = (switch_core_session_t *) 0 ;
    char *arg2 ;
    unsigned int arg3 ;
    unsigned int arg4 ;
    char *arg5 ;
    char *arg6 ;
    unsigned int arg7 ;
    int result;
    VALUE vresult = Qnil;
    
    if ((argc < 7) || (argc > 7))
    rb_raise(rb_eArgError, "wrong # of arguments(%d for 7)",argc);
    SWIG_ConvertPtr(argv[0], (void **) &arg1, SWIGTYPE_p_switch_core_session_t, 1);
    arg2 = StringValuePtr(argv[1]);
    arg3 = NUM2UINT(argv[2]);
    arg4 = NUM2UINT(argv[3]);
    arg5 = StringValuePtr(argv[4]);
    arg6 = StringValuePtr(argv[5]);
    arg7 = NUM2UINT(argv[6]);
    result = (int)fs_switch_ivr_collect_digits_count(arg1,arg2,arg3,arg4,(char const *)arg5,arg6,arg7);
    
    vresult = INT2NUM(result);
    return vresult;
}


static VALUE
_wrap_fs_switch_ivr_originate(int argc, VALUE *argv, VALUE self) {
    switch_core_session_t *arg1 = (switch_core_session_t *) 0 ;
    switch_core_session_t **arg2 = (switch_core_session_t **) 0 ;
    char *arg3 ;
    uint32_t arg4 ;
    int result;
    VALUE vresult = Qnil;
    
    if ((argc < 4) || (argc > 4))
    rb_raise(rb_eArgError, "wrong # of arguments(%d for 4)",argc);
    SWIG_ConvertPtr(argv[0], (void **) &arg1, SWIGTYPE_p_switch_core_session_t, 1);
    SWIG_ConvertPtr(argv[1], (void **) &arg2, SWIGTYPE_p_p_switch_core_session_t, 1);
    arg3 = StringValuePtr(argv[2]);
    {
        uint32_t * ptr;
        SWIG_ConvertPtr(argv[3], (void **) &ptr, SWIGTYPE_p_uint32_t, 1);
        if (ptr) arg4 = *ptr;
    }
    result = (int)fs_switch_ivr_originate(arg1,arg2,arg3,arg4);
    
    vresult = INT2NUM(result);
    return vresult;
}


static VALUE
_wrap_fs_switch_ivr_session_transfer(int argc, VALUE *argv, VALUE self) {
    switch_core_session_t *arg1 = (switch_core_session_t *) 0 ;
    char *arg2 ;
    char *arg3 ;
    char *arg4 ;
    int result;
    VALUE vresult = Qnil;
    
    if ((argc < 4) || (argc > 4))
    rb_raise(rb_eArgError, "wrong # of arguments(%d for 4)",argc);
    SWIG_ConvertPtr(argv[0], (void **) &arg1, SWIGTYPE_p_switch_core_session_t, 1);
    arg2 = StringValuePtr(argv[1]);
    arg3 = StringValuePtr(argv[2]);
    arg4 = StringValuePtr(argv[3]);
    result = (int)fs_switch_ivr_session_transfer(arg1,arg2,arg3,arg4);
    
    vresult = INT2NUM(result);
    return vresult;
}


static VALUE
_wrap_fs_switch_ivr_speak_text(int argc, VALUE *argv, VALUE self) {
    switch_core_session_t *arg1 = (switch_core_session_t *) 0 ;
    char *arg2 ;
    char *arg3 ;
    uint32_t arg4 ;
    char *arg5 ;
    int result;
    VALUE vresult = Qnil;
    
    if ((argc < 5) || (argc > 5))
    rb_raise(rb_eArgError, "wrong # of arguments(%d for 5)",argc);
    SWIG_ConvertPtr(argv[0], (void **) &arg1, SWIGTYPE_p_switch_core_session_t, 1);
    arg2 = StringValuePtr(argv[1]);
    arg3 = StringValuePtr(argv[2]);
    {
        uint32_t * ptr;
        SWIG_ConvertPtr(argv[3], (void **) &ptr, SWIGTYPE_p_uint32_t, 1);
        if (ptr) arg4 = *ptr;
    }
    arg5 = StringValuePtr(argv[4]);
    result = (int)fs_switch_ivr_speak_text(arg1,arg2,arg3,arg4,arg5);
    
    vresult = INT2NUM(result);
    return vresult;
}


static VALUE
_wrap_fs_switch_channel_get_variable(int argc, VALUE *argv, VALUE self) {
    switch_channel_t *arg1 = (switch_channel_t *) 0 ;
    char *arg2 ;
    char *result;
    VALUE vresult = Qnil;
    
    if ((argc < 2) || (argc > 2))
    rb_raise(rb_eArgError, "wrong # of arguments(%d for 2)",argc);
    SWIG_ConvertPtr(argv[0], (void **) &arg1, SWIGTYPE_p_switch_channel_t, 1);
    arg2 = StringValuePtr(argv[1]);
    result = (char *)fs_switch_channel_get_variable(arg1,arg2);
    
    vresult = rb_str_new2(result);
    return vresult;
}


static VALUE
_wrap_fs_switch_channel_set_variable(int argc, VALUE *argv, VALUE self) {
    switch_channel_t *arg1 = (switch_channel_t *) 0 ;
    char *arg2 ;
    char *arg3 ;
    int result;
    VALUE vresult = Qnil;
    
    if ((argc < 3) || (argc > 3))
    rb_raise(rb_eArgError, "wrong # of arguments(%d for 3)",argc);
    SWIG_ConvertPtr(argv[0], (void **) &arg1, SWIGTYPE_p_switch_channel_t, 1);
    arg2 = StringValuePtr(argv[1]);
    arg3 = StringValuePtr(argv[2]);
    result = (int)fs_switch_channel_set_variable(arg1,arg2,arg3);
    
    vresult = INT2NUM(result);
    return vresult;
}



/* -------- TYPE CONVERSION AND EQUIVALENCE RULES (BEGIN) -------- */

static swig_type_info _swigt__p_switch_channel_t[] = {{"_p_switch_channel_t", 0, "switch_channel_t *", 0},{"_p_switch_channel_t"},{0}};
static swig_type_info _swigt__p_switch_file_handle_t[] = {{"_p_switch_file_handle_t", 0, "switch_file_handle_t *", 0},{"_p_switch_file_handle_t"},{0}};
static swig_type_info _swigt__p_switch_core_session_t[] = {{"_p_switch_core_session_t", 0, "switch_core_session_t *", 0},{"_p_switch_core_session_t"},{0}};
static swig_type_info _swigt__p_p_switch_core_session_t[] = {{"_p_p_switch_core_session_t", 0, "switch_core_session_t **", 0},{"_p_p_switch_core_session_t"},{0}};
static swig_type_info _swigt__p_uint32_t[] = {{"_p_uint32_t", 0, "uint32_t *", 0},{"_p_uint32_t"},{0}};
static swig_type_info _swigt__p_switch_input_callback_function_t[] = {{"_p_switch_input_callback_function_t", 0, "switch_input_callback_function_t *", 0},{"_p_switch_input_callback_function_t"},{0}};

static swig_type_info *swig_types_initial[] = {
_swigt__p_switch_channel_t, 
_swigt__p_switch_file_handle_t, 
_swigt__p_switch_core_session_t, 
_swigt__p_p_switch_core_session_t, 
_swigt__p_uint32_t, 
_swigt__p_switch_input_callback_function_t, 
0
};


/* -------- TYPE CONVERSION AND EQUIVALENCE RULES (END) -------- */


#ifdef __cplusplus
extern "C"
#endif
SWIGEXPORT(void) Init_freeswitch(void) {
    int i;
    
    SWIG_InitRuntime();
    mFreeswitch = rb_define_module("Freeswitch");
    
    for (i = 0; swig_types_initial[i]; i++) {
        swig_types[i] = SWIG_TypeRegister(swig_types_initial[i]);
        SWIG_define_class(swig_types[i]);
    }
    
    rb_define_module_function(mFreeswitch, "fs_core_set_globals", _wrap_fs_core_set_globals, -1);
    rb_define_module_function(mFreeswitch, "fs_core_init", _wrap_fs_core_init, -1);
    rb_define_module_function(mFreeswitch, "fs_core_destroy", _wrap_fs_core_destroy, -1);
    rb_define_module_function(mFreeswitch, "fs_loadable_module_init", _wrap_fs_loadable_module_init, -1);
    rb_define_module_function(mFreeswitch, "fs_loadable_module_shutdown", _wrap_fs_loadable_module_shutdown, -1);
    rb_define_module_function(mFreeswitch, "fs_console_loop", _wrap_fs_console_loop, -1);
    rb_define_module_function(mFreeswitch, "fs_consol_log", _wrap_fs_consol_log, -1);
    rb_define_module_function(mFreeswitch, "fs_consol_clean", _wrap_fs_consol_clean, -1);
    rb_define_module_function(mFreeswitch, "fs_core_session_locate", _wrap_fs_core_session_locate, -1);
    rb_define_module_function(mFreeswitch, "fs_channel_answer", _wrap_fs_channel_answer, -1);
    rb_define_module_function(mFreeswitch, "fs_channel_pre_answer", _wrap_fs_channel_pre_answer, -1);
    rb_define_module_function(mFreeswitch, "fs_channel_hangup", _wrap_fs_channel_hangup, -1);
    rb_define_module_function(mFreeswitch, "fs_channel_set_variable", _wrap_fs_channel_set_variable, -1);
    rb_define_module_function(mFreeswitch, "fs_channel_get_variable", _wrap_fs_channel_get_variable, -1);
    rb_define_module_function(mFreeswitch, "fs_channel_set_state", _wrap_fs_channel_set_state, -1);
    rb_define_module_function(mFreeswitch, "fs_ivr_play_file", _wrap_fs_ivr_play_file, -1);
    rb_define_module_function(mFreeswitch, "fs_switch_ivr_record_file", _wrap_fs_switch_ivr_record_file, -1);
    rb_define_module_function(mFreeswitch, "fs_switch_ivr_sleep", _wrap_fs_switch_ivr_sleep, -1);
    rb_define_module_function(mFreeswitch, "fs_ivr_play_file2", _wrap_fs_ivr_play_file2, -1);
    rb_define_module_function(mFreeswitch, "fs_switch_ivr_collect_digits_callback", _wrap_fs_switch_ivr_collect_digits_callback, -1);
    rb_define_module_function(mFreeswitch, "fs_switch_ivr_collect_digits_count", _wrap_fs_switch_ivr_collect_digits_count, -1);
    rb_define_module_function(mFreeswitch, "fs_switch_ivr_originate", _wrap_fs_switch_ivr_originate, -1);
    rb_define_module_function(mFreeswitch, "fs_switch_ivr_session_transfer", _wrap_fs_switch_ivr_session_transfer, -1);
    rb_define_module_function(mFreeswitch, "fs_switch_ivr_speak_text", _wrap_fs_switch_ivr_speak_text, -1);
    rb_define_module_function(mFreeswitch, "fs_switch_channel_get_variable", _wrap_fs_switch_channel_get_variable, -1);
    rb_define_module_function(mFreeswitch, "fs_switch_channel_set_variable", _wrap_fs_switch_channel_set_variable, -1);
}


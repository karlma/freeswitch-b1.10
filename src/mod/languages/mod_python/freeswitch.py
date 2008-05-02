# This file was automatically generated by SWIG (http://www.swig.org).
# Version 1.3.31
#
# Don't modify this file, modify the SWIG interface instead.
# This file is compatible with both classic and new-style classes.

import _freeswitch
import new
new_instancemethod = new.instancemethod
try:
    _swig_property = property
except NameError:
    pass # Python < 2.2 doesn't have 'property'.
def _swig_setattr_nondynamic(self,class_type,name,value,static=1):
    if (name == "thisown"): return self.this.own(value)
    if (name == "this"):
        if type(value).__name__ == 'PySwigObject':
            self.__dict__[name] = value
            return
    method = class_type.__swig_setmethods__.get(name,None)
    if method: return method(self,value)
    if (not static) or hasattr(self,name):
        self.__dict__[name] = value
    else:
        raise AttributeError("You cannot add attributes to %s" % self)

def _swig_setattr(self,class_type,name,value):
    return _swig_setattr_nondynamic(self,class_type,name,value,0)

def _swig_getattr(self,class_type,name):
    if (name == "thisown"): return self.this.own()
    method = class_type.__swig_getmethods__.get(name,None)
    if method: return method(self)
    raise AttributeError,name

def _swig_repr(self):
    try: strthis = "proxy of " + self.this.__repr__()
    except: strthis = ""
    return "<%s.%s; %s >" % (self.__class__.__module__, self.__class__.__name__, strthis,)

import types
try:
    _object = types.ObjectType
    _newclass = 1
except AttributeError:
    class _object : pass
    _newclass = 0
del types


consoleLog = _freeswitch.consoleLog
consoleCleanLog = _freeswitch.consoleCleanLog
class API(_object):
    __swig_setmethods__ = {}
    __setattr__ = lambda self, name, value: _swig_setattr(self, API, name, value)
    __swig_getmethods__ = {}
    __getattr__ = lambda self, name: _swig_getattr(self, API, name)
    __repr__ = _swig_repr
    def __init__(self, *args): 
        this = _freeswitch.new_API(*args)
        try: self.this.append(this)
        except: self.this = this
    __swig_destroy__ = _freeswitch.delete_API
    __del__ = lambda self : None;
    def execute(*args): return _freeswitch.API_execute(*args)
    def executeString(*args): return _freeswitch.API_executeString(*args)
API_swigregister = _freeswitch.API_swigregister
API_swigregister(API)

class input_callback_state_t(_object):
    __swig_setmethods__ = {}
    __setattr__ = lambda self, name, value: _swig_setattr(self, input_callback_state_t, name, value)
    __swig_getmethods__ = {}
    __getattr__ = lambda self, name: _swig_getattr(self, input_callback_state_t, name)
    __repr__ = _swig_repr
    __swig_setmethods__["function"] = _freeswitch.input_callback_state_t_function_set
    __swig_getmethods__["function"] = _freeswitch.input_callback_state_t_function_get
    if _newclass:function = _swig_property(_freeswitch.input_callback_state_t_function_get, _freeswitch.input_callback_state_t_function_set)
    __swig_setmethods__["threadState"] = _freeswitch.input_callback_state_t_threadState_set
    __swig_getmethods__["threadState"] = _freeswitch.input_callback_state_t_threadState_get
    if _newclass:threadState = _swig_property(_freeswitch.input_callback_state_t_threadState_get, _freeswitch.input_callback_state_t_threadState_set)
    __swig_setmethods__["extra"] = _freeswitch.input_callback_state_t_extra_set
    __swig_getmethods__["extra"] = _freeswitch.input_callback_state_t_extra_get
    if _newclass:extra = _swig_property(_freeswitch.input_callback_state_t_extra_get, _freeswitch.input_callback_state_t_extra_set)
    __swig_setmethods__["funcargs"] = _freeswitch.input_callback_state_t_funcargs_set
    __swig_getmethods__["funcargs"] = _freeswitch.input_callback_state_t_funcargs_get
    if _newclass:funcargs = _swig_property(_freeswitch.input_callback_state_t_funcargs_get, _freeswitch.input_callback_state_t_funcargs_set)
    def __init__(self, *args): 
        this = _freeswitch.new_input_callback_state_t(*args)
        try: self.this.append(this)
        except: self.this = this
    __swig_destroy__ = _freeswitch.delete_input_callback_state_t
    __del__ = lambda self : None;
input_callback_state_t_swigregister = _freeswitch.input_callback_state_t_swigregister
input_callback_state_t_swigregister(input_callback_state_t)

S_HUP = _freeswitch.S_HUP
S_FREE = _freeswitch.S_FREE
S_RDLOCK = _freeswitch.S_RDLOCK
class Stream(_object):
    __swig_setmethods__ = {}
    __setattr__ = lambda self, name, value: _swig_setattr(self, Stream, name, value)
    __swig_getmethods__ = {}
    __getattr__ = lambda self, name: _swig_getattr(self, Stream, name)
    __repr__ = _swig_repr
    def __init__(self, *args): 
        this = _freeswitch.new_Stream(*args)
        try: self.this.append(this)
        except: self.this = this
    __swig_destroy__ = _freeswitch.delete_Stream
    __del__ = lambda self : None;
    def write(*args): return _freeswitch.Stream_write(*args)
    def get_data(*args): return _freeswitch.Stream_get_data(*args)
Stream_swigregister = _freeswitch.Stream_swigregister
Stream_swigregister(Stream)

class Event(_object):
    __swig_setmethods__ = {}
    __setattr__ = lambda self, name, value: _swig_setattr(self, Event, name, value)
    __swig_getmethods__ = {}
    __getattr__ = lambda self, name: _swig_getattr(self, Event, name)
    __repr__ = _swig_repr
    __swig_setmethods__["event"] = _freeswitch.Event_event_set
    __swig_getmethods__["event"] = _freeswitch.Event_event_get
    if _newclass:event = _swig_property(_freeswitch.Event_event_get, _freeswitch.Event_event_set)
    __swig_setmethods__["serialized_string"] = _freeswitch.Event_serialized_string_set
    __swig_getmethods__["serialized_string"] = _freeswitch.Event_serialized_string_get
    if _newclass:serialized_string = _swig_property(_freeswitch.Event_serialized_string_get, _freeswitch.Event_serialized_string_set)
    __swig_setmethods__["mine"] = _freeswitch.Event_mine_set
    __swig_getmethods__["mine"] = _freeswitch.Event_mine_get
    if _newclass:mine = _swig_property(_freeswitch.Event_mine_get, _freeswitch.Event_mine_set)
    def __init__(self, *args): 
        this = _freeswitch.new_Event(*args)
        try: self.this.append(this)
        except: self.this = this
    __swig_destroy__ = _freeswitch.delete_Event
    __del__ = lambda self : None;
    def serialize(*args): return _freeswitch.Event_serialize(*args)
    def setPriority(*args): return _freeswitch.Event_setPriority(*args)
    def getHeader(*args): return _freeswitch.Event_getHeader(*args)
    def getBody(*args): return _freeswitch.Event_getBody(*args)
    def getType(*args): return _freeswitch.Event_getType(*args)
    def addBody(*args): return _freeswitch.Event_addBody(*args)
    def addHeader(*args): return _freeswitch.Event_addHeader(*args)
    def delHeader(*args): return _freeswitch.Event_delHeader(*args)
    def fire(*args): return _freeswitch.Event_fire(*args)
Event_swigregister = _freeswitch.Event_swigregister
Event_swigregister(Event)

class CoreSession(_object):
    __swig_setmethods__ = {}
    __setattr__ = lambda self, name, value: _swig_setattr(self, CoreSession, name, value)
    __swig_getmethods__ = {}
    __getattr__ = lambda self, name: _swig_getattr(self, CoreSession, name)
    def __init__(self): raise AttributeError, "No constructor defined"
    __repr__ = _swig_repr
    __swig_destroy__ = _freeswitch.delete_CoreSession
    __del__ = lambda self : None;
    __swig_setmethods__["session"] = _freeswitch.CoreSession_session_set
    __swig_getmethods__["session"] = _freeswitch.CoreSession_session_get
    if _newclass:session = _swig_property(_freeswitch.CoreSession_session_get, _freeswitch.CoreSession_session_set)
    __swig_setmethods__["channel"] = _freeswitch.CoreSession_channel_set
    __swig_getmethods__["channel"] = _freeswitch.CoreSession_channel_get
    if _newclass:channel = _swig_property(_freeswitch.CoreSession_channel_get, _freeswitch.CoreSession_channel_set)
    __swig_setmethods__["flags"] = _freeswitch.CoreSession_flags_set
    __swig_getmethods__["flags"] = _freeswitch.CoreSession_flags_get
    if _newclass:flags = _swig_property(_freeswitch.CoreSession_flags_get, _freeswitch.CoreSession_flags_set)
    __swig_setmethods__["allocated"] = _freeswitch.CoreSession_allocated_set
    __swig_getmethods__["allocated"] = _freeswitch.CoreSession_allocated_get
    if _newclass:allocated = _swig_property(_freeswitch.CoreSession_allocated_get, _freeswitch.CoreSession_allocated_set)
    __swig_setmethods__["cb_state"] = _freeswitch.CoreSession_cb_state_set
    __swig_getmethods__["cb_state"] = _freeswitch.CoreSession_cb_state_get
    if _newclass:cb_state = _swig_property(_freeswitch.CoreSession_cb_state_get, _freeswitch.CoreSession_cb_state_set)
    __swig_setmethods__["hook_state"] = _freeswitch.CoreSession_hook_state_set
    __swig_getmethods__["hook_state"] = _freeswitch.CoreSession_hook_state_get
    if _newclass:hook_state = _swig_property(_freeswitch.CoreSession_hook_state_get, _freeswitch.CoreSession_hook_state_set)
    def answer(*args): return _freeswitch.CoreSession_answer(*args)
    def preAnswer(*args): return _freeswitch.CoreSession_preAnswer(*args)
    def hangup(*args): return _freeswitch.CoreSession_hangup(*args)
    def setVariable(*args): return _freeswitch.CoreSession_setVariable(*args)
    def setPrivate(*args): return _freeswitch.CoreSession_setPrivate(*args)
    def getPrivate(*args): return _freeswitch.CoreSession_getPrivate(*args)
    def getVariable(*args): return _freeswitch.CoreSession_getVariable(*args)
    def recordFile(*args): return _freeswitch.CoreSession_recordFile(*args)
    def setCallerData(*args): return _freeswitch.CoreSession_setCallerData(*args)
    def originate(*args): return _freeswitch.CoreSession_originate(*args)
    def setDTMFCallback(*args): return _freeswitch.CoreSession_setDTMFCallback(*args)
    def speak(*args): return _freeswitch.CoreSession_speak(*args)
    def set_tts_parms(*args): return _freeswitch.CoreSession_set_tts_parms(*args)
    def collectDigits(*args): return _freeswitch.CoreSession_collectDigits(*args)
    def getDigits(*args): return _freeswitch.CoreSession_getDigits(*args)
    def transfer(*args): return _freeswitch.CoreSession_transfer(*args)
    def playAndGetDigits(*args): return _freeswitch.CoreSession_playAndGetDigits(*args)
    def streamFile(*args): return _freeswitch.CoreSession_streamFile(*args)
    def flushEvents(*args): return _freeswitch.CoreSession_flushEvents(*args)
    def flushDigits(*args): return _freeswitch.CoreSession_flushDigits(*args)
    def setAutoHangup(*args): return _freeswitch.CoreSession_setAutoHangup(*args)
    def setHangupHook(*args): return _freeswitch.CoreSession_setHangupHook(*args)
    def ready(*args): return _freeswitch.CoreSession_ready(*args)
    def execute(*args): return _freeswitch.CoreSession_execute(*args)
    def sendEvent(*args): return _freeswitch.CoreSession_sendEvent(*args)
    def begin_allow_threads(*args): return _freeswitch.CoreSession_begin_allow_threads(*args)
    def end_allow_threads(*args): return _freeswitch.CoreSession_end_allow_threads(*args)
    def get_uuid(*args): return _freeswitch.CoreSession_get_uuid(*args)
    def get_cb_args(*args): return _freeswitch.CoreSession_get_cb_args(*args)
    def check_hangup_hook(*args): return _freeswitch.CoreSession_check_hangup_hook(*args)
    def run_dtmf_callback(*args): return _freeswitch.CoreSession_run_dtmf_callback(*args)
CoreSession_swigregister = _freeswitch.CoreSession_swigregister
CoreSession_swigregister(CoreSession)

console_log = _freeswitch.console_log
console_clean_log = _freeswitch.console_clean_log
bridge = _freeswitch.bridge
hanguphook = _freeswitch.hanguphook
dtmf_callback = _freeswitch.dtmf_callback
S_SWAPPED_IN = _freeswitch.S_SWAPPED_IN
S_SWAPPED_OUT = _freeswitch.S_SWAPPED_OUT
class PySession(CoreSession):
    __swig_setmethods__ = {}
    for _s in [CoreSession]: __swig_setmethods__.update(getattr(_s,'__swig_setmethods__',{}))
    __setattr__ = lambda self, name, value: _swig_setattr(self, PySession, name, value)
    __swig_getmethods__ = {}
    for _s in [CoreSession]: __swig_getmethods__.update(getattr(_s,'__swig_getmethods__',{}))
    __getattr__ = lambda self, name: _swig_getattr(self, PySession, name)
    __repr__ = _swig_repr
    def __init__(self, *args): 
        this = _freeswitch.new_PySession(*args)
        try: self.this.append(this)
        except: self.this = this
    __swig_destroy__ = _freeswitch.delete_PySession
    __del__ = lambda self : None;
    def setDTMFCallback(*args): return _freeswitch.PySession_setDTMFCallback(*args)
    def setHangupHook(*args): return _freeswitch.PySession_setHangupHook(*args)
    def check_hangup_hook(*args): return _freeswitch.PySession_check_hangup_hook(*args)
    def hangup(*args): return _freeswitch.PySession_hangup(*args)
    def begin_allow_threads(*args): return _freeswitch.PySession_begin_allow_threads(*args)
    def end_allow_threads(*args): return _freeswitch.PySession_end_allow_threads(*args)
    def run_dtmf_callback(*args): return _freeswitch.PySession_run_dtmf_callback(*args)
PySession_swigregister = _freeswitch.PySession_swigregister
PySession_swigregister(PySession)




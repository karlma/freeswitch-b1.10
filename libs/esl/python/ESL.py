# This file was automatically generated by SWIG (http://www.swig.org).
# Version 1.3.35
#
# Don't modify this file, modify the SWIG interface instead.
# This file is compatible with both classic and new-style classes.

import _ESL
import new
new_instancemethod = new.instancemethod
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

class ESLevent:
    __swig_setmethods__ = {}
    __setattr__ = lambda self, name, value: _swig_setattr(self, ESLevent, name, value)
    __swig_getmethods__ = {}
    __getattr__ = lambda self, name: _swig_getattr(self, ESLevent, name)
    __repr__ = _swig_repr
    __swig_setmethods__["event"] = _ESL.ESLevent_event_set
    __swig_getmethods__["event"] = _ESL.ESLevent_event_get
    __swig_setmethods__["serialized_string"] = _ESL.ESLevent_serialized_string_set
    __swig_getmethods__["serialized_string"] = _ESL.ESLevent_serialized_string_get
    __swig_setmethods__["mine"] = _ESL.ESLevent_mine_set
    __swig_getmethods__["mine"] = _ESL.ESLevent_mine_get
    def __init__(self, *args): 
        this = apply(_ESL.new_ESLevent, args)
        try: self.this.append(this)
        except: self.this = this
    __swig_destroy__ = _ESL.delete_ESLevent
    __del__ = lambda self : None;
    def serialize(*args): return apply(_ESL.ESLevent_serialize, args)
    def setPriority(*args): return apply(_ESL.ESLevent_setPriority, args)
    def getHeader(*args): return apply(_ESL.ESLevent_getHeader, args)
    def getBody(*args): return apply(_ESL.ESLevent_getBody, args)
    def getType(*args): return apply(_ESL.ESLevent_getType, args)
    def addBody(*args): return apply(_ESL.ESLevent_addBody, args)
    def addHeader(*args): return apply(_ESL.ESLevent_addHeader, args)
    def delHeader(*args): return apply(_ESL.ESLevent_delHeader, args)
    def firstHeader(*args): return apply(_ESL.ESLevent_firstHeader, args)
    def nextHeader(*args): return apply(_ESL.ESLevent_nextHeader, args)
ESLevent_swigregister = _ESL.ESLevent_swigregister
ESLevent_swigregister(ESLevent)

class ESLconnection:
    __swig_setmethods__ = {}
    __setattr__ = lambda self, name, value: _swig_setattr(self, ESLconnection, name, value)
    __swig_getmethods__ = {}
    __getattr__ = lambda self, name: _swig_getattr(self, ESLconnection, name)
    __repr__ = _swig_repr
    def __init__(self, *args): 
        this = apply(_ESL.new_ESLconnection, args)
        try: self.this.append(this)
        except: self.this = this
    __swig_destroy__ = _ESL.delete_ESLconnection
    __del__ = lambda self : None;
    def connected(*args): return apply(_ESL.ESLconnection_connected, args)
    def getInfo(*args): return apply(_ESL.ESLconnection_getInfo, args)
    def send(*args): return apply(_ESL.ESLconnection_send, args)
    def sendRecv(*args): return apply(_ESL.ESLconnection_sendRecv, args)
    def api(*args): return apply(_ESL.ESLconnection_api, args)
    def bgapi(*args): return apply(_ESL.ESLconnection_bgapi, args)
    def sendEvent(*args): return apply(_ESL.ESLconnection_sendEvent, args)
    def recvEvent(*args): return apply(_ESL.ESLconnection_recvEvent, args)
    def recvEventTimed(*args): return apply(_ESL.ESLconnection_recvEventTimed, args)
    def filter(*args): return apply(_ESL.ESLconnection_filter, args)
    def events(*args): return apply(_ESL.ESLconnection_events, args)
    def execute(*args): return apply(_ESL.ESLconnection_execute, args)
    def setBlockingExecute(*args): return apply(_ESL.ESLconnection_setBlockingExecute, args)
    def setEventLock(*args): return apply(_ESL.ESLconnection_setEventLock, args)
    def disconnect(*args): return apply(_ESL.ESLconnection_disconnect, args)
ESLconnection_swigregister = _ESL.ESLconnection_swigregister
ESLconnection_swigregister(ESLconnection)

eslSetLogLevel = _ESL.eslSetLogLevel



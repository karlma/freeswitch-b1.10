<?php

/* ----------------------------------------------------------------------------
 * This file was automatically generated by SWIG (http://www.swig.org).
 * Version 2.0.12
 *
 * This file is not intended to be easily readable and contains a number of
 * coding conventions designed to improve portability and efficiency. Do not make
 * changes to this file unless you know what you are doing--modify the SWIG
 * interface file instead.
 * ----------------------------------------------------------------------------- */

// Try to load our extension if it's not already loaded.
if (!extension_loaded('ESL')) {
  if (strtolower(substr(PHP_OS, 0, 3)) === 'win') {
    if (!dl('php_ESL.dll')) return;
  } else {
    // PHP_SHLIB_SUFFIX gives 'dylib' on MacOS X but modules are 'so'.
    if (PHP_SHLIB_SUFFIX === 'dylib') {
      if (!dl('ESL.so')) return;
    } else {
      if (!dl('ESL.'.PHP_SHLIB_SUFFIX)) return;
    }
  }
}



abstract class ESL {
	static function eslSetLogLevel($level) {
		eslSetLogLevel($level);
	}
}

/* PHP Proxy Classes */
class ESLevent {
	public $_cPtr=null;
	protected $_pData=array();

	function __set($var,$value) {
		$func = 'ESLevent_'.$var.'_set';
		if (function_exists($func)) return call_user_func($func,$this->_cPtr,$value);
		if ($var === 'thisown') return swig_ESL_alter_newobject($this->_cPtr,$value);
		$this->_pData[$var] = $value;
	}

	function __isset($var) {
		if (function_exists('ESLevent_'.$var.'_set')) return true;
		if ($var === 'thisown') return true;
		return array_key_exists($var, $this->_pData);
	}

	function __get($var) {
		$func = 'ESLevent_'.$var.'_get';
		if (function_exists($func)) return call_user_func($func,$this->_cPtr);
		if ($var === 'thisown') return swig_ESL_get_newobject($this->_cPtr);
		return $this->_pData[$var];
	}

	function __construct($type_or_wrap_me_or_me,$subclass_name_or_free_me=null) {
		if (is_resource($type_or_wrap_me_or_me) && get_resource_type($type_or_wrap_me_or_me) === '_p_ESLevent') {
			$this->_cPtr=$type_or_wrap_me_or_me;
			return;
		}
		switch (func_num_args()) {
		case 1: $this->_cPtr=new_ESLevent($type_or_wrap_me_or_me); break;
		default: $this->_cPtr=new_ESLevent($type_or_wrap_me_or_me,$subclass_name_or_free_me);
		}
	}

	function serialize($format=null) {
		switch (func_num_args()) {
		case 0: $r=ESLevent_serialize($this->_cPtr); break;
		default: $r=ESLevent_serialize($this->_cPtr,$format);
		}
		return $r;
	}

	function setPriority($priority=null) {
		switch (func_num_args()) {
		case 0: $r=ESLevent_setPriority($this->_cPtr); break;
		default: $r=ESLevent_setPriority($this->_cPtr,$priority);
		}
		return $r;
	}

	function getHeader($header_name,$idx=-1) {
		return ESLevent_getHeader($this->_cPtr,$header_name,$idx);
	}

	function getBody() {
		return ESLevent_getBody($this->_cPtr);
	}

	function getType() {
		return ESLevent_getType($this->_cPtr);
	}

	function addBody($value) {
		return ESLevent_addBody($this->_cPtr,$value);
	}

	function addHeader($header_name,$value) {
		return ESLevent_addHeader($this->_cPtr,$header_name,$value);
	}

	function pushHeader($header_name,$value) {
		return ESLevent_pushHeader($this->_cPtr,$header_name,$value);
	}

	function unshiftHeader($header_name,$value) {
		return ESLevent_unshiftHeader($this->_cPtr,$header_name,$value);
	}

	function delHeader($header_name) {
		return ESLevent_delHeader($this->_cPtr,$header_name);
	}

	function firstHeader() {
		return ESLevent_firstHeader($this->_cPtr);
	}

	function nextHeader() {
		return ESLevent_nextHeader($this->_cPtr);
	}
}

class ESLconnection {
	public $_cPtr=null;
	protected $_pData=array();

	function __set($var,$value) {
		if ($var === 'thisown') return swig_ESL_alter_newobject($this->_cPtr,$value);
		$this->_pData[$var] = $value;
	}

	function __isset($var) {
		if ($var === 'thisown') return true;
		return array_key_exists($var, $this->_pData);
	}

	function __get($var) {
		if ($var === 'thisown') return swig_ESL_get_newobject($this->_cPtr);
		return $this->_pData[$var];
	}

	function __construct($host_or_socket,$port=null,$user_or_password=null,$password=null) {
		if (is_resource($host_or_socket) && get_resource_type($host_or_socket) === '_p_ESLconnection') {
			$this->_cPtr=$host_or_socket;
			return;
		}
		switch (func_num_args()) {
		case 1: $this->_cPtr=new_ESLconnection($host_or_socket); break;
		case 2: $this->_cPtr=new_ESLconnection($host_or_socket,$port); break;
		case 3: $this->_cPtr=new_ESLconnection($host_or_socket,$port,$user_or_password); break;
		default: $this->_cPtr=new_ESLconnection($host_or_socket,$port,$user_or_password,$password);
		}
	}

	function socketDescriptor() {
		return ESLconnection_socketDescriptor($this->_cPtr);
	}

	function connected() {
		return ESLconnection_connected($this->_cPtr);
	}

	function getInfo() {
		$r=ESLconnection_getInfo($this->_cPtr);
		if (is_resource($r)) {
			$c=substr(get_resource_type($r), (strpos(get_resource_type($r), '__') ? strpos(get_resource_type($r), '__') + 2 : 3));
			if (class_exists($c)) return new $c($r);
			return new ESLevent($r);
		}
		return $r;
	}

	function send($cmd) {
		return ESLconnection_send($this->_cPtr,$cmd);
	}

	function sendRecv($cmd) {
		$r=ESLconnection_sendRecv($this->_cPtr,$cmd);
		if (is_resource($r)) {
			$c=substr(get_resource_type($r), (strpos(get_resource_type($r), '__') ? strpos(get_resource_type($r), '__') + 2 : 3));
			if (class_exists($c)) return new $c($r);
			return new ESLevent($r);
		}
		return $r;
	}

	function api($cmd,$arg=null) {
		switch (func_num_args()) {
		case 1: $r=ESLconnection_api($this->_cPtr,$cmd); break;
		default: $r=ESLconnection_api($this->_cPtr,$cmd,$arg);
		}
		if (is_resource($r)) {
			$c=substr(get_resource_type($r), (strpos(get_resource_type($r), '__') ? strpos(get_resource_type($r), '__') + 2 : 3));
			if (class_exists($c)) return new $c($r);
			return new ESLevent($r);
		}
		return $r;
	}

	function bgapi($cmd,$arg=null,$job_uuid=null) {
		switch (func_num_args()) {
		case 1: $r=ESLconnection_bgapi($this->_cPtr,$cmd); break;
		case 2: $r=ESLconnection_bgapi($this->_cPtr,$cmd,$arg); break;
		default: $r=ESLconnection_bgapi($this->_cPtr,$cmd,$arg,$job_uuid);
		}
		if (is_resource($r)) {
			$c=substr(get_resource_type($r), (strpos(get_resource_type($r), '__') ? strpos(get_resource_type($r), '__') + 2 : 3));
			if (class_exists($c)) return new $c($r);
			return new ESLevent($r);
		}
		return $r;
	}

	function sendEvent($send_me) {
		$r=ESLconnection_sendEvent($this->_cPtr,$send_me);
		if (is_resource($r)) {
			$c=substr(get_resource_type($r), (strpos(get_resource_type($r), '__') ? strpos(get_resource_type($r), '__') + 2 : 3));
			if (class_exists($c)) return new $c($r);
			return new ESLevent($r);
		}
		return $r;
	}

	function sendMSG($send_me,$uuid=null) {
		switch (func_num_args()) {
		case 1: $r=ESLconnection_sendMSG($this->_cPtr,$send_me); break;
		default: $r=ESLconnection_sendMSG($this->_cPtr,$send_me,$uuid);
		}
		return $r;
	}

	function recvEvent() {
		$r=ESLconnection_recvEvent($this->_cPtr);
		if (is_resource($r)) {
			$c=substr(get_resource_type($r), (strpos(get_resource_type($r), '__') ? strpos(get_resource_type($r), '__') + 2 : 3));
			if (class_exists($c)) return new $c($r);
			return new ESLevent($r);
		}
		return $r;
	}

	function recvEventTimed($ms) {
		$r=ESLconnection_recvEventTimed($this->_cPtr,$ms);
		if (is_resource($r)) {
			$c=substr(get_resource_type($r), (strpos(get_resource_type($r), '__') ? strpos(get_resource_type($r), '__') + 2 : 3));
			if (class_exists($c)) return new $c($r);
			return new ESLevent($r);
		}
		return $r;
	}

	function filter($header,$value) {
		$r=ESLconnection_filter($this->_cPtr,$header,$value);
		if (is_resource($r)) {
			$c=substr(get_resource_type($r), (strpos(get_resource_type($r), '__') ? strpos(get_resource_type($r), '__') + 2 : 3));
			if (class_exists($c)) return new $c($r);
			return new ESLevent($r);
		}
		return $r;
	}

	function events($etype,$value) {
		return ESLconnection_events($this->_cPtr,$etype,$value);
	}

	function execute($app,$arg=null,$uuid=null) {
		switch (func_num_args()) {
		case 1: $r=ESLconnection_execute($this->_cPtr,$app); break;
		case 2: $r=ESLconnection_execute($this->_cPtr,$app,$arg); break;
		default: $r=ESLconnection_execute($this->_cPtr,$app,$arg,$uuid);
		}
		if (is_resource($r)) {
			$c=substr(get_resource_type($r), (strpos(get_resource_type($r), '__') ? strpos(get_resource_type($r), '__') + 2 : 3));
			if (class_exists($c)) return new $c($r);
			return new ESLevent($r);
		}
		return $r;
	}

	function executeAsync($app,$arg=null,$uuid=null) {
		switch (func_num_args()) {
		case 1: $r=ESLconnection_executeAsync($this->_cPtr,$app); break;
		case 2: $r=ESLconnection_executeAsync($this->_cPtr,$app,$arg); break;
		default: $r=ESLconnection_executeAsync($this->_cPtr,$app,$arg,$uuid);
		}
		if (is_resource($r)) {
			$c=substr(get_resource_type($r), (strpos(get_resource_type($r), '__') ? strpos(get_resource_type($r), '__') + 2 : 3));
			if (class_exists($c)) return new $c($r);
			return new ESLevent($r);
		}
		return $r;
	}

	function setAsyncExecute($val) {
		return ESLconnection_setAsyncExecute($this->_cPtr,$val);
	}

	function setEventLock($val) {
		return ESLconnection_setEventLock($this->_cPtr,$val);
	}

	function disconnect() {
		return ESLconnection_disconnect($this->_cPtr);
	}
}


?>

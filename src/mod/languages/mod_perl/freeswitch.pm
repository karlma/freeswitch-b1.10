# This file was automatically generated by SWIG (http://www.swig.org).
# Version 3.0.10
#
# Do not make changes to this file unless you know what you are doing--modify
# the SWIG interface file instead.

package freeswitch;
use base qw(Exporter);
package freeswitchc;
package freeswitchc;
boot_freeswitch();
package freeswitch;
@EXPORT = qw();

# ---------- BASE METHODS -------------

package freeswitch;

sub TIEHASH {
    my ($classname,$obj) = @_;
    return bless $obj, $classname;
}

sub CLEAR { }

sub FIRSTKEY { }

sub NEXTKEY { }

sub FETCH {
    my ($self,$field) = @_;
    my $member_func = "swig_${field}_get";
    $self->$member_func();
}

sub STORE {
    my ($self,$field,$newval) = @_;
    my $member_func = "swig_${field}_set";
    $self->$member_func($newval);
}

sub this {
    my $ptr = shift;
    return tied(%$ptr);
}


# ------- FUNCTION WRAPPERS --------

package freeswitch;

*setGlobalVariable = *freeswitchc::setGlobalVariable;
*getGlobalVariable = *freeswitchc::getGlobalVariable;
*consoleLog = *freeswitchc::consoleLog;
*consoleLog2 = *freeswitchc::consoleLog2;
*consoleCleanLog = *freeswitchc::consoleCleanLog;
*running = *freeswitchc::running;
*email = *freeswitchc::email;
*console_log = *freeswitchc::console_log;
*console_log2 = *freeswitchc::console_log2;
*console_clean_log = *freeswitchc::console_clean_log;
*msleep = *freeswitchc::msleep;
*bridge = *freeswitchc::bridge;
*hanguphook = *freeswitchc::hanguphook;
*dtmf_callback = *freeswitchc::dtmf_callback;

############# Class : freeswitch::IVRMenu ##############

package freeswitch::IVRMenu;
use vars qw(@ISA %OWNER %ITERATORS %BLESSEDMEMBERS);
@ISA = qw( freeswitch );
%OWNER = ();
%ITERATORS = ();
sub new {
    my $pkg = shift;
    my $self = freeswitchc::new_IVRMenu(@_);
    bless $self, $pkg if defined($self);
}

sub DESTROY {
    return unless $_[0]->isa('HASH');
    my $self = tied(%{$_[0]});
    return unless defined $self;
    delete $ITERATORS{$self};
    if (exists $OWNER{$self}) {
        freeswitchc::delete_IVRMenu($self);
        delete $OWNER{$self};
    }
}

*bindAction = *freeswitchc::IVRMenu_bindAction;
*execute = *freeswitchc::IVRMenu_execute;
sub DISOWN {
    my $self = shift;
    my $ptr = tied(%$self);
    delete $OWNER{$ptr};
}

sub ACQUIRE {
    my $self = shift;
    my $ptr = tied(%$self);
    $OWNER{$ptr} = 1;
}


############# Class : freeswitch::API ##############

package freeswitch::API;
use vars qw(@ISA %OWNER %ITERATORS %BLESSEDMEMBERS);
@ISA = qw( freeswitch );
%OWNER = ();
%ITERATORS = ();
sub new {
    my $pkg = shift;
    my $self = freeswitchc::new_API(@_);
    bless $self, $pkg if defined($self);
}

sub DESTROY {
    return unless $_[0]->isa('HASH');
    my $self = tied(%{$_[0]});
    return unless defined $self;
    delete $ITERATORS{$self};
    if (exists $OWNER{$self}) {
        freeswitchc::delete_API($self);
        delete $OWNER{$self};
    }
}

*execute = *freeswitchc::API_execute;
*executeString = *freeswitchc::API_executeString;
*getTime = *freeswitchc::API_getTime;
sub DISOWN {
    my $self = shift;
    my $ptr = tied(%$self);
    delete $OWNER{$ptr};
}

sub ACQUIRE {
    my $self = shift;
    my $ptr = tied(%$self);
    $OWNER{$ptr} = 1;
}


############# Class : freeswitch::input_callback_state_t ##############

package freeswitch::input_callback_state_t;
use vars qw(@ISA %OWNER %ITERATORS %BLESSEDMEMBERS);
@ISA = qw( freeswitch );
%OWNER = ();
%ITERATORS = ();
*swig_function_get = *freeswitchc::input_callback_state_t_function_get;
*swig_function_set = *freeswitchc::input_callback_state_t_function_set;
*swig_threadState_get = *freeswitchc::input_callback_state_t_threadState_get;
*swig_threadState_set = *freeswitchc::input_callback_state_t_threadState_set;
*swig_extra_get = *freeswitchc::input_callback_state_t_extra_get;
*swig_extra_set = *freeswitchc::input_callback_state_t_extra_set;
*swig_funcargs_get = *freeswitchc::input_callback_state_t_funcargs_get;
*swig_funcargs_set = *freeswitchc::input_callback_state_t_funcargs_set;
sub new {
    my $pkg = shift;
    my $self = freeswitchc::new_input_callback_state_t(@_);
    bless $self, $pkg if defined($self);
}

sub DESTROY {
    return unless $_[0]->isa('HASH');
    my $self = tied(%{$_[0]});
    return unless defined $self;
    delete $ITERATORS{$self};
    if (exists $OWNER{$self}) {
        freeswitchc::delete_input_callback_state_t($self);
        delete $OWNER{$self};
    }
}

sub DISOWN {
    my $self = shift;
    my $ptr = tied(%$self);
    delete $OWNER{$ptr};
}

sub ACQUIRE {
    my $self = shift;
    my $ptr = tied(%$self);
    $OWNER{$ptr} = 1;
}


############# Class : freeswitch::DTMF ##############

package freeswitch::DTMF;
use vars qw(@ISA %OWNER %ITERATORS %BLESSEDMEMBERS);
@ISA = qw( freeswitch );
%OWNER = ();
%ITERATORS = ();
*swig_digit_get = *freeswitchc::DTMF_digit_get;
*swig_digit_set = *freeswitchc::DTMF_digit_set;
*swig_duration_get = *freeswitchc::DTMF_duration_get;
*swig_duration_set = *freeswitchc::DTMF_duration_set;
sub new {
    my $pkg = shift;
    my $self = freeswitchc::new_DTMF(@_);
    bless $self, $pkg if defined($self);
}

sub DESTROY {
    return unless $_[0]->isa('HASH');
    my $self = tied(%{$_[0]});
    return unless defined $self;
    delete $ITERATORS{$self};
    if (exists $OWNER{$self}) {
        freeswitchc::delete_DTMF($self);
        delete $OWNER{$self};
    }
}

sub DISOWN {
    my $self = shift;
    my $ptr = tied(%$self);
    delete $OWNER{$ptr};
}

sub ACQUIRE {
    my $self = shift;
    my $ptr = tied(%$self);
    $OWNER{$ptr} = 1;
}


############# Class : freeswitch::Stream ##############

package freeswitch::Stream;
use vars qw(@ISA %OWNER %ITERATORS %BLESSEDMEMBERS);
@ISA = qw( freeswitch );
%OWNER = ();
%ITERATORS = ();
sub new {
    my $pkg = shift;
    my $self = freeswitchc::new_Stream(@_);
    bless $self, $pkg if defined($self);
}

sub DESTROY {
    return unless $_[0]->isa('HASH');
    my $self = tied(%{$_[0]});
    return unless defined $self;
    delete $ITERATORS{$self};
    if (exists $OWNER{$self}) {
        freeswitchc::delete_Stream($self);
        delete $OWNER{$self};
    }
}

*read = *freeswitchc::Stream_read;
*write = *freeswitchc::Stream_write;
*raw_write = *freeswitchc::Stream_raw_write;
*get_data = *freeswitchc::Stream_get_data;
sub DISOWN {
    my $self = shift;
    my $ptr = tied(%$self);
    delete $OWNER{$ptr};
}

sub ACQUIRE {
    my $self = shift;
    my $ptr = tied(%$self);
    $OWNER{$ptr} = 1;
}


############# Class : freeswitch::Event ##############

package freeswitch::Event;
use vars qw(@ISA %OWNER %ITERATORS %BLESSEDMEMBERS);
@ISA = qw( freeswitch );
%OWNER = ();
%ITERATORS = ();
*swig_event_get = *freeswitchc::Event_event_get;
*swig_event_set = *freeswitchc::Event_event_set;
*swig_serialized_string_get = *freeswitchc::Event_serialized_string_get;
*swig_serialized_string_set = *freeswitchc::Event_serialized_string_set;
*swig_mine_get = *freeswitchc::Event_mine_get;
*swig_mine_set = *freeswitchc::Event_mine_set;
sub new {
    my $pkg = shift;
    my $self = freeswitchc::new_Event(@_);
    bless $self, $pkg if defined($self);
}

sub DESTROY {
    return unless $_[0]->isa('HASH');
    my $self = tied(%{$_[0]});
    return unless defined $self;
    delete $ITERATORS{$self};
    if (exists $OWNER{$self}) {
        freeswitchc::delete_Event($self);
        delete $OWNER{$self};
    }
}

*chat_execute = *freeswitchc::Event_chat_execute;
*chat_send = *freeswitchc::Event_chat_send;
*serialize = *freeswitchc::Event_serialize;
*setPriority = *freeswitchc::Event_setPriority;
*getHeader = *freeswitchc::Event_getHeader;
*getBody = *freeswitchc::Event_getBody;
*getType = *freeswitchc::Event_getType;
*addBody = *freeswitchc::Event_addBody;
*addHeader = *freeswitchc::Event_addHeader;
*delHeader = *freeswitchc::Event_delHeader;
*fire = *freeswitchc::Event_fire;
sub DISOWN {
    my $self = shift;
    my $ptr = tied(%$self);
    delete $OWNER{$ptr};
}

sub ACQUIRE {
    my $self = shift;
    my $ptr = tied(%$self);
    $OWNER{$ptr} = 1;
}


############# Class : freeswitch::EventConsumer ##############

package freeswitch::EventConsumer;
use vars qw(@ISA %OWNER %ITERATORS %BLESSEDMEMBERS);
@ISA = qw( freeswitch );
%OWNER = ();
%ITERATORS = ();
*swig_events_get = *freeswitchc::EventConsumer_events_get;
*swig_events_set = *freeswitchc::EventConsumer_events_set;
*swig_e_event_id_get = *freeswitchc::EventConsumer_e_event_id_get;
*swig_e_event_id_set = *freeswitchc::EventConsumer_e_event_id_set;
*swig_e_callback_get = *freeswitchc::EventConsumer_e_callback_get;
*swig_e_callback_set = *freeswitchc::EventConsumer_e_callback_set;
*swig_e_subclass_name_get = *freeswitchc::EventConsumer_e_subclass_name_get;
*swig_e_subclass_name_set = *freeswitchc::EventConsumer_e_subclass_name_set;
*swig_e_cb_arg_get = *freeswitchc::EventConsumer_e_cb_arg_get;
*swig_e_cb_arg_set = *freeswitchc::EventConsumer_e_cb_arg_set;
*swig_enodes_get = *freeswitchc::EventConsumer_enodes_get;
*swig_enodes_set = *freeswitchc::EventConsumer_enodes_set;
*swig_node_index_get = *freeswitchc::EventConsumer_node_index_get;
*swig_node_index_set = *freeswitchc::EventConsumer_node_index_set;
sub new {
    my $pkg = shift;
    my $self = freeswitchc::new_EventConsumer(@_);
    bless $self, $pkg if defined($self);
}

sub DESTROY {
    return unless $_[0]->isa('HASH');
    my $self = tied(%{$_[0]});
    return unless defined $self;
    delete $ITERATORS{$self};
    if (exists $OWNER{$self}) {
        freeswitchc::delete_EventConsumer($self);
        delete $OWNER{$self};
    }
}

*bind = *freeswitchc::EventConsumer_bind;
*pop = *freeswitchc::EventConsumer_pop;
*cleanup = *freeswitchc::EventConsumer_cleanup;
sub DISOWN {
    my $self = shift;
    my $ptr = tied(%$self);
    delete $OWNER{$ptr};
}

sub ACQUIRE {
    my $self = shift;
    my $ptr = tied(%$self);
    $OWNER{$ptr} = 1;
}


############# Class : freeswitch::CoreSession ##############

package freeswitch::CoreSession;
use vars qw(@ISA %OWNER %ITERATORS %BLESSEDMEMBERS);
@ISA = qw( freeswitch );
%OWNER = ();
%ITERATORS = ();
sub DESTROY {
    return unless $_[0]->isa('HASH');
    my $self = tied(%{$_[0]});
    return unless defined $self;
    delete $ITERATORS{$self};
    if (exists $OWNER{$self}) {
        freeswitchc::delete_CoreSession($self);
        delete $OWNER{$self};
    }
}

*swig_session_get = *freeswitchc::CoreSession_session_get;
*swig_session_set = *freeswitchc::CoreSession_session_set;
*swig_channel_get = *freeswitchc::CoreSession_channel_get;
*swig_channel_set = *freeswitchc::CoreSession_channel_set;
*swig_flags_get = *freeswitchc::CoreSession_flags_get;
*swig_flags_set = *freeswitchc::CoreSession_flags_set;
*swig_allocated_get = *freeswitchc::CoreSession_allocated_get;
*swig_allocated_set = *freeswitchc::CoreSession_allocated_set;
*swig_cb_state_get = *freeswitchc::CoreSession_cb_state_get;
*swig_cb_state_set = *freeswitchc::CoreSession_cb_state_set;
*swig_hook_state_get = *freeswitchc::CoreSession_hook_state_get;
*swig_hook_state_set = *freeswitchc::CoreSession_hook_state_set;
*swig_cause_get = *freeswitchc::CoreSession_cause_get;
*swig_cause_set = *freeswitchc::CoreSession_cause_set;
*swig_uuid_get = *freeswitchc::CoreSession_uuid_get;
*swig_uuid_set = *freeswitchc::CoreSession_uuid_set;
*swig_tts_name_get = *freeswitchc::CoreSession_tts_name_get;
*swig_tts_name_set = *freeswitchc::CoreSession_tts_name_set;
*swig_voice_name_get = *freeswitchc::CoreSession_voice_name_get;
*swig_voice_name_set = *freeswitchc::CoreSession_voice_name_set;
*insertFile = *freeswitchc::CoreSession_insertFile;
*answer = *freeswitchc::CoreSession_answer;
*print = *freeswitchc::CoreSession_print;
*preAnswer = *freeswitchc::CoreSession_preAnswer;
*hangup = *freeswitchc::CoreSession_hangup;
*hangupState = *freeswitchc::CoreSession_hangupState;
*setVariable = *freeswitchc::CoreSession_setVariable;
*setPrivate = *freeswitchc::CoreSession_setPrivate;
*getPrivate = *freeswitchc::CoreSession_getPrivate;
*getVariable = *freeswitchc::CoreSession_getVariable;
*process_callback_result = *freeswitchc::CoreSession_process_callback_result;
*say = *freeswitchc::CoreSession_say;
*sayPhrase = *freeswitchc::CoreSession_sayPhrase;
*hangupCause = *freeswitchc::CoreSession_hangupCause;
*getState = *freeswitchc::CoreSession_getState;
*recordFile = *freeswitchc::CoreSession_recordFile;
*originate = *freeswitchc::CoreSession_originate;
*destroy = *freeswitchc::CoreSession_destroy;
*setDTMFCallback = *freeswitchc::CoreSession_setDTMFCallback;
*speak = *freeswitchc::CoreSession_speak;
*set_tts_parms = *freeswitchc::CoreSession_set_tts_parms;
*set_tts_params = *freeswitchc::CoreSession_set_tts_params;
*collectDigits = *freeswitchc::CoreSession_collectDigits;
*getDigits = *freeswitchc::CoreSession_getDigits;
*transfer = *freeswitchc::CoreSession_transfer;
*read = *freeswitchc::CoreSession_read;
*playAndGetDigits = *freeswitchc::CoreSession_playAndGetDigits;
*streamFile = *freeswitchc::CoreSession_streamFile;
*sleep = *freeswitchc::CoreSession_sleep;
*flushEvents = *freeswitchc::CoreSession_flushEvents;
*flushDigits = *freeswitchc::CoreSession_flushDigits;
*setAutoHangup = *freeswitchc::CoreSession_setAutoHangup;
*setHangupHook = *freeswitchc::CoreSession_setHangupHook;
*ready = *freeswitchc::CoreSession_ready;
*bridged = *freeswitchc::CoreSession_bridged;
*answered = *freeswitchc::CoreSession_answered;
*mediaReady = *freeswitchc::CoreSession_mediaReady;
*waitForAnswer = *freeswitchc::CoreSession_waitForAnswer;
*execute = *freeswitchc::CoreSession_execute;
*sendEvent = *freeswitchc::CoreSession_sendEvent;
*setEventData = *freeswitchc::CoreSession_setEventData;
*getXMLCDR = *freeswitchc::CoreSession_getXMLCDR;
*begin_allow_threads = *freeswitchc::CoreSession_begin_allow_threads;
*end_allow_threads = *freeswitchc::CoreSession_end_allow_threads;
*get_uuid = *freeswitchc::CoreSession_get_uuid;
*get_cb_args = *freeswitchc::CoreSession_get_cb_args;
*check_hangup_hook = *freeswitchc::CoreSession_check_hangup_hook;
*run_dtmf_callback = *freeswitchc::CoreSession_run_dtmf_callback;
*consoleLog = *freeswitchc::CoreSession_consoleLog;
*consoleLog2 = *freeswitchc::CoreSession_consoleLog2;
sub DISOWN {
    my $self = shift;
    my $ptr = tied(%$self);
    delete $OWNER{$ptr};
}

sub ACQUIRE {
    my $self = shift;
    my $ptr = tied(%$self);
    $OWNER{$ptr} = 1;
}


############# Class : freeswitch::Session ##############

package freeswitch::Session;
use vars qw(@ISA %OWNER %ITERATORS %BLESSEDMEMBERS);
@ISA = qw( freeswitch::CoreSession freeswitch );
%OWNER = ();
%ITERATORS = ();
sub new {
    my $pkg = shift;
    my $self = freeswitchc::new_Session(@_);
    bless $self, $pkg if defined($self);
}

sub DESTROY {
    return unless $_[0]->isa('HASH');
    my $self = tied(%{$_[0]});
    return unless defined $self;
    delete $ITERATORS{$self};
    if (exists $OWNER{$self}) {
        freeswitchc::delete_Session($self);
        delete $OWNER{$self};
    }
}

*destroy = *freeswitchc::Session_destroy;
*begin_allow_threads = *freeswitchc::Session_begin_allow_threads;
*end_allow_threads = *freeswitchc::Session_end_allow_threads;
*check_hangup_hook = *freeswitchc::Session_check_hangup_hook;
*run_dtmf_callback = *freeswitchc::Session_run_dtmf_callback;
*setME = *freeswitchc::Session_setME;
*setInputCallback = *freeswitchc::Session_setInputCallback;
*unsetInputCallback = *freeswitchc::Session_unsetInputCallback;
*setHangupHook = *freeswitchc::Session_setHangupHook;
*ready = *freeswitchc::Session_ready;
*swig_suuid_get = *freeswitchc::Session_suuid_get;
*swig_suuid_set = *freeswitchc::Session_suuid_set;
*swig_cb_function_get = *freeswitchc::Session_cb_function_get;
*swig_cb_function_set = *freeswitchc::Session_cb_function_set;
*swig_cb_arg_get = *freeswitchc::Session_cb_arg_get;
*swig_cb_arg_set = *freeswitchc::Session_cb_arg_set;
*swig_hangup_func_str_get = *freeswitchc::Session_hangup_func_str_get;
*swig_hangup_func_str_set = *freeswitchc::Session_hangup_func_str_set;
*swig_hangup_func_arg_get = *freeswitchc::Session_hangup_func_arg_get;
*swig_hangup_func_arg_set = *freeswitchc::Session_hangup_func_arg_set;
*setPERL = *freeswitchc::Session_setPERL;
sub DISOWN {
    my $self = shift;
    my $ptr = tied(%$self);
    delete $OWNER{$ptr};
}

sub ACQUIRE {
    my $self = shift;
    my $ptr = tied(%$self);
    $OWNER{$ptr} = 1;
}


# ------- VARIABLE STUBS --------

package freeswitch;

*S_HUP = *freeswitchc::S_HUP;
*S_FREE = *freeswitchc::S_FREE;
*S_RDLOCK = *freeswitchc::S_RDLOCK;
1;

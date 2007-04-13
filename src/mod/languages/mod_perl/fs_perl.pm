# This file was automatically generated by SWIG
package fs_perl;
require Exporter;
require DynaLoader;
@ISA = qw(Exporter DynaLoader);
package fs_perlc;
bootstrap fs_perl;
package fs_perl;
@EXPORT = qw( );

# ---------- BASE METHODS -------------

package fs_perl;

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

package fs_perl;

*fs_core_set_globals = *fs_perlc::fs_core_set_globals;
*fs_core_init = *fs_perlc::fs_core_init;
*fs_core_destroy = *fs_perlc::fs_core_destroy;
*fs_loadable_module_init = *fs_perlc::fs_loadable_module_init;
*fs_loadable_module_shutdown = *fs_perlc::fs_loadable_module_shutdown;
*fs_console_loop = *fs_perlc::fs_console_loop;
*fs_consol_log = *fs_perlc::fs_consol_log;
*fs_consol_clean = *fs_perlc::fs_consol_clean;
*fs_core_session_locate = *fs_perlc::fs_core_session_locate;
*fs_channel_answer = *fs_perlc::fs_channel_answer;
*fs_channel_pre_answer = *fs_perlc::fs_channel_pre_answer;
*fs_channel_hangup = *fs_perlc::fs_channel_hangup;
*fs_channel_set_variable = *fs_perlc::fs_channel_set_variable;
*fs_channel_get_variable = *fs_perlc::fs_channel_get_variable;
*fs_channel_set_state = *fs_perlc::fs_channel_set_state;
*fs_ivr_play_file = *fs_perlc::fs_ivr_play_file;
*fs_switch_ivr_record_file = *fs_perlc::fs_switch_ivr_record_file;
*fs_switch_ivr_sleep = *fs_perlc::fs_switch_ivr_sleep;
*fs_ivr_play_file2 = *fs_perlc::fs_ivr_play_file2;
*fs_switch_ivr_collect_digits_callback = *fs_perlc::fs_switch_ivr_collect_digits_callback;
*fs_switch_ivr_collect_digits_count = *fs_perlc::fs_switch_ivr_collect_digits_count;
*fs_switch_ivr_originate = *fs_perlc::fs_switch_ivr_originate;
*fs_switch_ivr_session_transfer = *fs_perlc::fs_switch_ivr_session_transfer;
*fs_switch_ivr_speak_text = *fs_perlc::fs_switch_ivr_speak_text;
*fs_switch_channel_get_variable = *fs_perlc::fs_switch_channel_get_variable;
*fs_switch_channel_set_variable = *fs_perlc::fs_switch_channel_set_variable;

# ------- VARIABLE STUBS --------

package fs_perl;

*FREESWITCH_PEN = *fs_perlc::FREESWITCH_PEN;
*FREESWITCH_OID_PREFIX = *fs_perlc::FREESWITCH_OID_PREFIX;
*FREESWITCH_ITAD = *fs_perlc::FREESWITCH_ITAD;
*__EXTENSIONS__ = *fs_perlc::__EXTENSIONS__;
1;

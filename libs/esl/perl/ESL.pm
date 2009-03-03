# This file was created automatically by SWIG 1.3.29.
# Don't modify this file, modify the SWIG interface instead.
package ESL;
require Exporter;
require DynaLoader;
@ISA = qw(Exporter DynaLoader);
package ESLc;
bootstrap ESL;
package ESL;
@EXPORT = qw( );

# ---------- BASE METHODS -------------

package ESL;

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

package ESL;

*eslSetLogLevel = *ESLc::eslSetLogLevel;

############# Class : ESL::ESLevent ##############

package ESL::ESLevent;
use vars qw(@ISA %OWNER %ITERATORS %BLESSEDMEMBERS);
@ISA = qw( ESL );
%OWNER = ();
%ITERATORS = ();
*swig_event_get = *ESLc::ESLevent_event_get;
*swig_event_set = *ESLc::ESLevent_event_set;
*swig_serialized_string_get = *ESLc::ESLevent_serialized_string_get;
*swig_serialized_string_set = *ESLc::ESLevent_serialized_string_set;
*swig_mine_get = *ESLc::ESLevent_mine_get;
*swig_mine_set = *ESLc::ESLevent_mine_set;
sub new {
    my $pkg = shift;
    my $self = ESLc::new_ESLevent(@_);
    bless $self, $pkg if defined($self);
}

sub DESTROY {
    return unless $_[0]->isa('HASH');
    my $self = tied(%{$_[0]});
    return unless defined $self;
    delete $ITERATORS{$self};
    if (exists $OWNER{$self}) {
        ESLc::delete_ESLevent($self);
        delete $OWNER{$self};
    }
}

*serialize = *ESLc::ESLevent_serialize;
*setPriority = *ESLc::ESLevent_setPriority;
*getHeader = *ESLc::ESLevent_getHeader;
*getBody = *ESLc::ESLevent_getBody;
*getType = *ESLc::ESLevent_getType;
*addBody = *ESLc::ESLevent_addBody;
*addHeader = *ESLc::ESLevent_addHeader;
*delHeader = *ESLc::ESLevent_delHeader;
*firstHeader = *ESLc::ESLevent_firstHeader;
*nextHeader = *ESLc::ESLevent_nextHeader;
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


############# Class : ESL::ESLconnection ##############

package ESL::ESLconnection;
use vars qw(@ISA %OWNER %ITERATORS %BLESSEDMEMBERS);
@ISA = qw( ESL );
%OWNER = ();
%ITERATORS = ();
sub new {
    my $pkg = shift;
    my $self = ESLc::new_ESLconnection(@_);
    bless $self, $pkg if defined($self);
}

sub DESTROY {
    return unless $_[0]->isa('HASH');
    my $self = tied(%{$_[0]});
    return unless defined $self;
    delete $ITERATORS{$self};
    if (exists $OWNER{$self}) {
        ESLc::delete_ESLconnection($self);
        delete $OWNER{$self};
    }
}

*connected = *ESLc::ESLconnection_connected;
*getInfo = *ESLc::ESLconnection_getInfo;
*send = *ESLc::ESLconnection_send;
*sendRecv = *ESLc::ESLconnection_sendRecv;
*api = *ESLc::ESLconnection_api;
*bgapi = *ESLc::ESLconnection_bgapi;
*sendEvent = *ESLc::ESLconnection_sendEvent;
*recvEvent = *ESLc::ESLconnection_recvEvent;
*recvEventTimed = *ESLc::ESLconnection_recvEventTimed;
*filter = *ESLc::ESLconnection_filter;
*events = *ESLc::ESLconnection_events;
*execute = *ESLc::ESLconnection_execute;
*setBlockingExecute = *ESLc::ESLconnection_setBlockingExecute;
*setEventLock = *ESLc::ESLconnection_setEventLock;
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

package ESL;

1;

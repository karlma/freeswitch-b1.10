# This file was automatically generated by SWIG (http://www.swig.org).
# Version 1.3.35
#
# Don't modify this file, modify the SWIG interface instead.

package MCAST;
require Exporter;
require DynaLoader;
@ISA = qw(Exporter DynaLoader);
package MCASTc;
bootstrap MCAST;
package MCAST;
@EXPORT = qw( );

# ---------- BASE METHODS -------------

package MCAST;

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

package MCAST;


############# Class : MCAST::McastHandle ##############

package MCAST::McastHandle;
use vars qw(@ISA %OWNER %ITERATORS %BLESSEDMEMBERS);
@ISA = qw( MCAST );
%OWNER = ();
%ITERATORS = ();
sub new {
    my $pkg = shift;
    my $self = MCASTc::new_McastHandle(@_);
    bless $self, $pkg if defined($self);
}

sub DESTROY {
    return unless $_[0]->isa('HASH');
    my $self = tied(%{$_[0]});
    return unless defined $self;
    delete $ITERATORS{$self};
    if (exists $OWNER{$self}) {
        MCASTc::delete_McastHandle($self);
        delete $OWNER{$self};
    }
}

*send = *MCASTc::McastHandle_send;
*recv = *MCASTc::McastHandle_recv;
*fileno = *MCASTc::McastHandle_fileno;
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

package MCAST;


use constant {
	MCAST_SEND => (1 << 0),
	MCAST_RECV => (1 << 1),
	MCAST_TTL_HOST => (1 << 2),
	MCAST_TTL_SUBNET => (1 << 3),
	MCAST_TTL_SITE => (1 << 4),
	MCAST_TTL_REGION => (1 << 5),
	MCAST_TTL_CONTINENT => (1 << 6),
	MCAST_TTL_UNIVERSE => (1 << 7)  
};
1;

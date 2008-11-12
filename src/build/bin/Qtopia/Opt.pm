package Qtopia::Opt;

use strict;
use warnings;

use File::Basename;
use Qtopia::Cache;
use Carp;
#perl2exe_include Carp::Heavy
$Carp::CarpLevel = 1;

require Exporter;
our @ISA = qw(Exporter);

# Items to export into callers namespace by default. Note: do not export
# names by default without a very good reason. Use EXPORT_OK instead.
# Do not simply export all your public functions/methods/constants.

# This allows declaration	use Qtopia::Opt ':all';
# If you do not need this, moving things directly into @EXPORT or @EXPORT_OK
# will save memory.
our %EXPORT_TAGS = ( 'all' => [ qw(
    %optvar_storage
    @ordered_optvar_keys
    _resolve_to_scalar
    _resolve_to_array
    opt
    validate
) ] );

our @EXPORT_OK = ( @{ $EXPORT_TAGS{'all'} } );

our @EXPORT = qw(
    opt_sanity_check
    opt_get_options
    opt_apply_defaults
    opt_resolve
    opt_print_autodetect
    set_optvar
    opt
    add_separator
    add_note
);

our $VERSION = '0.01';

# the engine to use
our $engine = 'Getopt';

# opt variables
our %optvar_storage;
our @ordered_optvar_keys;

# Do a sanity check on the opt_ variables
sub opt_sanity_check()
{
    OPT: for my $optname ( keys %optvar_storage ) {
        my $optref = $optvar_storage{$optname};
        if ( scalar(keys %$optref) == 0 ) {
            print "$optname has no attributes!\n";
            next OPT;
        }
        if ( $optref->{"role"} && $optref->{"role"} eq "placeholder" ) {
            # don't check placeholders because they won't have anything set
            next OPT;
        }
        my $setref = $optref->{"set"};
        my $unsetref = $optref->{"unset"};
        my $arg = $optref->{"arg"};
        my $type = $optref->{"type"};
        if ( !defined($setref) && !defined($unsetref) ) {
            print "$optname has no set/get attribute!\n";
            next OPT;
        }
        for my $set ( qw(set unset) ) {
            my $ref = eval '$'.$set.'ref';
            if ( defined($ref) ) {
                if ( ref($ref) eq "ARRAY" ) {
                    if ( scalar(@$ref) != 2 ) {
                        print "$optname->$set must be a 2-element array!\n";
                    }
                } else {
                    print "$optname->$set is not an array!\n";
                }
            }
        }
    }
}

# Get options and print help
sub opt_get_options
{
    my $ret;
    eval "require Qtopia::Opt::$engine;";
    die $@ if ( $@ );
    eval "\$ret = Qtopia::Opt::${engine}::opt_get_options(\@_)";
    die $@ if ( $@ );
    return $ret;
}

sub get_help
{
    my $ret;
    eval "require Qtopia::Opt::$engine;";
    die $@ if ( $@ );
    eval "\$ret = Qtopia::Opt::${engine}::get_help(\@_)";
    die $@ if ( $@ );
    return $ret;
}

# Print out the autodetect message
sub opt_print_autodetect()
{
    my $ret;
    eval "require Qtopia::Opt::$engine;";
    die $@ if ( $@ );
    eval "\$ret = Qtopia::Opt::${engine}::opt_print_autodetect(\@_)";
    die $@ if ( $@ );
    return $ret;
}

# Apply defaults
sub opt_apply_defaults
{
    for my $optname ( @_, keys %optvar_storage ) {
        my $optref = $optvar_storage{$optname};
        my $auto = ( !defined($optref->{"autodep"}) || _resolve_to_scalar($optref->{"autodep"}) );
        my $vis = ( !defined($optref->{"visible"}) || _resolve_to_scalar($optref->{"visible"}) );
        my $force_default = _resolve_to_scalar($optref->{"force_default"});
        my $def;
        my $ok = ( !defined($optref->{"value"}) && defined($optref->{"default"}) );
        if ( $optref->{"type"} && $optref->{"type"} eq '@' ) {
            my $val = $optref->{"value"};
            $def = $optref->{"default"};
            $ok = 0;
            if ( defined($val) && scalar(@$val) == 0 && defined($def) && scalar(@$def) > 0 ) {
                $ok = 1;
            }
        }
        if ( $ok ) {
            if ( ( $auto && $vis ) || $force_default ) {
                if ( !defined($def) ) {
                    $def = _resolve_to_scalar($optref->{"default"});
                }
                if ( $def && $optref->{"type"} && $optref->{"type"} eq "multi-value" ) {
                    $def =~ s/,/ /g;
                }
                $optref->{"value"} = $def;
                $optref->{"auto"} = 1;
            }
        }
    }
}

# Return the value that the option would have, if defaults had been applied
sub opt_resolve($)
{
    my ( $optname ) = @_;
    my $optref = $optvar_storage{$optname};
    my $auto = ( !defined($optref->{"autodep"}) || _resolve_to_scalar($optref->{"autodep"}) );
    if ( defined($optref->{"value"}) ) {
        return $optref->{"value"};
    }
    if ( !defined($optref->{"value"}) && defined($optref->{"default"}) ) {
        my $defref = $optref->{"default"};
        my $def;
        if ( ref($defref) ne "CODE" ) {
            $def = $defref;
        } elsif ( $auto ) {
            if ( ref($defref) eq "CODE" ) {
                $def = &$defref();
            }
        }
        if ( $def && $optref->{"type"} && $optref->{"type"} eq "multi-value" ) {
            $def =~ s/,/ /g;
        }
        return $def;
    }
    return undef;
}

# Create an opt_ variable
sub set_optvar($$)
{
    my ( $optname, $hashref ) = @_;
    if ( exists($optvar_storage{$optname}) ) {
        croak "opt var $optname already exists!";
    }
    $optvar_storage{$optname} = $hashref;
    push(@ordered_optvar_keys, $optname);
}

# Shorthand to access the value
sub opt : lvalue
{
    if ( scalar(@_) < 1 ) {
        croak "You must supply a name to opt()!";
    }
    my $optname = $_[0];
    my $key = "value";
    if ( scalar(@_) == 2 ) {
        $key = $_[1];
    }
    if ( !exists($optvar_storage{$optname}) ) {
        croak "opt var $optname does not exist!";
    }
    my $optref = $optvar_storage{$optname};
    $optref->{$key};
}

sub add_separator()
{
    if ( $ordered_optvar_keys[$#ordered_optvar_keys] ne "---" ) {
        push(@ordered_optvar_keys, "---");
    }
}

sub add_note($)
{
    my ( $note ) = @_;
    add_separator();
    push(@ordered_optvar_keys, "---$note");
    add_separator();
}

# Dump the opt_ variables to config.cache
sub write_config_cache()
{
    my %ignored_attributes = map { $_ => 1 } qw(set unset setfunc unsetfunc setaliases arg visible autodep);
    my $ret = "";
    OPT: for my $optname ( keys %optvar_storage ) {
        my $noted = 0;
        my $optref = $optvar_storage{$optname};
        ATTR: for my $attribute ( grep { !exists($ignored_attributes{$_}) } keys %$optref ) {
            my $ref = $optref->{$attribute};
            my $value;
            if ( !defined($ref) ) {
                $value = "undef";
            } elsif ( ref($ref) eq "" ) {
                $value = $ref;
            } elsif ( ref($ref) eq "ARRAY" ) {
                for ( my $i = 0; $i < scalar(@$ref); $i++ ) {
                    $value = $$ref[$i];
                    $value = "undef" unless defined($value);
                    $noted = 1;
                    $value =~ s/\\n/\\\\n/g; # \n is likely in path names on Windows
                    $value =~ s/\n/\\n/g;
                    $ret .= "opt.$optname.$attribute.\[$i\]=$value\n";
                }
                next ATTR;
            } elsif ( ref($ref) eq "CODE" ) {
                my $auto = ( !defined($optref->{"autodep"}) || _resolve_to_scalar($optref->{"autodep"}) );
                if ( $attribute ne "default" || $auto ) {
                    $value = &$ref();
                    my @test = &$ref();
                    if ( $value && @test && $value eq scalar(@test) ) {
                        # The function returned an array
                        for ( my $i = 0; $i < scalar(@test); $i++ ) {
                            $value = $test[$i];
                            $value = "undef" unless defined($value);
                            $noted = 1;
                            $value =~ s/\\n/\\\\n/g; # \n is likely in path names on Windows
                            $value =~ s/\n/\\n/g;
                            $ret .= "opt.$optname.$attribute.\[$i\]=$value\n";
                        }
                        next ATTR;
                    }
                } else {
                    $value = "undef";
                }
            } else {
                $value = $ref;
            }
            if ( defined($value) ) {
                $noted = 1;
                $value =~ s/\\n/\\\\n/g; # \n is likely in path names on Windows
                $value =~ s/\n/\\n/g;
                $ret .= "opt.$optname.$attribute=$value\n";
            }
        }
        if ( !$noted ) {
            # Add a bogus entry for this one so it appears in config.cache
            $ret .= "opt.$optname.value=undef\n";
        }
    }
    Qtopia::Cache::replace("opt", $ret);
    return $ret;
}

# Load the opt_ variables from config.cache
sub read_config_cache()
{
    # clear out anything that's in there now
    %optvar_storage = ();

    my @cache = Qtopia::Cache::load("opt");
    for ( @cache ) {
        if ( /^opt\.([^\.=]+)\.([^\.=]+)([^=]*)=(.*)/ ) {
            my $optname = $1;
            my $attribute = $2;
            my $extra = $3;
            my $value = $4;
            $value =~ s/\\\\/__LITERAL__BACK__SLASH__/g;
            $value =~ s/\\n/\n/g;
            $value =~ s/__LITERAL__BACK__SLASH__/\\\\/g;
            $value =~ s/\\\\n/\\n/g;
            if ( $value eq "undef" ) {
                $value = undef;
            }
            my $optref = $optvar_storage{$optname};
            if ( !$optref ) {
                $optref = $optvar_storage{$optname} = +{};
                push(@ordered_optvar_keys, $optname);
            }
            if ( $extra ) {
                # currently we only support @
                if ( $extra =~ /\.\[([0-9]+)\]/ ) {
                    my $i = $1;
                    my $ref = $optref->{$attribute};
                    if ( !$ref ) {
                        $ref = $optref->{$attribute} = [];
                    }
                    $$ref[$i] = $value;
                }
            } else {
                $optref->{$attribute} = $value;
                if ( $attribute eq "type" && $value eq '@' ) {
                    if ( ref($optref->{"value"}) ne "ARRAY" ) {
                        $optref->{"value"} = [];
                    }
                }
            }
        }
    }
}

sub _resolve_to_scalar
{
    my ( $ref ) = @_;

    my $ret;

    if ( !defined($ref) ) {
        $ret = undef;
    } elsif ( ref($ref) eq "ARRAY" ) {
        $ret = join(" ", @$ref);
    } elsif ( ref($ref) eq "CODE" ) {
        $ret = &$ref();
    } else {
        $ret = $ref;
    }

    $ret;
}

sub _resolve_to_array
{
    my ( $ref ) = @_;

    my @ret;

    if ( !defined($ref) ) {
        @ret = ();
    } elsif ( ref($ref) eq "CODE" ) {
        @ret = &$ref();
    } elsif ( ref($ref) eq "ARRAY" ) {
        @ret = @$ref;
    } else {
        push(@ret, $ref);
    }

    @ret;
}

sub setEngine
{
    ( $engine ) = @_;
}

# Validate one (or all) options
sub validate
{
    my @check = @_;
    if ( scalar(@check) == 0 ) {
        @check = keys %optvar_storage
    }

    my $ok = 1;

    # check input against "available", if present
    AVAILCHECK: for my $optname ( @check ) {
        my $optref = $optvar_storage{$optname};
        if ( defined($optref->{"value"}) && $optref->{"available"} && ref($optref->{"available"}) ne "" ) {
            my @available = _resolve_to_array($optref->{"available"});
            AVAILWORD: for my $word ( split(/ /, $optref->{"value"}) ) {
                if ( $word ) {
                    for my $a ( @available ) {
                        if ( $word eq $a ) {
                            next AVAILWORD;
                        }
                    }
                    warn "ERROR: Invalid value for option \"$optname\": $word\n".
                         "       Valid values: ".join(",", @available)."\n";
                    $ok = 0;
                    next AVAILCHECK;
                }
            }
        }
    }

    $ok;
}

# Make this file require()able.
1;
__END__

=head1 NAME

Qtopia::Opt - Option handling system

=head1 SYNOPSIS

    use Qtopia::Opt;
    set_optvar(+{
        ...
    });
    opt_get_options();
    opt_apply_defaults();
    opt_print_autodetect();

=head1 DESCRIPTION

Please see doc/src/buildsys/buildsystem-internals.qdoc for information about the opt system.

=head2 EXPORT

    opt_sanity_check
    opt_get_options
    opt_apply_defaults
    opt_resolve
    opt_print_autodetect
    set_optvar
    opt
    add_separator
    add_note

=head1 AUTHOR

Trolltech AS

=head1 COPYRIGHT AND LICENSE

Copyright (C) 2006 $TROLLTECH$. All rights reserved.

=cut

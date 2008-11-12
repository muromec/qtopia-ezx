package Qtopia::Opt::Getopt;

use strict;
use warnings;

use File::Basename;
use Getopt::Long;
use FileHandle;
use Qtopia::Opt ':all';

# help variables
our $optiondesc;
our $optionhelp;
our $optionstar;
our $optiondef;
our $optionavail;

my $cols = $ENV{COLUMNS};
$cols = 80 unless ( $cols );
# the width of the first column (calculated in _init_formats)
my $fcwidth;

# Get options and print help
sub opt_get_options
{
    my $dohelp = 1;
    my $dieextra = 0;
    my $dovalidate = 1;
    for ( @_ ) {
        if ( $_ eq "nohelp" ) {
            $dohelp = 0;
        }
        if ( $_ eq "noextra" ) {
            $dieextra = 1;
        }
        if ( $_ eq "novalidate" ) {
            $dovalidate = 0;
        }
    }

    # Setup the GetOptions info
    my @optl;
    for my $optname ( keys %optvar_storage ) {
        my $optref = $optvar_storage{$optname};
        my $vis = _resolve_to_scalar($optref->{"visible"});
        my %ignore; map { $ignore{$_} = 1; } _resolve_to_array($optref->{"silentignore"});
        if ( !defined($optref->{"visible"}) || $vis || keys %ignore ) {
            for my $set ( qw(set unset) ) {
                my $ref = $optref->{"$set"};
                if ( defined($ref) && ref($ref) eq "ARRAY" ) {
                    my $paramstring = $$ref[0];
                    if ( index($paramstring, "%") != -1 ) {
                        my $paramname = $optname;
                        $paramname =~ s/_/-/g;
                        $paramstring =~ s/%/$paramname/g;
                    }
                    my $funcref = $optref->{$set."func"};
                    # Don't let an invisible switch cause variables to change!
                    if ( (defined($optref->{"visible"}) && !$vis) ) {
                        # Silently ignore this switch if optref->{"silentignore"} says to, otherwise
                        # leave it unhandled so it causes an error.
                        if ( $ignore{$paramstring} ) {
                            $funcref = sub {
                                my $p = $paramstring;
                                $p =~ s/=.*//;
                                warn "WARNING: -$p has no effect in this configuration.\n";
                            };
                        } else {
                            next;
                        }
                    }
                    if ( !defined($funcref) ) {
                        if ( !defined($optref->{"value"}) ) {
                            if ( defined($optref->{"type"}) && $optref->{"type"} eq '@' ) {
                                my $listref = [];
                                $optref->{"value"} = $listref;
                                $funcref = $listref;
                            } elsif ( defined($optref->{"type"}) && $optref->{"type"} eq 'multi-value' ) {
                                $funcref = sub { opt($optname) = $_[1]; opt($optname) =~ s/,/ /g; };
                            } else {
                                # FIXME should this really be here?
                                $optref->{"value"} = undef;
                            }
                        }
                        if ( !defined($funcref) ) {
                            if ( $set eq "set" ) {
                                $funcref = \$optref->{"value"};
                            } else {
                                $funcref = sub { opt($optname) = 0 };
                            }
                        }
                    }
                    #print "Adding -$paramstring for $optname\n";
                    push(@optl, $paramstring => $funcref);
                    # Aliases (for backwards compatibility)
                    my $aliasref = $optref->{$set."aliases"};
                    if ( defined($aliasref) && ref($aliasref) eq "ARRAY" ) {
                        for my $alias ( @$aliasref ) {
                            my $func = sub {
                                warn "WARNING: -$alias is deprecated. Please use -$paramstring instead.\n";
                                if ( ref($funcref) eq "CODE" ) {
                                    &$funcref();
                                } elsif ( ref($funcref eq "ARRAY") ) {
                                    push(@$funcref, $_[1]);
                                } elsif ( ref($funcref eq "SCALAR") ) {
                                    $$funcref = $_[1];
                                } else {
                                    $$funcref = $_[1];
                                }
                            };
                            push(@optl, $alias => $func);
                        }
                    }
                }
            }
        }
    }
    Getopt::Long::Configure("bundling_override");
    my $ok = GetOptions(@optl);
    if ( $dieextra && @ARGV ) {
        print "Unhandled arguments: ".join(" ",@ARGV)."\n";
        $ok = 0;
    }

    if ( $dovalidate && !Qtopia::Opt::validate() ) {
        $ok = 0;
    }

    if ( !$dohelp ) {
        return $ok;
    }
    if ( !$ok || (exists($optvar_storage{"help"}) && opt("help")) ) {
        get_help();
    }
}

sub get_help
{
    my $doexit = 1;
    for ( @_ ) {
        if ( $_ eq "noexit" ) {
            $doexit = 0;
        }
    }

    # print out some help
    my @helpinfo;
    for my $optname ( @ordered_optvar_keys ) {
        if ( $optname =~ /^---/ ) {
            push(@helpinfo, $optname);
            next;
        }
        my $optref = $optvar_storage{$optname};
        my $vis = _resolve_to_scalar($optref->{"visible"});
        if ( !defined($optref->{"visible"}) || $vis ) {
            my @info = ();
            my $count = 0;
            for my $set ( qw(set unset) ) {
                my $ref = $optref->{$set};
                if ( defined($ref) ) {
                    my $paramstring = @$ref[0];
                    my $paramhelp = @$ref[1];
                    my @paramavail = ();
                    my $paramdef = "";
                    my $paramstar = " ";
                    if ( $paramhelp eq "hidden" ) {
                        next;
                    }
                    if ( index($paramstring, "%") != -1 ) {
                        my $paramname = $optname;
                        $paramname =~ s/_/-/g;
                        $paramstring =~ s/%/$paramname/g;
                    }
                    if ( $paramstring =~ s/=.*// ) {
                        my $arg = $optref->{"arg"};
                        if ( !defined($arg) ) {
                            $arg = "arg";
                        }
                        $paramstring .= " $arg";
                        push(@paramavail, _resolve_to_array($optref->{"available"}));
                        $paramdef = _resolve_to_scalar($optref->{"default"});
                        $paramdef = "" if ( !defined($paramdef) );
                        if ( $optref->{"type"} && $optref->{"type"} eq "multi-value" ) {
                            $paramdef =~ s/\s+/,/g;
                        }
                        if ( $optref->{"type"} && $optref->{"type"} eq '@' ) {
                            my @def = _resolve_to_array($optref->{"default"});
                            if ( @def ) {
                                $paramdef = "\"".join("\" \"", @def)."\"";
                            } else {
                                $paramdef = "";
                            }
                        }
                        if ( $optref->{"default_tested"} ) {
                            $paramstar = "+";
                        }
                    } else {
                        my $def = _resolve_to_scalar($optref->{"default"});
                        if ( ($set eq "set" && $def) ||
                             ($set eq "unset" && defined($def) && !$def) ) {
                            $paramstar = "*";
                            if ( $set eq "set" && $optref->{"default_tested"} ) {
                                $paramstar = "+";
                            }
                        }
                    }
                    if ( exists($optvar_storage{"help"}) && opt("help") ) {
                        push(@info, "-$paramstring", $paramhelp, $paramstar, $paramdef, join(", ", @paramavail));
                    } else {
                        push(@info, "-$paramstring");
                    }
                    $count++;
                }
            }
            if ( scalar(@info) == 1 ) {
                push(@helpinfo, "[ ".$info[0]." ]");
            } elsif ( scalar(@info) == 2 ) {
                push(@helpinfo, "[ ".$info[0]." | ".$info[1]." ]");
            } else {
                push(@helpinfo, "---") if ( $count == 2 );
                push(@helpinfo, @info);
                push(@helpinfo, "---") if ( $count == 2 );
            }
        }
    }
    # Setup the terminal-width-dependant formats;
    if ( exists($optvar_storage{"help"}) && opt("help") ) { 
        _init_formats(@helpinfo);
        print "Usage:  ".basename($0)." [options]

The defaults (*) are usually acceptable. A plus (+) denotes a default
value that needs to be evaluated. If the evaluation succeeds, the
feature is included. Here is a short explanation of each option:

";

        my $spacer = 0;
        while ( @helpinfo ) {
            $optiondesc = shift(@helpinfo);
            if ( $optiondesc =~ s/^---// ) {
                if ( $optiondesc ) {
                    $spacer = 0;
                    format_name STDOUT "NOTE";
                    write;
                } elsif ( !$spacer ) {
                    $spacer = 1;
                    print "\n";
                }
                next;
            }
            $spacer = 0;
            if ( length($optiondesc) < $fcwidth ) {
                $optiondesc .= " ";
                while ( length($optiondesc) < $fcwidth ) {
                    $optiondesc .= ".";
                }
            }
            $optionhelp = shift(@helpinfo);
            $optionstar = shift(@helpinfo);
            $optiondef = shift(@helpinfo);
            $optionavail = shift(@helpinfo);
            format_name STDOUT "LONGHELP";
            write;
        }
        print "\n";
        exit 0 if ( $doexit );
    } else {
        my $header = "Usage:  ".basename($0)." ";
        my $leader = "                  ";
        my $col = length($header);
        print $header;
        for my $option ( @helpinfo ) {
            if ( $option =~ s/^---// ) {
                # skip notes
                next;
            }
            if ( length($option) + $col > ($cols-1) ) {
                print "\n$leader";
                $col = length($leader);
            }
            print "$option ";
            $col += length($option) + 1;
        }
        print "\n\n        Pass -help for a detailed explanation of each option.\n\n";
    }

    exit 2 if ( $doexit );
}

# Print out the autodetect message
sub opt_print_autodetect
{
    my $haveauto = 0;
    for my $optname ( @ordered_optvar_keys ) {
        if ( $optname =~ /^---/ ) {
            next;
        }
        my $optref = $optvar_storage{$optname};
        my $setref = $optref->{"set"};
        my $unsetref = $optref->{"unset"};
        my $line;
        my $value = $optref->{"value"};
        if ( !defined($value) ) {
            $value = "";
        }
        if ( $optref->{"type"} && $optref->{"type"} eq "multi-value" ) {
            $value =~ s/\s+/,/g;
        }
        if ( ($setref && index($$setref[0], "=") == -1) ||
             ($unsetref && index($$unsetref[0], "=") == -1) ) {
            if ( $value ) {
                $line = $$setref[0];
            } else {
                $line = $$unsetref[0];
            }
            if ( !$line ) {
                next;
            }
            my $paramname = $optname;
            $paramname =~ s/_/-/g;
            $line =~ s/%/$paramname/g;
        } elsif ( $setref && index($$setref[0], "=") != -1 ) {
            $line = substr($$setref[0], 0, index($$setref[0], "="))." ";
            my $paramname = $optname;
            $paramname =~ s/_/-/g;
            $line =~ s/%/$paramname/g;
            if ( defined($optref->{"type"}) && $optref->{"type"} eq '@' ) {
                if ( ref($value) eq "ARRAY" && @$value ) {
                    $line .= "'".join("' '", @$value)."'";
                } else {
                    $line = undef;
                }
            } else {
                if ( $value ) {
                    $line .= "'".$value."'";
                } else {
                    $line = undef;
                }
            }
        } else {
            next;
        }
        if ( !defined($line) ) {
            next;
        }
        $line =~ s/%/$optname/;
        my $sa = _resolve_to_scalar($optref->{"showauto"});
        my $vis = _resolve_to_scalar($optref->{"visible"});
        if ( (!defined($optref->{"visible"}) || $vis) && $sa && $optref->{"auto"} ) {
            if ( !$haveauto ) {
                $haveauto = 1;
                print "The following default/detected values have been used:\n";
            }
            print "    -$line\n";
        }
    }
    if ( $haveauto ) {
        print "\n";
    }
}

sub _init_formats
{
    my @helpinfo = @_;

    # work out a good width for the first column
    $fcwidth = 24;

    my $qscr = $cols / 4;
    if ( $qscr > 24 ) {
        while ( @helpinfo ) {
            $optiondesc = shift(@helpinfo);
            next if ( $optiondesc =~ s/^---// );

            $optionhelp = shift(@helpinfo);
            $optionstar = shift(@helpinfo);
            $optiondef = shift(@helpinfo);
            $optionavail = shift(@helpinfo);

            if ( length($optiondesc) > $fcwidth ) {
                $fcwidth = length($optiondesc);
            }
        }
        if ( $fcwidth > $qscr ) {
            $fcwidth = $qscr;
        }
    }

    my $scwidth = $cols - $fcwidth - 7;
    my $fmt = "format LONGHELP =\n".
              '  @ ^'.'<'x($fcwidth).' ^'.'<'x($scwidth)."\n".
              '  $optionstar, $optiondesc,  $optionhelp'."\n".
              '~~   ^'.'<'x($fcwidth-1).' ^'.'<'x($scwidth)."\n".
              '     $optiondesc,            $optionhelp'."\n".
              '~     '.' 'x($fcwidth-1).' Available: ^'.'<'x($scwidth-11)."\n".
              '                             $optionavail'."\n".
              '~~    '.' 'x($fcwidth-1).' ^'.'<'x($scwidth)."\n".
              '                             $optionavail'."\n".
              '~     '.' 'x($fcwidth-1).' Default: ^'.'<'x($scwidth-9)."\n".
              '                             $optiondef'."\n".
              '~~    '.' 'x($fcwidth-1).' ^'.'<'x($scwidth)."\n".
              '                             $optiondef'."\n".
              ".\n";
    eval $fmt;
    die $@ if ( $@ );

    $fmt = "format NOTE =\n".
           '~~      ^'.'<'x($cols-10)."\n\$optiondesc\n.";
    eval $fmt;
    die $@ if ( $@ );

    # Only split on spaces, not the - character
    $: = " ";
}

# Make this file require()able.
1;
__END__

=head1 NAME

Qtopia::Opt::Getopt - Getopt engine for the Qtopia::Opt system.

=cut

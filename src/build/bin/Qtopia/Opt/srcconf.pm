package Qtopia::Opt::srcconf;

use strict;
use warnings;

use Qtopia::Opt ':all';
use Qtopia::Paths;
use File::Path;
use Qtopia::File;

# Get options and print help
sub opt_get_options
{
    my %configmap;
    my @optl;
    for my $optname ( @ordered_optvar_keys ) {
        my $optref = $optvar_storage{$optname};
        my $engineconf = $optref->{"engine"};
        if ( $engineconf && $engineconf =~ /nogui/ ) {
            next;
        }
        my $vis = _resolve_to_scalar($optref->{"visible"});
        my $typ = $optref->{"type"};
        my @avail = _resolve_to_array($optref->{"available"});
        my $type = "string";
        if ( $optref->{"set"} && $optref->{"unset"} ) {
            $type = "bool";
        }
        if ( $typ ) {
            if ( $typ eq "bool" || $typ eq "string" ) {
                $type = $optref->{type};
            }
            if ( $typ eq "multi-value" ) {
                $type = "menu";
            }
            if ( $typ eq '@' ) {
                opt($optname) = [];
                $type = "string";
            }
        }
        if ( $type ne "menu" && scalar(@avail) > 1 ) {
            $type = "choice";
        }
        my $default = _resolve_to_scalar($optref->{"default"});
        my @default = _resolve_to_array($optref->{"default"});
        for my $set ( qw(set) ) {
            my $ref = $optref->{"$set"};
            if ( defined($ref) && ref($ref) eq "ARRAY" ) {
                my $paramstring = $$ref[0];
                if ( index($paramstring, "%") != -1 ) {
                    my $paramname = $optname;
                    $paramname =~ s/_/-/g;
                    $paramstring =~ s/%/$paramname/g;
                }
                my ( $description, $extrahelp ) = extract_desc_help($$ref[1]);
                # FIXME we should enable these based on a global "Show advanced options" switch
                next if ( $description eq "hidden" );

                if ( $type eq "choice" ) {
                    push(@optl, "choice", "prompt \"$description\"",
                                "help");
                    if ( $extrahelp ) {
                        push(@optl, $extrahelp);
                    }
                    push(@optl, " This option is equivalent to the -$paramstring switch.");
                    if ( $default ) {
                        push(@optl, "default ${optname}_$default");
                    } else {
                        push(@optl, "optional");
                    }
                    my $t;
                    if ( $typ && $typ eq "multi-value" ) {
                        $t = "tristate";
                    } else {
                        $t = "bool";
                    }
                    foreach ( @avail ) {
                        $configmap{"${optname}_$_"} = $optname;
                        push(@optl, "config ${optname}_$_", "$t \"$_\"");
                    }
                    push(@optl, "endchoice");
                } elsif ( $type eq "menu" ) {
                    push(@optl, "menu \"$description\"");
                    foreach ( @avail ) {
                        $configmap{"${optname}_$_"} = $optname;
                        push(@optl, "config ${optname}_$_", "bool \"$_\"");
                        push(@optl, "help");
                        if ( $extrahelp ) {
                            push(@optl, $extrahelp);
                        }
                        push(@optl, " This option is equivalent to the -$paramstring switch.");
                        for my $d ( @default ) {
                            if ( $d eq $_ ) {
                                push(@optl, "default \"y\"");
                                last;
                            }
                        }
                    }
                    push(@optl, "endmenu");
                } else {
                    $configmap{"$optname"} = $optname;
                    push(@optl, "config $optname", "$type \"$description\"",
                                "help");
                    if ( $extrahelp ) {
                        push(@optl, $extrahelp);
                    }
                    push(@optl, " This option is equivalent to the -$paramstring switch.");
                    if ( $typ && $typ eq '@' ) {
                        push(@optl, "\n Separate multiple entries using whitespace.",
                                    "\n Quote any whitespace you want to keep with \\ or \".",
                                    "   eg. \"foo bar\" foo\\ bar",
                                    "\n Escape any quotes that you want to keep with \\.",
                                    "   eg. foo=\\\"bar\\\"",
                                    "\n Escape any backslashes that you want to keep with \\.",
                                    "   eg. foo\\\\ bar (this is 2 entries)");
                    }
                    if ( $default ) {
                        if ( $type eq "bool" ) {
                            $default = $default?"y":"n";
                        }
                        push(@optl, "default \"$default\"");
                    }
                }
            }
        }
    }

    my $conf;
    for ( qw(qconf mconf conf) ) {
        if ( -f "$QPEDIR/src/build/bin/$_" ) {
            $conf = "$QPEDIR/src/build/bin/$_";
            last;
        }
    }

    my $build = 0;
    if ( $conf ) {
        if ( needCopy("$depotpath/src/build/bin/Qtopia/Opt/srcconf.pm", $conf) ) {
            $build = 1;
        }
    } else {
        $build = 1;
    }

    if ( $build ) {
        print "Building srcconf...\n";
        if ( ! -d "$QPEDIR/src/build/srcconf/lkc" ) {
            mkpath("$QPEDIR/src/build/srcconf/lkc");
        }
        unless ( open OUT, ">$QPEDIR/src/build/srcconf/lkc/prefix.h" ) {
            warn "Can't write $QPEDIR/src/build/srcconf/lkc/prefix.h\n";
            return 0;
        }
        print OUT "#define PREFIX \"$QPEDIR/src/build\"\n";
        close OUT;
        chdir "$QPEDIR/src/build/srcconf";
        unless ( system("$depotpath/src/build/srcconf/configure --prefix=$QPEDIR/src/build >$QPEDIR/src/build/srcconf/build.log 2>&1") == 0 ) {
            warn "Please see $QPEDIR/src/build/srcconf/build.log for error\n";
            return 0;
        }
        system("make -k >>$QPEDIR/src/build/srcconf/build.log 2>&1");
        unless ( system("make >>$QPEDIR/src/build/srcconf/build.log 2>&1") == 0 ) {
            warn "Please see $QPEDIR/src/build/srcconf/build.log for error\n";
            return 0;
        }
        unless ( system("make install >>$QPEDIR/src/build/srcconf/build.log 2>&1") == 0 ) {
            warn "Please see $QPEDIR/src/build/srcconf/build.log for error\n";
            return 0;
        }
    }

    for ( qw(qconf mconf conf) ) {
        if ( -f "$QPEDIR/src/build/bin/$_" ) {
            $conf = "$QPEDIR/src/build/bin/$_";
            last;
        }
    }
    if ( ! $conf ) {
        warn "Could not find any srcconf binaries\n";
        return 0;
    }

    unless ( open OUT, ">$QPEDIR/src/build/qtopia.conf" ) {
        warn "Can't write $QPEDIR/src/build/qtopia.conf\n";
        return 0;
    }
    if ( $conf =~ /qconf$/ ) {
        print OUT "menu configuration\n";
    }
    print OUT join("\n", @optl)."\n";
    if ( $conf =~ /qconf$/ ) {
        print OUT "endmenu\n";
    }
    close OUT;

    # I think we want to preserve the previous value rather than starting from scratch each time...
=pod
    if ( -f "$QPEDIR/.config" ) {
        unlink "$QPEDIR/.config";
    }
=cut

    chdir "$QPEDIR";
    my $ret = system("$conf $QPEDIR/src/build/qtopia.conf");
    return 0 if ( $ret != 0 );

    if ( -f "$QPEDIR/.config" ) {
        unless ( open IN, "$QPEDIR/.config" ) {
            warn "Can't read $QPEDIR/.config\n";
            return 0;
        }
        my @data = <IN>;
        close IN;
        for ( @data) {
            my $option;
            my $value;
            my $optname;
            if ( /^CONFIG_([^=]+)=(.*)/ ) {
                $option = $1;
                $value = $2;
                $optname = $configmap{$option};
                $option =~ s/^\Q$optname\E//;
                $option =~ s/^_//;
                my $typ = opt($optname, "type");
                if ( $option ne "" ) {
                    if ( $typ && $typ eq "multi-value" ) {
                        my @value = _resolve_to_array(opt($optname));
                        push(@value, $option);
                        $value = join(" ", @value);
                    } else {
                        $value = $option;
                    }
                }
                if ( $value && $value eq "y" ) {
                    $value = 1;
                }
                $value =~ s/^"//;
                $value =~ s/"$//;
                if ( $typ && $typ eq '@' ) {
                    my $ref = opt($optname);
                    $value =~ s/\\"/"/g;
                    $value =~ s/\\\\/\\/g;
                    my @value = split(//, $value);
                    my $quote = 0;
                    my $v = "";
                    while ( @value ) {
                        $_ = shift(@value);
                        if ( /"/ ) {
                            if ( $quote == 0 ) {
                                $quote = 1;
                            } else {
                                $quote = 0;
                            }
                            next;
                        }
                        if ( !$quote && / / ) {
                            push(@$ref, $v);
                            $v = "";
                            next;
                        }
                        if ( /\\/ ) {
                            $_ = shift(@value);
                        }
                        $v .= $_;
                    }
                    push(@$ref, $v) if ( $v );
                } else {
                    opt($optname) = $value;
                }
            }
        }
    }

    return 1;
}

sub get_help
{
    # help is displayed by mconf
    exit 2;
}

sub opt_print_autodetect
{
    # this is shown by mconf
}

sub extract_desc_help
{
    my ( $input ) = @_;

    # Clean up the input string
    $input =~ s/"/\\"/g;
    $input =~ s/^\s+//;
    $input =~ s/\s+$//;
    $input =~ s/\.$//;
    $input =~ s/\s+$//;

    my $d = $input;
    my $h = "";
    if ( $d =~ /(.+?\.\s+)(.*)/ ) {
        $d = $1;
        $h = $2;
        $d =~ s/\s+$//;
        $d =~ s/\.$//;
        $d =~ s/\s+$//;
    } else {
    }

    my @ret;
    push(@ret, $d, $h);
    return @ret;
}

# Make this file require()able.
1;
__END__

=head1 NAME

Qtopia::Opt::srcconf - srcconf engine for the Qtopia::Opt system.

=cut

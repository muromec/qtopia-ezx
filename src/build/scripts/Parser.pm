package Parser;

use strict;
use warnings;
use Carp;
$Carp::CarpLevel = 1;
use Hash::Ordered;

use constant DEBUG => 1;

DEBUG and $| = 1;

my %_variables;
my %_file;
my %_filedata;
my %_filedataindex;
my %_lastcondition;

sub new
{
    my $class = shift;
    my $self = bless \(my $dummy), $class;
    $_variables{$self} = newhash();
    $_file{$self} = undef;
    $_filedata{$self} = undef;
    $_filedataindex{$self} = undef;
    $_lastcondition{$self} = undef;
    return $self;
}

sub DESTROY
{
    my $self = $_[0];
    delete $_variables{$self};
    delete $_file{$self};
    delete $_filedata{$self};
    delete $_filedataindex{$self};
    delete $_lastcondition{$self};
    my $super = $self->can("SUPER::DESTROY");
    goto &$super if $super;
}

sub dbgmsg
{
    DEBUG and print @_;
}

sub _eval
{
    my ( $self, $line ) = @_;
    $_filedata{$self} = [ split(/\n/, $line) ];
    $_filedataindex{$self} = 0;
    eval { $self->parse(); };
    if ( $@ ) {
        die $@."While processing eval $line\n";
    }
}

sub parse
{
    my ( $self ) = @_;
    my $line;
    LINE: while ( defined($line = $self->getLine()) ) {
        dbgmsg "PARSE: $line\n";

        # remove closing scopes (they're not interesting to us)
        $line =~ s/^\s*}//;#{

        # conditionals ( foo:FOO=bar )
        if ( $line =~ s/(.+):// ) {
            my $condition = $1;
            if ( !$self->evaluate($condition) ) {
                next;
            }
            dbgmsg "PARSE: $line\n";
        }

        # block scoping ( foo { ... } )
        if ( $line =~ /{$/ ) {#}
            my $condition = $line;
            $condition =~ s/{$//;#}
            if ( !$self->evaluate($condition) ) {
                # skip this block
                my $find = 0;
                $find++ while ( $line =~ s/{// );#}
                SKIP: while ( defined($line = $self->getLine()) ) {
                    dbgmsg "skipping line $line\n";
                    $find-- while ( $line =~ s/}// );#{
                    last if ( $find == 0 );
                    $find++ while ( $line =~ s/{// );#}
                }
                if ( $find != 0 ) {
                    die "Unterminated {";
                }
                # Skip back a line because there might be another condition to consider
                $_filedataindex{$self}--;
            }
            # this line has nothing interesting... jump to the next one
            next LINE;
        }

        # variable ops (+= *= -= = ~=)
        my $var_re = '[A-Za-z0-9_.]+[A-Za-z0-9_.\-]*[A-Za-z0-9_.]+';
        if ( $line =~ /^\s*($var_re)\s*([\+\*]=|-=|=|~=)(.*)/ ) {
            my ($var, $op, $line) = ( $line =~ /^\s*($var_re)\s*([\+\*]=|-=|=|~=)(.*)/ );
            $line = $self->replaceVariables($line);

            my @values;
            $line =~ s/^\s+//;
            $line =~ s/\s+$//;
            # FIXME handle quotes
            @values = split(/\s+/, $line);

            my $vars = $_variables{$self};
            if ( !exists($$vars{$var}) || $op eq "=" ) {
                $$vars{$var} = newhash();
            }
            if ( $op eq "-=" ) {
                map {
                    return if ( !$_ || !exists($$vars{$var}->{$_}) );
                    delete($$vars{$var}->{$_});
                } @values;
            } elsif ( $op eq "~=" ) {
                die "I can't handle the ~= operator!";
            } else {
                map { $$vars{$var}->{$_} = 1 } @values;
            }
            # remove "empty" variables
            if ( scalar(keys %{$$vars{$var}}) == 0 ) {
                delete $$vars{$var};
            }
            next;
        }

        # bare function calls
        if ( $line =~ /([a-zA-Z0-9_]+)\((.*)\)/ ) {
            my $function = $1;
            my $args = $2;
            $self->callFunction($function, $args);
            next;
        }

        next if ( trim($line) eq "" );

        die "Unhandled line $line";
    }
}

sub newhash
{
    my $hashref = +{};
    tie %$hashref, 'Hash::Ordered';
    return $hashref;
}

sub dumpVariables
{
    my ( $self ) = @_;
    my $vars = $_variables{$self};
    print "\n\nDUMPING VARIABLES\n\n";
    map { print "$_ = ".join(" ", keys %{$$vars{$_}})."\n"; } keys %$vars;
}

sub replaceVariables
{
    my ( $self, $line ) = @_;
    my $vars = $_variables{$self};
    while ( $line =~ /\$\$/ ) {
        my $replace;
        my $args;
        my $source = "func";
        ( $replace, $args ) = ( $line =~ /(\$\$[a-zA-Z0-9_.]+)\((.*)\)/ );
        if ( !$replace ) {
            $source = "vars";
            ( $replace ) = ( $line =~ /(\$\$[a-zA-Z0-9_.]+)/ );
            if ( !$replace ) {
                ( $replace ) = ( $line =~ /(\$\${[a-zA-Z0-9_.]+})/ );
                if ( !$replace ) {
                    $source = "env";
                    ( $replace ) = ( $line =~ /(\$\$\([a-zA-Z0-9_.]+\))/ );
                }
            }
        }
        if ( !$replace ) {
            die "Can't figure out what \$\$ is attached to in line:\n$line";
        }
        my $var = $replace;
        $var =~ s/\$\$//;
        $var =~ s/^{//;
        $var =~ s/}$//;
        my $value;
        if ( $source eq "vars" ) {
            if ( !exists($$vars{$var}) ) {
                #warn "WARNING: Variable $var does not exist!\n";
                $value = "";
            } else {
                $value = join(" ", keys %{$$vars{$var}});
            }
        } elsif ( $source eq "env" ) {
            $value = $ENV{$var};
            if ( !defined($value) ) {
                #warn "Environment variable $var does not exist";
                $value = "";
            }
        } elsif ( $source eq "func" ) {
            $args = trim($self->replaceVariables($args));
            $value = $self->callFunction('$$'.$var, $args);
        } else {
            $value = "";
        }
        dbgmsg "replacing $replace with $value\n";
        $line =~ s/\Q$replace\E/$value/g;
    }
    return $line;
}

sub getLine
{
    my ( $self ) = @_;
    my $filedata = $_filedata{$self};
    my $cont = "";
    while ( 1 ) {
        if ( $_filedataindex{$self} >= scalar(@$filedata) ) {
            return undef;
        }
        my $line = $$filedata[$_filedataindex{$self}++];
        chomp $line;

        # strip all comments out early
        if ( $line =~ s/#.*// ) {
            # if we were continuing a line, a comment implies that we'll keep processing
            # beyond the current line even though there's no \ character
            if ( $cont ) {
                $cont .= $line;
                next;
            }
        }

        # collapse line continuations
        if ( $line =~ s/\\\s*$// ) {
            $cont .= $line;
            next;
        }
        if ( $cont ) {
            $line = $cont.$line;
            $cont = "";
        }

        # strip leading and trailing spaces
        $line = trim($line);

        # ignore blank lines
        next if ( $line eq "" );

        # simplify whitespace
        # FIXME ignore anything in quotes
        $line =~ s/\s+/ /g;

        return $line;
    }
}

sub evaluate
{
    my ( $self, $condition ) = @_;
    $condition = trim($self->replaceVariables($condition));
    my $ret;
    # special case for "else"
    if ( $condition eq "else" ) {
        $ret = (!$self->evaluate($_lastcondition{$self}));
    # special case for !
    } elsif ( $condition =~ s/^\!// ) {
        $ret = (!$self->evaluate($condition));
    } else {
        # multiple conditions "and"ed together
        if ( $condition =~ s/(.+):// ) {
            my $cond = $1;
            if ( $self->evaluate($cond) ) {
                $ret = $self->evaluate($condition);
            } else {
                $ret = 0;
            }
        # multiple conditions "or"ed together
        } elsif ( $condition =~ s/(.+)\|// ) {
            my $cond = $1;
            if ( $self->evaluate($cond) ) {
                $ret = 1;
            } else {
                $ret = $self->evaluate($condition);
            }
        } else {
            # a single condition
            dbgmsg "evaluating $condition\n";
            $_lastcondition{$self} = $condition;
            my $vars = $_variables{$self};
            my $ret = 0;
            if ( $condition =~ /([a-zA-Z0-9_]+)\((.*)\)/ ) {
                my $function = $1;
                my $args = $2;
                $ret = $self->callFunction($function, $args);
            } elsif ( scalar(keys %{$$vars{CONFIG}}) > 0 ) {
                # implicit CONFIG check
                $ret = $$vars{CONFIG}->{$condition};
            }
        }
    }
    return $ret;
}

sub trim
{
    my ( $input ) = @_;
    $input =~ s/^\s+//;
    $input =~ s/\s+$//;
    return $input;
}

sub callFunction
{
    my ( $self, $function, $args ) = @_;
    my $ret;
    my @argv = split(/,/, $args);
    for(@argv){$_ = trim($_);}
    my $vars = $_variables{$self};

    dbgmsg "calling function $function(".join(" ", @argv).") : ";
    if ( $function eq "equals" ) {
        my $variable;
        if ( exists($$vars{$argv[0]}) ) {
            $variable = join(" ", keys %{$$vars{$argv[0]}});
        } else {
            $variable = "";
        }
        dbgmsg "$variable === $argv[1]";
        $ret = ( $variable eq $argv[1] );
    } elsif ( $function eq "isEmpty" ) {
        if ( exists($$vars{$argv[0]}) ) {
            $ret = ( scalar(keys %{$$vars{$argv[0]}}) > 0 );
        } else {
            $ret = 1;
        }
        dbgmsg $ret;
    } elsif ( $function eq "contains" ) {
        $ret = 0;
        if ( exists($$vars{$argv[0]}) ) {
            dbgmsg "$argv[0] in { ".join(" ", keys %{$$vars{$argv[0]}})." }";
            for ( keys %{$$vars{$argv[0]}} ) {
                if ( $argv[1] eq $_ ) {
                    $ret = 1;
                    last;
                }
            }
        }
    } elsif ( $function eq '$$fixpath' ) {
        $ret = $argv[0];
    } elsif ( $function eq 'include' ) {
        $ret = 0;
        my $file = $argv[0];

        if ( open IN, $file ) {
            my $prevfile = $_file{$self};
            my $prevfiledata = $_filedata{$self};
            my $prevfiledataindex = $_filedataindex{$self};

            my @data = <IN>;
            close IN;

            $_file{$self} = $file;
            $_filedata{$self} = \@data;
            $_filedataindex{$self} = 0;

            dbgmsg(" : PARSING\n");
            $self->parse();
            dbgmsg "calling function $function(".join(" ", @argv).") : ";

            $_file{$self} = $prevfile;
            $_filedata{$self} = $prevfiledata;
            $_filedataindex{$self} = $prevfiledataindex;
        }
    } else {
        my $skip = 0;
        for ( qw(qtopia_project depends idep dep license qt_inc resolve_include $$files $$fromfile for requires uses_export $$system createHeader defineReplace defineTest eval export setupTranslatables exists getAllTranslatables) ) {
            if ( $function eq $_ ) {
                #warn "Skipping function $function";
                $skip = 1;
            }
        }
        if ( !$skip ) {
            warn "Unknown function $function";
        }
        $ret = 0;
    }
    dbgmsg " : $ret\n";
    return $ret;
}

sub copy
{
    my ( $self ) = @_;
    my $newself = new Parser;
    $_variables{$newself} = deep_copy($_variables{$self});
    return $newself;
}

sub deep_copy
{
    my $this = shift;
    if (not ref $this) {
        return $this;
    } elsif (ref $this eq "HASH") {
        my $hash = newhash();
        map { $hash->{$_} = deep_copy($this->{$_}); } keys %$this;
        return $hash;
    } else {
        die "what type is $_?";
    }
}

# Make this file require()able.
1;

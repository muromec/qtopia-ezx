#!/usr/bin/perl

use strict;
use warnings;

use File::Basename;
use lib ( dirname($0) );
use Hash::Ordered;
use Parser;
use Cwd;
use File::Find;

use constant DEBUG => 0;

my %projects;
tie %projects, 'Hash::Ordered';

my $root;
$root = shift(@ARGV) or usage();
my $defparser = new Parser;
# FIXME use the real one!
$defparser->_eval("include(config.pri)");
find(sub {
    my $file = $File::Find::name;
    if ( $file =~ /\.pro$/ ) {
        my $parser = $defparser->copy();
        $parser->_eval("include($file)");
        #$parser->dumpVariables();
        $projects{$file} = $parser;
    }
}, $root);

print "\n\n\n";

for ( keys %projects ) {
    #print "Project $_\n";
}

exit 0;

sub usage
{
    print "Usage:  ./".basename($0)." /path/to/qtopia\n";
    exit 2;
}


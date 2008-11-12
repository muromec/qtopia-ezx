#!/usr/bin/perl

use strict;
use warnings;

use Cwd;

my @files = qw/
    main.cpp
    malpkg.cpp
    malpkg.h
    malpkg.html
    mal.png
    /;

my $proj_file = "malpkg.pro";
my $desktop_file = "malpkg.desktop";

my @domains;
{
    # supress warnings about the commas
    no warnings;
    @domains = qw' untrusted untrusted trusted';
}
my $projroot = getcwd();
my $proj_def;
open PRO, $proj_file or die "open $proj_file : $!\n";
{
    local $/ = undef;
    $proj_def = <PRO>;
}
close PRO;

# pkg.multi=pkgA pkgB pkgC pkgD pkgE pkgF pkgG pkgH pkgJ pkgK
my ( $multi_str ) = $proj_def =~ m/pkg.multi\s*=\s*(.*)\s*?$/m ;

print ">>$multi_str<<\n";
# debug

my @projects = split( /\s+/, $multi_str );

my $dom = 0;

my $num_domains = @domains;
for my $pkdir ( @projects )
{
    -d $pkdir || mkdir $pkdir or die "mkdir $pkdir : $!\n";
    chdir $pkdir or die "chdir $pkdir : $!\n";
    for my $f ( @files )
    {
        next if ( -l "$f" );
        symlink "../$f", "$f" or die "$! : symlink $projroot/$f -> $projroot/$pkdir/$f\n";
    }
    for my $ts ( <../*.ts> )
    {
        my ( $ts_name, $lang ) = $ts =~ m/..\/(.*?)-(\w+).ts$/ ;
        my $ts_link = "mal$pkdir-$lang.ts" ;
        next if ( -l "$ts_link" );
        symlink "$ts", "$ts_link" or die "$! : symlink ../$ts $ts_link\n";
    }
    my $new_desktop_file = "mal$pkdir.desktop";
    unless ( -f $new_desktop_file )
    {
        my $orig_desktop_file = "../$desktop_file";
        open ORIG, "<$orig_desktop_file" or die "open $orig_desktop_file in $pkdir : $!\n";
        open DESKTOP, ">$new_desktop_file" or die "open $new_desktop_file in $pkdir : $!\n";
        while ( <ORIG> )
        {
            if ( m/^Exec/ )
            {
                print DESKTOP "Exec=mal$pkdir\n";
            }
            elsif ( m/^Icon/ )
            {
                print DESKTOP "Icon=mal$pkdir\/mal\n";
            }
            elsif (m/^Name\[\]/ )
            {
                print DESKTOP "Name[]=Mal$pkdir\n";
            }
            else
            {
                print DESKTOP;
            }
        }
        close ORIG;
        close DESKTOP;
    }
    unless ( -f $proj_file )
    {
        my $this_def = $proj_def;
        $this_def =~ s/malpkg/mal$pkdir/g ;
        $this_def =~ s/pkg.domain=untrusted/pkg.domain=${domains[$dom]}/ ;
        $this_def =~ s/(help.files=mal)$pkdir(.html)/$1pkg$2/; 

        open PRO, ">$proj_file" or die "open $projroot/$proj_file : $!\n";
        print PRO $this_def;
        close PRO;
    }
    symlink "../malpkg.cpp", "mal$pkdir.cpp" unless ( -l "mal$pkdir.cpp" );
    symlink "../malpkg.h", "mal$pkdir.h" unless ( -l "mal$pkdir.h" );
    mkdir "help" unless ( -d "help" );
    mkdir "help/html" unless ( -d "help/html" );
    symlink "../../../malpkg.html", "help/html/mal$pkdir.html" unless ( -l "help/html/mal$pkdir.html" );
    system( "qtopiamake" );
    system( "make packages FORMAT=qpk" );
    while ( <*.qpk> ) { rename "$_", "../$_"; }
    chdir "..";
    $dom++;
    $dom %= $num_domains;
}

print <<END

  Packages all built.  Now do:

  find . -name "*.qpk" -exec mv -f "{}" \$PWD \\;
  \$QPEDIR/bin/mkPackages /path/to/webroot

END

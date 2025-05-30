#!/usr/bin/perl
use strict;
use warnings;
use v5.40;

use File::Basename;
use Getopt::Long;
use Socket;

my $addr = "255.255.255.255";
my $port = 9;
my $showHelp;

my $basename = basename($0);
my $usage    = <<END;
usage: $basename [FLAGS] <MAC>...
    -i      broadcast address (default: $addr)
    -p      port number (default: $port)
    -h      print help
END

sub main() {
    GetOptions(
        "p=i"  => \$port,
        "i=s"  => \$addr,
        "help" => \$showHelp
    ) or exit(2);

    if ($showHelp) {
        print($usage);
        exit(0);
    }

    my @macs = @ARGV;
    if ( @macs == 0 ) {
        print($usage);
        exit(1);
    }

    socket( my $socket, PF_INET, SOCK_DGRAM, getprotobyname("udp") ) or die;
    setsockopt( $socket, SOL_SOCKET, SO_BROADCAST, 1 )               or die;
    my $portaddr = sockaddr_in( $port, inet_aton($addr) );
    connect( $socket, $portaddr ) or die;

    for my $mac (@macs) {
        send( $socket, make_magic($mac), 0 ) or die;
    }
}

sub make_magic($mac) {
    unless ( $mac =~ /^([a-f0-9]{2}:){5}[a-f0-9]{2}$/i ) {
        die("invalid MAC: \"$mac\"");
    }
    $mac =~ s/://g;
    my $scalar = '';
    for ( my $i = 0 ; $i < length($mac) ; $i += 2 ) {
        my $byte = substr( $mac, $i, 2 );
        $scalar .= pack( 'H*', $byte );
    }

    return "\xff" x 6 . $scalar x 16;
}

main();

#!/usr/bin/perl5
#
# System call args checker
# Copyright (C) 2007 Mike McCormack
#

use strict;
my @args;
my $name;
my $count;
my %funcs;

#
# Count the number of args of each Nt function in winternl.h
#
open(WIN,"<../include/winternl.h") || die "winternl.h not found\n";
while(<WIN>) {
	next if ( ! s/^NTSTATUS[ ]*NTAPI[ ]*// );
	next if ( ! /^Nt/ );
	chomp;
	s/ //;
	$name = $_;
	$name =~ s/\(.*$//;
	@args = split /,/;
	$count = @args;
	$funcs{$name} = $count;
	#printf("%s -> %d\n", $name, $funcs{$name});
}
close WIN;

#
# Check the number of args in each declaration in syscall.cpp
#
open(SYS,"<syscall.cpp") || die "syscall.cpp not found\n";
while(<SYS>) {
	chomp;
	s/ //;
	next if ( ! s/[ ]*\),$// );

	# check for functions that aren't declared that could be
	if ( s/^.*NUL\([ ]*// ) {
		$name = $_;
		next if ( !$funcs{$name} );
		printf("    DEC( %s, %s ),\n", $name, $funcs{$name});
	}

	# check for functions with the wrong number of args
	elsif ( s/^.*DEC\([ ]*// ) {
		($name, $count) = split /,/;
		next if ( $count == $funcs{$name} );
		printf ("args mismatch: %s %d != %d\n", $name, $count, $funcs{$name});
	}
}
close SYS;

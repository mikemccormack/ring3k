#!/usr/bin/perl5
use strict;

sub count_syscalls($) {
	my $header = shift;
	my $functions;
	my $undeclared;
	my $declared;
	my $implemented;
	my $start;
	open(FILE,"<../include/$header.h") || die;
	while(<FILE>) {
		if (/NUL/) {
			$undeclared++;
		}
		if (/DEC/) {
			$declared++;
		}
		if (/IMP/) {
			$implemented++;
		}
	}
	close FILE;
	$functions = $undeclared + $declared + $implemented;
	printf ("%s:\n", $header);
	printf ("\tundeclared:    %3d (%2.0f%%)\n", $undeclared, 100*$undeclared/$functions);
	printf ("\tdeclared:      %3d (%2.0f%%)\n", $declared, 100*$declared/$functions);
	printf ("\timplemented:   %3d (%2.0f%%)\n", $implemented, 100*$implemented/$functions);
	printf ("\ttotal:         %3d\n", $functions);
	printf ("\n");
}

count_syscalls("ntsyscall");
count_syscalls("uisyscall");

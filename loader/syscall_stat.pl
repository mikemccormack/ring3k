#!/usr/bin/perl5
use strict;

sub count_syscalls($) {
	my $type = shift;
	my $functions;
	my $undeclared;
	my $declared;
	my $implemented;
	my $start;
	open(FILE,"<syscall.cpp") || die;
	while(<FILE>) {
		if ( /^ntcalldesc $type/ ) {
			$start = 1;
		}
		
		if ($start && /^};/) {
			$start = 0;
		}
		if ($start) {
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
	}
	close FILE;
	$functions = $undeclared + $declared + $implemented;
	printf ("%s:\n", $type);
	printf ("\tundeclared:    %3d (%2.0f%%)\n", $undeclared, 100*$undeclared/$functions);
	printf ("\tdeclared:      %3d (%2.0f%%)\n", $declared, 100*$declared/$functions);
	printf ("\timplemented:   %3d (%2.0f%%)\n", $implemented, 100*$implemented/$functions);
	printf ("\ttotal:         %3d\n", $functions);
	printf ("\n");
}

count_syscalls("ntcalls");
count_syscalls("ntgdicalls");

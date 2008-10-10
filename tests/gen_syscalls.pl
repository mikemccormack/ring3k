#!/usr/bin/perl
#
# nt loader
#
# Copyright 2006-2008 Mike McCormack
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA

my $infile = $ARGV[0];
open(IN,"<$infile") || die("failed to open $infile\n");

# need an entry point
print <<EOF
	.text
.globl _DllMain
_DllMain:
	movl \$1, %eax
	ret \$12
EOF
;

while(<IN>) {
    s/#.*$//;
    next if ( /^$/ );
    ($ord, $func, $numargs) = split / /;
    $numargs *= 4;
    print <<EOF
	.text
.align 4
.globl _$func\@$numargs
_$func\@$numargs:
	mov \$0x$ord, %eax
	lea 4(%esp), %edx
	int \$0x2e
	ret \$$numargs
EOF
}

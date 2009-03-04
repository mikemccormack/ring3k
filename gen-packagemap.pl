#!/usr/bin/perl
# generate a packagemap definition from the ring3k git repository

open(IN,"git ls-tree -r --name-only HEAD|") || die "git ls-tree failed";
open(OUT,">packagemap.xml");
print OUT "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
print OUT "<fileset>\n";
while(<IN>)
{
	chomp;
	my $file = $_;

	# ignore some files
	next if ($file =~ /\.gitignore$/ );
	next if ($file =~ /README$/ );
	next if ($file =~ /^COPYING\.LIB$/ );
	next if ($file =~ /install-sh$/ );
	next if ($file =~ /^documents/ );
	next if ($file =~ /^configure$/ );
	next if ($file =~ /^config\.sub$/ );
	next if ($file =~ /^config\.guess$/ );
	next if ($file =~ /^config\.status$/ );
	next if ($file =~ /config\.h\.in$/ );
	next if ($file =~ /^TODO$/ );
	next if ($file =~ /\.rc$/ );
	next if ($file =~ /\.bmp$/ );
	next if ($file =~ /\.ico$/ );
	next if ($file =~ /\.dat$/ );
	next if ($file =~ /ntwin32\.in$/ );

	# classify everything else
	if ($file =~ /^ring3k-setup\.in$/ ) {
		$type = "shell";
	} elsif ($file =~ /^ring3k\.in$/ ) {
		$type = "shell";
	} elsif ($file =~ /^runtest$/ ) {
		$type = "shell";
	} elsif ($file =~ /\.cpp$/ ) {
		$type = "C++";
	} elsif ($file =~ /kernel\/.*\.h$/ ) {
		$type = "C++";
	} elsif ($file =~ /\.inl$/ ) {
		$type = "C++";
	} elsif ($file =~ /\.h$/ ) {
		$type = "C";
	} elsif ($file =~ /\.c$/ ) {
		$type = "C";
	} elsif ($file =~ /Makefile\.in$/ ) {
		$type = "makefile";
	} elsif ($file =~ /^configure.ac$/ ) {
		$type = "autoconf";
	} elsif ($file =~ /\.xml\.in$/ ) {
		$type = "xml";
	} elsif ($file =~ /\.xml$/ ) {
		$type = "xml";
	} elsif ($file =~ /\.pl$/ ) {
		$type = "perl";
	} elsif ($file =~ /\.svg$/ ) {
		$type = "svg";
	} else {
		print STDERR "unclassified file $file\n";
		next;
	}

	print OUT " <path>$file</path>\n";
	print OUT " <type>$type</type>\n";
}
print OUT "</fileset>\n";
close IN;
close OUT;

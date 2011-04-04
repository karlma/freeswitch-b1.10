#!/usr/bin/perl

# Doxygen is usefull for html documentation, but sucks 
# in making manual pages. Still tool also parses the .h
# files with the doxygen documentation and creates
# the man page we want
#
# 2 way process
# 1. All the .h files are processed to create in file in which:
# filename | API | description | return values
# are documented
# 2. Another file is parsed which states which function should
# be grouped together in which manpage. Symlinks are also created.
#
# With this all in place, all documentation should be autogenerated
# from the doxydoc.

use Getopt::Std;

my $state;
my $description;
my $struct_description;
my $key;
my $return;
my $param;
my $api;
my $const;

my %description;
my %api;
my %return;
my %options;
my %manpages;
my %see_also;

my $BASE="doc/man";
my $MAN_SECTION = "3";
my $MAN_HEADER = ".TH ldns $MAN_SECTION \"30 May 2006\"\n";
my $MAN_MIDDLE = ".SH AUTHOR
The ldns team at NLnet Labs. Which consists out of
Jelte Jansen and Miek Gieben.

.SH REPORTING BUGS
Please report bugs to ldns-team\@nlnetlabs.nl or in 
our bugzilla at
http://www.nlnetlabs.nl/bugs/index.html

.SH COPYRIGHT
Copyright (c) 2004 - 2006 NLnet Labs.
.PP
Licensed under the BSD License. There is NO warranty; not even for
MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.
";
my $MAN_FOOTER = ".SH REMARKS
This manpage was automaticly generated from the ldns source code by
use of Doxygen and some perl.
";

getopts("m:",\%options);
# if -m manpage file is given process that file
# parse the file which tells us what manpages go together
my $functions, $see_also;
if (defined $options{'m'}) {
	# process
	open(MAN, "<$options{'m'}") or die "Cannot open $options{'m'}";
		# it's line based:
		# func1, func2, .. | see_also1, see_also2, ...
		while(<MAN>) {
			chomp;
			if (/^#/) { next; }
			if (/^$/) { next; }
			($functions, $see_also) = split /[\t ]*\|[\t ]*/, $_;
			#print "{$functions}\n";
			#print "{$see_also}\n";
			my @funcs = split /[\t ]*,[\t ]*/, $functions;
			my @also = split /[\t ]*,[\t ]*/, $see_also;
			$manpages{$funcs[0]} = \@funcs;
			$see_also{$funcs[0]} = \@also;
			#print "[", $funcs[0], "]\n";
		}
	close(MAN);
} else {
	print "Need -m file to process the .h files\n";
	exit 1;
}

# 0 - somewhere in the file
# 1 - in a doxygen par
# 2 - after doxygen, except funcion

# create our pwd
mkdir "doc";
mkdir "doc/man";
mkdir "doc/man/man$MAN_SECTION";

$state = 0;
my $i;
my @lines = <STDIN>;
my $max = @lines;

while($i < $max) {
	$typedef = "";
	if ($lines[$i] =~ /^typedef struct/ and $lines[$i + 1] =~ /^struct/) {
		# move typedef to below struct
		$typedef = $lines[$i];
		$j = $i;
		while ($lines[$j] !~ /}/) {
			$lines[$j] = $lines[$j+1];
			$j++;
		}
	$lines[$j] = $lines[$j+1];
	$lines[$j + 1] = $typedef;
	}

	$cur_line = $lines[$i];
	chomp($cur_line);
	if ($cur_line =~ /^\/\*\*[\t ]*$/) {
		# /** Seen
		#print "Comment seen! [$cur_line]\n";
		$state = 1;
		undef $description;
		undef $struct_description;
		$i++;
		next;
	}
	if ($cur_line =~ /\*\// and $state == 1) {
		#print "END Comment seen!\n";
		$state = 2;
		$i++;
		next;
	}

	if ($state == 1) {
		# inside doxygen 
		$cur_line =~ s/\\/\\\\/g;
		$cur_line =~ s/^[ \t]*\* ?//;
		$description = $description . "\n" . $cur_line;
		#$description = $description . "\n.br\n" . $cur_line;
	}
	if ($state == 2 and $cur_line =~ /const/) {
		# the const word exists in the function call
		#$const = "const";
		#s/[\t ]*const[\t ]*//;
	} else {
		#undef $const;
	}
	
	if ($cur_line =~ /^INLINE/) {
		$cur_line =~ s/^INLINE\s*//;
		while ($cur_line !~ /{/) {
			$i++;
			$cur_line .= " ".$lines[$i];
			$cur_line =~ s/\n//;
		}
		$cur_line =~ s/{/;/;
	}
	
	if ($cur_line =~ /^[^#*\/ ]([\w\*]+)[\t ]+(.*?)[({](.*)\s*/ and $state == 2) {
		while ($cur_line !~ /\)\s*;/) {
			$i++;
			$cur_line .= $lines[$i];
			chomp($cur_line);
			$cur_line =~ s/\n/ /g;
			$cur_line =~ s/\s\s*/ /g;
		}
		$cur_line =~ /([\w\* ]+)[\t ]+(.*?)\((.*)\)\s*;/;
		# this should also end the current comment parsing
		$return = $1;
		$key = $2;
		$api = $3;
		# sometimes the * is stuck to the function
		# name instead to the return type
		if ($key =~ /^\*/) {
			#print"Name starts with *\n";
			$key =~ s/^\*//;
			if (defined($const)) {
				$return =  $const . " " . $return . '*';
			} else {
				$return =  $return . '*';
			}
		}
		$description =~ s/\\param\[in\][ \t]*([\*\w]+)[ \t]+/.br\n\\fB$1\\fR: /g;
		$description =~ s/\\param\[out\][ \t]*([\*\w]+)[ \t]+/.br\n\\fB$1\\fR: /g;
		$description =~ s/\\return[ \t]*/.br\nReturns /g;

		$description{$key} = $description;
		$api{$key} = $api;
		$return{$key} = $return;
		undef $description;
		undef $struct_description;
		$state = 0;
	} elsif ($state == 2 and (
			$cur_line =~ /^typedef\sstruct\s(\w+)\s(\w+);/ or
			$cur_line =~ /^typedef\senum\s(\w+)\s(\w+);/)) {
		$struct_description .= "\n.br\n" . $cur_line;
		$key = $2;
		$struct_description =~ s/\/\*\*\s*(.*?)\s*\*\//\\fB$1:\\fR/g;
		$description{$key} = $struct_description;
		$api{$key} = "struct";
		$return{$key} = $1;
		undef $description;
		undef $struct_description;
		$state = 0;
	} else {
		$struct_description .= "\n.br\n" . $cur_line;
	}
	$i++;
}

# create the manpages
foreach (keys %manpages) {
	$name = $manpages{$_};
	$also = $see_also{$_};

	$filename = @$name[0];
	$filename = "$BASE/man$MAN_SECTION/$filename.$MAN_SECTION";

	my $symlink_file = @$name[0] . "." . $MAN_SECTION;

#	print STDOUT $filename,"\n";
	open (MAN, ">$filename") or die "Can not open $filename";

	print MAN  $MAN_HEADER;
	print MAN  ".SH NAME\n";
	print MAN  join ", ", @$name;
	print MAN  "\n\n";
	print MAN  ".SH SYNOPSIS\n";

	print MAN  "#include <stdint.h>\n.br\n";
	print MAN  "#include <stdbool.h>\n.br\n";

	print MAN  ".PP\n";
	print MAN  "#include <ldns/ldns.h>\n";
	print MAN  ".PP\n";

	foreach (@$name) {
		$b = $return{$_};
		$b =~ s/\s+$//;
		if ($api{$_} ne "struct") {
			print MAN  $b, " ", $_;
			print MAN  "(", $api{$_},");\n";
			print MAN  ".PP\n";
		}
	}

	print MAN  "\n.SH DESCRIPTION\n";
	foreach (@$name) {
		print MAN  ".HP\n";
		print MAN "\\fI", $_, "\\fR";
		if ($api{$_} ne "struct") {
			print MAN "()"; 
		}
#		print MAN ".br\n";
		print MAN  $description{$_};
		print MAN  "\n.PP\n";
	}

	print MAN $MAN_MIDDLE;

	if (defined(@$also)) {
		print MAN "\n.SH SEE ALSO\n\\fI";
		print MAN join "\\fR, \\fI", @$also;
		print MAN "\\fR.\nAnd ";
		print MAN "\\fBperldoc Net::DNS\\fR, \\fBRFC1034\\fR,
\\fBRFC1035\\fR, \\fBRFC4033\\fR, \\fBRFC4034\\fR  and \\fBRFC4035\\fR.\n";
	} else {
		print MAN ".SH SEE ALSO
\\fBperldoc Net::DNS\\fR, \\fBRFC1034\\fR,
\\fBRFC1035\\fR, \\fBRFC4033\\fR, \\fBRFC4034\\fR and \\fBRFC4035\\fR.\n";
	}
	
	print MAN $MAN_FOOTER;

	# create symlinks
	chdir("$BASE/man$MAN_SECTION");
	foreach (@$name) {
		print STDERR $_,"\n";
		my $new_file = $_ . "." . $MAN_SECTION;
		if ($new_file eq $symlink_file) {
			next;
		}
		#print STDOUT "\t", $new_file, " -> ", $symlink_file, "\n";
		symlink $symlink_file, $new_file;
	}
	chdir("../../.."); # and back, tricky and fragile...
	close(MAN);
}

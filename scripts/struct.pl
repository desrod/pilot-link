#!/usr/bin/perl
#
# Copyright (c) 1997, Kenneth Albanowksi
#
# Licensed under GPL v2.

# Turn struct definition into portable set of access routines

$example = "

/* begin struct xyz
 *   Char aa
 *   Word a
 *   DWord b
 *   SWord c 
 *   Char[foo][bar] d
 *   Byte e
 *   DWord[5][bar] e2
 *   DWord[5][5] e2
 *   Char[5] f
 *   Char[0] g
 * struct access */
#define get_name_a(ptr) get_short(ptr)
#define get_name_b(ptr) get_long(ptr+2)
#define get_name_c(ptr) get_sshort(ptr+6)
#define ptr_name_d(ptr) ptr+8
#define get_name_d(ptr,idx)
#define sizeof_name     28
 /* end struct xyz */

/* begin struct abc
 *  Char a
 *  Char b
 *  Char c
 *  Word d
 * end struct abc 
 */

/* begin struct abc
 *  Char a
 *  Char b
 *  Char c
 *  Word d
 * struct access */
/* struct offsets */
 * end struct abc */
";

%sizes = (	Word => 2, SWord => -2, DWord => 4, SDWord => -4, CharPtr => 4,
			Char => 1, Boolean => 1, Byte => 1, Handle => 4, Ptr => 4,
			DlpDateTimeType => 8);
%access = ( 1 => "byte", 2 => "short", 3 => "treble", 4 => "long",
           -1 => "sbyte", -2 => "sshort", -3 => "streble", -4 => "slong",
           8 => "date");


$items = undef;
$name = undef;
while(<>) {
	if (m!^\s*/\*\s*begin\s+struct\s+(\S+)!) {
		$name = $1;
		@e=();
		$pos = "0";
		$npos = 0;
		$items = $name;
		print;
	} elsif (defined $name and m!/*\s*end\s+struct\s+$endname!) {
		print;
		$name = undef;
		$items = undef;
	} elsif (defined $name and m!\*\s*\s+struct\s+access\s*\*/!) {
		$pos = "0";
		$npos = 0;
		print;
		foreach (@e) {
			my($pos, $type,$array,$item) = @$_;
			my($size,$access) = ($sizes{$type},$access{$sizes{$type}});
			if ($array)  {
				my(@array) = @$array;
				my($offset) = join("*",$size,@array[1..$#array]);
				print "#define get_${name}_$item(ptr,idx)	get_$access((ptr)+$pos+$offset*(idx))\n";
				print "#define set_${name}_$item(ptr,idx,val)	set_$access((ptr)+$pos+$offset*(idx),(val))\n";
				print "#define ptr_${name}_$item(ptr,idx)	((ptr)+$pos+$offset*(idx))\n";
			} else {
				print "#define get_${name}_$item(ptr)		get_$access((ptr)+$pos)\n";
				print "#define set_${name}_$item(ptr,val)	set_$access((ptr)+$pos,(val))\n";
			}
		}
		print "#define sizeof_$name		($len)\n";
		$items = undef;
	} elsif (defined $name and m!\*\s*struct\s+offsets\s*\*/!) {
		$pos = "0";
		$npos = 0;
		print;
		foreach (@e) {
			my($pos, $type,$array,$item) = @$_;
			my($size,$access) = ($sizes{$type},$access{$sizes{$type}});
			print "#define ${name}_$item		($pos)\n";
		}
		print "#define sizeof_$name		($len)\n";
		$items = undef;
	} elsif (defined $name and m!\*\s*struct\s+read\s+access\s*\*/!) {
		$pos = "0";
		$npos = 0;
		print;
		foreach (@e) {
			my($pos, $type,$array,$item) = @$_;
			my($size,$access) = ($sizes{$type},$access{$sizes{$type}});
			if ($array)  {
				my(@array) = @$array;
				my($offset) = join("*",$size,@array[1..$#array]);
				print "#define ${name}_$item(ptr,idx)	get_$access((ptr)+$pos+$offset*(idx))\n";
				print "#define ptr_${name}_$item(ptr,idx)	((ptr)+$pos+$offset*(idx))\n";
			} else {
				print "#define ${name}_$item(ptr)		get_$access((ptr)+$pos)\n";
			}
		}
		print "#define sizeof_$name		($len)\n";
		$items = undef;
	} elsif (defined $name and m!\*\s*struct\s+write\s+access\s*\*/!) {
		$pos = "0";
		$npos = 0;
		print;
		foreach (@e) {
			my($pos, $type,$array,$item) = @$_;
			my($size,$access) = ($sizes{$type},$access{$sizes{$type}});
			if ($array)  {
				my(@array) = @$array;
				my($offset) = join("*",$size,@array[1..$#array]);
				print "#define ${name}_$item(ptr,idx,val)	set_$access((ptr)+$pos+$offset*(idx),(val))\n";
				print "#define ptr_${name}_$item(ptr,idx)	((ptr)+$pos+$offset*(idx))\n";
			} else {
				print "#define ${name}_$item(ptr,val)		set_$access((ptr)+$pos,(val))\n";
			}
		}
		print "#define sizeof_$name		($len)\n";
		$items = undef;
	} elsif (defined $items and m!^\s*\*\s+(\S+?)((\[\S+?\])*)\s+(\S+)!) {
		my($type,$array,$name) = ($1,$2,$4);
		my(@array);
		push @array, $1 while $array =~ m/\[(.*?)\]/g;
		my($size,$access) = ($sizes{$type},$access{$sizes{$type}});

		if ($size>1 and $npos & 1) {
			$pos =~ s/(\*\d+)$/$1+0/;
			$pos =~ s/(\d+)$/$1+1/e;
			$npos++;
		}

		push @e, [$pos, $type, (@array ? \@array : undef),$name];

		s/([ \t]*)(\S+\(.*?\))?\s*$/(length($1) ? $1 : "\t\t") . "$access($pos)\n"/e;

		if (@array)  {
			my($offset) = join("*",$size,@array[1..$#array]);
			#$pos .= "+" . ;
			$x = $size;
			$textpos = 0;
			foreach (@array) {
				$x *= $_;
				if (/\D/) {
					$textpos = 1;
				}
			}
			if ($textpos) {
				$pos .= "+" .join("*", @array, $size);
				$npos += 0;
			} else {
				$pos =~ s/(\*\d+)$/$1+0/;
				$pos =~ s/(\d+)$/$1+$x/e;
				$npos += $x;
			}
		} else {
			$s = abs($size);
			$pos =~ s/(\*\d+)$/$1+$s/;
			$pos =~ s/(\d+)$/$1+$s/e;
			$npos += $s;
		}
		$len = $pos;
		print;
	} elsif (defined $name) {
		if (m!/\*\s*end\s+struct\s+$endname!) {
			print;
			$items = undef;
			$name= undef;
		}
	} else {
		print;
	}
}

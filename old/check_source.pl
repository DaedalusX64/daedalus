#!/usr/bin/perl


my $arg = shift || '';

scan_dir(".");

exit 0;

sub scan_dir
{
	my ($dir) = @_;

	#print "Scanning $dir\n";

	my @files = <$dir/*>;
	for my $file (@files) {

		if (-d $file) {
			scan_dir($file);
		} else {
			processFile($file);
		}
	}
}

sub processFile
{
	my ($file) = @_;

	# Ignore non C files.
	if ($file !~ /\.cpp$/ &&
		$file !~ /\.c$/ &&
		$file !~ /\.inl$/ &&
		$file !~ /\.h$/) {
		return;
	}

	# Ignore third-party code.
	if ($file =~ /third_party/) {
		return;
	}

	my $is_header = ($file =~ /.h$/);

	my $found_crlf = 0;
	my $found_lf = 0;
	my $missing_newline_at_end = 0;
	my $line_no = 0;

	my $guard = undef;
	my ($expected_guard) = $file =~ m/\/Source\/(.*)$/;

	if ($expected_guard) {
		$expected_guard =~ s/[\/.]/_/g;
		$expected_guard = uc ($expected_guard . '_');
	}

	my $lines = '';
	my $changed = 0;
	my $guard_lines = 0;

	open FILE, "<$file" or die("Couldn't open $file for reading");
	while(<FILE>) {
		my $line = $_;
		my $orig_line = $line;

		$line_no = $line_no+1;
		if ($line =~ /\r\n$/) { $found_crlf++; }
		elsif ($line =~ /\n$/) { $found_lf++; }
		else { $missing_newline_at_end = 1; }

		if ($is_header and $expected_guard) {
			if (!$guard) {
				my ($guard_text) = $line =~ m/#ifndef ([A-Z_][A-Z0-9_]+)/;
				if ($guard_text) {
					$guard = quotemeta $guard_text;
					if ($guard_text ne $expected_guard) {
						print("Found guard $guard_text at line $line_no, expected $expected_guard\n");
					}
				}
			}

			if ($guard) {
				if ($line =~ m/$guard/) {
					$guard_lines++;
					$line =~ s/$guard/$expected_guard/;
				}
			}
		}

		if ($line ne $orig_line) {
			$changed = 1;
		}

		$lines .= $line;
	}
	close FILE;

	if ($guard_lines != 0 and $guard_lines != 3) {
		print("$file: missing closing guard comment?\n");
	}

	if ($changed) {
		open FILE, ">$file" or die("Couldn't open $file for writing");
		binmode FILE;
		print FILE $lines;
		close FILE;
	}

	my $total_lines = $found_crlf+$found_lf;

	my $threshold = 0.0;

	if ($found_lf && $found_crlf) {
		if ($found_crlf > $found_lf) {
			printf("$file: MIXED! Windowsy (%f%% Windows, %f%% Unix)\n", $found_crlf*100.0/$total_lines, $found_lf*100.0/$total_lines) if $found_crlf*100.0/$total_lines > $threshold;
		} else {
			printf("$file: MIXED! Unixy (%f%% Unix, %f%% Windows)\n", $found_lf*100.0/$total_lines, $found_crlf*100.0/$total_lines) if $found_lf*100.0/$total_lines > $threshold;
		}
	}

	#print "$file: Windows\n" if $found_crlf && !$found_lf;
	#print "$file: Unix\n" if !$found_crlf && $found_lf;

	print "$file: Missing newline at end\n" if $missing_newline_at_end;
}

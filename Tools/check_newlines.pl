#!/usr/bin/perl


my ($arg) = shift || '';

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

	if ($file !~ /\.cpp$/ &&
		$file !~ /\.c$/ &&
		$file !~ /\.inl$/ &&
		$file !~ /\.h$/) {
		return;
	}

	my $found_crlf = 0;
	my $found_lf = 0;
	my $missing_newline_at_end = 0;
	my $line = 0;

	open FILE, "<$file" or die("Couldn't open $file");
	while(<FILE>) {
		my $l = $_;
		$line = $line+1;
		if ($l =~ /\r\n$/) { $found_crlf++; }
		elsif ($l =~ /\n$/) { $found_lf++; }
		else { $missing_newline_at_end = 1; }
	}
	close FILE;

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

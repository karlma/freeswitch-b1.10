use SMS::Ringtone::RTTTL::Parser;

our $NOTES = {
	      'A' =>[27.50, 55.00, 110.00, 220.00, 440.00, 880.00, 1760.00, 3520.00], 
	      'A#' =>[29.14, 58.27, 116.54, 233.08, 466.16, 932.33, 1864.66, 3729.31], 
	      'B' =>[30.87, 61.74, 123.47, 246.94, 493.88, 987.77, 1975.53, 3951.07], 
	      'C' =>[16.35, 32.70, 65.41, 130.81, 261.63, 523.25, 1046.50, 2093.00, 4186.01], 
	      'C#' =>[17.32, 34.65, 69.30, 138.59, 277.18, 554.37, 1108.73, 2217.46, 4434.92], 
	      'D' =>[18.35, 36.71, 73.42, 146.83, 293.66, 587.33, 1174.66, 2349.32, 4698.64], 
	      'D#' =>[19.45, 38.89, 77.78, 155.56, 311.13, 622.25, 1244.51, 2489.02, 4978.03], 
	      'E' =>[20.60, 41.20, 82.41, 164.81, 329.63, 659.26, 1318.51, 2637.02], 
	      'F' =>[21.83, 43.65, 87.31, 174.61, 349.23, 698.46, 1396.91, 2793.83], 
	      'F#' =>[23.12, 46.25, 92.50, 185.00, 369.99, 739.99, 1479.98, 2959.96], 
	      'G' =>[24.50, 49.00, 98.00, 196.00, 392.00, 783.99, 1567.98, 3135.96], 
	      'G#' =>[25.96, 51.91, 103.83, 207.65, 415.30, 830.61, 1661.22, 3322.44], 
	      'P' =>[0, 0, 0, 0, 0, 0, 0, 0]
	     };


my $file = shift or die "no file\n";
my $rtttl;
my $cr = "\n";

if ($file eq "-nocr") {
  $cr = "";
  $file = shift;
}

if ($file eq "-") {
  $rtttl = <STDIN>;
} else {
  $rtttl = `cat $file`;
}

$rtttl =~ s/\n//g;

#$rtttl =~ tr/[a-z]/[A-Z]/;
#print  $rtttl;

my $r = new SMS::Ringtone::RTTTL::Parser($rtttl);
# ....or....
#my $r = new SMS::Ringtone::RTTTL::Parser($rtttl,{'STRICT_NOTE_PART_ORDER' => 0});


# Check for errors
if ($r->has_errors()) {
  print STDERR "The following RTTTL errors were found:\n";
  foreach (@{$r->get_errors()}) {
    print STDERR "$_\n";
  }
  #exit;
}

# Dump parse results to STDOUT
#$r->puke();

$all = $r->get_notes();

$ms_per_beat = int (60000 / $r->get_bpm());

foreach (@{$all}) {

  my $ms = $ms_per_beat * (1 / $_->[0]) * 4;

  #print STDERR "$_->[0] $_->[1] $_->[2] $_->[3]\n";
  
  print "%($ms,0,$NOTES->{$_->[1]}->[$_->[2]]);" . $cr;

}



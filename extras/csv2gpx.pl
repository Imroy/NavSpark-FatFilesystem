#!/usr/bin/perl -w

# This program converts a CSV file produced by the 'logger' sketch into a GPX file.

# Under Debian/Ubuntu make sure these Perl libraries are installed:
#
# sudo apt-get install libdatetime-perl libdatetime-format-iso8601-perl libtext-csv-perl
#

use strict;
use IO::Handle;
use DateTime;
use DateTime::Duration;
use DateTime::Format::ISO8601;
use Text::CSV;

sub gpx_header {
  print << "EOX";
<?xml version="1.0" encoding="utf-8"?>
<gpx xmlns="http://www.topografix.com/GPX/1/1"
     xmlns:xml="http://www.w3.org/XML/1998/namespace"
     xmlns:xsd="http://www.w3.org/2001/XMLSchema"
     xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
     xsi:schemaLocation="http://www.topografix.com/GPX/1/1 http://www.topografix.com/GPX/1/1/gpx.xsd"
     version="1.1" creator="NavSpark logger, csv2gpx">
EOX
}

sub gpx_start_track {
  my ($num) = @_;
  $num ||= 0;
  print << "EOX";
  <trk>
    <number>$num</number>
EOX
}

sub gpx_start_trackseg {
  print << "EOX";
    <trkseg>
EOX
}

my @fixtypes = qw{none none 2d 3d dgps};

sub gpx_track_point {
  my ($pt) = @_;
  my $time = $pt->{time}->strftime('%FT%T.%03NZ');

  print << "EOX";
      <trkpt lat="$pt->{latitude}" lon="$pt->{longitude}">
        <ele>$pt->{altitude}</ele>
        <time>$time</time>
        <fix>$pt->{fix}</fix>
        <sat>$pt->{numsats}</sat>
        <hdop>$pt->{hdop}</hdop>
        <vdop>$pt->{vdop}</vdop>
        <pdop>$pt->{pdop}</pdop>
      </trkpt>
EOX
}

sub gpx_end_trackseg {
  print << "EOX";
    </trkseg>
EOX
}

sub gpx_end_track {
  print << "EOX";
  </trk>
EOX
}

sub gpx_footer {
  print << "EOX";
</gpx>
EOX
}

my $input;
if (scalar @ARGV > 0) {
  open $input, "<", $ARGV[0] or die "$ARGV[0]: $!";
} else {
  $input = IO::Handle->new();
  $input->fdopen(fileno(STDIN),"r") or die "stdin: $!";
}

&gpx_header;

my $csv = Text::CSV->new ({ binary => 1, eol => $/ });
my @column_names = @{$csv->getline($input)};
foreach (@column_names) {
  s{^\s+} {};
  s{\s+$} {};
  $_ = lc;
}

my $track_num = 1;
gpx_start_track($track_num);
&gpx_start_trackseg;
my $prev_time;
my $max_interval = DateTime::Duration->new(minutes => 5);
my $trackseg_count = 0;
while (my $row = $csv->getline($input)) {
  if ((scalar @{$row} == 1) && ($row->[0] eq '')) {
    if ($trackseg_count > 0) {
      &gpx_end_trackseg;
      &gpx_start_trackseg;
      $trackseg_count = 0;
    }
    next;
  }

  foreach (@{$row}) {
    s{^\s+} {};
    s{\s+$} {};
  }

  my %pt;
  @pt{@column_names} = @{$row};
  $pt{fix} = $fixtypes[delete $pt{type}];
  $pt{time} = DateTime::Format::ISO8601->parse_datetime($pt{date} . 'T' . $pt{time} . 'Z');
  delete $pt{date};
  if ((defined $prev_time) && ($pt{time} > $prev_time + $max_interval)) {
    &gpx_end_track;
    $track_num++;
    &gpx_start_track($track_num);
  }
  $prev_time = $pt{time};

  gpx_track_point(\%pt);
  $trackseg_count++;
}
&gpx_end_track;
&gpx_footer;

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

my $gpx_tag_level = 0;	# 0 => doc, 1 => gpx root, 2 => trk, 3 => trkseg

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
  $gpx_tag_level = 1;
}

my $track_num = 1;

sub gpx_start_track {
  &gpx_end_track if ($gpx_tag_level > 1);
  &gpx_header if ($gpx_tag_level < 1);

  print << "EOX";
  <trk>
    <number>$track_num</number>
EOX
  $gpx_tag_level = 2;
}

sub gpx_start_trackseg {
  &gpx_end_trackseg if ($gpx_tag_level > 2);
  &gpx_start_track if ($gpx_tag_level < 2);

  print << "EOX";
    <trkseg>
EOX
  $gpx_tag_level = 3;
}

my @fixtypes = qw{none none 2d 3d dgps};

sub gpx_track_point {
  my ($pt) = @_;
  my $time = $pt->{time}->strftime('%FT%T.%03NZ');

  &gpx_start_trackseg if ($gpx_tag_level < 3);

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
  if ($gpx_tag_level == 3) {
    print << "EOX";
    </trkseg>
EOX
    $gpx_tag_level = 2;
  }
}

sub gpx_end_track {
  &gpx_end_trackseg if ($gpx_tag_level > 2);

  if ($gpx_tag_level == 2) {
    print << "EOX";
  </trk>
EOX
    $track_num++;
    $gpx_tag_level = 1;
  }
}

sub gpx_footer {
  &gpx_end_track if ($gpx_tag_level > 1);

  if ($gpx_tag_level == 1) {
    print << "EOX";
</gpx>
EOX
    $gpx_tag_level = 0;
  }
}

sub read_csv_filehandle {
  my ($input) = @_;

  my $csv = Text::CSV->new ({ binary => 1, eol => $/ });
  my @column_names = @{$csv->getline($input)};
  foreach (@column_names) {
    s{^\s+} {};
    s{\s+$} {};
    $_ = lc;
  }

  my $prev_time;
  my $max_interval = DateTime::Duration->new(minutes => 5);
  my $trackseg_count = 0;
  while (my $row = $csv->getline($input)) {
    if ((scalar @{$row} == 1) && ($row->[0] eq '')) {
      &gpx_end_trackseg;
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
    }
    $prev_time = $pt{time};

    gpx_track_point(\%pt);
  }
}

if (scalar @ARGV > 0) {
  foreach my $filename (@ARGV) {
    open my $input, "<", $filename or die "$filename: $!";
    read_csv_filehandle($input);
  }
} else {
  my $input = IO::Handle->new();
  $input->fdopen(fileno(STDIN),"r") or die "stdin: $!";
  read_csv_filehandle($input);
}

my $csv = Text::CSV->new ({ binary => 1, eol => $/ });
my @column_names = @{$csv->getline($input)};
foreach (@column_names) {
  s{^\s+} {};
  s{\s+$} {};
  $_ = lc;
}

my $prev_time;
my $max_interval = DateTime::Duration->new(minutes => 5);
my $trackseg_count = 0;
while (my $row = $csv->getline($input)) {
  if ((scalar @{$row} == 1) && ($row->[0] eq '')) {
    &gpx_end_trackseg;
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
  }
  $prev_time = $pt{time};

  gpx_track_point(\%pt);
}

&gpx_footer;

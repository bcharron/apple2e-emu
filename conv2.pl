#!/usr/bin/perl -w
# To convert the opcode list into an assembler table

my %hash;

for (my $x = 0; $x < 256; $x++)
{
    my $s = sprintf("%02X", $x);
    my $txt = sprintf("{ %-21s1, 1 }, // 0x$s", "\"???\", ");

    $hash{$s} = $txt;
}

while (<STDIN>)
{
# |ADC|Immediate|ADC #Oper|69|2|2
    s/[\r\n]$//g;
    #chomp($_);
    my ($a, $b, $addr_mode, $instr, $opcode, $len, $cycles) = split(/\|/);

    $instr =~ s/Zpg/\$%02X/g;
    $instr =~ s/Abs/\$%02X%02X/g;
    $instr =~ s/Oper/\$%02X/g;

    if (length($opcode) == 1) {
	$opcode = "0$opcode";
    }

    my $name = "\"$instr\", ";
    my $txt = sprintf("{ %-21s$len, $cycles }, // 0x$opcode", $name);

    $hash{$opcode} = $txt;
}

foreach my $key (sort keys %hash) {
    my $val = $hash{$key};
    print "$val\n";
}

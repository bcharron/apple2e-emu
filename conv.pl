#!/usr/bin/perl -w
# To convert the opcode list into an assembler table

my %hash;

for (my $x = 0; $x < 256; $x++)
{
    my $s = sprintf("%02X", $x);
    my $txt = sprintf("{ %-19s1, 1 }, // 0x$s", "\"???\", ");

    $hash{$s} = $txt;
}

while (<STDIN>)
{
    my ($addr_mode, $instr, $operand, $opcode, $len, $cycles);
    chomp;
    my @fields = split(/ +/);

    if ($#fields == 4) {
	($addr_mode, $instr, $opcode, $len, $cycles) = @fields;
	$operand = "";
    } else {
	($addr_mode, $instr, $operand, $opcode, $len, $cycles) = @fields;
    }

    if ($addr_mode =~ /absolute/) {
	$operand =~ s/oper/\$%02X%02X/g;
    } else {
	$operand =~ s/oper/\$%02X/g;
    }

    $cycles =~ s/\*//g;

    my $txt;

    if ($operand eq "") {
	my $name = "\"$instr\", ";
	$txt = sprintf("{ %-19s$len, $cycles }, // 0x$opcode", $name);
    } else {
	my $name = "\"$instr $operand\", ";
	$txt = sprintf("{ %-19s$len, $cycles }, // 0x$opcode", $name);
    }

    $hash{$opcode} = $txt;
}

foreach my $key (sort keys %hash) {
    my $val = $hash{$key};
    print "$val\n";
}

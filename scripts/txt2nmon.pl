#!/usr/bin/perl

$a = 0x80000000;

while (<>) {
	chop;
	printf("expect \"nmon> \"\n");
	printf("send \"w%08x$_\"\n", $a);
	$a = $a + 4;
}
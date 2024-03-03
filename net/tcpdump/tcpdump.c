// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: 2024 Antony Pavlov <antonynpavlov@gmail.com>

#define pr_fmt(fmt) "tcpdump: " fmt

#include <common.h>
#include <globalvar.h>
#include <magicvar.h>

#include "netdissect.h"

void ndo_set_function_pointers(netdissect_options *ndo);
void ether_if_print(netdissect_options *ndo, const struct pcap_pkthdr *h,
			const u_char *p);
void pretty_print_packet(netdissect_options *ndo, const struct pcap_pkthdr *h,
				const u_char *sp, u_int packets_captured);

void tcpdump(unsigned char *pkt, int len);

static int tcpdump_enabled;
static int tcpdump_Xflag;
static int tcpdump_eflag;
static int tcpdump_vflag;

static int tcpdump_init(void)
{
	globalvar_add_simple_bool("tcpdump.enabled", &tcpdump_enabled);
	globalvar_add_simple_int("tcpdump.Xflag", &tcpdump_Xflag, "%d");
	globalvar_add_simple_bool("tcpdump.eflag", &tcpdump_eflag);
	globalvar_add_simple_int("tcpdump.vflag", &tcpdump_vflag, "%d");

	tcpdump_enabled = 0;
	tcpdump_Xflag = 0;
	tcpdump_eflag = 0;
	tcpdump_vflag = 0;

	return 0;
}
device_initcall(tcpdump_init);

BAREBOX_MAGICVAR(global.tcpdump.enabled,
		"If true, dump network packets with tcpdump library");
BAREBOX_MAGICVAR(global.tcpdump.Xflag, "0-2, print packet in hex/ASCII");
BAREBOX_MAGICVAR(global.tcpdump.eflag, "print ethernet header");
BAREBOX_MAGICVAR(global.tcpdump.vflag, "verbosity level");

void tcpdump(unsigned char *pkt, int len)
{
	struct pcap_pkthdr h;

	netdissect_options Ndo;
	netdissect_options *ndo = &Ndo;

	if (!tcpdump_enabled)
		return;

	memset(ndo, 0, sizeof(netdissect_options));
	ndo_set_function_pointers(ndo);

	ndo->program_name = "tcpdump";

	ndo->ndo_snapend = pkt + len;
	ndo->ndo_packetp = pkt;

	ndo->ndo_protocol = "";

	ndo->ndo_nflag = 0; /* convert addresses to names */
	ndo->ndo_lengths = 1; /* print packet header caplen and len */

	ndo->ndo_Xflag = tcpdump_Xflag;
	ndo->ndo_eflag = tcpdump_eflag;
	ndo->ndo_vflag = tcpdump_vflag;

	switch (setjmp(ndo->ndo_early_end)) {
	case 0:
		/* Print the packet. */
		ndo->ndo_if_printer = ether_if_print;
		h.len = len;
		h.caplen = len;
		pretty_print_packet(ndo, &h, pkt, 1);
		break;
	case ND_TRUNCATED:
		pr_err("A printer quit because the packet was truncated\n");
		/* A printer quit because the packet was truncated; report it */
		nd_print_trunc(ndo);
		/* Print the full packet */
		ndo->ndo_ll_hdr_len = 0;
		break;
	}

	printf("\n");
}

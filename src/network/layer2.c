/**
 * @file layer2.c
 *
 * @brief Layer 2 handling.
 * @author David Suárez
 * @date Sun, 28 Oct 2018 16:14:56 +0100
 *
 * Copyright (c) 2018 David Suárez.
 * Email: david.sephirot@gmail.com
 *
 */

#include "compat/compat.h"

#include <string.h>

#ifdef __FreeBSD__
#include <netinet/in_systm.h>
#include <netinet/in.h>
#else
  #ifndef __CYGWIN__
    #include <netinet/ether.h>
	#include <netinet/ip6.h>
  #endif
#endif

#include <netinet/ip.h>

/*
 * Freebsd and Cygwin doesn't define 'ethhdr'
 */
#if defined(__FreeBSD__) || defined(__CYGWIN__)

#define ETH_ALEN	6			/* Octets in one ethernet addr	 */
#define ETH_P_IP	0x0800		/* Internet Protocol packet	*/
#define ETH_P_ARP	0x0806		/* Address Resolution packet	*/
#define ETH_P_IPV6	0x86DD		/* IPv6 over bluebook		*/

struct ethhdr {
	unsigned char   h_dest[ETH_ALEN];
	unsigned char   h_source[ETH_ALEN];
	u_int16_t       h_proto;
} __attribute__((packed));
#endif

#include <pcap.h>                 /* for DLT_IEEE802_11_RADIO, DLT_IEEE802_11 */

#include "common/log.h"
#include "pcap_engine.h"          /* for datalink_info_t */
#include "layer2.h"

/* ETH_P_PAE is named ETHERTYPE_PAE in freebsd, define it  */
#ifndef ETH_P_PAE
#define ETH_P_PAE 0x888E
#endif

struct ieee80211_radiotap_header {
	u_int8_t        it_version;     /* set to 0 */
	u_int8_t        it_pad;
	u_int16_t       it_len;         /* entire length */
	u_int32_t       it_present;     /* fields present */
};

struct ieee80211_frame {
    u_int16_t fc;
    u_int16_t wi_duration;
    u_int8_t wi_add1[6];
    u_int8_t wi_add2[6];
    u_int8_t wi_add3[6];
    u_int16_t wi_sequenceControl;
    // u_int8_t wi_add4[6];
    //unsigned int qosControl:2;
    //unsigned int frameBody[23124];
};

struct frame_control {
	unsigned protocol:2;
	unsigned type:2;
	unsigned subtype:4;
	unsigned to_ds:1;
	unsigned from_ds:1;
	unsigned more_frag:1;
	unsigned retry:1;
	unsigned pwr_mgt:1;
	unsigned more_data:1;
	unsigned wep:1;
	unsigned order:1;
};

/* SNAP LLC header format */
struct snap_header {
  u_int8_t dsap;
  u_int8_t ssap;
  u_int8_t ctl;
  u_int8_t org1;
  u_int8_t org2;
  u_int8_t org3;
  u_int16_t ether_type;          /* ethernet type */
};

/*
 * 802.11 is a little different from most other L2 protocols:
 *   - Not all frames are data frames (control, data, management) (data frame == L3 or higher included)
 *   - Not all data frames have data (QoS frames are "data" frames, but have no L3 header)
 *   - L2 header is 802.11 + an 802.2/802.2SNAP header
 */
static int parse_ieee80211(const u_char *pkt, uint32_t caplen,
		unsigned short *llnextproto, int *offsetnext)
{
	struct ieee80211_frame *_80211_header;
	struct frame_control *control;

	_80211_header = (struct ieee80211_frame *) pkt;
	control       = (struct frame_control *) &_80211_header->fc;

	/* check protocol version */
	if (control->protocol != 0) {
		return -1;
	}

	/* check for data frame */
	if (control->type != 2) {
		return -1;
	}

	/* data and QoS frames */
	if (control->subtype != 0 && control->subtype != 8) {
		return -1;
	}

	/* check encrypted data  */
	if (control->wep == 1) {
		return -1;
	}

	size_t header_len = sizeof(struct ieee80211_frame);

	/* QoS frame, add control bits */
	if (control->subtype == 8)
		header_len += 2;

	/*
	 * To  From   a1    a2    a3    a4
	 * --------------------------------
	 *  0    0   dst   src   bssid  x
	 *  0    1   dst   bssid src    x
	 *  1    0   bssid src   dst    x
	 *  1    1   rcv   trx   dst   src
	 */
	if (control->to_ds == 1 && control->from_ds == 1) {
		header_len += 6;
	}

	/* get the snap header */
	struct snap_header *llc_header = (struct snap_header *)
			(pkt + header_len);

	/* verify the header is 802.2SNAP (8 bytes) not 802.2 (3 bytes) */
	if (llc_header->dsap != 0xAA || llc_header->ssap != 0xAA) {
		return -1;
	}

	/* get the offset of the next header and protocol */
	*offsetnext = header_len + sizeof (struct snap_header);
	*llnextproto = ntohs(llc_header->ether_type);

    return 0;
}

int handle_link_layer(datalink_info_t *info, const u_char *pkt, uint32_t caplen,
		uint8_t *nextproto, int *offsetnext)
{
	unsigned short llnextproto = 0;
	struct ieee80211_radiotap_header *radiotap_header;
	const u_char *next_header;
	struct ethhdr *eptr;
	size_t remaining_len;

	switch (info->type) {

		/* Ethernet packet */
		case DLT_EN10MB:
			eptr = (struct ethhdr *) pkt;

			/* get the offset of the next header and the protocol */
			*offsetnext = sizeof(struct ethhdr);
			llnextproto = ntohs(eptr->h_proto);

			break;

		/* radiotap packet */
		case DLT_IEEE802_11_RADIO:
			/*
			 * A radiotap packet consist of a radiotap information header followed by
			 * an 802.11 packet header.
			 */
			radiotap_header = (struct ieee80211_radiotap_header *) pkt;

			/* calculate the offset of next 802.11 header */
			next_header     = pkt + radiotap_header->it_len;
			remaining_len   = caplen - radiotap_header->it_len;

			if (parse_ieee80211(next_header, remaining_len, &llnextproto, offsetnext)) {
				return -1;
			}

			*offsetnext += radiotap_header->it_len;

			break;

		case DLT_IEEE802_11:
			if (parse_ieee80211(pkt, caplen, &llnextproto, offsetnext)) {
				return -1;
			}

			break;

		default:
			/* unsupported packet */
			log_msg(LOG_WARNING, "link-level (%s) header is not supported",
					info->name);
			return -1;
	}

	/* check next header protocol */
	switch (llnextproto) {

		case ETH_P_IP: /* Next header is IP */
			*nextproto = IPPROTO_IP;
			break;

		case ETH_P_IPV6: /* Next header is IPv6 */
			*nextproto = IPPROTO_IPV6;
			break;

		case ETH_P_ARP:
		case ETH_P_PAE:
			return -1;

		default:
			log_msg(LOG_WARNING,
					"link-level next protocol (%hd) is not supported",
					llnextproto);

			return -1;
	}


	return 0;
}

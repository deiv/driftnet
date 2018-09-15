/*
 * pcap.h:
 * Network packet capture handling.
 *
 * Copyright (c) 2001 Chris Lightfoot.
 * Email: chris@ex-parrot.com; WWW: http://www.ex-parrot.com/~chris/
 *
 * Copyright (c) 2012 David Suárez.
 * Email: david.sephirot@gmail.com
 *
 */

#ifdef HAVE_CONFIG_H
    #include <config.h>
#endif

#include "compat.h"

#include <stdio.h>
#include <stdlib.h> /* On many systems (Darwin...), stdio.h is a prerequisite. */
#include <string.h>
#include <unistd.h>
#include <signal.h> /* sig_atomic */
#include <sys/socket.h> /* On Darwin, stdlib.h is a prerequisite.  */
#include <netinet/in.h> /* needs to be before <arpa/inet.h> on OpenBSD */
#include <arpa/inet.h>
#ifdef HAVE_LIMITS_H
    #include <limits.h>
#endif
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>

#include <pcap.h>

#include "log.h"
#include "driftnet.h"
#include "media.h"
#include "connection.h"
#include "options.h"
#include "layer3.h"
#include "layer2.h"

#include "packetcapture.h"

static void process_packet(u_char *user, const struct pcap_pkthdr *hdr, const u_char *pkt);
static datalink_info_t get_datalink_info(pcap_t *pcap);

#define SNAPLEN 262144      /* largest chunk of data we accept from pcap */
#define WRAPLEN 262144      /* out-of-order packet margin */

/* ugh. */
static pcap_t *pc = NULL;
static datalink_info_t datalink_info;

void packetcapture_list_interfaces(void)
{ 
	pcap_if_t *alldevs;
    pcap_if_t *d;
    int i=0;
    char errbuf[PCAP_ERRBUF_SIZE];
    
    if (pcap_findalldevs(&alldevs, errbuf) == -1) {
        log_msg(LOG_ERROR, "Error listing interfaces: %s", errbuf);
        unexpected_exit(1);
    }
    
    for(d = alldevs; d != NULL; d= d->next) {
		log_msg(LOG_SIMPLY, "- %d.: %s", ++i, d->name);
		
		if (d->description) {
			log_msg(LOG_SIMPLY, "\t%s", d->description);
		} else {
			log_msg(LOG_SIMPLY, "");
		}
    }
    
	
}

void packetcapture_open_offline(char* dumpfile)
{
    char ebuf[PCAP_ERRBUF_SIZE];

    if (!(pc = pcap_open_offline(dumpfile, ebuf))) {
        log_msg(LOG_ERROR, "pcap_open_offline: %s", ebuf);
        unexpected_exit (-1);
    }

    log_msg(LOG_INFO, "reading packets from %s", dumpfile);

    datalink_info = get_datalink_info(pc);
}

void check_pcap_error(int error, char* msg)
{
    if (error != 0) {
        log_msg(LOG_ERROR, "can't set option: %s", msg);
        unexpected_exit (-1);      
    }
}

void packetcapture_open_live(char* interface, char* filterexpr, int promisc, int monitor_mode)
{
    char ebuf[PCAP_ERRBUF_SIZE];
    struct bpf_program filter;
    int error;
    
    pc = pcap_create(interface, ebuf);
    
    if (pc == NULL) {
        log_msg(LOG_ERROR, "pcap_open_live: %s", ebuf);

        if (getuid() != 0)
            log_msg(LOG_ERROR, "perhaps you need to be root?");
        else if (!interface)
            log_msg(LOG_ERROR, "perhaps try selecting an interface with the -i option?");

        unexpected_exit (-1);   
    }
    
    check_pcap_error(pcap_set_rfmon(pc, monitor_mode), "pcap_set_rfmon");
    check_pcap_error(pcap_set_promisc(pc, promisc), "pcap_set_promisc");
    check_pcap_error(pcap_set_snaplen(pc, SNAPLEN), "pcap_set_snaplen");
    check_pcap_error(pcap_set_timeout(pc, 1000), "pcap_set_timeout");
    
    error = pcap_activate(pc);
    
    if (error < 0) {
        log_msg(LOG_ERROR, "pcap_activate: %s", pcap_statustostr(error));
        unexpected_exit (-1);
    }

    /* Only apply a filter to live packets. Is this right? */
    if (pcap_compile(pc, &filter, filterexpr, 1, 0) == -1) {
        log_msg(LOG_ERROR, "pcap_compile: %s", pcap_geterr(pc));
        unexpected_exit (-1);
    }

    if (pcap_setfilter(pc, &filter) == -1) {
        log_msg(LOG_ERROR, "pcap_setfilter: %s", pcap_geterr(pc));
        unexpected_exit (-1);
    }

    log_msg(LOG_INFO, "listening on %s%s",
        interface ? interface : "all interfaces",
        promisc ? " in promiscuous mode" : "");

    datalink_info = get_datalink_info(pc);
}

void packetcapture_close(void)
{
	if (pc != NULL)
		pcap_close(pc);
}

char* get_default_interface(void)
{
    char ebuf[PCAP_ERRBUF_SIZE];
    char *interface;

    interface = pcap_lookupdev(ebuf);

    if (!interface) {
        log_msg(LOG_ERROR, "pcap_lookupdev: %s", ebuf);
        log_msg(LOG_ERROR, "try specifying an interface with -i");
        log_msg(LOG_ERROR, "or a pcap capture file with -f");
        unexpected_exit (-1);
    }

    return interface;
}

void packetcapture_dispatch(void)
{
    pcap_dispatch(pc, -1, process_packet, NULL);
}

/* process_packet:
 * Callback which processes a packet captured by libpcap. */
void process_packet(u_char *user, const struct pcap_pkthdr *hdr, const u_char *pkt)
{
    struct tcphdr tcp;
    int off, len, delta;
    connection *C, c;
    struct sockaddr_storage src, dst;
    struct sockaddr *s, *d;
    uint8_t proto;
    
    s = (struct sockaddr *)&src;
    d = (struct sockaddr *)&dst;

    if (handle_link_layer(&datalink_info, pkt, hdr->caplen, &proto, &off))
    	return;
	
    if (layer3_find_tcp(pkt, proto, &off, s, d, &tcp))
    	return;

    len = hdr->caplen - off;

    /* XXX fragmented packets and other nasties. */

    /* try to find the connection slot associated with this. */
    C = find_connection(s, d);

    /* no connection at all, so we need to allocate one. */
    if (!C) {
        log_msg(LOG_INFO, "new connection: %s", connection_string(s,d));
        C = alloc_connection();
        *C = connection_new(s, d);
        /* This might or might not be an entirely new connection (SYN flag
         * set). Either way we need a sequence number to start at. */
        (*C)->isn = ntohl(tcp.th_seq);
    }

    /* Now we need to process this segment. */
    c = *C;
    delta = 0;/*tcp.syn ? 1 : 0;*/

    /* NB (STD0007):
     *    SEG.LEN = the number of octets occupied by the data in the
     *    segment (counting SYN and FIN) */
#if 0
    if (tcp.syn)
        /* getting a new isn. */
        c->isn = htonl(tcp.seq);
#endif

    if (tcp.th_flags & TH_RST) {
        /* Looks like this connection is bogus, and so might be a
         * connection going the other way. */
        log_msg(LOG_INFO, "connection reset: %s", connection_string(s, d));

        connection_delete(c);
        *C = NULL;

        if ((C = find_connection(d, s))) {
            connection_delete(*C);
            *C = NULL;
        }

        return;
    }

    if (len > 0) {
        /* We have some data in the packet. If this data occurred after
         * the first data we collected for this connection, then save it
         * so that we can look for images. Otherwise, discard it. */
        unsigned int offset;

        offset = ntohl(tcp.th_seq);

        /* Modulo 2**32 arithmetic; offset = seq - isn + delta. */
        if (offset < (c->isn + delta))
            offset = 0xffffffff - (c->isn + delta - offset);
        else
            offset -= c->isn + delta;

        if (offset > c->len + WRAPLEN) {
            /* Out-of-order packet. */
            log_msg(LOG_INFO, "out of order packet: %s", connection_string(s, d));
        } else {
            connection_push(c, pkt + off, offset, len);
            extract_media(c);
        }
    }
    if (tcp.th_flags & TH_FIN) {
        /* Connection closing; mark it as closed, but let sweep_connections
         * free it if appropriate. */
        log_msg(LOG_INFO, "connection closing: %s, %d bytes transferred", connection_string(s, d), c->len);
        c->fin = 1;
    }

    /* sweep out old connections */
    sweep_connections();
}


datalink_info_t get_datalink_info(pcap_t *pcap)
{
	datalink_info_t info;

    //pkt_offset = get_link_level_hdr_length(pcap_datalink(pc));

	info.type = pcap_datalink(pcap);
	info.name = pcap_datalink_val_to_name(pcap_datalink(pcap));

    //log_msg(LOG_INFO, "link-level header length is %d bytes", pkt_offset);

    return info;
}

#if 0
/* get_link_level_hdr_length:
 * Find out how long the link-level header is, based on the datalink layer
 * type. This is based on init_linktype in the libpcap distribution; I
 * don't know why libpcap doesn't expose the information directly. The
 * constants here are taken from 0.6.2, but I've added #ifdefs in the hope
 * that it will still compile with earlier versions. */
int get_link_level_hdr_length(int type)
{
    switch (type) {
        case DLT_EN10MB:
            return 14;

        case DLT_SLIP:
            return 16;

        case DLT_SLIP_BSDOS:
            return 24;

        case DLT_NULL:
#ifdef DLT_LOOP
        case DLT_LOOP:
#endif
            return 4;

        case DLT_PPP:
#ifdef DLT_C_HDLC
        case DLT_C_HDLC:
#endif
#ifdef DLT_PPP_SERIAL
        case DLT_PPP_SERIAL:
#endif
            return 4;

        case DLT_PPP_BSDOS:
            return 24;

        case DLT_FDDI:
            return 21;

        case DLT_IEEE802:
            return 22;

#ifdef DLT_ATM_RFC1483
        case DLT_ATM_RFC1483:
            return 8;
#endif

#ifdef DLT_PRISM_HEADER
        case DLT_PRISM_HEADER:
            return 32;
#endif

        case DLT_RAW:
            return 0;

#ifdef DLT_ATM_CLIP
        case DLT_ATM_CLIP:	/* Linux ATM defines this */
            return 8;
#endif

#ifdef DLT_LINUX_SLL
        case DLT_LINUX_SLL:	/* fake header for Linux cooked socket */
            return 16;
#endif

#ifdef DLT_IEEE802_11           /* 802.11 wireless ethernet */
        case DLT_IEEE802_11:
            return 32; /* 20030606 email from Nikhil Bobb */ /*44; */
#endif

        default:;
    }
    log_msg(LOG_ERROR, "unknown data link type %d", type);
    unexpected_exit(1);
}
#endif

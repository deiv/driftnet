/*
 * driftnet.c:
 * Pick out images from passing network traffic.
 *
 * Copyright (c) 2001 Chris Lightfoot. All rights reserved.
 * Email: chris@ex-parrot.com; WWW: http://www.ex-parrot.com/~chris/
 *
 */

static const char rcsid[] = "$Id: driftnet.c,v 1.11 2002/02/15 12:35:11 chris Exp $";

#undef NDEBUG

#include <assert.h>
#include <errno.h>
#include <pcap.h>
#include <linux/if_ether.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>
#include <sys/stat.h>

#include "driftnet.h"

connection *slots;
unsigned int slotsused, slotsalloc;

/* ugh. */
pcap_t *pc;

/* image.c */
unsigned char *find_gif_image(const unsigned char *data, const size_t len, unsigned char **gifdata, size_t *giflen);
unsigned char *find_jpeg_image(const unsigned char *data, const size_t len, unsigned char **jpegdata, size_t *jpeglen);

connection connection_new(const struct in_addr *src, const struct in_addr *dst, const short int sport, const short int dport) {
    connection c = (connection)calloc(1, sizeof(struct _connection));
    c->src = *src;
    c->dst = *dst;
    c->sport = sport;
    c->dport = dport;
    c->alloc = 16384;
    c->data = c->gif = c->jpeg = (unsigned char*)malloc(c->alloc);
    c->last = time(NULL);
    return c;
}

void connection_delete(connection c) {
    free(c->data);
    free(c);
}

void connection_push(connection c, const unsigned char *data, unsigned int off, unsigned int len) {
    size_t goff = c->gif - c->data, joff = c->jpeg - c->data;
/*    printf("connection_push(%p, %p, %u, %u)\n", c, data, off, len);*/
    assert(c->alloc > 0);
    if (off + len > c->alloc) {
        /* Allocate more memory. */
        while (off + len > c->alloc) {
            c->alloc *= 2;
            c->data = (unsigned char*)realloc(c->data, c->alloc);
        }
    }
    c->gif = c->data + goff;
    c->jpeg = c->data + joff;
    memcpy(c->data + off, data, len);

    if (off + len > c->len) c->len = off + len;
    c->last = time(NULL);
}

pid_t dpychld;
int dpychld_fd;

int dodisplay(int argc, char *argv[]);

void image_notify(int len, char *filename) {
    struct pipemsg m = {0};
    m.len = len;
    strcpy(m.filename, filename);
    write(dpychld_fd, &m, sizeof(m));
}

void connection_harvest_images(connection c) {
    unsigned char *ptr, *oldptr, *img;
    size_t ilen;

    /* look for GIF files */
    ptr = c->gif;
    oldptr = NULL;

    while (ptr != oldptr) {
        oldptr = ptr;
        ptr = find_gif_image(ptr, c->len - (ptr - c->data), &img, &ilen);

        if (img) {
            char buf[128];
            int fd;
            sprintf(buf, "/tmp/imgdump/%d.%d.gif", (int)time(NULL), rand());
            fd = open(buf, O_WRONLY|O_CREAT|O_EXCL, 0644);
            write(fd, img, ilen);
            close(fd);
/*            printf("saved GIF data of length %u in %s\n", ilen, buf);*/
            image_notify(ilen, buf);
        }
    }

    c->gif = ptr;

    /* look for JPEG files */
    ptr = c->jpeg;
    oldptr = NULL;
    
    while (ptr != oldptr) {
        oldptr = ptr;
        ptr = find_jpeg_image(ptr, c->len - (ptr - c->data), &img, &ilen);

        if (img) {
            char buf[128];
            int fd;
            sprintf(buf, "/tmp/imgdump/%d.%d.jpg", (int)time(NULL), rand());
            fd = open(buf, O_WRONLY|O_CREAT|O_EXCL, 0644);
            write(fd, img, ilen);
            close(fd);
/*            printf("saved JPEG data of length %u in %s\n", ilen, buf);*/
            image_notify(ilen, buf);
        }
    }

    c->jpeg = ptr;
}

connection *alloc_connection(void) {
    connection *C;
    for (C = slots; C < slots + slotsalloc; ++C) {
        if (!*C) return C;
    }
    /* No connection slots left. */
    slots = (connection*)realloc(slots, slotsalloc * 2 * sizeof(connection));
    memset(slots + slotsalloc, 0, slotsalloc * sizeof(connection));
    C = slots + slotsalloc;
    slotsalloc *= 2;
    return C;
}

connection *find_connection(const struct in_addr *src, const struct in_addr *dst, const short int sport, const short int dport) {
    connection *C;
    for (C = slots; C < slots + slotsalloc; ++C) {
        connection c = *C;
        if (c && c->sport == sport && c->dport == dport
            && memcmp(&(c->src), src, sizeof(struct in_addr)) == 0
            && memcmp(&(c->dst), dst, sizeof(struct in_addr)) == 0)
            return C;
    }
    return NULL;
}

#define TIMEOUT 5

void sweep_connections() {
    time_t now;
    connection *C;
    now = time(NULL);
    for (C = slots; C < slots + slotsalloc; ++C) {
        if (*C) {
            connection c = *C;
            if ((now - c->last) > TIMEOUT) {
                /* get any last images out of this one */
                connection_harvest_images(c);
                connection_delete(c);
                *C = NULL;
            }
        }
    }
}

void dump_data(FILE *fp, const unsigned char *data, const unsigned int len) {
    const unsigned char *p;
    for (p = data; p < data + len; ++p) {
        if (isprint((int)*p)) fputc(*p, fp);
        else fprintf(fp, "\\x%02x", (unsigned int)*p);
    }
}

/* get_link_level_hdr_length:
 * Find out how long the link-level header is, based on the datalink layer
 * type. This is based on init_linktype in the libpcap distribution; I
 * don't know why libpcap doesn't expose the information directly. The
 * constants here are taken from 0.6.2, but I've added #ifdefs in the hope
 * that it will still compile with earlier versions.
 */
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

        case DLT_ATM_RFC1483:
            return 8;

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

        default:;
    }
    fprintf(stderr, PROGNAME": unknown data link type %d", type);
    exit(1);
}


/* usage:
 * Print usage information. */
void usage(FILE *fp) {
    fprintf(fp,
"driftnet, version %s\n"
"Capture images from network traffic and display them in an X window.\n"
"\n"
"Synopsis: driftnet -h | [-i interface] [-p] [-v] [filter code]\n"
"\n"
"  -h               Display this help message.\n"
"  -i interface     Select the interface on which to listen (default: all\n"
"                   interfaces).\n"
"  -p               Do not put the listening interface into promiscuous mode.\n"
"  -v               Verbose operation.\n"
"  -x prefix        Prefix to use when saving images.\n"
"\n"
"Filter code can be specified after any options in the manner of tcpdump(8).\n"
"The filter code will be evaluated as `tcp and (user filter code)'\n"
"\n"
"driftnet, copyright (c) 2001-2 Chris Lightfoot <chris@ex-parrot.com>\n"
"home page: http://www.ex-parrot.com/~chris/driftnet/\n"
"\n"
"This program is free software; you can redistribute it and/or modify\n"
"it under the terms of the GNU General Public License as published by\n"
"the Free Software Foundation; either version 2 of the License, or\n"
"(at your option) any later version.\n"
"\n",
            DRIFTNET_VERSION);
}

/* terminate_on_signal:
 * Terminate on receipt of an appropriate signal. This is really ugly, because
 * the pcap_next call in the main loop may block, so it's best to just exit
 * here. */
void terminate_on_signal(int s) {
    if (dpychld == 0) {
        _exit(0);
    } else {
        close(dpychld_fd);
        pcap_close(pc);
        _exit(0);
    }
}

/* setup_signals:
 * Set up signal handlers. */
void setup_signals(void) {
    int *p;
    /* Signals to ignore. */
    int ignore_signals[] = {SIGPIPE, 0};
    /* Signals which mean we should quit, killing the display child if
     * applicable. */
    int terminate_signals[] = {SIGTERM, SIGINT, SIGSEGV, SIGBUS, SIGCHLD, 0};
    struct sigaction sa;

    sa.sa_flags = SA_RESTART;
    
    for (p = ignore_signals; *p; ++p) {
        memset(&sa, 0, sizeof(sa));
        sa.sa_handler = SIG_IGN;
        sigaction(*p, &sa, NULL);
    }

    for (p = terminate_signals; *p; ++p) {
        memset(&sa, 0, sizeof(sa));
        sa.sa_handler = terminate_on_signal;
        sigaction(*p, &sa, NULL);
    }
}

/* connection_string:
 * Return a string of the form w.x.y.z:foo -> a.b.c.d:bar for a pair of
 * addresses and ports. */
char *connection_string(const struct in_addr s, const unsigned short s_port, const struct in_addr d, const unsigned short d_port) {
    static char buf[50] = {0};
    sprintf(buf, "%s:%d -> ", inet_ntoa(s), (int)s_port);
    sprintf(buf + strlen(buf), "%s:%d", inet_ntoa(d), (int)d_port);
    return buf;
}

/* main:
 * Entry point. Process command line options, start up pcap and enter capture
 * loop. */
char optstring[] = "hi:pvx:";

int verbose;

int main(int argc, char *argv[]) {
    char *interface = NULL, *filterexpr;
    int promisc = 1;
    struct bpf_program filter;
    char ebuf[PCAP_ERRBUF_SIZE];
    struct pcap_pkthdr hdr;
    const unsigned char *pkt;
    int pfd[2];
    int pkt_offset;
    char c;
    struct stat st;
    extern char *savedimgpfx;    /* in display.c */

    /* Handle command-line options. */
    opterr = 0;
    while ((c = getopt(argc, argv, optstring)) != EOF) {
        switch(c) {
            case 'h':
                usage(stdout);
                return 0;

            case 'i':
                interface = optarg;
                break;

            case 'v':
                verbose = 1;
                break;

            case 'p':
                promisc = 0;
                break;

            case 'x':
                savedimgpfx = optarg;
                break;

            case '?':
            default:
                if (strchr(optstring, optopt))
                    fprintf(stderr, PROGNAME": option -%c requires an argument\n", optopt);
                else
                    fprintf(stderr, PROGNAME": unrecognised option -%c\n", optopt);
                usage(stderr);
                return 1;
        }
    }

    /* Since most users won't read the instructions, check for and create the
     * /tmp/imgdump directory. */
    if (stat("/tmp/imgdump", &st) == -1) {
        if (errno == ENOENT) {
            fprintf(stderr, PROGNAME": /tmp/imgdump does not exist; creating it\n");
            if (mkdir("/tmp/imgdump", 0700) == -1) {
                perror(PROGNAME": mkdir");
                return -1;
            }
        } else {
            perror(PROGNAME": stat(/tmp/imgdump)");
            return -1;
        }
    } else {
        if (!S_ISDIR(st.st_mode)) {
            fprintf(stderr, PROGNAME": /tmp/imgdump exists, but is not a directory. Quitting.\n");
            return -1;
        }
    }
    
    
    if (verbose)
        fprintf(stderr, PROGNAME": listening on %s%s\n", interface ? interface : "all interfaces", promisc ? " in promiscuous mode" : "");

    if (optind < argc) {
        char **a;
        int l;
        for (a = argv + optind, l = sizeof("tcp and ()"); *a; l += strlen(*a) + 1, ++a);
        filterexpr = (char*)calloc(l, 1);
        strcpy(filterexpr, "tcp and (");
        for (a = argv + optind; *a; ++a) {
            strcat(filterexpr, *a);
            if (*(a + 1)) strcat(filterexpr, " ");
        }
        strcat(filterexpr, ")");
    } else filterexpr = "tcp";

    if (verbose)
        fprintf(stderr, PROGNAME": using filter expression `%s'\n", filterexpr);
    
    setup_signals();

    if (verbose)
        fprintf(stderr, PROGNAME": using saved image prefix `%s'\n", savedimgpfx);
    
    /* fork to start the display child process */
    pipe(pfd);
    switch (dpychld = fork()) {
        case 0:
            /* we are the child */
            close(pfd[1]);
            dpychld_fd = pfd[0];
            dodisplay(argc, argv);
            return -1;

        case -1:
            perror(PROGNAME "fork");
            return -1;

        default:
            close(pfd[0]);
            dpychld_fd = pfd[1];
            if (verbose)
                fprintf(stderr, PROGNAME ": started display child, pid %d\n", (int)dpychld);
            break;
    }
    
    slotsused = 0;
    slotsalloc = 64;
    slots = (connection*)calloc(slotsalloc, sizeof(connection));

    /* Start up pcap. */
    pc = pcap_open_live(interface, 262144, promisc, 1, ebuf);
    if (!pc) {
        fprintf(stderr, PROGNAME": pcap_open_live: %s\n", ebuf);
        kill(dpychld, SIGTERM);
        return -1;
    }
    
    if (pcap_compile(pc, &filter, (char*)filterexpr, 1, 0) == -1) {
        fprintf(stderr, PROGNAME": pcap_compile: %s\n", pcap_geterr(pc));
        kill(dpychld, SIGTERM);
        return -1;
    }
    
    if (pcap_setfilter(pc, &filter) == -1) {
        fprintf(stderr, PROGNAME": pcap_setfilter: %s\n", pcap_geterr(pc));
        kill(dpychld, SIGTERM);
        return -1;
    }

    /* Figure out the offset from the start of a returned packet to the data in
     * it. */
    pkt_offset = get_link_level_hdr_length(pcap_datalink(pc));
    if (verbose)
        fprintf(stderr, PROGNAME": link-level header length is %d bytes\n", pkt_offset);

    while (1) {
        struct iphdr ip;
        struct tcphdr tcp;
        struct in_addr s, d;
        int off, len;
        connection *C, c;
        int delta;

        /* Capture of a packet may time out. If so, retry. */
        if (!(pkt = pcap_next(pc, &hdr)))
            continue;

        if (verbose)
            fprintf(stderr, ".");
/*
        fprintf(stderr, "packet len = %d captured = %d!\n", hdr.len, hdr.caplen);
*/
        memcpy(&ip, pkt + pkt_offset, sizeof(ip));
        memcpy(&s, &ip.saddr, sizeof(ip.saddr));
        memcpy(&d, &ip.daddr, sizeof(ip.daddr));

        memcpy(&tcp, pkt + pkt_offset + (ip.ihl << 2), sizeof(tcp));
        off = pkt_offset + (ip.ihl << 2) + (tcp.doff << 2);
        len = hdr.caplen - off;

        /*
        if (verbose)
            fprintf(stderr, PROGNAME": captured packet: %s:%d -> %s:%d\n", inet_ntoa(s), ntohs(tcp.source), inet_ntoa(d), ntohs(tcp.dest));
        */
        
        /* XXX fragmented packets and other nasties. */
        
        /* try to find the connection slot associated with this. */
        C = find_connection(&s, &d, ntohs(tcp.source), ntohs(tcp.dest));

        /* no connection at all, so we need to allocate one. */
        if (!C) {
            if (verbose)
                fprintf(stderr, PROGNAME": new connection: %s\n", connection_string(s, ntohs(tcp.source), d, ntohs(tcp.dest)));
            C = alloc_connection();
            *C = connection_new(&s, &d, ntohs(tcp.source), ntohs(tcp.dest));
            /* This might or might not be an entirely new connection (SYN flag
             * set). Either way we need a sequence number to start at. */
            (*C)->isn = ntohl(tcp.seq);
        }

        /* Now we need to process this segment. */
        c = *C;
        delta = 0;//tcp.syn ? 1 : 0;

        /* NB (STD0007):
         *    SEG.LEN = the number of octets occupied by the data in the
         *    segment (counting SYN and FIN) */
#if 0
        if (tcp.syn)
            /* getting a new isn. */
            c->isn = htonl(tcp.seq);
#endif

        if (tcp.rst) {
            /* Looks like this connection is bogus, and so might be a
             * connection going the other way. */
            if (verbose)
                fprintf(stderr, PROGNAME": connection reset: %s\n", connection_string(s, ntohs(tcp.source), d, ntohs(tcp.dest)));
            
            connection_delete(c);
            *C = NULL;

            if ((C = find_connection(&d, &s, ntohs(tcp.dest), ntohs(tcp.source)))) {
                connection_delete(*C);
                *C = NULL;
            }

            continue;
        }
        
        if (len > 0) {
            /* We have some data in the packet. If this data occurred after
             * the first data we collected for this connection, then save it
             * so that we can look for images. Otherwise, discard it. */
            unsigned int offset = ntohl(tcp.seq);

            /* Modulo 2**32 arithmetic; offset = seq - isn + delta. */
            if (offset < (c->isn + delta))
                offset = 0xffffffff - (c->isn + delta - offset);
            else
                offset -= c->isn + delta;
            
            if (offset > c->len + 262144) {
                /* Out-of-order packet. */
                if (verbose) 
                    fprintf(stderr, PROGNAME": out of order packet: %s\n", connection_string(s, ntohs(tcp.source), d, ntohs(tcp.dest)));
            } else {
/*                if (verbose)
                    fprintf(stderr, PROGNAME": captured %d bytes: %s:%d -> %s:%d\n", (int)len, inet_ntoa(s), ntohs(tcp.source), inet_ntoa(d), ntohs(tcp.dest));*/
                connection_push(c, pkt + off, offset, len);
                connection_harvest_images(c);
            }
        }

        if (tcp.fin) {
            /* Connection closing. */
            if (verbose)
                fprintf(stderr, PROGNAME": connection closing: %s, %d bytes transferred\n", connection_string(s, ntohs(tcp.source), d, ntohs(tcp.dest)), c->len);
            connection_harvest_images(c);
            connection_delete(c);
            *C = NULL;
        }

        /* sweep out old connections */
        sweep_connections();
    }

    pcap_close(pc);

    return 0;
}

/*
 * driftnet.c:
 * Pick out images from passing network traffic.
 *
 * Copyright (c) 2001 Chris Lightfoot. All rights reserved.
 * Email: chris@ex-parrot.com; WWW: http://www.ex-parrot.com/~chris/
 *
 */

static const char rcsid[] = "$Id: driftnet.c,v 1.7 2001/08/08 00:23:09 chris Exp $";

#undef NDEBUG

#include <assert.h>
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

#include "driftnet.h"

static const char filter_expr[] = "tcp";

#define PKT_OFFSET      14 /* offset between start of pcap returned packed and IP header */

connection *slots;
unsigned int slotsused, slotsalloc;

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

int main(int argc, char *argv[]) {
    pcap_t *pc;
    struct bpf_program filter;
    char ebuf[PCAP_ERRBUF_SIZE];
    struct pcap_pkthdr hdr;
    const unsigned char *pkt;
    int pfd[2];

    signal(SIGPIPE, SIG_IGN);

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
            fprintf(stderr, PROGNAME ": started display child, pid %d\n", (int)dpychld);
            break;
    }
    
    slotsused = 0;
    slotsalloc = 64;
    slots = (connection*)calloc(slotsalloc, sizeof(connection));

    /* Start up pcap. */
    pc = pcap_open_live("eth0", 262144, 0 /*1*/, 0, ebuf);
    if (!pc) {
        fprintf(stderr, PROGNAME "pcap_open_live: %s\n", ebuf);
        return -1;
    }
    
    if (pcap_compile(pc, &filter, (char*)filter_expr, 1, 0) == -1) {
        fprintf(stderr, PROGNAME "pcap_compile: %s\n", pcap_geterr(pc));
        return -1;
    }
    
    if (pcap_setfilter(pc, &filter) == -1) {
        fprintf(stderr, PROGNAME "pcap_setfilter: %s\n", pcap_geterr(pc));
        return -1;
    }

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
/*
        fprintf(stderr, "packet len = %d captured = %d!\n", hdr.len, hdr.caplen);
*/
        memcpy(&ip, pkt + PKT_OFFSET, sizeof(ip));
        memcpy(&s, &ip.saddr, sizeof(ip.saddr));
        memcpy(&d, &ip.daddr, sizeof(ip.daddr));
/*
        fprintf(stderr, "   ip src = %s dst = %s hdr len = %d\n",
                inet_ntoa(s), inet_ntoa(d), ip.ihl << 2);
*/
        memcpy(&tcp, pkt + PKT_OFFSET + (ip.ihl << 2), sizeof(tcp));
        off = PKT_OFFSET + (ip.ihl << 2) + (tcp.doff << 2);
        len = hdr.caplen - off;
        
        /* XXX fragmented packets and other nasties. */
        
        /* try to find the connection slot associated with this. */
        C = find_connection(&s, &d, htons(tcp.source), htons(tcp.dest));

        /* no connection at all, so we need to allocate one. */
        if (!C) {
            C = alloc_connection();
            *C = connection_new(&s, &d, htons(tcp.source), htons(tcp.dest));
            /* This might or might not be an entirely new connection (SYN flag
             * set). Either way we need a sequence number to start at. */
            (*C)->isn = htonl(tcp.seq);
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
            connection_delete(c);
            *C = NULL;

            if ((C = find_connection(&d, &s, htons(tcp.dest), htons(tcp.source)))) {
                connection_delete(*C);
                *C = NULL;
            }

            continue;
        }
        
        if (len > 0) {
            unsigned int offset = htonl(tcp.seq);

            /* Modulo 2**32 arithmetic; offset = seq - isn + delta. */
            if (offset < (c->isn + delta)) {
                printf("isn = %u, seq = %u, delta = %u; offset = ", c->isn, offset, delta);
                offset = 0xffffffff - (c->isn + delta - offset);
                printf("%u\n", offset);
            } else
                offset -= c->isn + delta;
            
            connection_push(c, pkt + off, offset, len);
            connection_harvest_images(c);
        }

        if (tcp.fin) {
            /* Connection closing. */
            connection_harvest_images(c);
            connection_delete(c);
            *C = NULL;
        }

        /* sweep out old connections */
        sweep_connections();
#if 0
        /* dump out all the connections we have */
        printf("\033c\n");
        for (C = slots; C < slots + slotsalloc; ++C) {
            if (*C) {
                connection c = *C;
                printf("%s:%d (%c) %u --> %s:%d (%c) %u\n",
                        inet_ntoa(c->src), c->sport, c->sdfin ? '*' : ' ', c->sdisn,
                        inet_ntoa(c->dst), c->dport, c->dsfin ? '*' : ' ', c->dsisn);
                
                printf("--> (%u) ", c->sdlen);
                dump_data(stdout, c->sddata, c->sdlen);
                printf("\n<-- (%u) ", c->dslen);
                dump_data(stdout, c->dsdata, c->dslen);
                printf("\n");
            }
        }
#endif
    }

    fprintf(stderr, PROGNAME": pcap_next: %s\n", pcap_geterr(pc));
        
    return 0;
}

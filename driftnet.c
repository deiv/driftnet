/*
 * driftnet.c:
 * Pick out images from passing network traffic.
 *
 * Copyright (c) 2001 Chris Lightfoot. All rights reserved.
 * Email: chris@ex-parrot.com; WWW: http://www.ex-parrot.com/~chris/
 *
 */

static const char rcsid[] = "$Id: driftnet.c,v 1.2 2001/07/16 00:09:33 chris Exp $";

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
    c->sdalloc = 16384;
    c->sddata = c->sdgif = c->sdjpeg = (unsigned char*)malloc(c->sdalloc);
    c->dsalloc = 16384;
    c->dsdata = c->dsgif = c->dsjpeg = (unsigned char*)malloc(c->dsalloc);
    c->last = time(NULL);
    return c;
}

void connection_delete(connection c) {
    free(c->sddata);
    free(c->dsdata);
    free(c);
}

void connection_push_sd(connection c, const unsigned char *data, unsigned int off, unsigned int len) {
    size_t goff = c->sdgif - c->sddata, joff = c->sdjpeg - c->sddata;
    assert(c->sdalloc > 0);
    while (off + len > c->sdalloc) {
        c->sdalloc *= 2;
        c->sddata = (unsigned char*)realloc(c->sddata, c->sdalloc);
    }
    c->sdgif = c->sddata + goff;
    c->sdjpeg = c->sddata + joff;
    memcpy(c->sddata + off, data, len);
/*    printf("push_sd: %u %u\n", off + len, c->sdlen);*/
    if (off + len > c->sdlen) c->sdlen = off + len;
    c->last = time(NULL);
}

void connection_push_ds(connection c, const unsigned char *data, unsigned int off, unsigned int len) {
    size_t goff = c->dsgif - c->dsdata, joff = c->dsjpeg - c->dsdata;
    assert(c->dsalloc > 0);
    while (off + len > c->dsalloc) {
        c->dsalloc *= 2;
        c->dsdata = (unsigned char*)realloc(c->dsdata, c->dsalloc);
    }
    c->dsgif = c->dsdata + goff;
    c->dsjpeg = c->dsdata + joff;
    memcpy(c->dsdata + off, data, len);
/*    printf("push_ds: %u %u\n", off + len, c->dslen);*/
    if (off + len > c->dslen) c->dslen = off + len;
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

void connection_harvest_images(connection c, int backwards) {
    unsigned char *ptr, *oldptr, *img;
    size_t ilen;

    /* look for GIF files */
    if (backwards)
        ptr = c->dsgif;
    else
        ptr = c->sdgif;
    oldptr = NULL;

    while (ptr != oldptr) {
        oldptr = ptr;
        if (backwards)
            ptr = find_gif_image(ptr, c->dslen - (ptr - c->dsdata), &img, &ilen);
        else
            ptr = find_gif_image(ptr, c->sdlen - (ptr - c->sddata), &img, &ilen);

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

    if (backwards)
        c->dsgif = ptr;
    else
        c->sdgif = ptr;

    /* look for JPEG files */
    if (backwards)
        ptr = c->dsjpeg;
    else
        ptr = c->sdjpeg;
    oldptr = NULL;
    
    while (ptr != oldptr) {
        oldptr = ptr;
        if (backwards)
            ptr = find_jpeg_image(ptr, c->dslen - (ptr - c->dsdata), &img, &ilen);
        else
            ptr = find_jpeg_image(ptr, c->sdlen - (ptr - c->sddata), &img, &ilen);

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

    if (backwards)
        c->dsjpeg = ptr;
    else
        c->sdjpeg = ptr;
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

#define TIMEOUT 10

void sweep_connections() {
    time_t now;
    connection *C;
    now = time(NULL);
    for (C = slots; C < slots + slotsalloc; ++C) {
        if (*C) {
            connection c = *C;
            if ((now - c->last) > TIMEOUT) {
                /* get any last images out of this one */
                connection_harvest_images(c, 0);
                connection_harvest_images(c, 1);
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

    /* fork to start the dispaly child process */
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
    pc = pcap_open_live("eth0", 262144, 1, 0, ebuf);
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

    while ((pkt = pcap_next(pc, &hdr))) {
        struct iphdr ip;
        struct tcphdr tcp;
        struct in_addr s, d;
        int off, len;
        int backwards = 0;
        connection *C;

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
        if (!C) {
            backwards = 1;
            C = find_connection(&d, &s, htons(tcp.dest), htons(tcp.source));
        }

        /* found a connection object */
        if (C) {
            connection c = *C;
            int delta = tcp.syn ? 1 : 0;
            if (tcp.syn) {
                /* may be getting a new isn. */
                if (backwards) c->dsisn = htonl(tcp.seq);
                else c->sdisn = htonl(tcp.seq);
            }

            if (tcp.rst) {
                /* looks like this connection is bogus. */
                connection_delete(c);
                *C = NULL;
                continue;
            }
            
            if (len > 0) {
                if (backwards) connection_push_ds(c, pkt + off, htonl(tcp.seq) - c->dsisn - delta, len);
                else connection_push_sd(c, pkt + off, htonl(tcp.seq) - c->sdisn - delta, len);

                connection_harvest_images(c, backwards);
            }

            if (tcp.fin) {
                if (backwards) c->dsfin = 1;
                else c->sdfin = 1;
                if (c->sdfin && c->dsfin) {
                    /* connection closed; look for image data in this connection. */
                    connection_harvest_images(c, 0);
                    connection_harvest_images(c, 1);
                    connection_delete(c);
                    *C = NULL;
                }
            }

        } else {
            if (tcp.syn) {
                /* new connection being opened, we track it. */
                connection *C = alloc_connection();
                *C = connection_new(&s, &d, htons(tcp.source), htons(tcp.dest));
                (*C)->sdisn = htonl(tcp.seq);

                if (len > 0) {
                    connection_push_sd(*C, pkt + off, htonl(tcp.seq) - (*C)->sdisn - 1, len);
                    connection_harvest_images(*C, 0);
                }

/*                fprintf(stderr, "allocating new connection\n");*/
            }
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

/*
 * driftnet.c:
 * Pick out images from passing network traffic.
 *
 * Copyright (c) 2001 Chris Lightfoot. All rights reserved.
 * Email: chris@ex-parrot.com; WWW: http://www.ex-parrot.com/~chris/
 *
 */

static const char rcsid[] = "$Id: driftnet.c,v 1.25 2002/07/08 20:57:17 chris Exp $";

#undef NDEBUG

#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <pcap.h>

#include <netinet/in.h> /* needs to be before <arpa/inet.h> on OpenBSD */
#include <arpa/inet.h>
#include <limits.h>

#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>

#include <ctype.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include "driftnet.h"

#define SNAPLEN 262144      /* largest chunk of data we accept from pcap */
#define WRAPLEN 262144      /* out-of-order packet margin */

/* slots for storing information about connections */
connection *slots;
unsigned int slotsused, slotsalloc;

/* flags: verbose, adjunct mode, temporary directory to use, media types to extract. */
int extract_images = 1;
int verbose, adjunct;
int tmpdir_specified;
char *tmpdir;
int max_tmpfiles;

enum mediatype extract_type = m_image;

/* ugh. */
pcap_t *pc;

#ifndef NO_DISPLAY_WINDOW
/* PID of display child and file descriptor on pipe to same. */
pid_t dpychld;
int dpychld_fd;

/* display.c */
int dodisplay(int argc, char *argv[]);
#endif /* !NO_DISPLAY_WINDOW */

/* playaudio.c */
void do_mpeg_player(void);

/* clean_temporary_directory:
 * Ensure that our temporary directory is clear of any files. */
void clean_temporary_directory(void) {
    DIR *d;
    
    /* If tmpdir_specified is true, the user specified a particular temporary
     * directory. We presume that the user doesn't want the directory removed
     * and that we shouldn't nuke any files in that directory which don't look
     * like ours. */

    d = opendir(tmpdir);
    if (d) {
        struct dirent *de;
        char *buf;
        size_t buflen;

        buf = malloc(buflen = strlen(tmpdir) + 64);

        while ((de = readdir(d))) {
            char *p;
            p = strrchr(de->d_name, '.');
            if (!tmpdir_specified || (p && strncmp(de->d_name, "driftnet-", 9) == 0 && (strcmp(p, ".jpeg") == 0 || strcmp(p, ".gif") == 0 || strcmp(p, ".mp3") == 0))) {
                if (buflen < strlen(tmpdir) + strlen(de->d_name) + 1)
                    buf = realloc(buf, buflen = strlen(tmpdir) + strlen(de->d_name) + 64);
                
                sprintf(buf, "%s/%s", tmpdir, de->d_name);
                unlink(buf);
            }
        }
        closedir(d);

        free(buf);
    }


    if (!tmpdir_specified && rmdir(tmpdir) == -1 && errno != ENOENT) /* lame attempt to avoid race */
        fprintf(stderr, PROGNAME": rmdir(%s): %s\n", tmpdir, strerror(errno));
}

/* alloc_connection:
 * Find a free slot in which to allocate a connection object. */
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

/* find_connection:
 * Find a connection running between the two named addresses. */
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


/* sweep_connections:
 * Free finished connection slots. */
#define TIMEOUT 5

void sweep_connections(void) {
    time_t now;
    connection *C;
    now = time(NULL);
    for (C = slots; C < slots + slotsalloc; ++C) {
        if (*C) {
            connection c = *C;
            /* We discard connections which have seen no activity for TIMEOUT
             * or for which a FIN has been seen and for which there are no
             * gaps in the stream. */
            if ((now - c->last) > TIMEOUT || (c->fin && (!c->blocks || !c->blocks->next))) {
                connection_extract_media(c, extract_type);
                connection_delete(c);
                *C = NULL;
            }
        }
    }
}

/* dump_data:
 * Print some binary data on a file descriptor. */
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
#ifdef NO_DISPLAY_WINDOW
"\n"
"Actually, this version of driftnet was compiled with the NO_DISPLAY_WINDOW\n"
"option, so that it can only be used in adjunct mode. See below.\n"
#endif /* NO_DISPLAY_WINDOW */
"\n"
"Synopsis: driftnet [options] [filter code]\n"
"\n"
"Options:\n"
"\n"
"  -h               Display this help message.\n"
"  -v               Verbose operation.\n"
"  -i interface     Select the interface on which to listen (default: all\n"
"                   interfaces).\n"
"  -p               Do not put the listening interface into promiscuous mode.\n""  -a               Adjunct mode: do not display images on screen, but save\n"
"                   them to a temporary directory and announce their names on\n"
"                   standard output.\n"
"  -m number        Maximum number of images to keep in temporary directory\n"
"                   in adjunct mode.\n"
"  -d directory     Use the named temporary directory.\n"
"  -x prefix        Prefix to use when saving images.\n"
"  -s               Attempt to extract streamed audio data from the network,\n"
"                   in addition to images. At present this supports MPEG data\n"
"                   only.\n"
"  -S               Extract streamed audio but not images.\n"
"  -M command       Use the given command to play MPEG audio data extracted\n"
"                   with the -s option; this should process MPEG frames\n"
"                   supplied on standard input. Default: `mpg123 -'.\n"
"\n"
"Filter code can be specified after any options in the manner of tcpdump(8).\n"
"The filter code will be evaluated as `tcp and (user filter code)'\n"
"\n"
"You can save images to the current directory by clicking on them.\n"
"\n"
"Adjunct mode is designed to be used by other programs which want to use\n"
"driftnet to gather images from the network. With the -m option, driftnet will\n"
"silently drop images if more than the specified number of images are saved\n"
"in its temporary directory. It is assumed that some other process is\n"
"collecting and deleting the image files.\n"
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
sig_atomic_t foad;

void terminate_on_signal(int s) {
    extern pid_t mpeg_mgr_pid; /* in playaudio.c */
    /* Pass on the signal to the MPEG player manager so that it can abort,
     * since it won't die when the pipe into it dies. */
    if (mpeg_mgr_pid)
        kill(mpeg_mgr_pid, s);
    foad = s;
}

/* setup_signals:
 * Set up signal handlers. */
void setup_signals(void) {
    int *p;
    /* Signals to ignore. */
    int ignore_signals[] = {SIGPIPE, 0};
    /* Signals which mean we should quit, killing the display child if
     * applicable. */
    int terminate_signals[] = {SIGTERM, SIGINT, /*SIGSEGV,*/ SIGBUS, SIGCHLD, 0};
    struct sigaction sa;

    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    
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

/* process_packet:
 * Callback which processes a packet captured by libpcap. */
int pkt_offset; /* offset of IP packet within wire packet */

void process_packet(u_char *user, const struct pcap_pkthdr *hdr, const u_char *pkt) {
    struct ip ip;
    struct tcphdr tcp;
    struct in_addr s, d;
    int off, len, delta;
    connection *C, c;

    if (verbose)
        fprintf(stderr, ".");

    memcpy(&ip, pkt + pkt_offset, sizeof(ip));
    memcpy(&s, &ip.ip_src, sizeof(ip.ip_src));
    memcpy(&d, &ip.ip_dst, sizeof(ip.ip_dst));

    memcpy(&tcp, pkt + pkt_offset + (ip.ip_hl << 2), sizeof(tcp));
    off = pkt_offset + (ip.ip_hl << 2) + (tcp.th_off << 2);
    len = hdr->caplen - off;

    /* XXX fragmented packets and other nasties. */
    
    /* try to find the connection slot associated with this. */
    C = find_connection(&s, &d, ntohs(tcp.th_sport), ntohs(tcp.th_dport));

    /* no connection at all, so we need to allocate one. */
    if (!C) {
        if (verbose)
            fprintf(stderr, PROGNAME": new connection: %s\n", connection_string(s, ntohs(tcp.th_sport), d, ntohs(tcp.th_dport)));
        C = alloc_connection();
        *C = connection_new(&s, &d, ntohs(tcp.th_sport), ntohs(tcp.th_dport));
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
        if (verbose)
            fprintf(stderr, PROGNAME": connection reset: %s\n", connection_string(s, ntohs(tcp.th_sport), d, ntohs(tcp.th_dport)));
        
        connection_delete(c);
        *C = NULL;

        if ((C = find_connection(&d, &s, ntohs(tcp.th_dport), ntohs(tcp.th_sport)))) {
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
            if (verbose) 
                fprintf(stderr, PROGNAME": out of order packet: %s\n", connection_string(s, ntohs(tcp.th_sport), d, ntohs(tcp.th_dport)));
        } else {
            connection_push(c, pkt + off, offset, len);
            connection_extract_media(c, extract_type);
        }
    }
    if (tcp.th_flags & TH_FIN) {
        /* Connection closing; mark it as closed, but let sweep_connections
         * free it if appropriate. */
        if (verbose)
            fprintf(stderr, PROGNAME": connection closing: %s, %d bytes transferred\n", connection_string(s, ntohs(tcp.th_sport), d, ntohs(tcp.th_dport)), c->len);
        c->fin = 1;
    }

    /* sweep out old connections */
    sweep_connections();
}

/* packet_capture_thread:
 * Thread in which packet capture runs. */
void *packet_capture_thread(void *v) {
    while (!foad)
        pcap_dispatch(pc, -1, process_packet, NULL);
    return NULL;
}

/* main:
 * Entry point. Process command line options, start up pcap and enter capture
 * loop. */
char optstring[] = "hi:psSMvam:d:x:";

int main(int argc, char *argv[]) {
    char *interface = NULL, *filterexpr;
    int promisc = 1;
    struct bpf_program filter;
    char ebuf[PCAP_ERRBUF_SIZE];
    int c;
#ifndef NO_DISPLAY_WINDOW
    extern char *savedimgpfx;       /* in display.c */
#endif
    extern char *audio_mpeg_player; /* in playaudio.c */
    int newpfx = 0;
    int mpeg_player_specified = 0;
    pthread_t packetth;

    /* Handle command-line options. */
    opterr = 0;
    while ((c = getopt(argc, argv, optstring)) != -1) {
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

            case 's':
                extract_type |= m_audio;
                break;

            case 'S':
                extract_type = m_audio;
                break;

            case 'M':
                audio_mpeg_player = optarg;
                mpeg_player_specified = 1;
                break;

            case 'a':
                adjunct = 1;
                break;

            case 'm':
                max_tmpfiles = atoi(optarg);
                if (max_tmpfiles <= 0) {
                    fprintf(stderr, PROGNAME": `%s' does not make sense for -m\n", optarg);
                    return -1;
                }
                break;

            case 'd':
                tmpdir = optarg;
                tmpdir_specified = 1; /* so we don't delete it. */
                break;

#ifndef NO_DISPLAY_WINDOW
            case 'x':
                savedimgpfx = optarg;
                newpfx = 1;
                break;
#endif

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
    
#ifdef NO_DISPLAY_WINDOW
    if (!adjunct) {
        fprintf(stderr, PROGNAME": this version of driftnet was compiled without display support\n");
        fprintf(stderr, PROGNAME": use the -a option to run it in adjunct mode\n");
        return -1;
    }
#endif /* !NO_DISPLAY_WINDOW */
    
    /* Let's not be too fascist about option checking.... */
    if (max_tmpfiles && !adjunct) {
        fprintf(stderr, PROGNAME": warning: -m only makes sense with -a\n");
        max_tmpfiles = 0;
    }

    if (adjunct && newpfx)
        fprintf(stderr, PROGNAME": warning: -x ignored -a\n");

    if (mpeg_player_specified && !(extract_type & m_audio))
        fprintf(stderr, PROGNAME": warning: -M only makes sense with -s\n");

    if (mpeg_player_specified && adjunct)
        fprintf(stderr, PROGNAME": warning: -M ignored with -a\n");

    if (max_tmpfiles && adjunct && verbose)
        fprintf(stderr, PROGNAME": a maximum of %d images will be buffered\n", max_tmpfiles);

    /* If a directory name has not been specified, then we need to create one.
     * Otherwise, check that it's a directory into which we may write files. */
    if (tmpdir) {
        struct stat st;
        if (stat(tmpdir, &st) == -1) {
            fprintf(stderr, PROGNAME": stat(%s): %s\n", tmpdir, strerror(errno));
            return -1;
        } else if (!S_ISDIR(st.st_mode)) {
            fprintf(stderr, PROGNAME": %s: not a directory\n", tmpdir);
            return -1;
        } else if (access(tmpdir, R_OK | W_OK) != 0) { /* access is unsafe but we don't really care. */
            fprintf(stderr, PROGNAME": %s: %s\n", tmpdir, strerror(errno));
            return -1;
        }
    } else {
        /* need to make a temporary directory. */
        for (;;) {
            tmpdir = strdup(tmpnam(NULL));
            if (mkdir(tmpdir, 0700) == 0)
                break;
            free(tmpdir);
        }
    }

    if (verbose) 
        fprintf(stderr, PROGNAME": using temporary file directory %s\n", tmpdir);

    if (!interface && !(interface = pcap_lookupdev(ebuf))) {
        fprintf(stderr, PROGNAME": pcap_lookupdev: %s\n", ebuf);
        fprintf(stderr, PROGNAME": try specifying an interface with -i\n");
        return -1;
    }

    if (verbose)
        fprintf(stderr, PROGNAME": listening on %s%s\n", interface ? interface : "all interfaces", promisc ? " in promiscuous mode" : "");

    /* Build up filter. */
    if (optind < argc) {
        char **a;
        int l;
        for (a = argv + optind, l = sizeof("tcp and ()"); *a; l += strlen(*a) + 1, ++a);
        filterexpr = calloc(l, 1);
        strcpy(filterexpr, "tcp and (");
        for (a = argv + optind; *a; ++a) {
            strcat(filterexpr, *a);
            if (*(a + 1)) strcat(filterexpr, " ");
        }
        strcat(filterexpr, ")");
    } else filterexpr = "tcp";

    if (verbose)
        fprintf(stderr, PROGNAME": using filter expression `%s'\n", filterexpr);
    

#ifndef NO_DISPLAY_WINDOW
    if (verbose && newpfx && !adjunct)
        fprintf(stderr, PROGNAME": using saved image prefix `%s'\n", savedimgpfx);
#endif

    setup_signals();
    
    /* Start up the audio player, if required. */
    if (!adjunct && (extract_type & m_audio))
        do_mpeg_player();
    
#ifndef NO_DISPLAY_WINDOW
    /* Possibly fork to start the display child process */
    if (!adjunct && (extract_type & m_image)) {
        int pfd[2];
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
                    fprintf(stderr, PROGNAME": started display child, pid %d\n", (int)dpychld);
                break;
        }
    } else if (verbose)
        fprintf(stderr, PROGNAME": operating in adjunct mode\n");
#endif /* !NO_DISPLAY_WINDOW */
 
    /* Start up pcap. */

    pc = pcap_open_live(interface, SNAPLEN, promisc, 1000, ebuf);
    if (!pc) {
        fprintf(stderr, PROGNAME": pcap_open_live: %s\n", ebuf);

        if (getuid() != 0)
            fprintf(stderr, PROGNAME": perhaps you need to be root?\n");
        else if (!interface)
            fprintf(stderr, PROGNAME": perhaps try selecting an interface with the -i option?\n");
            
        return -1;
    }
    
    if (pcap_compile(pc, &filter, (char*)filterexpr, 1, 0) == -1) {
        fprintf(stderr, PROGNAME": pcap_compile: %s\n", pcap_geterr(pc));
        return -1;
    }
    
    if (pcap_setfilter(pc, &filter) == -1) {
        fprintf(stderr, PROGNAME": pcap_setfilter: %s\n", pcap_geterr(pc));
        return -1;
    }

    /* Figure out the offset from the start of a returned packet to the data in
     * it. */
    pkt_offset = get_link_level_hdr_length(pcap_datalink(pc));
    if (verbose)
        fprintf(stderr, PROGNAME": link-level header length is %d bytes\n", pkt_offset);

    slotsused = 0;
    slotsalloc = 64;
    slots = (connection*)calloc(slotsalloc, sizeof(connection));

    /* Actually start the capture stuff up. Unfortunately, on many platforms,
     * libpcap doesn't have read timeouts, so we start the thing up in a
     * separate thread. Yay! */
    pthread_create(&packetth, NULL, packet_capture_thread, NULL);

    while (!foad)
        sleep(1);

    if (verbose) {
        if (foad == SIGCHLD) {
            pid_t pp;
            int st;
            while ((pp = waitpid(-1, &st, WNOHANG)) > 0) {
                if (WIFEXITED(st))
                    fprintf(stderr, PROGNAME": child process %d exited with status %d\n", (int)pp, WEXITSTATUS(st));
                else if (WIFSIGNALED(st))
                    fprintf(stderr, PROGNAME": child process %d killed by signal %d\n", (int)pp, WTERMSIG(st));
                else
                    fprintf(stderr, PROGNAME": child process %d died, not sure why\n", (int)pp);
            }
            
        } else
            fprintf(stderr, PROGNAME": caught signal %d\n", foad);
    }
    
    pthread_cancel(packetth); /* make sure thread quits even if it's stuck in pcap_dispatch */
    pthread_join(packetth, NULL);
    
    /* Clean up. */
    pcap_close(pc);
    clean_temporary_directory();

    return 0;
}

/*
 * driftnet.h:
 * Pick out images from passing network traffic.
 *
 * Copyright (c) 2001 Chris Lightfoot. All rights reserved.
 * Email: chris@ex-parrot.com; WWW: http://www.ex-parrot.com/~chris/
 *
 * $Id: driftnet.h,v 1.13 2004/04/08 23:06:29 chris Exp $
 *
 */

#ifndef __DRIFTNET_H_ /* include guard */
#define __DRIFTNET_H_

#define QUOTE(str) #str
#define PROGNAME QUOTE(DRIFTNET_PROGNAME)

#ifdef HAVE_CONFIG_H
    #include <config.h>
#endif
#include "compat.h"

#include <sys/types.h>

#include <stdio.h> 
#include <stdlib.h> /* On many systems (Darwin...), stdio.h is a prerequisite. */
#include <sys/socket.h> /* On Darwin, stdlib.h is a prerequisite.  */
#include <netinet/in.h>
#include <arpa/inet.h>


/* alloc_struct S P
 * Make P point to a new struct S, initialised as if in static storage (like
 * = {0}). */
#define alloc_struct(S, p)  do { struct S as__z = {0}; p = xmalloc(sizeof *p); *p = as__z; } while (0)

/* enum mediatype:
 * Bit field to characterise types of media which we can extract. */
enum mediatype { m_image = 1, m_audio = 2, m_text = 4 };

#define NMEDIATYPES     5       /* keep up to date with media.c */

/* struct datablock:
 * Represents an extent in a captured stream. */
struct datablock {
    int off, len, moff[NMEDIATYPES], dirty;
    struct datablock *next;
};

/* connection:
 * Object representing one half of a TCP stream connection. Each connection
 * maintains a record of the data which has been recovered from the network
 * and a list of blocks of data which represent valid data in the buffer, so
 * that if there is a gap in the received data, we don't search it for
 * data. */
typedef struct _connection {
    /* Source/destination address/port of this half-duplex connection. */
    struct in_addr src, dst;
    short int sport, dport;
    /* The TCP initial-sequence-number of the connection. */
    uint32_t isn;
    /* The highest offset and the buffer size allocated, and the buffer
     * itself. */
    unsigned int len, alloc;
    unsigned char *data;
    /* Flag indicating that we've seen a FIN-flagged segment for this stream,
     * so that it is undergoing a shutdown. */
    int fin;
    /* The time at which we last received any data on this stream. */
    time_t last;
    /* A list of the extents in the buffer which contain valid data. */
    struct datablock *blocks;
} *connection;

/* driftnet.c */
char *connection_string(const struct in_addr s, const unsigned short s_port, const struct in_addr d, const unsigned short d_port);
void dump_data(FILE *fp, const unsigned char *data, const unsigned int len);

/* connection.c */
connection connection_new(const struct in_addr *src, const struct in_addr *dst, const short int sport, const short int dport);
void connection_delete(connection c);
void connection_push(connection c, const unsigned char *data, unsigned int off, unsigned int len);
connection *alloc_connection(void);
connection *find_connection(const struct in_addr *src, const struct in_addr *dst, const short int sport, const short int dport);

/* media.c */
void connection_extract_media(connection c, const enum mediatype T);

/* util.c */
void *xmalloc(size_t n);
void *xcalloc(size_t n, size_t m);
void *xrealloc(void *w, size_t n);
void xfree(void *v);
char *xstrdup(const char *s);
unsigned char *memstr(const unsigned char *haystack, const size_t hlen, const unsigned char *needle, const size_t nlen);
void xnanosleep(long nanosecs);

#define TMPNAMELEN      64

#endif /* __DRIFTNET_H_ */

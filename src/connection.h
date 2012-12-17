/*
 * Connection.h:
 * Connection handling.
 *
 * Copyright (c) 2012 David Suárez. All rights reserved.
 * Email: david.sephirot@gmail.com
 *
 * Copyright (c) 2002 Chris Lightfoot. All rights reserved.
 * Email: chris@ex-parrot.com; WWW: http://www.ex-parrot.com/~chris/
 *
 */

#ifndef __CONNECTION_H__
#define __CONNECTION_H__

#ifdef HAVE_CONFIG_H
    #include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h> /* On many systems (Darwin...), stdio.h is a prerequisite. */

#include <sys/socket.h> /* On Darwin, stdlib.h is a prerequisite.  */
#include <netinet/in.h> /* needs to be before <arpa/inet.h> on OpenBSD */
#include <arpa/inet.h>
#ifdef HAVE_LIMITS_H
    #include <limits.h>
#endif
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>

struct datablock;

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

void connection_alloc_slots(void);
void connection_free_slots(void);

connection connection_new(const struct in_addr *src, const struct in_addr *dst, const short int sport, const short int dport);
void connection_delete(connection c);
void connection_push(connection c, const unsigned char *data, unsigned int off, unsigned int len);
connection *alloc_connection(void);
connection *find_connection(const struct in_addr *src, const struct in_addr *dst, const short int sport, const short int dport);

char *connection_string(const struct in_addr s, const unsigned short s_port, const struct in_addr d, const unsigned short d_port);
void sweep_connections(void);


#include "media.h" /* NMEDIATYPES */

/* struct datablock:
 * Represents an extent in a captured stream. */
struct datablock {
    int off, len, moff[NMEDIATYPES], dirty;
    struct datablock *next;
};

#endif /* __CONNECTION_H__ */

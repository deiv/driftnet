/*
 * driftnet.h:
 * Pick out images from passing network traffic.
 *
 * Copyright (c) 2001 Chris Lightfoot. All rights reserved.
 * Email: chris@ex-parrot.com; WWW: http://www.ex-parrot.com/~chris/
 *
 * $Id: driftnet.h,v 1.9 2002/06/13 20:06:42 chris Exp $
 *
 */

#ifndef __DRIFTNET_H_ /* include guard */
#define __DRIFTNET_H_

#define PROGNAME    "driftnet"

#include <sys/types.h> /* added 20020604 edobbs for OpenBSD */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <stdio.h>

/* enum mediatype:
 * Characterise types of media which we can extract. */
enum mediatype { m_image = 1, m_audio = 2 };

#define NMEDIATYPES     3       /* keep up to date with media.c */

/* struct datablock:
 * Represents an extent in a captured stream. */
struct datablock {
    int off, len, moff[NMEDIATYPES], dirty;
    struct datablock *next;
};

/* connection:
 * Object representing one half of a TCP stream connection. */
typedef struct _connection {
    struct in_addr src, dst;
    short int sport, dport;
    uint32_t isn;
    unsigned int len, off, alloc;
    unsigned char *data, *gif, *jpeg, *mpeg;
    int fin;
    time_t last;
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

#define TMPNAMELEN      64

#endif /* __DRIFTNET_H_ */

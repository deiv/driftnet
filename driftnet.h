/*
 * driftnet.h:
 * Pick out images from passing network traffic.
 *
 * Copyright (c) 2001 Chris Lightfoot. All rights reserved.
 * Email: chris@ex-parrot.com; WWW: http://www.ex-parrot.com/~chris/
 *
 * $Id: driftnet.h,v 1.5 2002/06/01 17:39:11 chris Exp $
 *
 */

#ifndef __DRIFTNET_H_ /* include guard */
#define __DRIFTNET_H_

#define PROGNAME    "driftnet"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>

struct datablock {
    int off, len, done;
    struct datablock *next;
};

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
connection connection_new(const struct in_addr *src, const struct in_addr *dst, const short int sport, const short int dport);
void connection_delete(connection c);
void connection_push(connection c, const unsigned char *data, unsigned int off, unsigned int len);
connection *alloc_connection(void);
connection *find_connection(const struct in_addr *src, const struct in_addr *dst, const short int sport, const short int dport);

struct pipemsg {
    int len;
    char filename[256]; /* ugh. */
};

#endif /* __DRIFTNET_H_ */

/*
 * driftnet.h:
 * Pick out images from passing network traffic.
 *
 * Copyright (c) 2001 Chris Lightfoot. All rights reserved.
 * Email: chris@ex-parrot.com; WWW: http://www.ex-parrot.com/~chris/
 *
 * $Id: driftnet.h,v 1.1 2001/07/15 11:07:33 chris Exp $
 *
 */

#ifndef __DRIFTNET_H_ /* include guard */
#define __DRIFTNET_H_

#define PROGNAME    "driftnet"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>

typedef struct _connection {
    struct in_addr src, dst;
    short int sport, dport;
    uint32_t sdisn;
    unsigned int sdlen, sdoff, sdalloc;
    unsigned char *sddata, *sdgif, *sdjpeg;
    uint32_t dsisn;
    unsigned int dslen, dsoff, dsalloc;
    unsigned char *dsdata, *dsgif, *dsjpeg;
    int sdfin, dsfin;
    time_t last;
} *connection;

/* driftnet.c */
connection connection_new(const struct in_addr *src, const struct in_addr *dst, const short int sport, const short int dport);
void connection_delete(connection c);
void connection_push_sd(connection c, const unsigned char *data, unsigned int off, unsigned int len);
void connection_push_ds(connection c, const unsigned char *data, unsigned int off, unsigned int len);
connection *alloc_connection(void);
connection *find_connection(const struct in_addr *src, const struct in_addr *dst, const short int sport, const short int dport);

struct pipemsg {
    int len;
    char filename[128];
};

#endif /* __DRIFTNET_H_ */

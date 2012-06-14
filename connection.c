/*
 * connection.c:
 * Connection objects.
 *
 * Copyright (c) 2002 Chris Lightfoot. All rights reserved.
 * Email: chris@ex-parrot.com; WWW: http://www.ex-parrot.com/~chris/
 *
 */

static const char rcsid[] = "$Id: connection.c,v 1.7 2003/10/16 11:56:37 chris Exp $";

#include <sys/types.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "driftnet.h"

/* connection_new SOURCE DEST SPORT DPORT
 * Allocate a new connection structure for data sent from SOURCE:SPORT to
 * DEST:DPORT. */
connection connection_new(const struct in_addr *src, const struct in_addr *dst, const short int sport, const short int dport) {
    connection c;
    alloc_struct(_connection, c);
    c->src = *src;
    c->dst = *dst;
    c->sport = sport;
    c->dport = dport;
    c->alloc = 16384;
    c->data = xmalloc(c->alloc);
    c->last = time(NULL);
    c->blocks = NULL;
    return c;
}

/* connection_delete CONNECTION
 * Free CONNECTION. */
void connection_delete(connection c) {
    struct datablock *b;
    for (b = c->blocks; b;) {
        struct datablock *b2;
        b2 = b->next;
        free(b);
        b = b2;
    }
    free(c->data);
    free(c);
}

/* connection_push CONNECTION DATA OFFSET LENGTH
 * Add LENGTH bytes of DATA received at OFFSET in the stream to CONNECTION. */
void connection_push(connection c, const unsigned char *data, unsigned int off, unsigned int len) {
    struct datablock *B, *b, *bl, BZ = {0};
    int a;

    assert(c->alloc > 0);
    if (off + len > c->alloc) {
        /* Allocate more memory. */
        do 
            c->alloc *= 2;
        while (off + len > c->alloc);
        c->data = (unsigned char*)xrealloc(c->data, c->alloc);
    }

    memcpy(c->data + off, data, len);

    if (off + len > c->len) c->len = off + len;
    c->last = time(NULL);
    
    B = xmalloc(sizeof *B);
    *B = BZ;
    B->off = off;
    B->len = len;
    B->dirty = 1;
    B->next = NULL;
    
    /* Insert the block into the sorted block list. */
    for (b = c->blocks, bl = NULL; ; bl = b, b = b->next) {
        if ((!b || off <= b->off) && (!bl || off > bl->off)) {
            B->next = b;
            if (bl)
                bl->next = B;
            else
                c->blocks = B;
            break;
        }
    }

    /* Now go through the list combining blocks. */
    do {
        a = 0;
        for (b = c->blocks; b; b = b->next) {
            if (b->next && b->off + b->len >= b->next->off) {
                struct datablock *bb;
                bb = b->next;
                b->len = (bb->off + bb->len) - b->off;
                b->next = bb->next;
                b->dirty = 1;
                free(bb);
                ++a;
            }
        }
    } while (a);
}


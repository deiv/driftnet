/*
 * connection.c:
 *
 * Copyright (c) 2002 Chris Lightfoot. All rights reserved.
 * Email: chris@ex-parrot.com; WWW: http://www.ex-parrot.com/~chris/
 *
 */

static const char rcsid[] = "$Id: connection.c,v 1.3 2002/07/08 23:32:33 chris Exp $";

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "driftnet.h"

/* connection_new:
 * Allocate a new connection structure between the given addresses. */
connection connection_new(const struct in_addr *src, const struct in_addr *dst, const short int sport, const short int dport) {
    connection c = (connection)calloc(1, sizeof(struct _connection));
    c->src = *src;
    c->dst = *dst;
    c->sport = sport;
    c->dport = dport;
    c->alloc = 16384;
    c->data = c->gif = c->jpeg = c->mpeg = malloc(c->alloc);
    c->last = time(NULL);
    c->blocks = NULL;
    return c;
}

/* connection_delete:
 * Free the named connection structure. */
void connection_delete(connection c) {
    free(c->data);
    free(c);
}

/* connection_push:
 * Put some more data in a connection. */
void connection_push(connection c, const unsigned char *data, unsigned int off, unsigned int len) {
    size_t goff = c->gif - c->data, joff = c->jpeg - c->data, moff = c->mpeg - c->data;
    struct datablock *B, *b, *bl, BZ = {0};
    int a;

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
    c->mpeg = c->data + moff;
    memcpy(c->data + off, data, len);

    if (off + len > c->len) c->len = off + len;
    c->last = time(NULL);
    
    B = malloc(sizeof *B);
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
/*
        {
            printf("%p: ", c);
            for (b = c->blocks; b; b = b->next)
                printf("[%d (%d) -> %d] ", b->off, b->len, b->off + b->len);
            printf("\n");
        }
*/
}


/**
 * @file connection.c
 *
 * @brief Connection handling.
 * @author David Suárez
 * @author Chris Lightfoot
 * @date Sun, 28 Oct 2018 16:14:56 +0100
 *
 * Copyright (c) 2002 Chris Lightfoot.
 * Email: chris@ex-parrot.com; WWW: http://www.ex-parrot.com/~chris/
 *
 * Copyright (c) 2018 David Suárez.
 * Email: david.sephirot@gmail.com
 *
 */

#include <compat/compat.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h> /* On many systems (Darwin...), stdio.h is a prerequisite. */
#include <string.h>
#include <time.h>

#include "common/util.h"
#include "media/media.h"

#include "pcap_engine.h"
#include "connection.h"

/* slots for storing information about connections */
static connection *slots = NULL;
static unsigned int slotsused, slotsalloc=0;

static char* print_ipport_pair(const struct sockaddr *addr, char *buf, size_t buf_len);

void connection_alloc_slots(void)
{
	slotsused = 0;
	slotsalloc = 64;
	slots = (connection*) xcalloc(slotsalloc, sizeof(connection));
}

void connection_free_slots(void)
{
	connection *C;

	for (C = slots; C < slots + slotsalloc; ++C) {
		if (*C)
			connection_delete(*C);
	}

	xfree(slots);
}

/* alloc_connection:
 * Find a free slot in which to allocate a connection object. */
connection *alloc_connection(void)
{
	connection *C;

	for (C = slots; C < slots + slotsalloc; ++C) {
		if (!*C)
			return C;
	}

	/* No connection slots left. */
	slots = (connection*) xrealloc(slots, slotsalloc * 2 * sizeof(connection));
	memset(slots + slotsalloc, 0, slotsalloc * sizeof(connection));
	C = slots + slotsalloc;
	slotsalloc *= 2;

	return C;
}

/*
 * Find a connection running between the two named addresses.
 */
connection *find_connection(const struct sockaddr *src,
		const struct sockaddr *dst)
{
	connection *C;

	for (C = slots; C < slots + slotsalloc; ++C) {
		connection c = *C;
		if (c && memcmp(&(c->src), src, sizeof(struct sockaddr_storage)) == 0
				&& memcmp(&(c->dst), dst, sizeof(struct sockaddr_storage)) == 0)
			return C;
	}

	return NULL;
}

/*
 * Return a string of the form w.x.y.z:foo -> a.b.c.d:bar for a pair of
 * addresses and ports.
 */
char *connection_string(const struct sockaddr *src, const struct sockaddr *dst)
{
	#define CONNECTION_STRING_LEN 272 /* 272 = (128 (ipv6 max) * 2)  16 */

	static char buf[CONNECTION_STRING_LEN];
	size_t len = CONNECTION_STRING_LEN;
	char *p;

	p = buf;
	//bzero(p, CONNECTION_STRING_LEN);

	print_ipport_pair(src, p, len);

	len = strlen(buf);
	p = buf + len;

	snprintf(p, 5, " -> ");

	len = strlen(buf);
	p = buf + len;

	print_ipport_pair(dst, p, CONNECTION_STRING_LEN - len);

	return buf;
}

char* print_ipport_pair(const struct sockaddr *addr, char *buf, size_t buf_len)
{
    struct sockaddr_in *v4;
    struct sockaddr_in6 *v6;
    int port;
    size_t plen;
    char *p = buf;

    switch (addr->sa_family) {
        case AF_INET:
    	    v4 = (struct sockaddr_in *)addr;
    	    inet_ntop(v4->sin_family, &v4->sin_addr.s_addr, buf, buf_len);
	        port = ntohs(v4->sin_port);
	        break;

        case AF_INET6:
    	    v6 = (struct sockaddr_in6 *)addr;
    	    inet_ntop(v6->sin6_family, &v6->sin6_addr.s6_addr, buf, buf_len);
	        port = ntohs(v6->sin6_port);
	        break;

        default:
            snprintf(buf, buf_len, "(unknown family)");
	        port = 0;
    }

    plen = strlen(buf);
    p += plen;

    snprintf(p, buf_len - plen, ":%d", port);

    return buf;
}

/* sweep_connections:
 * Free finished connection slots. */
#define TIMEOUT 5
#define MAXCONNECTIONDATA   (8 * 1024 * 1024)

void sweep_connections(void)
{
	time_t now;
	connection *C;

	now = time(NULL );

	for (C = slots; C < slots + slotsalloc; ++C) {
		if (*C) {
			connection c = *C;
			/* We discard connections which have seen no activity for TIMEOUT
			 * or for which a FIN has been seen and for which there are no
			 * gaps in the stream, or where more than MAXCONNECTIONDATA have
			 * been captured. */
			if ((now - c->last) > TIMEOUT
					|| (c->fin && (!c->blocks || !c->blocks->next))
					|| c->len > MAXCONNECTIONDATA) {
				extract_media(c);
				connection_delete(c);
				*C = NULL;
			}
		}
	}
}

/* connection_new SOURCE DEST SPORT DPORT
 * Allocate a new connection structure for data sent from SOURCE:SPORT to
 * DEST:DPORT. */
connection connection_new(const struct sockaddr *src,
		const struct sockaddr *dst)
{
	connection c;

	alloc_struct(_connection, c);

	memcpy(&c->src, src, sizeof(struct sockaddr_storage));
	memcpy(&c->dst, dst, sizeof(struct sockaddr_storage));

	c->alloc = 16384;
	c->data = xmalloc(c->alloc);
	c->last = time(NULL );
	c->blocks = NULL;

	return c;
}

/* connection_delete CONNECTION
 * Free CONNECTION. */
void connection_delete(connection c)
{
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
void connection_push(connection c, const unsigned char *data, unsigned int off,
		unsigned int len)
{
	struct datablock *B, *b, *bl, BZ = { 0 };
	int a;

	assert(c->alloc > 0);

	if (off + len > c->alloc) {
		/* Allocate more memory. */
		do
			c->alloc *= 2;
		while (off + len > c->alloc);
		c->data = (unsigned char*) xrealloc(c->data, c->alloc);
	}

	memcpy(c->data + off, data, len);

	if (off + len > c->len)
		c->len = off + len;
	c->last = time(NULL );

	B = xmalloc(sizeof *B);
	*B = BZ;
	B->off = off;
	B->len = len;
	B->dirty = 1;
	B->next = NULL;

	/* Insert the block into the sorted block list. */
	for (b = c->blocks, bl = NULL ;; bl = b, b = b->next) {
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

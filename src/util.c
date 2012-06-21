/*
 * util.c:
 * Various utility functions.
 *
 * Copyright (c) 2012 David Suárez. All rights reserved.
 * Email: david.sephirot@gmail.com
 *
 * Copyright (c) 2003 Chris Lightfoot. All rights reserved.
 * Email: chris@ex-parrot.com; WWW: http://www.ex-parrot.com/~chris/
 *
 */

#ifdef HAVE_CONFIG_H
    #include <config.h>
#endif
#include "compat.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h> /* On many systems (Darwin...), stdio.h is a prerequisite. */

#if HAVE_STRING_H
    #include <string.h>
#endif

#if HAVE_NANOSLEEP
    #include <time.h>
//#include <sys/time.h>
#elif HAVE_USLEEP
    #include <unistd.h>
#endif

#include "util.h"

/* xmalloc COUNT
 * Malloc, and abort if malloc fails. */
void *xmalloc(size_t n) {
    void *v;
    v = malloc(n);
    if (!v) abort();
    return v;
}

/* xcalloc NITEMS COUNT
 * As above. */
void *xcalloc(size_t n, size_t m) {
    void *v;
    v = calloc(n, m);
    if (!v) abort();
    return v;
}

/* xrealloc PTR COUNT
 * As above. */
void *xrealloc(void *w, size_t n) {
    void *v;
    v = realloc(w, n);
    if (n != 0 && !v) abort();
    return v;
}

/* xfree PTR
 * Free, ignoring a passed NULL value. */
void xfree(void *v) {
    if (v) free(v);
}

/* xstrdup:
 * Strdup, aborting on failure. */
char *xstrdup(const char *s) {
    char *t;
    t = xmalloc(strlen(s) + 1);
    strcpy(t, s);
    return t;
}

/* memstr:
 * Locate needle, of length n_len, in haystack, of length h_len, returning NULL.
 */
unsigned char *memstr(const unsigned char *haystack, const size_t hlen,
		const unsigned char *needle, const size_t nlen)
{
	char *p;

	for (p = haystack; p <= (haystack - nlen + hlen); p++)
	{
		if (memcmp(p, needle, nlen) == 0)
			return p; /* found */
	}
	return NULL;
}

void xnanosleep(long nanosecs)
{
#if HAVE_NANOSLEEP
    struct timespec tm = {0, nanosecs};

    nanosleep(&tm, NULL);

#elif HAVE_USLEEP
    unsigned int microsecs = (nanosecs < 1000) ? 1 : nanosecs / 1000;

    usleep(microsecs); /* obsolete: POSIX.1-2001 */

#else
    /* sleep() can't help ... */
    #error cannot find an usable sleep function
#endif
}

/*
 * dump_data
 *
 * Print some binary data on a file descriptor.
 *
 * XXX: Unused
 */
void dump_data(FILE *fp, const unsigned char *data, const unsigned int len) {
    const unsigned char *p;
    for (p = data; p < data + len; ++p) {
        if (isprint((int)*p)) fputc(*p, fp);
        else fprintf(fp, "\\x%02x", (unsigned int)*p);
    }
}

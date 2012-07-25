/*
 * util.c:
 * Various utility functions.
 *
 * Copyright (c) 2003 Chris Lightfoot. All rights reserved.
 * Email: chris@ex-parrot.com; WWW: http://www.ex-parrot.com/~chris/
 *
 */

static const char rcsid[] = "$Id: util.c,v 1.1 2003/08/25 12:24:08 chris Exp $";

#ifdef HAVE_CONFIG_H
    #include <config.h>
#endif
#include "compat.h"

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

#include "driftnet.h"

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
 * Uses the Boyer-Moore search algorithm. Cf.
 *  http://www-igm.univ-mlv.fr/~lecroq/string/node14.html */
unsigned char *memstr(const unsigned char *haystack, const size_t hlen,
                             const unsigned char *needle, const size_t nlen) {
    int skip[256], k;

    if (nlen == 0) return (unsigned char*)haystack;

    /* Set up the finite state machine we use. */
    for (k = 0; k < 256; ++k) skip[k] = nlen;
    for (k = 0; k < nlen - 1; ++k) skip[needle[k]] = nlen - k - 1;

    /* Do the search. */
    for (k = nlen - 1; k < hlen; k += skip[haystack[k]]) {
        int i, j;
        for (j = nlen - 1, i = k; j >= 0 && haystack[i] == needle[j]; j--) i--;
        if (j == -1) return (unsigned char*)(haystack + i + 1);
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

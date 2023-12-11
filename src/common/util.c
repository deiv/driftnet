/**
 * @file util.c
 *
 * @brief Various wrappers around some utility functions.
 *
 * @author David Suárez
 * @author Chris Lightfoot
 * @date Sun, 21 Oct 2018 18:41:11 +0200
 *
 * Copyright (c) 2018-2024 David Suárez.
 * Email: david.sephirot@gmail.com
 *
 * Copyright (c) 2003 Chris Lightfoot.
 * Email: chris@ex-parrot.com; WWW: http://www.ex-parrot.com/~chris/
 *
 */

#include "compat.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h> /* On many systems (Darwin...), stdio.h is a prerequisite. */

#if HAVE_STRING_H
    #include <string.h>
#endif

#if HAVE_NANOSLEEP
    #include <time.h>
#elif HAVE_USLEEP
    #include <unistd.h>
#endif

#include "util.h"

/*
 * Malloc, and abort if malloc fails.
 */
void *xmalloc(size_t n)
{
    void *v;

    v = malloc(n);
    if (!v) abort();

    return v;
}

/*
 * xcalloc NITEMS COUNT
 */
void *xcalloc(size_t n, size_t m) {
    void *v;
    v = calloc(n, m);
    if (!v) abort();
    return v;
}

/*
 * xrealloc PTR COUNT
 */
void *xrealloc(void *w, size_t n) {
    void *v;
    v = realloc(w, n);
    if (n != 0 && !v) abort();
    return v;
}

/*
 * Free, ignoring a passed NULL value.
 */
void xfree(void *v) {
    if (v) free(v);
}

/*
 * Strdup, aborting on failure.
 */
char *xstrdup(const char *s) {
    char *t;
    t = xmalloc(strlen(s) + 1);
    strcpy(t, s);
    return t;
}

/*
 * Locate needle, of length n_len, in haystack, of length h_len, returning NULL.
 */
unsigned char *memstr(const unsigned char *haystack, const size_t hlen,
		const unsigned char *needle, const size_t nlen)
{
	const unsigned char *p = haystack;

	for (; p <= (haystack - nlen + hlen); p++) {
		if (memcmp(p, needle, nlen) == 0)
			return (unsigned char*) p; /* found */
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

int mssleep(long miliseconds)
{
#if HAVE_NANOSLEEP
    struct timespec tm = {
        (int)(miliseconds / 1000),
        (miliseconds % 1000) * 1000000
    };

    nanosleep(&tm, NULL);

#elif HAVE_USLEEP
    usleep(miliseconds * 1000); /* obsolete: POSIX.1-2001 */

#else
    /* sleep() can't help ... */
#error cannot find an usable sleep function
#endif
}

char* compose_path(const char* base, const char* filename)
{
    char* filepath;
    int len;

    len  = strlen(base);
    len += strlen(filename);
    len += 2; /* for / and null */
    filepath = xmalloc(len);

    snprintf(filepath, len, "%s/%s", base, filename);

    return filepath;
}

const char* xbasename(const char* pathname)
{
    if (pathname == NULL) {
        return NULL;
    }

    char* base = strrchr(pathname, '/');

    if (base == NULL) {
        return NULL;
    }

    if (*(base+1) == 0) {
        return NULL;
    }

    return base + 1;
}

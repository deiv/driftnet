 /*
 * util.h:
 * Helper functions.
 *
 * Copyright (c) 2012 David Suárez. All rights reserved.
 * Email: david.sephirot@gmail.com
 *
 * Copyright (c) 2002 Chris Lightfoot. All rights reserved.
 * Email: chris@ex-parrot.com; WWW: http://www.ex-parrot.com/~chris/
 *
 */

#ifndef __UTIL_H__
#define __UTIL_H__

#ifdef HAVE_CONFIG_H
    #include <config.h>
#endif

void *xmalloc(size_t n);
void *xcalloc(size_t n, size_t m);
void *xrealloc(void *w, size_t n);
void xfree(void *v);
char *xstrdup(const char *s);

unsigned char *memstr(const unsigned char *haystack, const size_t hlen, const unsigned char *needle, const size_t nlen);

void xnanosleep(long nanosecs);

//void dump_data(FILE *fp, const unsigned char *data, const unsigned int len);



#endif /* __UTIL_H__ */

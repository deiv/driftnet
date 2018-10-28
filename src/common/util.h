/**
 * @file util.h
 *
 * @brief Various wrappers around some utility functions.
 * @author David Suárez
 * @author Chris Lightfoot
 * @date Sun, 21 Oct 2018 18:41:11 +0200
 *
 * Copyright (c) 2018 David Suárez.
 * Email: david.sephirot@gmail.com
 *
 * Copyright (c) 2003 Chris Lightfoot.
 * Email: chris@ex-parrot.com; WWW: http://www.ex-parrot.com/~chris/
 *
 */

#ifndef __UTIL_H__
#define __UTIL_H__

#ifdef HAVE_CONFIG_H
    #include <config.h>
#endif

#include <stddef.h>

/**
 * @brief Malloc, and abort if fails.
 *
 * @param n size of memory to malloc
 * @return pointer to the allocated mem
 */
void *xmalloc(size_t n);

/**
 * @brief Calloc, and abort if fails.
 *
 * @param n number of elements to alloc
 * @param m size of elements
 * @return pointer to the allocated mem
 */
void *xcalloc(size_t n, size_t m);

/**
 * @brief Realloc, and abort if fails.
 *
 * @param w pointer to the memory
 * @param n size of memory to recalloc
 * @return pointer to the reallocated mem
 */
void *xrealloc(void *w, size_t n);

/**
 * @brief Free, ignoring a passed NULL value.
 *
 * @param v the memory to free
 */
void xfree(void *v);

/**
 * @brief Strdup, and abort if fails.
 *
 * @param n original string
 * @return duplicated string
 */
char *xstrdup(const char *s);

/**
 * @brief Locate needle, of length n_len, in haystack, of length h_len, returning NULL.
 *
 * @param haystack string to search in
 * @param size of haystack
 * @param needle string to search for
 * @param nlen size of needle
 * @return a pointer to the found string or NULL if nothing found
 */

unsigned char *memstr(const unsigned char *haystack, const size_t hlen, const unsigned char *needle, const size_t nlen);

/**
 * @brief Sleep for x nanoseconds
 *
 * @param nanosecs nanosecs to sleep
 */
void xnanosleep(long nanosecs);

/**
 * @brief Make P point to a new struct S, initialised as if in static storage (like = {0}).
 */
#define alloc_struct(S, p)  p = xmalloc(sizeof *p); memset(p, 0, sizeof *p);

//#define alloc_struct(S, p)  do { struct S as__z = {0}; p = xmalloc(sizeof *p); *p = as__z; } while (0)

#endif /* __UTIL_H__ */

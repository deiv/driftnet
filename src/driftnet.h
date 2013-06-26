/*
 * driftnet.h:
 * Pick out images from passing network traffic.
 *
 * Copyright (c) 2001 Chris Lightfoot.
 * Email: chris@ex-parrot.com; WWW: http://www.ex-parrot.com/~chris/
 *
 * $Id: driftnet.h,v 1.13 2004/04/08 23:06:29 chris Exp $
 *
 */

#ifndef __DRIFTNET_H_ /* include guard */
#define __DRIFTNET_H_

#ifdef HAVE_CONFIG_H
    #include <config.h>
#endif

#define PROGNAME DRIFTNET_PROGNAME

/* alloc_struct S P
 * Make P point to a new struct S, initialised as if in static storage (like = {0}).
 */
//#define alloc_struct(S, p)  do { struct S as__z = {0}; p = xmalloc(sizeof *p); *p = as__z; } while (0)
#define alloc_struct(S, p)  p = xmalloc(sizeof *p); memset(p, 0, sizeof *p);

#endif /* __DRIFTNET_H_ */

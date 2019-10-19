/**
 * @file compat.h
 *
 * @brief Try to make things more portable
 * @author David Suárez
 * @date Sun, 21 Oct 2018 18:41:11 +0200
 *
 * Copyright (c) 2018 David Suárez.
 * Email: david.sephirot@gmail.com
 *
 */

#ifndef __COMPAT_H__
#define __COMPAT_H__

#ifdef HAVE_CONFIG_H
    #include <config.h>
#endif

#ifdef __FreeBSD__
    #include <sys/types.h>
#endif

#if HAVE_STDINT_H
    #include <stdint.h>     /* C99 integer types (less poluted than inttypes.h). */
#elif HAVE_INTTYPES_H
    #include <inttypes.h>   /* Some implementations have inttypes.h but not stdint.h (Solaris 7). */
#elif HAVE_SYS_TYPES_H && !__FreeBSD__
    #include <sys/types.h>  /* Solaris etc. XXX: not sure (old code) */
#else
    #error "Not a valid integer types include found"
#endif


#define FALSE 0
#define TRUE 1

#endif /* __COMPAT_H__ */

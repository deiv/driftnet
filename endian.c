/*
 * endian.c:
 * Determine platform endianness.
 *
 * Copyright (c) 2002 . All rights reserved.
 * Email: chris@ex-parrot.com; WWW: http://www.ex-parrot.com/~chris/
 *
 */

static const char rcsid[] = "$Id: endian.c,v 1.5 2002/06/10 21:25:48 chris Exp $";

#include <stdio.h>
#ifdef USE_SYS_TYPES_H
#   include <sys/types.h>       /* Solaris etc. */
#else
#   include <stdint.h>          /* C99 standard. */
#endif

int main(void) {
#if defined(LITTLE_ENDIAN) || defined(_LITTLE_ENDIAN)
    printf("-DDRIFTNET_LITTLE_ENDIAN\n");
	return 0;
#elif defined(BIG_ENDIAN) || defined(_BIG_ENDIAN)
    printf("-DDRIFTNET_BIG_ENDIAN\n");
	return 0;
#else
    uint32_t a = 0;
    *((uint8_t*)&a) = 0xff;
    if (a == 0xff000000)
        printf("-DDRIFTNET_BIG_ENDIAN\n");
    else if (a == 0x000000ff)
        printf("-DDRIFTNET_LITTLE_ENDIAN\n");
    else
        return -1; /* don't know. */
#endif  /* endianness test */
    return 0;
}

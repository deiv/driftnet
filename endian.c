/*
 * endian.c:
 * Determine platform endianness.
 *
 * Copyright (c) 2002 . All rights reserved.
 * Email: chris@ex-parrot.com; WWW: http://www.ex-parrot.com/~chris/
 *
 */

static const char rcsid[] = "$Id: endian.c,v 1.1 2002/06/01 15:21:48 chris Exp $";

#include <stdio.h>
#ifdef USE_SYS_INT_TYPES_H
#   include <sys/int_types.h>   /* Solaris etc. */
#else
#   include <stdint.h>          /* C99 standard. */
#endif

int main(void) {
    uint32_t a = 0;
    *((uint8_t*)&a) = 0xff;
    if (a == 0xff000000)
        printf("-DBIG_ENDIAN\n");
    else if (a == 0x000000ff)
        printf("-DLITTLE_ENDIAN\n");
    else
        return -1; /* don't know. */
    return 0;
}

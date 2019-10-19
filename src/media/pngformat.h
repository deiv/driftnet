/*
 * pngformat.h:
 * Png file handling.
 *
 * Copyright (c) 2012 David Suárez.
 * Email: david.sephirot@gmail.com
 *
 */

#ifndef __PNGFORMAT_H__
#define __PNGFORMAT_H__

#ifdef HAVE_CONFIG_H
    #include <config.h>
#endif
#include "compat/compat.h"

#define PNG_CODE_LEN 4
#define PNG_CRC_LEN  4
#define PNG_SIG_LEN  8

struct png_chunk {
   uint32_t datalen;
   unsigned char code[PNG_CODE_LEN];
};

#endif /*__PNGFORMAT_H__ */

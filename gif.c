/*
 * gif.c:
 * GIF file support.
 *
 * Copyright (c) 2001 Chris Lightfoot. All rights reserved.
 * Email: chris@ex-parrot.com; WWW: http://www.ex-parrot.com/~chris/
 *
 */

#ifndef NO_DISPLAY_WINDOW

static const char rcsid[] = "$Id: gif.c,v 1.4 2002/07/08 20:57:17 chris Exp $";

#include <gif_lib.h>

#include "img.h"

/* gif_load_hdr:
 * Find width/height of GIF file.
 */
int gif_load_hdr(img I) {
    GifFileType *g;
    g = I->us = DGifOpenFileHandle(fileno(I->fp));
    if (!I->us) {
        I->err = IE_HDRFORMAT;
        return 0;
    }

    I->width = g->SWidth;
    I->height = g->SHeight;

    return 1;
}

/* gif_abort_load:
 * Abort loading a GIF file after the header is done.
 */
int gif_abort_load(img I) {
    DGifCloseFile((GifFileType*)I->us);
    return 1;
}

/* gif_load_img:
 * Load GIF image.
 */
int gif_load_img(img I) {
    GifFileType *g = I->us;
    struct SavedImage *si;
    int ret = 0;
    unsigned char *p, *end;
    GifColorType *pal;
    pel *q;

    if (DGifSlurp(g) == GIF_ERROR) {
        I->err = IE_IMGFORMAT;
        return 0;
    }

    /* Now allocate memory and copy the image into it. */
    img_alloc(I);

    /* Retrieve only the first image. */
    if (g->ImageCount < 1) {
        I->err = IE_IMGFORMAT;
        goto fail;
    }

    si = g->SavedImages;
    if (si->ImageDesc.Width != I->width || si->ImageDesc.Height != I->height) {
        I->err = IE_IMGFORMAT;
        goto fail;
    }

    if (si->ImageDesc.ColorMap)
        pal = si->ImageDesc.ColorMap->Colors;
    else
        pal = g->SColorMap->Colors;

    if (si->ImageDesc.Interlace) {
        int i;
        unsigned char *gifsrc = si->RasterBits;

        /* Deal with deranged interlaced GIF file. */
#define COPYROW(src, dest)      for (p = (src), q = (dest); p < (src) + I->width; ++p, ++q) \
                                    *q = PELA(pal[*p].Red, pal[*p].Green, pal[*p].Blue, *p == g->SBackGroundColor ? 255 : 0);

        /* Pass 1: every 8th row, starting at row 0. */
        for (i = 0; i < I->height; i += 8) {
            COPYROW(gifsrc, I->data[i]);
            gifsrc += I->width;
        }

        /* Pass 2: every 8th row, starting at row 4. */
        for (i = 4; i < I->height; i += 8) {
            COPYROW(gifsrc, I->data[i]);
            gifsrc += I->width;
        }

        /* Pass 3: every 4th row, starting at row 2. */
        for (i = 2; i < I->height; i += 4) {
            COPYROW(gifsrc, I->data[i]);
            gifsrc += I->width;
        }

        /* Pass 4: every 2nd row, starting at row 1. */
        for (i = 1; i < I->height; i += 2) {
            COPYROW(gifsrc, I->data[i]);
            gifsrc += I->width;
        }
    } else 
        for (p = (unsigned char*)si->RasterBits, end = p + I->width * I->height, q = I->flat; p < end; ++p, ++q)
            *q = PELA(pal[*p].Red, pal[*p].Green, pal[*p].Blue, *p == g->SBackGroundColor ? 255 : 0);

    ret = 1;
fail:

    DGifCloseFile(g);
    
    return ret;
}

#endif /* !NO_DISPLAY_WINDOW */

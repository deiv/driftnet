/*
 * pnm.c:
 * PNM file support.
 *
 * Copyright (c) 2001 Chris Lightfoot. All rights reserved.
 * Email: chris@ex-parrot.com; WWW: http://www.ex-parrot.com/~chris/
 *
 */

static const char rcsid[] = "$Id: pnm.c,v 1.1 2001/07/15 11:07:33 chris Exp $";

#include <pnm.h>

#include "img.h"

/* pnm_load_hdr:
 * Find width/height of PNM file. Do this by seeing if the first two lines
 * look plausible.
 */
int pnm_load_hdr(img I) {
    char linebuf[40];
    int w, h;
    long pos;
    pos = ftell(I->fp);
    fseek(I->fp, 0, SEEK_SET);
    fgets(linebuf, 40, I->fp);
    if (*linebuf != 'P' || !strchr("0123456789", *(linebuf + 1))) {
        I->err = IE_HDRFORMAT;
        fseek(I->fp, pos, SEEK_SET);
        return 0;
    }
    if (fscanf(I->fp, "%d %d", &w, &h) != 2) {
        I->err = IE_HDRFORMAT;
        fseek(I->fp, pos, SEEK_SET);
        return 0;
    }
    I->width = w;
    I->height = h;
    fseek(I->fp, pos, SEEK_SET);
    return 1;
}

/* pnm_abort_load:
 * Abort loading a PNM file after the header is done.
 */
int pnm_abort_load(img I) {
    return 1;
}

/* pnm_load_img:
 * Load an image from a PNM file.
 */
int pnm_load_img(img I) {
    pel **irow;
    xel *row;
    xelval max;
    int w, h, fmt, y;

    img_alloc(I);
    pnm_pbmmaxval = 255;
    pnm_readpnminit(I->fp, &w, &h, &max, &fmt);
    if (w != I->width || h != I->height) {
        I->err = IE_HDRFORMAT;
        return 0;
    }

    row = pnm_allocrow(I->width);

    for (y = 0, irow = I->data; y < I->height; ++y, ++irow) {
        xel *p;
        pel *q;

        pnm_readpnmrow(I->fp, row, I->width, max, fmt);
        
        if (max == 255)
            for (p = row, q = *irow; p < row + I->width; ++p, ++q)
                *q = PEL(PPM_GETR(*p), PPM_GETG(*p), PPM_GETR(*p));
        else
            for (p = row, q = *irow; p < row + I->width; ++p, ++q)
                *q = PEL((255 * PPM_GETR(*p)) / max,
                         (255 * PPM_GETG(*p)) / max,
                         (255 * PPM_GETR(*p)) / max);
    }

    pnm_freerow(row);

    return 1;
}

/* pnm_save_img:
 * Write an image out into a PNM file.
 */
int pnm_save_img(const img I, FILE *fp) {
    pel **irow;
    xel *row;
    int y;

    pnm_writepnminit(fp, I->width, I->height, 255, PPM_FORMAT, 0);

    row = pnm_allocrow(I->width);
    for (y = 0, irow = I->data; y < I->height; ++y, ++irow) {
        pel *p;
        xel *q;
        for (p = *irow, q = row; q < row + I->width; ++p, ++q)
            PPM_ASSIGN(*q, GETR(*p), GETG(*p), GETB(*p));

        pnm_writepnmrow(fp, row, I->width, 255, PPM_FORMAT, 0);
    }

    pnm_freerow(row);

    return 1;
}

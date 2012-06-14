/*
 * img.c:
 * Image file type.
 *
 * Copyright (c) 2001 Chris Lightfoot. All rights reserved.
 * Email: chris@ex-parrot.com; WWW: http://www.ex-parrot.com/~chris/
 *
 */

#ifndef NO_DISPLAY_WINDOW

static const char rcsid[] = "$Id: img.c,v 1.9 2002/07/08 20:57:17 chris Exp $";

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "img.h"

#define INLINE  inline

#if 0
/* pnm.c */
int pnm_load_hdr(img I);
int pnm_abort_load(img I);
int pnm_load_img(img I);
int pnm_save_img(const img I, FILE *fp);
#endif

/* gif.c */
int gif_load_hdr(img I);
int gif_abort_load(img I);
int gif_load_img(img I);

/* jpeg.c */
int jpeg_load_hdr(img I);
int jpeg_abort_load(img I);
int jpeg_load_img(img I);
int jpeg_save_img(const img I, FILE *fp);

#if 0
/* raw.c */
int raw_load_img(img I);
int raw_save_img(img I, FILE *fp);
*/
#endif

/* Image file formats known about, and the routines used to load/save them. */
struct filedrv {
    imgtype type;
    char *suffices;
    int (*loadhdr)(img);
    int (*abortload)(img);
    int (*loadimg)(img);
    int (*saveimg)(const img, FILE*);
} filedrvs[] = {
/*
        {pnm,       ".pnm\0.ppm\0.pgm\0.pbm\0", pnm_load_hdr,   pnm_abort_load,     pnm_load_img,   pnm_save_img},
*/
        {gif,       ".gif\0",                   gif_load_hdr,   gif_abort_load,     gif_load_img,   NULL},
        {jpeg,      ".jpg\0.jpeg\0",            jpeg_load_hdr,  jpeg_abort_load,    jpeg_load_img,  jpeg_save_img},
/*
        {png,       ".png\0",                   png_load_hdr,   png_abort_load,     png_load_img,   png_save_img},
        {raw,       "",                         NULL,           raw_load_img,       NULL,           raw_save_img},
*/
    };

#define NUMFILEDRVS (sizeof(filedrvs) / sizeof(struct filedrv))

/* img_new:
 * Create a new empty image object.
 */
img img_new(void) {
    img I;
    I = (img)malloc(sizeof(struct _img));
    memset(I, 0, sizeof(struct _img));
    return I;
}

/* img_new_blank:
 * Create a new image object.
 */
img img_new_blank(const unsigned int width, const unsigned int height) {
    img I = img_new();
    I->width = width;
    I->height = height;
    I->load = header;

    return I;
}

/* img_alloc:
 * Allocate memory for an image object. The memory is allocated as a big
 * block, with pointers fixed up at the beginning.
 */
void img_alloc(img I) {
    pel **p, *q;
    I->data = (pel**)calloc(I->height * sizeof(pel*) + I->width * I->height * sizeof(pel), 1);
    I->flat = (pel*)(I->data + I->height);
    for (p = I->data, q = I->flat; p < I->data + I->height; ++p, q += I->width)
        *p = q;
}

/* img_delete:
 * Free memory associated with an image object.
 */
void img_delete(img I) {
    if (I->data) free(I->data);
    free(I);
}

/* img_load:
 * Load an image, or part of it, from the associated stream. Returns 1 on
 * success or 0 on failure.
 */
int img_load(img I, const imgstate howmuch, const imgtype type) {
    int i;
    if (type == unknown) {
        I->err = IE_UNKNOWNTYPE;
        return 0;
    } else if (!I->fp) {
        I->err = IE_NOSTREAM;
        return 0;
    } else if (howmuch == none) return 1;
    else for (i = 0; i < NUMFILEDRVS; ++i)
        if (filedrvs[i].type == type) {
            int r;
            if (howmuch == header && filedrvs[i].loadhdr) {
                r = filedrvs[i].loadhdr(I);
                if (r) I->load = howmuch;
                return r;
            } else if (filedrvs[i].loadimg) {
                /* May have to load header first. */
                if (I->load != header && filedrvs[i].loadhdr && !filedrvs[i].loadhdr(I))
                    return 0;
                I->load = header;
                r = filedrvs[i].loadimg(I);
                if (r) I->load = full;
                return r;
            }
        }

    I->err = IE_UNKNOWNTYPE;
    return 0;
}

/* img_load_stream:
 * Associate an image with a stream and load something from it.
 */
int img_load_stream(img I, FILE *fp, const imgstate howmuch, const imgtype type) {
    I->fp = fp;
    return img_load(I, howmuch, type);
}

/* img_load_file:
 * Load an image, or part of it, from a file.
 */
int img_load_file(img I, const char *name, const imgstate howmuch, const imgtype type) {
    if (howmuch == none) return 1;
    I->fp = fopen(name, "rb");
    if (!I->fp) {
        I->err = IE_SYSERROR;
        return 0;
    }

    if (type == unknown) {
        /* Try to figure out type. */
        char *p;
        int i;
        p = strrchr(name, '.');
        for (i = 0; i < NUMFILEDRVS; ++i) {
            char *q;
            for (q = filedrvs[i].suffices; *q; q += strlen(q) + 1)
                if (strcasecmp(p, q) == 0) {
                    I->type = filedrvs[i].type;
                    return img_load(I, howmuch, I->type);
                }
        }
    } else return img_load(I, howmuch, type);

    I->err = IE_UNKNOWNTYPE;
    return 0;
}

/* img_save_file:
 * Save an image in a file of the specified type.
 */
int img_save(const img I, FILE *fp, const imgtype type) {
    int i;
    if (type == unknown) {
        I->err = IE_UNKNOWNTYPE;
        return 0;
    } else for (i = 0; i < NUMFILEDRVS; ++i)
        if (filedrvs[i].type == type && filedrvs[i].saveimg)
            return filedrvs[i].saveimg(I, fp);

    I->err = IE_UNKNOWNTYPE;
    return 0;
}

/* img_clip_adj_x:
 * img_clip_adj_y:
 * Return an adjustment to the passed coordinate which will put it in the
 * clipping region for the image.
 */
INLINE int img_clip_adj_x(const img I, const int x) {
    if (x < 0) return -x;
    if (x >= I->width) return I->width - x;
    return 0;
}

INLINE int img_clip_adj_y(const img I, const int y) {
    if (y < 0) return -y;
    if (y >= I->height) return I->height - y;
    return 0;
}

/* img_clip:
 * Clip coordinates against an image.
 */
INLINE void img_clip(const img I, int *x, int *y) {
    *x += img_clip_adj_x(I, *x);
    *y += img_clip_adj_y(I, *y);
}

/* img_simple_blt:
 * Copy a rectangle, ignoring clipping and overlapping regions.
 */
INLINE void img_simple_blt(img dest, const int dx, const int dy, img src, const int sx, const int sy, const int w, const int h) {
    int y, y2;
    for (y = sy, y2 = dy; y < sy + h; ++y, ++y2)
        memcpy(dest->data[y2] + dx, src->data[y] + sx, w * sizeof(pel));
}

#if 0
/* img_blt:
 * Copy a rectangle from one location to another.
 */
void img_blt(img dest, const int dx, const int dy, img src, const int sx, const int sy, const int w, const int h) {
    int srcx, srcy, destx, desty, width, height;
    int Dx, Dy;

    /* Clip the regions. */
    srcx = sx;  srcy = sy;
    destx = dx; desty = dy;
    width = w;  height = h;

    /* Source TL. */
    Dx = img_clip_adj_x(src, srcx);
    Dy = img_clip_adj_y(src, srcy);
    srcx += Dx; destx += Dx;
    srcy += Dy; desty += Dy;
    width -= Dx; height -= Dy;

    if (width <= 0 || height <= 0) return;  /* Nothing to copy. */

    /* Source BR. */
    Dx = img_clip_adj_x(src, srcx + width - 1);
    Dy = img_clip_adj_y(src, srcy + height - 1);
    width -= Dx; height -= Dy;

    if (width <= 0 || height <= 0) return;  /* Nothing to copy. */

    /* Dest TL. */
    Dx = img_clip_adj_x(dest, destx);
    Dy = img_clip_adj_y(dest, desty);
    srcx += Dx; destx += Dx;
    srcy += Dy; desty += Dy;
    width -= Dx; height -= Dy;

    if (width <= 0 || height <= 0) return;  /* Nothing to copy. */
    
    /* Dest BR. */
    Dx = img_clip_adj_x(dest, destx + width - 1);
    Dy = img_clip_adj_y(dest, desty + height - 1);
    width -= Dx; height -= Dy;

    if (width <= 0 || height <= 0) return;  /* Nothing to copy. */
    
    if (dest == src) {
        if (srcx == destx && srcy == desty) return;
        else if (srcy + height < desty || desty + height < srcy || srcx + width < destx || destx + width < srcx)
            /* No overlap. */
            img_simple_blt(dest, destx, desty, src, srcx, srcy, width, height);
        else {
        }
    } else
        /* Different images; simple copy. */
        img_simple_blt(dest, destx, desty, src, srcx, srcy, width, height);
}

#endif

#endif /* !NO_DISPLAY_WINDOW */

/*
 * img.h:
 * Image file type.
 *
 * Copyright (c) 2001 Chris Lightfoot. All rights reserved.
 * Email: chris@ex-parrot.com; WWW: http://www.ex-parrot.com/~chris/
 *
 * $Id: img.h,v 1.7 2002/07/08 20:57:17 chris Exp $
 *
 */

#ifndef NO_DISPLAY_WINDOW

#ifndef __IMG_H_ /* include guard */
#define __IMG_H_

#ifdef USE_SYS_TYPES_H
#   include <sys/types.h>   /* Solaris etc. */
#else
#   include <stdint.h>          /* C99 standard */
#endif

#include <stdio.h>

typedef uint8_t chan;
typedef uint32_t pel;

/* Yuk. GDKRGB expects data in a specific ordering. */
#if defined(DRIFTNET_LITTLE_ENDIAN)
#   define PEL(r, g, b)        ((pel)((chan)(r) | ((chan)(g) << 8) | ((chan)(b) << 16)))
#   define PELA(r, g, b, a)    ((pel)((chan)(r) | ((chan)(g) << 8) | ((chan)(b) << 16) | ((chan)(a) << 24)))

#   define GETR(p)             ((chan)(((p) & (pel)0x000000ff)      ))
#   define GETG(p)             ((chan)(((p) & (pel)0x0000ff00) >>  8))
#   define GETB(p)             ((chan)(((p) & (pel)0x00ff0000) >> 16))
#   define GETA(p)             ((chan)(((p) & (pel)0xff000000) >> 24))
#elif defined(DRIFTNET_BIG_ENDIAN)
#   define PEL(r, g, b)        ((pel)(((chan)(r) << 24) | ((chan)(g) << 16) | ((chan)(b) << 8)))
#   define PELA(r, g, b, a)    ((pel)(((chan)(r) << 24) | ((chan)(g) << 16) | ((chan)(b) << 8) | ((chan)(a))))

#   define GETR(p)             ((chan)(((p) & (pel)0xff000000) >> 24))
#   define GETG(p)             ((chan)(((p) & (pel)0x00ff0000) >> 16))
#   define GETB(p)             ((chan)(((p) & (pel)0x0000ff00) >>  8))
#   define GETA(p)             ((chan)(((p) & (pel)0x000000ff)      ))
#else
#   error "no endianness defined"
#endif

typedef enum { unknown = 0, pnm = 1, gif = 2, jpeg = 3, png = 4, raw = 5 } imgtype;
typedef enum { none = 0, header = 1, full = 2 } imgstate;

typedef enum {
    IE_OK = 0,
    IE_SYSERROR,
    IE_NOSTREAM,
    IE_UNKNOWNTYPE,
    IE_HDRFORMAT,
    IE_IMGFORMAT
} imgerr;

typedef struct _img {
    imgtype type;
    imgstate load;
    unsigned int width, height;
    pel **data, *flat;
    FILE *fp;
    void *us;
    imgerr err;
} *img;

img img_new(void);
img img_new_blank(const unsigned int width, const unsigned int height);
void img_alloc(img I);

int img_load(img I, const imgstate howmuch, const imgtype type);
int img_load_stream(img I, FILE *fp, const imgstate howmuch, const imgtype type);
int img_load_file(img I, const char *name, const imgstate howmuch, const imgtype type);

int img_save(const img I, FILE *fp, const imgtype type);

/* img img_clone(const img I); */

void img_delete(img I);

void img_simple_blt(img dest, const int dx, const int dy, img src, const int sx, const int sy, const int w, const int h);

#endif /* __IMG_H_ */

#endif /* !NO_DISPLAY_WINDOW */

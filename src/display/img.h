/*
 * img.h:
 * Image file type.
 *
 * Copyright (c) 2001 Chris Lightfoot.
 * Email: chris@ex-parrot.com; WWW: http://www.ex-parrot.com/~chris/
 *
 * $Id: img.h,v 1.9 2003/11/03 10:40:23 chris Exp $
 *
 */

#ifndef __IMG_H_ /* include guard */
#define __IMG_H_

#ifdef HAVE_CONFIG_H
    #include <config.h>
#endif
#include "compat/compat.h"

#include <stdio.h>

typedef uint8_t chan;
typedef uint32_t pel;

/*
 * Yuk. GDKRGB expects data in a specific ordering:
 */
#if defined(DRIFTNET_LITTLE_ENDIAN)
/*
 *   - for GTK2 gdk_draw_rgb_32_image():               0x00BBGGRR
 *   - for GTK3 cairo_image_surface_create_for_data(): 0x00RRGGBB
 */
#ifdef DISABLE_GTK3
#   define PEL(r, g, b)        ((pel)((chan)(r) | ((chan)(g) << 8) | ((chan)(b) << 16)))
#   define PELA(r, g, b, a)    ((pel)((chan)(r) | ((chan)(g) << 8) | ((chan)(b) << 16) | ((chan)(a) << 24)))

#   define GETR(p)             ((chan)(((p) & (pel)0x000000ff)      ))
#   define GETG(p)             ((chan)(((p) & (pel)0x0000ff00) >>  8))
#   define GETB(p)             ((chan)(((p) & (pel)0x00ff0000) >> 16))
#   define GETA(p)             ((chan)(((p) & (pel)0xff000000) >> 24))
#else
#   define PEL(r, g, b)        ((pel)((chan)(r) << 16 | ((chan)(g) << 8) | ((chan)(b))))
#   define PELA(r, g, b, a)    ((pel)((chan)(r) << 16 | ((chan)(g) << 8) | ((chan)(b)) | ((chan)(a) << 24)))

#   define GETR(p)             ((chan)(((p) & (pel)0x00ff0000) >> 16))
#   define GETG(p)             ((chan)(((p) & (pel)0x0000ff00) >>  8))
#   define GETB(p)             ((chan)(((p) & (pel)0x000000ff)      ))
#   define GETA(p)             ((chan)(((p) & (pel)0xff000000) >> 24))
#endif
#elif defined(DRIFTNET_BIG_ENDIAN)
/*
 *  - for GTK2 gdk_draw_rgb_32_image():               0xRRGGBB00
 *  - for GTK3 cairo_image_surface_create_for_data(): 0xBBGGRR00
 */
#ifdef DISABLE_GTK3
#   define PEL(r, g, b)        ((pel)(((chan)(r) << 24) | ((chan)(g) << 16) | ((chan)(b) << 8)))
#   define PELA(r, g, b, a)    ((pel)(((chan)(r) << 24) | ((chan)(g) << 16) | ((chan)(b) << 8) | ((chan)(a))))

#   define GETR(p)             ((chan)(((p) & (pel)0xff000000) >> 24))
#   define GETG(p)             ((chan)(((p) & (pel)0x00ff0000) >> 16))
#   define GETB(p)             ((chan)(((p) & (pel)0x0000ff00) >>  8))
#   define GETA(p)             ((chan)(((p) & (pel)0x000000ff)      ))
#else
#   define PEL(r, g, b)        ((pel)(((chan)(r) << 8) | ((chan)(g) << 16) | ((chan)(b) << 24)))
#   define PELA(r, g, b, a)    ((pel)(((chan)(r) << 8) | ((chan)(g) << 16) | ((chan)(b) << 24) | ((chan)(a))))

#   define GETR(p)             ((chan)(((p) & (pel)0x0000ff00) >>  8))
#   define GETG(p)             ((chan)(((p) & (pel)0x00ff0000) >> 16))
#   define GETB(p)             ((chan)(((p) & (pel)0xff000000) >> 24))
#   define GETA(p)             ((chan)(((p) & (pel)0x000000ff)      ))
#endif
#else
#   error "no endianness defined"
#endif


typedef enum { unknown = 0, pnm = 1, gif = 2, jpeg = 3, png = 4, webp = 5, raw = 6 } imgtype;
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

/*
 * png.c:
 *
 * Copyright (c) 2001 Chris Lightfoot. All rights reserved.
 * Email: chris@ex-parrot.com; WWW: http://www.ex-parrot.com/~chris/
 *
 */

#ifndef NO_DISPLAY_WINDOW

static const char rcsid[] = "$Id: png.c,v 1.3 2002/06/10 21:25:48 chris Exp $";

#include <stdlib.h>
#include <png.h>
#include "img.h"

struct png_state {
    png_structp png;
    png_infop info;
    int depth;
    int color;
};

static void png_done(img I) {
    struct png_state *p = I->us;
    if (p) {
	png_destroy_read_struct(&p->png, &p->info, 0);
	free(p);
    }
}

/* png_catch_error: */
/* Catch errors signalled by libpng, clean up and go on. */
void png_catch_error(png_structp png_ptr, png_const_charp error_msg) {
   jmp_buf *jmpbuf_ptr;
   
   fprintf(stderr, "libpng error: %s (skipping image).\n", error_msg);
   fflush(stderr);

   jmpbuf_ptr=png_jmpbuf(png_ptr);
   if (jmpbuf_ptr==NULL) {
      fprintf(stderr, "libpng unrecoverable error, terminating.\n");
      fflush(stderr);
      exit(20);
   }

   longjmp(jmpbuf_ptr, 1);
}
         
/* png_load_hdr:
 * Load the header of a PNG file. */
int png_load_hdr(img I) {
    struct png_state *p = calloc(sizeof(struct png_state), 1);
    if (p == 0) {
	I->err = IE_HDRFORMAT;
	return 0;
    }
    I->us = p;
    p->png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, png_catch_error, NULL);
    if (p->png == 0) {
	png_done(I);
	I->err = IE_HDRFORMAT;
	return 0;
    }

    if (setjmp(png_jmpbuf(p->png))) {
       png_done(I);
       I->err = IE_HDRFORMAT;
       return 0;
    }

    p->info = png_create_info_struct(p->png);
    if (p->info == 0) {
	png_done(I);
	I->err = IE_HDRFORMAT;
	return 0;
    }
    png_init_io(p->png, I->fp);
    png_read_info(p->png, p->info);
    png_get_IHDR(p->png, p->info,
		 (png_uint_32 *)&I->width, (png_uint_32 *)&I->height,
		 &p->depth, &p->color, 0, 0, 0);
    switch (p->color) {
      case PNG_COLOR_TYPE_GRAY:
      case PNG_COLOR_TYPE_GRAY_ALPHA:
      case PNG_COLOR_TYPE_PALETTE:
      case PNG_COLOR_TYPE_RGB:
      case PNG_COLOR_TYPE_RGB_ALPHA:
	return 1;
      default:
	png_done(I);
	I->err = IE_IMGFORMAT;
	return 0;
    }
}

/* png_abort_load:
 * Abort loading a PNG after the header is done. */
int png_abort_load(img I) {
    png_done(I);
    return 1;
}

/* png_load_img:
 * Read a PNG file into an image. */
int png_load_img(img I) {
    struct png_state *p = I->us;
    img_alloc(I);
    switch (p->depth) {
      case 8:
	break;
      case 16:
	png_set_strip_16(p->png);
	break;
      default:
	png_set_packing(p->png);
	break;
    }
    switch (p->color) {
      case PNG_COLOR_TYPE_GRAY:
      case PNG_COLOR_TYPE_GRAY_ALPHA:
	png_set_gray_to_rgb(p->png);
	break;
      case PNG_COLOR_TYPE_PALETTE:
	png_set_palette_to_rgb(p->png);
	break;
      case PNG_COLOR_TYPE_RGB:
      case PNG_COLOR_TYPE_RGB_ALPHA:
	break;
      default:
	png_done(I);
	return 0;
    }
    if (!(p->color & PNG_COLOR_MASK_ALPHA))
	png_set_filler(p->png, 0xFF, PNG_FILLER_AFTER);
    png_read_update_info(p->png, p->info);
    png_read_image(p->png, (png_bytep *)I->data);
    png_done(I);
    return 1;
}

#endif /* !NO_DISPLAY_WINDOW */

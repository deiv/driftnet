/*
 * png.c:
 * PNG image support.
 *
 * Copyright (c) 2012 David Suárez.
 * Email: david.sephirot@gmail.com
 *
 * Copyright (c) 2003 Drew Roedersheimer, Chris Lightfoot.
 * Email: chris@ex-parrot.com; WWW: http://www.ex-parrot.com/~chris/
 *
 */

#ifdef HAVE_CONFIG_H
    #include <config.h>
#endif

#include "compat/compat.h"

#include <stdlib.h>
#include <stdio.h>                      /* for rewind, FILE */

#include <png.h>

#include "common/log.h"
#include "img.h"
#include "pngformat.h"

void png_catch_error(png_structp png_ptr, png_const_charp error_msg)
{
   log_msg(LOG_WARNING, "libpng, %s (skipping image).", error_msg);

   if (setjmp(png_jmpbuf(png_ptr))) {
	  log_msg(LOG_ERROR, "libpng, unrecoverable error, terminating.");
      abort();
   }
}

int png_load_hdr(img I) {
    unsigned char sig[PNG_SIG_LEN];
    png_structp png_ptr;
    png_infop info_ptr;

    rewind(I->fp);

    if (fread(sig, sizeof(sig[0]), PNG_SIG_LEN, I->fp) != PNG_SIG_LEN) {
        return(0);
    }

    /* Check the PNG signature of the file */
    if (png_sig_cmp(sig, (png_size_t)0, PNG_SIG_LEN)) {
        I->err = IE_HDRFORMAT;
        return 0;
    }

    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, 
        NULL, png_catch_error, NULL);

    if (png_ptr == NULL) {
        I->err = IE_HDRFORMAT;
        return 0;
    }
   
    if (setjmp(png_jmpbuf(png_ptr))) {
       png_destroy_read_struct(&png_ptr, (png_infopp)NULL, (png_infopp)NULL);
       I->err = IE_HDRFORMAT;
       return 0;
    }

    info_ptr = png_create_info_struct(png_ptr);

    if (info_ptr == NULL) {
        png_destroy_read_struct(&png_ptr, (png_infopp)NULL, (png_infopp)NULL);
        I->err = IE_HDRFORMAT;
        return 0;
    }

    rewind(I->fp);
    png_init_io(png_ptr, I->fp);

    png_read_info(png_ptr, info_ptr);

    I->width = png_get_image_width(png_ptr, info_ptr);
    I->height = png_get_image_height(png_ptr, info_ptr);

    png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);

    return(1);
}

int png_abort_load(img I) {
    return 1;
}

int png_load_img(img I) {
    unsigned char **p, **q;
    png_structp png_ptr;
    png_infop info_ptr;
    png_uint_32 width, height;
    int i, j, bit_depth, color_type, interlace_type;
    png_bytepp row_pointers;

    img_alloc(I);

    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
        NULL, png_catch_error, NULL);

    if (png_ptr == NULL) {
        I->err = IE_HDRFORMAT;
        return 0;
    }
    
    if (setjmp(png_jmpbuf(png_ptr))) {
       png_destroy_read_struct(&png_ptr, (png_infopp)NULL, (png_infopp)NULL);
       I->err = IE_HDRFORMAT;
       return 0;
    }

    info_ptr = png_create_info_struct(png_ptr);

    if (info_ptr == NULL) {
        png_destroy_read_struct(&png_ptr, (png_infopp)NULL, (png_infopp)NULL);
        I->err = IE_HDRFORMAT;
        return 0;
    }

    rewind(I->fp);
    png_init_io(png_ptr, I->fp);

    png_read_info(png_ptr, info_ptr);

    /* Get image specific data */
    png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type,
        &interlace_type, NULL, NULL);

    /* Convert greyscale images to 8-bit RGB */
    if (color_type == PNG_COLOR_TYPE_GRAY ||
        color_type == PNG_COLOR_TYPE_GRAY_ALPHA) {
        if (bit_depth < 8) {
            png_set_expand_gray_1_2_4_to_8(png_ptr);
        }
        png_set_gray_to_rgb(png_ptr);
    }

    /* Change paletted images to RGB */
    if (color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_expand(png_ptr);

    if (bit_depth < 8)
        png_set_expand(png_ptr);

    if (bit_depth == 16)
        png_set_strip_16(png_ptr);

    /* The gdk img widget appears to expect 8-bit RGB followed by a
     * filler byte. */
    png_set_filler(png_ptr, 0, PNG_FILLER_AFTER);

    /* Update the info structure after the transforms */
    png_read_update_info(png_ptr, info_ptr);
/*    png_set_rows(png_ptr, info_ptr, row_pointers)*/

    /* Allocate space before reading the image */
    row_pointers = png_malloc(png_ptr, height * sizeof(png_bytep));
    for (i = 0; i < height; i++) {
        row_pointers[i] = png_malloc(png_ptr, png_get_rowbytes(png_ptr, info_ptr));
    }

    /* Read in the image and copy it to the gdk img structure */
    png_read_image(png_ptr, row_pointers);

    p = (unsigned char **)I->data;
    q = (unsigned char **)row_pointers;

    for (i = 0; i < height; i++) {
        for (j = 0; j < png_get_rowbytes(png_ptr, info_ptr); j++) {
           p[i][j] = q[i][j];
        }
    }

    png_read_end(png_ptr, info_ptr);

    /* Clean up */
    png_free(png_ptr, row_pointers);
    png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);

    return 1;
}

int png_save_img(const img I, FILE *fp) {
    return 1;
}

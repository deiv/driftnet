/*
 * jpeg.c:
 * JPEG file support. Based on example.c in the IJG jpeg-6b distribution.
 *
 * Copyright (c) 2001 Chris Lightfoot. All rights reserved.
 * Email: chris@ex-parrot.com; WWW: http://www.ex-parrot.com/~chris/
 *
 */

#ifndef NO_DISPLAY_WINDOW

static const char rcsid[] = "$Id: jpeg.c,v 1.5 2002/07/08 20:57:17 chris Exp $";

#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <jpeglib.h>

#include "img.h"

/* struct my_error_mgr:
 * Error handling struct for JPEG library interaction. */
struct my_error_mgr {
    struct jpeg_error_mgr pub;
    jmp_buf jb;
};

/* my_error_exit:
 * Error handler method for JPEG library. */
static void my_error_exit(j_common_ptr cinfo) {
    struct my_error_mgr *e = (struct my_error_mgr*)cinfo->err;
    (*cinfo->err->output_message)(cinfo);
    longjmp(e->jb, 1);
}

/* jpeg_load_hdr:
 * Load the header of a JPEG file. */
int jpeg_load_hdr(img I) {
    struct jpeg_decompress_struct *cinfo;
    struct my_error_mgr *jerr;
    cinfo = (struct jpeg_decompress_struct*)calloc(sizeof(struct jpeg_decompress_struct), 1);
    I->us = cinfo;
    jerr = (struct my_error_mgr*)calloc(sizeof(struct my_error_mgr), 1);
    cinfo->err = jpeg_std_error(&jerr->pub);
    jerr->pub.error_exit = my_error_exit;
    if (setjmp(jerr->jb)) {
        /* Oops, something went wrong. */
        I->err = IE_HDRFORMAT;
        jpeg_destroy_decompress(cinfo);
        return 0;
    }

    jpeg_create_decompress(cinfo);
    jpeg_stdio_src(cinfo, I->fp);

    /* Read the header of the image. */
    jpeg_read_header(cinfo, TRUE);

    jpeg_start_decompress(cinfo);
    
    I->width = cinfo->output_width;
    I->height = cinfo->output_height;

    return 1;
}

/* jpeg_abort_load:
 * Abort loading a JPEG after the header is done. */
int jpeg_abort_load(img I) {
    jpeg_finish_decompress((struct jpeg_decompress_struct*)I->us);
    jpeg_destroy_decompress((struct jpeg_decompress_struct*)I->us);
    return 1;
}

/* jpeg_load_img:
 * Read a JPEG file into an image. */
int jpeg_load_img(img I) {
    struct jpeg_decompress_struct *cinfo = I->us;
    struct my_error_mgr *jerr;
    JSAMPARRAY buffer;
    img_alloc(I);
    jerr = (struct my_error_mgr*)cinfo->err;
    if (setjmp(jerr->jb)) {
        /* Oops, something went wrong. */
        I->err = IE_IMGFORMAT;
        jpeg_destroy_decompress(cinfo);
        return 0;
    }

    cinfo->out_color_space = JCS_RGB;
    cinfo->out_color_components = cinfo->output_components = 3;
    
    /* Start decompression. */
    buffer = cinfo->mem->alloc_sarray((j_common_ptr)cinfo, JPOOL_IMAGE, cinfo->output_width * cinfo->output_components, 1);

    while (cinfo->output_scanline < cinfo->output_height) {
        pel *p, *end;
        unsigned char *q;
        jpeg_read_scanlines(cinfo, buffer, 1);

        /* Now we have a buffer in RGB format. */
        for (p = I->data[cinfo->output_scanline - 1], end = p + I->width, q = (unsigned char*)buffer[0]; p < end; ++p, q += 3)
            *p = PEL(*q, *(q + 1), *(q + 2));
    }

    jpeg_finish_decompress(cinfo);
    jpeg_destroy_decompress(cinfo);

    return 1;
}

/* jpeg_save_img:
 * Write an image out into a JPEG file. */
int jpeg_save_img(const img I, FILE *fp) {
    struct jpeg_compress_struct cinfo;
    struct my_error_mgr jerr;
    JSAMPROW *buffer;

    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = my_error_exit;
    if (setjmp(jerr.jb)) {
        /* Oops, something went wrong. */
        I->err = IE_SYSERROR;
        jpeg_destroy_compress(&cinfo);
        return 0;
    }

    jpeg_create_compress(&cinfo);

    /* Compressor will write to fp. */
    jpeg_stdio_dest(&cinfo, fp);

    /* Image parameters. */
    cinfo.image_width = I->width;
    cinfo.image_height = I->height;
    cinfo.input_components = 3;
    cinfo.in_color_space = JCS_RGB;

    /* Default parameters. */
    jpeg_set_defaults(&cinfo);

    /* XXX compression quality? */

    jpeg_start_compress(&cinfo, TRUE);

    buffer = cinfo.mem->alloc_sarray((j_common_ptr)&cinfo, JPOOL_IMAGE, I->width * 3, 1);

    while (cinfo.next_scanline < cinfo.image_height) {
        pel *p, *end;
        unsigned char *q;
        /* Copy image data into correct format. */
        for (p = I->data[cinfo.next_scanline], end = p + I->width, q = (unsigned char*)buffer[0]; p < end; ++p) {
            *q++ = (unsigned char)GETR(*p);
            *q++ = (unsigned char)GETG(*p);
            *q++ = (unsigned char)GETB(*p);
        }
        
        /* Write scanline. */
        jpeg_write_scanlines(&cinfo, buffer, 1);
    }

    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);

    return 1;
}

#endif /* !NO_DISPLAY_WINDOW */

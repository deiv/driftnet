/*
 * raw.c:
 * Write raw images.
 *
 * Copyright (c) 2001 Chris Lightfoot. All rights reserved.
 * Email: chris@ex-parrot.com; WWW: http://www.ex-parrot.com/~chris/
 *
 */

static const char rcsid[] = "$Id: raw.c,v 1.1 2001/07/15 11:07:33 chris Exp $";

#include <stdio.h>

#include "img.h"

/* raw_load_img:
 * Load an image in raw (RGBA machine-word) format.
 */
int raw_load_img(img I) {
    img_alloc(I);
    if (fread(I->flat, I->width, I->height, I->fp) != I->height) {
        I->err = IE_SYSERROR;
        return 0;
    } else return 1;
}

/* raw_save_img:
 * Save an image in raw (RGBA machine-word) format.
 */
int raw_save_img(img I, FILE *fp) {
    if (fwrite(I->flat, I->width, I->height, I->fp) != I->height) {
        I->err = IE_SYSERROR;
        return 0;
    } else return 1;   
}

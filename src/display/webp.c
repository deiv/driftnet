/*
 * webp.c:
 * WebP file support.
 *
 * Copyright (c) 2022 David Suárez.
 * Email: david.sephirot@gmail.com
 *
 * Copyright (c) 2022 Martin.
 * Email: martin@sandsmark.ninja
 *
 */


#include <webp/decode.h>

#include <string.h>
#include <stdlib.h>

#include "img.h"

typedef struct {
    size_t size;
    unsigned char *data;
} webp_internal;

int webp_load_hdr(img I) {
    size_t actual;
    int width, height;
    webp_internal *internal;
    int ret;

    internal = (webp_internal*)malloc(sizeof(webp_internal));
    I->us = internal;

    // blah, need to read in entire file because for some reason driftnet stores things to disk
    fseek(I->fp, 0L, SEEK_END);
    internal->size = ftell(I->fp);
    rewind(I->fp);
    internal->data = malloc(internal->size);
    if (!internal->data) {
        return 0;
    }

    actual = fread(internal->data, internal->size, 1, I->fp);
    if (actual != internal->size) {
        // wtf;
        return 0;
    }

    // Validate header etc.
    ret = WebPGetInfo(internal->data, internal->size, &width, &height);
    if (!ret) {
        return 0;
    }
    I->width = width;
    I->height = height;

    return 1;
}

int webp_load_img(img I) {
    unsigned char *decoded;
    webp_internal *internal = (webp_internal*)I->us;
    size_t allocated_size;

    allocated_size = I->height * sizeof(pel*) + I->width * I->height * sizeof(pel); // copied from img.c, because why the fuck not
    decoded = WebPDecodeRGBInto(internal->data, internal->size,
            (unsigned char*)*I->data, allocated_size, I->width); // apparently we always use dumb stride

    if (!decoded) {
        return 0;
    }

    return 1;
}

int webp_abort_load(img I) {
    webp_internal *internal;

    if (!I->us) {
        return 1;
    }

    internal = (webp_internal*)I->us;


    free(internal->data);
    free(internal);
    I->us = NULL;

    return 1;
}

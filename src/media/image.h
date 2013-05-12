/*
 * image.h:
 * Attempt to find GIF/JPEG/PNG data embedded in buffers.
 *
 * Copyright (c) 2012 David Suárez. All rights reserved.
 * Email: david.sephirot@gmail.com
 *
 * Copyright (c) 2001 Chris Lightfoot. All rights reserved.
 * Email: chris@ex-parrot.com; WWW: http://www.ex-parrot.com/~chris/
 *
 */

#ifndef __IMAGE_H__
#define __IMAGE_H__

unsigned char *find_gif_image(const unsigned char *data, const size_t len,
        unsigned char **gifdata, size_t *giflen);

unsigned char *find_jpeg_image(const unsigned char *data, const size_t len,
        unsigned char **jpegdata, size_t *jpeglen);

unsigned char *find_png_image(const unsigned char *data, const size_t len,
        unsigned char **pngdata, size_t *pnglen);

#endif /* __IMAGE_H__ */

/*
 * image.c:
 * Attempt to find GIF/JPEG/PNG data embedded in buffers.
 *
 * Copyright (c) 2001 Chris Lightfoot.
 * Email: chris@ex-parrot.com; WWW: http://www.ex-parrot.com/~chris/
 *
 */

#ifdef HAVE_CONFIG_H
    #include <config.h>
#endif

#include "compat.h"

#include <stdio.h>
#include <stdlib.h> /* On many systems (Darwin...), stdio.h is a prerequisite. */
#include <string.h>

#include <netinet/in.h> /* ntohl */

#include "util.h"

#include "pngformat.h"

/* If we run out of space, put us back to the last candidate GIF header. */
/*#define spaceleft       do { if (block > data + len) { printf("ran out of space\n"); return gifhdr; } } while (0)*/
#define spaceleft       if (block >= data + len) return gifhdr /* > ?? */

unsigned char *find_gif_image(const unsigned char *data, const size_t len, unsigned char **gifdata, size_t *giflen) {
    unsigned char *gifhdr, *block;
    /*int gotimgblock = 0;*/
    int ncolours;

    *gifdata = NULL;

    if (len < 6) return (unsigned char*)data;

    gifhdr = memstr(data, len, (unsigned char*)"GIF89a", 6);
    if (!gifhdr) gifhdr = memstr(data, len, (unsigned char*)"GIF87a", 6);
    if (!gifhdr) return (unsigned char*)(data + len - 6);
    if (data + len - gifhdr < 14) return gifhdr; /* no space for header */

    ncolours = (1 << ((gifhdr[10] & 0x7) + 1));
    /* printf("gif header %d colours\n", ncolours); */
    block = gifhdr + 13;
    spaceleft;
    if (gifhdr[10] & 0x80) block += 3 * ncolours; /* global colour table */
    spaceleft;

    do {
         /* printf("gifhdr = %p block = %p off = %u %02x\n", gifhdr, block, block - gifhdr, (unsigned int)*block); */
        switch (*block) {
            case 0x2c:
                /* image block */
                /* printf("image data\n"); */
                if (block + 9 > data + len) return gifhdr;
                if (block[9] & 0x80) {
                    /* local colour table */
                    block += 3 * ((1 << ((gifhdr[9] & 0x7) + 1)));
                    spaceleft;
                }
                block += 10;
                ++block;        /* lzw code size */
                do {
                    spaceleft;
                    block += *block + 1;
                    spaceleft;
                } while (*block);
                ++block;
                spaceleft;
                /*gotimgblock = 1;*/

                break;

            case 0x21:
                /* extension */
                ++block;
                spaceleft;
                if (*block == 0xf9) {
                    /* graphic control */
                    /* printf("graphic control\n"); */
                    ++block;
                    spaceleft;
                    block += *block + 2;
                    spaceleft;
                    break;
                } else if (*block == 0xfe) {
                    /* comment */
                    /* printf("comment\n"); */
                    ++block;
                    do {
                        spaceleft;
                        block += *block + 1;
                        spaceleft;
                    } while (*block);
                    ++block;
                    spaceleft;
                } else if (*block == 0x01) {
                    /* text label */
                    /* printf("text label\n"); */
                    ++block;
                    spaceleft;
                    if (*block != 12) return gifhdr + 6;
                    block += 13;
                    do {
                        spaceleft;
                        block += *block + 1;
                        spaceleft;
                    } while (*block);
                    ++block;
                    spaceleft;
                } else if (*block == 0xff) {
                    /* printf("application extension\n"); */
                    ++block;
                    spaceleft;
                    if (*block != 11) return gifhdr + 6;
                    block += 12;
                    do {
                        spaceleft;
                        /* printf("app extension data %d bytes\n", (int)*block); */
                        block += *block + 1;
                        spaceleft;
                    } while (*block);
                    ++block;
                    spaceleft;
                } else {
                    /* printf("unknown extension block\n"); */
                    return gifhdr + 6;
                }
                break;

            case 0x3b:
                /* end of file block: we win. */
                /* printf("gif data from %p to %p\n", gifhdr, block); */
                *gifdata = gifhdr;
                *giflen = block - gifhdr + 1;
                return block + 1;
                break;

            default:
                /* printf("unknown block %02x\n", *block); */
                return gifhdr + 6;
        }
    } while (1);
}

/* If we run out of space, put us back to the last candidate JPEG header. */

#define jpegcount(c)    ((*(c) << 8) | *((c) + 1))

unsigned char *jpeg_next_marker(unsigned char *d, size_t len) {
    unsigned char *end = d + len;
    while (d < end && *d != 0xff) ++d;
    if (d == end) return NULL;
    while (d < end && *d == 0xff) ++d; /* skip 0xff padding */
    if (d == end) return NULL;

    return d;
}

unsigned char *jpeg_skip_block(unsigned char *d, size_t len) {
    int l;
    if (len < 2) return NULL;
    l = jpegcount(d);
    if (l > len) return NULL;

    return d + l;
}

unsigned char *find_jpeg_image(const unsigned char *data, const size_t len, unsigned char **jpegdata, size_t *jpeglen) {
    unsigned char *jpeghdr, *block;

    *jpegdata = NULL;

    jpeghdr = memstr(data, len, (unsigned char*)"\xff\xd8", 2); /* JPEG SOI marker */
    if (!jpeghdr) return (unsigned char*)(data + len - 1);

    /* printf("SOI marker at %p\n", jpeghdr); */

    if (jpeghdr + 2 > data + len) return jpeghdr;
    block = jpeg_next_marker(jpeghdr + 2, len - 2 - (jpeghdr - data));
    /* printf("next block at %p\n", block); */
    if (!block || (block - data) >= len) return jpeghdr;

    /* now we need to find the onward count from this place */
    while ((block = jpeg_skip_block(block + 1, len - (block + 1 - data)))) {
        /* printf("data = %p block = %p\n", data, block); */

        block = jpeg_next_marker(block, len - (block - data));
        if (!block || (block - data) >= len) return jpeghdr;

        /* printf("got block of type %02x\n", *block); */

        if (*block == 0xda) {
            /* start of scan; dunno how to parse this but just look for end of
             * image marker. XXX this is broken, fix it! */
            block = memstr(block, len - (block - data), (unsigned char*)"\xff\xd9", 2);
            if (block) {
                *jpegdata = jpeghdr;
                *jpeglen = block + 2 - jpeghdr;
                return block + 2;
            } else break;
        }
    }
    /* printf("nope, no complete JPEG here\n"); */
    return jpeghdr;
}

/* find_png_eoi BUFFER LEN
 * Returns the first position in BUFFER of LEN bytes after the end of the image
 * or NULL if end of image not found. */
unsigned char *find_png_eoi(unsigned char *buffer, const size_t len) {
    unsigned char *end_data, *data, chunk_code[PNG_CODE_LEN + 1];
    struct png_chunk chunk;
    u_int32_t datalen;

    /* Move past the PNG header */
    data = (buffer + PNG_SIG_LEN);
    end_data = (buffer + len - (sizeof(struct png_chunk) + PNG_CRC_LEN));

    while (data <= end_data) {
        memcpy(&chunk, data, sizeof chunk);
/*        chunk = (struct png_chunk *)data; */ /* can't do that. */
        memset(chunk_code, '\0', PNG_CODE_LEN + 1);
        memcpy(chunk_code, chunk.code, PNG_CODE_LEN);

        datalen = ntohl(chunk.datalen);

        if (!strncasecmp((char*)chunk_code, "iend", PNG_CODE_LEN))
            return (unsigned char *)(data + sizeof(struct png_chunk) + PNG_CRC_LEN);

        /* Would this push us off the end of the buffer? */
        if (datalen > (len - (data - buffer)))
            return NULL;

        data += (sizeof(struct png_chunk) + datalen + PNG_CRC_LEN);
    }

    return NULL;
}

/* find_png_image DATA LEN PNGDATA PNGLEN
 * Look for PNG images in LEN bytes buffer DATA. */
unsigned char *find_png_image(const unsigned char *data, const size_t len, unsigned char **pngdata, size_t *pnglen) {
    unsigned char *pnghdr, *data_end, *png_eoi;

    *pngdata = NULL;

    if (len < PNG_SIG_LEN)
       return (unsigned char*)data;

    pnghdr = memstr(data, len, (unsigned char*)"\x89\x50\x4e\x47\x0d\x0a\x1a\x0a", PNG_SIG_LEN);
    if (!pnghdr)
        return (unsigned char*)(data + len - PNG_SIG_LEN);

    data_end = (unsigned char *)(data + len);

    if ((png_eoi = find_png_eoi(pnghdr, (data_end - pnghdr))) == NULL)
        return pnghdr;

    *pngdata = pnghdr;
    *pnglen = (png_eoi - pnghdr);
    return png_eoi;
}


#if 0
#include <unistd.h>
#include <fcntl.h>

int main(int argc, char **argv) {
    unsigned char buf[262144];
    char **a;
    for (a = argv + 1; *a; ++a) {
        unsigned char *p, *img;
        size_t len;
        int fd = open(*a, O_RDONLY);
        read(fd, buf + rand() % 256, 261000);
        /* printf("jpeg file %s\n", *a); */
        p = buf;
        do {
            /* printf("--> now p = %p\n", p); */
            p = find_jpeg_image(p, 262144 - (p - buf), &img, &len);
            if (img) /* printf("   found image %p len %u\n", img, len); */
        } while (p);
    }
}
#endif

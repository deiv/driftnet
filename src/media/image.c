/*
 * image.c:
 * Attempt to find GIF/JPEG/PNG data embedded in buffers.
 *
 * Copyright (c) 2001 Chris Lightfoot.
 * Email: chris@ex-parrot.com; WWW: http://www.ex-parrot.com/~chris/
 *
 */

#include "compat/compat.h"

#include <stdio.h>
#include <stdlib.h> /* On many systems (Darwin...), stdio.h is a prerequisite. */
#include <string.h>

#include <netinet/in.h> /* ntohl */

#include "common/util.h"

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

enum jpeg_marker {
    SOI = 0xD8,
    DHT = 0xC4,
    DAC = 0xCC,
    DQT = 0xDB,
    DRI = 0xDD,
    DHP = 0xDE,
    EXP = 0xDF,
    SOS = 0xDA,
    COM = 0xFE,
    EOI = 0xD9,

    RST0 = 0xD0,
    RST1 = 0xD1,
    RST2 = 0xD2,
    RST3 = 0xD3,
    RST4 = 0xD4,
    RST5 = 0xD5,
    RST6 = 0xD6,
    RST7 = 0xD7,

    SOF0 = 0xC0, SOF1 = 0xC1, SOF2 = 0xC2, SOF3 = 0xC3, SOF5 = 0xC5, SOF6 = 0xC6, SOF7 = 0xC7,
    SOF8 = 0xC8, SOF9 = 0xC9, SOFA = 0xCA, SOFB = 0xCB, SOFD = 0xCD, SOFE = 0xCE, SOFF = 0xCF,

    APP0 = 0xE0, APP1 = 0xE1, APP2 = 0xE2, APP3 = 0xE3, APP4 = 0xE4, APP5 = 0xE5, APP6 = 0xE6, APP7 = 0xE7,
    APP8 = 0xE8, APP9 = 0xE9, APPA = 0xEA, APPB = 0xEB, APPC = 0xEC, APPD = 0xED, APPE = 0xEE, APPF = 0xEF,

    JPG0 = 0xF0, JPG1 = 0xF1, JPG2 = 0xF2, JPG3 = 0xF3, JPG4 = 0xF4, JPG5 = 0xF5, JPG6 = 0xF6,
    JPG7 = 0xF7, JPG8 = 0xF8, JPG9 = 0xF9, JPGA = 0xFA, JPGB = 0xFB, JPGC = 0xFC, JPGD = 0xFD
};

typedef struct {
    unsigned char start_of_marker;     /* always 0xFF */
    unsigned char marker_type;
    unsigned char length[2];
} jpg_segment_t;

/* If we run out of space, put us back to the last candidate JPEG header. */
#define jpegcount(c)    ((*(c) << 8) | *((c) + 1))

unsigned int is_jpeg_segment(jpg_segment_t* segment)
{
    if (segment->start_of_marker != 0xFF) {
        return 0;
    }

    /*
     * Check for markers
     */
    switch (segment->marker_type) {

        /* without payload */
        case SOI:
        case EOI:
        case RST0:
        case RST1:
        case RST2:
        case RST3:
        case RST4:
        case RST5:
        case RST6:
        case RST7:
        case DHP:
        case EXP:
        case DAC:
            return 4;

        /* constant payload */
        case DRI:
            return 4 + 4;

        /* variable payload */
        case DHT:
        case DQT:
        case SOS:
        case COM:

        case SOF0:case SOF1:case SOF2:case SOF3:case SOF5:case SOF6:case SOF7:
        case SOF8:case SOF9:case SOFA:case SOFB:case SOFD:case SOFE:case SOFF:

        case APP0:case APP1:case APP2:case APP3:case APP4:case APP5:case APP6:case APP7:
        case APP8:case APP9:case APPA:case APPB:case APPC:case APPD:case APPE:case APPF:

        case JPG0:case JPG1:case JPG2:case JPG3:case JPG4:case JPG5:case JPG6:
        case JPG7:case JPG8:case JPG9:case JPGA:case JPGB:case JPGC:case JPGD:

            return (((segment->length[0] << 8) & 0xFF00) | segment->length[1]) + 2;
    }

    return 0;
}

unsigned char *find_jpeg_image(const unsigned char *data, const size_t len, unsigned char **jpegdata, size_t *jpeglen)
{
    unsigned char *jpeghdr;

    *jpegdata = NULL;
    *jpeglen = 0;

    if (data == NULL) {
        return NULL;
    }

    /* find SOI marker */
    jpeghdr = memstr(data, len, (unsigned char*)"\xff\xd8", 2);
    if (!jpeghdr) return (unsigned char*)(data + len - 1);

    jpg_segment_t* segment;
    unsigned int segment_lenght = 2;
    unsigned char *block = jpeghdr;

    do {
        /* more data to advance ? */
        if (data + len < block + segment_lenght) {
            break;
        }

        /* advance to next block */
        block = block + segment_lenght;
        segment = (jpg_segment_t*) block;
        segment_lenght = is_jpeg_segment(segment);

        /*
         * start of scan
         *
         * XXX: dunno how to parse...
         */
        if (segment->marker_type == SOS) {
            block = memstr(block, len - (block - data), (unsigned char*)"\xff\xd9", 2);

            if (block) {
                *jpegdata = jpeghdr;
                *jpeglen = block + 2 - jpeghdr;
                return block + 2;
            } else break;
        }

    } while (segment_lenght != 0);

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

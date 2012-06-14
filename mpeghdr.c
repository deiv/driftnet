/*
 * mpeghdr.c:
 * Parse MPEG audio headers.
 *
 * Copyright (c) 2002 Chris Lightfoot. All rights reserved.
 * Email: chris@ex-parrot.com; WWW: http://www.ex-parrot.com/~chris/
 *
 */

static const char rcsid[] = "$Id: mpeghdr.c,v 1.4 2002/06/10 21:25:48 chris Exp $";

#include <stdio.h>

#include "mpeghdr.h"

/* bits:
 * Get bits a through b < a of val. */
//#define bits(v, a, b)   (((v) & ((4 << ((a) - (b))) - 1)) >> (b))

#define bits(v, a, b)       (((v) & (((2 << ((a) - (b))) - 1) << (b))) >> (b))

/* bitrate_tbl:
 * Table of bit rates. Column index: 0 = v1 l1; 1 = v1 l2; 2 = v1 l3;
 * 3 = v2 l1; 4 = v2 l2, l3. */
static int bitrate_tbl[16][5] = {
        {  0,   0,   0,   0,   0},  /* free */
        { 32,  32,  32,  32,   8},
        { 64,  48,  40,  48,  16},
        { 96,  56,  48,  56,  24},
        {128,  64,  56,  64,  32},
        {160,  80,  64,  80,  40},
        {192,  96,  80,  96,  48},
        {224, 112,  96, 112,  56},
        {256, 128, 112, 128,  64},
        {288, 160, 128, 144,  80},
        {320, 192, 160, 160,  96},
        {352, 224, 192, 176, 112},
        {384, 256, 224, 192, 128},
        {416, 320, 256, 224, 144},
        {448, 384, 320, 256, 160},
        { -1,  -1,  -1,  -1,  -1}   /* bad */
    };

/* samplerate_tbl:
 * Table of sample rates. Column index: v1 = 0, v2 = 1, v2.5 = 2. */
static int samplerate_tbl[4][3] = {
        {44100, 22050, 11025},
        {48000, 24000, 12000},
        {32000, 16000,  8000},
        {   -1,    -1,    -1}       /* reserved */
    };

/* mpeg_hdr_parse:
 * Parse an MPEG header into a struct mpeg_audio_hdr. */
int mpeg_hdr_parse(const uint8_t *data, struct mpeg_audio_hdr *h) {
    struct mpeg_audio_hdr hz = {0};
    uint32_t hh;
    int i;

    *h = hz;
    
    if (data[0] != 0xff || (data[1] & 0xe0) != 0xe0)
        return 0;

    hh = (data[1] << 16) | (data[2] << 8) | data[3];

/*fprintf(stderr, "%08x\n", hh);*/
    
    /* Version. */
    switch (bits(hh, 20, 19)) {
        case 0: h->version = m_vers_2_5;    break;
        case 2: h->version = m_vers_2;      break;
        case 3: h->version = m_vers_1;      break;
        
        default:    return 0;
    }

    /* Layer. */
    switch (bits(hh, 18, 17)) {
        case 1: h->layer = m_layer_3;       break;
        case 2: h->layer = m_layer_2;       break;
        case 3: h->layer = m_layer_1;       break;

        default:    return 0;
    }

    /* Possible CRC. */
    h->has_crc = bits(hh, 16, 16);
    if (h->has_crc)
        h->crc = (data[5] << 8) | (data[6]);
    
    /* Bit rate. */
    i = (h->version == m_vers_1) ? 0 : 3;
    i += h->layer - 1;
    if (i == 5) i = 4;
    h->bitrate = bitrate_tbl[bits(hh, 15, 12)][i];
    if (h->bitrate == -1)
        return 0;

    /* Sample rate. */
    i = h->version - 1;
    h->samplerate = samplerate_tbl[bits(hh, 11, 10)][i];
    if (h->samplerate == - 1)
        return 0;

    /* Is frame padded? */
    h->padded = bits(hh, 9, 9);

    /* Channel mode. */
    h->channels = bits(hh, 7, 6);

    /* Mode extension. */
    h->mode_extn = bits(hh, 5, 4);

    h->copyr = bits(hh, 3, 3);
    h->original = bits(hh, 2, 2);
    h->emph = bits(hh, 1, 0);

    return 1;
}

/* mpeg_hdr_nextframe_offset:
 * Return the offset of the next MPEG header frame.
 * XXX this doesn't work with (at least) L3 mono frames-- need to fix it. */
int mpeg_hdr_nextframe_offset(const struct mpeg_audio_hdr *h) {
    int off = 0;
/*    if (h->has_crc) off += 2; */
    if (h->layer == m_layer_1)
        off += ((12 * h->bitrate * 1000) / h->samplerate + h->padded) * 4;
    else
        off += (144 * h->bitrate * 1000) / h->samplerate + h->padded;
    return off;
}

/* mpeg_hdr_print:
 * Print out useful information about an MPEG audio header. */
static char *vers_tbl[] = {"unknown", "1", "2", "2.5"};
static char *channel_tbl[] = {"stereo", "joint stereo", "dual stereo", "mono"};

void mpeg_hdr_print(FILE *fp, const struct mpeg_audio_hdr *h) {
    fprintf(fp, "version %s layer %d\n", vers_tbl[h->version], h->layer);
    if (h->has_crc)
        fprintf(fp, "  has CRC %04x\n", h->crc);
    fprintf(fp, "bit rate %d kbps\nsample rate %dHz\n", h->bitrate, h->samplerate);
    fprintf(fp, "%s padded\n", h->padded ? "is" : "is not");
    fprintf(fp, "%s\n", channel_tbl[h->channels]);
    if (h->channels == m_chan_joint)
        fprintf(fp, "mode extension %d\n", h->mode_extn);
    fprintf(fp, "%s copyright\n%s original\n", h->copyr ? "is" : "is not", h->original ? "is" : "is not");
    if (h->emph)
        fprintf(fp, "emphasis %d\n", h->emph);
}

#if 0
int main(int argc, char *argv[]) {
    char buf[8192], *p;
    struct mpeg_audio_hdr h;
    read(0, buf, sizeof buf);
    for (p = buf; p < buf + sizeof buf; ) {
        if (!mpeg_hdr_parse(p, &h)) {
            fprintf(stderr, "no MPEG audio header\n");
            return -1;
        }
        mpeg_hdr_print(stdout, &h);
        printf("offset to next frame = %d\n", mpeg_hdr_nextframe_offset(&h));
        p += mpeg_hdr_nextframe_offset(&h);
    }
}
#endif

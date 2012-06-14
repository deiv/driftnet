/*
 * audio.c:
 * Attempt to extract audio frames embedded in buffers.
 *
 * We only try MPEG at the moment. Ogg Vorbis is a bit harder because we'd need
 * to find the stream-start packet as well-- in practice, we probably would be
 * able to, but it's a bit of a hassle.
 *
 * Copyright (c) 2002 Chris Lightfoot. All rights reserved.
 * Email: chris@ex-parrot.com; WWW: http://www.ex-parrot.com/~chris/
 *
 */

static const char rcsid[] = "$Id: audio.c,v 1.3 2002/06/10 21:25:48 chris Exp $";

#include <string.h>

#include "driftnet.h"
#include "mpeghdr.h"

/* The minimum chunk of MPEG data, in frames, which we accept. MPEG layer 2/3
 * frames have 1152 samples, so there are something like 40 frames/s at
 * 44.1kHz. If there are several MPEG streams available, driftnet may chop
 * between them in chunks of this size. */
#define MIN_MPEG_EXTENT     100

/* find_mpeg_stream:
 * Try to find some MPEG data in a stream. The game here is that we look for
 * an MPEG audio header and see whether it's followed by a bunch more MPEG
 * audio headers. If there's as much as MIN_MPEG_EXTEND data, then we give
 * it back to the application and move our pointer on. */
unsigned char *find_mpeg_stream(const unsigned char *data, const size_t len, unsigned char **mpegdata, size_t *mpeglen) {
    unsigned char *stream_start, *p;
    struct mpeg_audio_hdr H;
    *mpegdata = NULL;

    if (len < 4) return (unsigned char*)data;
/*printf("find_mpeg_stream\n"); */
    p = (unsigned char*)data;
    while (p < data + len - 4) {
        int nframes;
        unsigned char *q;

        /* Look for something which might be a frame header. */
        stream_start = memchr(p, 0xff, len - 4 - (p - data));
        if (!stream_start)
            return (unsigned char*)(data + len - 4);

        if ((stream_start[1] & 0xe0) != 0xe0) {
/*printf(" not followed by e0\n");*/
            p = stream_start + 1;
            continue;
        }

        /* OK, found something which might be a header.... */
        if (!mpeg_hdr_parse(stream_start, &H)) {
            p = stream_start + 1;
            continue;
        }

        /* See how many frames we get. */
        nframes = 0;
        q = stream_start;
        do {
            int delta;
            ++nframes;
            delta = mpeg_hdr_nextframe_offset(&H);
            if (delta == 0)
                return q + 1;
            q += delta;
        } while (nframes < MIN_MPEG_EXTENT && q < data + len - 4 && mpeg_hdr_parse(q, &H));

        if (nframes >= MIN_MPEG_EXTENT) {
            /* got some data. */
/*            printf("stream_start = %p, q = %p, len = %d\n", stream_start, q, q - stream_start);*/
            *mpegdata = stream_start;
            *mpeglen = q - stream_start;
            return q;
        } else
            return stream_start;
    }

    return p;
}

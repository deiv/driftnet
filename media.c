/*
 * media.c:
 * Extract various media types from files.
 *
 * Copyright (c) 2002 Chris Lightfoot. All rights reserved.
 * Email: chris@ex-parrot.com; WWW: http://www.ex-parrot.com/~chris/
 *
 */

static const char rcsid[] = "$Id: media.c,v 1.9 2003/08/25 12:23:43 chris Exp $";

#include <assert.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h> /* On many systems (Darwin...), stdio.h is a prerequisite. */
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "options.h"
#include "tmpdir.h"
#include "driftnet.h"

extern int adjunct;
extern int dpychld_fd;

/* image.c */
unsigned char *find_gif_image(const unsigned char *data, const size_t len, unsigned char **gifdata, size_t *giflen);
unsigned char *find_jpeg_image(const unsigned char *data, const size_t len, unsigned char **jpegdata, size_t *jpeglen);
unsigned char *find_png_image(const unsigned char *data, const size_t len, unsigned char **pngdata, size_t *pnglen);

/* audio.c */
unsigned char *find_mpeg_stream(const unsigned char *data, const size_t len, unsigned char **mpegdata, size_t *mpeglen);

/* http.c */
unsigned char *find_http_req(const unsigned char *data, const size_t len, unsigned char **http, size_t *httplen);
void dispatch_http_req(const char *mname, const unsigned char *data, const size_t len);

/* playaudio.c */
void mpeg_submit_chunk(const unsigned char *data, const size_t len);

/* dispatch_image:
 * Throw some image data at the display process. */
void dispatch_image(const char *mname, const unsigned char *data, const size_t len) {
    char *buf, name[TMPNAMELEN] = {0};
    int fd;
    buf = xmalloc(strlen(get_tmpdir()) + 64);
    sprintf(name, "driftnet-%08x%08x.%s", (unsigned int)time(NULL), rand(), mname);
    sprintf(buf, "%s/%s", get_tmpdir(), name);
    fd = open(buf, O_WRONLY | O_CREAT | O_EXCL, 0644);
    if (fd == -1)
        return;
    write(fd, data, len);
    close(fd);

    /* XXX: remove get_options()->adjunct access */
    if (get_options()->adjunct)
        printf("%s\n", buf);
#ifndef NO_DISPLAY_WINDOW
    else
        write(dpychld_fd, name, sizeof name);
#endif /* !NO_DISPLAY_WINDOW */

    xfree(buf);
}

/* dispatch_mpeg_audio:
 * Throw some MPEG audio into the player process or temporary directory as
 * appropriate. */
void dispatch_mpeg_audio(const char *mname, const unsigned char *data, const size_t len) {
    mpeg_submit_chunk(data, len);
}

/* Media types we handle. */
static struct mediadrv {
    char *name;
    enum mediatype type;
    unsigned char *(*find_data)(const unsigned char *data, const size_t len, unsigned char **found, size_t *foundlen);
    void (*dispatch_data)(const char *mname, const unsigned char *data, const size_t len);
} driver[NMEDIATYPES] = {
        { "gif",  m_image, find_gif_image,   dispatch_image },
        { "jpeg", m_image, find_jpeg_image,  dispatch_image },
        { "png",  m_image, find_png_image,   dispatch_image },
        { "mpeg", m_audio, find_mpeg_stream, dispatch_mpeg_audio },
        { "HTTP", m_text,  find_http_req,    dispatch_http_req }
    };

/* connection_extract_media CONNECTION TYPE
 * Attempt to extract media data of the given TYPE from CONNECTION. */
void connection_extract_media(connection c, const enum mediatype T) {
    struct datablock *b;

    /* Walk through the list of blocks and try to extract media data from
     * those which have changed. */
    for (b = c->blocks; b; b = b->next) {
        if (b->len > 0 && b->dirty) {
            int i;
            for (i = 0; i < NMEDIATYPES; ++i)
                if (driver[i].type & T) {
                    unsigned char *ptr, *oldptr, *media;
                    size_t mlen;

                    ptr = c->data + b->off + b->moff[i];
                    oldptr = NULL;

                    while (ptr != oldptr && ptr < c->data + b->off + b->len) {
                        oldptr = ptr;
                        ptr = driver[i].find_data(ptr, (b->off + b->len) - (ptr - c->data), &media, &mlen);
                        if (media && !tmpfiles_limit_reached())
                            driver[i].dispatch_data(driver[i].name, media, mlen);
                    }

                    b->moff[i] = ptr - c->data - b->off;
                }
            b->dirty = 0;
        }
    }
}

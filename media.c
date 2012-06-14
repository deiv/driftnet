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
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "driftnet.h"

extern char *tmpdir;    /* in driftnet.c */
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

/* count_temporary_files:
 * How many of our files remain in the temporary directory? We do this a
 * maximum of once every five seconds. */
static int count_temporary_files(void) {
    static int num;
    static time_t last_counted;
    if (last_counted < time(NULL) - 5) {
        DIR *d;
        struct dirent *de;
        num = 0;
        d = opendir(tmpdir);
        if (d) {
            while ((de = readdir(d))) {
                char *p;
                p = strrchr(de->d_name, '.');
                if (p && (strncmp(de->d_name, "driftnet-", 9) == 0 && (strcmp(p, ".jpeg") == 0 || strcmp(p, ".gif") == 0 || strcmp(p, ".png") == 0 || strcmp(p, ".mp3") == 0)))
                    ++num;
            }
            closedir(d);
            last_counted = time(NULL);
        }
    }
    return num;
}

/* dispatch_image:
 * Throw some image data at the display process. */
void dispatch_image(const char *mname, const unsigned char *data, const size_t len) {
    char *buf, name[TMPNAMELEN] = {0};
    int fd;
    buf = xmalloc(strlen(tmpdir) + 64);
    sprintf(name, "driftnet-%08x%08x.%s", (unsigned int)time(NULL), rand(), mname);
    sprintf(buf, "%s/%s", tmpdir, name);
    fd = open(buf, O_WRONLY | O_CREAT | O_EXCL, 0644);
    if (fd == -1)
        return;
    write(fd, data, len);
    close(fd);

    if (adjunct)
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
    extern int max_tmpfiles;  /* in driftnet.c */

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
                        if (media && (!max_tmpfiles || count_temporary_files() < max_tmpfiles))
                            driver[i].dispatch_data(driver[i].name, media, mlen);
                    }

                    b->moff[i] = ptr - c->data - b->off;
                }
            b->dirty = 0;
        }
    }
}

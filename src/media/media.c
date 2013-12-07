/*
 * media.c:
 * Extract various media types from files.
 *
 * Copyright (c) 2012 David Suárez.
 * Email: david.sephirot@gmail.com
 *
 * Copyright (c) 2002 Chris Lightfoot.
 * Email: chris@ex-parrot.com; WWW: http://www.ex-parrot.com/~chris/
 *
 */

#ifdef HAVE_CONFIG_H
    #include <config.h>
#endif

#include "compat.h"

#include <string.h>

#include "util.h"
#include "tmpdir.h"
#ifndef NO_DISPLAY_WINDOW
#include "display.h"
#endif
#include "image.h"
#include "audio.h"
#include "http.h"
#include "playaudio.h"

#include "media.h"

static mediatype_t extract_type;
static int play_media;

/*
 * dispatch_image:
 * Throw some image data at the display process.
 */
void dispatch_image(const char *mname, const unsigned char *data, const size_t len)
{
    const char *name;

    name = tmpfile_write(mname, data, len);
    if (name == NULL)
        return;

    if (!play_media)
        printf("%s/%s\n", get_tmpdir(), name);
#ifndef NO_DISPLAY_WINDOW
    else
        display_send_img(name, TMPNAMELEN);
#endif /* !NO_DISPLAY_WINDOW */

}

/*
 * dispatch_mpeg_audio:
 * Throw some MPEG audio into the player process or temporary directory as
 * appropriate.
 */
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


void init_mediadrv(mediatype_t media_type, int play)
{
    extract_type = media_type;
    play_media = play;
}

/* connection_extract_media CONNECTION TYPE
 * Attempt to extract media data of the given TYPE from CONNECTION. */
void extract_media(connection c)
{
    struct datablock *b;

    /* Walk through the list of blocks and try to extract media data from
     * those which have changed. */
    for (b = c->blocks; b; b = b->next) {
        if (b->len > 0 && b->dirty) {
            int i;
            for (i = 0; i < NMEDIATYPES; ++i)
                if (driver[i].type & extract_type) {
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

/**
 * @file media.c
 *
 * @brief Media data handling.
 * @author David Suárez
 * @author Chris Lightfoot
 * @date Sun, 28 Oct 2018 16:14:56 +0100
 *
 * Copyright (c) 2002 Chris Lightfoot.
 * Email: chris@ex-parrot.com; WWW: http://www.ex-parrot.com/~chris/
 *
 * Copyright (c) 2018 David Suárez.
 * Email: david.sephirot@gmail.com
 *
 */

#include "compat/compat.h"

#include <string.h>

#include "common/util.h"
#include "common/tmpdir.h"
#include "image.h"
#include "audio.h"
#include "http.h"
#include "playaudio.h"

#include "media.h"

static mediadrv_t media_drivers[NMEDIATYPES] = {
    { "gif",  MEDIATYPE_IMAGE, find_gif_image },
    { "jpeg", MEDIATYPE_IMAGE, find_jpeg_image },
    { "png",  MEDIATYPE_IMAGE, find_png_image },
    { "mpeg", MEDIATYPE_AUDIO, find_mpeg_stream },
    { "HTTP", MEDIATYPE_TEXT,  find_http_req }
};


drivers_t* get_drivers_for_mediatype(mediatype_t type)
{
    drivers_t* drivers = NULL;
    int driver_count = 0;
    int current_drv = 0;

    for (int i = 0; i < NMEDIATYPES; ++i) {
        if (media_drivers[i].type & type) {
            driver_count++;
        }
    }

    drivers = xmalloc(sizeof(drivers_t));

    drivers->type = type;
    drivers->count = driver_count;
    drivers->list = xmalloc(sizeof(mediadrv_t*) * driver_count);

    for (int i = 0; i < NMEDIATYPES; ++i) {
        if (media_drivers[i].type & type) {
            drivers->list[current_drv] = &media_drivers[i];
            current_drv++;
        }
    }

    return drivers;
}

void close_media_drivers(drivers_t* drivers)
{
    if (drivers == NULL) {
        return;
    }

    xfree(drivers->list);
    xfree(drivers);
}

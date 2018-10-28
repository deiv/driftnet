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

static mediadrv_t drivers[NMEDIATYPES] = {
    { "gif",  MEDIATYPE_IMAGE, find_gif_image },
    { "jpeg", MEDIATYPE_IMAGE, find_jpeg_image },
    { "png",  MEDIATYPE_IMAGE, find_png_image },
    { "mpeg", MEDIATYPE_AUDIO, find_mpeg_stream },
    { "HTTP", MEDIATYPE_TEXT,  find_http_req }
};

/*
 * TODO: improve the return type
 */
mediadrv_t** get_drivers_for_mediatype(mediatype_t type)
{
    static mediadrv_t* drivers_o[NMEDIATYPES];
    int current_drv = 0;

    for (int i = 0; i < NMEDIATYPES; ++i) {
        if (drivers[i].type & type) {
            drivers_o[current_drv] = &drivers[i];
            current_drv++;
        }
    }

    if (current_drv == 0) {
        return NULL;
    }

    for (int i = current_drv; i < NMEDIATYPES; i++) {
        drivers_o[i] = NULL;
    }

    return drivers_o;
}

/*
 * media.h:
 * Media data handling.
 *
 * Copyright (c) 2012 David Suárez.
 * Email: david.sephirot@gmail.com
 *
 * Copyright (c) 2002 Chris Lightfoot.
 * Email: chris@ex-parrot.com; WWW: http://www.ex-parrot.com/~chris/
 *
 */

#ifndef __MEDIA_H__
#define __MEDIA_H__

#ifdef HAVE_CONFIG_H
    #include <config.h>
#endif

/* define before connection.h include: circular dependency */
#define NMEDIATYPES 5
//struct connection;

#include "network/connection.h"

/**
 * @brief Bit field to characterise types of media which we can extract.
 */
typedef enum mediatype {
    MEDIATYPE_IMAGE = 1,
    MEDIATYPE_AUDIO = 1 << 1,
    MEDIATYPE_TEXT  = 1 << 2
} mediatype_t;

void init_mediadrv(mediatype_t media_type, int play, int enable_ws, int enable_gtk);
void extract_media(connection c);

#endif /* __MEDIA_H__ */

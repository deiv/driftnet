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
struct connection;

#include "connection.h"

/*
 * enum mediatype:
 * Bit field to characterise types of media which we can extract.
 */
typedef enum mediatype { m_image = 1, m_audio = 2, m_text = 4 } mediatype_t;

void init_mediadrv(mediatype_t media_type, int play);
void extract_media(connection c);

#endif /* __MEDIA_H__ */

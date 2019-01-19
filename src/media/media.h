/**
 * @file media.h
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

#ifndef __MEDIA_H__
#define __MEDIA_H__

#ifdef HAVE_CONFIG_H
    #include <config.h>
#endif

#include <stddef.h>

/**
 * @brief Number of media types we recognize.
 */
#define NMEDIATYPES 5

/* TODO: NMEDIATYPES -> NMEDIA_DRIVERS */

/**
 * @brief Bit field to characterise types of media which we can extract.
 */
typedef enum mediatype {
    MEDIATYPE_IMAGE = 1,
    MEDIATYPE_AUDIO = 1 << 1,
    MEDIATYPE_TEXT  = 1 << 2
} mediatype_t;

/**
 * @brief Info for each media driver.
 */
typedef struct mediadrv {
    /** Media name: gif, jpeg ... */
    char *name;

    /** Type of media @see mediatype_t */
    enum mediatype type;

    /** Function to find data for the media the driver knows about */
    unsigned char *(*find_data)(const unsigned char *data, const size_t len, unsigned char **found, size_t *foundlen);

    /** Pointer to function to dispatch this type of media; this should be initialized by the user */
    void (*dispatch_data)(const char *mname, const unsigned char *data, const size_t len);
} mediadrv_t;

/* TODO: ... */
typedef struct drivers {
    mediatype_t type;
    mediadrv_t** list;
    int count;
} drivers_t;

/**
 * @brief Obtains a list of media drivers.
 *
 * @param filter to this media type
 * @return drivers list (don't free it)
 */
mediadrv_t** get_drivers_for_mediatype(mediatype_t type);

#endif /* __MEDIA_H__ */

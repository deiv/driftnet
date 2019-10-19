/**
 * @file http.h
 *
 * @brief Look for HTTP requests in buffers.
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

#ifndef __HTTP_H__
#define __HTTP_H__

#ifdef HAVE_CONFIG_H
    #include <config.h>
#endif

#include <stddef.h>

unsigned char *find_http_req(const unsigned char *data, const size_t len,
                             unsigned char **http, size_t *httplen);

#endif /* __HTTP_H__ */

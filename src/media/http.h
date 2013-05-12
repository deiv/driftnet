 /*
 * http.h:
 * Look for HTTP requests in buffers.
 *
 * Copyright (c) 2012 David Suárez. All rights reserved.
 * Email: david.sephirot@gmail.com
 *
 * Copyright (c) 2002 Chris Lightfoot. All rights reserved.
 * Email: chris@ex-parrot.com; WWW: http://www.ex-parrot.com/~chris/
 *
 */

#ifndef __HTTP_H__
#define __HTTP_H__

#ifdef HAVE_CONFIG_H
    #include <config.h>
#endif

unsigned char *find_http_req(const unsigned char *data, const size_t len,
        unsigned char **http, size_t *httplen);

void dispatch_http_req(const char *mname, const unsigned char *data, const size_t len);

#endif /* __HTTP_H__ */

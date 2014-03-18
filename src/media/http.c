/*
 * http.c:
 * Look for HTTP requests in buffers.
 *
 * We look for GET requests only, and only if the response is of type
 * text/html.
 *
 * Copyright (c) 2003 Chris Lightfoot.
 * Email: chris@ex-parrot.com; WWW: http://www.ex-parrot.com/~chris/
 *
 */

#ifdef HAVE_CONFIG_H
    #include <config.h>
#endif

#include "compat.h"

#include <stdio.h>
#include <stdlib.h> /* On many systems (Darwin...), stdio.h is a prerequisite. */
#include <string.h>

#include "util.h"


/* find_http_req DATA LEN FOUND FOUNDLEN
 * Look for an HTTP request and response in buffer DATA of length LEN. The
 * return value is a pointer into DATA suitable for a subsequent call to this
 * function; *FOUND is either NULL, or a pointer to the start of an HTTP
 * request; in the latter case, *FOUNDLEN is the length of the match
 * containing enough information to obtain the URL. */
unsigned char *find_http_req(const unsigned char *data, const size_t len, unsigned char **http, size_t *httplen) {
    unsigned char *req, *le, *blankline, *hosthdr;

	#define remaining(x)    (len - ((x) - data))
	#define MAX_REQ         16384
	#define HTTPGET_LEN 4
	#define HTTPPOST_LEN 5

    /* HTTP requests look like:
     *
     *      GET {path} HTTP/1.(0|1)\r\n
     *      header: value\r\n
     *          ...
     *      \r\n
     *
     * We may care about the Host: header in the request. */
    if (len < 40)
        return (unsigned char*)data;

    if (!(req = memstr(data, len, (unsigned char*)"GET ", HTTPGET_LEN)) &&
    	!(req = memstr(data, len, (unsigned char*)"POST ", HTTPPOST_LEN))	)
        return (unsigned char*)(data + len - HTTPGET_LEN);

    /* Find the end of the request line. */
    if (!(le = memstr(req + HTTPGET_LEN, remaining(req + HTTPGET_LEN), (unsigned char*)"\r\n", 2))) {
        if (remaining(req + HTTPGET_LEN) > MAX_REQ)
            return (unsigned char*)(req + HTTPGET_LEN);
        else
            return (unsigned char*)req;
    }

    /* Not enough space for a path. */
    if (le < req + 5)
        return le + 2;

    /* Not an HTTP request, just a line starting GET.... */
    if (memcmp(le - 9, " HTTP/1.", 8) || !strchr("01", (int)*(le - 1)))
        return le + 2;

    /* Find the end of the request headers. */
    if (!(blankline = memstr(le + 2, remaining(le + 2), (unsigned char*)"\r\n\r\n", 4))) {
        if (remaining(le + 2) > MAX_REQ)
            return (unsigned char*)(data + len - 4);
        else
            return req;
    }

    if ((memcmp(req + HTTPGET_LEN, "http://", 7) == 0) ||
    	(memcmp(req + HTTPPOST_LEN, "http://", 7) == 0))
        /* Probably a cache request; in any case, don't need to look for a Host:. */
        goto found;

    /* Is there a Host: header? */
    if (!(hosthdr = memstr(le, blankline - le + 2, (unsigned char*)"\r\nHost: ", 8))) {
        return blankline + HTTPGET_LEN;
    }

found:

    *http = req;
    *httplen = blankline - req;

    return blankline + 4;
}

void dispatch_http_req(const char *mname, const unsigned char *data, const size_t len) {
    char *url;
    const char *path, *host;
    int pathlen, hostlen;
    const unsigned char *p;

    if (!(p = memstr(data, len, (unsigned char*)"\r\n", 2)))
        return;

    path = (const char*)(data + 4);
    pathlen = (p - 9) - (unsigned char*)path;

    if (memcmp(path, "http://", 7) == 0) {
        url = malloc(pathlen + 1);
        sprintf(url, "%.*s", pathlen, path);
    } else {

        if (!(p = memstr(p, len - (p - data), (unsigned char*)"\r\nHost: ", 8)))
            return;

        host = (const char*)(p + 8);

        if (!(p = memstr(p + 8, len - (p + 8 - data), (unsigned char*)"\r\n", 2)))
            return;

        hostlen = p - (const unsigned char*)host;

        if (hostlen == 0)
            return;

        url = malloc(hostlen + pathlen + 9);
        sprintf(url, "http://%.*s%.*s", hostlen, host, pathlen, path);
    }

    fprintf(stderr, "\n\n  %s\n\n", url);
    free(url);
}

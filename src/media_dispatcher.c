/*
 * media_dispatcher.c:
 * Dispathc media handling.
 *
 * Copyright (c) 2025 David Suárez.
 * Email: david.sephirot@gmail.com
 *
 */

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common/tmpdir.h"
#include "common/util.h"
#include "common/log.h"
#include "playaudio.h"
#ifndef NO_DISPLAY_WINDOW
    #include "display.h"
#endif
#ifndef NO_HTTP_DISPLAY
    #include "httpd.h"
#endif

#include "media_dispatcher.h"


const char* tmpfile_write_mediaffile(const char* mname, const unsigned char *data, const size_t len)
{
    const char* name = generate_new_tmp_filename(mname);

    tmpfile_write_file(name, data, len);

    return name;
}

void dispatch_image_to_stdout(const char *mname, const unsigned char *data, const size_t len)
{
    const char *name;

    name = tmpfile_write_mediaffile(mname, data, len);
    if (name == NULL)
        return;

    printf("%s/%s\n", get_tmpdir(), name);
}

/*
 * dispatch_image:
 * Throw some image data at the display process.
 */
#ifndef NO_DISPLAY_WINDOW
void dispatch_image_to_display(const char *mname, const unsigned char *data, const size_t len)
{
    const char *name;

    name = tmpfile_write_mediaffile(mname, data, len);
    if (name == NULL)
        return;


    display_send_img(name, TMPNAMELEN);

}
#endif /* !NO_DISPLAY_WINDOW */

/*
 * dispatch_image:
 * Throw some image data at the http display process.
 */
#ifndef NO_HTTP_DISPLAY
void dispatch_image_to_httpdisplay(const char *mname, const unsigned char *data, const size_t len)
{
    const char *name;

    name = tmpfile_write_mediaffile(mname, data, len);
    if (name == NULL)
        return;


    ws_send_text(name);
}
#endif /* !NO_HTTP_DISPLAY */

/*
 * dispatch_mpeg_audio:
 * Throw some MPEG audio into the player process or temporary directory as
 * appropriate.
 */
void dispatch_mpeg_audio(const char *mname, const unsigned char *data, const size_t len) {
    mpeg_submit_chunk(data, len);
}

void dispatch_http_req(const char *mname, const unsigned char *data, const size_t len) {
    char *url;
    const unsigned char *p;

    if (!(p = memstr(data, len, (unsigned char*)"\r\n", 2)))
        return;

    const char *path = (const char *) (data + 4);
    int pathlen = (p - 9) - (unsigned char *) path;

    if (memcmp(path, "http://", 7) == 0) {
        url = malloc(pathlen + 1);
        sprintf(url, "%.*s", pathlen, path);

    } else {
        if (!(p = memstr(p, len - (p - data), (unsigned char*)"\r\nHost: ", 8)))
            return;

        const char *host = (const char *) (p + 8);

        if (!(p = memstr(p + 8, len - (p + 8 - data), (unsigned char*)"\r\n", 2)))
            return;

        int hostlen = p - (const unsigned char *) host;

        if (hostlen == 0)
            return;

        url = malloc(hostlen + pathlen + 9);
        sprintf(url, "http://%.*s%.*s", hostlen, host, pathlen, path);
    }

    log_msg(LOG_SIMPLY, "HTTP Request Captured: %s", url);
    free(url);
}

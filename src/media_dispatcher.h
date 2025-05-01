/*
* media_dispatcher.c:
 * Dispatch media handling.
 *
 * Copyright (c) 2025 David Suárez.
 * Email: david.sephirot@gmail.com
 *
 */

#include <stddef.h>

#ifndef MEDIA_DISPATCHER_H
#define MEDIA_DISPATCHER_H

void dispatch_image_to_stdout(const char *mname, const unsigned char *data, const size_t len);
#ifndef NO_DISPLAY_WINDOW
void dispatch_image_to_display(const char *mname, const unsigned char *data, const size_t len);
#endif /* !NO_DISPLAY_WINDOW */

#ifndef NO_HTTP_DISPLAY
void dispatch_image_to_httpdisplay(const char *mname, const unsigned char *data, const size_t len);
#endif /* !NO_HTTP_DISPLAY */

void dispatch_mpeg_audio(const char *mname, const unsigned char *data, const size_t len);

void dispatch_text_to_stdout(const char *mname, const unsigned char *data, const size_t len);
#ifndef NO_HTTP_DISPLAY
void dispatch_text_to_httpdisplay(const char *mname, const unsigned char *data, const size_t len);
#endif /* !NO_HTTP_DISPLAY */

#endif //MEDIA_DISPATCHER_H

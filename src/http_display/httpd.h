/*
 * httpd.c:
 * HTTP server.
 *
 * Copyright (c) 2012-2018 David Suárez.
 * Email: david.sephirot@gmail.com
 *
 */

#ifndef __HTTPD_H_
#define __HTTPD_H_

void init_http_display(const char* server_root, int port);
void stop_http_display();

void ws_send_media(const char* text, mediatype_t type);

#endif /* __HTTPD_H_ */

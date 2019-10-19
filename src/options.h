/*
 * options.h:
 * Options parsing.
 *
 * Copyright (c) 2012 David Suárez.
 * Email: david.sephirot@gmail.com
 *
 */

#ifndef __OPTIONS_H__
#define __OPTIONS_H__

#ifdef HAVE_CONFIG_H
    #include <config.h>
#endif

#include "media/media.h" /* for enum mediatype */

typedef struct {
    const char *tmpdir;
    int tmpdir_especified;
    int max_tmpfiles;
    int extract_images;
    int verbose;
    int adjunct;
    int beep;
    int promisc;
    char *dumpfile;
    char *interface;
    char *filterexpr;
    mediatype_t extract_type;
    char *audio_mpeg_player;
    int mpeg_player_specified;
    int newpfx;
#ifndef NO_DISPLAY_WINDOW
    char *savedimgpfx;
    int enable_gtk_display;
#endif
    char *drop_username;
	int list_interfaces;
    int monitor_mode;
    int enable_http_display;
    int http_server_port;
} options_t;

options_t* parse_options(int argc, char *argv[]);

#endif /* __OPTIONS_H__ */

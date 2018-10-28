/*
 * options.h:
 * Options parsing.
 *
 * Copyright (c) 2012 David Suárez.
 * Email: david.sephirot@gmail.com
 *
 */

#ifdef HAVE_CONFIG_H
    #include <config.h>
#endif

#include "compat/compat.h"

#include <stdio.h>
#include <stdlib.h> /* On many systems (Darwin...), stdio.h is a prerequisite. */
#include <unistd.h>
#if HAVE_STRING_H
    #include <string.h>
#endif
#include <getopt.h>                     // for optarg, optind, optopt, etc

#include "common/log.h"
#include "network/network.h"

#include "options.h"

options_t options = {NULL, FALSE, 0, TRUE, FALSE, FALSE, FALSE, TRUE,
        NULL, NULL, NULL, MEDIATYPE_IMAGE, NULL, FALSE, FALSE,
#ifndef NO_DISPLAY_WINDOW
        "driftnet-",
        FALSE,
#endif
        NULL, 0, 0, FALSE, 9090
};

static int validate_options(options_t* options);
static void usage(FILE *fp);

/*
 * Handle command-line options
 */
options_t* parse_options(int argc, char *argv[])
{
    char optstring[] = "abd:f:hi:M:m:pSsvx:Z:lr:wW:g";
    int c;

    opterr = 0;
    while ((c = getopt(argc, argv, optstring)) != -1) {
        switch(c) {
            case 'h':
                usage(stdout);
                exit (0);

            case 'i':
                if (options.dumpfile) {
                    log_msg(LOG_ERROR, "can't specify -i and -f");
                    return NULL;
                }
                options.interface = optarg;
                break;

            case 'v':
                options.verbose = TRUE;
                break;

            case 'b':
                if (!isatty(1))
                    log_msg(LOG_WARNING, "can't beep unless standard output is a terminal");
                else
                    options.beep = TRUE;
                break;

            case 'p':
                options.promisc = FALSE;
                break;

            case 's':
                options.extract_type |= MEDIATYPE_AUDIO;
                break;

            case 'S':
                options.extract_type = MEDIATYPE_AUDIO;
                break;

            case 'M':
                options.audio_mpeg_player = optarg;
                options.mpeg_player_specified = TRUE;
                break;

            case 'a':
                options.adjunct = TRUE;
                break;

            case 'm':
                options.max_tmpfiles = atoi(optarg);
                if (options.max_tmpfiles <= 0) {
                    log_msg(LOG_ERROR, "`%s' does not make sense for -m", optarg);
                    return NULL;
                }
                break;

            case 'd':
                options.tmpdir = strdup(optarg);
                options.tmpdir_especified = TRUE; /* so we don't delete it. */
                break;

            case 'f':
                if (options.interface) {
                    log_msg(LOG_ERROR, "can't specify -i and -f");
                    return NULL;
                }
                options.dumpfile = optarg;
                break;

#ifndef NO_DISPLAY_WINDOW
            case 'x':
                options.savedimgpfx = optarg;
                options.newpfx = TRUE;
                break;

            case 'g':

                options.enable_gtk_display = TRUE;
                break;
#endif
            case 'Z':
                options.drop_username = strdup(optarg);
                break;
				
			case 'l':
				options.list_interfaces = TRUE;
				break;
                
            case 'r':
				options.monitor_mode = TRUE;
				break;

#ifndef NO_HTTP_DISPLAY
            case 'w':
                options.enable_http_display = TRUE;
                break;

            case 'W':
                options.http_server_port = atoi(optarg);
                options.enable_http_display = TRUE;
                break;
#endif

            case '?':
            default:
                if (strchr(optstring, optopt))
                    log_msg(LOG_ERROR, "option -%c requires an argument", optopt);
                else
                    log_msg(LOG_ERROR, "unrecognised option -%c", optopt);
                usage(stderr);
                return NULL;
        }
    }

    /* Build up filter. */
    if (optind < argc) {
        if (options.dumpfile)
            log_msg(LOG_WARNING, "filter code ignored with dump file");
        else {
            char **a;
            int l;
            for (a = argv + optind, l = sizeof("tcp and ()"); *a; l += strlen(*a) + 1, ++a);
            options.filterexpr = calloc(l, 1);
            strcpy(options.filterexpr, "tcp and (");
            for (a = argv + optind; *a; ++a) {
                strcat(options.filterexpr, *a);
                if (*(a + 1)) strcat(options.filterexpr, " ");
            }
            strcat(options.filterexpr, ")");
        }
    } else options.filterexpr = "tcp";

    log_msg(LOG_INFO, "using filter expression `%s'", options.filterexpr);

#ifndef NO_DISPLAY_WINDOW
    if (options.newpfx && !options.adjunct)
        log_msg(LOG_INFO, "using saved image prefix `%s'", options.savedimgpfx);
#endif

    if (validate_options(&options) != TRUE) {
        return NULL;
    }

    return &options;
}

int validate_options(options_t* options)
{
	if (options->list_interfaces == 1) {
		return TRUE;
	}

    if (!options->dumpfile) {
        if (!options->interface) {
            /* TODO: on linux works "any" for all interfaces */
            options->interface = network_get_default_interface();

            if (!options->interface) {
                return FALSE;
            }
        }
    }

    /* Let's not be too fascist about option checking.... */
    if (options->max_tmpfiles && !options->adjunct) {
        log_msg(LOG_WARNING, "-m only makes sense with -a");
        options->max_tmpfiles = 0;
    }

    if (options->adjunct && options->newpfx)
        log_msg(LOG_WARNING, "-x ignored -a");

    if (options->mpeg_player_specified && !(options->extract_type & MEDIATYPE_AUDIO))
        log_msg(LOG_WARNING, "-M only makes sense with -s");

    if (options->mpeg_player_specified && options->adjunct)
        log_msg(LOG_WARNING, "-M ignored with -a");

    if (options->max_tmpfiles && options->adjunct)
        log_msg(LOG_INFO, "a maximum of %d images will be buffered", options->max_tmpfiles);

    if (options->beep && options->adjunct)
        log_msg(LOG_WARNING, "can't beep in adjunct mode");

    /*
     * Check for (at least) one display option (GTK by default), if not in adjunct mode.
     */
    if (!options->adjunct) {
        if (options->enable_gtk_display && options->enable_http_display) {
            log_msg(LOG_ERROR, "can't specify -w and -g");
            return FALSE;
        }

        if (!(options->enable_gtk_display || options->enable_http_display)) {
#ifndef NO_DISPLAY_WINDOW
            options->enable_gtk_display = TRUE;

#else
  #ifndef NO_DISPLAY_WINDOW
            options->enable_http_display = TRUE;
  #else
            log_msg(LOG_WARNING, "this version of driftnet was compiled without any display support");
            log_msg(LOG_WARNING, "switching to adjunct mode");
            options->adjunct = TRUE;
  #endif
#endif
        }
    }

    return TRUE;
}

/* usage:
 * Print usage information. */
void usage(FILE *fp)
{
    fprintf(fp,
"driftnet, version %s\n"
"Capture images from network traffic and display them.\n"
"\n"
"Synopsis: driftnet [options] [filter code]\n"
"\n"
"Options:\n"
"\n"
"  -h               Display this help message.\n"
"  -v               Verbose operation.\n"
"  -b               Beep when a new image is captured.\n"
"  -i interface     Select the interface on which to listen (default: all\n"
"                   interfaces).\n"
"  -f file          Instead of listening on an interface, read captured\n"
"                   packets from a pcap dump file; file can be a named pipe\n"
"                   for use with Kismet or similar.\n"
"  -p               Do not put the listening interface into promiscuous mode.\n"
"  -a               Adjunct mode: do not display images on screen, but save\n"
"                   them to a temporary directory and announce their names on\n"
"                   standard output.\n"
"  -m number        Maximum number of images to keep in temporary directory\n"
"                   in adjunct mode.\n"
"  -d directory     Use the named temporary directory.\n"
"  -x prefix        Prefix to use when saving images.\n"
"  -s               Attempt to extract streamed audio data from the network,\n"
"                   in addition to images. At present this supports MPEG data\n"
"                   only.\n"
"  -S               Extract streamed audio but not images.\n"
"  -M command       Use the given command to play MPEG audio data extracted\n"
"                   with the -s option; this should process MPEG frames\n"
"                   supplied on standard input. Default: `mpg123 -'.\n"
"  -Z username      Drop privileges to user 'username' after starting pcap.\n"
"  -l               List the system capture interfaces.\n"
"  -p               Put the interface in monitor mode (not supported on all interfaces).\n"
#ifndef NO_DISPLAY_WINDOW
"  -g               Enable GTK display (this is the default).\n"
#endif
#ifndef NO_HTTP_DISPLAY
"  -w               Enable the HTTP server to display images.\n"
"  -W               Port number for the HTTP server (implies -w). Default: 9090.\n"
#endif
"\n"
"Filter code can be specified after any options in the manner of tcpdump(8).\n"
"The filter code will be evaluated as `tcp and (user filter code)'\n"
"\n"
"You can save images to the current directory by clicking on them.\n"
"\n"
"Adjunct mode is designed to be used by other programs which want to use\n"
"driftnet to gather images from the network. With the -m option, driftnet will\n"
"silently drop images if more than the specified number of images are saved\n"
"in its temporary directory. It is assumed that some other process is\n"
"collecting and deleting the image files.\n"
"\n"
"driftnet, copyright (c) 2001-2 Chris Lightfoot <chris@ex-parrot.com>\n"
"          copyright (c) 2012-18 David Suárez <david.sephirot@gmail.com>\n"
"home page: https://github.com/deiv/driftnet\n"
"old home page: http://www.ex-parrot.com/~chris/driftnet/\n"
"\n"
"This program is free software; you can redistribute it and/or modify\n"
"it under the terms of the GNU General Public License as published by\n"
"the Free Software Foundation; either version 2 of the License, or\n"
"(at your option) any later version.\n"
"\n",
            DRIFTNET_VERSION);
}

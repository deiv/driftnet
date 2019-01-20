/*
 * driftnet.c:
 * Pick out images from passing network traffic.
 *
 * Copyright (c) 2012 David Suárez.
 * Email: david.sephirot@gmail.com
 *
 * Copyright (c) 2001 Chris Lightfoot.
 * Email: chris@ex-parrot.com; WWW: http://www.ex-parrot.com/~chris/
 *
 */

#include <compat/compat.h>

#include <stdio.h>
#include <stdlib.h> /* On many systems (Darwin...), stdio.h is a prerequisite. */
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

#include "common/util.h"
#include "common/log.h"
#include "options.h"
#include "common/tmpdir.h"
#include "pid.h"
#include "network/network.h"
#include "playaudio.h"
#include "uid.h"
#ifndef NO_DISPLAY_WINDOW
    #include "display.h"
#endif
#ifndef NO_HTTP_DISPLAY
    #include "httpd.h"
#endif

#include "driftnet.h"

static void terminate_on_signal(int s);
static void setup_signals(void);

/*
void unexpected_exit(int ret)
{
    // clean things a litle
#ifndef NO_HTTP_DISPLAY
    stop_http_display();
#endif
    network_close();
    connection_free_slots();
    clean_tmpdir();
    close_pidfile();

	exit(ret);
}*/

/* terminate_on_signal:
 * Terminate on receipt of an appropriate signal. */
sig_atomic_t foad;

void terminate_on_signal(int s)
{
    extern pid_t mpeg_mgr_pid; /* in playaudio.c */

    if (mpeg_mgr_pid) {
        stop_mpeg_player();
    }

    foad = s;
}

/*
 * Set up signal handlers.
 */
void setup_signals(void) {
    int *p;
    /* Signals to ignore. */
    int ignore_signals[] = {SIGPIPE, 0};
    /* Signals which mean we should quit, killing the display child if
     * applicable. */
    int terminate_signals[] = {SIGTERM, SIGINT, /*SIGSEGV,*/ SIGBUS, SIGCHLD, 0};
    struct sigaction sa;

    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;

    for (p = ignore_signals; *p; ++p) {
        memset(&sa, 0, sizeof(sa));
        sa.sa_handler = SIG_IGN;
        sigaction(*p, &sa, NULL);
    }

    for (p = terminate_signals; *p; ++p) {
        memset(&sa, 0, sizeof(sa));
        sa.sa_handler = terminate_on_signal;
        sigaction(*p, &sa, NULL);
    }
}



static void print_exit_reason(void)
{
    if (foad == SIGCHLD) {
        pid_t pp;
        int st;

        while ((pp = waitpid(-1, &st, WNOHANG)) > 0) {
            if (WIFEXITED(st))
                log_msg(LOG_INFO, "child process %d exited with status %d", (int)pp, WEXITSTATUS(st));
            else if (WIFSIGNALED(st))
                log_msg(LOG_INFO, "child process %d killed by signal %d", (int)pp, WTERMSIG(st));
            else
                log_msg(LOG_INFO, "child process %d died, not sure why", (int)pp);

        }

    } else
        log_msg(LOG_INFO, "caught signal %d", foad);
}

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

/*
 * Entry point. Process command line options, start up pcap and enter capture loop.
 */
int main(int argc, char *argv[])
{
    options_t *options;
    int ok;

    options = parse_options(argc, argv);

    if (options == NULL) {
        return 1;
    }
	
	if (options->verbose) {
        set_loglevel(LOG_INFO);
    }
	
	if (options->list_interfaces == 1) {
		return !network_list_interfaces();
	}

    /* Start up pcap as soon as posible to later drop root privileges. */
    if (options->dumpfile) {
        ok = network_open_offline(options->dumpfile);

    } else {
        ok = network_open_live(options->interface, options->filterexpr, options->promisc, options->monitor_mode);
    }

    if (!ok) {
        return 1;
    }

    /* If we are root and an username was specified, drop privileges to that user */
    if (getuid() == 0 || geteuid() == 0) {
        if (options->drop_username) {
            drop_root(options->drop_username);
        }
    }

    if (options->adjunct)
        create_pidfile();

    /*
     * In adjunct mode, it's important that the attached program gets
     * notification of images in a timely manner. Make stdout line-buffered
     * for this reason.
     */
    if (options->adjunct)
        setvbuf(stdout, NULL, _IOLBF, 0);

    /*
     * If a directory name has not been specified, then we need to create one.
     * Otherwise, check that it's a directory into which we may write files.
     */
    if (options->tmpdir) {
        if (check_dir_is_rw(options->tmpdir) == FALSE) {
            log_msg(LOG_ERROR, "we can't write to the temporary directory");
            exit(1);
        }
        set_tmpdir(options->tmpdir, TMPDIR_USER_OWNED, options->max_tmpfiles, options->adjunct);

    } else {
        /* need to make a temporary directory. */
        const char* tmp_dir = make_tmpdir();

        if (tmp_dir == NULL) {
            log_msg(LOG_ERROR, "can't make a new temporary directory");
            exit(1);
        }
        set_tmpdir(tmp_dir, TMPDIR_APP_OWNED, options->max_tmpfiles, options->adjunct);
    }

    setup_signals();

    /* Start up the audio player, if required. */
    if (!options->adjunct && (options->extract_type & MEDIATYPE_AUDIO))
        do_mpeg_player();

#ifndef NO_DISPLAY_WINDOW
    /* Possibly fork to start the display child process */
    if (options->enable_gtk_display && !options->adjunct && (options->extract_type & MEDIATYPE_IMAGE))
        do_image_display(options->savedimgpfx, options->beep);

#endif /* !NO_DISPLAY_WINDOW */

#ifndef NO_HTTP_DISPLAY
    if (options->enable_http_display && !options->adjunct) {
        init_http_display(get_tmpdir(), options->http_server_port);
    }
#endif
    if (options->adjunct) {
        log_msg(LOG_INFO, "operating in adjunct mode");
    }

    mediadrv_t** drivers = get_drivers_for_mediatype(options->extract_type);

    for (int i = 0; i < NMEDIATYPES; ++i) {

        if (drivers[i] == NULL) {
            continue;
        }

        switch (drivers[i]->type) {
            case MEDIATYPE_IMAGE:
                /*
                 * XXX: options->enable_http_display, options->enable_gtk_display
                 */
                if (options->adjunct) {
                    drivers[i]->dispatch_data = dispatch_image_to_stdout;

                } else {
                    if (options->enable_gtk_display) {
#ifndef NO_DISPLAY_WINDOW
                        drivers[i]->dispatch_data = dispatch_image_to_display;
#endif
                    } else {
#ifndef NO_HTTP_DISPLAY
                        drivers[i]->dispatch_data = dispatch_image_to_httpdisplay;
#endif
                    }
                }
                break;

            case MEDIATYPE_AUDIO:
                drivers[i]->dispatch_data = dispatch_mpeg_audio;
                break;

            case MEDIATYPE_TEXT:
                drivers[i]->dispatch_data = dispatch_http_req;
                break;
        }
    }

    network_start(drivers);

    while (!foad)
        sleep(1);

    if (options->verbose)
        print_exit_reason();

#ifndef NO_HTTP_DISPLAY
    if (options->enable_http_display) {
        stop_http_display();
    }
#endif

    stop_mpeg_player();

    /* Clean up. */
    /*    pcap_freecode(pc, &filter);*/ /* not on some systems... */
    network_close();

    clean_tmpdir();

    if (options->adjunct)
        close_pidfile();

    return 0;
}

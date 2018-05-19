/*
 * httpd.c:
 * HTTP server.
 *
 * Copyright (c) 2012-2018 David Suárez.
 * Email: david.sephirot@gmail.com
 *
 */

#ifdef HAVE_CONFIG_H
    #include <config.h>
#endif

#include <libwebsockets.h>
#include <string.h>
#include <signal.h>

#include "web_data.h"
#include "log.h"
#include "util.h"

int server_port;
int interrupted = 0;
pthread_t server_thread;

/*
 * TODO: multi client (atm we only suppport one connection)... */
struct lws *client = NULL;

struct session_ws_images {
    int number;
};

int ws_callback(struct lws *wsi, enum lws_callback_reasons reason,
                     void *user, void *in, size_t len);

static struct lws_protocols protocols[] = {
        { "http", lws_callback_http_dummy, 0, 0 },
        {
          "images-pipe-protocol",
          ws_callback,
           sizeof(struct session_ws_images),
              128,                              /* rx buf size */
        },
        { NULL, NULL, 0, 0 }                    /* terminator */
};

/*
 * TODO: use the tmpdir related functions to handle the web static files.
 */
void write_static_file(char* server_root, char* filename, unsigned char *file_data, unsigned int data_len)
{
    int fd1;
    char* resource_filename;
    int len;

    len  = strlen(server_root);
    len += strlen(filename);
    len += 1; /* for null */
    resource_filename = xmalloc(len);

    snprintf(resource_filename, len, "%s%s", server_root, filename);

    fd1 = open(resource_filename, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd1 == -1) {
        log_msg(LOG_ERROR, "%s: %s", resource_filename, strerror(errno));
        close(fd1);
        xfree(resource_filename);
        return;
    }

    unsigned char *buf_ptr = file_data;
    size_t buf_len = data_len;

    while (buf_len > 0) {
        int written = write(fd1, buf_ptr, buf_len);

        if (written <= 0) {
            log_msg(LOG_ERROR, "%s: %s", resource_filename, strerror(errno));
            break;
        }

        buf_ptr += written;
        buf_len -= written;
    }

    xfree(resource_filename);
    close(fd1);
}

void delete_static_file(char* server_root, char* filename)
{
    char* resource_filename;
    int len;

    len  = strlen(server_root);
    len += strlen(filename);
    len += 1; /* for null */
    resource_filename = xmalloc(len);

    snprintf(resource_filename, len, "%s%s", server_root, filename);

    unlink(resource_filename);

    xfree(resource_filename);
}

void write_static_resources(char *server_root)
{
    write_static_file(server_root, "/index.html", static_web_index_html, static_web_index_html_len);
    write_static_file(server_root, "/jquery.js", static_web_jquery_js, static_web_jquery_js_len);
    write_static_file(server_root, "/close.png", static_web_close_png, static_web_close_png_len);
}

void delete_static_resources(char *server_root)
{
    delete_static_file(server_root, "/index.html");
    delete_static_file(server_root, "/jquery.js");
    delete_static_file(server_root, "/close.png");
}

static void * http_server_dispatch(void *arg)
{
    char* server_root = arg;
    int n = 0;
    struct lws_context_creation_info info;
    struct lws_context *context;

   static const struct lws_protocol_vhost_options mime_types[] = {
       { NULL, NULL, ".jpeg", "image/jpeg" }
   };

   const struct lws_http_mount mount = {
        (struct lws_http_mount *)NULL,	/* linked-list pointer to next*/
        "/",		                    /* mountpoint in URL namespace on this vhost */
        server_root,                    /* where to go on the filesystem for that */
        "index.html",	                /* default filename if none given */
        NULL,
        NULL,
        mime_types,
        NULL,
        0,
        0,
        0,
        0,
        0,
        0,
        LWSMPRO_FILE,	                /* mount type is a directory in a filesystem */
        1,		                        /* strlen("/"), ie length of the mountpoint */
        NULL,
        { NULL, NULL } // sentinel
    };


    write_static_resources(server_root);

    memset(&info, 0, sizeof info);

    info.port = server_port;
    info.mounts = &mount;
    info.protocols = protocols;

    lws_set_log_level(LLL_ERR | LLL_WARN, NULL);

    context = lws_create_context(&info);

    if (!context) {
        log_msg(LOG_ERROR, "http server init failed");
        return NULL;
    }

    log_msg(LOG_WARNING, "http server initializated. go to http://localhost:%d", server_port);

    while (n >= 0 && !interrupted) {
        n = lws_service(context, 1000);
    }

    lws_context_destroy(context);

    delete_static_resources(server_root);

    return NULL;
}

void ws_send_text(const char* text)
{
    if (client != NULL) {
        lws_write(client, (unsigned char*)text, strlen(text), LWS_WRITE_TEXT);
    }
}

int ws_callback(struct lws *wsi, enum lws_callback_reasons reason,
                 void *user, void *in, size_t len)
{
    struct session_ws_images *pss = (struct session_ws_images *)user;

    switch (reason) {

        case LWS_CALLBACK_ESTABLISHED:
            client = wsi;
            pss->number = 0;
            break;

        default:
            break;
    }

    return 0;
}

void init_http_display(const char* server_root, int port)
{
    server_port = port;

    pthread_create(&server_thread, NULL, http_server_dispatch, (void*)server_root);
}

void stop_http_display()
{
    interrupted = 1;

    pthread_cancel(server_thread);
    pthread_join(server_thread, NULL);
}

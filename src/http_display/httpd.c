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
#include "tmpdir.h"

int server_port;
int interrupted = 0;
pthread_t server_thread;

struct session_ws_images {
    struct lws *client;
    struct session_ws_images *next;
};

struct session_ws_images *client_list;
struct session_ws_images *client_list_tail = NULL;

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

void write_static_resources()
{
    web_static_file_t* static_file = &static_content[0];

    while (static_file->size != 0) {
        tmpfile_write_file(static_file->name, static_file->data, static_file->size);
        static_file++;
    }
}

void delete_static_resources()
{
    web_static_file_t* static_file = &static_content[0];

    while (static_file->size != 0) {
        tmpfile_delete_file(static_file->name);
        static_file++;
    }
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


    write_static_resources();

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

    delete_static_resources();

    return NULL;
}

/*
 * XXX: [2018/05/19 23:30:14:7811] ERR: ****** 0x7fdf74004e40: Sending new 32 (�driftnet-5b009766), pending truncated ...
       It's illegal to do an lws_write outside of
       the writable callback: fix your code

 */
void ws_send_text(const char* text)
{
    struct session_ws_images *client_list_head = client_list;

    while (client_list_head != NULL) {
        lws_write(client_list_head->client, (unsigned char*)text, strlen(text), LWS_WRITE_TEXT);
        client_list_head = client_list_head->next;
    }
}

int ws_callback(struct lws *wsi, enum lws_callback_reasons reason,
                 void *user, void *in, size_t len)
{
    struct session_ws_images *pss = (struct session_ws_images *)user;

    switch (reason) {

        case LWS_CALLBACK_ESTABLISHED:
            pss->client = wsi;

            /* first client ? */
            if (client_list_tail == NULL) {
                client_list_tail = pss;
                client_list = client_list_tail;

            } else {
                client_list_tail->next = pss;
                client_list_tail = pss;
                pss->next = NULL;
            }
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

/*
 * display.c:
 * Display images gathered by driftnet.
 *
 * Copyright (c) 2001 Chris Lightfoot. All rights reserved.
 * Email: chris@ex-parrot.com; WWW: http://www.ex-parrot.com/~chris/
 *
 */

static const char rcsid[] = "$Id: display.c,v 1.4 2001/08/03 17:55:01 chris Exp $";

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#include "driftnet.h"
#include "img.h"

#define BORDER  6

static GtkWidget *window, *darea;

static int width, height, wrx, wry, rowheight;
static img backing_image;

gint delete_event(GtkWidget *widget, GdkEvent *event, gpointer data) {
    fprintf(stderr, PROGNAME ": display child shutting down\n");
    return FALSE;   /* do destroy window */
}

void make_backing_image() {
    img I;
    printf("%d, %d\n", width, height);
    I = img_new_blank(width, height);
    img_alloc(I);
    if (backing_image) {
        /* XXX copy old contents of backing image over. */
        img_delete(backing_image);
    }
    backing_image = I;
    wrx = BORDER;
    wry = height - BORDER;
    rowheight = 2 * BORDER;
}

void update_window() {
    if (backing_image) {
        GdkGC *gc = gdk_gc_new(darea->window);
        gdk_draw_rgb_32_image(darea->window, gc, 0, 0, width, height, GDK_RGB_DITHER_NORMAL, (guchar*)backing_image->flat, sizeof(pel) * width);
        gdk_gc_destroy(gc);
    }
}

void scroll_backing_image(const int dy) {
    pel **row1, **row2;
    for (row1 = backing_image->data, row2 = backing_image->data + dy;
         row2 < backing_image->data + height; ++row1, ++row2)
        memcpy(*row1, *row2, width * sizeof(pel));

    for (row2 = row1; row2 < backing_image->data + height; ++row2)
        memset(*row2, 0, width * sizeof(pel));
}

void expose_event(GtkWidget *widget, GdkEvent *event, gpointer data) {
    gdk_window_get_size(darea->window, &width, &height);
    if (!backing_image || backing_image->width != width || backing_image->height != height)
        make_backing_image();

    update_window();
}

void configure_event(GtkWidget *widget, GdkEvent *event, gpointer data) {
    gdk_window_get_size(darea->window, &width, &height);
    if (!backing_image || backing_image->width != width || backing_image->height != height)
        make_backing_image();

    update_window();
}

void destroy(GtkWidget *widget, gpointer data) {
    gtk_main_quit();
}

extern int dpychld_fd;  /* in driftnet.c */

gboolean pipe_event(GIOChannel chan, GIOCondition cond, gpointer data) {
    struct pipemsg m = {0};
    while (read(dpychld_fd, &m, sizeof(m)) == sizeof(m)) {
        printf(PROGNAME": received image %s of size %d\n", m.filename, m.len);
        /* checks to see whether this looks like an image we're interested in. */
        if (m.len > 256) {
            /* small images are probably bollocks. */
            img i = img_new();
            if (!img_load_file(i, m.filename, header, unknown))
                fprintf(stderr, PROGNAME": %s: bogus image (err = %d)\n", m.filename, i->err);
            else {
                if (i->width > 8 && i->height > 8) {
                    if (img_load(i, full, i->type)) {
                        /* slot in the new image at some plausible place. */
                        int w, h;
                        if (i->width > width - 2 * BORDER) w = width - 2 * BORDER;
                        else w = i->width;
                        if (i->height > height - 2 * BORDER) h = height - 2 * BORDER;
                        else h = i->height;

                        /* is there space on this row? */
                        if (width - wrx < w) {
                            /* no */
                            scroll_backing_image(h + BORDER);
                            wrx = BORDER;
                            rowheight = h + BORDER;
                        }
                        if (rowheight < h + BORDER) {
                            scroll_backing_image(h + BORDER - rowheight);
                            rowheight = h + BORDER;
                        }

                        img_simple_blt(backing_image, wrx, wry - h, i, 0, 0, w, h);

                        update_window();

                        wrx += w + BORDER;
                    } else fprintf(stderr, PROGNAME": %s: bogus image (err = %d)\n", m.filename, i->err);
                } else fprintf(stderr, PROGNAME": %s: image dimensions (%d x %d) too small to bother with\n", m.filename, i->width, i->height);
            }

            img_delete(i);
            unlink(m.filename);
        } else fprintf(stderr, PROGNAME": image data too small (%d bytes) to bother with\n", (int)m.len);
    }
    return TRUE;
}

int dodisplay(int argc, char *argv[]) {
    GIOChannel *chan;

    /* have our main loop poll the pipe file descriptor */
    chan = g_io_channel_unix_new(dpychld_fd);
    g_io_add_watch(chan, G_IO_IN | G_IO_ERR | G_IO_HUP, (GIOFunc)pipe_event, NULL);
    fcntl(dpychld_fd, F_SETFL, O_NONBLOCK);
       
    /* do some init thing */
    gtk_init(&argc, &argv);
    gdk_rgb_init();

    gtk_widget_set_default_colormap(gdk_rgb_get_cmap());
    gtk_widget_set_default_visual(gdk_rgb_get_visual());

    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_widget_set_usize(window, 500, 500);

    darea = gtk_drawing_area_new();
    gtk_container_add(GTK_CONTAINER(window), darea);
    gtk_widget_set_events(darea, GDK_EXPOSURE_MASK);

    gtk_signal_connect(GTK_OBJECT(window), "delete_event", GTK_SIGNAL_FUNC(delete_event), NULL);
    gtk_signal_connect(GTK_OBJECT(window), "destroy", GTK_SIGNAL_FUNC(destroy), NULL);

    gtk_signal_connect(GTK_OBJECT(darea), "expose-event", GTK_SIGNAL_FUNC(expose_event), NULL);
    gtk_signal_connect(GTK_OBJECT(darea), "configure_event", GTK_SIGNAL_FUNC(expose_event), NULL);
    
    gtk_widget_show_all(window);
    
    gtk_main();
                         
    return 0;
}

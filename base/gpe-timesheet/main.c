/*
 * Copyright (C) 2002 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <sys/types.h>
#include <stdlib.h>
#include <time.h>
#include <libintl.h>

#include <gtk/gtk.h>

#include "pixmaps.h"
#include "init.h"

#define _(_x) gettext(_x)

#define MY_PIXMAPS_DIR "/usr/share/gpe-timesheet/pixmaps"
#define PIXMAPS_DIR "/usr/share/gpe/pixmaps"

static const guint window_x = 240, window_y = 320;

struct pix my_pix[] = {
  { "delete", PIXMAPS_DIR "/delrecord.xpm" },
  { "new", PIXMAPS_DIR "/new.xpm" },
  { "start", MY_PIXMAPS_DIR "/clock.png" },
  { "stop", MY_PIXMAPS_DIR "/xclock.png" },
  { "tick", PIXMAPS_DIR "/tick.xpm" },
  { NULL, NULL }
};

int
main(int argc, char *argv[])
{
  GtkWidget *window;
  GtkWidget *vbox_top;
  GtkWidget *toolbar;
  GtkWidget *pw;
  GtkWidget *tree;
  struct pix *p;

  if (gpe_application_init (&argc, &argv) == FALSE)
    exit (1);

  if (load_pixmaps (my_pix) == FALSE)
    exit (1);

  toolbar = gtk_toolbar_new (GTK_ORIENTATION_HORIZONTAL, GTK_TOOLBAR_ICONS);

  gtk_toolbar_set_button_relief (GTK_TOOLBAR (toolbar), GTK_RELIEF_NONE);

  tree = gtk_ctree_new (2, 0);

  vbox_top = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox_top), toolbar, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox_top), tree, TRUE, TRUE, 0);

  p = find_pixmap ("new");
  pw = gtk_pixmap_new (p->pixmap, p->mask);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("New task"), 
			   _("New task"), _("New task"),
			   pw, NULL, NULL);

  p = find_pixmap ("delete");
  pw = gtk_pixmap_new (p->pixmap, p->mask);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Delete task"),
			   _("Delete task"), _("Delete task"),
			   pw, NULL, NULL);

  p = find_pixmap ("start");
  pw = gtk_pixmap_new (p->pixmap, p->mask);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Start timing"),
			   _("Start timing"), _("Start timing"),
			   pw, NULL, NULL);
  
  p = find_pixmap ("stop");
  pw = gtk_pixmap_new (p->pixmap, p->mask);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Stop timing"),
			   _("Stop timing"), _("Stop timing"),
			   pw, NULL, NULL);


  gtk_widget_show (toolbar);
  gtk_widget_show (vbox_top);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_container_add (GTK_CONTAINER (window), vbox_top);

  gtk_widget_show (window);

  gtk_widget_set_usize (window, window_x, window_y);

  gtk_main ();

  return 0;
}

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
#include <locale.h>

#include <gtk/gtk.h>

#include "todo.h"
#include "todo-sql.h"
#include "pixmaps.h"
#include "init.h"
#include "what.h"

static struct gpe_icon my_icons[] = {
  { "ok", "ok" },
  { "cancel", "cancel" },
  { "save", "save" },
  { "new", "new" },
  { "hide", "hide" },
  { "properties", "properties" },
  { "delete", "delete" },
  { "cancel", "cancel" },
  { "exit", "exit" },
  { "icon", PREFIX "/share/pixmaps/gpe-todo.png" },
  { NULL, NULL }
};

#define _(_x) gettext(_x)

guint window_x = 240, window_y = 320;

GtkWidget *the_notebook;
GtkWidget *window;

extern GtkWidget *top_level (GtkWidget *window);

int hide=0;
  
static void
open_window (void)
{
  GdkPixmap *pmap;
  GdkBitmap *bmap;
  GtkWidget *top;

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

  top = top_level(window);

  gtk_container_add (GTK_CONTAINER (window), top);

  gtk_signal_connect (GTK_OBJECT (window), "destroy",
		      GTK_SIGNAL_FUNC (gtk_exit), NULL);

  gtk_widget_set_usize (window, 240, 320);

  gtk_widget_realize (window);
  if (gpe_find_icon_pixmap ("icon", &pmap, &bmap))
    gdk_window_set_icon (window->window, NULL, pmap, bmap);

  gtk_widget_show (window);
}

int
main(int argc, char *argv[])
{
  if (gpe_application_init (&argc, &argv) == FALSE)
    exit (1);

  setlocale (LC_ALL, "");

  bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
  textdomain (PACKAGE);

  if (gpe_load_icons (my_icons) == FALSE)
    exit (1);

  if (sql_start ())
    exit (1);

  what_init ();

  open_window ();
  
  gtk_main ();

  return 0;
}

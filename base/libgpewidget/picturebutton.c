/*
 * Copyright (C) 2001, 2002 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <gtk/gtk.h>

#include "render.h"
#include "pixmaps.h"
#include "picturebutton.h"

GtkWidget *
gpe_picture_button (GtkStyle *style, gchar *text, gchar *icon)
{
  GdkPixbuf *p = gpe_try_find_icon (icon, NULL);
  GtkWidget *button = gtk_button_new ();
  GtkWidget *hbox = gtk_hbox_new (FALSE, 0);
  GtkWidget *hbox2 = gtk_hbox_new (FALSE, 0);

  if (p)
    {
      GtkWidget *pw = gpe_render_icon (style, p);
      gtk_box_pack_start (GTK_BOX (hbox), pw, FALSE, FALSE, 0);
      gtk_widget_show (pw);
    }
  if (text)
    {
      GtkWidget *label = gtk_label_new (text);
      gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 4);
      gtk_widget_show (label);
    }

  gtk_widget_show (hbox2);
  gtk_widget_show (hbox);
  gtk_widget_show (button);

  gtk_box_pack_start (GTK_BOX (hbox2), hbox, TRUE, FALSE, 0);

  gtk_container_add (GTK_CONTAINER (button), hbox2);

  return button;
}

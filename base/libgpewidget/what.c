/*
 * Copyright (C) 2002 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <stdio.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>

#include <libintl.h>

#define _(x) dgettext(PACKAGE, x)

static GdkAtom help_atom, string_atom;
static Atom help_xatom, string_xatom;

void gpe_what_mark_widget (GtkWidget *widget);

GSList *widgets;

static void
send_text (Display *dpy, Window w, char *text)
{
  XChangeProperty (dpy, w, help_xatom, string_xatom, 8, PropModeReplace, text, strlen (text));
}

static GdkFilterReturn 
filter_func (GdkXEvent *xev, GdkEvent *ev, gpointer p)
{
  XClientMessageEvent *xc = (XClientMessageEvent *)xev;
  GSList *list;
  Window sender = xc->data.l[0];

  for (list = widgets; list; list = list->next)
    {
      GtkWidget *widget = GTK_WIDGET (list->data);
      int x = xc->data.l[1], y = xc->data.l[2];

      if (widget->window == ev->any.window
	  && x >= widget->allocation.x && x < widget->allocation.x + widget->allocation.width
	  && y >= widget->allocation.y && y < widget->allocation.y + widget->allocation.height)
	{
	  GtkTooltipsData *data = gtk_tooltips_data_get (widget);
	  send_text (GDK_WINDOW_XDISPLAY (widget->window), sender, data->tip_text);
	  return GDK_FILTER_CONTINUE;
	}
    }

  send_text (GDK_WINDOW_XDISPLAY (ev->any.window), sender, _("No help available."));

  return GDK_FILTER_CONTINUE;
}

void
gpe_what_mark_widget (GtkWidget *widget)
{
  if (widget->window)
    {
      widgets = g_slist_prepend (widgets, widget);

      gdk_property_change (widget->window,
			   help_atom,
			   help_atom,
			   8,
			   GDK_PROP_MODE_REPLACE,
			   NULL,
			   0);
    }
  else
    g_signal_connect_after (widget, "realize", G_CALLBACK (gpe_what_mark_widget), NULL);
}

void
gpe_what_handle_event (GdkEvent *event, GtkWidget *widget)
{
  fprintf (stderr, "handling event\n");

  if (event->client.message_type == help_atom)
    {
      GtkTooltipsData *tooltipsdata;

      tooltipsdata = gtk_tooltips_data_get (widget);

      printf ("%s\n", tooltipsdata->tip_text);
    }
}

void
gpe_what_init (void)
{
  help_atom = gdk_atom_intern ("GPE_INTERACTIVE_HELP", FALSE);
  string_atom = gdk_atom_intern ("STRING", False);

  help_xatom = gdk_x11_atom_to_xatom (help_atom);
  string_xatom = gdk_x11_atom_to_xatom (string_atom);

  gdk_add_client_message_filter (help_atom, filter_func, NULL);
}

/*
 * Copyright (C) 2001, 2002 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#ifndef CLIST_VIEW_H
#define CLIST_VIEW_H

#include <glib.h>
#include <gtk/gtk.h>

extern void selection_made( GtkWidget      *clist,
			    gint            row,
			    gint            column,
			    GdkEventButton *event,
			    GtkWidget      *widget);

#endif

/*
 * Copyright (C) 2001, 2002 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <gtk/gtk.h>
#include <libintl.h>

#include <gpe/init.h>
#include <gpe/pixmaps.h>
#include <gpe/render.h>

#include "event-db.h"
#include "event-ui.h"
#include "globals.h"

#include "day_view.h"
#include "week_view.h"
#include "future_view.h"
#include "year_view.h"
#include "month_view.h"

#define _(_x) gettext (_x)

extern void about (void);

GList *times;
time_t viewtime;

GtkWidget *main_window, *pop_window;

GdkFont *yearfont, *datefont;

struct gpe_icon my_icons[] = {
  { "new", "new" },
  { "home", "home" },
  { "future_view", "future_view" },
  { "day_view", "day_view" },
  { "week_view", "week_view" },
  { "month_view", "month_view" },
  { "year_view", "year_view" },
  { "exit", "exit" },
  { "delete", "delete" },
  { "save", "save" },
  { "cancel", "cancel" },
  { "bell", "bell" },
  { "recur", "recur" },
  { "bell_recur", "bell_recur" },
  { "icon", PREFIX "/share/pixmaps/gpe-calendar.png" },
  {NULL, NULL}
};

static GtkWidget *day, *week, *month, *future, *year, *current_view;
static GtkWidget *day_button, *week_button, *month_button, *year_button, *future_button;

guint window_x = 240, window_y = 310;

gboolean week_starts_monday = TRUE;

static guint nr_days[] = { 31, 28, 31, 30, 31, 30, 
			   31, 31, 30, 31, 30, 31 };

guint
days_in_month (guint year, guint month)
{
  if (month == 1)
    {
      return ((year % 4) == 0
	      && ((year % 100) != 0
		  || (year % 400) == 0)) ? 29 : 28;
    }

  return nr_days[month];
}

void
update_current_view (void)
{
  if (current_view)
    {
      gpointer p = gtk_object_get_data (GTK_OBJECT (current_view),
					"update_hook");
      if (p)
	{
	  void (*f)(void) = p;
	  f ();
	}
    }
}

static void
new_view (GtkWidget *widget)
{
  if (current_view)
    gtk_widget_hide (current_view);

  if (pop_window)
    gtk_widget_destroy (pop_window);
  
  gtk_widget_show (widget);
  current_view = widget;

  update_current_view ();
}

void
set_day_view (void)
{
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (day_button), TRUE);
  new_view (day);
}

static void
new_appointment (void)
{
  GtkWidget *appt = new_event (viewtime, 0);
  gtk_widget_show (appt);
}

static void
set_today(void)
{
  time (&viewtime);
  update_current_view ();
}

static void
button_toggled (GtkWidget *widget, gpointer data)
{
  new_view (data);
}

int
main (int argc, char *argv[])
{
  GtkWidget *vbox, *toolbar;
  GdkPixbuf *p;
  GtkWidget *pw;
  GdkPixmap *pmap;
  GdkBitmap *bmap;

  guint hour;

  if (gpe_application_init (&argc, &argv) == FALSE)
    exit (1);

  if (gpe_load_icons (my_icons) == FALSE)
    exit (1);

  setlocale (LC_ALL, "");

  bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
  textdomain (PACKAGE);

  if (event_db_start () == FALSE)
    exit (1);

  for (hour = 0; hour < 24; hour++)
    {
      char buf[32];
      struct tm tm;
      time_t t=time(NULL);
      
      localtime_r (&t, &tm);
      tm.tm_hour = hour;
      tm.tm_min = 0;
      strftime (buf, sizeof(buf), TIMEFMT, &tm);
      times = g_list_append (times, g_strdup (buf));
      tm.tm_hour = hour;
      tm.tm_min = 30;
      strftime (buf, sizeof(buf), TIMEFMT, &tm);
      times = g_list_append (times, g_strdup (buf));
    }

  datefont = gdk_font_load ("-*-*-medium-r-normal--8-*-*-*-c-*-*");
  if (datefont == NULL)
    {
      printf ("Couldn't get date font\n");
      abort ();
    }
  yearfont = gdk_font_load ("-*-*-medium-r-normal--8-*-*-*-*-*-*");
  if (yearfont == NULL)
    {
      printf ("Couldn't get year font\n");
      abort ();
    }

  vbox = gtk_vbox_new (FALSE, 0);

  time (&viewtime);
  week = week_view ();
  day = day_view ();
  month = month_view ();
  future = future_view ();
  year = year_view ();

  main_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW(main_window), "Calendar");
  gtk_signal_connect (GTK_OBJECT (main_window), "destroy",
		      GTK_SIGNAL_FUNC (gtk_exit), NULL);

  gtk_widget_realize (main_window);

#if GTK_MAJOR_VERSION < 2
  toolbar = gtk_toolbar_new (GTK_ORIENTATION_HORIZONTAL, GTK_TOOLBAR_ICONS);
  gtk_toolbar_set_button_relief (GTK_TOOLBAR (toolbar), GTK_RELIEF_NONE);
  gtk_toolbar_set_space_style (GTK_TOOLBAR (toolbar), GTK_TOOLBAR_SPACE_LINE);
#else
  toolbar = gtk_toolbar_new ();
  gtk_toolbar_set_orientation (GTK_TOOLBAR (toolbar), GTK_ORIENTATION_HORIZONTAL);
  gtk_toolbar_set_style (GTK_TOOLBAR (toolbar), GTK_TOOLBAR_ICONS);
#endif

  p = gpe_find_icon ("new");
  pw = gpe_render_icon (main_window->style, p);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("New Appointment"), 
			   _("New Appointment"), _("New Appointment"), pw, new_appointment, NULL);

  gtk_toolbar_append_space (GTK_TOOLBAR (toolbar));

  p = gpe_find_icon ("home");
  pw = gpe_render_icon (main_window->style, p);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Today"), 
			   _("Today"), _("Today"), pw, set_today, NULL);

  gtk_toolbar_append_space (GTK_TOOLBAR (toolbar));

  p = gpe_find_icon ("future_view");
  pw = gpe_render_icon (main_window->style, p);
  future_button = gtk_toolbar_append_element (GTK_TOOLBAR (toolbar),
					      GTK_TOOLBAR_CHILD_RADIOBUTTON, NULL,
					      _("Future View"), _("Future View"), _("Future View"),
					      pw, GTK_SIGNAL_FUNC (button_toggled), future);

  p = gpe_find_icon ("day_view");
  pw = gpe_render_icon (main_window->style, p);
  day_button = gtk_toolbar_append_element (GTK_TOOLBAR (toolbar),
					   GTK_TOOLBAR_CHILD_RADIOBUTTON, future_button,
					   ("Day View"), _("Day View"), _("Day View"),
					   pw, GTK_SIGNAL_FUNC (button_toggled), day);
  
  p = gpe_find_icon ("week_view");
  pw = gpe_render_icon (main_window->style, p);
  week_button = gtk_toolbar_append_element (GTK_TOOLBAR (toolbar),
					    GTK_TOOLBAR_CHILD_RADIOBUTTON, day_button,
					    _("Week View"), _("Week View"), _("Week View"),
					    pw, GTK_SIGNAL_FUNC (button_toggled), week);

  p = gpe_find_icon ("month_view");
  pw = gpe_render_icon (main_window->style, p);
  month_button = gtk_toolbar_append_element (GTK_TOOLBAR (toolbar),
					     GTK_TOOLBAR_CHILD_RADIOBUTTON, week_button,
					     _("Month View"), _("Month View"), _("Month View"),
					     pw, GTK_SIGNAL_FUNC (button_toggled), month);

  p = gpe_find_icon ("year_view");
  pw = gpe_render_icon (main_window->style, p);
  year_button = gtk_toolbar_append_element (GTK_TOOLBAR (toolbar),
					    GTK_TOOLBAR_CHILD_RADIOBUTTON, month_button,
					    _("Year View"), _("Year View"), _("Year View"),
					    pw, GTK_SIGNAL_FUNC (button_toggled), year);

  gtk_toolbar_append_space (GTK_TOOLBAR (toolbar));
  
  p = gpe_find_icon ("exit");
  pw = gpe_render_icon (main_window->style, p);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Exit"), 
			   _("Exit"), _("Exit"), pw, GTK_SIGNAL_FUNC (gtk_exit), NULL);

  gtk_box_pack_start (GTK_BOX (vbox), toolbar, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), day, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), week, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), month, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), future, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), year, TRUE, TRUE, 0);

  gtk_container_add (GTK_CONTAINER (main_window), vbox);

  gtk_widget_set_usize (GTK_WIDGET (main_window), window_x, window_y);

  if (gpe_find_icon_pixmap ("icon", &pmap, &bmap))
    gdk_window_set_icon (main_window->window, NULL, pmap, bmap);

  gtk_widget_show (main_window);
  gtk_widget_show (vbox);
  gtk_widget_show (toolbar);

  set_day_view ();

  gtk_main ();

  return 0;
}


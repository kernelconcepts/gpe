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
#include <string.h>
#include <time.h>

#include <gtk/gtk.h>
#include <glib.h>

#include "event-db.h"
#include "event-ui.h"
#include "gtkdatesel.h"
#include "globals.h"
#include "month_view.h"

static GSList *strings;
static GtkWidget *day_list;
static GtkWidget *datesel;

static GtkStyle *light_style, *dark_style, *time_style;
static GdkColor light_color, dark_color, time_color;

static char *
format_event (event_t ev)
{
  char buf[256];
  char *p = buf;
  size_t l = sizeof (buf);
  size_t r;
  struct tm tm;
  time_t t;
  event_details_t evd;

  localtime_r (&ev->start, &tm);
  r = strftime (p, l, "%R-", &tm);
  if (r == 0) return NULL;
  p += r;
  l -= r;

  t = ev->start + ev->duration;
  localtime_r (&t, &tm);
  r = strftime (p, l, "%R ", &tm);
  if (r == 0) return NULL;
  p += r;
  l -= r;

  evd = event_db_get_details (ev);
  if (evd == NULL) return NULL;
  strncpy (p, evd->summary, l - 1);
  p[l - 1] = 0;
  
  return g_strdup (buf);
}

static void 
selection_made( GtkWidget      *clist,
		gint            row,
		gint            column,
		GdkEventButton *event,
		GtkWidget      *widget)
{
  event_t ev;
    
  if (event->type == GDK_2BUTTON_PRESS)
    {
      struct tm tm;
      
      ev = gtk_clist_get_row_data (GTK_CLIST (clist), row);

      if (ev) 
	{
	  gtk_widget_show (edit_event (ev));
	}
      else 
	{
	  char *t;
	  gtk_clist_get_text (GTK_CLIST (clist), row, 0, &t);
	  localtime_r (&viewtime, &tm);
	  strptime (t, MONTHTIMEFMT, &tm);
	  viewtime = mktime (&tm);
	  set_day_view ();
	}
    }
}

static gint
month_view_update ()
{
  guint day, row = 0;
  time_t start, end;
  struct tm tm_start, tm_end;
  char buf[10];
  gchar *line_info[2];
  GSList *day_events[32];
  guint width = 0, i;
  GSList *iter;
  static gboolean width_set;
  guint days;

  gtk_date_sel_set_time (GTK_DATE_SEL (datesel), viewtime);
      
  if (! light_style)
    {
      guint j;
      light_color.red = 60000;
      light_color.green = 60000;
      light_color.blue = 60000;
  
      time_color.red = 20000;
      time_color.green = 20000;
      time_color.blue = 20000;
  
      dark_color.red = 45000;
      dark_color.green = 45000;
      dark_color.blue = 45000;

      light_style = gtk_style_copy (gtk_widget_get_style (day_list));
      dark_style = gtk_style_copy (gtk_widget_get_style (day_list));
      time_style = gtk_style_copy (gtk_widget_get_style (day_list));
      time_style->font = datefont;
 
      for (j = 0; j < 5; j++) 
	{
	  light_style->base[j] = light_color;
	  dark_style->base[j] = dark_color;
	  time_style->base[j] = time_color;
	  time_style->fg[j] = light_color;
	}
    }
  
  gtk_clist_freeze (GTK_CLIST (day_list));
  gtk_clist_clear (GTK_CLIST (day_list));

  if (strings)
    {
      for (iter = strings; iter; iter = iter->next)
	g_free (iter->data);
      g_slist_free (strings);
      strings = NULL;
    }
  
  localtime_r (&viewtime, &tm_start);
  days = days_in_month (tm_start.tm_year + 1900, tm_start.tm_mon);

  for (day = 1; day <= days; day++)
    {
      localtime_r (&viewtime, &tm_start);

      tm_start.tm_mday = day;
      tm_start.tm_hour = 0;
      tm_start.tm_min = 0;
      tm_start.tm_sec = 0;
      start = mktime (&tm_start);
      localtime_r (&viewtime, &tm_end);
      tm_start.tm_mday = day;
      tm_end.tm_hour = 23;
      tm_end.tm_min = 59;
      tm_end.tm_sec = 59;
      end = mktime (&tm_end);

      day_events[day] = event_db_list_for_period (start, end);
      
      for (iter = day_events[day]; iter; iter = iter->next)
	((event_t)iter->data)->mark = FALSE;
    }

  for (day = 1; day <= days; day++)
    {
      guint w;
      gchar *text = NULL;
      event_t ev = NULL;
 
      tm_start.tm_mday = day;
      tm_start.tm_hour = 0;
      tm_start.tm_min = 0;
      tm_start.tm_sec = 0;
      mktime (&tm_start);

      for (iter = day_events[day]; iter; iter = iter->next)
	{
	  ev = (event_t) iter->data;

	  if (ev->mark == FALSE)
	    {
	      ev->mark = TRUE;
	      text = format_event (ev);
	      g_slist_append (strings, text);
	      break;
	    }
	} 

      line_info[1] = text;
      strftime (buf, sizeof (buf), MONTHTIMEFMT, &tm_start);
      line_info[0] = buf;
     
      if (! width_set)
	{
	  w = gdk_string_width (time_style->font, buf);
	  if (w > width)
	    width = w;
	}

      gtk_clist_append (GTK_CLIST (day_list), line_info);

      if (ev)
	gtk_clist_set_row_data (GTK_CLIST (day_list), row, ev);
      
      gtk_clist_set_cell_style (GTK_CLIST (day_list), row, 1, 
				day_events[day] ? dark_style : light_style); 
      row++;

      for (; iter; iter = iter->next)
	{
	  ev = (event_t) iter->data;

	  if (ev->mark)
	    continue;

	  ev->mark = TRUE;

	  line_info[0] = NULL;
	  line_info[1] = format_event (ev);
	  g_slist_append (strings, line_info[1]);

          gtk_clist_append (GTK_CLIST (day_list), line_info);
	  gtk_clist_set_row_data (GTK_CLIST (day_list), row, ev);
	
	  gtk_clist_set_cell_style (GTK_CLIST (day_list), row, 1, dark_style);
	  row++;
       } 
    }

  for (day = 1; day <= days; day++)
    event_db_list_destroy (day_events[day]);

  for (i = 0; i < row; i++)
    gtk_clist_set_cell_style (GTK_CLIST (day_list), i, 0, time_style);

  if (! width_set)
    {
      width_set = TRUE;
      gtk_clist_set_column_width (GTK_CLIST (day_list), 0, width + 4);
    }

  gtk_clist_thaw (GTK_CLIST (day_list));
  
  return TRUE;
}

static void
changed_callback(GtkWidget *widget,
		 GtkWidget *clist)
{
  viewtime = gtk_date_sel_get_time (GTK_DATE_SEL (widget));
  month_view_update ();
}

GtkWidget *
month_view(void)
{
  GtkWidget *vbox = gtk_vbox_new (FALSE, 0);
  GtkWidget *scrolled_window = gtk_scrolled_window_new (NULL, NULL);

  datesel = gtk_date_sel_new (GTKDATESEL_MONTH);
  
  day_list = gtk_clist_new (2);
  
  gtk_signal_connect (GTK_OBJECT (day_list), "select_row",
                       GTK_SIGNAL_FUNC (selection_made),
                       NULL);

  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
		GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  
  gtk_container_add (GTK_CONTAINER (scrolled_window), day_list);
  
  gtk_box_pack_start (GTK_BOX (vbox), scrolled_window, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), datesel, FALSE, FALSE, 0);

  gtk_signal_connect(GTK_OBJECT (datesel), "changed",
		     GTK_SIGNAL_FUNC (changed_callback), day_list);
  
  gtk_object_set_data (GTK_OBJECT (vbox), "update_hook", 
		       (gpointer) month_view_update);

  return vbox;
}

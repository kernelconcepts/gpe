/*
 * Copyright (C) 2001, 2002, 2004, 2005, 2006 Philip Blundell <philb@gnu.org>
 * Copyright (C) 2006 Neal H. Walfield <neal@walfield.org>
 * Copyright (C) 2004 Luca De Cicco <ldecicco@gmx.net> 
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <string.h>
#include <time.h>
#include <locale.h>
#include <langinfo.h>

#include <gpe/pixmaps.h>
#include <gtk/gtk.h>
#include <glib.h>
#include <gdk/gdkkeysyms.h>

#include <gpe/event-db.h>
#include <gpe/spacing.h>
#include "view.h"
#include "event-ui.h"
#include "globals.h"
#include "day_view.h"
#include "calendars-widgets.h"
#include "event-menu.h"

// #define DEBUG
#ifdef DEBUG
#define D(x) x
#else
#define D(x) do { } while (0)
#endif

struct event_rect
{
  int width;
  int height;
  int x;
  int y;
  Event *event;
  GdkGC *bgcolor_gc;
};

static struct event_rect *
event_rect_new (Event *ev, int x, int width, int y, int height)
{
  struct event_rect *er = g_malloc (sizeof (struct event_rect));

  er->event = ev;

  er->x = x;
  er->width = width;
  er->y = y;
  er->height = height;

  /* The background color will be initialized in the expose
     routine.  */
  er->bgcolor_gc = NULL;

  return er;
}

#define ARC_SIZE 7

static void
event_rect_expose (struct event_rect *er, GtkDrawingArea *area,
		   time_t period_start, time_t period_end)
{
  GdkDrawable *w = GTK_WIDGET (area)->window;
  GtkStyle *style = GTK_WIDGET (area)->style;

  int x = er->x + 1;
  int width = er->width - 2;
  int y = er->y + 1;
  int height = er->height - 2;

  if (! er->bgcolor_gc)
    /* Get the background color.  */
    {
      GdkColor bgcolor;
      EventCalendar *ec = event_get_calendar (er->event);
      if (event_calendar_get_color (ec, &bgcolor))
	er->bgcolor_gc = pen_new (GTK_WIDGET (area),
				  bgcolor.red, bgcolor.green, bgcolor.blue);
      else
	/* Light butter.  */
	er->bgcolor_gc = pen_new (GTK_WIDGET (area),
				  0xfc00, 0xe900, 0x4f00);
      g_object_unref (ec);
    }

  /* "Flood" the rectangle.  */
  if (height > 2 * ARC_SIZE)
    gdk_draw_rectangle (w, er->bgcolor_gc, TRUE,
			x, y + ARC_SIZE,
			width, height - 2 * ARC_SIZE);
  if (width > 2 * ARC_SIZE)
    gdk_draw_rectangle (w, er->bgcolor_gc, TRUE,
			x + ARC_SIZE, y,
			width - 2 * ARC_SIZE, height);

  /* Draw the outline.  */

  /* Top.  */
  gdk_draw_line (w, style->black_gc,
		 x + ARC_SIZE, y,
		 x + width - 1 - ARC_SIZE, y);
  /* Bottom.  */
  gdk_draw_line (w, style->black_gc,
		 x + ARC_SIZE, y + height - 1,
		 x + width - 1 - ARC_SIZE, y + height - 1);
  /* Left.  */
  gdk_draw_line (w, style->black_gc,
		 x, ARC_SIZE + y, x,
		 y + height - 1 - ARC_SIZE);
  /* Right.  */
  gdk_draw_line (w, style->black_gc,
		 x + width - 1, ARC_SIZE + y,
		 x + width - 1, y + height - 1 - ARC_SIZE);

  /* Draw the corners.  */

  /* North-west corner */
  gdk_draw_arc (w, er->bgcolor_gc, TRUE,
		x, y, ARC_SIZE * 2, ARC_SIZE * 2, 90 * 64, 90 * 64);
  gdk_draw_arc (w, style->black_gc, FALSE,
		x, y, ARC_SIZE * 2, ARC_SIZE * 2, 90 * 64, 90 * 64);
  /* North-east corner */
  gdk_draw_arc (w, er->bgcolor_gc, TRUE,
		x + width - 1 - 2 * ARC_SIZE, y,
		ARC_SIZE * 2, ARC_SIZE * 2, 0 * 64, 90 * 64);
  gdk_draw_arc (w, style->black_gc, FALSE,
		x + width - 1 - ARC_SIZE * 2, y,
		ARC_SIZE * 2, ARC_SIZE * 2, 0 * 64, 90 * 64);
  /* South-west corner */
  gdk_draw_arc (w, er->bgcolor_gc, TRUE,
		x, y + height - 1 - 2 * ARC_SIZE,
		ARC_SIZE * 2, ARC_SIZE * 2, 180 * 64, 90 * 64);
  gdk_draw_arc (w, style->black_gc, FALSE,
		x, y + height - 1 - 2 * ARC_SIZE,
		ARC_SIZE * 2, ARC_SIZE * 2, 180 * 64, 90 * 64);
  /* South-east corner */
  gdk_draw_arc (w, er->bgcolor_gc, TRUE,
		x + width - 1 - 2 * ARC_SIZE, y + height - 1 - 2 * ARC_SIZE,
		ARC_SIZE * 2, ARC_SIZE * 2, 270 * 64, 90 * 64);
  gdk_draw_arc (w, style->black_gc, FALSE,
		x + width - 1 - 2 * ARC_SIZE, y + height - 1 - 2 * ARC_SIZE,
		ARC_SIZE * 2, ARC_SIZE * 2, 270 * 64, 90 * 64);	

  /* Write summary... */
  char *summary = event_get_summary (er->event);
  char *location = event_get_location (er->event);
  char *description = event_get_description (er->event);

  time_t end = event_get_start (er->event) + event_get_duration (er->event);
  struct tm tm;
  localtime_r (&end, &tm);
  char *until = NULL;
  if (! event_get_untimed (er->event))
    {
      if (end > period_end)
	until = strftime_strdup_utf8_locale (_("Until %b %d"), &tm);
      else
	until = g_strdup_printf (_("Until %d:%02d"), tm.tm_hour, tm.tm_min);
    }

  char *text (const char *loc_desc_sep)
    {
      return
	g_strdup_printf ("<b>%s</b>%s%s%s%s%s%s",
			 summary,
			 (summary && summary[0]
			  && ((location && location[0])
			      || (description && description[0])))
			 ? ": " : "",
			 location ? location : "",
			 location && location[0] ? loc_desc_sep : "",
			 description ? description : "",
			 until && until[0] ? loc_desc_sep : "",
			 until ? until : "");
    }

  gchar *buffer = text ("\n");

  GdkRectangle gr;
  gr.width = width - 4;
  gr.height = height - 2;
  gr.x = x + 2;

  PangoLayout *pl
    = gtk_widget_create_pango_layout (GTK_WIDGET (area), NULL);
  pango_layout_set_width (pl, PANGO_SCALE * gr.width);
  pango_layout_set_markup (pl, buffer, -1);

  PangoRectangle pr;
  pango_layout_get_pixel_extents (pl, NULL, &pr);
  if (pr.height >= gr.height)
    /* Text doesn't fit, try removing "\n" between the location
       and the description.  */
    {
      g_free (buffer);
      buffer = text ("; ");
      pango_layout_set_markup (pl, buffer, -1);
      pango_layout_get_pixel_extents (pl, NULL, &pr);
    }
  if (pr.height >= gr.height)
    /* Try replaing all "\n"'s with spaces.  */
    {
      char *p;
      for (p = strchr (buffer, '\n'); p; p = strchr (p, '\n'))
	*p = ' ';
      pango_layout_set_markup (pl, buffer, -1);
      pango_layout_get_pixel_extents (pl, NULL, &pr);
    }

  if (pr.height >= gr.height)
    /* Still doesn't fit, simply cut off the bottom of the
       text.  */
    gr.y = y + 1;
  else
    /* Vertically center the text.  */
    gr.y = (y + 1) + (gr.height - pr.height) / 2;

  g_free (buffer);
  g_free (until);
  g_free (summary);
  g_free (description);
  g_free (location);

  gtk_paint_layout (style, w,
		    GTK_WIDGET_STATE (area),
		    FALSE, &gr, GTK_WIDGET (area), "label",
		    gr.x, gr.y, pl);

  if (event_get_alarm (er->event))
    {
      static GdkPixbuf *bell_pb;

      if (! bell_pb)
	/* Load the icon.  */
	bell_pb = gpe_find_icon ("bell");
      if (bell_pb)
	{
	  gint width_pix, height_pix;
	  width_pix = gdk_pixbuf_get_width (bell_pb);
	  height_pix = gdk_pixbuf_get_height (bell_pb);
    
	  gdk_draw_pixbuf (w,
			   er->bgcolor_gc,
			   bell_pb,
			   0, 0,
			   x + width - width_pix - 1,
			   y + 1, MIN (width_pix, width - 1),
			   MIN (height_pix, height - 1),
			   GDK_RGB_DITHER_NORMAL, 0, 0);
	}
    }
}

static void
event_rect_delete (struct event_rect *ev)
{
  if (ev->bgcolor_gc)
    g_object_unref (ev->bgcolor_gc);
  g_free (ev);
}

static void
event_rect_list_free (GSList *list)
{
  GSList *er;
  for (er = list; er; er = er->next)
    event_rect_delete ((struct event_rect *) er->data);
  g_slist_free (list);
}

#define NUM_HOURS 30

/* First row which must be displayed.  */
#define ROWS_HARD_FIRST 7
/* Number of rows which must be displayed (starting with
   ROWS_HARD_FIRST).  */
#define ROWS_HARD 12

struct _DayView
{
  GtkView view;

  /*
      Basic layout of a DayView:

       /-DayView----------------\
       | /-reminder_area------\ |
       | |                    | |
       | \--------------------/ |
       | /-appointment_window-\ |
       | |/-appointment_area-\| |
       | ||                  || |
       | |\------------------/| |
       | \--------------------/ |
       \------------------------/
   */

  /* List of events.  */
  GSList *appointments;
  GSList *reminders;

  /* List of event_recentangles.  */
  GSList *appointment_rects;
  GSList *reminder_rects;

  /* The start of the time period we are displaying.  */
  time_t period_start;
  guint duration;
  guint rows;

  /* The height and width of the display window (i.e. what is
     visible).  */
  gint visible_width;
  gint visible_height;

  /* The actual canvas size to use.  */
  gint height;
  gint width;

  GdkGC *hour_bg_gc;
  GdkGC *events_bg_gc;
  /* Color for the Marcus Bain's bar.  */
  GdkGC *time_gc;

  /* Given the current set of events, the first row to display and the
     number of rows.  */
  time_t visible_start;
  gint visible_duration;

  /* Width of the time column in pixels.  */
  gint time_width;
  /* The minimum height of a row in pixels.  */
  gint row_height_min;

  /* The reminder drawing area (if present).  */
  GtkDrawingArea *reminder_area;

  /* When we draw the hour bar, we'd like to update it every 10
     minutes.  */
  guint hour_bar_repaint;
  /* Center of the hour_bar.  */
  int hour_bar_pos;

  /* The appointment drawing area (if present).  */
  GtkWidget *appointment_window;
  GtkDrawingArea *appointment_area;

  /* If the events need to be reloaded.  */
  gboolean pending_reload;
  /* If update_extents needs to be called.  */
  gboolean pending_update_extents;
};

typedef struct
{
  GtkViewClass view_class;
} DayViewClass;

static void day_view_base_class_init (gpointer klass, gpointer klass_data);
static void day_view_init (GTypeInstance *instance, gpointer klass);
static void day_view_dispose (GObject *obj);
static void day_view_finalize (GObject *object);
static void day_view_set_time (GtkView *view, time_t time);
static void day_view_reload_events (GtkView *view);
static gboolean day_view_key_press_event (GtkWidget *widget,
					  GdkEventKey *event);

static void size_request (GtkWidget *widget, GtkRequisition *requisition,
			  DayView *day_view);
static gboolean configure_event (GtkWidget *widget, GdkEventConfigure *event,
				 DayView *day_view);
static gboolean button_press_event (GtkWidget *widget,
				    GdkEventButton *event,
				    DayView *day_view);
static gboolean appointment_area_expose_event (GtkWidget *widget,
					       GdkEventExpose *event,
					       DayView *day_view);
static gboolean reminder_area_expose_event (GtkWidget *widget,
					    GdkEventExpose *event,
					    DayView *day_view);

static void reload_events_hard (DayView *day_view);

static GtkWidgetClass *parent_class;

GType
day_view_get_type (void)
{
  static GType type;

  if (! type)
    {
      static const GTypeInfo info =
      {
	sizeof (DayViewClass),
	NULL,
	NULL,
	day_view_base_class_init,
	NULL,
	NULL,
	sizeof (struct _DayView),
	0,
	day_view_init
      };

      type = g_type_register_static (gtk_view_get_type (),
				     "DayView", &info, 0);
    }

  return type;
}

static void
day_view_base_class_init (gpointer klass, gpointer klass_data)
{
  GObjectClass *object_class;
  GtkWidgetClass *widget_class;
  GtkViewClass *view_class;

  parent_class = g_type_class_ref (gtk_view_get_type ());

  object_class = G_OBJECT_CLASS (klass);
  object_class->finalize = day_view_finalize;
  object_class->dispose = day_view_dispose;

  widget_class = (GtkWidgetClass *) klass;
  widget_class->key_press_event = day_view_key_press_event;

  view_class = (GtkViewClass *) klass;
  view_class->set_time = day_view_set_time;
  view_class->reload_events = day_view_reload_events;
}

static void
day_view_init (GTypeInstance *instance, gpointer klass)
{
  DayView *day_view = DAY_VIEW (instance);

  GTK_WIDGET_SET_FLAGS (GTK_WIDGET (day_view), GTK_CAN_FOCUS);
  gtk_widget_add_events (GTK_WIDGET (day_view), GDK_KEY_PRESS_MASK);
  day_view->duration = NUM_HOURS * 60 * 60;
  day_view->rows = 30;

  day_view->appointment_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy
    (GTK_SCROLLED_WINDOW (day_view->appointment_window),
     GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_box_pack_end (GTK_BOX (day_view), day_view->appointment_window,
		    TRUE, TRUE, 0);
  gtk_widget_show (day_view->appointment_window);

  day_view->appointment_area = GTK_DRAWING_AREA (gtk_drawing_area_new ());
  gtk_widget_add_events (GTK_WIDGET (day_view->appointment_area),
			 GDK_BUTTON_PRESS_MASK);
  g_signal_connect (day_view->appointment_area, "button-press-event",
		    G_CALLBACK (button_press_event), day_view);
  g_signal_connect (day_view->appointment_area, "size-request",
		    G_CALLBACK (size_request), day_view);
  g_signal_connect (day_view->appointment_area, "configure-event",
		    G_CALLBACK (configure_event), day_view);
  g_signal_connect (day_view->appointment_area, "expose-event",
		    G_CALLBACK (appointment_area_expose_event), day_view);
  gtk_scrolled_window_add_with_viewport
    (GTK_SCROLLED_WINDOW (day_view->appointment_window),
     GTK_WIDGET (day_view->appointment_area));
  gtk_viewport_set_shadow_type
    (GTK_VIEWPORT (GTK_WIDGET (day_view->appointment_area)->parent),
     GTK_SHADOW_NONE);
  gtk_widget_show (GTK_WIDGET (day_view->appointment_area));

  day_view->pending_update_extents = TRUE;
}

static void
day_view_dispose (GObject *obj)
{
  /* Chain up to the parent class.  */
  G_OBJECT_CLASS (parent_class)->dispose (obj);
}

static void
day_view_finalize (GObject *object)
{
  DayView *day_view = DAY_VIEW (object);

  if (day_view->hour_bg_gc)
    g_object_unref (day_view->hour_bg_gc);
  if (day_view->events_bg_gc)
    g_object_unref (day_view->events_bg_gc);
  if (day_view->time_gc)
    g_object_unref (day_view->time_gc);

  if (day_view->hour_bar_repaint > 0)
    /* Cancel any outstanding timeout.  */
    g_source_remove (day_view->hour_bar_repaint);

  event_rect_list_free (day_view->appointment_rects);
  event_rect_list_free (day_view->reminder_rects);

  event_list_unref (day_view->appointments);
  event_list_unref (day_view->reminders);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void hour_bar_calc (DayView *day_view);

static GSList *
calc_events_positions (DayView *day_view, GSList *events)
{
  /* If there are no events, then we are done.  */
  if (! events)
    return NULL;

  /* Return TRUE if A and B overlap, FALSE otherwise.  */
  gboolean overlap (Event *a, Event *b)
    {
      if (event_get_start (a) <= event_get_start (b)
	  && (event_get_start (b)
	      < event_get_start (a) + event_get_duration (a)))
	/* Start of B occurs during A.  */
	return TRUE;
      if (event_get_start (b) <= event_get_start (a)
	  && (event_get_start (a)
	      < event_get_start (b) + event_get_duration (b)))
	/* Start of A occurs during B.  */
	return TRUE;
      if (event_get_start (b) < event_get_start (a) + event_get_duration (a)
	  && (event_get_start (a) + event_get_duration (a)
	      <= event_get_start (b) + event_get_duration (b)))
	/* End of A occurs during B.  */
	return TRUE;
      /* We don't need to check if the end of B occurs during A as
	 this cannot occur without one of the above conditions also
	 occuring.  */
      return FALSE;
    }

  /* Calculate the number of other events with which each event
     directly overlaps.  */
  int n = g_slist_length (events);
  struct info
  {
    Event *event;
    int direct_overlap;
  };
  struct info info[n];
  memset (info, 0, sizeof (info));

  int i, j;
  GSList *a, *b;
  for (a = events, i = 0; a; a = a->next)
    {
      if (! event_get_visible (a->data))
	continue;

      info[i].event = a->data;

      for (b = a->next, j = i + 1; b; b = b->next, j ++)
	if (overlap (a->data, b->data))
	  {
	    info[i].direct_overlap ++;
	    info[j].direct_overlap ++;
	  }

      i ++;
    }
  n = i;

  /* A and B overlap.  Returns TRUE if A dominates B.  An event
     dominates another if it directly overlaps with more events than
     the other or, if the direct overlap is equivalent and it occurs
     earlier.  */
  gboolean dominates (struct info *a, struct info *b)
    {
      if (a->direct_overlap > b->direct_overlap)
	return TRUE;
      if (b->direct_overlap > a->direct_overlap)
	return FALSE;
      return event_compare_func (a->event, b->event) < 0;
    }

  /* Given node N, insert it into the tree.  */
  void insert (GNode *root, GNode *node)
    {
      struct info *n = node->data;
      GNode *child;

      D (char *s = event_get_summary (n->event);
	 printf ("%s:%d: inserting %s\n", __func__, __LINE__, s);
	 g_free (s));

      child = root->children;
      while (child)
	{
	  struct info *c = child->data;

	  if (overlap (c->event, n->event))
	    /* CHILD and NODE overlap.  */
	    {
	      D (char *s1 = event_get_summary (n->event);
		 char *s2 = event_get_summary (c->event);
		 printf ("%s:%d: %.10s overlaps with %.10s\n",
			 __func__, __LINE__, s1, s2);
		 g_free (s1);
		 g_free (s2));

	      if (dominates (n, c))
		/* N dominates C (and transitively, everything which C
		   dominates is also dominated by N).  Replace C with
		   N and proceed to insert C under N.  If may also be
		   the case that N dominates some of the events
		   following C.  Consider those.  One of those may in
		   turn dominate C.  */
		{
		  g_node_insert_before (root, child, node);

		  GNode *next;
		  do
		    {
		      D (char *s1 = event_get_summary (n->event);
			 char *s2 = event_get_summary (c->event);
			 printf ("%s:%d: %s (%d) dominates %s (%d)\n",
				 __func__, __LINE__,
				 s1, n->direct_overlap,
				 s2, c->direct_overlap);
			 g_free (s1);
			 g_free (s2));

		      next = child->next;
		      g_node_unlink (child);
		      g_node_append (node, child);

		      if (! next)
			break;

		      child = next;
		      c = child->data;
		    }
		  while (overlap (c->event, n->event)
			 && dominates (n, c));

		  if (next && overlap (c->event, n->event) && dominates (c, n))
		    {
		      D (char *s = event_get_summary (c->event);
			 printf ("%s:%d: and is dominated by %.10s (%d)\n",
				 __func__, __LINE__,
				 s, c->direct_overlap);
			 g_free (s));
		      g_node_unlink (node);
		      g_node_prepend (child, node);
		    }
		  break;
		}
	      else
		{
		  D (char *s = event_get_summary (c->event);
		     printf (" continuing with %.10s as root\n", s);
		     g_free (s));

		  /* C dominates N.  N belongs under C.  */
		  root = child;
		  child = child->children;
		}
	    }
	  else if (event_compare_func (n->event, c->event) < 0)
	    {
	      D (char *s1 = event_get_summary (n->event);
		 char *s2 = event_get_summary (c->event);
		 printf ("%s:%d: %.10s occurs before %.10s\n",
			 __func__, __LINE__, s1, s2);
		 g_free (s1);
		 g_free (s2));
	      g_node_insert_before (root, child, node);
	      break;
	    }
	  else if (! child->next)
	    {
	      /* CHILD ends before NODE but a child of CHILD could
		 overlap with NODE.  */
	      child = child->children;
	    }
	  else
	    child = child->next;
	}

      if (! child)
	/* Occurs after any child.  */
	{
	  D (char *s1 = event_get_summary (n->event);
	     char *s2 = root->data
	       ? event_get_summary (((struct info *) root->data)->event)
	       : "ROOT";
	     printf ("%s:%d: %.10s occurs after any child of %.10s\n",
		     __func__, __LINE__, s1, s2);
	     g_free (s1);
	     if (root->data)
	       g_free (s2));
	  g_node_append (root, node);
	}
    }

  GNode *root = g_node_new (NULL);
  for (i = 0; i < n; i ++)
    insert (root, g_node_new (&info[i]));

  GSList *list = NULL;

  void traverse (GNode *node, gfloat left)
    {
      if (node->data)
	{
	  struct info *info = node->data;
	  int cols = g_node_max_height (node);
	  gfloat width = left / cols;

	  D (char *s = event_get_summary (info->event);
	     printf ("Event %.10s at depth=%d, x=%g, width=%g\n",
		     s, g_node_depth (node), 1 - left, width);
	     g_free (s));

	  Event *ev = EVENT (info->event);

	  int y;
	  if (event_get_start (ev) < day_view->period_start)
	    /* Event starts prior to DAY_VIEW's start.  */
	    y = 0;
	  else
	    y = day_view->height
	      * (event_get_start (ev) - day_view->visible_start)
	      / day_view->visible_duration;

	  int h;
	  if (event_get_start (ev) + event_get_duration (ev)
	      < day_view->visible_start + day_view->visible_duration)
	    /* Event ends prior to DAY_VIEW's end.  */
	    h = day_view->height
	      * (event_get_start (ev) + event_get_duration (ev)
		 - day_view->visible_start)
	      / day_view->visible_duration - y;
	  else
	    /* Event ends after the end of the period.  */
	    h = day_view->height - y;
	  /* We don't use CLAMP as the "max" might be less than the
	     "min".  */
	  h = MIN (MAX (h, day_view->row_height_min), day_view->height - y);

	  int x = day_view->time_width
	    + (day_view->width - day_view->time_width) * (1 - left);
	  int w = (day_view->width - day_view->time_width) * width;

	  struct event_rect *er = event_rect_new (ev, x, w, y, h);
	  list = g_slist_prepend (list, er);

	  left -= width;
	}

      GNode *i;
      for (i = node->children; i; i = i->next)
	traverse (i, left);
    }

  traverse (root, 1);

  g_node_destroy (root);

  return list;
}

static void
time_layout (PangoLayout *pl, int hour)
{
  struct tm tm;
  memset (&tm, 0, sizeof (tm));
  tm.tm_hour = hour;

  char *am = nl_langinfo (AM_STR);
  char timebuf[10];
  strftime (timebuf, sizeof (timebuf),
	    am && *am ? "%I%P" : "%H", &tm);
  char buf[60];
  snprintf (buf, sizeof (buf), "<span font_desc='normal'>%s</span>",
	    *timebuf == '0' ? &timebuf[1] : timebuf);
  char *buffer = g_locale_to_utf8 (buf, -1, NULL, NULL, NULL);
  pango_layout_set_markup (pl, buffer, -1);
  g_free (buffer);
}

/* Update the calculated extents of DAY_VIEW.  */
static void
update_extents (DayView *day_view)
{
  day_view->width = day_view->visible_width;
  day_view->height
    = MAX (day_view->visible_height,
	   day_view->visible_duration / 60 / 60 * day_view->row_height_min);

  /* Calculate DAY_VIEW->TIME_WIDTH.  */
  PangoLayout *pl
    = gtk_widget_create_pango_layout (GTK_WIDGET (day_view), NULL);
  day_view->time_width = 0;
  int hour = (day_view->visible_start - day_view->period_start) / 60 / 60;
  int i;
  for (i = 0; i < day_view->visible_duration / 60 / 60; i ++)
    {
      time_layout (pl, hour + i);

      PangoRectangle pr;
      pango_layout_get_pixel_extents (pl, NULL, &pr);

      day_view->time_width = MAX (day_view->time_width, pr.width + 2);
    }
  g_object_unref (pl);

  hour_bar_calc (day_view);

  /* Recalculate the position of the events.  */
  event_rect_list_free (day_view->appointment_rects);
  day_view->appointment_rects
    = calc_events_positions (day_view, day_view->appointments);

  event_rect_list_free (day_view->reminder_rects);
  day_view->reminder_rects = NULL;
  int n = g_slist_length (day_view->reminders);
  GSList *l;
  for (i = 0, l = day_view->reminders; i < n; i ++, l = l->next)
    day_view->reminder_rects =
      g_slist_prepend (day_view->reminder_rects,
		       event_rect_new (EVENT (l->data),
				       i * day_view->width / n,
				       day_view->width / n,
				       0, day_view->row_height_min));

  day_view->pending_update_extents = FALSE;
}

static void
size_request (GtkWidget *widget, GtkRequisition *requisition,
	      DayView *day_view)
{
  PangoLayout *pl = gtk_widget_create_pango_layout (widget, NULL);
  PangoContext *context = pango_layout_get_context (pl);
  PangoFontMetrics *metrics
    = pango_context_get_metrics (context,
				 widget->style->font_desc,
				 pango_context_get_language (context));

  int min = (pango_font_metrics_get_ascent (metrics)
	     + pango_font_metrics_get_descent (metrics)) / PANGO_SCALE
    + 4 /* For border.  */;
  if (min != day_view->row_height_min)
    {
      day_view->row_height_min = min;
      day_view->pending_update_extents = TRUE;
    }

  requisition->height
    = MAX (requisition->height, ROWS_HARD * day_view->row_height_min);

  requisition->width
    = MAX (requisition->width,
	   pango_font_metrics_get_approximate_char_width (metrics) * 24
	   / PANGO_SCALE);

  g_object_unref (pl);
  pango_font_metrics_unref (metrics);
}

static gboolean
configure_event (GtkWidget *widget, GdkEventConfigure *event,
		 DayView *day_view)
{
  gboolean update = FALSE;

  if (day_view->visible_width != event->width)
    {
      day_view->visible_width = event->width;
      update = TRUE;
    }

  if (day_view->visible_height != event->height)
    {
      day_view->visible_height = event->height;
      update = TRUE;
    }

  if (update)
    day_view->pending_update_extents = TRUE;

  return FALSE;
}

#define HOUR_BAR_HEIGHT(dv) \
  ((dv)->visible_height / ((dv)->visible_duration / 60 / 60))

static gboolean hour_bar_move (gpointer d);

/* Calculate the position of the hour bar.  */
static void
hour_bar_calc (DayView *day_view)
{
  time_t now = time (NULL);

  if (day_view->visible_start <= now
      && now <= day_view->visible_start + day_view->visible_duration)
    {
      day_view->hour_bar_pos
	= day_view->height * (now - day_view->visible_start)
	/ day_view->visible_duration;

      if (! day_view->hour_bar_repaint)
	/* Repaint approximately every 10 minutes.  */
	day_view->hour_bar_repaint
	  = g_timeout_add (10 * 60 * 1000, hour_bar_move, day_view);
    }
  else
    day_view->hour_bar_pos = -1;
}

/* Callback to move the hour bar periodically.  */
static gboolean
hour_bar_move (gpointer d)
{
  DayView *day_view = DAY_VIEW (d);

  int top = day_view->hour_bar_pos - HOUR_BAR_HEIGHT (day_view) / 2;
  int bottom = day_view->hour_bar_pos + HOUR_BAR_HEIGHT (day_view) / 2;

  gtk_widget_queue_draw_area (GTK_WIDGET (day_view),
			      0, top,
			      day_view->visible_width, bottom - top + 1);

  hour_bar_calc (day_view);

  if (day_view->hour_bar_pos >= 0)
    /* Still visible.  */
    {
      top = day_view->hour_bar_pos - HOUR_BAR_HEIGHT (day_view) / 2;
      bottom = day_view->hour_bar_pos + HOUR_BAR_HEIGHT (day_view) / 2;

      gtk_widget_queue_draw_area (GTK_WIDGET (day_view),
				  0, top,
				  day_view->visible_width, bottom - top + 1);

      return TRUE;
    }
  else
    /* The hour bar is no longer visible: don't recalculate.  */
    {
      day_view->hour_bar_repaint = 0;
      return FALSE;
    }
}

static gboolean
appointment_area_expose_event (GtkWidget *widget, GdkEventExpose *event,
			       DayView *day_view)
{
  if (event->count > 0)
    return FALSE;

  if (day_view->pending_reload)
    reload_events_hard (day_view);
  if (day_view->pending_update_extents)
    update_extents (day_view);

  if (! day_view->hour_bg_gc)
    /* Light aluminium.  */
    day_view->hour_bg_gc = pen_new (widget, 0xd300, 0xd700, 0xcf00);

  /* Draw the hour column's background.  */
  gdk_draw_rectangle (widget->window, day_view->hour_bg_gc, TRUE,
		      0, 0,
		      day_view->width - 1,
		      day_view->height - 1);

  if (day_view->hour_bar_pos >= 0)
    /* Paint the hour bar.  */
    {
      if (! day_view->time_gc)
	/* Chocolate.  */
	day_view->time_gc = pen_new (widget, 0xe900, 0xb900, 0x6e00);

      gdk_draw_rectangle (widget->window, day_view->time_gc, TRUE,
			  0,
			  day_view->hour_bar_pos
			  - HOUR_BAR_HEIGHT (day_view) / 2,
			  day_view->visible_width - 1,
			  HOUR_BAR_HEIGHT (day_view) / 2);
    }

  PangoLayout *pl = gtk_widget_create_pango_layout (widget, NULL);
  pango_layout_set_alignment (pl, PANGO_ALIGN_CENTER);
  pango_layout_set_wrap (pl, PANGO_WRAP_WORD_CHAR);

  int hour = (day_view->visible_start - day_view->period_start) / 60 / 60;
  int i;
  for (i = 0; i < day_view->visible_duration / 60 / 60; i ++)
    {
      time_layout (pl, hour + i);

      PangoRectangle pr;
      pango_layout_get_pixel_extents (pl, NULL, &pr);

      GdkRectangle gr;
      gr.width = pr.width;
      gr.height = pr.height;
      gr.x = 1;
      gr.y = day_view->height * i * 60 * 60 / day_view->visible_duration;

      gtk_paint_layout (widget->style,
			widget->window,
			GTK_WIDGET_STATE (widget),
			FALSE, &gr, widget, "label", gr.x, gr.y, pl);
    }

  g_object_unref (pl);

  if (! day_view->events_bg_gc)
    /* Light aluminium.  */
    day_view->events_bg_gc = pen_new (widget, 0xee00, 0xee00, 0xec00);


  gdk_draw_rectangle (widget->window, day_view->events_bg_gc, TRUE,
		      day_view->time_width, 0,
		      day_view->visible_width - 1,
		      day_view->visible_height - 1);

  /* And now each row's background.  */
  for (i = 0; i < day_view->visible_duration; i += 60 * 60)
    {
      int top = day_view->height * i / day_view->visible_duration;

      gdk_draw_line (widget->window, day_view->events_bg_gc,
		     0, top, day_view->time_width - 1, top);

      gdk_draw_line (widget->window, day_view->hour_bg_gc,
		     day_view->time_width, top,
		     day_view->visible_width - 1, top);
    }

  if (day_view->hour_bar_pos >= 0)
    /* Draw the hour bar.  */
    gdk_draw_rectangle (widget->window, day_view->time_gc, TRUE,
			day_view->time_width,
			day_view->hour_bar_pos
			- HOUR_BAR_HEIGHT (day_view) / 2,
			day_view->visible_width - 1,
			HOUR_BAR_HEIGHT (day_view) / 2);

  /* Draw the black vertical line dividing the hour column from the
     events.  */
  gdk_draw_line (widget->window, widget->style->black_gc,
		 day_view->time_width - 1, 0,
		 day_view->time_width - 1, day_view->visible_height - 1);

  /* Now, draw the events on top.  */
  GSList *l;
  for (l = day_view->appointment_rects; l; l = l->next)
    event_rect_expose (l->data, day_view->appointment_area,
		       day_view->period_start,
		       day_view->period_start + day_view->duration);

  return FALSE;
}

static gboolean
reminder_area_expose_event (GtkWidget *widget, GdkEventExpose *event,
			    DayView *day_view)
{
  if (event->count > 0)
    return FALSE;

  if (day_view->pending_reload)
    reload_events_hard (day_view);
  if (day_view->pending_update_extents)
    update_extents (day_view);

  GSList *l;
  for (l = day_view->reminder_rects; l; l = l->next)
    event_rect_expose (l->data, day_view->reminder_area,
		       day_view->period_start,
		       day_view->period_start + day_view->duration);

  return FALSE;
}

static gboolean
button_press_event (GtkWidget *widget, GdkEventButton *event,
		    DayView *day_view)
{
  GSList *l;

  /* Find the event in which the user clicked (in any).  */
  if (widget == GTK_WIDGET (day_view->appointment_area))
    l = day_view->appointment_rects;
  else
    l = day_view->reminder_rects;

  for (; l; l = l->next)
    {
      struct event_rect *er = l->data;

      if ((er->x <= event->x && event->x < er->x + er->width)
	  && (er->y <= event->y && event->y < er->y + er->height))
	/* Click was within ER, pop up a menu.  */
	{
	  GtkMenu *event_menu = event_menu_new (er->event, TRUE);
	  gtk_menu_popup (event_menu, NULL, NULL, NULL, NULL,
			  event->button, event->time);

	  return FALSE;
	}
    }

  GtkWidget *w = new_event (day_view->visible_start
			    + day_view->visible_duration * event->y
			    / day_view->height);
  gtk_widget_show (w);

  return FALSE;
}

static gboolean
day_view_key_press_event (GtkWidget *widget, GdkEventKey *event)
{
  switch (event->keyval)
    {
    case GDK_Left:
    case GDK_Up:
    case GDK_Page_Up:
      gtk_view_set_time (GTK_VIEW (widget),
			 gtk_view_get_time (GTK_VIEW (widget))
			 - 24 * 60 * 60);
      return TRUE;
    case GDK_Right:
    case GDK_Down:
    case GDK_Page_Down:
      gtk_view_set_time (GTK_VIEW (widget),
			 gtk_view_get_time (GTK_VIEW (widget))
			 + 24 * 60 * 60);
      return TRUE;
    default:
      return GTK_WIDGET_CLASS (parent_class)->key_press_event (widget, event);
    }
}

static void
day_view_set_time (GtkView *view, time_t current)
{
  time_t new = gtk_view_get_time (view);
  struct tm c_tm;
  struct tm n_tm;

  localtime_r (&current, &c_tm);
  localtime_r (&new, &n_tm);

  if (c_tm.tm_year != n_tm.tm_year || c_tm.tm_yday != n_tm.tm_yday)
    /* Day changed.  */
    day_view_reload_events (view);
}

static void
day_view_reload_events (GtkView *view)
{
  DAY_VIEW (view)->pending_reload = TRUE;

  gtk_widget_queue_draw (GTK_WIDGET (view));
}

static void
reload_events_hard (DayView *day_view)
{
  /* Get the events for the current period.  */
  event_list_unref (day_view->appointments);
  day_view->appointments = NULL;
  event_list_unref (day_view->reminders);
  day_view->reminders = NULL;

  time_t t = gtk_view_get_time (GTK_VIEW (day_view));
  struct tm vt;
  localtime_r (&t, &vt);
  vt.tm_hour = 0;
  vt.tm_min = 0;
  vt.tm_sec = 0;
  vt.tm_isdst = -1;
  time_t start = mktime (&vt);
  day_view->period_start = start;

  time_t end = start + 30 * 60 * 60 - 1;

  time_t earliest = day_view->period_start + ROWS_HARD_FIRST * 60 * 60;
  time_t latest = day_view->period_start
    + (ROWS_HARD_FIRST + ROWS_HARD) * 60 * 60;

  GSList *events = event_db_list_for_period (event_db, start, end);
  GSList *iter;
  for (iter = events; iter; iter = iter->next)
    {
      Event *ev = EVENT (iter->data);

      if (event_get_start (ev) <= start
	  && (start + 24 * 60 * 60
	      <= event_get_start (ev) + event_get_duration (ev)))
	/* All day event or a multi-day event.  */
	day_view->reminders = g_slist_prepend (day_view->reminders, ev);
      else if (is_reminder (ev))
	/* Normal reminder.  */
	{
	  if (event_get_start (ev) < start + 24 * 60 * 60)
	    day_view->reminders = g_slist_prepend (day_view->reminders, ev);
	  else
	    /* We're not interested in reminders which start
	       tomorrow.  */
	    g_object_unref (ev);
	}
      else
	{
	  day_view->appointments
	    = g_slist_prepend (day_view->appointments, ev);

	  if (day_view->period_start <= event_get_start (ev)
	      && event_get_start (ev) < earliest)
	    earliest = event_get_start (ev);
	  if (event_get_start (ev) + MAX (event_get_duration (ev), 60 * 60)
	      <= day_view->period_start + day_view->duration)
	    latest = MAX (event_get_start (ev)
			  + MAX (event_get_duration (ev), 60 * 60),
			  latest);
	}
    }

  g_assert (day_view->period_start <= earliest);
  g_assert (latest <= day_view->period_start + day_view->duration);

  day_view->visible_start = (earliest / 60 / 60) * 60 * 60;
  day_view->visible_duration
    = (latest - day_view->visible_start + 60 * 60 - 1) / 60 / 60 * 60 * 60;

  if (! day_view->reminders && day_view->reminder_area)
    /* There are no longer any reminders, destroy its render area.  */
    {
      gtk_container_remove (GTK_CONTAINER (day_view),
			    GTK_WIDGET (day_view->reminder_area));
      day_view->reminder_area = NULL;
    }
  if (day_view->reminders && ! day_view->reminder_area)
    /* We have reminders but no render area.  Create one.  */
    {
      day_view->reminder_area = GTK_DRAWING_AREA (gtk_drawing_area_new ());
      gtk_widget_set_size_request (GTK_WIDGET (day_view->reminder_area),
				   -1, day_view->row_height_min);
      gtk_widget_add_events (GTK_WIDGET (day_view->reminder_area),
			     GDK_BUTTON_PRESS_MASK);
      g_signal_connect (day_view->reminder_area, "button-press-event",
			G_CALLBACK (button_press_event), day_view);
      g_signal_connect (day_view->reminder_area, "expose-event",
			G_CALLBACK (reminder_area_expose_event), day_view);
      gtk_box_pack_start (GTK_BOX (day_view),
			  GTK_WIDGET (day_view->reminder_area),
			  FALSE, FALSE, 0);
      gtk_widget_show (GTK_WIDGET (day_view->reminder_area));
    }

  update_extents (day_view);

  day_view->pending_reload = FALSE;
}

GtkWidget *
day_view_new (time_t time)
{
  DayView *day_view = DAY_VIEW (g_object_new (day_view_get_type (), NULL));

  gtk_view_set_time (GTK_VIEW (day_view), time);

  return GTK_WIDGET (day_view);
}

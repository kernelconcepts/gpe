/*
 * Copyright (C) 2001, 2002, 2003 Philip Blundell <philb@gnu.org>
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
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <libintl.h>

#include <gpe/init.h>
#include <gpe/pixmaps.h>
#include <gpe/render.h>
#include <gpe/errorbox.h>

#include <gpe/event-db.h>

#include "event-ui.h"
#include "globals.h"

#include "day_view.h"
#include "week_view.h"
#include "month_view.h"
#include "import-vcal.h"
#include "gtkdatesel.h"

#include <libdisplaymigration/displaymigration.h>
#include <gpe/pim-categories.h>

#include <locale.h>

#define _(_x) gettext (_x)

extern gboolean gpe_calendar_start_xsettings (void);

GList *times;
time_t viewtime;
gboolean force_today = FALSE;
gboolean just_new = FALSE;

GtkWidget *main_window, *pop_window;
GtkWidget *notebook;
extern GtkWidget* day_list;

struct gpe_icon my_icons[] = {
  { "day_view", "day_view" },
  { "week_view", "week_view" },
  { "month_view", "month_view" },
  { "bell", "bell" },
  { "recur", "recur" },
  { "bell_recur", "bell_recur" },
  { "icon", PREFIX "/share/pixmaps/gpe-calendar.png" },
  { NULL, NULL }
};

static GtkWidget *day, *week, *month, *current_view;
static GtkWidget *day_button, *week_button, *month_button, *today_button;

guint window_x = 240, window_y = 310;

guint week_offset = 0;
gboolean week_starts_monday = TRUE;
gboolean day_view_combined_times;
gchar collected_keys[32] = "";
gint ccpos = 0;

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

time_t
time_from_day (int year, int month, int day)
{
  struct tm tm;
  time_t selected_time;
  localtime_r (&viewtime, &tm);
  tm.tm_year = year;
  tm.tm_mon = month;
  tm.tm_mday = day;
  selected_time = mktime (&tm);
  return selected_time;
}

void
set_time_all_views(void)
{
  gpointer ds;
  
  ds = g_object_get_data (G_OBJECT (main_window), "datesel-week");
  if (ds)
    gtk_date_sel_set_time(GTK_DATE_SEL (ds), viewtime);
  ds = g_object_get_data (G_OBJECT (main_window), "datesel-month");
  if (ds)
    gtk_date_sel_set_time (GTK_DATE_SEL (ds), viewtime);
  ds = g_object_get_data(G_OBJECT (main_window), "datesel-day");
  if (ds)
    gtk_date_sel_set_time (GTK_DATE_SEL (ds), viewtime);
}

void
update_view (GtkWidget *view)
{
  gpointer p = g_object_get_data (G_OBJECT (view), "update_hook");
  if (p)
    {
      void (*f)(void) = p;
      f ();
    }
}

void
update_current_view (void)
{
  if (current_view)
    update_view (current_view);
}

void
update_all_views (void)
{
  update_view (day);
  update_view (week);
  update_view (month);
}

static void
new_view (GtkWidget *widget)
{
  guint i = 0;
  GtkWidget *w;

  if (pop_window)
    gtk_widget_destroy (pop_window);

  do
    {
      w = gtk_notebook_get_nth_page (GTK_NOTEBOOK (notebook), i);
      if (w == widget)
        {
          current_view = w;
          gtk_notebook_set_page (GTK_NOTEBOOK (notebook), i);
          if (w == day) /* nasty hack to compensate missing update of clist */
            update_current_view();
          return;
        }
      i++;
    } while (w != NULL);
}

static gboolean
do_reset_new(gpointer d)
{
  just_new = FALSE;
  return FALSE;
}


static gboolean
do_insert_text (GtkWidget *window)
{
  GtkWidget *entry;
  
  entry = g_object_get_data(G_OBJECT(window), "default-entry");
  if (entry)
    {
      gtk_entry_prepend_text(GTK_ENTRY(entry), collected_keys);
      gtk_editable_set_position(GTK_EDITABLE(entry),-1);
      memset(collected_keys, 0, sizeof(gchar) * 32);
      ccpos = 0;
    }
  return FALSE;  
}


static void
new_appointment (void)
{
  GtkWidget *appt;
  
  if (just_new)
    return;
  just_new = TRUE;
  g_timeout_add(1000, do_reset_new, NULL);
  
  appt = new_event (viewtime, 0);
  g_timeout_add(500, do_insert_text, (gpointer)appt);
  gtk_widget_show (appt);
}

static void
set_today(void)
{
  static time_t selected_time;

  force_today = !force_today;
  if (force_today)
    {
      selected_time = viewtime;
      time (&viewtime);
    } 
  else
    viewtime = selected_time;
  
  set_time_all_views();
}

void
set_time_and_day_view(time_t selected_time)
{
  viewtime=selected_time;
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (day_button), TRUE);
  new_view (day);
  update_current_view();
}

static void
button_toggled (GtkWidget *widget, gpointer data)
{
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
    new_view (data);
}

static void
gpe_cal_exit (void)
{
  schedule_next (0, 0);
  event_db_stop ();
  gtk_main_quit ();
}

static int 
import_one_file(gchar *filename)
{
  GError *err = NULL;
  gchar *content;
  gsize count;
  int result = 0;
	
  if (g_file_get_contents(filename,&content,&count,&err))
    {
      result = import_vcal(content,count);
      g_free(content);
    }
  else
    result = -1;
  return result;
}

static void
on_import_vcal (GtkWidget *widget, gpointer data)
{
  GtkWidget *filesel, *feedbackdlg;
  
  filesel = gtk_file_selection_new(_("Choose file"));
  gtk_file_selection_set_select_multiple(GTK_FILE_SELECTION(filesel),TRUE);
  
  if (gtk_dialog_run(GTK_DIALOG(filesel)) == GTK_RESPONSE_OK)
    {
      gchar *errstr = NULL;
      int ec = 0, i = 0;
      gchar **files = 
        gtk_file_selection_get_selections(GTK_FILE_SELECTION(filesel));
      gtk_widget_hide(filesel); 
      while (files[i])
        {
          if (import_one_file(files[i]) < 0) 
            {
              gchar *tmp;
              if (!errstr) 
                errstr=g_strdup("");
              ec++;
			  tmp = g_strdup_printf("%s\n%s",errstr,strrchr(files[i],'/')+1);
              if (errstr) 
                 g_free(errstr);
              errstr = tmp;
            }
          i++;  
        }
      if (ec)
        feedbackdlg = gtk_message_dialog_new(GTK_WINDOW(main_window),
          GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK, 
          "%s %i %s\n%s",_("Import of"),ec,_("files failed:"),errstr);
      else
        feedbackdlg = gtk_message_dialog_new(GTK_WINDOW(main_window),
          GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK, 
          _("Import successful"));
      gtk_dialog_run(GTK_DIALOG(feedbackdlg));
      gtk_widget_destroy(feedbackdlg);
    }
  gtk_widget_destroy(filesel);
  day_free_lists();
  week_free_lists();
  month_free_lists();
  event_db_refresh();
  update_all_views();  
}

static void
notebook_switch_page (GtkNotebook *notebook,
                      GtkNotebookPage *page,
                      guint page_num,
                      gpointer user_data)
{
  set_time_all_views();
}


static gboolean
main_window_key_press_event (GtkWidget *widget, GdkEventKey *k, GtkWidget *data)
{
  if (k->state & GDK_CONTROL_MASK)
    switch (k->keyval)
      {
        case GDK_n:
          new_appointment();
        break;	
        case GDK_o:
        case GDK_i:
          on_import_vcal(widget, NULL);
        break;	
        case GDK_t:
          set_today();
        break;	
        case GDK_q:
          gpe_cal_exit();
        break;	
      }
  /* ignore if ctrl or alt pressed */    
  if ((k->state & GDK_CONTROL_MASK) 
       || (k->state & GDK_MOD1_MASK))
    return FALSE;
  /* automatic event */
  if (k->string && isalpha(k->string[0]))
    {
        if (!just_new) 
          new_appointment();
        if (ccpos < 31) 
          collected_keys[ccpos] = k->string[0];
        ccpos++;
        return TRUE;
    }
    
  return FALSE;
}

int
main (int argc, char *argv[])
{
  GtkWidget *vbox, *toolbar;
  GdkPixbuf *p;
  GtkWidget *pw;

  guint hour, skip=0, uid=0;
  int option_letter;
  gboolean schedule_only=FALSE;
  extern char *optarg;
  gchar *ifile = NULL;

  if (gpe_application_init (&argc, &argv) == FALSE)
    exit (1);

  if (gpe_load_icons (my_icons) == FALSE)
    exit (1);

  setlocale (LC_ALL, "");

  bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
  bind_textdomain_codeset (PACKAGE, "UTF-8");
  textdomain (PACKAGE);

  displaymigration_init ();

  if (event_db_start () == FALSE)
    exit (1);

  if (gpe_pim_categories_init () == FALSE)
    exit (1);

  while ((option_letter = getopt (argc, argv, "s:e:i:")) != -1)
    {
      if (option_letter == 's')
        {
	      skip = atol (optarg);
	      schedule_only = TRUE;
        }

      if (option_letter == 'e')
        uid = atol (optarg);
	  if (option_letter == 'i')
		ifile = optarg;
    }

  schedule_next (skip, uid);

  if (schedule_only)
    exit (EXIT_SUCCESS);
  
  if (ifile)
    {
	  GtkWidget *dialog;
      if (import_one_file(ifile))
        dialog = gtk_message_dialog_new(NULL,0,GTK_MESSAGE_INFO,
	                                    GTK_BUTTONS_OK,
	                                    _("Could not import file %s."),
	                                    ifile);
	  else
        dialog = gtk_message_dialog_new(NULL,0,GTK_MESSAGE_INFO,
	                                      GTK_BUTTONS_OK,
	                                      _("File %s imported sucessfully."),
	                                      ifile);
      gtk_dialog_run(GTK_DIALOG(dialog));
      exit (EXIT_SUCCESS);
    }
	
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

  vbox = gtk_vbox_new (FALSE, 0);
  notebook = gtk_notebook_new ();

  /* main window */
  window_x = gdk_screen_width() / 2;
  window_y = gdk_screen_height() / 2;  
  if (window_x < 240) window_x = 240;
  if (window_y < 310) window_y = 310;
    
  main_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (main_window), _("Calendar"));
  g_signal_connect (G_OBJECT (main_window), "delete-event",
                    G_CALLBACK (gpe_cal_exit), NULL);
  gtk_window_set_default_size (GTK_WINDOW (main_window), window_x, window_y);

  displaymigration_mark_window (main_window);

  gtk_widget_realize (main_window);

  gtk_container_add (GTK_CONTAINER (main_window), vbox);

  time (&viewtime);
  week = week_view ();
  day = day_view ();
  month = month_view ();
  toolbar = gtk_toolbar_new ();

  gtk_toolbar_set_orientation (GTK_TOOLBAR (toolbar), GTK_ORIENTATION_HORIZONTAL);
  GTK_WIDGET_UNSET_FLAGS(toolbar, GTK_CAN_FOCUS);

  gtk_box_pack_start (GTK_BOX (vbox), toolbar, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), notebook, TRUE, TRUE, 0);

  pw = gtk_toolbar_insert_stock (GTK_TOOLBAR (toolbar), GTK_STOCK_NEW,
			    _("New appointment"), _("Tap here to add a new appointment or reminder"),
			    G_CALLBACK (new_appointment), NULL, -1);
  GTK_WIDGET_UNSET_FLAGS(pw, GTK_CAN_FOCUS);

  if (window_x > 260) 
    gtk_toolbar_append_space (GTK_TOOLBAR (toolbar));

  pw = gtk_image_new_from_stock (GTK_STOCK_HOME, gtk_toolbar_get_icon_size (GTK_TOOLBAR (toolbar)));
  today_button = gtk_toolbar_append_element (GTK_TOOLBAR (toolbar), 
                                             GTK_TOOLBAR_CHILD_TOGGLEBUTTON, 
                                             NULL,
                                             _("Today"), 
			                                 _("Switch to today and stay there/return to day selecting."), 
                                             NULL, pw, set_today, NULL);
  GTK_WIDGET_UNSET_FLAGS(today_button, GTK_CAN_FOCUS);

  if (window_x > 260) 
    gtk_toolbar_append_space (GTK_TOOLBAR (toolbar));

  p = gpe_find_icon_scaled ("day_view", gtk_toolbar_get_icon_size (GTK_TOOLBAR (toolbar)));
  pw = gtk_image_new_from_pixbuf (p);
  day_button = gtk_toolbar_append_element (GTK_TOOLBAR (toolbar),
					   GTK_TOOLBAR_CHILD_RADIOBUTTON, NULL,
					   ("Day"), 
					   _("Tap here to select day-at-a-time view."), NULL,
					   pw, GTK_SIGNAL_FUNC (button_toggled), day);
  GTK_WIDGET_UNSET_FLAGS(day_button, GTK_CAN_FOCUS);

  p = gpe_find_icon_scaled ("week_view", gtk_toolbar_get_icon_size (GTK_TOOLBAR (toolbar)));
  pw = gtk_image_new_from_pixbuf (p);
  week_button = gtk_toolbar_append_element (GTK_TOOLBAR (toolbar),
					    GTK_TOOLBAR_CHILD_RADIOBUTTON, day_button,
					    _("Week"),
					    _("Tap here to select week-at-a-time view."), NULL,
					    pw, GTK_SIGNAL_FUNC (button_toggled), week);
  GTK_WIDGET_UNSET_FLAGS(week_button, GTK_CAN_FOCUS);

  p = gpe_find_icon_scaled ("month_view", gtk_toolbar_get_icon_size (GTK_TOOLBAR (toolbar)));
  pw = gtk_image_new_from_pixbuf (p);
  month_button = gtk_toolbar_append_element (GTK_TOOLBAR (toolbar),
					     GTK_TOOLBAR_CHILD_RADIOBUTTON, week_button,
					     _("Month"), 
					     _("Tap here to select month-at-a-time view."), NULL,
					     pw, GTK_SIGNAL_FUNC (button_toggled), month);
  GTK_WIDGET_UNSET_FLAGS(month_button, GTK_CAN_FOCUS);

  pw = gtk_image_new_from_stock(GTK_STOCK_OPEN, 
                                gtk_toolbar_get_icon_size(GTK_TOOLBAR (toolbar)));
  pw = gtk_toolbar_append_item (GTK_TOOLBAR (toolbar),
			    _("Import"), _("Open file to import an event from it."), 
                NULL, pw, G_CALLBACK (on_import_vcal), NULL);
  GTK_WIDGET_UNSET_FLAGS(pw, GTK_CAN_FOCUS);
              
  gtk_toolbar_append_space (GTK_TOOLBAR (toolbar));

  pw = gtk_toolbar_insert_stock (GTK_TOOLBAR (toolbar), GTK_STOCK_QUIT,
		                 	     _("Tap here to exit the program"), NULL, 
			                     G_CALLBACK (gpe_cal_exit), NULL, -1);
  GTK_WIDGET_UNSET_FLAGS(pw, GTK_CAN_FOCUS);

  gtk_notebook_set_show_tabs (GTK_NOTEBOOK (notebook), FALSE);

  gtk_widget_show (day);
  gtk_widget_show (week);
  gtk_widget_show (month);

  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), day, NULL);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), week, NULL);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), month, NULL);

  g_signal_connect(G_OBJECT(notebook),"switch-page",
                   G_CALLBACK(notebook_switch_page),NULL);
  g_signal_connect (G_OBJECT (main_window), "key_press_event", 
		    G_CALLBACK (main_window_key_press_event), NULL);
  gtk_widget_add_events (GTK_WIDGET (main_window), 
                         GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK);

  gtk_widget_show (notebook);

  gpe_set_window_icon (main_window, "icon");

  gtk_widget_show (main_window);
  gtk_widget_show (vbox);
  gtk_widget_show (toolbar);

  gpe_calendar_start_xsettings ();

  update_all_views ();

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (day_button), TRUE);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (today_button), FALSE);
  new_view (day);

  gtk_main ();

  return 0;
}

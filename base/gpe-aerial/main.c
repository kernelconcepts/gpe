/*
 * gpe-aerial (c) 2003 Florian Boor <florian.boor@kernelconcepts.de>
 *
 * Base applet taken from gpe-bluetooth (see below)
 *
 * Copyright (C) 2002, 2003 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

//#include <sys/types.h>
#include <stdlib.h>
//#include <time.h>
#include <libintl.h>

/*#include <locale.h>
#include <pty.h>
#include <fcntl.h>
*/
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>
#include <stdio.h>

#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include <gpe/init.h>
#include <gpe/pixmaps.h>
#include <gpe/errorbox.h>
#include <gpe/gpe-iconlist.h>
#include <gpe/tray.h>
#include <gpe/popup.h>

#include <sys/socket.h>

#include "main.h"

#define _(x) gettext(x)

static GThread *scan_thread;

struct gpe_icon my_icons[] = {
  { "scan-on", "scan-on"},
  { "scan-off", "scan-off"},
  { "gpe-aerial" },
  { NULL }
};

static GtkWidget *icon;

static pid_t scan_pid;

static GtkWidget *menu;
static GtkWidget *menu_radio_on, *menu_radio_off;
static GtkWidget *menu_devices;
static GtkWidget *devices_window;
static GtkWidget *iconlist;

static GSList *devices;

gboolean radio_is_on = FALSE;
GdkWindow *dock_window;

static int
fork_scanner()
{
	return 0;
}


static void
radio_on (void)
{
  sigset_t sigs;
 
  gtk_widget_hide (menu_radio_on);
  gtk_widget_show (menu_radio_off);
  gtk_widget_set_sensitive (menu_devices, TRUE);

  gtk_image_set_from_pixbuf (GTK_IMAGE (icon), gpe_find_icon ("scan-on"));
  radio_is_on = TRUE;
  sigemptyset (&sigs);
  sigaddset (&sigs, SIGCHLD);
  sigprocmask (SIG_BLOCK, &sigs, NULL);
  scan_pid = fork_scanner ();
  sigprocmask (SIG_UNBLOCK, &sigs, NULL);
}

static void
do_stop_radio (void)
{
  radio_is_on = FALSE;

}

static void
radio_off (void)
{
  gtk_widget_hide (menu_radio_off);
  gtk_widget_show (menu_radio_on);
  gtk_widget_set_sensitive (menu_devices, FALSE);

  gtk_image_set_from_pixbuf (GTK_IMAGE (icon), gpe_find_icon ("scan-off"));

  do_stop_radio ();
}

static gboolean
devices_window_destroyed (void)
{
  devices_window = NULL;

  return FALSE;
}


static void
show_device_info (GtkWidget *w, struct bt_device *this_device)
{
}


static void
sigchld_handler (int sig)
{
  int status;
  pid_t p = waitpid (0, &status, WNOHANG);

  if (p == scan_pid)
    {
      scan_pid = 0;
      if (radio_is_on)
	{
	  gpe_error_box_nonblocking (_("scanner died unexpectedly"));
	  radio_off ();
	}
    }
  else if (p > 0)
    {
      fprintf (stderr, "unknown pid %d exited\n", p);
    }
}



static void
cancel_dock_message (guint id)
{
  gdk_threads_enter ();
  gpe_system_tray_cancel_message (dock_window, id);
  gdk_threads_leave ();
}

void
schedule_message_delete (guint id, guint time)
{
  g_timeout_add (time, (GSourceFunc) cancel_dock_message, (gpointer)id);
}

static void
clicked (GtkWidget *w, GdkEventButton *ev)
{
  gtk_menu_popup (GTK_MENU (menu), NULL, NULL, gpe_popup_menu_position, w, ev->button, ev->time);
}

int
main (int argc, char *argv[])
{
  Display *dpy;
  GtkWidget *window;
  GdkBitmap *bitmap;
  GtkWidget *menu_remove;
  GtkTooltips *tooltips;
  int dd;

  g_thread_init (NULL);
  gdk_threads_init ();

  if (gpe_application_init (&argc, &argv) == FALSE)
    exit (1);

  setlocale (LC_ALL, "");

  bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
  bind_textdomain_codeset (PACKAGE, "UTF-8");
  textdomain (PACKAGE);

  window = gtk_plug_new (0);
  gtk_widget_set_usize (window, 16, 16);
  gtk_widget_realize (window);

  gtk_window_set_title (GTK_WINDOW (window), _("Wireless LAN control"));

//  signal (SIGCHLD, sigchld_handler);

  menu = gtk_menu_new ();
  menu_radio_on = gtk_menu_item_new_with_label (_("Switch scanner on"));
  menu_radio_off = gtk_menu_item_new_with_label (_("Switch scanner off"));
  menu_devices = gtk_menu_item_new_with_label (_("Networks..."));
  menu_remove = gtk_menu_item_new_with_label (_("Remove from dock"));

  g_signal_connect (G_OBJECT (menu_radio_on), "activate", G_CALLBACK (radio_on), NULL);
  g_signal_connect (G_OBJECT (menu_radio_off), "activate", G_CALLBACK (radio_off), NULL);
//  g_signal_connect (G_OBJECT (menu_devices), "activate", G_CALLBACK (show_devices), NULL);
  g_signal_connect (G_OBJECT (menu_remove), "activate", G_CALLBACK (gtk_main_quit), NULL);

  if (! radio_is_on)
    {
      gtk_widget_set_sensitive (menu_devices, FALSE);
      gtk_widget_show (menu_radio_on);
    }

  gtk_widget_show (menu_devices);
  gtk_widget_show (menu_remove);

  gtk_menu_append (GTK_MENU (menu), menu_radio_on);
  gtk_menu_append (GTK_MENU (menu), menu_radio_off);
  gtk_menu_append (GTK_MENU (menu), menu_devices);
  gtk_menu_append (GTK_MENU (menu), menu_remove);

  if (gpe_load_icons (my_icons) == FALSE)
    exit (1);

  icon = gtk_image_new_from_pixbuf (gpe_find_icon (radio_is_on ? "scan-on" : "scan-off"));
  gtk_widget_show (icon);
  gdk_pixbuf_render_pixmap_and_mask (gpe_find_icon ("scan-off"), NULL, &bitmap, 255);
  gtk_widget_shape_combine_mask (window, bitmap, 2, 2);
  gdk_bitmap_unref (bitmap);

  gpe_set_window_icon (window, "gpe-aerial");

  tooltips = gtk_tooltips_new ();
  gtk_tooltips_set_tip (GTK_TOOLTIPS (tooltips), window, _("This is the wireless LAN selector."), NULL);

  g_signal_connect (G_OBJECT (window), "button-press-event", G_CALLBACK (clicked), NULL);
  gtk_widget_add_events (window, GDK_BUTTON_PRESS_MASK);

  gtk_container_add (GTK_CONTAINER (window), icon);

  dpy = GDK_WINDOW_XDISPLAY (window->window);

  gtk_widget_show (window);

  atexit (do_stop_radio);

  dock_window = window->window;
  gpe_system_tray_dock (window->window);

  gdk_threads_enter ();
  gtk_main ();
  gdk_threads_leave ();

  exit (0);
}

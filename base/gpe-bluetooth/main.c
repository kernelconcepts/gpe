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
#include <pty.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include <gpe/init.h>
#include <gpe/pixmaps.h>
#include <gpe/errorbox.h>
#include <gpe/gpe-iconlist.h>
#include "tray.h"

#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <bluetooth/sdp.h>
#include <bluetooth/sdp_lib.h>

#define _(x) gettext(x)

#define HCIATTACH "/etc/bluetooth/hciattach"

struct gpe_icon my_icons[] = {
  { "bluetooth/bt-on" },
  { "bluetooth/bt-off" },
  { "bluetooth/cellphone" },
  { "bluetooth/network" },
  { "bt-logo" ),
  { NULL }
};

struct bt_device
{
  gchar *name;
  guint class;
  bdaddr_t bdaddr;
  GdkPixbuf *pixbuf;
};

static GtkWidget *icon;

static pid_t hcid_pid;
static pid_t hciattach_pid;

static GtkWidget *menu;
static GtkWidget *menu_radio_on, *menu_radio_off;
static GtkWidget *menu_devices;
static GtkWidget *devices_window;
static GtkWidget *iconlist;

static gboolean radio_is_on;

static GSList *devices;

static gboolean
run_scan (void)
{
  bdaddr_t bdaddr;
  inquiry_info *info = NULL;
  int dev_id = hci_get_route(&bdaddr);
  int num_rsp, length, flags;
  char name[248];
  int i, opt, dd;

  if (dev_id < 0) 
    {
      gpe_perror_box ("Device is not available");
      return FALSE;
    }
  
  length  = 4;  /* ~10 seconds */
  num_rsp = 10;
  flags = 0;

  num_rsp = hci_inquiry(dev_id, length, num_rsp, NULL, &info, flags);
  if (num_rsp < 0) 
    {
      gpe_perror_box ("Inquiry failed.");
      close (dev_id);
      return FALSE;
    }

  dd = hci_open_dev(dev_id);
  if (dd < 0) 
    {
      gpe_perror_box ("HCI device open failed");
      close (dev_id);
      free(info);
      return FALSE;
    }

  for (i = 0; i < num_rsp; i++) 
    {
      struct bt_device *bd;

      memset(name, 0, sizeof(name));
      if (hci_read_remote_name (dd, &(info+i)->bdaddr, sizeof(name), name, 100000) < 0)
	strcpy (name, "n/a");
      baswap (&bdaddr, &(info+i)->bdaddr);

      bd = g_malloc (sizeof (struct bt_device));
      bd->name = g_strdup (name);
      memcpy (&bd->bdaddr, &bdaddr, sizeof (bdaddr));
      bd->class = ((info+i)->dev_class[2] << 16) | ((info+i)->dev_class[1] << 8) | (info+i)->dev_class[0];

      switch (bd->class & 0x1f00)
	{
	case 0x200:
	  bd->pixbuf = gpe_find_icon ("cellphone");
	  break;
	case 0x300:
	  bd->pixbuf = gpe_find_icon ("network");
	  break;
	default:
	  bd->pixbuf = gpe_find_icon ("bt-logo");
	  break;
	}
      gdk_pixbuf_ref (bd->pixbuf);

      devices = g_slist_append (devices, bd);
    }
  
  close (dd);
  close (dev_id);
  return TRUE;
}

static pid_t
fork_hcid (void)
{
  pid_t p = vfork ();
  if (p == 0)
    {
      execlp ("hcid", "hcid", "-n", NULL);
      perror ("hcid");
      _exit (1);
    }

  return p;
}

static pid_t
fork_hciattach (void)
{
  if (access (HCIATTACH, X_OK) == 0)
    {
      pid_t p = vfork ();
      if (p == 0)
	{
	  execl (HCIATTACH, HCIATTACH, NULL);
	  perror (HCIATTACH);
	  _exit (1);
	}

      return p;
    }

  return 0;
}

static void
radio_on (void)
{
  sigset_t sigs;

  gtk_widget_hide (menu_radio_on);
  gtk_widget_show (menu_radio_off);
  gtk_widget_set_sensitive (menu_devices, TRUE);

  gtk_image_set_from_pixbuf (GTK_IMAGE (icon), gpe_find_icon ("bt-on"));
  radio_is_on = TRUE;
  sigemptyset (&sigs);
  sigaddset (&sigs, SIGCHLD);
  sigprocmask (SIG_BLOCK, &sigs, NULL);
  hcid_pid = fork_hcid ();
  hciattach_pid = fork_hciattach ();
  sigprocmask (SIG_UNBLOCK, &sigs, NULL);
}

static void
radio_off (void)
{
  gtk_widget_hide (menu_radio_off);
  gtk_widget_show (menu_radio_on);
  gtk_widget_set_sensitive (menu_devices, FALSE);

  gtk_image_set_from_pixbuf (GTK_IMAGE (icon), gpe_find_icon ("bt-off"));
  radio_is_on = FALSE;
  if (hcid_pid)
    {
      kill (hcid_pid, 15);
      hcid_pid = 0;
    }
  if (hciattach_pid)
    {
      kill (hciattach_pid, 15);
      hciattach_pid = 0;
    }
  system ("hciconfig hci0 down");
}

static gboolean
devices_window_destroyed (void)
{
  devices_window = NULL;

  return FALSE;
}

static gboolean
browse_device (bdaddr_t *bdaddr)
{
  GSList *attrid, *search;
  GSList *pSeq = NULL;
  GSList *pGSList = NULL;
  uint32_t range = 0x0000ffff;
  uint16_t count = 0;
  int status = -1, i;
  uuid_t group;

  sdp_create_uuid16 (&group, PUBLIC_BROWSE_GROUP);

  printf ("Searching %s\n", batostr (bdaddr));

  attrid = g_slist_append (NULL, &range);
  search = g_slist_append (NULL, &group);
  status = sdp_service_search_attr_req (bdaddr, search, SDP_ATTR_REQ_RANGE,
					attrid, 65535, &pSeq, &count);

  if (status) 
    {
      gpe_error_box_fmt (_("Service search failed (status %x)"), status);
      return FALSE;
    }

  for (i = 0; i < (int) g_slist_length(pSeq); i++) 
    {
      uint32_t svcrec;
      svcrec = *(uint32_t *) g_slist_nth_data(pSeq, i);

      /* ... */

    }
  
  return status;
}

static void
device_clicked (GtkWidget *w, gpointer data)
{
  struct bt_device *bd = (struct bt_device *)data;
  GtkWidget *window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  GtkWidget *vbox1 = gtk_vbox_new (FALSE, 0);
  GtkWidget *hbox1 = gtk_hbox_new (FALSE, 0);
  GtkWidget *labelname = gtk_label_new (bd->name);
  GtkWidget *labeladdr = gtk_label_new (batostr (&bd->bdaddr));
  GtkWidget *image = gtk_image_new_from_pixbuf (bd->pixbuf);
  GtkWidget *vbox2 = gtk_vbox_new (FALSE, 0);

  gtk_widget_show (vbox1);
  gtk_widget_show (hbox1);
  gtk_widget_show (vbox2);
  gtk_widget_show (labelname);
  gtk_widget_show (labeladdr);
  gtk_widget_show (image);

  gtk_misc_set_alignment (GTK_MISC (labelname), 0.0, 0.5);
  gtk_misc_set_alignment (GTK_MISC (labeladdr), 0.0, 0.5);

  gtk_box_pack_start (GTK_BOX (vbox1), labelname, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox1), labeladdr, TRUE, TRUE, 0);

  gtk_box_pack_start (GTK_BOX (hbox1), vbox1, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox1), image, TRUE, TRUE, 0);

  gtk_box_pack_start (GTK_BOX (vbox2), hbox1, FALSE, FALSE, 0);

  gtk_container_add (GTK_CONTAINER (window), vbox2);

  gtk_widget_realize (window);
  gdk_window_set_transient_for (window->window, devices_window->window);
  
  gtk_widget_show (window);

  browse_device (&bd->bdaddr);
}

static void
show_devices (void)
{
  GSList *iter;

  if (devices_window == NULL)
    {
      devices_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

      gtk_window_set_title (GTK_WINDOW (devices_window), _("Bluetooth devices"));

      iconlist = gpe_iconlist_new ();
      gtk_widget_show (iconlist);
      gtk_container_add (GTK_CONTAINER (devices_window), iconlist);
      gpe_iconlist_set_embolden (GPE_ICONLIST (iconlist), FALSE);

      g_signal_connect (G_OBJECT (iconlist), "clicked", 
			G_CALLBACK (device_clicked), NULL);
      
      g_signal_connect (G_OBJECT (devices_window), "destroy", 
			G_CALLBACK (devices_window_destroyed), NULL);
    }

  gtk_widget_show (devices_window);

  run_scan ();

  gpe_iconlist_clear (GPE_ICONLIST (iconlist));

  for (iter = devices; iter; iter = iter->next)
    {
      struct bt_device *bd = iter->data;
      gpe_iconlist_add_item_pixbuf (GPE_ICONLIST (iconlist), bd->name, bd->pixbuf, bd);
    }
}

static void
sigchld_handler (int sig)
{
  int status;
  pid_t p = waitpid (0, &status, WNOHANG);

  if (p == hcid_pid)
    {
      hcid_pid = 0;
      if (radio_is_on)
	{
	  gpe_error_box (_("hcid died unexpectedly"));
	  radio_off ();
	}
    }
  else if (p == hciattach_pid)
    {
      hciattach_pid = 0;
      if (radio_is_on)
	{
	  gpe_error_box (_("hciattach died unexpectedly"));
	  radio_off ();
	}
    }
  else if (p != -1)
    {
      fprintf (stderr, "unknown pid %d exited\n", p);
    }
}

static void
clicked (GtkWidget *w, GdkEventButton *ev)
{
  gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL, ev->button, ev->time);
}

static GdkFilterReturn
filter (GdkXEvent *xevp, GdkEvent *ev, gpointer p)
{
  XEvent *xev = (XEvent *)xevp;
  
  if (xev->type == ClientMessage || xev->type == ReparentNotify)
    {
      XAnyEvent *any = (XAnyEvent *)xev;
      tray_handle_event (any->display, any->window, xev);
      if (xev->type == ReparentNotify)
	gtk_widget_show (GTK_WIDGET (p));
    }
  return GDK_FILTER_CONTINUE;
}

int
main (int argc, char *argv[])
{
  Display *dpy;
  GtkWidget *window;
  GdkBitmap *bitmap;

  if (gpe_application_init (&argc, &argv) == FALSE)
    exit (1);

  setlocale (LC_ALL, "");

  bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
  textdomain (PACKAGE);

  sdp_init ();

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_widget_set_usize (window, 16, 16);
  gtk_widget_realize (window);

  signal (SIGCHLD, sigchld_handler);

  menu = gtk_menu_new ();
  menu_radio_on = gtk_menu_item_new_with_label (_("Switch radio on"));
  menu_radio_off = gtk_menu_item_new_with_label (_("Switch radio off"));
  menu_devices = gtk_menu_item_new_with_label (_("Devices..."));

  g_signal_connect (G_OBJECT (menu_radio_on), "activate", G_CALLBACK (radio_on), NULL);
  g_signal_connect (G_OBJECT (menu_radio_off), "activate", G_CALLBACK (radio_off), NULL);
  g_signal_connect (G_OBJECT (menu_devices), "activate", G_CALLBACK (show_devices), NULL);

  gtk_widget_set_sensitive (menu_devices, FALSE);

  gtk_widget_show (menu_radio_on);
  gtk_widget_show (menu_devices);

  gtk_menu_append (GTK_MENU (menu), menu_radio_on);
  gtk_menu_append (GTK_MENU (menu), menu_radio_off);
  gtk_menu_append (GTK_MENU (menu), menu_devices);

  if (gpe_load_icons (my_icons) == FALSE)
    exit (1);

  icon = gtk_image_new_from_pixbuf (gpe_find_icon ("bt-off"));
  gtk_widget_show (icon);
  gdk_pixbuf_render_pixmap_and_mask (gpe_find_icon ("bt-off"), NULL, &bitmap, 255);
  gtk_widget_shape_combine_mask (window, bitmap, 0, 0);
  gdk_bitmap_unref (bitmap);

  g_signal_connect (G_OBJECT (window), "button-press-event", G_CALLBACK (clicked), NULL);
  gtk_widget_add_events (window, GDK_BUTTON_PRESS_MASK);

  gtk_container_add (GTK_CONTAINER (window), icon);

  dpy = GDK_WINDOW_XDISPLAY (window->window);

  tray_init (dpy, GDK_WINDOW_XWINDOW (window->window));
  gdk_window_add_filter (window->window, filter, window);

  gtk_main ();

  exit (0);
}

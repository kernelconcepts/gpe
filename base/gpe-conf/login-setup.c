/*
 * login-setup module for gpe-conf
 *
 * Copyright (C) 2002 Colin Marquardt <ipaq@marquardt-home.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <pwd.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <gtk/gtk.h>
#define _XOPEN_SOURCE /* For GlibC2 */
#include <time.h>
#include <unistd.h> /* for readlink () */

#include <gpe/errorbox.h>
#include <gpe/pixmaps.h>
#include <gpe/render.h>
#include <gpe/spacing.h>

#include "applets.h"
#include "misc.h"
#include "login-setup.h"

#define GPE_OWNERINFO_DONTSHOW_FILE "/etc/gpe/gpe-ownerinfo.dontshow"
#define GPE_LOGIN_BG_LINKED_FILE    "/etc/gpe/gpe-login-bg.png"
#define GPE_LOGIN_BG_DONTSHOW_FILE  "/etc/gpe/gpe-login-bg.dontshow"
 
GtkWidget *login_bg_show_check  = NULL;
GtkWidget *ownerinfo_show_check = NULL;
GtkWidget *login_bg_pixmap;
GtkWidget *controlvbox1;

GtkWidget *login_bg_file_label;
GtkWidget *login_bg_file_entry;
GtkWidget *login_bg_file_button;

gboolean login_bg_show  = FALSE;
gboolean ownerinfo_show = FALSE;

gboolean login_bg_show_initial  = FALSE;
gboolean ownerinfo_show_initial = FALSE;

gboolean login_bg_show_writable  = FALSE;
gboolean ownerinfo_show_writable = FALSE;

guint buttonwidth, buttonheight = 42;

static gchar login_bg_filename[PATH_MAX + 1];

GtkWidget *Login_Setup_Build_Objects()
{
  GtkWidget *mainvbox;
  GtkWidget *rootwarn_hbox;
  GtkWidget *rootwarn_icon;
  GtkWidget *rootwarn_label;
  
  GtkWidget *categories;
  GtkWidget *catvbox1;
  GtkWidget *catlabel1;
  GtkWidget *catconthbox1;
  GtkWidget *catindentlabel1;
  
  GdkPixbuf *pixbuf;

  gchar *gpe_catindent = gpe_get_catindent ();
  guint gpe_catspacing = gpe_get_catspacing ();
  guint gpe_boxspacing = gpe_get_boxspacing ();
  guint gpe_border     = gpe_get_border ();

  gint bytes;

  if ((bytes = readlink (GPE_LOGIN_BG_LINKED_FILE,
			 login_bg_filename,
			 sizeof (login_bg_filename) - 1)) < 0)
    {
      perror ("login-setup: Cannot read file at end of symlink (with readlink)");
      login_bg_filename[0] = '\0';
    }
  else
    {
      login_bg_filename[bytes] = '\0';
    }
  g_message ("login-setup: File at end of symlink: %s", login_bg_filename);

  /* FIXME: GTK2: use g_file_test(), not access()
     (see http://developer.gnome.org/doc/API/2.0/glib/glib-file-utilities.html) */
  
  /* either we can write to the file... */
  if ((access (GPE_LOGIN_BG_DONTSHOW_FILE, W_OK) == 0) ||
      /* ...or we are allowed to write in this directory, and the file does not yet exist */
      (((access (gpe_dirname (g_strdup (GPE_LOGIN_BG_DONTSHOW_FILE)), W_OK)) == 0) &&
       (access (GPE_LOGIN_BG_DONTSHOW_FILE, F_OK) != 0)))
    login_bg_show_writable = TRUE;
  else
    login_bg_show_writable = FALSE;
  
  /* either we can write to the file... */
  if ((access (GPE_OWNERINFO_DONTSHOW_FILE, W_OK) == 0) ||
      /* ...or we are allowed to write in this directory, and the file does not yet exist */
      (((access (gpe_dirname (g_strdup (GPE_OWNERINFO_DONTSHOW_FILE)), W_OK)) == 0) &&
       (access (GPE_OWNERINFO_DONTSHOW_FILE, F_OK) != 0)))
    ownerinfo_show_writable = TRUE;
  else
    ownerinfo_show_writable = FALSE;
  
  /* ======================================================================== */
  /* draw the GUI */

  /* the vbox which can hold the warning hbox (containing icon and text) */
  mainvbox = gtk_vbox_new (FALSE, gpe_boxspacing);
  gtk_widget_show (mainvbox);
  gtk_container_set_border_width (GTK_CONTAINER (mainvbox), gpe_border);
  
  rootwarn_hbox = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (rootwarn_hbox);

  /* FIXME: GTK2: make this text bold */
  rootwarn_label = gtk_label_new (_("Some or all of these settings can only be changed by the user 'root'"));
  gtk_widget_show (rootwarn_label);
  gtk_label_set_justify (GTK_LABEL (rootwarn_label), GTK_JUSTIFY_LEFT);
  gtk_label_set_line_wrap (GTK_LABEL (rootwarn_label), TRUE);
  gtk_misc_set_alignment (GTK_MISC (rootwarn_label), 0, 0);

  pixbuf = gpe_find_icon ("warning16");
  rootwarn_icon = gpe_render_icon (rootwarn_hbox->style, pixbuf);
  gtk_misc_set_alignment (GTK_MISC (rootwarn_icon), 0, 0);
  gtk_box_pack_start (GTK_BOX (rootwarn_hbox), rootwarn_icon, FALSE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (rootwarn_hbox), rootwarn_label, TRUE, TRUE, 2*gpe_boxspacing);
  if ((!login_bg_show_writable) || (!ownerinfo_show_writable))
    gtk_box_pack_start (GTK_BOX (mainvbox), rootwarn_hbox, FALSE, TRUE, 0);
   
  /* -------------------------------------------------------------------------- */
  categories = gtk_vbox_new (FALSE, gpe_catspacing);
  gtk_box_pack_start (GTK_BOX (mainvbox), categories, TRUE, TRUE, 0);
  
  catvbox1 = gtk_vbox_new (FALSE, gpe_boxspacing);
  gtk_box_pack_start (GTK_BOX (categories), catvbox1, TRUE, TRUE, 0);
  
  catlabel1 = gtk_label_new (_("General")); /* FIXME: GTK2: make this bold */
  gtk_box_pack_start (GTK_BOX (catvbox1), catlabel1, FALSE, FALSE, 0);
  gtk_label_set_justify (GTK_LABEL (catlabel1), GTK_JUSTIFY_LEFT);
  gtk_misc_set_alignment (GTK_MISC (catlabel1), 0, 0.5);
  
  catconthbox1 = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (catvbox1), catconthbox1, TRUE, TRUE, 0);
  
  catindentlabel1 = gtk_label_new (gpe_catindent);
  gtk_box_pack_start (GTK_BOX (catconthbox1), catindentlabel1, FALSE, FALSE, 0);
  
  controlvbox1 = gtk_vbox_new (FALSE, gpe_boxspacing);
  gtk_box_pack_start (GTK_BOX (catconthbox1), controlvbox1, TRUE, TRUE, 0);
  
  ownerinfo_show_check =
    gtk_check_button_new_with_label ("Show owner information at login");
  login_bg_show_check =
    gtk_check_button_new_with_label ("Use background image for login screen");

  /* check the dontshow files to set initial values for the checkboxes */
  get_initial_values();
  ownerinfo_show = ownerinfo_show_initial;
  login_bg_show  = login_bg_show_initial;

  gtk_box_pack_start (GTK_BOX(controlvbox1), ownerinfo_show_check, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX(controlvbox1), login_bg_show_check, FALSE, FALSE, 0);

  /* ------------------------------------------------------------------------ */
  login_bg_file_label = gtk_label_new (_("Image:"));
  gtk_box_pack_start (GTK_BOX(controlvbox1), login_bg_file_label, FALSE, FALSE, 0);
  gtk_label_set_justify (GTK_LABEL (login_bg_file_label), GTK_JUSTIFY_LEFT);
  gtk_misc_set_alignment (GTK_MISC (login_bg_file_label), 0, 0.5);
  gtk_misc_set_padding (GTK_MISC (login_bg_file_label), 0, gpe_boxspacing);

  login_bg_file_entry = gtk_entry_new ();
  gtk_box_pack_start (GTK_BOX(controlvbox1), login_bg_file_entry, FALSE, FALSE, 0);
  gtk_entry_set_text (GTK_ENTRY (login_bg_file_entry), login_bg_filename);

  login_bg_file_button = gtk_button_new ();
  gtk_box_pack_start (GTK_BOX(controlvbox1), login_bg_file_button, TRUE, TRUE, 0);

  /* these values are just a dummy size for the initial display */ 
  login_bg_pixmap = gpe_create_pixmap (controlvbox1, login_bg_filename, 24, 32);
  /* add the container in on_login_bg_file_button_size_allocate: */
  /* gtk_container_add (GTK_CONTAINER (login_bg_file_button), login_bg_pixmap); */

  gtk_signal_connect (GTK_OBJECT (login_bg_file_button), "clicked",
 		      GTK_SIGNAL_FUNC (choose_login_bg_file),
 		      NULL);
  
  gtk_signal_connect (GTK_OBJECT (login_bg_file_button), "size_allocate",
		      GTK_SIGNAL_FUNC (on_login_bg_file_button_size_allocate),
		      NULL);

  /* FIXME: trigger on any change */
  gtk_signal_connect (GTK_OBJECT (login_bg_show_check), "clicked",
                      GTK_SIGNAL_FUNC (update_login_bg_show),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (ownerinfo_show_check), "clicked",
                      GTK_SIGNAL_FUNC (update_ownerinfo_show),
                      NULL);

  if (ownerinfo_show_writable)
    gtk_widget_set_sensitive (ownerinfo_show_check, TRUE);
  else 
    gtk_widget_set_sensitive (ownerinfo_show_check, FALSE);

  if (login_bg_show)
    {
      gtk_widget_set_sensitive (login_bg_file_label, TRUE);
      gtk_widget_set_sensitive (login_bg_file_entry, TRUE);
      gtk_widget_set_sensitive (login_bg_file_button, TRUE);
    }
  else
    {
      gtk_widget_set_sensitive (login_bg_file_label, FALSE);
      gtk_widget_set_sensitive (login_bg_file_entry, FALSE);
      gtk_widget_set_sensitive (login_bg_file_button, FALSE);
    }

  if (login_bg_show_writable)
    {
      gtk_widget_set_sensitive (login_bg_show_check, TRUE);
      gtk_widget_set_sensitive (login_bg_file_label, TRUE);
      gtk_widget_set_sensitive (login_bg_file_entry, TRUE);
      gtk_widget_set_sensitive (login_bg_file_button, TRUE);
    }
  else
    {
      gtk_widget_set_sensitive (login_bg_show_check, FALSE);
      gtk_widget_set_sensitive (login_bg_file_label, FALSE);
      gtk_widget_set_sensitive (login_bg_file_entry, FALSE);
      gtk_widget_set_sensitive (login_bg_file_button, FALSE);
    }


  return mainvbox;
}

void
Login_Setup_Free_Objects ()
{
}

void
Login_Setup_Save ()
{
  /* applying immediately without saving, no explicit save necessary */
}

void
Login_Setup_Restore ()
{
  // FIXME: gets called when applet changes!
  g_message ("Requested explicit restoration of initial values");
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(login_bg_show_check),
				login_bg_show_initial);
  login_bg_show = login_bg_show_initial;
  
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(ownerinfo_show_check),
				ownerinfo_show_initial);
  ownerinfo_show = ownerinfo_show_initial;
}


void
choose_login_bg_file (GtkWidget *button,
		      gpointer  user_data)
{
  ask_user_a_file (gpe_dirname (login_bg_filename), NULL, File_Selected, NULL, NULL);
}

static void
File_Selected (char *file,
	       gpointer data)
{
  /* check if we can read the selected file */
  /* FIXME: gotta check if it's a valid png file */
  if (access (file, R_OK) == 0) {
    gtk_entry_set_text (GTK_ENTRY (login_bg_file_entry), file);
    
    gtk_container_remove (GTK_CONTAINER (login_bg_file_button), login_bg_pixmap);
    login_bg_pixmap = gpe_create_pixmap (controlvbox1, file,
					 buttonwidth,
					 buttonheight);
    gtk_container_add (GTK_CONTAINER (login_bg_file_button), login_bg_pixmap);
    gtk_widget_show (GTK_WIDGET (login_bg_pixmap));
  }
  else {
    g_message ("Can't read '%s'.", file);
  }
  /* FIXME: GTK2: use g_strlcpy */
  strncpy (login_bg_filename, file, sizeof (login_bg_filename));

  // FIXME: if dontshow is writeable but the png is not
  update_login_bg_show ();
}


void
get_initial_values ()
{
  g_message ("Checking the dontshow files to set initial values for the checkboxes");
  /* check if the dontshow files are there */
  if (access (GPE_LOGIN_BG_DONTSHOW_FILE, F_OK) == 0)
    login_bg_show_initial = FALSE;
  else
    login_bg_show_initial = TRUE;
  
  if (access (GPE_OWNERINFO_DONTSHOW_FILE, F_OK) == 0)
    ownerinfo_show_initial = FALSE;
  else
    ownerinfo_show_initial = TRUE;
  
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(login_bg_show_check),
				login_bg_show_initial);
  
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(ownerinfo_show_check),
				ownerinfo_show_initial);
}


void 
update_login_bg_show ()
{
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(login_bg_show_check)))
    login_bg_show = TRUE;
  else
    login_bg_show = FALSE;
  
  if (login_bg_show &&
      (access (GPE_LOGIN_BG_DONTSHOW_FILE, W_OK) == 0) &&
      (access (gpe_dirname (g_strdup (GPE_LOGIN_BG_DONTSHOW_FILE)), W_OK) == 0))
    {
      gtk_widget_set_sensitive (login_bg_file_label, TRUE);
      gtk_widget_set_sensitive (login_bg_file_entry, TRUE);
      gtk_widget_set_sensitive (login_bg_file_button, TRUE);
    }
  else
    {
      gtk_widget_set_sensitive (login_bg_file_label, FALSE);
      gtk_widget_set_sensitive (login_bg_file_entry, FALSE);
      gtk_widget_set_sensitive (login_bg_file_button, FALSE);
    }

  if (login_bg_show) {
    g_message("EEEEEEEEEEEEEE");
    /* check if the dontshow file exists and is writable */
    if (login_bg_show_writable) {
      g_message("rm (GPE_LOGIN_BG_DONTSHOW_FILE);");
      system_printf("rm %s", GPE_LOGIN_BG_DONTSHOW_FILE);
    }
    
  }
  else {
    /* check if the dontshow file doesn't exist */
    if (access (GPE_LOGIN_BG_DONTSHOW_FILE, F_OK) != 0) {
      if (login_bg_show_writable) {
	g_message("touch (GPE_LOGIN_BG_DONTSHOW_FILE);");
	system_printf("touch %s", GPE_LOGIN_BG_DONTSHOW_FILE);
      }
    }
  }
}

void 
update_ownerinfo_show ()
{
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(ownerinfo_show_check)))
    ownerinfo_show = TRUE;
  else
    ownerinfo_show = FALSE;
    
  if (ownerinfo_show) {
    /* check if the dontshow file exists and is writable */
    if (access (GPE_OWNERINFO_DONTSHOW_FILE, W_OK) == 0) {
      if (ownerinfo_show_writable) {
	g_message("rm (GPE_OWNERINFO_DONTSHOW_FILE);");
	system_printf("rm %s", GPE_OWNERINFO_DONTSHOW_FILE);
      }
    }
  }
  else {
    /* check if the dontshow file doesn't exist */
    if (access (GPE_OWNERINFO_DONTSHOW_FILE, F_OK) != 0) {
      if (ownerinfo_show_writable) {
	g_message("touch (GPE_OWNERINFO_DONTSHOW_FILE);");
	system_printf("touch %s", GPE_OWNERINFO_DONTSHOW_FILE);
      }
    }
  }
}


void
on_login_bg_file_button_size_allocate (GtkWidget       *widget,
                                       GtkAllocation   *allocation,
                                       gpointer         user_data)
{
  /* Note: this updates only the first time the button/pixmap is
     drawn, but this is fine. */

  /* g_message ("allocation1: %d x %d", allocation->width, allocation->height); */
  
  buttonwidth  = allocation->width;
  buttonheight = allocation->height;
  /*
   * gtk_container_children() brought to me by PaxAnima. Thanks.
   * It looks like in GTK2, this function is called
   *   gtk_container_get_children().
   */
  if (gtk_container_children (GTK_CONTAINER (login_bg_file_button)) == NULL) {
    login_bg_pixmap = gpe_create_pixmap (controlvbox1, login_bg_filename,
					 buttonwidth,
					 buttonheight);
    gtk_container_add (GTK_CONTAINER (login_bg_file_button), login_bg_pixmap);
    gtk_widget_show (GTK_WIDGET (login_bg_pixmap));
  }
}

/*
 * Copyright (C) 2002 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <pwd.h>
#include <sys/types.h>
#include <shadow.h>
#include <crypt.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>
#include <libintl.h>
#include <time.h>
#include <stdio.h>
#include <grp.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <gtk/gtk.h>
#include <gdk_imlib.h>
#include <gdk/gdkx.h>

#include "errorbox.h"

#define _(x) gettext(x)

#define GPE_ICON "/usr/share/gpe/pixmaps/gpe-logo.png"
#define GPE_LOGIN_SETUP "/etc/X11/gpe-login-setup"
#define PASSWORD_FILE "/etc/passwd"
#define GROUP_FILE "/etc/group"
#define SHELL "/bin/sh"

#define bin_to_ascii(c) ((c)>=38?((c)-38+'a'):(c)>=12?((c)-12+'A'):(c)+'.')

static const char *current_username;
static GtkWidget *label_result;
static gboolean have_users;
static pid_t cpid;
static const char *xkbd_path = "/usr/bin/xkbd";

static GtkWidget *entry_username, *entry_fullname;
static GtkWidget *entry_password, *entry_confirm;

static void
cleanup_children (void)
{
  kill (-cpid, 15);
}

static void
cleanup_children_and_exit (int s)
{
  cleanup_children ();
  exit (0);
}

static void
set_username (GtkWidget *widget, gpointer data)
{
  current_username = (const char *)data;
}

static void
slurp_passwd (GtkWidget *menu)
{
  struct passwd *pw;
  while (pw = getpwent (), pw != NULL)
    {
      const char *name;
      GtkWidget *item;

      if (pw->pw_uid < 100 || pw->pw_uid >= 65534)
	continue;

      have_users = TRUE;
      name = g_strdup (pw->pw_name);
      item = gtk_menu_item_new_with_label (name);

      gtk_signal_connect (GTK_OBJECT(item), "activate", 
			  GTK_SIGNAL_FUNC (set_username), (gpointer)name);
      gtk_menu_append (GTK_MENU (menu), item);

      if (current_username == NULL)
	current_username = name;
    }
}

static void
move_callback (GtkWidget *widget, GtkWidget *entry)
{
  gtk_widget_grab_focus (entry);
}

static void
do_login (uid_t uid, gid_t gid, char *dir)
{
  pid_t spid;

  cleanup_children ();
  
  spid = fork ();
  
  if (spid == 0)
    {
      setuid (uid);
      setgid (gid);
      setenv ("HOME", dir, 1);
      chdir (dir);
      
      if (access (".xsession", X_OK) == 0)
	execl (".xsession", ".xsession", NULL);
      execl ("/etc/X11/Xsession", "/etc/X11/Xsession", NULL);
      _exit (1);
    }

  waitpid (spid, NULL, 0);
}

static void
enter_callback (GtkWidget *widget, GtkWidget *entry)
{
  gchar *pwstr;
  struct passwd *pwe;
  struct spwd *spe;
  char *p;

  if (current_username == NULL)
    return;

  pwstr = gtk_entry_get_text (GTK_ENTRY(entry));
  gtk_entry_set_text (GTK_ENTRY(entry), "");

  pwe = getpwnam (current_username);
  if (pwe == NULL)
    goto login_incorrect;

  spe = getspnam (current_username);
  if (spe)
    pwe->pw_passwd = spe->sp_pwdp;

  p = crypt (pwstr, pwe->pw_passwd);
  if (strcmp (p, pwe->pw_passwd))
    goto login_incorrect;
  
  do_login (pwe->pw_uid, pwe->pw_gid, pwe->pw_dir);

 login_incorrect:
  gtk_label_set_text (GTK_LABEL (label_result), _("Login incorrect"));
}

static void
enter_newuser_callback (GtkWidget *widget, gpointer h)
{
  gchar *username, *fullname, *password, *confirm;
  char *cryptstr;
  struct passwd *pwe;
  struct group *gre;
  time_t tm;
  char salt[2];
  char buf[256];
  char home[80];
  gid_t gid = 100;
  uid_t uid = 100;
  FILE *fp;

  username = gtk_entry_get_text (GTK_ENTRY (entry_username));
  fullname = gtk_entry_get_text (GTK_ENTRY (entry_fullname));
  password = gtk_entry_get_text (GTK_ENTRY (entry_password));
  confirm = gtk_entry_get_text (GTK_ENTRY (entry_confirm));

  pwe = getpwnam (username);
  if (pwe)
    {
      gpe_error_box (_("User already exists"));

      gtk_entry_set_text (GTK_ENTRY (entry_username), "");
      gtk_entry_set_text (GTK_ENTRY (entry_password), "");
      gtk_entry_set_text (GTK_ENTRY (entry_confirm), "");
      gtk_widget_grab_focus (entry_username);
      return;
    }

  if (strcmp (password, confirm))
    {
      gpe_error_box (_("Passwords don't match"));
      gtk_entry_set_text (GTK_ENTRY (entry_password), "");
      gtk_entry_set_text (GTK_ENTRY (entry_confirm), "");
      gtk_widget_grab_focus (entry_password);
      return;
    }

  if (password[0] == 0)
    {
      gpe_error_box (_("Empty password not allowed"));
      gtk_widget_grab_focus (entry_password);
      return;
    }

  time (&tm);
  salt[0] = bin_to_ascii (tm & 0x3f);
  salt[1] = bin_to_ascii ((tm >> 6) & 0x3f);
  cryptstr = crypt (password, salt);

  while (pwe = getpwent (), pwe != NULL)
    {
      if (pwe->pw_uid < 60000 && pwe->pw_uid >= uid)
	uid = pwe->pw_uid + 1;
    }
  endpwent ();

  while (gre = getgrent (), gre != NULL)
    {
      if (gre->gr_gid >= gid)
	gid = gre->gr_gid + 1;
    }
  endgrent ();

  snprintf (home, sizeof (home), "/home/%s", username);
  mkdir (home, 0770);
  chown (home, uid, gid);

  fp = fopen (PASSWORD_FILE, "a");
  if (fp == NULL)
    {
      gpe_perror_box (PASSWORD_FILE);
      exit (1);
    }
  snprintf (buf, sizeof (buf), "%s:%s:%d:%d:%s:%s:%s\n",
	    username, cryptstr, uid, gid, fullname, home, SHELL);
  fputs (buf, fp);
  fclose (fp);

  fp = fopen (GROUP_FILE, "a");
  if (fp == NULL)
    {
      gpe_perror_box (GROUP_FILE);
      exit (1);
    }
  snprintf (buf, sizeof (buf), "%s:x:%d:\n", username, gid);
  fputs (buf, fp);
  fclose (fp);

  do_login (uid, gid, home);
}

int
main (int argc, char *argv[])
{
  GtkWidget *window;
  GtkWidget *option, *menu;
  GtkWidget *vbox, *hbox, *vbox2;
  GtkWidget *next_button;
  GtkWidget *frame;
  GtkWidget *logo = NULL;
  GtkWidget *focus;
  GtkWidget *socket;
  pid_t kpid;
  int fd[2];
  guint xkbd_xid = 0;

  GdkPixmap *gpe_pix;
  GdkBitmap *gpe_pix_mask;

  gtk_set_locale ();
  gtk_init (&argc, &argv);
  gdk_imlib_init ();

  cpid = fork ();
  if (cpid == 0)
    {
      pid_t mypid = getpid ();
      setpgid (0, mypid);
      system (GPE_LOGIN_SETUP);
      _exit (0);
    }

  waitpid (cpid, NULL, 0);

  signal (SIGINT, cleanup_children_and_exit);
  signal (SIGQUIT, cleanup_children_and_exit);
  signal (SIGTERM, cleanup_children_and_exit);
  
  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

  socket = gtk_socket_new ();

  pipe (fd);
  kpid = fork ();
  if (kpid == 0)
    {
      close (fd[0]);
      if (dup2 (fd[1], 1) < 0)
	perror ("dup2");
      close (fd[1]);
      if (fcntl (1, F_SETFD, 0))
	perror ("fcntl");
      execl (xkbd_path, xkbd_path, "-xid", NULL);
      _exit (1);
    }

  close (fd[1]);

  {
    char buf[256];
    char c;
    int a = 0;
    size_t n;

    do {
      n = read (fd[0], &c, 1);
      if (n)
	{
	  buf[a++] = c;
	}
    } while (n && (c != 10) && (a < (sizeof (buf) - 1)));

    if (a)
      {
	buf[a] = 0;
	xkbd_xid = atoi (buf);
      }
  }

  if (gdk_imlib_load_file_to_pixmap (GPE_ICON, &gpe_pix, &gpe_pix_mask))
    logo = gtk_pixmap_new (gpe_pix, gpe_pix_mask);

  menu = gtk_menu_new ();
  slurp_passwd (menu);

  next_button = gtk_button_new_with_label (_("OK"));

  vbox2 = gtk_vbox_new (FALSE, 0);

  if (logo)
    gtk_box_pack_start (GTK_BOX (vbox2), logo, FALSE, FALSE, 0);

  hbox = gtk_hbox_new (FALSE, 0);

  if (have_users)
    {
      GtkWidget *hbox_user, *hbox_password;
      GtkWidget *login_label, *password_label;
      GtkWidget *entry;

      frame = gtk_frame_new (_("Log in"));

      login_label = gtk_label_new (_("Username"));
      password_label = gtk_label_new (_("Password"));
      label_result = gtk_label_new ("");

      option = gtk_option_menu_new ();
      gtk_option_menu_set_menu (GTK_OPTION_MENU (option), menu);
      entry = gtk_entry_new ();
      gtk_entry_set_visibility (GTK_ENTRY (entry), FALSE);

      gtk_widget_set_usize (entry, 120, -1);
      gtk_widget_set_usize (option, 120, -1);

      gtk_signal_connect (GTK_OBJECT (window), "delete_event",
			  GTK_SIGNAL_FUNC (gtk_main_quit), NULL);
      
      hbox_user = gtk_hbox_new (FALSE, 0);
      gtk_box_pack_start (GTK_BOX (hbox_user), login_label, TRUE, TRUE, 4);
      gtk_box_pack_start (GTK_BOX (hbox_user), option, TRUE, TRUE, 0);
      
      hbox_password = gtk_hbox_new (FALSE, 0);
      gtk_box_pack_start (GTK_BOX (hbox_password), password_label, 
			  TRUE, TRUE, 4);
      gtk_box_pack_start (GTK_BOX (hbox_password), entry, TRUE, TRUE, 0);
      
      vbox = gtk_vbox_new (FALSE, 0);

      gtk_box_pack_start (GTK_BOX (vbox), hbox_user, FALSE, FALSE, 0);
      gtk_box_pack_start (GTK_BOX (vbox), hbox_password, FALSE, FALSE, 0);
      gtk_box_pack_start (GTK_BOX (vbox), label_result, FALSE, FALSE, 0);
      gtk_box_pack_start (GTK_BOX (vbox), socket, TRUE, TRUE, 0);
      
      gtk_container_add (GTK_CONTAINER (frame), vbox);
      gtk_container_set_border_width (GTK_CONTAINER (frame), 5);
      gtk_container_set_border_width (GTK_CONTAINER (vbox), 5);
      
      gtk_signal_connect(GTK_OBJECT (entry), "activate",
			 GTK_SIGNAL_FUNC (enter_callback), entry);
      gtk_signal_connect(GTK_OBJECT (next_button), "clicked",
			 GTK_SIGNAL_FUNC (enter_callback), entry);
  
      focus = entry;
    }
  else
    {
      GtkWidget *label_username, *label_fullname;
      GtkWidget *label_password, *label_confirm;
      GtkWidget *hbox_username, *hbox_fullname;
      GtkWidget *hbox_password, *hbox_confirm;
      GtkWidget *table;
      GtkWidget *vbox;

      label_username = gtk_label_new (_("Username"));
      label_fullname = gtk_label_new (_("Full name"));
      label_password = gtk_label_new (_("Password"));
      label_confirm = gtk_label_new (_("Confirm password"));

      entry_username = gtk_entry_new ();
      entry_fullname = gtk_entry_new ();
      entry_password = gtk_entry_new ();
      entry_confirm = gtk_entry_new ();

      hbox_username = gtk_hbox_new (0, FALSE);
      hbox_fullname = gtk_hbox_new (0, FALSE);
      hbox_password = gtk_hbox_new (0, FALSE);
      hbox_confirm = gtk_hbox_new (0, FALSE);

      gtk_entry_set_visibility (GTK_ENTRY (entry_password), FALSE);
      gtk_entry_set_visibility (GTK_ENTRY (entry_confirm), FALSE);

      gtk_box_pack_start (GTK_BOX (hbox_username), label_username, 
			  FALSE, FALSE, 4);
      gtk_box_pack_start (GTK_BOX (hbox_fullname), label_fullname, 
			  FALSE, FALSE, 4);
      gtk_box_pack_start (GTK_BOX (hbox_password), label_password, 
			  FALSE, FALSE, 4);
      gtk_box_pack_start (GTK_BOX (hbox_confirm), label_confirm,
			  FALSE, FALSE, 4);

      table = gtk_table_new (4, 2, FALSE);
      gtk_table_attach_defaults (GTK_TABLE (table), hbox_username, 
				 0, 1, 0, 1);
      gtk_table_attach_defaults (GTK_TABLE (table), hbox_fullname, 
				 0, 1, 1, 2);
      gtk_table_attach_defaults (GTK_TABLE (table), hbox_password,
				 0, 1, 2, 3);
      gtk_table_attach_defaults (GTK_TABLE (table), hbox_confirm, 
				 0, 1, 3, 4);

      gtk_table_attach_defaults (GTK_TABLE (table), entry_username, 
				 1, 2, 0, 1);
      gtk_table_attach_defaults (GTK_TABLE (table), entry_fullname, 
				 1, 2, 1, 2);
      gtk_table_attach_defaults (GTK_TABLE (table), entry_password,
				 1, 2, 2, 3);
      gtk_table_attach_defaults (GTK_TABLE (table), entry_confirm, 
				 1, 2, 3, 4);

      gtk_signal_connect(GTK_OBJECT (entry_username), "activate",
			 GTK_SIGNAL_FUNC (move_callback), entry_fullname);
      gtk_signal_connect(GTK_OBJECT (entry_fullname), "activate",
			 GTK_SIGNAL_FUNC (move_callback), entry_password);
      gtk_signal_connect(GTK_OBJECT (entry_password), "activate",
			 GTK_SIGNAL_FUNC (move_callback), entry_confirm);
      gtk_signal_connect(GTK_OBJECT (entry_confirm), "activate",
			 GTK_SIGNAL_FUNC (enter_newuser_callback), NULL);
      gtk_signal_connect(GTK_OBJECT (next_button), "clicked",
			 GTK_SIGNAL_FUNC (enter_newuser_callback), NULL);

      frame = gtk_frame_new (_("New user"));

      vbox = gtk_vbox_new (FALSE, 0);
      gtk_box_pack_start (GTK_BOX (vbox), table, TRUE, TRUE, 0);
      gtk_box_pack_start (GTK_BOX (vbox), socket, TRUE, TRUE, 0);
      
      gtk_container_add (GTK_CONTAINER (frame), vbox);
      gtk_container_set_border_width (GTK_CONTAINER (frame), 5);
      gtk_container_set_border_width (GTK_CONTAINER (table), 5);

      focus = entry_username;
    }

  gtk_widget_set_usize (next_button, 48, -1);
  gtk_box_pack_end (GTK_BOX (hbox), next_button, FALSE, FALSE, 5);
      
  gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_CENTER);

  gtk_box_pack_start (GTK_BOX (vbox2), frame, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 5);

  gtk_container_add (GTK_CONTAINER (window), vbox2);

  gtk_widget_show_all (window);

  if (xkbd_xid)
    gtk_socket_steal (socket, xkbd_xid);

  gtk_widget_grab_focus (focus);

  gtk_main ();

  cleanup_children ();

  return 0;
}

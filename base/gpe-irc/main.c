/*
 * Copyright (C) 2001, 2002 Damien Tanner <dctanner@magenet.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <glib.h>
#include <locale.h>
#include <libintl.h>
#include <time.h>
#include <stdio.h>

#include <gtk/gtk.h>

#include <gpe/init.h>
#include <gpe/errorbox.h>
#include <gpe/render.h>
#include <gpe/pixmaps.h>
#include <gpe/picturebutton.h>
#include <gpe/question.h>

#include "irc.h"

#define WINDOW_NAME "IRC Client"
#define _(_x) gettext (_x)

struct gpe_icon my_icons[] = {
  { "new", "new" },
  { "delete", "delete" },
  { "edit", "edit" },
  { "properties", "properties" },
  { "close", "close" },
  { "stop", "stop" },
  { "error", "error" },
  { "globe", "irc/globe" },
  { "icon", PREFIX "/share/pixmaps/gpe-irc.png" },
  {NULL, NULL}
};

guint window_x = 240, window_y = 310;

GList *servers = NULL;
gboolean users_list_visible = FALSE;

GtkWidget *main_window;
GtkWidget *main_button_hbox;
GtkWidget *main_entry;
GtkWidget *main_text_view;
GtkWidget *main_hbox;
GtkWidget *users_button_arrow;
GtkWidget *users_button_arrow2;
GtkListStore *users_list_store;
GtkWidget *users_tree_view, *users_scroll;
GtkWidget *nick_label;
GtkTextBuffer *text_buffer;
GtkWidget *scroll;

void
toggle_users_list ()
{
  if (users_list_visible == TRUE)
  {
    gtk_widget_hide (users_scroll);
    gtk_arrow_set (GTK_ARROW (users_button_arrow), GTK_ARROW_LEFT, GTK_SHADOW_NONE);
    gtk_arrow_set (GTK_ARROW (users_button_arrow2), GTK_ARROW_LEFT, GTK_SHADOW_NONE);
    users_list_visible = FALSE;
  }
  else
  {
    gtk_widget_show (users_scroll);
    gtk_arrow_set (GTK_ARROW (users_button_arrow), GTK_ARROW_RIGHT, GTK_SHADOW_NONE);
    gtk_arrow_set (GTK_ARROW (users_button_arrow2), GTK_ARROW_RIGHT, GTK_SHADOW_NONE);
    users_list_visible = TRUE;
  }
}

gchar *selected_type;
IRCServer *selected_server;
IRCChannel *selected_channel;
GtkWidget *selected_button;

void
kill_widget (GtkWidget *parent, GtkWidget *widget)
{
  gtk_widget_destroy (widget);
}

void
update_text_view (gchar *text)
{
  GtkTextBuffer *text_buffer;
  GtkTextIter start, end;
  GtkAdjustment *vadjust;

  if (text)
  {
    text_buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (main_text_view));
    vadjust = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (scroll));

    gtk_text_buffer_get_bounds (text_buffer, &start, &end);
    gtk_text_buffer_insert (text_buffer, &end, text, strlen (text));
    gtk_adjustment_set_value (vadjust, vadjust->upper);
  }
}

void
clear_text_view ()
{
  GtkTextBuffer *text_buffer;
  GtkTextIter start, end;

  text_buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (main_text_view));

  gtk_text_buffer_get_bounds (text_buffer, &start, &end);
  gtk_text_buffer_delete (text_buffer, &start, &end);
}

void
button_clicked (GtkWidget *button)
{
  IRCServer *server;
  //IRCChannel *channel;

  if (button != selected_button)
  {
    if (gtk_object_get_data (GTK_OBJECT (button), "type") == IRC_SERVER)
    {
      server = gtk_object_get_data (GTK_OBJECT (button), "IRCServer");
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (selected_button), FALSE);
      selected_button = button;
      selected_server = server;
      clear_text_view ();
      printf ("Button's text passed: %s\n", server->text->str);
      //update_text_view (server->text->str);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
    }
  }
}

void
do_irc_iter ()
{
  gchar *text = NULL;
  GList *iter = NULL;

  iter = g_list_first (servers);

  while (iter)
  {
    //printf ("----- Reading from irc server %s\n", ((IRCServer *) iter->data)->name);
    text = irc_server_read ((IRCServer *) iter->data);

    if (text)
    {
      if ((IRCServer *) iter->data == selected_server)
	update_text_view (text);

      ((IRCServer *) iter->data)->text = g_string_append (((IRCServer *) iter->data)->text, text);
    }

    iter = iter->next;
  }
}

void
connection_postinit ()
{
  gtk_label_set_text (GTK_LABEL (nick_label), selected_server->user_info->nick);
}

void
new_connection (GtkWidget *parent, GtkWidget *parent_window)
{
  GtkWidget *server_combo_entry, *nick_entry, *real_name_entry, *password_entry, *button;
  IRCServer *server;

  clear_text_view ();

  server = g_malloc (sizeof (*server));
  server->user_info = g_malloc (sizeof (*server->user_info));
  server->text = g_string_new ("");
  server->channel = g_hash_table_new (g_str_hash, g_str_equal);

  server_combo_entry = gtk_object_get_data (GTK_OBJECT (parent), "server_combo_entry");
  nick_entry = gtk_object_get_data (GTK_OBJECT (parent), "nick_entry");
  real_name_entry = gtk_object_get_data (GTK_OBJECT (parent), "real_name_entry");
  password_entry = gtk_object_get_data (GTK_OBJECT (parent), "password_entry");

  server->name = g_strdup (gtk_entry_get_text (GTK_ENTRY (server_combo_entry)));
  server->user_info->nick = g_strdup (gtk_entry_get_text (GTK_ENTRY (nick_entry)));
  server->user_info->username = g_strdup (gtk_entry_get_text (GTK_ENTRY (nick_entry)));
  server->user_info->real_name = g_strdup (gtk_entry_get_text (GTK_ENTRY (real_name_entry)));
  if (strlen (gtk_entry_get_text (GTK_ENTRY (password_entry))) > 0)
    server->user_info->password = g_strdup (gtk_entry_get_text (GTK_ENTRY (password_entry)));
  else
    server->user_info->password = NULL;

  servers = g_list_append (servers, (gpointer) server);
  selected_server = server;

  if (selected_button)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (selected_button), FALSE);

  button = gtk_toggle_button_new_with_label (server->name);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
  gtk_box_pack_start (GTK_BOX (main_button_hbox), button, FALSE, FALSE, 0);
  gtk_object_set_data (GTK_OBJECT (button), "type", (gpointer) IRC_SERVER);
  gtk_object_set_data (GTK_OBJECT (button), "IRCServer", (gpointer) server);
  gtk_signal_connect (GTK_OBJECT (button), "clicked", GTK_SIGNAL_FUNC (button_clicked), NULL);
  gtk_widget_show (button);

  server->button = button;
  selected_button = button;

  gtk_label_set_text (GTK_LABEL (nick_label), server->user_info->nick);

  gtk_widget_destroy (parent_window);

  while (gtk_events_pending ())
    gtk_main_iteration ();

  irc_server_connect (server);
}

void
new_connection_dialog ()
{
  GtkWidget *window, *table, *vbox, *hbox, *button_hbox, *label, *hsep;
  GtkWidget *network_combo, *server_combo, *nick_entry, *real_name_entry, *password_entry;
  GtkWidget *connect_button, *close_button, *network_properties_button;
  GdkPixmap *pmap;
  GdkBitmap *bmap;

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (window), "IRC Client - New Connection");
  gtk_widget_set_usize (GTK_WIDGET (window), window_x, window_y);
  gtk_signal_connect (GTK_OBJECT (window), "destroy",
		      GTK_SIGNAL_FUNC (gtk_widget_destroy), NULL);

  gtk_widget_realize (window);

  vbox = gtk_vbox_new (FALSE, 0);
  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_set_spacing (GTK_BOX (hbox), 3);
  button_hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_set_spacing (GTK_BOX (button_hbox), 6);
  gtk_container_set_border_width (GTK_CONTAINER (button_hbox), 6);

  table = gtk_table_new (2, 5, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);

  label = gtk_label_new ("Network");
  gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 0, 1);
  gtk_misc_set_alignment (GTK_MISC (label), 0, .5);

  label = gtk_label_new ("Server");
  gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 1, 2);
  gtk_misc_set_alignment (GTK_MISC (label), 0, .5);

  label = gtk_label_new ("Nickname");
  gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 2, 3);
  gtk_misc_set_alignment (GTK_MISC (label), 0, .5);

  label = gtk_label_new ("Real Name");
  gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 3, 4);
  gtk_misc_set_alignment (GTK_MISC (label), 0, .5);

  label = gtk_label_new ("Password");
  gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 4, 5);
  gtk_misc_set_alignment (GTK_MISC (label), 0, .5);

  hsep = gtk_hseparator_new ();

  network_combo = gtk_combo_new ();
  server_combo = gtk_combo_new ();

  nick_entry = gtk_entry_new ();
  real_name_entry = gtk_entry_new ();
  password_entry = gtk_entry_new ();

  connect_button = gpe_picture_button (button_hbox->style, "Connect", "globe");
  close_button = gpe_picture_button (button_hbox->style, "Close", "close");
  network_properties_button = gpe_picture_button (button_hbox->style, NULL, "properties");

  gtk_object_set_data (GTK_OBJECT (connect_button), "server_combo_entry", (gpointer) GTK_COMBO (server_combo)->entry);
  gtk_object_set_data (GTK_OBJECT (connect_button), "nick_entry", (gpointer) nick_entry);
  gtk_object_set_data (GTK_OBJECT (connect_button), "real_name_entry", (gpointer) real_name_entry);
  gtk_object_set_data (GTK_OBJECT (connect_button), "password_entry", (gpointer) password_entry);

  gtk_signal_connect (GTK_OBJECT (close_button), "clicked",
    		      GTK_SIGNAL_FUNC (kill_widget), window);
  gtk_signal_connect (GTK_OBJECT (connect_button), "clicked",
    		      GTK_SIGNAL_FUNC (new_connection), window);

  gtk_container_add (GTK_CONTAINER (window), GTK_WIDGET (vbox));
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_box_pack_end (GTK_BOX (vbox), button_hbox, FALSE, FALSE, 0);
  gtk_box_pack_end (GTK_BOX (vbox), hsep, FALSE, FALSE, 0);
  gtk_box_pack_end (GTK_BOX (button_hbox), close_button, FALSE, FALSE, 0);
  gtk_box_pack_end (GTK_BOX (button_hbox), connect_button, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), network_combo, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), network_properties_button, FALSE, FALSE, 0);

  gtk_table_attach_defaults (GTK_TABLE (table), hbox, 1, 2, 0, 1);
  gtk_table_attach_defaults (GTK_TABLE (table), server_combo, 1, 2, 1, 2);
  gtk_table_attach_defaults (GTK_TABLE (table), nick_entry, 1, 2, 2, 3);
  gtk_table_attach_defaults (GTK_TABLE (table), real_name_entry, 1, 2, 3, 4);
  gtk_table_attach_defaults (GTK_TABLE (table), password_entry, 1, 2, 4, 5);

  if (gpe_find_icon_pixmap ("globe", &pmap, &bmap))
    gdk_window_set_icon (window->window, NULL, pmap, bmap);

  gtk_widget_show_all (window);
}

int
main (int argc, char *argv[])
{
  GtkWidget *vbox, *hbox, *users_button_vbox, *users_button_label, *hsep;
  GtkWidget *users_button, *close_button, *new_connection_button;
  GdkPixmap *pmap;
  GdkBitmap *bmap;

  if (gpe_application_init (&argc, &argv) == FALSE)
    exit (1);

  if (gpe_load_icons (my_icons) == FALSE)
    exit (1);

  setlocale (LC_ALL, "");
  
  bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
  textdomain (PACKAGE);

  main_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (main_window), "IRC Client");
  gtk_widget_set_usize (GTK_WIDGET (main_window), window_x, window_y);
  gtk_signal_connect (GTK_OBJECT (main_window), "destroy",
		      GTK_SIGNAL_FUNC (gtk_exit), NULL);

  gtk_widget_realize (main_window);

  vbox = gtk_vbox_new (FALSE, 0);
  hbox = gtk_hbox_new (FALSE, 0);
  main_hbox = gtk_hbox_new (FALSE, 0);
  main_button_hbox = gtk_hbox_new (FALSE, 0);
  users_button_vbox = gtk_vbox_new (FALSE, 0);

  gtk_box_set_spacing (GTK_BOX (main_button_hbox), 3);
  gtk_box_set_spacing (GTK_BOX (hbox), 3);

  hsep = gtk_hseparator_new ();

  scroll = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

  users_scroll = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (users_scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  main_text_view = gtk_text_view_new ();
  gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (main_text_view), GTK_WRAP_WORD);
  gtk_text_view_set_editable (GTK_TEXT_VIEW (main_text_view), FALSE);

  main_entry = gtk_entry_new ();

  users_list_store = gtk_list_store_new (1, G_TYPE_STRING);
  users_tree_view = gtk_tree_view_new_with_model (users_list_store);

  users_button_arrow = gtk_arrow_new (GTK_ARROW_LEFT, GTK_SHADOW_NONE);
  users_button_arrow2 = gtk_arrow_new (GTK_ARROW_LEFT, GTK_SHADOW_NONE);

  close_button = gpe_picture_button (main_button_hbox->style, NULL, "close");
  new_connection_button = gpe_picture_button (hbox->style, NULL, "globe");
  users_button = gtk_button_new ();

  gtk_signal_connect (GTK_OBJECT (new_connection_button), "clicked",
    		      GTK_SIGNAL_FUNC (new_connection_dialog), NULL);
  gtk_signal_connect (GTK_OBJECT (users_button), "clicked",
    		      GTK_SIGNAL_FUNC (toggle_users_list), NULL);

  nick_label = gtk_label_new ("\t");
  users_button_label = gtk_label_new ("u\ns\ne\nr\ns");
  gtk_misc_set_alignment (GTK_MISC (users_button_label), 0.5, 0.5);

  gtk_container_add (GTK_CONTAINER (main_window), GTK_WIDGET (vbox));
  gtk_container_add (GTK_CONTAINER (scroll), GTK_WIDGET (main_text_view));
  gtk_container_add (GTK_CONTAINER (users_scroll), GTK_WIDGET (users_tree_view));
  gtk_container_add (GTK_CONTAINER (users_button), GTK_WIDGET (users_button_vbox));
  gtk_box_pack_start (GTK_BOX (users_button_vbox), users_button_arrow, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (users_button_vbox), users_button_label, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (users_button_vbox), users_button_arrow2, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), main_button_hbox, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hsep, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), main_hbox, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_box_pack_end (GTK_BOX (main_button_hbox), close_button, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (main_hbox), scroll, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (main_hbox), users_button, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (main_hbox), users_scroll, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), nick_label, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), main_entry, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), new_connection_button, FALSE, FALSE, 0);

  if (gpe_find_icon_pixmap ("icon", &pmap, &bmap))
    gdk_window_set_icon (main_window->window, NULL, pmap, bmap);

  gtk_widget_show_all (main_window);
  gtk_widget_hide (users_scroll);

  gtk_widget_grab_focus (main_entry);

  //connection_init (server);

  while (1)
  {
    do_irc_iter ();

    while (gtk_events_pending ())
      gtk_main_iteration ();
  }

  return 0;
}

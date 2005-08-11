/*
 * gpe-mini-browser v0.16
 *
 * Basic web browser based on gtk-webcore 
 * 
 * Interface calls.
 *
 * Copyright (c) 2005 Philippe De Swert
 *
 * Contact : philippedeswert@scarlet.be
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>

#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <webi.h>

#include <gdk/gdk.h>

#include <glib.h>

#include <gpe/init.h>
#include "gpe/pixmaps.h"
#include "gpe/init.h"
#include "gpe/picturebutton.h"
#include <gpe/errorbox.h>
#include <gpe/spacing.h>

#include "gpe-mini-browser.h"

//#define DEBUG /* uncomment this if you want debug output*/

/* ======================================================== */

/* pop up a window to enter an URL */
void
show_url_window (GtkWidget * show, GtkWidget * html)
{
  GtkWidget *url_window, *entry;
  GtkWidget *hbox, *vbox;
  GtkWidget *label;
  GtkWidget *buttonok, *buttoncancel;
  struct url_data *data;

  /* create dialog window */
  url_window = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (url_window), ("Where to go to?"));

  hbox = gtk_hbox_new (FALSE, 0);
  entry = gtk_entry_new ();
  gtk_entry_set_activates_default (GTK_ENTRY(entry), TRUE);
  label = gtk_label_new (("Enter url:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);

  gtk_container_set_border_width (GTK_CONTAINER (url_window),
				  gpe_get_border ());

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), entry, FALSE, FALSE, 0);

  gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (url_window)->vbox),
		      hbox, FALSE, FALSE, 0);

  /* add the buttons */
  buttonok = gpe_button_new_from_stock (GTK_STOCK_OK, GPE_BUTTON_TYPE_BOTH);
  buttoncancel =
    gpe_button_new_from_stock (GTK_STOCK_CANCEL, GPE_BUTTON_TYPE_BOTH);

  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (url_window)->action_area),
		      buttoncancel, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (url_window)->action_area),
		      buttonok, TRUE, TRUE, 0);

  GTK_WIDGET_SET_FLAGS (buttonok, GTK_CAN_DEFAULT);
  gtk_widget_grab_default (buttonok);

  data = malloc (sizeof (struct url_data));
  data->html = html;
  data->entry = entry;
  data->window = url_window;

  /* add button callbacks */
  g_signal_connect (GTK_OBJECT (buttonok), "clicked",
		    G_CALLBACK (load_text_entry), (gpointer *) data);
  g_signal_connect (GTK_OBJECT (buttoncancel), "clicked",
		    G_CALLBACK (destroy_window), (gpointer *) url_window);
  g_signal_connect (GTK_OBJECT (buttonok), "clicked",
		    G_CALLBACK (destroy_window), (gpointer *) url_window);
  g_signal_connect (GTK_OBJECT (entry), "activate",
                        G_CALLBACK (load_text_entry), (gpointer *) data);
  g_signal_connect (GTK_OBJECT (entry), "activate",
                        G_CALLBACK (destroy_window), (gpointer *) url_window);

  gtk_widget_show_all (url_window);
  gtk_widget_grab_focus (entry);
}

/* ======================================================== */

void
destroy_window (GtkButton * button, gpointer * window)
{
  gtk_widget_destroy (GTK_WIDGET (window));
}

/* ======================================================== */

void
create_status_window (Webi * html, gpointer * status_data)
{
  GtkWidget *statusbox, *pbar, *label;
  struct status_data *data;

  data = (struct status_data *) status_data;
  /* the stop signal is not always generated. Like when you click on a link in a page
  before it is fully loaded. So to avoid several status bars to appear (and not all 
  disappearing), check if there is already one and remoce it. */
  if (data->exists == TRUE)
  {
   gtk_widget_destroy (GTK_WIDGET (data->statusbox));
  }	

#ifdef DEBUG
  printf ("status = loading\n");
#endif
  gtk_tool_button_set_stock_id(GTK_TOOL_BUTTON(stop_reload_button), "gtk-stop"); 
  gtk_tool_button_set_icon_widget(GTK_TOOL_BUTTON(stop_reload_button), NULL);

  statusbox = gtk_hbox_new (FALSE, 0);
  pbar = gtk_progress_bar_new ();
  label = gtk_label_new ("loading");

  data->statusbox = statusbox;
  data->pbar = pbar;
  data->exists = TRUE;

  gtk_box_pack_end (GTK_BOX (data->main_window), statusbox, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (statusbox), label, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (statusbox), GTK_WIDGET (pbar), FALSE, FALSE,
		      0);

  gtk_widget_show (statusbox);
  gtk_widget_show (GTK_WIDGET (pbar));
  gtk_widget_show (label);
}

/* ======================================================== */

void
destroy_status_window (Webi * html, gpointer * status_data)
{
  struct status_data *data;

  data = (struct status_data *) status_data;

#ifdef DEBUG
  printf ("loading stopped\n");
#endif
  gtk_tool_button_set_stock_id(GTK_TOOL_BUTTON(stop_reload_button), "gtk-refresh"); 
  gtk_tool_button_set_icon_widget(GTK_TOOL_BUTTON(stop_reload_button), NULL);

  if (data->exists == TRUE)
    {
      data->exists = FALSE;
      /* set pbar to NULL for testing in activate_statusbar (we do not want to access a destroyed widget) */
      data->pbar = NULL;
      gtk_widget_destroy (GTK_WIDGET (data->statusbox));
    }
}

/* ======================================================== */

void
activate_statusbar (Webi * html, WebiLoadStatus * status,
		    gpointer status_data)
{
  gdouble fraction = 0.0;
  struct status_data *data;

#ifdef DEBUG
  printf ("progressbar status changed\n");
#endif

  data = (struct status_data *) status_data;

  /* test if an error occured */
  if (status->status == WEBI_LOADING_ERROR)
    {
      gpe_error_box ("An error occured loading the webpage!");
    }

  /* copied from the reference implementation of osb-browser, needs to be improved for this app */
  if (status->filesWithSize < status->files)
    {
      gdouble ratio =	(gdouble) status->filesWithSize / (gdouble) status->files;
      fraction += status->received >=	status->size ? ratio : ratio * (gdouble) status->received /
	(gdouble) status->size;
      fraction += status->ready >= status->files ? (1.0 - ratio) : (1.0 - ratio) * 
		(gdouble) (status->ready - status->filesWithSize) /
	(gdouble) (status->files - status->filesWithSize);
    }
  else
    {
      fraction = (gdouble) status->received / (gdouble) status->size;
    }
  if (fraction > 1.0)
    {
      fraction = 1.0;
    }
  /* see if the widget still exists */
  if (data->pbar != NULL)
    gtk_progress_bar_set_fraction ((GtkProgressBar *) data->pbar, fraction);
}

/* ======================================================== */

void
set_title (Webi * html, GtkWidget * app_window)
{
  const gchar *title;

  title = webi_get_title (html);
  title = g_strconcat (title, " - mini-browser", NULL);
  gtk_window_set_title (GTK_WINDOW (app_window), title);
  g_free ((gpointer *) title);

}

/* ======================================================== */

void
update_text_entry (Webi * html, GtkWidget * entrybox)
{
  const gchar *location;

  location = webi_get_location (html);
  if(entrybox != NULL)
	  gtk_entry_set_text (GTK_ENTRY (entrybox), location);

}

/* ======================================================== */

GtkWidget * show_big_screen_interface ( Webi *html, GtkWidget *toolbar, WebiSettings *set)
{
      GtkWidget *urlbox;
      GtkToolItem *zoom_in_button, *zoom_out_button, *sep, *sep2, *hide_button;
      struct urlbar_data *hide;

      urlbox = create_url_bar (html);
     
      /* fill in the data needed for the zoom functionality */
      struct zoom_data *zoom;

      zoom = malloc(sizeof(struct zoom_data));
      zoom->html = html;
      zoom->settings = set;

      /* add extra zoom in/out buttons + spacing to the toolbar */ 

      zoom_in_button = gtk_tool_button_new_from_stock (GTK_STOCK_ZOOM_IN);
      gtk_tool_item_set_homogeneous(zoom_in_button, FALSE);
      gtk_toolbar_insert (GTK_TOOLBAR(toolbar), zoom_in_button, -1);

      zoom_out_button = gtk_tool_button_new_from_stock (GTK_STOCK_ZOOM_OUT);
      gtk_tool_item_set_homogeneous(zoom_out_button, FALSE);
      gtk_toolbar_insert (GTK_TOOLBAR(toolbar), zoom_out_button, -1);

      sep = gtk_separator_tool_item_new();
      gtk_tool_item_set_homogeneous(sep, FALSE);
      gtk_toolbar_insert (GTK_TOOLBAR(toolbar), sep, -1);

      hide_button = gtk_tool_button_new_from_stock (GTK_STOCK_UNDO);
      gtk_tool_item_set_homogeneous(hide_button, FALSE);
      gtk_tool_button_set_label (GTK_TOOL_BUTTON(hide_button), "Hide Url");
      gtk_toolbar_insert (GTK_TOOLBAR(toolbar), hide_button, -1); 

      sep2 = gtk_separator_tool_item_new();
      gtk_tool_item_set_homogeneous(sep2, FALSE);
      gtk_toolbar_insert (GTK_TOOLBAR(toolbar), sep2, -1);

      /* fill in info for hiding */

      hide = malloc(sizeof(struct urlbar_data));
      hide->urlbox = urlbox;
      hide->hidden = 0;  /* when hidden this value will be 1 */     
      hide->hiding_button = hide_button;

      /* add button callbacks */ 
      g_signal_connect (GTK_OBJECT (zoom_in_button), "clicked",
			G_CALLBACK (zoom_in), zoom);
      g_signal_connect (GTK_OBJECT (zoom_out_button), "clicked",
			G_CALLBACK (zoom_out), zoom);
      g_signal_connect (GTK_OBJECT (hide_button), "clicked",
			G_CALLBACK (hide_url_bar), hide);

      return(urlbox);
}

/* ======================================================== */

GtkWidget * create_url_bar (Webi *html)
{
     struct url_data *data;
     GtkWidget *urlbox, *urllabel, *urlentry, *okbutton;

      /* create all necessary widgets */
      urlbox = gtk_hbox_new (FALSE, 0);
      urllabel = gtk_label_new ((" Url:"));
      gtk_misc_set_alignment (GTK_MISC (urllabel), 0.0, 0.5);
      urlentry = gtk_entry_new ();
      gtk_entry_set_activates_default (GTK_ENTRY(urlentry), TRUE);
      okbutton =
        gpe_button_new_from_stock (GTK_STOCK_OK, GPE_BUTTON_TYPE_BOTH);

      /* pack everything in the hbox */
      gtk_box_pack_start (GTK_BOX (urlbox), urllabel, FALSE, FALSE, 0);
      gtk_box_pack_start (GTK_BOX (urlbox), urlentry, TRUE, TRUE, 5);
      gtk_box_pack_start (GTK_BOX (urlbox), okbutton, FALSE, FALSE, 10);

      data = malloc (sizeof (struct url_data));
      data->html = (GtkWidget *)html;
      data->entry = urlentry;
      data->window = NULL;      /* set to NULL to be easy to recognize to avoid freeing in load_text_entry  as this window is not destroyed unlike the pop-up */

      g_signal_connect (GTK_OBJECT (okbutton), "clicked",
                        G_CALLBACK (load_text_entry), (gpointer *) data);
      g_signal_connect (GTK_OBJECT (html), "location",
                        G_CALLBACK (update_text_entry),
                        (gpointer *) urlentry);
      g_signal_connect (GTK_OBJECT (urlentry), "activate",
                        G_CALLBACK (load_text_entry), (gpointer *) data);

      gtk_widget_grab_focus (urlentry);
      /*final settings */
      GTK_WIDGET_SET_FLAGS (okbutton, GTK_CAN_DEFAULT);
      gtk_button_set_relief (GTK_BUTTON (okbutton), GTK_RELIEF_NONE);
      gtk_widget_grab_default (okbutton);

      return (urlbox);
}

/* ======================================================== */

void hide_url_bar (GtkWidget * button, struct urlbar_data * url_bar)
{
     struct urlbar_data *data = NULL;

     data = (struct urlbar_data *) url_bar;

     if(data->hidden != 1)
	{
		gtk_widget_hide(data->urlbox);
		data->hidden = 1;
  		gtk_tool_button_set_stock_id(GTK_TOOL_BUTTON(data->hiding_button), "gtk-redo"); 
	        gtk_tool_button_set_icon_widget(GTK_TOOL_BUTTON(data->hiding_button), NULL);
     	        gtk_tool_button_set_label (GTK_TOOL_BUTTON(data->hiding_button), "Show Url");
	}
     else
	{
		gtk_widget_show_all(data->urlbox);
		data->hidden = 0;
  		gtk_tool_button_set_stock_id(GTK_TOOL_BUTTON(data->hiding_button), "gtk-undo"); 
	        gtk_tool_button_set_icon_widget(GTK_TOOL_BUTTON(data->hiding_button), NULL);
      		gtk_tool_button_set_label (GTK_TOOL_BUTTON(data->hiding_button), "Hide Url");
	}	

}

/* ======================================================== */

void show_bookmarks (GtkWidget * button, Webi * html)
{
	GtkWidget *bookmarks_window, *bookbox;
	GtkToolbar *booktool;
	GtkToolItem *add_current, *add_new, *del, *new_category, *del_category;

	bookmarks_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        gtk_window_set_title(GTK_WINDOW(bookmarks_window), "Bookmarks");
	//temporary debug code
 	gtk_window_set_default_size (GTK_WINDOW (bookmarks_window), 800, 480);

	bookbox = gtk_vbox_new(FALSE, 0);
	booktool = GTK_TOOLBAR(gtk_toolbar_new());

	add_current = gtk_tool_button_new_from_stock (GTK_STOCK_ADD);
	gtk_tool_item_set_homogeneous (add_current, FALSE);
        gtk_tool_button_set_label (GTK_TOOL_BUTTON(add_current), "Add Current");
	gtk_toolbar_insert (booktool, add_current, -1);
	
	add_new = gtk_tool_button_new_from_stock (GTK_STOCK_NEW);
	gtk_tool_item_set_homogeneous (add_new, FALSE);
        gtk_tool_button_set_label (GTK_TOOL_BUTTON(add_new), "Add New");
	gtk_toolbar_insert (booktool, add_new, -1);
	
	del = gtk_tool_button_new_from_stock (GTK_STOCK_REMOVE);
	gtk_tool_item_set_homogeneous (del, FALSE);
	gtk_toolbar_insert (booktool, del, -1);

	new_category = gtk_tool_button_new_from_stock (GTK_STOCK_EDIT);
	gtk_tool_item_set_homogeneous (new_category, FALSE);
        gtk_tool_button_set_label (GTK_TOOL_BUTTON(new_category), "New Category");
	gtk_toolbar_insert (booktool, new_category, -1);

	del_category = gtk_tool_button_new_from_stock (GTK_STOCK_DELETE);
	gtk_tool_item_set_homogeneous (del_category, FALSE);
	gtk_tool_button_set_label (GTK_TOOL_BUTTON(del_category), "Delete Category");
	gtk_toolbar_insert (booktool, del_category, -1);
     
	gtk_box_pack_start(GTK_BOX(bookbox), GTK_WIDGET(booktool), FALSE, FALSE, 0);
	gtk_container_add(GTK_CONTAINER(bookmarks_window), bookbox);
	
	gtk_widget_show_all(bookmarks_window);

}

/*
 * Copyright (C) 2001, 2002 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

/*
 * $Id$
 *
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <libintl.h>
#include <stdlib.h>
#include <string.h>

#include <gpe/init.h>
#include <gpe/pixmaps.h>
#include <gpe/smallbox.h>
#include <gpe/errorbox.h>
#include <gpe/render.h>
#include <gpe/gtkdatecombo.h>
#include <gpe/question.h>
#include <gpe/gtksimplemenu.h>

#include "interface.h"
#include "support.h"
#include "db.h"
#include "structure.h"
#include "proto.h"

#define MY_PIXMAPS_DIR PREFIX "/share/gpe-contacts/pixmaps"

static GtkWidget *categories_smenu;
GtkWidget *clist;
gchar *active_chars;

struct gpe_icon my_icons[] = {
  { "delete" },
  { "new" },
  { "save" },
  { "cancel" },
  { "properties" },
  { "frame", MY_PIXMAPS_DIR "/frame.xpm" },
  { "notebook", MY_PIXMAPS_DIR "/notebook.xpm" },
  { "entry", MY_PIXMAPS_DIR "/entry.xpm" },
  { NULL, NULL }
};

static void
update_categories (void)
{
  GSList *categories = db_get_categories (), *iter;
  GtkWidget *menu = categories_smenu;

  gtk_simple_menu_flush (menu);

  gtk_simple_menu_append_item (menu, _("All categories"));

  for (iter = categories; iter; iter = iter->next)
    {
      struct category *c = iter->data;
      gtk_simple_menu_append_item (menu, c->name);
    }

  for (iter = categories; iter; iter = iter->next)
    {
      struct category *c = iter->data;
      g_free (c->name);
      g_free (c);
    }

  g_slist_free (categories);
}

static void
store_special_fields (GtkWidget *edit, struct person *p)
{
  GSList *l, *bl;
  GtkWidget *w = lookup_widget (edit, "datecombo");
  struct tag_value *v = p ? db_find_tag (p, "BIRTHDAY") : NULL;
  if (v && v->value)
    {
      guint year, month, day;
      sscanf (v->value, "%04d%02d%02d", &year, &month, &day);
      gtk_date_combo_set_date (GTK_DATE_COMBO (w), year, month, day);
    }
  else
    gtk_date_combo_clear (GTK_DATE_COMBO (w));

  if (p)
    {
      bl = gtk_object_get_data (GTK_OBJECT (edit), "category-widgets");
      if (bl)
	{
	  for (l = p->data; l; l = l->next)
	    {
	      v = l->data;
	      if (!strcmp (v->tag, "CATEGORY")
		  && v->value)
		{
		  guint c = atoi (v->value);
		  GSList *i;
		  for (i = bl; i; i = i->next)
		    {
		      GtkWidget *w = i->data;
		      if ((guint)gtk_object_get_data (GTK_OBJECT (w), "category") == c)
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w), TRUE);
		    }
		}
	    }
	}
    }
}

void
edit_person (struct person *p)
{
  GtkWidget *w = edit_window ();
  GtkWidget *entry = lookup_widget (w, "name_entry");
  if (p)
    {
      GSList *tags = gtk_object_get_data (GTK_OBJECT (w), "tag-widgets");
      GSList *iter;
      for (iter = tags; iter; iter = iter->next)
	{
	  GtkWidget *w = iter->data;
	  gchar *tag = gtk_object_get_data (GTK_OBJECT (w), "db-tag");
	  struct tag_value *v = db_find_tag (p, tag);
	  guint pos = 0;
	  if (v && v->value)
	    gtk_editable_insert_text (GTK_EDITABLE (w), v->value, strlen (v->value), &pos);
	}
      gtk_object_set_data (GTK_OBJECT (w), "person", p);
    }
  store_special_fields (w, p);
  gtk_widget_show (w);
  gtk_widget_grab_focus (entry);
}

static void
new_contact(GtkWidget *widget, gpointer d)
{
  edit_person (NULL);
}

static void
delete_contact(GtkWidget *widget, gpointer d)
{
  if (GTK_CLIST (clist)->selection)
    {
      guint row = (guint)(GTK_CLIST (clist)->selection->data);
      guint uid = (guint)gtk_clist_get_row_data (GTK_CLIST (clist), row);
      if (gpe_question_ask (_("Really delete this contact?"), _("Confirm"), 
			    "question", _("Delete"), "delete", _("Cancel"), "cancel", NULL) == 0)
	{
	  if (db_delete_by_uid (uid))
	    update_display ();
	}
    }
}

static void
new_category (GtkWidget *w, gpointer p)
{
  gchar *name = smallbox(_("New Category"), _("Name"), "");
  if (name && name[0])
    {
      gchar *line_info[1];
      guint id;
      line_info[0] = name;
      if (db_insert_category (name, &id))
	gtk_clist_append (GTK_CLIST (p), line_info);
    }

  update_categories ();
}

static void
delete_category (GtkWidget *w, gpointer p)
{
  update_categories ();
}

static GtkWidget *
config_categories_box(void)
{
  GtkWidget *box = gtk_vbox_new (FALSE, 0);
  GtkWidget *clist = gtk_clist_new (1);
  GtkWidget *toolbar;
  GtkWidget *scrolled = gtk_scrolled_window_new (NULL, NULL);
  GtkWidget *pw;
  GSList *categories;

#if GTK_MAJOR_VERSION < 2
  toolbar = gtk_toolbar_new (GTK_ORIENTATION_HORIZONTAL, GTK_TOOLBAR_ICONS);
  gtk_toolbar_set_button_relief (GTK_TOOLBAR (toolbar), GTK_RELIEF_NONE);
  gtk_toolbar_set_space_style (GTK_TOOLBAR (toolbar), GTK_TOOLBAR_SPACE_LINE);
#else
  toolbar = gtk_toolbar_new ();
  gtk_toolbar_set_orientation (GTK_TOOLBAR (toolbar), GTK_ORIENTATION_HORIZONTAL);
  gtk_toolbar_set_style (GTK_TOOLBAR (toolbar), GTK_TOOLBAR_ICONS);
#endif

  gtk_widget_show (toolbar);

  pw = gpe_render_icon (NULL, gpe_find_icon ("new"));
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("New"), 
			   _("New"), _("New"), pw, (GtkSignalFunc)new_category, clist);

  pw = gpe_render_icon (NULL, gpe_find_icon ("delete"));
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Delete"), 
			   _("Delete"), _("Delete"), pw, 
			   (GtkSignalFunc)delete_category, clist);

  categories = db_get_categories ();
  if (categories)
    {
      GSList *iter;
      guint row = 0;
      
      for (iter = categories; iter; iter = iter->next)
	{
	  gchar *line_info[1];
	  struct category *c = iter->data;
	  line_info[0] = c->name;
	  gtk_clist_append (GTK_CLIST (clist), line_info);
	  gtk_clist_set_row_data (GTK_CLIST (clist), row, 
				  (gpointer) c->id);
	  g_free (c->name);
	  g_free (c);
	  row ++;
	}

      g_slist_free (categories);
    }

  gtk_container_add (GTK_CONTAINER (scrolled), clist);
  gtk_widget_show (clist);
  gtk_widget_show (scrolled);

  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
				  GTK_POLICY_NEVER,
				  GTK_POLICY_AUTOMATIC);

  gtk_box_pack_start (GTK_BOX (box), toolbar, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (box), scrolled, TRUE, TRUE, 0);

  gtk_widget_show (box);
  return box;
}

static void
configure (GtkWidget *widget, gpointer d)
{
  GtkWidget *window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  GtkWidget *notebook = gtk_notebook_new ();
  GtkWidget *editlabel = gtk_label_new (_("Editing Layout"));
  GtkWidget *editbox = edit_structure ();
  GtkWidget *categorieslabel = gtk_label_new (_("Categories"));
  GtkWidget *categoriesbox = config_categories_box ();

  gtk_widget_show (notebook);
  gtk_widget_show (editlabel);
  gtk_widget_show (editbox);
  gtk_widget_show (categorieslabel);
  gtk_widget_show (categoriesbox);

  gtk_notebook_append_page (GTK_NOTEBOOK (notebook),
			    editbox, editlabel);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook),
			    categoriesbox, categorieslabel);

  gtk_container_add (GTK_CONTAINER (window), notebook);

  gtk_widget_set_usize (window, 240, 300);

  gtk_widget_show (window);
}

static int
show_details (struct person *p)
{
  gtk_label_set_text (GTK_LABEL (lookup_widget (GTK_WIDGET (clist), "lPhone")), "");
  gtk_label_set_text (GTK_LABEL (lookup_widget (GTK_WIDGET (clist), "lMobile")), "");
  gtk_label_set_text (GTK_LABEL (lookup_widget (GTK_WIDGET (clist), "lMail")), "");
  gtk_label_set_text (GTK_LABEL (lookup_widget (GTK_WIDGET (clist), "lFAX")), "");

  if (p)
    {
      GSList *iter;

      for (iter = p->data; iter; iter = iter->next)
	{
	  struct tag_value *t = iter->data;
	  
	  if (strstr (t->tag, "TELEPHONE") != NULL)
	    gtk_label_set_text (GTK_LABEL (lookup_widget (GTK_WIDGET (clist), "lPhone")), t->value);
	  
	  if (strstr (t->tag, "MOBILE") != NULL)
	    gtk_label_set_text (GTK_LABEL (lookup_widget (GTK_WIDGET (clist), "lMobile")), t->value);
	  
	  if (strstr (t->tag, "EMAIL") != NULL)
	    gtk_label_set_text (GTK_LABEL (lookup_widget (GTK_WIDGET (clist), "lMail")), t->value);
	  
	  if (strstr (t->tag, "FAX") != NULL)
	    gtk_label_set_text (GTK_LABEL (lookup_widget (GTK_WIDGET (clist), "lFAX")), t->value);
	}
    }

  return 0;
}

void 
selection_made (GtkWidget      *clist,
		gint            row,
		gint            column,
		GdkEventButton *event,
		GtkWidget      *widget)
{
  guint id;
  struct person *p;

  id = (guint)gtk_clist_get_row_data (GTK_CLIST (clist), row);
  p = db_get_by_uid (id);
  show_details (p);
    
  if (event->type == GDK_2BUTTON_PRESS)
    {
      id = (guint)gtk_clist_get_row_data (GTK_CLIST (clist), row);
      edit_person (p);
    }
  else
    discard_person (p);
}

void
update_display (void)
{
  GSList *items = db_get_entries_alpha (active_chars), *iter;

  gtk_clist_freeze(GTK_CLIST(clist));
  gtk_clist_clear (GTK_CLIST (clist));

  for (iter = items; iter; iter = iter->next)
    {
      struct person *p = iter->data;
      gchar *text[2];
      int row;
      text[0] = p->name;
      text[1] = NULL;
      row = gtk_clist_append (GTK_CLIST (clist), text);
      gtk_clist_set_row_data (GTK_CLIST (clist), row, (gpointer)p->id);
      discard_person (p);
    }
  
  g_slist_free (items);
  gtk_clist_thaw(GTK_CLIST(clist));
}

int
main (int argc, char *argv[])
{
  GtkWidget *mainw;
  GtkWidget *toolbar;
  GtkWidget *pw;
  GtkWidget *vbox1;
  GtkWidget *hbox1;
  GtkWidget *smenu;

#ifdef ENABLE_NLS
  bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
  textdomain (PACKAGE);
#endif
	
  active_chars = malloc (5*sizeof (gchar));
  sprintf (active_chars, "ABCD");

  if (gpe_application_init (&argc, &argv) == FALSE)
    exit (1);

  if (gpe_load_icons (my_icons) == FALSE)
    exit (1);

  if (db_open ())
    exit (1);

  load_well_known_tags ();

  mainw = create_main ();
  smenu = gtk_simple_menu_new ();
  hbox1 = lookup_widget (GTK_WIDGET (mainw), "hbox3");
  gtk_box_pack_start (GTK_BOX (hbox1), smenu, TRUE, TRUE, 0);
  gtk_widget_show (smenu);
  categories_smenu = smenu;
  update_categories ();
  clist = lookup_widget (GTK_WIDGET (mainw), "clist1");
  update_display ();
  show_details (NULL);
  gtk_widget_show (mainw);
  gtk_signal_connect (GTK_OBJECT (clist), "select_row",
                       GTK_SIGNAL_FUNC (selection_made),
                       NULL);

  gtk_signal_connect (GTK_OBJECT (mainw), "destroy",
		      gtk_main_quit, NULL);

  vbox1 = lookup_widget (mainw, "vbox1");
#if GTK_MAJOR_VERSION < 2
  toolbar = gtk_toolbar_new (GTK_ORIENTATION_HORIZONTAL, GTK_TOOLBAR_ICONS);
#else
  toolbar = gtk_toolbar_new ();
  gtk_toolbar_set_orientation (GTK_TOOLBAR (toolbar), GTK_ORIENTATION_HORIZONTAL);
  gtk_toolbar_set_style (GTK_TOOLBAR (toolbar), GTK_TOOLBAR_ICONS);
#endif
  gtk_box_pack_start (GTK_BOX (vbox1), toolbar, FALSE, FALSE, 0);
  gtk_box_reorder_child (GTK_BOX (vbox1), toolbar, 0);
  gtk_widget_show (toolbar);

  pw = gpe_render_icon (mainw->style, gpe_find_icon ("new"));
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("New Contact"), 
			   _("New Contact"), _("New Contact"),
			   pw, (GtkSignalFunc)new_contact, NULL);

  pw = gpe_render_icon (mainw->style, gpe_find_icon ("delete"));
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Delete Contact"), 
			   _("Delete Contact"), _("Delete Contact"), 
			   pw, (GtkSignalFunc)delete_contact, NULL);

  pw = gpe_render_icon (mainw->style, gpe_find_icon ("properties"));
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Configure"), 
			   _("Configure"), _("Configure"),
			   pw, (GtkSignalFunc)configure, NULL);

  load_structure ();

  gtk_main ();
  return 0;
}

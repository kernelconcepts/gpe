/*
 * Copyright (C) 2002, 2003 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <signal.h>
#include <gtk/gtk.h>

#include <gpe/errorbox.h>
#include <gpe/pixmaps.h>
#include <gpe/gtkdatecombo.h>
#include <gpe/picturebutton.h>

#include "interface.h"
#include "support.h"
#include "structure.h"
#include "callbacks.h"
#include "db.h"

static void
add_tag (gchar *tag, GtkWidget *w, GtkWidget *pw)
{
  GSList *tags;
  gtk_object_set_data (GTK_OBJECT (w), "db-tag", tag);

  tags = gtk_object_get_data (GTK_OBJECT (pw), "tag-widgets");
  tags = g_slist_append (tags, w);
  gtk_object_set_data (GTK_OBJECT (pw), "tag-widgets", tags);
}

static void
pop_singles (GtkWidget *vbox, GSList *list, GtkWidget *pw)
{
  if (list)
    {
      guint l = g_slist_length (list);
      GtkWidget *table = gtk_table_new (l, 2, FALSE);
      guint x = 0;
      
      while (list)
	{
	  GSList *next = list->next;
	  edit_thing_t e = list->data;
	  GtkWidget *w = gtk_entry_new ();

	  add_tag (e->tag, w, pw);

	  gtk_table_attach (GTK_TABLE (table),
			    gtk_label_new (e->name),
			    0, 1, x, x + 1,
			    0, 0, 0, 0);
	  gtk_table_attach (GTK_TABLE (table),
			    w,
			    1, 2, x, x + 1,
			    GTK_EXPAND | GTK_SHRINK | GTK_FILL,
			    0, 0, 0);

	  g_slist_free_1 (list);
	  list = next;
	  x++;
	}

      gtk_table_set_col_spacings (GTK_TABLE (table), 2);
      gtk_container_set_border_width (GTK_CONTAINER (table), 2);

      gtk_box_pack_start (GTK_BOX (vbox), table, TRUE, TRUE, 4);
    }
}

static void
build_children (GtkWidget *vbox, GSList *children, GtkWidget *pw)
{
  GSList *child;
  GSList *singles = NULL;

  for (child = children; child; child = child->next)
    {
      edit_thing_t e = child->data;
      GtkWidget *w, *ww;
      
      switch (e->type)
	{
	case GROUP:
	  pop_singles (vbox, singles, pw);
	  singles = NULL;
	  w = gtk_frame_new (e->name);
	  ww = gtk_vbox_new (FALSE, 0);
	  gtk_container_add (GTK_CONTAINER (w), ww);
	  gtk_container_set_border_width (GTK_CONTAINER (w), 2);
	  build_children (ww, e->children, pw);
	  gtk_box_pack_start (GTK_BOX (vbox), w, FALSE, FALSE, 4);
	  break;

	case ITEM_MULTI_LINE:
	  pop_singles (vbox, singles, pw);
	  singles = NULL;
	  ww = gtk_text_view_new ();
	  gtk_text_view_set_editable (GTK_TEXT_VIEW (ww), TRUE);
	  gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (ww), GTK_WRAP_WORD);
	  gtk_widget_set_usize (GTK_WIDGET (ww), -1, 64);
	  if (e->name)
	    {
	      w = gtk_frame_new (e->name);
	      gtk_container_add (GTK_CONTAINER (w), ww);
	      gtk_container_set_border_width (GTK_CONTAINER (w), 2);
	    }
	  else
	    w = ww;
	  gtk_box_pack_start (GTK_BOX (vbox), w, TRUE, TRUE, 4);
	  add_tag (e->tag, ww, pw);
	  break;

	case ITEM_SINGLE_LINE:
	  singles = g_slist_append (singles, e);
	  break;

	default:
	  abort ();
	}
    }

  pop_singles (vbox, singles, pw);
  singles = NULL;
}

static void
destroy_category_widgets (gpointer p)
{
  g_slist_free ((GSList *)p);
}

static GtkWidget*
create_edit (void)
{
  GtkWidget *edit;
  GtkWidget *notebook2;
  GtkWidget *table1;
  GtkWidget *edit_bt_name;
  GtkWidget *label19;
  GtkWidget *label20;
  GtkWidget *label22;
  GtkWidget *hbox3;
  GtkWidget *datecombo;
  GtkWidget *edit_bt_bdate;
  GtkWidget *hbox4;
  GtkWidget *edit_bt_image;
  GtkWidget *scrolledwindow3;
  GtkWidget *cbox;
  GtkWidget *name_entry;
  GtkWidget *summary_entry;
  GtkWidget *label16;
  GtkWidget *edit_cancel;
  GtkWidget *edit_save;
  GtkWidget *catframe;
  GtkTooltips *tooltips;
  GtkWidget *topvbox;
  GSList *categories = db_get_categories ();
  GtkWidget *vbox, *action_area;

  tooltips = gtk_tooltips_new ();

  edit = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (edit), _("Edit Contact"));
  gpe_set_window_icon (edit, "icon");

  vbox = GTK_DIALOG (edit)->vbox;
  action_area = GTK_DIALOG (edit)->action_area;

  notebook2 = gtk_notebook_new ();
  gtk_box_pack_start (GTK_BOX (vbox), notebook2, 
		      TRUE, TRUE, 0);

  topvbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (notebook2), topvbox);

  table1 = gtk_table_new (4, 2, FALSE);
  gtk_box_pack_start (GTK_BOX (topvbox), table1, FALSE, FALSE, 2);
  gtk_table_set_row_spacings (GTK_TABLE (table1), 2);
  gtk_table_set_col_spacings (GTK_TABLE (table1), 3);

  edit_bt_name = gtk_button_new_with_label (_("Name"));
  gtk_table_attach (GTK_TABLE (table1), edit_bt_name, 0, 1, 0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_tooltips_set_tip (tooltips, edit_bt_name, _("more detailed name options"), NULL);

  label19 = gtk_label_new (_("Summary"));
  gtk_table_attach (GTK_TABLE (table1), label19, 0, 1, 1, 2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label19), 0.5, 0.5);

  label20 = gtk_label_new (_("Birthday"));
  gtk_table_attach (GTK_TABLE (table1), label20, 0, 1, 2, 3,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label20), 0.5, 0.5);

  label22 = gtk_label_new (_("Image"));
  gtk_table_attach (GTK_TABLE (table1), label22, 0, 1, 3, 4,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label22), 0.5, 0.5);

  hbox3 = gtk_hbox_new (FALSE, 0);
  gtk_table_attach (GTK_TABLE (table1), hbox3, 1, 2, 2, 3,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_FILL), 0, 0);

  datecombo = gtk_date_combo_new ();
  gtk_box_pack_start (GTK_BOX (hbox3), datecombo, TRUE, TRUE, 0);
  GTK_WIDGET_UNSET_FLAGS (datecombo, GTK_CAN_FOCUS);
  GTK_WIDGET_UNSET_FLAGS (datecombo, GTK_CAN_DEFAULT);

  edit_bt_bdate = gtk_check_button_new_with_label (_("Schedule"));
  gtk_widget_set_name (edit_bt_bdate, "edit_bt_bdate");
  gtk_widget_set_sensitive (edit_bt_bdate, FALSE);		/* XXX */
  gtk_box_pack_start (GTK_BOX (hbox3), edit_bt_bdate, TRUE, TRUE, 0);
  gtk_tooltips_set_tip (tooltips, edit_bt_bdate, _("automatic appointment"), NULL);

  hbox4 = gtk_hbox_new (FALSE, 0);
  gtk_table_attach (GTK_TABLE (table1), hbox4, 1, 2, 3, 4,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_FILL), 0, 0);

  edit_bt_image = gtk_button_new_with_label ("");
  gtk_box_pack_start (GTK_BOX (hbox4), edit_bt_image, TRUE, TRUE, 0);
  gtk_tooltips_set_tip (tooltips, edit_bt_image, _("click to choose file"), NULL);

  catframe = gtk_frame_new (_("Categories"));
  scrolledwindow3 = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow3), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_container_add (GTK_CONTAINER (catframe), scrolledwindow3);
  gtk_box_pack_start (GTK_BOX (topvbox), catframe, TRUE, TRUE, 2);

  cbox = gtk_vbox_new (FALSE, 0);
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolledwindow3), 
					 cbox);

  if (categories)
    {
      GSList *iter;
      GSList *category_widgets = NULL;
      for (iter = categories; iter; iter = iter->next)
	{
	  struct category *c = iter->data;
	  GtkWidget *w = gtk_check_button_new_with_label (c->name);
	  gtk_object_set_data (GTK_OBJECT (w), "category", (gpointer)c->id);
	  gtk_widget_show (w);
	  gtk_box_pack_start (GTK_BOX (cbox), w, FALSE, FALSE, 0);
	  g_free (c->name);
	  g_free (c);
	  category_widgets = g_slist_append (category_widgets, w);
	}
      
      gtk_object_set_data_full (GTK_OBJECT (edit), "category-widgets", category_widgets,
				(GtkDestroyNotify) destroy_category_widgets);

      g_slist_free (categories);
    }

  name_entry = gtk_entry_new ();
  add_tag ("NAME", name_entry, edit);
  gtk_table_attach (GTK_TABLE (table1), name_entry, 1, 2, 0, 1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  summary_entry = gtk_entry_new ();
  gtk_table_attach (GTK_TABLE (table1), summary_entry, 1, 2, 1, 2,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  label16 = gtk_label_new (_("Personal"));
  gtk_notebook_set_tab_label (GTK_NOTEBOOK (notebook2), gtk_notebook_get_nth_page (GTK_NOTEBOOK (notebook2), 0), label16);

  edit_cancel = gpe_button_new_from_stock (GTK_STOCK_CANCEL, GPE_BUTTON_TYPE_BOTH);
  gtk_container_add (GTK_CONTAINER (action_area), edit_cancel);
  GTK_WIDGET_SET_FLAGS (edit_cancel, GTK_CAN_DEFAULT);

  edit_save = gpe_button_new_from_stock (GTK_STOCK_SAVE, GPE_BUTTON_TYPE_BOTH);
  gtk_container_add (GTK_CONTAINER (action_area), edit_save);
  GTK_WIDGET_SET_FLAGS (edit_save, GTK_CAN_DEFAULT);

  gtk_signal_connect (GTK_OBJECT (edit_bt_image), "clicked",
                      GTK_SIGNAL_FUNC (on_edit_bt_image_clicked),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (edit_cancel), "clicked",
                      GTK_SIGNAL_FUNC (on_edit_cancel_clicked),
                      edit);
  gtk_signal_connect (GTK_OBJECT (edit_save), "clicked",
                      GTK_SIGNAL_FUNC (on_edit_save_clicked),
                      edit);

  g_object_set_data (G_OBJECT (edit), "tooltips", tooltips);
  g_object_set_data (G_OBJECT (edit), "notebook2", notebook2);
  g_object_set_data (G_OBJECT (edit), "name_entry", name_entry);
  g_object_set_data (G_OBJECT (edit), "datecombo", datecombo);

  return edit;
}

GtkWidget *
edit_window (void)
{
  GtkWidget *w = create_edit ();
  GtkWidget *book = lookup_widget (w, "notebook2");
  GtkWidget *displaylabel = gtk_label_new ("Display");
  GtkWidget *displayvbox = gtk_vbox_new (FALSE, 0);
  GSList *page;

  for (page = edit_pages; page; page = page->next)
    {
      edit_thing_t e = page->data;
      GtkWidget *vbox = gtk_vbox_new (FALSE, 0);
      GtkWidget *label = gtk_label_new (e->name);
      GtkWidget *scrolled_window = gtk_scrolled_window_new (NULL, NULL);

      build_children (vbox, e->children, w);

      gtk_widget_show_all (vbox);
      gtk_widget_show (label);
      gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
				      GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
      gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled_window), vbox);
      gtk_widget_show (scrolled_window);

      gtk_notebook_append_page (GTK_NOTEBOOK (book), scrolled_window, label);
    }

#if 0
  gtk_notebook_append_page (GTK_NOTEBOOK (book), displayvbox, 
			    displaylabel);
#endif
      
  return w;
}


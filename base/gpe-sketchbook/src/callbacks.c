/* gpe-sketchbook -- a sketches notebook program for PDA
 * Copyright (C) 2002 Luc Pionchon
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>

#include "callbacks.h"
#include "_interface.h"
#include "_support.h"

#include "gpe-sketchbook.h"
#include "files.h"
#include "selector.h"
#include "sketchpad.h"

gboolean _delete_current_sketch();
void     _clist_update_alternate_colors_from(GtkCList * clist, gint index);

void switch_windows(GtkWidget * window_to_hide, GtkWidget * window_to_show){
  gtk_widget_hide (window_to_hide);
  gtk_widget_show (window_to_show);
}

void open_indexed_sketch(gint index){
  gchar * fullpath_filename;
  gchar * title;

  fullpath_filename = (gchar *) gtk_clist_get_row_data(selector_clist, index);
  gtk_clist_get_text(selector_clist, index, 0, &title);
  sketchpad_open_file(fullpath_filename , title);
}

gboolean _delete_current_sketch(){
  gchar    * fullpath_filename;
  gboolean is_deleted = FALSE;
  
  if(is_current_sketch_new) return FALSE;

  fullpath_filename = gtk_clist_get_row_data(selector_clist, current_sketch);
  is_deleted = file_delete(fullpath_filename);

  if(is_deleted){
    _clist_update_alternate_colors_from(selector_clist, current_sketch);
    gtk_clist_remove(selector_clist, current_sketch);
    if(is_current_sketch_last) current_sketch--;
    sketch_list_size--;
  }
  return is_deleted;
}

void _clist_update_alternate_colors_from(GtkCList * clist, gint index){
  gint i;
  for(i = index; i < sketch_list_size; i++){
    if(i%2) gtk_clist_set_background(clist, i, &white);
    else    gtk_clist_set_background(clist, i, &bg_color);
  }
}


void on_window_selector_destroy (GtkObject *object, gpointer user_data){
  app_quit();
}


void on_button_selector_new_clicked (GtkButton *button, gpointer user_data){
  //FIXME: maybe save the previous one if unsaved
  current_sketch = SKETCH_NEW;
  sketchpad_new_sketch();
  switch_windows(window_selector, window_sketchpad);
}

void on_button_selector_open_clicked (GtkButton *button, gpointer user_data){
  if(!is_current_sketch_selected || is_current_sketch_new) return;
  open_indexed_sketch(current_sketch);
  switch_windows(window_selector, window_sketchpad);
}

void on_button_selector_delete_clicked (GtkButton *button, gpointer user_data){
  if(!is_current_sketch_selected || is_current_sketch_new) return;
  //--ask confirmation (maybe a preference)
  dialog_set_text("Delete sketch?");
  dialog_set_action(DELETE);
  gtk_widget_show(window_dialog);
}

void on_clist_selector_select_row (GtkCList *clist, gint row, gint column,
                                   GdkEvent *event, gpointer user_data){
  current_sketch = row;
  set_current_sketch_selected();

  if(event->type == GDK_2BUTTON_PRESS){//--> double click = open related sketch
    gchar     * fullpath_filename;
    gchar     * title;

    fullpath_filename = gtk_clist_get_row_data(clist, row);
    gtk_clist_get_text(clist, row, 0, &title);
    sketchpad_open_file(fullpath_filename, title);
    switch_windows(window_selector, window_sketchpad);
  }
}

void on_clist_selector_unselect_row (GtkCList *clist, gint row, gint column,
                                     GdkEvent *event, gpointer user_data){
  if(row == current_sketch) set_current_sketch_unselected();
}

void on_clist_selector_click_column (GtkCList *clist, gint column, gpointer user_data){
  //do nothing, as column is hidden!
}

void on_button_sketchpad_view_clicked (GtkButton *button, gpointer user_data){
  if(!is_current_sketch_selected) current_sketch = SKETCH_NEW;
  if(is_current_sketch_new) sketchpad_new_sketch();
  else open_indexed_sketch(current_sketch);
  switch_windows(window_selector, window_sketchpad);
}

void on_button_selector_about_clicked (GtkButton *button, gpointer user_data){
  gtk_widget_show(window_about);
}

void on_button_about_ok_clicked (GtkButton *button, gpointer user_data){
  gtk_widget_hide(window_about);
}


void on_button_dialog_cancel_clicked (GtkButton *button, gpointer user_data){
  gtk_widget_hide(window_dialog);
}

void on_button_dialog_ok_clicked (GtkButton *button, gpointer user_data){
  switch(dialog_action){
    case DELETE:
      {
        gboolean is_deleted = FALSE;
        is_deleted = _delete_current_sketch();
        if(is_deleted){
          if(is_sketch_list_empty){
            current_sketch = SKETCH_NEW;
            sketchpad_new_sketch();
          }
          else{
            open_indexed_sketch(current_sketch);
          }
        }
      }
      break;
    case SAVE: break;
  }
  gtk_widget_hide(window_dialog);
  if(gdk_window_is_visible(window_sketchpad->window)) gtk_widget_show(window_sketchpad);
}

//------------------------------------------------------------
//----------------------below is freshly kindly added by glade


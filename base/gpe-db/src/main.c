/*
 * Initial main.c file generated by Glade. Edit as required.
 * Glade will not overwrite this file.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>

#include "interface.h"
#include "support.h"

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

GtkWidget *GPE_DB_Main;
GtkWidget *DBFileSelector;

int
main (int argc, char *argv[])
{
gchar initpath[PATH_MAX];
GtkWidget *widget;

#ifdef ENABLE_NLS
  bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
  textdomain (PACKAGE);
#endif

  gtk_set_locale ();
  gtk_init (&argc, &argv);

  add_pixmap_directory (PACKAGE_DATA_DIR "/pixmaps");
  add_pixmap_directory (PACKAGE_SOURCE_DIR "/pixmaps");

  /*
   * The following code was added by Glade to create one of each component
   * (except popup menus), just so that you see something after building
   * the project. Delete any components that you don't want shown initially.
   */
  GPE_DB_Main = create_GPE_DB_Main ();
  widget=lookup_widget(GPE_DB_Main, "MainMenu");
  gtk_widget_hide(widget);

  DBFileSelector = create_DBSelection ();
  if (getenv("HOME") != NULL) {
  	initpath[0]='\0';
  	strcpy(initpath, getenv("HOME"));
  	strcat(initpath,"/.gpe/");
  	gtk_file_selection_set_filename(GTK_FILE_SELECTION(DBFileSelector), initpath);
  }

  gtk_widget_show (GPE_DB_Main);

  gtk_main ();
  return 0;
}


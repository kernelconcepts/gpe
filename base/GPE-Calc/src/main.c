/*
 * Initial main.c file generated by Glade. Edit as required.
 * Glade will not overwrite this file.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>

#include "interface.h"
#include "support.h"

int
main (int argc, char *argv[])
{
GtkWidget *GPE_Calc;

  gtk_set_locale ();
  gtk_init (&argc, &argv);

#if 0
  add_pixmap_directory (PACKAGE_DATA_DIR "/pixmaps");
  add_pixmap_directory (PACKAGE_SOURCE_DIR "/pixmaps");
#endif

  GPE_Calc=create_GPE_Calc();
  gtk_widget_show(GPE_Calc);

  gtk_main ();
  return 0;
}


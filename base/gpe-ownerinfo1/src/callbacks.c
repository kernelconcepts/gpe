#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>

#include <gpe/pixmaps.h>
#include <gpe/render.h>

#include "callbacks.h"
#include "interface.h"
#include "support.h"

void
on_smallphotobutton_clicked            (GtkButton       *button,
                                        gpointer         user_data)
{
  GtkWidget *notebook;

  notebook = lookup_widget (GTK_WIDGET (button), "notebook");
  gtk_notebook_set_page (GTK_NOTEBOOK (notebook), 1);
}

void
on_bigphotobutton_clicked              (GtkButton       *button,
                                        gpointer         user_data)
{
  GtkWidget *notebook;

  notebook = lookup_widget (GTK_WIDGET (button), "notebook");
  gtk_notebook_set_page (GTK_NOTEBOOK (notebook), 0);
  gtk_widget_queue_draw (GTK_WIDGET (notebook));
}

void
on_bigphotobutton_size_allocate        (GtkWidget       *widget,
                                        GtkAllocation   *allocation,
                                        gpointer         user_data)
{
  g_message("bigphotobutton allocation.width: %d", allocation->width);
  g_message("bigphotobutton allocation.height: %d", allocation->height);
}

void
on_smallphotobutton_size_allocate      (GtkWidget       *widget,
                                        GtkAllocation   *allocation,
                                        gpointer         user_data)
{
  g_message("smallphotobutton allocation.width: %d", allocation->width);
  g_message("smallphotobutton allocation.height: %d", allocation->height);
}

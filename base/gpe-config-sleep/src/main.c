/*
 * Initial main.c file generated by Glade. Edit as required.
 * Glade will not overwrite this file.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <sys/param.h>
#include <gtk/gtk.h>
#include <gpe/init.h>
#include <string.h>
#include <stdlib.h>

#include "interface.h"
#include "support.h"
#include "conf.h"
#include "confGUI.h"

GtkWidget *Sleep_Build_Objects()
{
  char cname[MAXPATHLEN];
  GtkWidget *GPE_Config_Sleep;
  ipaq_conf_t *ISconf;

  strcpy(cname, "/etc/ipaq-sleep.conf");
  ISconf = load_ISconf(cname);
  if(ISconf == NULL) {
    fprintf(stderr, "Error loading configuration file\n");
    exit(1);
  }
  strcpy(ISconf->binCmd, "/etc/init.d/ipaq-sleep");
  load_IRQs(ISconf, "/proc/interrupts");

  /*
   * The following code was added by Glade to create one of each component
   * (except popup menus), just so that you see something after building
   * the project. Delete any components that you don't want shown initially.
   */
  GPE_Config_Sleep = create_GPE_Config_Sleep (ISconf);

  set_conf_defaults(GPE_Config_Sleep, ISconf);
  check_configurable(ISconf);

  gtk_widget_show (GPE_Config_Sleep);

  return GPE_Config_Sleep;
}


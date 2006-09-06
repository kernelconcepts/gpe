#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <libintl.h>
#include <ctype.h>

#include <gtk/gtk.h>
#include <X11/Xmu/WinUtil.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gpe/spacing.h>
#include <gpe/popup.h>
#include <gpe/launch.h>

#include "panel.h"
#include "misc.h"
#include "plugin.h"
#include "bg.h"
#include "gtkbgbox.h"

#include "systray.h"
#include "eggtraymanager.h"
#include "fixedtip.h"

#include "dbg.h"

#define _(x) gettext(x)

typedef struct 
{
  gint pos;
  gchar **apps;
  tray *tr;
}t_sessionstatus;

static void run_application (gchar *exec, t_sessionstatus *session);


void
tray_save_session (tray *tr)
{
  GList *children = gtk_container_get_children (GTK_CONTAINER (tr->box));
  GList *iter;
  gchar *sessiondata = NULL;
  gchar *sessionfile = NULL;

  for (iter = children; iter; iter=iter->next)
    {
        GObject *socket = iter->data;
        Window *w = g_object_get_data (socket, "egg-tray-child-window");
        int cli_argc = 0;
        char **cli_argv = NULL;
        
        if (w)
          {
            if (XGetCommand(GDK_DISPLAY(), *w, &cli_argv, &cli_argc))
              {
                gchar *commandline = g_strdup (cli_argv[0]);
                gchar *tp;
                gint i;
                 
                for (i = 1; i < cli_argc; i++)
                  {
                    tp = g_strjoin (" ", commandline, cli_argv[i], NULL);
                    g_free (commandline);
                    commandline = tp;
                  }
                XFreeStringList(cli_argv);
                  
                tp = g_strjoin (sessiondata ? "\n" : "", 
                                sessiondata ? sessiondata : "",
                                commandline, NULL);
                g_free (sessiondata);
                sessiondata = tp;
                g_free (commandline);
              }
            else
              {
                g_printerr ("failed to get command %i\n", cli_argc);
              }
          }
        else /* separator widget */
          {
            gchar *tp;
            tp = g_strjoin ("", sessiondata, "\n", NULL);
            g_free (sessiondata);
            sessiondata = tp;
          }
    }
    
  sessionfile = g_strconcat (g_get_home_dir(), "/.matchbox/mbdock.session", NULL);
  if (!g_file_set_contents (sessionfile, sessiondata, -1, NULL))
      g_printerr ("Unable to write to %s", sessionfile);
  
  g_free (sessionfile);
  g_free (sessiondata);
}

static gboolean
is_command (const gchar *line)
{
   if (isblank(line[0])) 
       return FALSE;
   if (line[0] == '\n' || line[0] == 0) 
       return FALSE;
   if (line[0] == '#') 
       return FALSE;
   return TRUE;
}

static void
dock_complete (EggTrayManager *manager, GtkWidget *icon, gpointer data)
{
  t_sessionstatus *session = (t_sessionstatus*)data;

  if (!session->apps)
      goto save;
  
  session->pos++;
  
  if (session->apps[session->pos])
    {
      if (!is_command(session->apps[session->pos]) && session->tr->pack_start)
        {
           GtkWidget *spacer;
           spacer = gtk_event_box_new();
           gtk_widget_show (spacer);
           tray_add_widget (session->tr, spacer);
           session->tr->pack_start = FALSE;
           session->pos++;
        }
        
        if (!session->apps[session->pos] 
            || !is_command(session->apps[session->pos]))
          {
            g_strfreev (session->apps);
            session->apps = NULL;
            goto save;
          }
        run_application (session->apps[session->pos], session);
    }
  else
    {
      g_strfreev (session->apps);
      session->apps = NULL;
      goto save;
    }
  return;
    
save:    
    tray_save_session(session->tr);
}

static void
run_application (gchar *exec, t_sessionstatus *session)
{
  Display *dpy = GDK_DISPLAY();
    
  if (! gpe_launch_startup_is_pending (dpy, exec))
    {
      gpe_launch_program_with_callback (dpy, exec, exec, FALSE,
                                        NULL, (gpointer)session);
    }
}

void
tray_restore_session (tray *tr)
{
  gchar *sessionfile, *sfcontent, **sflines;
  gint i = 0;
  t_sessionstatus *session;
        
  sessionfile = g_strconcat (g_get_home_dir(), "/.matchbox/mbdock.session", NULL);
  
  if (g_file_get_contents (sessionfile, &sfcontent, NULL, NULL))
    {
      sflines = g_strsplit (sfcontent, "\n", 100);
      g_free (sfcontent);
      g_free (sessionfile);
      
      while (sflines[i] && !is_command(sflines[i]))
        i++;
      if ((sflines[i]) && is_command (sflines[i]))
        {
          session = g_malloc0 (sizeof(t_sessionstatus));
          session->apps = sflines;
          session->pos = i;
          session->tr = tr;
          g_signal_connect (tr->tray_manager, "tray_icon_added",
                            G_CALLBACK (dock_complete), session);
          run_application (sflines[i], session);
        }
      else
        g_strfreev (sflines);
    }
}

void 
tray_add_widget (tray *tr, GtkWidget *widget)
{
   gtk_box_pack_start (GTK_BOX (tr->box), widget, TRUE, TRUE, 0);
}

static void
tray_added (EggTrayManager *manager, GtkWidget *icon, void *data)
{
    tray *tr = (tray*)data;

    if (tr->pack_start)
      gtk_box_pack_start (GTK_BOX (tr->box), icon, FALSE, TRUE, 0);
    else
      gtk_box_pack_start (GTK_BOX (tr->box), icon, FALSE, TRUE, 0);
    
    gtk_widget_show (icon);
}

static void
tray_removed (EggTrayManager *manager, GtkWidget *icon, void *data)
{
  tray *tr = (tray*)data;
    
  tray_save_session (tr); 
}

static void
message_sent (EggTrayManager *manager, GtkWidget *icon, const char *text, glong id, glong timeout,
              void *data)
{
    /* FIXME multihead */
    gint x, y;
    
    gdk_window_get_origin (icon->window, &x, &y);
  
    fixed_tip_show (0, x, y, FALSE, gdk_screen_height () - 50, text);
}

static void
message_cancelled (EggTrayManager *manager, GtkWidget *icon, glong id,
                   void *data)
{
  fixed_tip_hide();
}

static void
tray_destructor(plugin *p)
{
    tray *tr = (tray *)p->priv;
    
    ENTER;
    /* Make sure we drop the manager selection */
    if (tr->tray_manager)
        g_object_unref (G_OBJECT (tr->tray_manager));
    fixed_tip_hide ();
    g_free(tr);
    RET();
}

static gboolean
tray_button_press (GtkWidget *box, GdkEventButton *b, gpointer user_data)
{
  tray *tr = user_data;
    
  if (b->button == 1)
    {
       gtk_menu_popup (GTK_MENU (tr->context_menu),
                       NULL, NULL, gpe_popup_menu_position, box, 1, b->time);
       return TRUE;
    }

  return FALSE;
}

static int
tray_constructor(plugin *p)
{
    line s;
    tray *tr;
    GdkScreen *screen;
    GtkWidget *box, *item, *tray_apps_menu;
    
    ENTER;
    s.len = 256;
    while (get_line(p->fp, &s) != LINE_BLOCK_END) {
        ERR("tray: illegal in this context %s\n", s.str);
        RET(0);
    }
    
    tr = g_new0(tray, 1);
    g_return_val_if_fail(tr != NULL, 0);
    p->priv = tr;
    tr->plug = p;
    tr->pack_start = TRUE;

    box = p->panel->my_box_new(FALSE, 0);
    tr->box = p->panel->my_box_new(FALSE, gpe_get_boxspacing());
    gtk_box_pack_start(GTK_BOX (box), tr->box, TRUE, TRUE, 0);
    gtk_container_add(GTK_CONTAINER(p->pwid), box);
    
    gtk_bgbox_set_background(p->pwid, BG_STYLE, 0, 0);
    gtk_container_set_border_width(GTK_CONTAINER(p->pwid), 0);
    screen = gtk_widget_get_screen (GTK_WIDGET (p->panel->topgwin));
    
    if (egg_tray_manager_check_running(screen)) {
        tr->tray_manager = NULL;
        ERR("tray: another systray already running\n");
        RET(1);
    }
    tr->tray_manager = egg_tray_manager_new ();
    if (!egg_tray_manager_manage_screen (tr->tray_manager, screen))
        g_printerr ("tray: System tray didn't get the system tray manager selection\n");
    
    g_signal_connect (tr->tray_manager, "tray_icon_added",
          G_CALLBACK (tray_added), tr);
    g_signal_connect (tr->tray_manager, "tray_icon_removed",
          G_CALLBACK (tray_removed), tr);
    g_signal_connect (tr->tray_manager, "message_sent",
          G_CALLBACK (message_sent), NULL);
    g_signal_connect (tr->tray_manager, "message_cancelled",
          G_CALLBACK (message_cancelled), NULL);
    
    gtk_widget_show_all(box);
    
    /* context menu */
    tr->context_menu = gtk_menu_new ();
    item = gtk_image_menu_item_new_from_stock (GTK_STOCK_ADD, NULL);
    gtk_menu_attach (GTK_MENU (tr->context_menu), item, 0, 1, 0, 1);
    
    tray_apps_menu = g_object_get_data (G_OBJECT (p->pwid->parent), 
                                        "tray-apps-menu");
    if (tray_apps_menu)
      gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), tray_apps_menu);
    item = gtk_image_menu_item_new_from_stock (GTK_STOCK_REMOVE, NULL);
    gtk_menu_attach (GTK_MENU (tr->context_menu), item, 0, 1, 1, 2);
    item = gtk_separator_menu_item_new ();
    gtk_menu_attach (GTK_MENU (tr->context_menu), item, 0, 1, 2, 3);
    item = gtk_image_menu_item_new_with_label (_("Hide"));
    gtk_menu_attach (GTK_MENU (tr->context_menu), item, 0, 1, 3, 4);
    
    gtk_menu_attach_to_widget (GTK_MENU(tr->context_menu), tr->box, NULL);
    g_signal_connect (tr->box, "button-press-event", 
          G_CALLBACK (tray_button_press), tr);
    
    gtk_widget_show_all (tr->context_menu);

    RET(1);

}


plugin_class tray_plugin_class = {
    fname: NULL,
    count: 0,

    type : "tray",
    name : "tray",
    version: "1.2",
    description : "X Tray/Notification area",

    constructor : tray_constructor,
    destructor  : tray_destructor,
};

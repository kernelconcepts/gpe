#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <unistd.h>

#include "callbacks.h"
#include "interface.h"
#include "support.h"

enum
{
  COL_DISPLAY_NAME,
  COL_PIXBUF,
  NUM_COLS
};

void
set_net_profile(GtkIconView *iconview, GtkTreePath *path, gpointer user_data)
{
	gchar *name;
	GtkTreeModel *model = gtk_icon_view_get_model (iconview);
	GtkWidget * window1 = lookup_widget(GTK_WIDGET(user_data), "window1");
	GtkTreeIter iter;
	gchar essid[64];
	printf("set_profile called!");

	gtk_tree_model_get_iter(model, &iter, path);
	gtk_tree_model_get (model, &iter, COL_DISPLAY_NAME, &name, -1);
	sscanf(name,"- %s",essid);
	printf("got:%s\n",essid);
	gtk_window_set_title(window1,_(essid));
	printf("set_profile complete!\n");


}

guint update_icon_list(gpointer user_data)
{

  GtkWidget * iconview1 = lookup_widget(GTK_WIDGET(user_data), "iconview1");
  GtkListStore *list_store = GTK_LIST_STORE(gtk_icon_view_get_model(GTK_ICON_VIEW(iconview1)));
  GdkPixbuf *p1, *p2;
  GtkTreeIter iter;
  GError *err = NULL;
  gchar pathname[255];
  char dline[64], func[12], value[64];
  char essid[100];
  FILE *f;

  if (list_store)
    gtk_list_store_clear(list_store);

  gtk_icon_view_set_orientation(GTK_ICON_VIEW(iconview1), GTK_ORIENTATION_HORIZONTAL);
  gtk_icon_view_set_columns(GTK_ICON_VIEW(iconview1), 1);
  gtk_icon_view_set_item_width(GTK_ICON_VIEW(iconview1),200);
  
  p1 = gdk_pixbuf_new_from_file ("/usr/share/pixmaps/gtk-yes.png", &err);
                            /* No error checking is done here */
  p2 = gdk_pixbuf_new_from_file ("/usr/share/pixmaps/gtk-no.png", &err);

  list_store = gtk_list_store_new (NUM_COLS, G_TYPE_STRING, GDK_TYPE_PIXBUF);

  system("/usr/bin/wifi-scan.sh");

  f=fopen("/tmp/wifi.scan", "r");
	
  if (! f) fprintf(stderr, "problem opening /tmp/wifi.conf\n");
	else {
		while(fgets(dline,sizeof(dline),f)) {
			if (sscanf(dline,"%s = %s\n", func, value) == 2) {
				if (strcmp(func, "essid")==0) {
				    sprintf(essid,"- %s",value);
				    gtk_list_store_append (list_store, &iter);
				    sprintf(pathname,"/etc/wifi/%s.cfg",value);
				    if (g_file_test (pathname, G_FILE_TEST_EXISTS)) {
					printf("found %s profile!\n",pathname);
					gtk_list_store_set (list_store, &iter, COL_DISPLAY_NAME, _(essid),
                    			    COL_PIXBUF, p1, -1);
                    		    }
                    		    else {
					gtk_list_store_set (list_store, &iter, COL_DISPLAY_NAME, _(essid),
                    			    COL_PIXBUF, p2, -1);
                    		    }
				}
			}
		}
	    fclose(f);
	}
  gtk_icon_view_set_model(GTK_ICON_VIEW(iconview1),GTK_TREE_MODEL(list_store));
  gtk_icon_view_set_text_column (GTK_ICON_VIEW (iconview1),
                                 COL_DISPLAY_NAME);
  gtk_icon_view_set_pixbuf_column (GTK_ICON_VIEW (iconview1), COL_PIXBUF);
  gtk_icon_view_set_selection_mode (GTK_ICON_VIEW (iconview1),
                                    GTK_SELECTION_MULTIPLE);

    return FALSE;
}

guint refresh_icon_list(gpointer user_data)
{

  GtkWidget * iconview1 = lookup_widget(GTK_WIDGET(user_data), "iconview1");
  GtkListStore *list_store = GTK_LIST_STORE(gtk_icon_view_get_model(GTK_ICON_VIEW(iconview1)));
  GdkPixbuf *p1;
  GtkTreeIter iter;
  GError *err = NULL;
  FILE *f;

  if (list_store)
    gtk_list_store_clear(list_store);

  gtk_icon_view_set_orientation(GTK_ICON_VIEW(iconview1), GTK_ORIENTATION_HORIZONTAL);
  gtk_icon_view_set_columns(GTK_ICON_VIEW(iconview1), 1);
  gtk_icon_view_set_item_width(GTK_ICON_VIEW(iconview1),200);
  
  p1 = gdk_pixbuf_new_from_file ("/usr/share/pixmaps/gtk-refresh.png", &err);

  list_store = gtk_list_store_new (NUM_COLS, G_TYPE_STRING, GDK_TYPE_PIXBUF);


  gtk_list_store_append (list_store, &iter);
  gtk_list_store_set (list_store, &iter, COL_DISPLAY_NAME, _(" - Scanning - Please Wait"),
                    			COL_PIXBUF, p1, -1);
  gtk_icon_view_set_model(GTK_ICON_VIEW(iconview1),GTK_TREE_MODEL(list_store));
  gtk_icon_view_set_text_column (GTK_ICON_VIEW (iconview1),
                                 COL_DISPLAY_NAME);
  gtk_icon_view_set_pixbuf_column (GTK_ICON_VIEW (iconview1), COL_PIXBUF);
  gtk_icon_view_set_selection_mode (GTK_ICON_VIEW (iconview1),
                                    GTK_SELECTION_MULTIPLE);

  g_timeout_add_full(G_PRIORITY_DEFAULT,1000,update_icon_list,user_data,NULL)	;


    return FALSE;
}

void
on_window1_show                        (GtkWidget       *widget,
                                        gpointer         user_data)
{


  refresh_icon_list(user_data);

}


gboolean
on_button1_button_press_event          (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{

  refresh_icon_list(user_data);


  return FALSE;
}


gboolean
on_button2_button_press_event          (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
  
  return FALSE;
}




gboolean
on_button1_button_release_event        (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{

  return FALSE;
}


gboolean
on_button3_button_press_event          (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{

  return FALSE;
}


gboolean
on_button3_button_release_event        (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
  GtkWidget * iconview1 = lookup_widget(GTK_WIDGET(user_data), "iconview1");
  GtkWidget * window1 = lookup_widget(GTK_WIDGET(user_data), "window1");


   pid_t pID = fork();
   if (pID == 0)              
   {

	gtk_icon_view_selected_foreach(GTK_ICON_VIEW(iconview1),set_net_profile,user_data);
	printf("title set as %s!\n",gtk_window_get_title(GTK_WINDOW(window1))); 

	if (!strcmp("WiFi Network Scan",gtk_window_get_title(GTK_WINDOW(window1)))){
	    printf("Wifi Network Scan set for title!\n"); 
	    execl("/usr/bin/net-connect","/usr/bin/net-connect","None",(char *)0);
	}
	else {
	    printf("title set for essid!\n"); 
	    execl("/usr/bin/net-connect","/usr/bin/net-connect",gtk_window_get_title(GTK_WINDOW(window1)),(char *)0);
	}
    
   }
   else
   {

    sleep(3);
    gtk_main_quit();

   }    


  return FALSE;
}


gboolean
on_button2_button_release_event        (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
  GtkWidget * iconview1 = lookup_widget(GTK_WIDGET(user_data), "iconview1");
  GtkWidget * window1 = lookup_widget(GTK_WIDGET(user_data), "window1");


   pid_t pID = fork();
   if (pID == 0)              
   {

	gtk_icon_view_selected_foreach(GTK_ICON_VIEW(iconview1),set_net_profile,user_data);
	printf("title set as %s!\n",gtk_window_get_title(GTK_WINDOW(window1))); 

	if (!strcmp("WiFi Network Scan",gtk_window_get_title(GTK_WINDOW(window1)))){
	    printf("Wifi Network Scan set for title!\n"); 
	    execl("/usr/bin/net-config","/usr/bin/net-config","None",	\
		    "/usr/bin/wifi-scan",(char *)0);
	}
	else {
	    printf("title set for essid!\n"); 
	    execl("/usr/bin/net-config","/usr/bin/net-config",	\
		    gtk_window_get_title(GTK_WINDOW(window1)),	\
		    "/usr/bin/wifi-scan",(char *)0);
	}
    
   }
   else
   {

    sleep(3);
    gtk_main_quit();

   }    

  return FALSE;
}


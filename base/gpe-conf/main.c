/*
 * gpe-conf
 *
 * Copyright (C) 2002  Pierre TARDY <tardyp@free.fr>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <pwd.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <gtk/gtk.h>
#include "applets.h"
#include "timeanddate.h"
#include "appmgr_setup.h"
#include "ipaqscreen.h"
#include "unimplemented.h"
#include "kbd.h"
#include "network.h"
#include "theme.h"


#include "gpe/init.h"

#include <gpe/picturebutton.h>
#include <gpe/init.h>
#include <gpe/pixmaps.h>

#define VERSION "0.0.1"

static struct {
  GtkWidget *w;

  GtkWidget *applet;
  GtkWidget *vbox;

  GtkWidget *save;
  GtkWidget *cancel;
  
  int cur_applet;
  int alone_applet;

}self;

struct Applet applets[]=
  {
    { &Time_Build_Objects, &Time_Free_Objects, &Time_Save, &Time_Restore , "Time" ,"time"},
    { &Appmgr_Build_Objects, &Appmgr_Free_Objects, &Appmgr_Save, &Appmgr_Restore , "Appmgr" ,"appmgr"},
    { &ipaqscreen_Build_Objects, &ipaqscreen_Free_Objects, &ipaqscreen_Save, &ipaqscreen_Restore , "Screen" , "ipaqscreen"},
    { &Unimplemented_Build_Objects, &Unimplemented_Free_Objects, &Unimplemented_Save, &Unimplemented_Restore , "Energy" ,"apm"},
    { &Kbd_Build_Objects, &Unimplemented_Free_Objects, &Kbd_Save, &Unimplemented_Restore , "Keyboard" ,"keyboard"},
    { &Unimplemented_Build_Objects, &Unimplemented_Free_Objects, &Unimplemented_Save, &Unimplemented_Restore , "Sound" ,"sound"},
    { &Unimplemented_Build_Objects, &Unimplemented_Free_Objects, &Unimplemented_Save, &Unimplemented_Restore , "Mouse" ,"mouse"},
    { &Network_Build_Objects, &Unimplemented_Free_Objects, &Unimplemented_Save, &Unimplemented_Restore , "Network" ,"network"},
    { &Unimplemented_Build_Objects, &Unimplemented_Free_Objects, &Unimplemented_Save, &Unimplemented_Restore , "Screensvr" ,"screensaver"},
    { &Unimplemented_Build_Objects, &Unimplemented_Free_Objects, &Unimplemented_Save, &Unimplemented_Restore , "Software" ,"software"},
    { &Theme_Build_Objects, &Unimplemented_Free_Objects, &Unimplemented_Save, &Unimplemented_Restore , "Theme" ,"theme"},
    { &Unimplemented_Build_Objects, &Unimplemented_Free_Objects, &Unimplemented_Save, &Unimplemented_Restore , "WallPaper" ,"wallpaper"},
    { &Unimplemented_Build_Objects, &Unimplemented_Free_Objects, &Unimplemented_Save, &Unimplemented_Restore , "WindowMgr" ,"windowmanager"}
  };
static struct gpe_icon my_icons[] = {
  { "save" },
  { "cancel" },
  { NULL, NULL }
};

int applets_nb = sizeof(applets) / sizeof(struct Applet);

void Save_Callback()
{
  applets[self.cur_applet].Save();
  if(self.alone_applet)
    gtk_exit(0);

}
void Restore_Callback()
{
  applets[self.cur_applet].Restore();
  if(self.alone_applet)
    gtk_exit(0);
}

void item_select(GtkWidget *ignored, gpointer user_data)
{
  int i = (int) user_data;

  if(self.cur_applet != - 1)
    {
      applets[self.cur_applet].Save();
      applets[self.cur_applet].Free_Objects();      
    }
  if(self.applet)
    {
      gtk_widget_hide(self.applet);
      // TODO there must be a memory leak here..
      gtk_container_remove(GTK_CONTAINER(self.vbox),self.applet);
    }
  self.cur_applet = i;

  self.applet = applets[i].Build_Objects();
  gtk_box_pack_start(GTK_BOX(self.vbox),self.applet,TRUE, TRUE, 0);

  gtk_widget_show_all(self.applet);
  gtk_widget_show(self.save);
  gtk_widget_show(self.cancel);
}

void initwindow()
{
  if (gpe_application_init (NULL, NULL) == FALSE)
    exit (1);

  if (gpe_load_icons (my_icons) == FALSE)
    exit (1);

   // main window
   self.w=gtk_window_new(GTK_WINDOW_TOPLEVEL);
   gtk_window_set_title(GTK_WINDOW(self.w),"GPE-Conf " VERSION);
   gtk_widget_set_usize(GTK_WIDGET(self.w),240, 300);


   gtk_signal_connect (GTK_OBJECT(self.w), "delete-event",
		       (GtkSignalFunc) gtk_main_quit, NULL);

   gtk_signal_connect (GTK_OBJECT(self.w), "destroy", 
      (GtkSignalFunc) gtk_main_quit, NULL);


}
void make_buttons()
{
  GtkWidget *hbuttons;
  hbuttons = gtk_hbutton_box_new();
  gtk_box_pack_end(GTK_BOX(self.vbox),hbuttons,TRUE, TRUE, 0);

  self.save = gpe_picture_button (self.w->style, ("Save"), "save");
  gtk_container_add(GTK_CONTAINER(hbuttons),self.save);

  self.cancel = gpe_picture_button (self.w->style, ("Cancel"), "cancel");
  gtk_container_add(GTK_CONTAINER(hbuttons),self.cancel);

  gtk_signal_connect (GTK_OBJECT(self.save), "clicked",
		      (GtkSignalFunc) Save_Callback, NULL);
  gtk_signal_connect (GTK_OBJECT(self.cancel), "clicked",
		      (GtkSignalFunc) Restore_Callback, NULL);

}
void main_all()
{
  int i;
  GtkWidget *ntree;
  GtkWidget *root_tree;
  GtkWidget *sys_root;
  GtkWidget *split;

  self.alone_applet=0;

  initwindow();
  split=gtk_hpaned_new();

  gtk_container_add (GTK_CONTAINER (self.w), split);

  gtk_widget_show(split);

  root_tree = gtk_tree_new();
  sys_root = gtk_tree_item_new_with_label("System");
  gtk_widget_show(sys_root);
  gtk_tree_append(GTK_TREE(root_tree),sys_root);

   

  gtk_paned_add1(GTK_PANED(split),root_tree);


  self.vbox = gtk_vbox_new(FALSE,0);
  gtk_paned_add2(GTK_PANED(split),self.vbox);

  self.applet = gtk_label_new("Gpe Configuration \n by Pierre Tardy\n\n inspired of sysset \nby James Weatheral..");
  gtk_box_pack_start(GTK_BOX(self.vbox),self.applet,TRUE, TRUE, 0);
  
  make_buttons();

  ntree =  gtk_tree_new();
  gtk_widget_show(ntree);

  for(i = 0; i< applets_nb ; i++)
    {
      GtkWidget *item;
      item = gtk_tree_item_new_with_label(applets[i].label);
      gtk_signal_connect (GTK_OBJECT(item), "select",
			  (GtkSignalFunc) item_select, (gpointer)i);
       
      gtk_widget_show(item);
      gtk_tree_append(GTK_TREE(ntree),item);
    }

  gtk_tree_item_set_subtree(GTK_TREE_ITEM(sys_root),ntree);
  gtk_tree_item_expand(GTK_TREE_ITEM(sys_root));

  self.cur_applet = -1;
  gtk_widget_show_all(self.w);
 
  gtk_widget_show(self.w);

  gtk_widget_hide(self.save);
  gtk_widget_hide(self.cancel);
  
  gtk_main();
  gtk_exit(0);
  return;
}



void main_one(int argc, char **argv,int applet)
{

  self.alone_applet=1;
  initwindow();

  self.vbox = gtk_vbox_new(FALSE,0);
  gtk_container_add(GTK_CONTAINER(self.w),self.vbox);

  self.cur_applet = -1;
  self.applet = NULL;

  make_buttons();


  gtk_widget_show_all(self.w);
 
  gtk_widget_show(self.w);

   
  item_select(NULL, (gpointer)applet);
  gtk_main();
  gtk_exit(0);
  return ;
  
}



int main(int argc, char **argv)
{
  int i;
  if(argc == 1)
    {
      main_all(argc,argv);
    }
  else
    {
      for( i = 0 ; i< applets_nb ; i++)
	{
	  if(strcmp(argv[1], applets[i].name) == 0)
	    main_one(argc,argv,i);
	}
      if (i ==applets_nb)
	{
	  fprintf(stderr,"Applet %s unknown!\n",argv[1]);
	  printf("\n\nUsage : gpe-conf [AppletName]\nwhere AppletName is in :\n");
	  for( i = 0 ; i< applets_nb ; i++)
	    if(applets[i].Build_Objects != Unimplemented_Build_Objects)
	      fprintf(stderr,"%s\n",applets[i].name);
	}
    }
  return 0;
}

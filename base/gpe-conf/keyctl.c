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
#include <gdk/gdk.h>
#define _XOPEN_SOURCE /* Pour GlibC2 */
#include <time.h>
#include "gpe/pixmaps.h"
#include "gpe/render.h"

#include "applets.h"
#include "keyctl.h"




char buttons[6][1024];
static struct{

  GtkWidget *button[5];
  GdkPixbuf *p;  
}self;

void FileSelected(char *file, gpointer data);

void init_buttons()
{
    FILE *fd;
    GtkButton *target;
    char buffer[1024];
    char *slash;
    char btext[16];
    int i;

    strcpy(buttons[0],"");

    /* read from configfile and set buttons */
    fd=fopen("/etc/keyctl.conf","r");
    if (fd==NULL) {
	/* defaults */
	strcpy(buttons[1],"/usr/bin/record");
        target=GTK_BUTTON(self.button[0]);
	gtk_label_set_text(GTK_LABEL(target->child),"record");

	strcpy(buttons[2],"/usr/bin/schedule");
        target=GTK_BUTTON(self.button[1]);
	gtk_label_set_text(GTK_LABEL(target->child),"schedule");

	strcpy(buttons[3],"/usr/bin/mingle");
        target=GTK_BUTTON(self.button[2]);
	gtk_label_set_text(GTK_LABEL(target->child),"mingle");

	strcpy(buttons[4],"/usr/bin/x-terminal-emulator");
        target=GTK_BUTTON(self.button[3]);
	gtk_label_set_text(GTK_LABEL(target->child),"terminal");

	strcpy(buttons[5],"/usr/bin/fmenu");
        target=GTK_BUTTON(self.button[4]);
	gtk_label_set_text(GTK_LABEL(target->child),"fmenu");

    } else {
	/* load from configfile */
	for (i=1;i<6;i++) {
	    fgets(buffer, 1023, fd);
	    slash=strchr(buffer,'\n');
	    if (slash!=NULL) { *slash='\x0'; }
	    strcpy(buttons[i],buffer);
	    slash=strrchr(buffer,'/')+1;
	    if (slash==NULL) { slash=buffer; }
	    strncpy(btext,slash,15);
	    btext[15]='\x0';
	    target=GTK_BUTTON(self.button[i-1]);
	    gtk_label_set_text(GTK_LABEL(target->child),btext);
	}
        fclose(fd);
    }
}

#if 0 //an attempt to draw some arrows from the pixmaps to the buttons. 
gboolean expose_event (GtkWidget *widget, GdkEventExpose *event, gpointer data)
{

/*  gdk_window_clear_area (widget->window,
                         event->area.x, event->area.y,
                         event->area.width, event->area.height);
  gdk_gc_set_clip_rectangle (widget->style->fg_gc[widget->state],
                             &event->area);
			     
*/ 
  gtk_gpe_pixmap_expose     (widget,
			     event);
  gdk_draw_arc (widget->window,
                widget->style->fg_gc[widget->state],
                TRUE,
                0, 0, widget->allocation.width, widget->allocation.height,
                0, 64 * 360);
/*  gdk_gc_set_clip_rectangle (widget->style->fg_gc[widget->state],
                             NULL);
*/
  return TRUE;
}
#endif

void
on_button_clicked                      (GtkButton       *button,
                                        gpointer         user_data)
{
    ask_user_a_file("/usr/bin",NULL,FileSelected,NULL,button);
}




GtkWidget *Keyctl_Build_Objects()
{
  GtkWidget *vbox1 = gtk_vbox_new (FALSE, 0);
  GtkWidget *layout1 = gtk_layout_new (NULL, NULL);
  GtkWidget *button_1 = gtk_button_new_with_label ("Record");
  GtkWidget *button_2 = gtk_button_new_with_label ("Calendar");
  GtkWidget *button_5 = gtk_button_new_with_label ("Menu");
  GtkWidget *button_3 = gtk_button_new_with_label ("Contacts");
  GtkWidget *button_4 = gtk_button_new_with_label ("Q");
  self.p = gpe_find_icon ("ipaq");


  self.button[0]=button_1;
  self.button[1]=button_2;
  self.button[2]=button_3;
  self.button[3]=button_4;
  self.button[4]=button_5;
  
  //  gtk_widget_show (scrolledwindow1);
  //gtk_container_add (GTK_CONTAINER (frame1), scrolledwindow1);
  //gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow1), GTK_POLICY_ALWAYS, GTK_POLICY_ALWAYS);

  gtk_widget_show (layout1);
  gtk_container_add (GTK_CONTAINER (vbox1), layout1);
  gtk_layout_set_size (GTK_LAYOUT (layout1), 400, 400);
  GTK_ADJUSTMENT (GTK_LAYOUT (layout1)->hadjustment)->step_increment = 10;
  GTK_ADJUSTMENT (GTK_LAYOUT (layout1)->vadjustment)->step_increment = 10;

  gtk_widget_show (button_1);
  gtk_layout_put (GTK_LAYOUT (layout1), button_1, 0, 66);
  gtk_widget_set_usize (button_1, 50, 20);

  gtk_widget_show (button_2);
  gtk_layout_put (GTK_LAYOUT (layout1), button_2, 8, 192);
  gtk_widget_set_usize (button_2, 50, 20);

  gtk_widget_show (button_5);
  gtk_layout_put (GTK_LAYOUT (layout1), button_5, 184, 192);
  gtk_widget_set_usize (button_5, 50, 20);

  gtk_widget_show (button_3);
  gtk_layout_put (GTK_LAYOUT (layout1), button_3, 64, 216);
  gtk_widget_set_usize (button_3, 50, 20);

  gtk_widget_show (button_4);
  gtk_layout_put (GTK_LAYOUT (layout1), button_4, 128, 216);
  gtk_widget_set_usize (button_4, 50, 20);

  if(self.p)
    {
      GtkWidget *pixmap1 = gpe_render_icon (wstyle, self.p);
      gtk_widget_show (pixmap1);
      gtk_layout_put (GTK_LAYOUT (layout1), pixmap1, 50, 10);
      gtk_widget_set_usize (pixmap1, 144, 208);


      //      gtk_signal_connect (GTK_OBJECT (pixmap1), "expose_event",GTK_SIGNAL_FUNC (expose_event),NULL);


      
    }
  
  gtk_signal_connect_object (GTK_OBJECT (button_1), "clicked",
                             GTK_SIGNAL_FUNC (on_button_clicked),
                             GTK_OBJECT (button_1));
  gtk_signal_connect_object (GTK_OBJECT (button_2), "clicked",
                             GTK_SIGNAL_FUNC (on_button_clicked),
                             GTK_OBJECT (button_2));
  gtk_signal_connect_object (GTK_OBJECT (button_5), "clicked",
                             GTK_SIGNAL_FUNC (on_button_clicked),
                             GTK_OBJECT (button_5));
  gtk_signal_connect_object (GTK_OBJECT (button_3), "clicked",
                             GTK_SIGNAL_FUNC (on_button_clicked),
                             GTK_OBJECT (button_3));
  gtk_signal_connect_object (GTK_OBJECT (button_4), "clicked",
                             GTK_SIGNAL_FUNC (on_button_clicked),
                             GTK_OBJECT (button_4));
  init_buttons();
  return vbox1;

}
void Keyctl_Free_Objects();
void Keyctl_Save()
{
    /* save new config, force keyctl to reload, and exit */
    int i;
    FILE *fd;

    fd=fopen("/etc/keyctl.conf","w");
    if (fd==NULL) {
	printf("ERROR: Can't open /etc/keyctl.conf for writing!\n");
	return;
	//        exit(1);
    }
    for (i=1;i<6;i++) {
#ifdef DEBUG
	printf("button #%d => %s\n",i,buttons[i]);
#endif
        fprintf(fd, "%s\n",buttons[i]);
    }
    fclose(fd);

}
void Keyctl_Restore()
{

  init_buttons();

}


void FileSelected(char *file, gpointer data)
{
  GtkButton *target;
  char btext[16];
  int len;
  char *slash;
  int button_nr;

  target=data;
  for(button_nr = 0; button_nr<5 && self.button[button_nr]!=GTK_WIDGET(target) ;button_nr++);

  if(button_nr==5)
    {
      printf("cant find button\n");
      return;
    }
	

  strncpy(buttons[button_nr],file,1023);
  buttons[button_nr][1023]='\x0';

#ifdef DEBUG
  printf("button %d changed to %s\n",button_nr,file);
#endif
  len=strlen(file);
  slash=strrchr(file,'/')+1;
  if (slash==NULL) { slash=file; }
  strncpy(btext,slash,15);
  btext[15]='\x0';
#ifdef DEBUG
  printf("setting label to %s\n",btext);
#endif
  gtk_label_set_text(GTK_LABEL(target->child),btext);
}
 

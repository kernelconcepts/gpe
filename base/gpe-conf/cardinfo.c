/*
 * gpe-conf
 *
 * Copyright (C) 2003  Florian Boor <florian.boor@kernelconcepts.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 *
 * GPE CF/PC card information and configuration module.
 *
 * Based on Cardinfo by David A. Hinds <dahinds@users.sourceforge.net>
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <libintl.h>
#define _(x) gettext(x)

#include <sys/types.h>
#include <stdarg.h>
#include <errno.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/file.h>

#include <pcmcia/version.h>
#include <pcmcia/cs_types.h>
#include <pcmcia/cs.h>
#include <pcmcia/cistpl.h>
#include <pcmcia/ds.h>

#include <gtk/gtk.h>

#include <gpe/errorbox.h>
#include <gpe/spacing.h>
#include <gpe/pixmaps.h>

#include "cardinfo.h"
#include "applets.h"
#include "suid.h"
#include "parser.h"
#include "misc.h"


/* --- local types and constants --- */

#define MAX_SOCK 4

typedef enum s_state {
    S_EMPTY, S_PRESENT, S_READY, S_BUSY, S_SUSPEND
} s_state;

typedef struct event_tag_t {
    event_t event;
    char *name;
} event_tag_t;

static event_tag_t event_tag[] = {
    { CS_EVENT_CARD_INSERTION, "card insertion" },
    { CS_EVENT_CARD_REMOVAL, "card removal" },
    { CS_EVENT_RESET_PHYSICAL, "prepare for reset" },
    { CS_EVENT_CARD_RESET, "card reset successful" },
    { CS_EVENT_RESET_COMPLETE, "reset request complete" },
    { CS_EVENT_EJECTION_REQUEST, "user eject request" },
    { CS_EVENT_INSERTION_REQUEST, "user insert request" },
    { CS_EVENT_PM_SUSPEND, "suspend card" },
    { CS_EVENT_PM_RESUME, "resume card" },
    { CS_EVENT_REQUEST_ATTENTION, "request attention" },
};


#define NTAGS (sizeof(event_tag)/sizeof(event_tag_t))

/* --- module global variables --- */

static GtkWidget *notebook;

int ns;
socket_info_t st[MAX_SOCK];

static char *pidfile = "/var/run/cardmgr.pid";
static char *stabfile;

#define PIXMAP_PATH PREFIX "/share/pixmaps/"

/* --- local intelligence --- */

static void do_alert(char *fmt, ...)
{
    char msg[132];
    va_list args;
    va_start(args, fmt);
    vsprintf(msg, fmt, args);
	gpe_error_box(msg);
    va_end(args);
} /* do_alert */

static void do_reset(GtkWidget *button)
{
    FILE *f;
    pid_t pid;
    
    f = fopen(pidfile, "r");
    if (f == NULL) {
	do_alert("%s %s",_("Could not open pidfile:"), strerror(errno));
	return;
    }
    if (fscanf(f, "%d", &pid) != 1) {
	do_alert(_("Could not read pidfile"));
	return;
    }
    if (kill(pid, SIGHUP) != 0)
	do_alert("%s %s", _("Could not signal cardmgr:"),strerror(errno));
}

static int lookup_dev(char *name)
{
    FILE *f;
    int n;
    char s[32], t[32];
    
    f = fopen("/proc/devices", "r");
    if (f == NULL)
	return -errno;
    while (fgets(s, 32, f) != NULL) {
	if (sscanf(s, "%d %s", &n, t) == 2)
	    if (strcmp(name, t) == 0)
		break;
    }
    fclose(f);
    if (strcmp(name, t) == 0)
	return n;
    else
	return -ENODEV;
} /* lookup_dev */


static int open_dev(dev_t dev)
{
    char *fn;
    int fd;
    
    if ((fn = tmpnam(NULL)) == NULL)
	return -1;
    if (mknod(fn, (S_IFCHR|S_IREAD), dev) != 0)
	return -1;
    if ((fd = open(fn, O_RDONLY)) < 0) {
	unlink(fn);
	return -1;
    }
    if (unlink(fn) != 0) {
	close(fd);
	return -1;
    }
    return fd;
} /* open_dev */


int
init_pcmcia_suid()
{
	int major;
    servinfo_t serv;

    if (geteuid() != 0) {
	  fprintf(stderr, "gpe-conf must be setuid root to use cardinfo seature\n");
	  return -1;
    }

    if (access("/var/lib/pcmcia", R_OK) == 0) {
	stabfile = "/var/lib/pcmcia/stab";
    } else {
	stabfile = "/var/run/stab";
    }
    
    major = lookup_dev("pcmcia");
    if (major < 0) {
	if (major == -ENODEV)
	    fprintf(stderr, "no pcmcia driver in /proc/devices\n");
	else
	    perror("could not open /proc/devices");
	  return -1;
    }
    
    for (ns = 0; ns < MAX_SOCK; ns++) {
	st[ns].fd = open_dev((major<<8)+ns);
	if (st[ns].fd < 0) break;
    }
    if (ns == 0) {
	fprintf(stderr, "no sockets found\n");
	  return -1;
    }

    if (ioctl(st[0].fd, DS_GET_CARD_SERVICES_INFO, &serv) == 0) {
	if (serv.Revision != CS_RELEASE_CODE)
	    fprintf(stderr, "Card Services release does not match!\n");
    } else {
	fprintf(stderr, "could not get CS revision info!\n");
	  return -1;
    }
	return 0;
}


GtkWidget *new_field(field_t *field, GtkWidget *parent, int pos, char *strlabel)
{
    GtkWidget *result;
    GtkWidget *label;
	
    label = gtk_label_new(strlabel);
    result = gtk_label_new(NULL);
	gtk_table_attach(GTK_TABLE(parent),label,0,1,pos,pos+1,GTK_FILL,0,0,0);
	gtk_table_attach(GTK_TABLE(parent),result,1,2,pos,pos+1,GTK_SHRINK | GTK_FILL,GTK_FILL | GTK_SHRINK,0,0);
	field->widget = result;
	gtk_misc_set_alignment(GTK_MISC(result),0.0,0.5);
	gtk_misc_set_alignment(GTK_MISC(label),0.0,0.5);
	
    field->str = strdup("");
	
	return result;
}

static void new_flag(flag_t *flag, GtkWidget *parent, char *strlabel)
{
	char *str;
	str = g_strdup_printf(" %s ",strlabel);
	flag->widget = gtk_label_new(str);
	free(str);
	gtk_misc_set_alignment(GTK_MISC(flag->widget),0.5,0.5);
	gtk_box_pack_start(GTK_BOX(parent),flag->widget,FALSE,TRUE,0);
	gtk_widget_set_sensitive(flag->widget,FALSE);
    flag->val = 0;
}

static void update_field(field_t *field, char *new)
{
    if (strcmp(field->str, new) != 0) {
	free(field->str);
	field->str = strdup(new);
	gtk_label_set_markup(GTK_LABEL(field->widget),new);
    }
}

static void update_flag(flag_t *flag, int new)
{
	char tmp[80];
    if (flag->val != new) {
	flag->val = new;
	if (new)
		snprintf(tmp,80,"<span background=\"#000000\" foreground=\"#DDDDDD\">%s</span>",gtk_label_get_text(GTK_LABEL(flag->widget)));
	else	
		snprintf(tmp,80,"%s",gtk_label_get_text(GTK_LABEL(flag->widget)));
	gtk_label_set_markup(GTK_LABEL(flag->widget),tmp);
	gtk_widget_set_sensitive(flag->widget,new);
	}
}

static void update_icon(char *type,GtkWidget *container)
{
	GtkWidget *ctype_pixmap;
	char *fname;
	
	fname=g_strdup_printf("%spccard-%s.png",PIXMAP_PATH,type);
	if (access(fname,R_OK))
	{
		free(fname);
		fname=g_strdup_printf("%spccard-unknown.png",PIXMAP_PATH);
	}
	ctype_pixmap = gtk_bin_get_child(GTK_BIN(container));
	if (ctype_pixmap)
	{
		gtk_container_remove(GTK_CONTAINER(container),ctype_pixmap);
	}
    ctype_pixmap = gpe_create_pixmap (container, fname, 48, 48);
    gtk_container_add (GTK_CONTAINER(container), ctype_pixmap);
    gtk_widget_show (GTK_WIDGET (ctype_pixmap));
	free(fname);
}

static int do_update(GtkWidget *widget)
{
    FILE *f;
    int i, j, event, ret, state;
    cs_status_t status;
    config_info_t cfg;
    char s[80], *t, d[80], type[20], drv[20], io[20], irq[4];
    ioaddr_t stop;
    struct stat buf;
    static time_t last = 0;
    time_t now;
    struct tm *tm;
    fd_set fds;
    struct timeval timeout;


    /* Poll for events */
    FD_ZERO(&fds);
    for (i = 0; i < ns; i++)
	FD_SET(st[i].fd, &fds);
    timeout.tv_sec = timeout.tv_usec = 0;
    ret = select(MAX_SOCK+4, &fds, NULL, NULL, &timeout);
    now = time(NULL);
    tm = localtime(&now);
    if (ret > 0) {
	for (i = 0; i < ns; i++) {
	    if (!FD_ISSET(st[i].fd, &fds))
		continue;
	    ret = read(st[i].fd, &event, 4);
	    if (ret != 4) continue;
	    for (j = 0; j < NTAGS; j++)
		if (event_tag[j].event == event) break;
	    if (j == NTAGS)
		sprintf(s, "%2d:%02d:%02d  socket %d: unknown event 0x%x",
			tm->tm_hour, tm->tm_min, tm->tm_sec, i, event);
	    else
		sprintf(s, "%2d:%02d:%02d  socket %d: %s", tm->tm_hour,
			tm->tm_min, tm->tm_sec, i, event_tag[j].name);
//	    fl_addto_browser(event_log, s);
	}
    }

    if ((stat(stabfile, &buf) == 0) && (buf.st_mtime >= last)) {
	f = fopen(stabfile, "r");
	if (f == NULL)
	    return TRUE;
	
	if (flock(fileno(f), LOCK_SH) != 0) {
	    do_alert("flock(stabfile) failed: %s", strerror(errno));
	    return FALSE;
	}
	last = now;
	fgetc(f);
	for (i = 0; i < ns; i++) {
	    if (!fgets(s, 80, f)) break;
	    s[strlen(s)-1] = '\0';
	    update_field(&st[i].card, s+9);
	    *d = '\0';
	    *type = '\0';
	    *drv = '\0';
	    for (;;) {
		int c = fgetc(f);
		if ((c == EOF) || (c == 'S')) {
		    update_field(&st[i].dev, d);
		    update_field(&st[i].type, type);
			update_icon(type,st[i].statusbutton);
		    update_field(&st[i].driver, drv);			
		    break;
		} else {
		    fgets(s, 80, f);
			t = s;
			t = strchr(t, '\t')+1;
			snprintf(type,strcspn(t, "\t\n")+1,"%s",t);
			t = strchr(t, '\t')+1;
			snprintf(drv,strcspn(t, "\t\n")+1,"%s",t);
		    for (j = 0; j < 2; j++)
			t = strchr(t, '\t')+1;
		    t[strcspn(t, "\t\n")] = '\0';
		    if (*d == '\0')
			strcpy(d, t);
		    else {
			strcat(d, ", ");
			strcat(d, t);
		    }
		}
	    }
	}
	flock(fileno(f), LOCK_UN);
	fclose(f);
    }

    for (i = 0; i < ns; i++) {
	
	state = S_EMPTY;
	status.Function = 0;
	ioctl(st[i].fd, DS_GET_STATUS, &status);
	if (strcmp(st[i].card.str, "empty") == 0) {
	    if (status.CardState & CS_EVENT_CARD_DETECT)
		state = S_PRESENT;
	} else {
	    if (status.CardState & CS_EVENT_PM_SUSPEND)
		state = S_SUSPEND;
	    else {
		if (status.CardState & CS_EVENT_READY_CHANGE)
		    state = S_READY;
		else
		    state = S_BUSY;
	    }
	}
	
	if (state != st[i].o_state) {
	    st[i].o_state = state;
	    for (j = 1; j <= 6; j++)
//		fl_set_menu_item_mode(st[i].menu, j, FL_PUP_GRAY);
	    switch (state) {
	    case S_EMPTY:
		update_field(&st[i].state, "");
		break;
	    case S_PRESENT:
//		fl_set_menu_item_mode(st[i].menu, 6, FL_PUP_NONE);
		update_field(&st[i].state, "");
		break;
	    case S_READY:
/*		fl_set_menu_item_mode(st[i].menu, 1, FL_PUP_NONE);
		fl_set_menu_item_mode(st[i].menu, 2, FL_PUP_NONE);
		fl_set_menu_item_mode(st[i].menu, 3, FL_PUP_NONE);
		fl_set_menu_item_mode(st[i].menu, 5, FL_PUP_NONE);
*/		snprintf(d,80,"<span foreground=\"#00A000\">%s</span>",_("ready"));
		update_field(&st[i].state, d);
		break;
	    case S_BUSY:
/*		fl_set_menu_item_mode(st[i].menu, 1, FL_PUP_NONE);
		fl_set_menu_item_mode(st[i].menu, 2, FL_PUP_NONE);
		fl_set_menu_item_mode(st[i].menu, 3, FL_PUP_NONE);
		fl_set_menu_item_mode(st[i].menu, 5, FL_PUP_NONE);
*/		snprintf(d,80,"<span foreground=\"#B00000\">%s</span>",_("not ready"));
		update_field(&st[i].state, d);
		break;
	    case S_SUSPEND:
/*		fl_set_menu_item_mode(st[i].menu, 1, FL_PUP_NONE);
		fl_set_menu_item_mode(st[i].menu, 4, FL_PUP_NONE);
		fl_set_menu_item_mode(st[i].menu, 5, FL_PUP_NONE);
*/		snprintf(d,80,"<span foreground=\"#000090\">%s</span>",_("suspended"));
		update_field(&st[i].state, d);
		break;
	    }
	}

	strcpy(io, "");
	strcpy(irq, "");
	memset(&cfg, 0, sizeof(cfg));
	ret = ioctl(st[i].fd, DS_GET_CONFIGURATION_INFO, &cfg);
	if (cfg.Attributes & CONF_VALID_CLIENT) {
	    if (cfg.AssignedIRQ != 0)
		sprintf(irq, "%d", cfg.AssignedIRQ);
	    if (cfg.NumPorts1 > 0) {
		stop = cfg.BasePort1+cfg.NumPorts1;
		if (cfg.NumPorts2 > 0) {
		    if (stop == cfg.BasePort2)
			sprintf(io, "%#x-%#x", cfg.BasePort1,
				stop+cfg.NumPorts2-1);
		    else
			sprintf(io, "%#x-%#x, %#x-%#x", cfg.BasePort1, stop-1,
				cfg.BasePort2, cfg.BasePort2+cfg.NumPorts2-1);
		} else
		    sprintf(io, "%#x-%#x", cfg.BasePort1, stop-1);
	    }
	}
	update_field(&st[i].irq, irq);
	update_field(&st[i].io, io);

	update_flag(&st[i].cd, status.CardState & CS_EVENT_CARD_DETECT);
	update_flag(&st[i].vcc, cfg.Vcc > 0);
	update_flag(&st[i].vpp, cfg.Vpp1 > 0);
	update_flag(&st[i].wp, status.CardState & CS_EVENT_WRITE_PROTECT);
    }
	return TRUE;
}


/* --- gpe-conf interface --- */

void
Cardinfo_Free_Objects ()
{
}

void
Cardinfo_Save ()
{

}

void
Cardinfo_Restore ()
{
	
}

GtkWidget *
Cardinfo_Build_Objects (void)
{
  GtkWidget *label, *ctype_pixmap;
  GtkWidget *table, *hbox;
  gchar iname[100];
  GtkTooltips *tooltips;
  int i;
	
  tooltips = gtk_tooltips_new();
  
  notebook = gtk_notebook_new();
  gtk_container_set_border_width (GTK_CONTAINER (notebook), gpe_get_border ());
  gtk_object_set_data(GTK_OBJECT(notebook),"tooltips",tooltips);
  
  for (i=0;i<ns;i++)
  {
	  table = gtk_table_new(3,8,FALSE);
	  gtk_tooltips_set_tip(tooltips,table,_("PC/CF card socket information."),NULL);
      gtk_container_set_border_width (GTK_CONTAINER (table), gpe_get_border());
      gtk_table_set_row_spacings (GTK_TABLE (table), 0);
      gtk_table_set_col_spacings (GTK_TABLE (table), gpe_get_boxspacing());
	  
	  sprintf(iname,"%s %d",_("Socket"),i);
	  label = gtk_label_new(iname);
	  gtk_notebook_append_page(GTK_NOTEBOOK(notebook),table,label);
	  gtk_tooltips_set_tip(tooltips,label,_("PC/CF card socket information."),NULL);
		
	  /* box for header */
	  hbox = gtk_hbox_new(FALSE,gpe_get_boxspacing());
	  
	  /* top label */
	  label = gtk_label_new(NULL);
	  snprintf(iname,100,"<b>%s %d</b>",_("Information for socket"),i);
	  gtk_label_set_markup(GTK_LABEL(label),iname);
	  gtk_misc_set_alignment(GTK_MISC(label),0.0,0.5);
      gtk_box_pack_start(GTK_BOX(hbox),label,FALSE,TRUE,0);

	  /* graphic button */
	  label = gtk_button_new();
	  gtk_widget_set_size_request(label,55,55);
	  st[i].statusbutton = label;
      gtk_box_pack_start(GTK_BOX(hbox),label,FALSE,TRUE,5);
	  gtk_button_set_relief(GTK_BUTTON(label),GTK_RELIEF_NONE);
	  
      ctype_pixmap = gpe_create_pixmap (label, PIXMAP_PATH "pccard-unknown.png", 48, 48);
      gtk_container_add (GTK_CONTAINER(label), ctype_pixmap);
	  
 	  gtk_table_attach(GTK_TABLE(table),hbox,0,4,0,1,GTK_FILL,0,0,0);
     
	  /* fields */
	  new_field(&st[i].card,table,1,_("Card:"));
	  new_field(&st[i].state,table,2,_("State:"));
	  new_field(&st[i].dev,table,3,_("Device:"));
	  new_field(&st[i].driver,table,4,_("Driver:"));
	  new_field(&st[i].type,table,5,_("Type:"));
	  new_field(&st[i].io,table,6,_("I/O:"));
	  new_field(&st[i].irq,table,7,_("IRQ:"));

	  /* flags */
	  hbox = gtk_hbox_new(TRUE,gpe_get_boxspacing());
      gtk_table_attach(GTK_TABLE(table),hbox,0,3,8,9,GTK_FILL,0,0,3);
	  
	  new_flag(&st[i].cd,hbox,"CD");
	  new_flag(&st[i].vcc,hbox,"VCC");
	  new_flag(&st[i].vpp,hbox,"VPP");
	  new_flag(&st[i].wp,hbox,"WP");
	  
  }
  
  gtk_timeout_add(500,(GtkFunction)do_update,NULL);
  return notebook;
}

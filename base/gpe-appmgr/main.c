/* gpe-appmgr - a program launcher

   Copyright 2002 Robert Mibus;

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
*/

/* Gtk etc. */
#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk/gdkkeysyms.h>
#include <gdk/gdkprivate.h>
#include <gdk/gdkx.h>

/* directory listing */
#include <sys/types.h>
#include <dirent.h>
#include <string.h>

/* catch SIGHUP */
#include <signal.h>

/* I/O */
#include <stdio.h>

/* malloc / free */
#include <stdlib.h>

/* time() */
#include <time.h>

#include <X11/Xatom.h>

/* i18n */
#include <libintl.h>
#include <locale.h>

/* GPE */
#include <gpe/init.h>
#include <gpe/pixmaps.h>
#include <gpe/render.h>
#include <gpe/spacing.h>
#include <gpe/gpe-iconlist.h>

/* everything else */
#include "main.h"

#include "cfg.h"
#include "package.h"
#include "plugin.h"
#include "popupmenu.h"
#include "xsi.h"
#include <dlfcn.h>
#include "tray.h"

//#define DEBUG
#ifdef DEBUG
#define DBG(x) {fprintf x ;}
#define TRACE(x) {fprintf(stderr,"TRACE: " x "\n");}
#else
#define TRACE(x) ;
#define DBG(x) ;
#endif

GList *recent_items=NULL;
GList *forced_groups=NULL;

GtkWidget *current_button=NULL;
int current_button_is_down=0;

GtkWidget *recent_tab=NULL;

time_t last_update=0;

GSList *translate_list;

/* For not starting an app twice after a double click */
int ignore_press = 0;

GdkPixbuf *default_pixbuf;

struct gpe_icon my_icons[] = {
	{ "icon", "/usr/share/pixmaps/gpe-appmgr.png" },
	//{ "bg", "/home/mibus/bg.png" },
	{NULL, NULL}
};

void create_recent_list ();
void create_recent_box(GtkBox *cont);
char *translate_group_name (char *name);

extern gboolean gpe_appmgr_start_xsettings (void);

void autohide_labels (int page) {
	GtkWidget *hb;
	int i=0;
	int pagenum = page == -1 ? gtk_notebook_get_current_page (GTK_NOTEBOOK(notebook)) : page;

	while (1)
	{
		GtkWidget *page_contents;
		GList *children;

		page_contents = gtk_notebook_get_nth_page (GTK_NOTEBOOK(notebook), i);
		if (!page_contents)
			break;


		hb = gtk_notebook_get_tab_label (GTK_NOTEBOOK(notebook), page_contents);
		if (!hb)
			continue;



		children = gtk_container_children (GTK_CONTAINER(hb));
		while (children)
		{
			if (GTK_IS_LABEL(children->data))
			{
				if (!cfg_options.auto_hide_group_labels || i == pagenum)
					gtk_widget_show (GTK_WIDGET(children->data));
				else
					gtk_widget_hide (GTK_WIDGET(children->data));
			}
			children = children->next;
		}

		i++;
	}
}

void nb_switch (GtkNotebook *nb, GtkNotebookPage *page, guint pagenum)
{
	autohide_labels (pagenum);
}

gint unignore_press (gpointer data)
{
	ignore_press = 0;
	return FALSE;
}

/* Remove the appmgr (not plugin) tabs from the notebook */
void clear_appmgr_tabs ()
{
	int i=0;
	while (1) {
		GtkWidget *page;
		page = gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook),i);
		if (page == NULL)
			return;
		if (!GTK_IS_SOCKET(page))
			gtk_notebook_remove_page (GTK_NOTEBOOK(notebook), i);
		else
			i++;
	}
}

char *find_icon (char *base)
{
	char *temp;
	FILE *inp;
	int i;

	char *directories[]=
	{
		"/usr/share/pixmaps",
		"/usr/X11R6/include/X11/pixmaps",
		"/mnt/ramfs/share/pixmaps",
		"/mnt/hda/usr/share/pixmaps",
		"/mnt/hda",
		NULL
	};

	if (!base)
		return NULL;

	inp = fopen (base, "r");
	if (inp)
	{
		fclose (inp);
		return (char *) strdup (base);
	}

	for (i=0;directories[i];i++)
	{
		temp = g_strdup_printf ("%s/%s", directories[i], base);
		inp = fopen (temp, "r");
		if (inp)
		{
			fclose (inp);
			return temp;
		}
		g_free (temp);
	}
	return NULL;
}

GtkWidget *create_icon_pixmap (GtkStyle *style, char *fn, int size)
{
	GdkPixbuf *pixbuf, *spixbuf;
	GtkWidget *w;

	pixbuf = gdk_pixbuf_new_from_file (fn, NULL);

	if (pixbuf == NULL)
		return NULL;

	spixbuf = gdk_pixbuf_scale_simple (pixbuf, size, size, 
					   GDK_INTERP_BILINEAR);
	gdk_pixbuf_unref (pixbuf);
	w = gpe_render_icon (style, spixbuf);
	gdk_pixbuf_unref (spixbuf);
	return w;
}

char *get_closest_icon (struct package *p, int iconsize)
{
	char *tmp;
	char *fn;

	tmp = g_strdup_printf ("icon%d", iconsize);
	fn = package_get_data (p, tmp);
	if (!fn)
		fn = package_get_data (p, "icon");
	if (!fn)
		fn = package_get_data (p, "icon48");
	if (!fn)
	{
		/* TODO: run 48->16
		 * maybe get actual closest, preferring larger?
		 */
	}

	g_free (tmp);

	return fn;
}

char *get_icon_fn (struct package *p, int iconsize)
{
	char *fn, *full_fn;

	fn = get_closest_icon(p,iconsize);
	if (!fn)
		return fn;
	full_fn = find_icon(fn);

	return full_fn;
}

int has_icon (struct package *p)
{
	char *tmp;
	if ((tmp = get_icon_fn(p,0)))
	{
		free (tmp);
		return 1;
	} else
		return 0;
}

void create_recent_list ()
{
#if 0
	GtkWidget *hb;
	GList *this_item;
	GList *cl;
	GtkWidget *w;

	TRACE("create_recent_list");

	if (recent_tab == NULL)
		return;

	/* Remove the previous list */
	cl = gtk_container_children(GTK_CONTAINER(recent_tab));
	if (cl)
	{
		w = cl->data;
		gtk_widget_destroy (w);
	}

	hb = gtk_hbox_new (0,0);

	gtk_container_add(GTK_CONTAINER(recent_tab),
			  hb);

	this_item = recent_items;
	while (this_item)
	{
		struct package *p;
		GtkWidget *btn;
		GtkWidget *img=NULL;
		char *icon;

		p = (struct package *) this_item->data;
		DBG((stderr, "recent info: %s\n", package_get_data (p, "title")));
		/* Button picture */
		icon = find_icon (get_closest_icon (p, 16));
		if (icon)
		{
			img = create_icon_pixmap (recent_tab->style, icon, 16);
			free (icon);
		}
		if (!img)
			img = create_icon_pixmap (recent_tab->style,
						 "/usr/share/pixmaps/menu_unknown_program16.png",
						 16);

		/* The button itself */
		btn = gtk_event_box_new ();

		gtk_container_add (GTK_CONTAINER(btn),img);
		gtk_object_set_data (GTK_OBJECT(btn), "program", (gpointer)p);
		gtk_signal_connect( GTK_OBJECT(btn), "button_release_event",
				    GTK_SIGNAL_FUNC(btn_released), NULL);
		gtk_signal_connect( GTK_OBJECT(btn), "button_press_event",
				    GTK_SIGNAL_FUNC(btn_pressed), NULL);
		gtk_signal_connect( GTK_OBJECT(btn), "enter_notify_event",
				    GTK_SIGNAL_FUNC(btn_enter), NULL);
		gtk_signal_connect( GTK_OBJECT(btn), "leave_notify_event",
				    GTK_SIGNAL_FUNC(btn_leave), NULL);

		gtk_box_pack_start (GTK_BOX(hb), btn, 0, 0, 0);

		this_item = this_item->next;
	}

	gtk_widget_show_all (recent_tab);
#endif
}

void cb_clicked (GtkWidget *il, gpointer udata, gpointer data) {
	struct package *p;
	p = udata;
	printf ("Clicked: %s\n", package_get_data (p, "title"));
	run_program (package_get_data (p, "command"),
		     cfg_options.use_windowtitle ? package_get_data (p, "windowtitle") : NULL);
}

void cb_popup (GtkWidget *il, gpointer udata, gpointer data) {
	struct package *p;
	p = udata;
	printf ("Popup: %s\n", package_get_data (p, "title"));
	popup_menu_activate (udata);
}

/* Make the contents for a notebook tab.
 * Generally we only want one group, but NULL means
 * ignore group setting (ie. "All").
 */
GtkWidget *create_tab (GList *all_items, char *current_group, tab_view_style style)
{
	GtkWidget *il;
	GList *this_item;
	//char *icon_file;

	il = gpe_iconlist_new ();

	gtk_signal_connect (GTK_OBJECT (il), "clicked", (GtkSignalFunc)cb_clicked, NULL);
	gtk_signal_connect (GTK_OBJECT (il), "show_popup", (GtkSignalFunc)cb_popup, NULL);
	//icon_file = g_strdup_printf ("/usr/share/pixmaps/group_%s.png", current_group ? current_group : "All");
	//gpe_iconlist_set_bg (il, icon_file);
	//g_free (icon_file);

	gpe_iconlist_set_bg (GPE_ICONLIST(il), "/usr/share/pixmaps/gpe-default-bg.png");


	this_item = all_items;
	while (this_item)
	{
		struct package *p;

		p = (struct package *) this_item->data;

		if (!current_group || (current_group && !strcmp (current_group, package_get_data (p, "section"))))
			gpe_iconlist_add_item (GPE_ICONLIST(il),
					       package_get_data (p, "title"),
					       get_icon_fn (p, 48),
					       (gpointer)p);
		
		this_item = this_item->next;

	}

	gtk_widget_show (il);
	return il;
}

/* Creates the image/label combo for a tab.
 */
GtkWidget *create_tab_label (char *name, char *icon_file, GtkStyle *style)
{
	GtkWidget *img=NULL,*lbl,*hb;

	img = create_icon_pixmap (style, icon_file, 18);
	if (!img)
		img = create_icon_pixmap (style, "/usr/share/pixmaps/menu_unknown_group16.png", 16);

	lbl = gtk_label_new (translate_group_name (name));

	hb = gtk_hbox_new (FALSE, 0);
	if (img)
		gtk_box_pack_start_defaults (GTK_BOX(hb), img);
	gtk_box_pack_start (GTK_BOX(hb), lbl, FALSE, FALSE, gpe_get_boxspacing());

	gtk_widget_show_all (hb);
	if (cfg_options.auto_hide_group_labels)
		gtk_widget_hide (lbl);

	return hb;
}

/* Creates the image/label combo for the tab
 * of a specified group.
 */
GtkWidget *create_group_tab_label (char *group, GtkStyle *style)
{
	GtkWidget *hb;
	char *icon_file;

	icon_file = g_strdup_printf ("/usr/share/pixmaps/group_%s.png", group);
	hb = create_tab_label (group, icon_file, style);
	g_free (icon_file);

	return hb;
}

/* Wipe the old tabs / icons and replace them with whats
 * currently supposed to be there */
void refresh_tabs ()
{
	GList *l;
	int old_tab;

	TRACE ("refresh_tabs");

	old_tab = gtk_notebook_get_current_page (GTK_NOTEBOOK(notebook));

	gtk_widget_hide (notebook);

	clear_appmgr_tabs ();

	/* Create the normal tabs if wanted */
	l = groups;
	while (l)
	{
		gtk_notebook_append_page (
			GTK_NOTEBOOK(notebook),
			create_tab (items, l->data, cfg_options.tab_view),
			create_group_tab_label(l->data, notebook->style));

		l = l->next;
	}

	if (old_tab != -1)
		gtk_notebook_set_page (GTK_NOTEBOOK(notebook), old_tab);

	l =  gtk_container_children (GTK_CONTAINER(window));
	if (l && l->data)
		create_recent_box (l->data);

	gtk_widget_show_all (notebook);

	autohide_labels (0);

	TRACE ("refresh_tabs: <end>");
}

GList *load_simple_list (char *fn)
{
	GList *l=NULL;
	char line[1024], *t;
	FILE *inp;

	inp = fopen (fn, "r");
	if (!inp)
		return NULL;

	t = line;
	do
	{
		*t = getc (inp);
		if (*t == '\n' || t - line > 1000)
		{
			*t = 0;
			l = g_list_append (l, strdup (line));
			t = line;
			continue;
		}
		t++;
	} while (!feof (inp));

	fclose (inp);

	return l;
}

GList *load_forced_groups ()
{
	char *fn;
	GList *l;

	fn = g_strdup_printf ("%s/.gpe/gpe-appmgr_group-order", g_get_home_dir ());

	l = load_simple_list (fn);

	g_free (fn);

	return l;
}

GList *load_ignored_items ()
{
	char *fn;
	GList *l;

	fn = g_strdup_printf ("%s/.gpe/gpe-appmgr_ignored-items", g_get_home_dir ());

	l = load_simple_list (fn);

	g_free (fn);

	return l;
}

GList *load_translations_from (char *fmt, char *a1, char *a2)
{
	GList *rl;
	char *p = g_strdup_printf (fmt, a1, a2);
	rl = load_simple_list (p);
	free (p);
	return rl;
}

struct translation_entry
{
  gchar *a, *b;
};


GSList *load_group_translations ()
{
	GList *l = NULL, *i;
	GSList *tmap = NULL;

	char *locale = setlocale (LC_MESSAGES, NULL);
	if (locale) {
		l = load_translations_from ("%s/.gpe/gpe-appmgr_group-map.%s", (char*)g_get_home_dir (), locale);
		if (!l)
			l = load_translations_from ("/usr/share/gpe/group-map.%s", locale, NULL);
		if (!l) {
			gchar *ln = g_strdup (locale);
			char *p = strchr (ln, '_');
			if (p) {
				*p = 0;
				l = load_translations_from ("%s/.gpe/gpe-appmgr_group-map.%s",  (char*)g_get_home_dir (), ln);
				if (!l)
					l = load_translations_from ("/usr/share/gpe/group-map.%s", ln, NULL);
			}
			g_free (ln);
		}
	}
	if (!l)
		l = load_translations_from ("%s/.gpe/gpe-appmgr_group-map",  (char*)g_get_home_dir (), NULL);
	if (!l)
		l = load_translations_from ("/usr/share/gpe/group-map", NULL, NULL);

	for (i = l; i; i = i->next) {
		char *s = i->data;
		char *p = strchr (s, '=');
		if (p) {
			struct translation_entry *t = g_malloc (sizeof (*t));
			*p++ = 0;
			t->a = g_strdup (s);
			t->b = g_strdup (p);
			tmap = g_slist_append (tmap, t);
		}
		free (i->data);
	}
	g_list_free (i);

	return tmap;
}

char *translate_group_name (char *name)
{
	GSList *i;
	
	for (i = translate_list; i; i = i->next) {
		struct translation_entry *t = i->data;
		if (!strcmp (t->a, name)) {
			return t->b;
		}
	}

	return name;
}

/* clean_up():
 * free all data etc.
 */
void clean_up ()
{
	GList *l;

	TRACE("clean_up");

	l = items;
	while (l)
	{
		if (l->data)
			package_free ((struct package *)l->data);
		l = l->next;
	}
	g_list_free (items);

	l = groups;
	while (l)
	{
		if (l->data)
			free (l->data);
		l = l->next;
	}
	g_list_free (groups);

	groups = items = NULL;
}

void cb_package_add (struct package *p)
{
	TRACE ("cb_package_add");

	/* add the item to the list */
	if (p)
	{
		/* The package has to provide an icon and a section */
		if (has_icon (p) && package_get_data (p, "section"))
		{
			/* Don't allow X/Y section names (eg. "Games/Strategy") */
			if (strchr (package_get_data (p, "section"), '/'))
				*(strchr(package_get_data (p, "section"), '/'))=0;
			items = g_list_insert_sorted (items, p, (GCompareFunc)package_compare);
			/* Update the groups if necessary */
			if ((g_list_find_custom (groups, package_get_data (p, "section"),
						 (GCompareFunc) strcmp) == NULL) &&
			    (g_list_find_custom (forced_groups, package_get_data (p, "section"),
						 (GCompareFunc) strcmp) == NULL))
				groups = g_list_insert_sorted (groups, strdup(package_get_data(p,"section")), (GCompareFunc)strcasecmp);
		}
		else
			package_free (p);

	}
}

/* Reloads the menu files into memory */
int refresh_list ()
{
//	GList *l, *old_items, *old_groups;
	GList *ignored_items;
	DIR *dir;
	struct dirent *entry;
	int i;
	char *user_menu=NULL, *home_dir;
	GSList *j;

	char *directories[]=
	{
		"/usr/local/lib/menu",
		"/usr/lib/menu",
		"/home/mibus/programming/c/gpe/gpe-appmgr/dist/usr/lib/menu", /* test dir */
		"/mnt/ramfs/lib/menu",
		"/mnt/hda/usr/lib/menu",
		NULL, /* Placeholder for ~/.gpe/menu */
		NULL
	};

	TRACE ("refresh_list");

	if ((home_dir = (char*) getenv ("HOME")))
	{
		for (i=0;directories[i];i++)
			;
		directories[i] = user_menu = g_strdup_printf ("%s/.gpe/menu", home_dir);
	}

	/* Wipe out 'recent' list */
	recent_items = NULL;
	create_recent_list ();

	/* Remove the tap-hold popup menu timeout if it's there */
	popup_menu_cancel ();

//	old_items = items;
//	old_groups = groups;

//	items = groups = NULL;
	clean_up ();

	/* flush old translations */
	for (j = translate_list; j; j = j->next) {
		struct translation_entry *t = j->data;
		g_free (t->a);
		g_free (t->b);
		g_free (t);
	}
	g_slist_free (translate_list);

	forced_groups = load_forced_groups ();
	ignored_items = load_ignored_items ();
	translate_list = load_group_translations ();
	
	for (i=0;directories[i];i++)
	{
		dir = opendir (directories[i]);
		if (!dir)
			continue;
		while ((entry = readdir (dir)))
		{
			char *temp;

			if (entry->d_name[0] == '.')
				continue;

			/* read the file if we don't want to ignore it */
			temp = g_strdup_printf ("%s/%s", directories[i], entry->d_name);
			if (g_list_find_custom (ignored_items, temp, (GCompareFunc)strcmp))
			{
				g_free (temp);
				continue;
			}

			package_read_to (temp, cb_package_add);
			g_free (temp);
		}
		closedir (dir);
	}

	groups = g_list_concat (forced_groups, groups);

#if 0
	/* free the old data. note that we do it now so that
	   any stuff that references it can't ever
	   reference invalid data */
	l = old_items;
	while (l)
	{
		if (l->data)
			package_free ((struct package *)l->data);
		l = l->next;
	}
	g_list_free (old_items);

	l = old_groups;
	while (l)
	{
		if (l->data)
			free (l->data);
		l = l->next;
	}
	g_list_free (old_groups);
#endif

	/* We allocated a string for the user's menu dir; free it */
	if (user_menu)
		g_free (user_menu);

	TRACE ("refresh_list: end");
	return FALSE;
}

/* refresh_tabs() is called through GTK so that
   the window already exists; thus we can get it's
   size and determine the number of columns available.
   It's also called when XRANDR does its job.

   It's done this way (a timeout via a draw callback)
   because calling refresh_tabs() directly from the
   draw callback was causing crashes (apparently
   within Gtk, not sure though)
*/

void cb_win_draw (GtkWidget *widget,
		  gpointer user_data)
{
	static int last_width = 0;
	TRACE ("cb_win_draw");
	if (last_width)
		return;
	DBG((stderr, "window: %p\n", window));
	DBG((stderr, "last_width: %d\n", last_width));
	DBG((stderr, "window->allocation.width: %d\n",
	     window->allocation.width));

	if (window && last_width != window->allocation.width)
	{
		refresh_tabs();
		last_width = window->allocation.width;
	}
	TRACE("cb_win_draw: end");
}

/* run on SIGHUP
 * just refreshes the program list, eg. if a new
 * program was just added */
static void catch_signal (int signo)
{
	TRACE ("catch_signal");
	/* FIXME: transfer this if needed */
	recent_items = NULL;

	cfg_load_if_newer (last_update);
	refresh_list ();
	refresh_tabs();
	last_update = time (NULL);
}

static GdkFilterReturn
filter (GdkXEvent *xevp, GdkEvent *ev, gpointer p)
{
  XEvent *xev = (XEvent *)xevp;
  
  if (xev->type == ClientMessage || xev->type == ReparentNotify)
    {
      XAnyEvent *any = (XAnyEvent *)xev;
      tray_handle_event (any->display, any->window, xev);
      if (xev->type == ReparentNotify)
	gtk_widget_show_all (GTK_WIDGET (p));
    }
  return GDK_FILTER_CONTINUE;
}

/* Create the 'recent' list as a dock app or normal widget */
void create_recent_box(GtkBox *cont)
{
#if 0
	GdkAtom window_type;
	GdkAtom window_type_dock;
	static GtkWidget *w=NULL;
	int can_dock=0;

	TRACE ("create_recent_box");

	if (w != NULL) {
		w = NULL;
	} else if (recent_tab != NULL) {
		gtk_widget_destroy (recent_tab);
		w = recent_tab = NULL;
	}

	if (cfg_options.show_recent_apps == 0)
		return;

        recent_tab = gtk_vbox_new(0,0);

	w = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_container_set_border_width(GTK_CONTAINER (w), 0);
	gtk_container_add(GTK_CONTAINER(w), recent_tab);
	gtk_widget_set_usize (w, 16*cfg_options.recent_apps_number, 16);

	window_type = gdk_atom_intern("_NET_WM_WINDOW_TYPE", FALSE);
	window_type_dock = gdk_atom_intern("_NET_WM_WINDOW_TYPE_DOCK", FALSE);
	gtk_widget_realize(w);
	//gtk_widget_show_all(recent_tab);
	gdk_property_change(w->window, window_type, XA_ATOM, 32, GDK_PROP_MODE_REPLACE, (guchar *) &window_type_dock, 1);
	can_dock = tray_init (GDK_WINDOW_XDISPLAY (w->window), GDK_WINDOW_XWINDOW (w->window));
	gdk_window_add_filter (w->window, filter, w);

	if (!can_dock)
	{
		//gtk_container_remove (GTK_CONTAINER(w), recent_box);
		gtk_widget_destroy (w);
		recent_tab = gtk_vbox_new(0,0);
		gtk_box_pack_start (GTK_BOX(cont), recent_tab,0,0,0);
		gtk_widget_show (recent_tab);
	}

	TRACE ("create_recent_box: end");
#endif
}

void icons_page_up_down (int down)
{
	GtkWidget *sw;
	GtkAdjustment *adj;
	int page;
	gfloat newval;

	page = gtk_notebook_get_current_page (GTK_NOTEBOOK(notebook));
	sw = gtk_notebook_get_nth_page (GTK_NOTEBOOK(notebook), page);

	adj = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW(sw));

	if (down)
		newval = adj->value + adj->page_increment;
	else
		newval = adj->value - adj->page_increment;

	if (newval < adj->lower)
		newval = adj->lower;
	else if (newval > adj->upper-adj->page_size)
		newval = adj->upper-adj->page_size;

	gtk_adjustment_set_value(adj, newval);
}

gint keysnoop (GtkWidget *grab_widget, GdkEventKey *event, gpointer func_data)
{
	if (event->type != GDK_KEY_PRESS)
		return 1;

	switch (event->keyval) {
	case GDK_Up:
		icons_page_up_down (0);
		break;
	case GDK_Down:
		icons_page_up_down (1);
		break;
	case GDK_Left:
		gtk_notebook_prev_page (GTK_NOTEBOOK(notebook));
		break;
	case GDK_Right:
		gtk_notebook_next_page (GTK_NOTEBOOK(notebook));
		break;
	default:
		DBG ((stderr, "Unhandled key: %d\n", event->keyval));
		DBG ((stderr, "     (%s)\n", gdk_keyval_name(event->keyval)));
		return 0;
	}
	return 1;
}

void set_window_pixmap ()
{
	GdkPixmap *pmap;
	GdkBitmap *bmap;

	if (gpe_find_icon_pixmap ("icon", &pmap, &bmap))
		gdk_window_set_icon (window->window, NULL, pmap, bmap);
}


gint on_window_delete (GtkObject *object, gpointer data)
{
	switch (cfg_options.on_window_close)
	{
	case WINDOW_CLOSE_IGNORE:
		/* TODO: send _WM_NET_PING(?) */
		return TRUE;
		break;
	case WINDOW_CLOSE_HIDE: /* doesn't work properly :( */
	{
		Display *display;
		Window window;
		display = GDK_WINDOW_XDISPLAY (GTK_WIDGET(object)->window);
		window = GDK_WINDOW_XWINDOW (GTK_WIDGET(object)->window);
		XLowerWindow (display, GDK_WINDOW_XWINDOW(GTK_WIDGET(object)->window));
	}
		break;
	case WINDOW_CLOSE_EXIT:
		gtk_main_quit ();
	break;
	}
	return FALSE;
}

/* Create the UI for the main window and
 * stuff
 */
void create_main_window()
{
	GtkWidget *vbox;
//	GtkWidget *widget, *sw;
	TRACE ("create_main_window");

	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

	gtk_window_set_title (GTK_WINDOW(window), "Programs");
	gtk_widget_set_usize (window, 235,280);
	gtk_widget_realize (window);
	set_window_pixmap();

#ifndef DEBUG
	gtk_signal_connect (GTK_OBJECT(window), "delete_event",
			    (GtkSignalFunc)on_window_delete, NULL);
#else
	gtk_signal_connect (GTK_OBJECT(window), "delete_event",
			    (GtkSignalFunc)gtk_main_quit, NULL);
#endif

	notebook = gtk_notebook_new ();
	gtk_notebook_set_homogeneous_tabs(GTK_NOTEBOOK(notebook), FALSE);
	gtk_notebook_set_scrollable (GTK_NOTEBOOK(notebook), TRUE);
	gtk_notebook_set_tab_border (GTK_NOTEBOOK(notebook), 0);
	gtk_signal_connect (GTK_OBJECT(notebook), "switch_page",
			    (GtkSignalFunc)nb_switch, NULL);

	/* If a person releases the stylus when over the table, we want
	   current_button etc. to get cleared properly, so: */
	/*
	gtk_signal_connect( GTK_OBJECT(notebook), "button_release_event",
			    GTK_SIGNAL_FUNC(btn_released), NULL);
	*/

	vbox = gtk_vbox_new(0,0);
	gtk_container_add (GTK_CONTAINER (window), vbox);
	gtk_box_pack_start (GTK_BOX(vbox), notebook, 1, 1, 0);

	gtk_widget_show_all (window);

	/* Example plugin usage:
	 plugin_load ("plug"); */

	/* Send all key events to the one place */
	gtk_key_snooper_install ((GtkKeySnoopFunc)keysnoop,NULL);

	cb_win_draw (NULL, NULL);
}

/* main():
 * init the libs, setup the signal handler,
 * run gtk bits
 */
int main(int argc, char *argv[]) {
        struct sigaction sa_old, sa_new;
	TRACE ("main");

	/* Hmm */
	items = groups = NULL;

	/* Init gtk & friends */
	if (gpe_application_init (&argc, &argv) == FALSE)
		exit (1);

	if (gpe_load_icons (my_icons) == FALSE)
		exit (1);

	/* load our configuration defaults */
	cfg_load ();

	/* update the menu data */
	refresh_list ();

	/* make the UI */
	create_main_window();

	/* Register SIGHUP */
        sa_new.sa_handler = catch_signal;
        sigemptyset (&sa_new.sa_mask);
        sa_new.sa_flags = 0;
        sigaction (SIGHUP,&sa_new,&sa_old);

	/* So we don't keep 'defunct' processes in the process table */
	signal (SIGCHLD, SIG_IGN);

	last_update = time (NULL);

	gpe_appmgr_start_xsettings ();

	/* start the event loop */
	gtk_main();

	clean_up();

	gtk_exit (0);

	return 0;
}

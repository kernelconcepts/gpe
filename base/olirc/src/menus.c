/*
 * ol-irc - A small irc client using GTK+
 *
 * Copyright (C) 1998, 1999 Yann Grossel [Olrick]
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

/* Management of menus : the menu-bar, and all the popups menus */

#include "olirc.h"
#include "prefs.h"
#include "menus.h"
#include "windows.h"
#include "servers_dialogs.h"
#include "channels_dialogs.h"
#include "dialogs.h"
#include "servers.h"
#include "channels.h"
#include "misc.h"
#include "toolbar.h"
#include "network.h"
#include "queries.h"
#include "ctcp.h"
#include "dcc.h"
#include "windows_dialogs.h"
#include "ignores.h"
#include "numerics.h"
#include "prefs_dialogs.h"

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <string.h>

/* Actions generated by menus */

enum { A_CANCEL, A_CONNECT, A_QUIT, A_PART, A_CLOSE, A_DISCONNECT, A_JOIN,
	A_CLEAR, A_SERVER, A_TOOLBAR, A_VW_OP, A_WINDOWSLIST, A_MESSAGES, A_LOG,
	A_SERVER_LIST, A_SOCKS_PREFS, A_RAW_WINDOW, A_MENUS, A_QUIT_ALL, A_QUIT_OLIRC,
	A_WHOIS, A_QUERY, A_DCC_CHAT, A_CTCP_PING, A_CTCP_VERSION, A_CTCP_USERINFO,
	A_CTCP_CLIENTINFO, A_CTCP_FINGER, A_OP, A_DEOP, A_VOICE, A_DEVOICE, A_KICK,
	A_BAN, A_BANKICK, A_DCC_CONNECTIONS, A_CTCP_TIME, A_IGNORE_LIST, A_IGNORE_USER,
	A_CHANGE_NICK, A_CTCP_SOURCE, A_CONNECT_ALL, A_DCC_SEND, A_SERVER_PROPS,
	A_CHAN_MODES, A_PREFS };

/* This struct describes a single menu item */

struct Menu
{
	gchar *Label;
	gint Action;
	GtkWidget **widget;
};

/* Important menu items */

GtkWidget *server_submenu, *channel_submenu, *query_submenu;
GtkWidget *item_window_log, *item_window_op;
GtkWidget *item_server_connect, *item_server_disconnect, *item_server_quit;
GtkWidget *item_channel_part, *item_channel_join;
GtkWidget *item_show_menus, *item_show_toolbar, *item_show_tabs;
GtkWidget *item_window_title, *item_member_title;
GtkWidget *item_change_nick, *item_window_ignore, *item_member_ignore;
GtkWidget *item_channel_modes;

/* The default menus */

static struct Menu mbar[] =
{
	{ "Ol-irc/New server", A_SERVER },
	{ "Ol-irc/Connect all", A_CONNECT_ALL },
	{ "Ol-irc/Quit all", A_QUIT_ALL },
	{ "Ol-irc/<separator>", A_CANCEL },
	{ "Ol-irc/Quit Olirc", A_QUIT_OLIRC },
	{ "Edit/Copy", A_CANCEL },
	{ "Edit/Cut", A_CANCEL },
	{ "Edit/Paste", A_CANCEL },
	{ "DCC/Connections", A_CANCEL },
	{ "Preferences/Settings", A_PREFS },
	{ "Preferences/Favorite servers", A_SERVER_LIST },
};

static struct Menu window_popup[] =
{
	{ "Title", A_CANCEL, &item_window_title },
	{ "<separator>", A_CANCEL },
	{ "New server", A_SERVER },
	{ "<separator>", A_CANCEL },
	{ "Window/Create a new window", A_VW_OP, &item_window_op },
	{ "Window/Close", A_CLOSE },
	{ "Window/Clear", A_CLEAR },
	{ "Window/<separator>", A_CANCEL },
	{ "Window/Show toolbar", A_TOOLBAR, &item_show_toolbar },
	{ "Window/Show tabs", A_WINDOWSLIST, &item_show_tabs },
	{ "Window/Show menus", A_MENUS, &item_show_menus },
	{ "Server/Connect", A_CONNECT, &item_server_connect },
	{ "Server/Quit", A_QUIT, &item_server_quit },
	{ "Server/Disconnect", A_DISCONNECT, &item_server_disconnect },
	{ "Server/Change...", A_CANCEL },
	{ "Server/<separator>", A_CANCEL },
	{ "Server/Properties", A_SERVER_PROPS },
	{ "Server/Ignore list", A_IGNORE_LIST },
	{ "Server/Change nick", A_CHANGE_NICK, &item_change_nick },
	{ "Server/<separator>", A_CANCEL },
	{ "Server/Raw window", A_RAW_WINDOW },
	{ "Channel/Join", A_JOIN, &item_channel_join },
	{ "Channel/Part", A_PART, &item_channel_part },
	{ "Channel/<separator>", A_CANCEL },
	{ "Channel/Modes", A_CANCEL, &item_channel_modes },
	{ "Channel/<separator>", A_CANCEL },
	{ "Channel/Server messages", A_MESSAGES },
	{ "User/Whois", A_WHOIS },
	{ "User/Ignore", A_IGNORE_USER, &item_window_ignore },
	{ "User/CTCP/Ping", A_CTCP_PING },
	{ "User/CTCP/Version", A_CTCP_VERSION },
	{ "User/CTCP/Finger", A_CTCP_FINGER },
	{ "User/CTCP/Time", A_CTCP_TIME },
	{ "User/CTCP/Userinfo", A_CTCP_USERINFO },
	{ "User/CTCP/Clientinfo", A_CTCP_CLIENTINFO },
	{ "User/CTCP/Source", A_CTCP_SOURCE },
	{ "User/DCC/Send", A_DCC_SEND },
	{ "User/DCC/Chat", A_DCC_CHAT },
	{ "Prefs", A_CANCEL },
	{ "<separator>", A_CANCEL },
	{ "Start log", A_LOG, &item_window_log }
};

static struct Menu channel_member_popup[] =
{
	{ "Title", A_CANCEL, &item_member_title },
	{ "<separator>", A_CANCEL },
	{ "Whois", A_WHOIS },
	{ "Query", A_QUERY },
	{ "Ignore", A_IGNORE_USER, &item_member_ignore },
	{ "Control/Op", A_OP },
	{ "Control/Deop", A_DEOP },
	{ "Control/Voice", A_VOICE },
	{ "Control/Devoice", A_DEVOICE },
	{ "Control/Kick", A_KICK },
	{ "Control/Ban", A_BAN },
	{ "Control/Ban + Kick", A_BANKICK },
	{ "CTCP/Ping", A_CTCP_PING },
	{ "CTCP/Version", A_CTCP_VERSION },
	{ "CTCP/Finger", A_CTCP_FINGER },
	{ "CTCP/Time", A_CTCP_TIME },
	{ "CTCP/Userinfo", A_CTCP_USERINFO },
	{ "CTCP/Clientinfo", A_CTCP_CLIENTINFO },
	{ "CTCP/Source", A_CTCP_SOURCE },
	{ "DCC/Send", A_DCC_SEND },
	{ "DCC/Chat", A_DCC_CHAT }
};

gchar menu_tmp[2048];

Channel *Target_Channel = NULL;
Server *Target_Server = NULL;
Member *Target_Member = NULL;
gboolean member_unignore = FALSE;
Virtual_Window *Target_VW = NULL;

GtkWidget *wpopup;
GtkWidget *upopup;

GtkAccelGroup *waccel, *uaccel;

/* ----------------------------------------------------------------------------------- */

struct menu_ban_struct
{
	gchar *channel;
	gboolean kick;
};

gboolean Menu_ban_who_reply(struct Message *m, gpointer data)
{
	struct menu_ban_struct *mbs = (struct menu_ban_struct *) data;

	if (m)
	{
		sprintf(menu_tmp, "MODE %s -o+b %s *!%s@%s", mbs->channel, m->args[5], m->args[2], m->args[3]);
		Server_Output(m->server, menu_tmp, TRUE);

		if (mbs->kick)
		{
			sprintf(menu_tmp, "KICK %s %s :%s", mbs->channel, m->args[5], m->server->current_nick);
			Server_Output(m->server, menu_tmp, TRUE);
		}
	}

	g_free(mbs->channel);
	g_free(mbs);

	return FALSE;
}

void action_control(gint Action)
{
	gchar *mode = NULL;
	gboolean kick = FALSE, ban = FALSE;

	switch (Action)
	{
		case A_OP: mode = "+o"; break;
		case A_DEOP: mode = "-o"; break;
		case A_VOICE: mode = "+v"; break;
		case A_DEVOICE: mode = "-v"; break;
		case A_BANKICK: kick = TRUE;	/* Note : No break here */
		case A_BAN: ban = TRUE; break;
		case A_KICK: kick = TRUE; break;
	}

	if (ban)
	{
		if (Target_Member->userhost)
		{
			sprintf(menu_tmp, "MODE %s -o+b %s *!%s", Target_Channel->Name, Target_Member->nick, Target_Member->userhost);
			Server_Output(Target_Server, menu_tmp, TRUE);
		}
		else
		{
			struct menu_ban_struct *mbs = (struct menu_ban_struct *) g_malloc0(sizeof(struct menu_ban_struct));

			mbs->channel = g_strdup(Target_Channel->Name);
			mbs->kick = kick;

			if (Server_Callback_Add(Target_Server, RPL_WHOREPLY, Target_Member->nick, 0, Menu_ban_who_reply, (gpointer) mbs))
			{
				VW_output(Target_Server->vw_active, T_WARNING, "sF-", Target_Server, "Retrieving %s userhost mask...", Target_Member->nick);
				sprintf(menu_tmp, "WHO %s", Target_Member->nick);
				Server_Output(Target_Server, menu_tmp, FALSE);
			}
			else
			{
				g_free(mbs->channel);
				g_free(mbs);
			}
			return;
		}
	}

	if (kick)
	{
		sprintf(menu_tmp, "KICK %s %s :%s", Target_Channel->Name, Target_Member->nick, Target_Server->current_nick);
		Server_Output(Target_Server, menu_tmp, TRUE);
	}

	if (!ban && !kick)
	{
		sprintf(menu_tmp, "MODE %s %s %s", Target_Channel->Name, mode, Target_Member->nick);
		Server_Output(Target_Server, menu_tmp, TRUE);
	}
}

void Menu_Action(GtkWidget *w, gpointer Action)
{
	Virtual_Window *vw_tmp;
	GList *g = NULL;

	switch ((gint) Action)
	{
		case A_SERVER: dialog_server_properties(NULL, NULL, (Target_VW)? Target_VW->rw : NULL); return;
		case A_PREFS: dialog_prefs(); return;
		case A_SERVER_LIST: dialog_favorite_servers(); return;
		case A_SOCKS_PREFS: Dialog_Socks(); return;
/*		case A_DCC_CONNECTIONS: DCC_Display_Connections(); return; */
		case A_CONNECT_ALL: Servers_Connect_All(); return;
		case A_QUIT_OLIRC: olirc_quit(NULL, /* <PREFS> True or false here */ TRUE); return;
		case A_QUIT_ALL:
		
			if (Servers_List)
			{
				Server *s;
				GList *l = Servers_List;

				while (l)
				{
					s = (Server *) l->data; l = l->next;
					if (s->State != SERVER_IDLE && s->State != SERVER_DISCONNECTING) g = g_list_append(g, (gpointer) s);
				}
			}
	
			if (g) dialog_quit_servers(g, NULL, NULL, NULL, FALSE);
			return;
	}

	g_return_if_fail(Target_VW);

	vw_tmp = Target_VW;

	switch ((gint) Action)
	{
		case A_CLEAR: VW_Clear(vw_tmp); return;
		case A_TOOLBAR: Toolbar_Switch(vw_tmp->rw); return;
		case A_WINDOWSLIST: GW_Windows_List_Switch(vw_tmp->rw); return;
		case A_MESSAGES: if (vw_tmp->pmask.w_type == W_CHANNEL) Dialog_Channel_Events(Target_Channel); return;
		case A_CLOSE: if (vw_tmp->pmask.w_type == W_SERVER && Target_Server->State != SERVER_IDLE) { g = g_list_append(NULL, Target_Server); dialog_quit_servers(g, NULL, vw_tmp, NULL, FALSE); } else VW_Close(vw_tmp); return;
		case A_CONNECT: if (vw_tmp->pmask.w_type == W_SERVER) Server_Connect(Target_Server); return;
		case A_QUIT: if (vw_tmp->pmask.w_type == W_SERVER) { g = g_list_append(NULL, Target_Server); dialog_quit_servers(g, NULL, NULL, NULL, FALSE); } return;
		case A_SERVER_PROPS: dialog_server_properties(NULL, Target_Server, NULL); return;
		case A_CHANGE_NICK: dialog_server_nick(Target_Server, "Please enter a new nickname :", Target_Server->current_nick); return;
		case A_IGNORE_LIST: Ignore_List_New(Target_Server); return;
		case A_IGNORE_USER: if (member_unignore) dialog_unignore(Target_Server, Target_Member); else dialog_ignore_properties(Target_Server, NULL, Target_Member, NULL); return;
		case A_DISCONNECT: if (vw_tmp->pmask.w_type == W_SERVER) Server_Disconnect(Target_Server, TRUE); return;
		case A_VW_OP: if (!GW_List->next) VW_Move_To_GW(vw_tmp, NULL); else dialog_rw_select(vw_tmp); return;
		case A_PART: if (vw_tmp->pmask.w_type == W_CHANNEL) Channel_Part(Target_Channel, NULL); return;
		case A_JOIN: if (vw_tmp->pmask.w_type == W_CHANNEL) Channel_Join(Target_Channel, NULL); return;
		case A_LOG: VW_Log(vw_tmp); return;
		case A_RAW_WINDOW: if (vw_tmp->pmask.w_type == W_SERVER) Server_Display_Raw_Window(Target_Server); return;
		case A_MENUS: Menus_Show(vw_tmp->rw); return;
		case A_QUERY: Query_New(Target_Server, Target_Member->nick); return;
		case A_WHOIS: sprintf(menu_tmp,"WHOIS %s", Target_Member->nick); Server_Output(Target_Server, menu_tmp, TRUE); return;
#ifdef USE_DCC
		case A_DCC_SEND: DCC_Send_Filesel(Target_Server, Target_Member->nick); return;
		case A_DCC_CHAT: DCC_Chat_New(Target_Member->nick, Target_Server, 0, 0); return;
#endif
#ifdef USE_CTCP
		case A_CTCP_PING: CTCP_Send(Target_Server, Target_Member->nick, FALSE, "PING", Create_CTCP_PING_Data(), TRUE); return;
		case A_CTCP_FINGER: CTCP_Send(Target_Server, Target_Member->nick, FALSE, "FINGER", NULL, TRUE); return;
		case A_CTCP_USERINFO: CTCP_Send(Target_Server, Target_Member->nick, FALSE, "USERINFO", NULL, TRUE); return;
		case A_CTCP_CLIENTINFO: CTCP_Send(Target_Server, Target_Member->nick, FALSE, "CLIENTINFO", NULL, TRUE); return;
		case A_CTCP_SOURCE: CTCP_Send(Target_Server, Target_Member->nick, FALSE, "SOURCE", NULL, TRUE); return;
		case A_CTCP_VERSION: CTCP_Send(Target_Server, Target_Member->nick, FALSE, "VERSION", NULL, TRUE); return;
		case A_CTCP_TIME: CTCP_Send(Target_Server, Target_Member->nick, FALSE, "TIME", NULL, TRUE); return;
#endif
		case A_OP: case A_DEOP: case A_VOICE: case A_DEVOICE:
		case A_KICK: case A_BAN: case A_BANKICK: action_control((gint) Action); return;
		case A_CHAN_MODES: return;
		default: g_warning("Unknown Action in Menu_Action(): %d\n", (gint) Action);
	}
}

/* ----- Creation of menus ----------------------------------------------------------- */

gchar *get_first_part(gchar *path)
{
	gchar *r;
	g_return_val_if_fail(path, NULL);
	if (!strchr(path, '/')) return NULL;
	r = g_strdup(path);
	*strchr(r, '/') = 0;
	return r;
}

GtkWidget *Create_Item(gchar *path, gint Action, GtkWidget **widget, GtkAccelGroup *accel)
{
	GtkWidget *item;
	guint accelkey;
	gchar *p;

	g_return_val_if_fail(path, NULL);

	p = strrchr(path, '/');

	if (p) p++; else p = path;

	if (strcmp(p, "<separator>")) item = gtk_menu_item_new_with_label(p);
	else item = gtk_menu_item_new();

	if (Action == A_CANCEL) gtk_widget_set_sensitive(item, FALSE);
	else gtk_signal_connect(GTK_OBJECT(item), "activate", GTK_SIGNAL_FUNC(Menu_Action), (gpointer) Action);

	gtk_widget_show(item);

	switch (Action)
	{
		case A_CONNECT_ALL: accelkey = 'A'; break;
		case A_CLEAR: accelkey = 'C'; break;
		case A_TOOLBAR: accelkey = 'T'; break;
		case A_WINDOWSLIST: accelkey = 'W'; break;
		case A_MESSAGES: accelkey = 'D'; break;
		case A_VW_OP: accelkey = 'N'; break;
		case A_SERVER: accelkey = 'S'; break;
		case A_MENUS: accelkey = 'M'; break;
		case A_QUIT_OLIRC: accelkey = 'Q'; break;
		case A_CHAN_MODES: accelkey = 'E'; break;
		default: accelkey = 0;
	}

	if (accel && accelkey) gtk_widget_add_accelerator(item, "activate", accel, accelkey, GDK_MOD1_MASK, GTK_ACCEL_VISIBLE | GTK_ACCEL_LOCKED);
	gtk_widget_lock_accelerators(item);

	if (widget) *widget = item;

	return item;
}

GtkWidget *Create_Menu(struct Menu *m, gint nitems, gchar *parent, GtkAccelGroup *accel)
{
	gint l, p, i = 0;
	gchar *r;
	GtkWidget *menu, *item;

	p = (parent)? strlen(parent) : 0;

	menu = gtk_menu_new();

	if (accel) gtk_menu_set_accel_group(GTK_MENU(menu), accel);

	while (i<nitems)
	{
		if (parent && (strncmp(m[i].Label, parent, p) || m[i].Label[p]!='/')) return menu;

		r = m[i].Label + p; if (*r == '/') r++;

		if (strchr(r, '/'))
		{
			GtkWidget *submenu;
			gchar *s = get_first_part(r);
			gchar *t;

			/* FIXME Create_Item should generate ALL of the items */

			item = gtk_menu_item_new_with_label(s);
			gtk_menu_append(GTK_MENU(menu), item);
			gtk_widget_show(item);

			/* FIXME Several items are labelled "Server" or "Channel"...
			         Moreover this prevents any translation of olirc */

			if (!strcmp(s, "Server")) server_submenu = item;
			else if (!strcmp(s, "Channel")) channel_submenu = item;
			else if (!strcmp(s, "User")) query_submenu = item;

			if (p)
			{
				t = g_malloc(p + strlen(s) + 2);
				sprintf(t, "%s/%s", parent, s);
				g_free(s);
			}
			else t = s;

			submenu = Create_Menu(&m[i], nitems-i, t, accel);
			gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), submenu);

			l = strlen(t);
			while (i<nitems && !strncmp(m[i].Label, t, l) && m[i].Label[l]=='/') i++;

			g_free(t);
		}
		else
		{
			gtk_menu_append(GTK_MENU(menu), Create_Item(r, m[i].Action, m[i].widget, accel));
			i++;
		}
	}

	return menu;
}

GtkWidget *Create_Menubar(struct Menu *m, gint nitems, GtkAccelGroup *accel)
{
	gint l, i = 0;
	gchar *title;
	GtkWidget *bar = gtk_menu_bar_new(), *item, *menu;

	while (i<nitems)
	{
		if (!(title = get_first_part(m[i].Label))) return NULL;

		menu = Create_Menu(&m[i], nitems-i, title, accel);
		item = gtk_menu_item_new_with_label(title);
		gtk_widget_show(item);
		gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), menu);
		gtk_menu_bar_append(GTK_MENU_BAR(bar), item);

		l = strlen(title);
		while (i<nitems && !strncmp(m[i].Label, title, l) && m[i].Label[l]=='/') i++;

		g_free(title);
	}

	return bar;
}

/* ----- Initialization of menus ------------------------------------------------------- */

void Menubar_Init(GUI_Window *rw)
{
	GtkAccelGroup *accel = gtk_accel_group_new();
	gtk_accel_group_attach(accel, GTK_OBJECT(rw->Gtk_Window));
	gtk_accel_group_attach(waccel, GTK_OBJECT(rw->Gtk_Window));
	gtk_accel_group_attach(uaccel, GTK_OBJECT(rw->Gtk_Window));

	rw->Menubar = Create_Menubar(mbar, sizeof(mbar)/sizeof(struct Menu), accel);

 	gtk_box_pack_start(GTK_BOX(rw->Main_Box), rw->Menubar, FALSE, FALSE, 2);
}

void Popup_Menus_Init()
{
	waccel = gtk_accel_group_new();
	uaccel = gtk_accel_group_new();

	wpopup = Create_Menu(window_popup, sizeof(window_popup)/sizeof(struct Menu), NULL, waccel);
	upopup = Create_Menu(channel_member_popup, sizeof(channel_member_popup)/sizeof(struct Menu), NULL, uaccel);
}

/* ------------------------------------------------------------------------------------- */

void Popupmenu_Window_Display(Virtual_Window *vw, GdkEventButton *v)
{
	g_return_if_fail(vw);

	Target_VW = vw;

	gtk_widget_hide(server_submenu);
	gtk_widget_hide(channel_submenu);
	gtk_widget_hide(query_submenu);

	switch (vw->pmask.w_type)
	{
		case W_SERVER:
		{
			Target_Server = (Server *) vw->Resource;
			gtk_widget_set_sensitive(item_server_connect, Target_Server->State==SERVER_IDLE);
			gtk_widget_set_sensitive(item_server_disconnect, Target_Server->State!=SERVER_IDLE);
			gtk_widget_set_sensitive(item_server_quit, Target_Server->State!=SERVER_IDLE);
			gtk_widget_set_sensitive(item_change_nick, Target_Server->State!=SERVER_IDLE);
			gtk_widget_show(server_submenu);
			break;
		}
		case W_CHANNEL:
		{
			Target_Channel = (Channel *) vw->Resource;
			Target_Server = Target_Channel->server;
			gtk_widget_set_sensitive(item_channel_join, Target_Channel->State!=CHANNEL_JOINED);
			gtk_widget_set_sensitive(item_channel_part, Target_Channel->State==CHANNEL_JOINED);
			/* gtk_widget_set_sensitive(item_channel_modes, Target_Channel->State==CHANNEL_JOINED); */
			gtk_widget_show(channel_submenu);
			break;
		}
		case W_QUERY:
		{
			Query *q = (Query *) vw->Resource;
			Target_Member = &(q->member);
			Target_Server = q->server;
			gtk_widget_show(query_submenu);
			break;
		}
	}

	if (!GW_List->next && !vw->rw->vw_list->next)
	{
		gtk_label_set((GtkLabel *) ((GtkMenuItem *) item_window_op)->item.bin.child, "Create a new window");
		gtk_widget_set_sensitive(item_window_op, FALSE);
	}
	else
	{
		gtk_widget_set_sensitive(item_window_op, TRUE);

		if (!GW_List->next)
			gtk_label_set((GtkLabel *) ((GtkMenuItem *) item_window_op)->item.bin.child, "Create a new window");
		else if (!vw->rw->vw_list->next)
			gtk_label_set((GtkLabel *) ((GtkMenuItem *) item_window_op)->item.bin.child, "Move into another window");
		else
			gtk_label_set((GtkLabel *) ((GtkMenuItem *) item_window_op)->item.bin.child, "Move or create");
	}

	gtk_label_set((GtkLabel *) ((GtkMenuItem *) item_window_title)->item.bin.child, vw->Name);
	gtk_label_set((GtkLabel *) ((GtkMenuItem *) item_show_menus)->item.bin.child, vw->rw->Menus_visible ? "Hide menus" : "Show menus");
	gtk_label_set((GtkLabel *) ((GtkMenuItem *) item_show_toolbar)->item.bin.child, vw->rw->Toolbar_visible ? "Hide toolbar" : "Show toolbar");
	gtk_label_set((GtkLabel *) ((GtkMenuItem *) item_show_tabs)->item.bin.child, vw->rw->Tabs_visible ? "Hide tabs" : "Show tabs");
	gtk_label_set((GtkLabel *) ((GtkMenuItem *) item_window_log)->item.bin.child, vw->fd_log==-1 ? "Start log" : "Stop log");

	gtk_menu_popup(GTK_MENU(wpopup), NULL, NULL, NULL, NULL, 0, 0);
}

void Popupmenu_Member_Display(Channel *c, Member *m, GdkEventButton *v)
{
	Target_VW = c->vw;
	Target_Channel = c;
	Target_Server = c->server;
	Target_Member = m;
	gtk_label_set((GtkLabel *) ((GtkMenuItem *) item_member_title)->item.bin.child, Target_Member->nick);

	gtk_label_set((GtkLabel *) ((GtkMenuItem *) item_member_ignore)->item.bin.child, "Ignore");
	gtk_widget_set_sensitive(item_member_ignore, TRUE);
	member_unignore = FALSE;

	if (Target_Member->userhost)
	{
		sprintf(menu_tmp, "%s!%s", Target_Member->nick, Target_Member->userhost);
	
		if (Ignore_Check(Target_Server, menu_tmp, -1))
		{
			gtk_label_set((GtkLabel *) ((GtkMenuItem *) item_member_ignore)->item.bin.child, "Unignore");
			member_unignore = TRUE;
			gtk_widget_set_sensitive(item_member_ignore, FALSE);
		 }
	}

	gtk_menu_popup(GTK_MENU(upopup), NULL, NULL, NULL, NULL, 0, 0);
}

/* ------------------------------------------------------------------------------------ */

void Menus_Show(GUI_Window *rw)
{
	g_return_if_fail(rw);

	if (rw->Menus_visible) gtk_widget_hide(rw->Menubar);
	else gtk_widget_show(rw->Menubar);
	rw->Menus_visible = ! rw->Menus_visible;
}

/* vi: set ts=3: */


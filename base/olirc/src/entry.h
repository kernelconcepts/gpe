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

#ifndef __OLIRC_ENTRY_H__
#define __OLIRC_ENTRY_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

void init_entry_parser_commands();
void parser_add_command(gchar *, gint , gint, gpointer, gint, gint, gchar *);
void parser_remove_command(gchar *, gint);

void Entry_Tab(Virtual_Window *, guint, gboolean);
void Parse_Entry(GtkWidget *, gpointer);
void Do_Command(Virtual_Window *, gchar *, gchar *);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __OLIRC_ENTRY_H_ */

/* vi: set ts=3: */


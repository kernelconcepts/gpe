/* 
 * MultiSync GPE Plugin
 * Copyright (C) 2004 Phil Blundell <pb@nexus.co.uk>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 */

#include <stdlib.h>
#include <errno.h>
#include <glib.h>
#include <libintl.h>
#include <stdio.h>
#include <string.h>

#include "gpe_sync.h"

GList *
sync_todo (GList *data, gpe_conn *conn, int newdb)
{
  return data;
}

gboolean
push_todo (gpe_conn *conn, const char *obj, const char *uid, 
	   char *returnuid, int *returnuidlen)
{
  return FALSE;
}

gboolean
delete_todo (gpe_conn *conn, const char *uid, gboolean soft)
{
  return FALSE;
}

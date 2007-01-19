/*
 * gpe-conf
 *
 * Copyright (C) 2006, 2007  Florian Boor <florian.boor@kernelconcepts.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 *
 * Device detection module.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <libintl.h>
#define _(x) gettext(x)
#include <gtk/gtk.h>

#include "device.h"

#define DEVICE_FILE PREFIX "/share/gpe-conf/deviceinfo"
typedef struct
{
	DeviceID_t id;
	gchar **pattern;
	gchar *name;
	gchar *model;
	gchar *manufacturer;
    gchar *type; /* was type */
	gint features;
} Device_t;

static gint local_device = -1;
static Device_t *DeviceMap = NULL;

/* Keep in sync with DeviceId_t defintion. */
static DeviceID_t IdClassVector[] = 
{
	DEVICE_CLASS_NONE,
	DEVICE_CLASS_PDA,
	DEVICE_CLASS_PDA,
	DEVICE_CLASS_TABLET,
	DEVICE_CLASS_MDE,
	DEVICE_CLASS_PDA,
	DEVICE_CLASS_PDA,
	DEVICE_CLASS_PDA,
	DEVICE_CLASS_PDA,
	DEVICE_CLASS_PDA,
	DEVICE_CLASS_PDA,
	DEVICE_CLASS_PDA,
	DEVICE_CLASS_TABLET,
	DEVICE_CLASS_MDE,
	DEVICE_CLASS_MDE,
	DEVICE_CLASS_CELLPHONE,
	DEVICE_CLASS_TABLET,
	DEVICE_CLASS_PDA,
	DEVICE_CLASS_PC,
	DEVICE_CLASS_PC,
	DEVICE_CLASS_PC,
	DEVICE_CLASS_PC,
	DEVICE_CLASS_PC,
	DEVICE_CLASS_PC,
	DEVICE_CLASS_PC,
	DEVICE_CLASS_PC,
	DEVICE_CLASS_PDA | DEVICE_CLASS_CELLPHONE,
	DEVICE_CLASS_TABLET,
};

static gboolean
devices_load(void)
{
	GKeyFile *devicefile;
	GError *err = NULL;
	gchar **devices;
	guint i, devcnt;
	
	devicefile = g_key_file_new();
	
	if (!g_key_file_load_from_file (devicefile, DEVICE_FILE,
                                    G_KEY_FILE_NONE, &err))
	{
		g_printerr ("%s\n", err->message);
		g_error_free(err);
		return FALSE;
	}
	
	devices = g_key_file_get_keys (devicefile, "Devices",  &devcnt, &err);
	if (devices)
	{
		DeviceMap = g_malloc0 (sizeof(Device_t) * (devcnt + 1));

		for (i=0; i < devcnt; i++)
		{
			Device_t dev = DeviceMap[i];
			
			dev.pattern = g_key_file_get_string_list (devicefile, "Pattern",
                                                      devices[i], NULL, NULL);
			dev.id = g_key_file_get_integer (devicefile, "ID",
                                                      devices[i], NULL);
			dev.name = g_key_file_get_string (devicefile, "Name",
                                                      devices[i], NULL);
			dev.model = g_key_file_get_string (devicefile, "Model",
                                                      devices[i], NULL);
			dev.type = g_key_file_get_string (devicefile, "Type",
                                                      devices[i], NULL);
			dev.manufacturer = g_key_file_get_string (devicefile, "Manufacturer",
                                                      devices[i], NULL);
			dev.features = g_key_file_get_integer (devicefile, "Features",
                                                      devices[i], NULL);
		}
		g_strfreev(devices);
	}
	return TRUE;
}


DeviceID_t 
device_get_id (void)
{
	static gint id = -1;
	gchar **strv;
	guint len = 0;
	gint i = 0;
	gint dnr, pnr;
	gchar *iptr, *str = NULL;
	
	if (id != -1)
		return id;
	
	if (DeviceMap == NULL)
		devices_load ();
	
	/* get device info from /proc/cpuinfo, only ARM and MIPS for now */
	if (g_file_get_contents("/proc/cpuinfo", &str, &len, NULL))
	{
		strv = g_strsplit(str, "\n", 128);
		g_free(str);
		while (strv[i])
		{
			if ((iptr = strstr(strv[i], "Hardware")) 
			     || (iptr = strstr(strv[i], "system type")))
			{
				for (dnr = 0; dnr < sizeof(DeviceMap)/sizeof(*DeviceMap); dnr++)
				{
					pnr = 0;
					while ((DeviceMap[dnr].pattern) && (DeviceMap[dnr].pattern[pnr]))
					{
						if (strstr (iptr, DeviceMap[dnr].pattern[pnr]))
						{
							id = DeviceMap[dnr].id;
							local_device = dnr;
							if (DeviceMap[dnr].name == NULL)
								DeviceMap[dnr].name = g_strdup(DeviceMap[dnr].pattern[pnr]);
							return id;
						}
						else
							pnr++;
					}
				}
			}
			else 
				if (strstr (strv[i], "fdiv_bug"))
				{
					id = DEV_X86;
       		         device_name = "Generic x86 PC";
       	    	     device_type = "workstation";
					g_strfreev(strv);
					return id;
				}
				else 
				if (strstr (strv[i], "machine"))
				{
					id = DEV_POWERPC;
            	    device_name = "PowerPC";
            	   	 device_type = "workstation";
				g_strfreev(strv);
				return id;
			}
			else
			if (strstr (strv[i], "promlib"))
			{
				id = DEV_SPARC;
                device_name = "Sparc";
                device_type = "workstation";
				g_strfreev(strv);
				return id;
			}
			else
			if (strstr (strv[i], ": Alpha"))
			{
				id = DEV_ALPHA;
                device_name = "Alpha";
                device_type = "workstation";
				g_strfreev(strv);
				return id;
			}
			
			i++;
		}
		g_strfreev(strv);
	}
	
  //  device_name = "Unknown Device";
  //  device_type = "unknown";
	id = DEV_UNKNOWN;
	local_device = 0;

	return id;
}

DeviceFeatureID_t
device_get_features (DeviceID_t device_id)
{
	if ((device_id >= 0) && (device_id < DEV_MAX))
		return 
			IdClassVector [device_id];
	else
		return
			DEVICE_CLASS_NONE;
}

const gchar *
device_get_name (void)
{
    if (local_device == -1)
        device_get_id ();
    
    return DeviceMap[local_device].name;
}

const gchar *
device_get_type (void)
{
    if (local_device == -1)
        device_get_id ();
    
    return DeviceMap[local_device].type;
}

gchar *
device_get_specific_file (const gchar *basename)
{
    gchar *result;
    const gchar *name = device_get_name ();
    
    /* check for a device specific file per name first */
    if (name != NULL)
    {
        result = g_strdup_printf ("%s.%s", basename, name);
        if (!access (result, R_OK))
            return result;
        g_free (result);
    }
    
    /* try device number then */
    result = g_strdup_printf ("%s.%d", basename, device_get_id());
    if (!access (result, R_OK))
        return result;
    g_free (result);
    
    /* try device type then */
    
    result = g_strdup_printf ("%s.%s", basename, device_get_type());
    if (!access (result, R_OK))
        return result;
    g_free (result);
    
    /* maybe add class here later */
    
    return NULL;
}

/* Time-stamp: <2006-04-05 22:53:35 jcs>
|
|  Copyright (C) 2002-2005 Jorg Schuler <jcsjcs at users sourceforge net>
|  Part of the gtkpod project.
| 
|  URL: http://www.gtkpod.org/
|  URL: http://gtkpod.sourceforge.net/
|
|  Part of this code is based on ipod-device.c from the libipoddevice
|  project (http://64.14.94.162/index.php/Libipoddevice).
|
|  The code contained in this file is free software; you can redistribute
|  it and/or modify it under the terms of the GNU Lesser General Public
|  License as published by the Free Software Foundation; either version
|  2.1 of the License, or (at your option) any later version.
|
|  This file is distributed in the hope that it will be useful,
|  but WITHOUT ANY WARRANTY; without even the implied warranty of
|  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
|  Lesser General Public License for more details.
|
|  You should have received a copy of the GNU Lesser General Public
|  License along with this code; if not, write to the Free Software
|  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
|  USA
|
|  iTunes and iPod are trademarks of Apple
|
|  This product is not supported/written/published by Apple!
|
|  $Id$
*/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "itdb_device.h"

#define GB 1024

static const Itdb_IpodModel ipod_model_table [] = {
	/* Handle idiots who hose their iPod file system, or 
	   lucky people with iPods we don't yet know about*/
	{"Invalid", 0,    MODEL_TYPE_INVALID,    UNKNOWN_GENERATION},
	{"Unknown", 0,    MODEL_TYPE_UNKNOWN,    UNKNOWN_GENERATION},
	
	/* First Generation */
        /* Mechanical buttons arranged around rotating "scroll wheel".
	   8513, 8541 and 8709 are Mac types, 8697 is PC */
	{"8513", 5  * GB, MODEL_TYPE_REGULAR,    FIRST_GENERATION},
	{"8541", 5  * GB, MODEL_TYPE_REGULAR,    FIRST_GENERATION},
	{"8697", 5  * GB, MODEL_TYPE_REGULAR,    FIRST_GENERATION},
	{"8709", 10 * GB, MODEL_TYPE_REGULAR,    FIRST_GENERATION},
	
	/* Second Generation */
	/* Same buttons as First Generation but around touch-sensitive
	 * "touch wheel". 8737 and 8738 are Mac types, 8740 and 8741
	   are PC */
	{"8737", 10 * GB, MODEL_TYPE_REGULAR,    SECOND_GENERATION},
	{"8740", 10 * GB, MODEL_TYPE_REGULAR,    SECOND_GENERATION},
	{"8738", 20 * GB, MODEL_TYPE_REGULAR,    SECOND_GENERATION},
	{"8741", 20 * GB, MODEL_TYPE_REGULAR,    SECOND_GENERATION},
	
	/* Third Generation */
	/* Touch sensitive buttons and arranged in a line above "touch
	   wheel". Docking connector was introduced here, same models
	   for Mac and PC from now on. */
	{"8976", 10 * GB, MODEL_TYPE_REGULAR,    THIRD_GENERATION},
	{"8946", 15 * GB, MODEL_TYPE_REGULAR,    THIRD_GENERATION},
	{"9460", 15 * GB, MODEL_TYPE_REGULAR,    THIRD_GENERATION},
	{"9244", 20 * GB, MODEL_TYPE_REGULAR,    THIRD_GENERATION},
	{"8948", 30 * GB, MODEL_TYPE_REGULAR,    THIRD_GENERATION},
	{"9245", 40 * GB, MODEL_TYPE_REGULAR,    THIRD_GENERATION},
	
	/* Fourth Generation */
	/* Buttons are now integrated into the "touch wheel". */
	{"9282", 20 * GB, MODEL_TYPE_REGULAR,    FOURTH_GENERATION},
	{"9787", 25 * GB, MODEL_TYPE_REGULAR_U2, FOURTH_GENERATION},
	{"9268", 40 * GB, MODEL_TYPE_REGULAR,    FOURTH_GENERATION},
	{"A079", 20 * GB, MODEL_TYPE_COLOR,      FOURTH_GENERATION},
	{"A127", 20 * GB, MODEL_TYPE_COLOR_U2,   FOURTH_GENERATION},
	{"9830", 60 * GB, MODEL_TYPE_COLOR,      FOURTH_GENERATION},
	
	/* First Generation Mini */
	{"9160", 4 * GB, MODEL_TYPE_MINI,        FIRST_GENERATION},
	{"9436", 4 * GB, MODEL_TYPE_MINI_BLUE,   FIRST_GENERATION},
	{"9435", 4 * GB, MODEL_TYPE_MINI_PINK,   FIRST_GENERATION},
	{"9434", 4 * GB, MODEL_TYPE_MINI_GREEN,  FIRST_GENERATION},
	{"9437", 4 * GB, MODEL_TYPE_MINI_GOLD,   FIRST_GENERATION},	

	/* Second Generation Mini */
	{"9800", 4 * GB, MODEL_TYPE_MINI,        SECOND_GENERATION},
	{"9802", 4 * GB, MODEL_TYPE_MINI_BLUE,   SECOND_GENERATION},
	{"9804", 4 * GB, MODEL_TYPE_MINI_PINK,   SECOND_GENERATION},
	{"9806", 4 * GB, MODEL_TYPE_MINI_GREEN,  SECOND_GENERATION},
	{"9801", 6 * GB, MODEL_TYPE_MINI,        SECOND_GENERATION},
	{"9803", 6 * GB, MODEL_TYPE_MINI_BLUE,   SECOND_GENERATION},
	{"9805", 6 * GB, MODEL_TYPE_MINI_PINK,   SECOND_GENERATION},
	{"9807", 6 * GB, MODEL_TYPE_MINI_GREEN,  SECOND_GENERATION},	

	/* Photo / Fourth Generation */
	/* Buttons are integrated into the "touch wheel". */
	{"9829", 30 * GB, MODEL_TYPE_COLOR,      FOURTH_GENERATION},
	{"9585", 40 * GB, MODEL_TYPE_COLOR,      FOURTH_GENERATION},
	{"9586", 60 * GB, MODEL_TYPE_COLOR,      FOURTH_GENERATION},
	{"9830", 60 * GB, MODEL_TYPE_COLOR,      FOURTH_GENERATION},
	
	/* Shuffle / Fourth Generation */
	{"9724", GB / 2, MODEL_TYPE_SHUFFLE,     FOURTH_GENERATION},
	{"9725", GB,     MODEL_TYPE_SHUFFLE,     FOURTH_GENERATION},
	
	/* Nano / Fifth Generation */
	/* Buttons are integrated into the "touch wheel". */
	{"A350", GB * 1, MODEL_TYPE_NANO_WHITE,  FIFTH_GENERATION},
	{"A352", GB * 1, MODEL_TYPE_NANO_BLACK,  FIFTH_GENERATION},
	{"A004", GB * 2, MODEL_TYPE_NANO_WHITE,  FIFTH_GENERATION},
	{"A099", GB * 2, MODEL_TYPE_NANO_BLACK,  FIFTH_GENERATION},
	{"A005", GB * 4, MODEL_TYPE_NANO_WHITE,  FIFTH_GENERATION},
	{"A107", GB * 4, MODEL_TYPE_NANO_BLACK,  FIFTH_GENERATION},

	/* Video / Fifth Generation */
	/* Buttons are integrated into the "touch wheel". */
	{"A002", GB * 30, MODEL_TYPE_VIDEO_WHITE, FIFTH_GENERATION},
	{"A146", GB * 30, MODEL_TYPE_VIDEO_BLACK, FIFTH_GENERATION},
	{"A003", GB * 60, MODEL_TYPE_VIDEO_WHITE, FIFTH_GENERATION},
	{"A147", GB * 60, MODEL_TYPE_VIDEO_BLACK, FIFTH_GENERATION},
	
	/* HP iPods, need contributions for this table */
	/* Buttons are integrated into the "touch wheel". */
	{"E436", 40 * GB, MODEL_TYPE_REGULAR,     FOURTH_GENERATION},
	{"S492", 30 * GB, MODEL_TYPE_COLOR,       FOURTH_GENERATION},

	/* No known model number -- create a Device/SysInfo file with
	 * one entry, e.g.:
	 ModelNumStr: Mmobile1
	*/
	{"mobile1", -1, MODEL_TYPE_MOBILE_1, MOBILE_GENERATION},
	
	{NULL, 0, 0, 0}
};

#if 0
static const gchar *ipod_model_name_table [] = {
	"Invalid",
	"Unknown",
	"Color",
	"Color U2",
	"Grayscale",
	"Grayscale U2",
	"Mini (Silver)",
	"Mini (Blue)",
	"Mini (Pink)",
	"Mini (Green)",
	"Mini (Gold)",
	"Shuffle",
	"Nano (White)",
	"Nano (Black)",
	"Video (White)",
	"Video (Black)",
	"Mobile (1)",
	NULL
};
#endif

static const Itdb_ArtworkFormat ipod_color_artwork_info[] = {
	{IPOD_COVER_SMALL,        56,  56, 1017},
	{IPOD_COVER_LARGE,       140, 140, 1016},
	{IPOD_PHOTO_SMALL,        42,  30, 1009},
	{IPOD_PHOTO_LARGE,       130,  88, 1015},
	{IPOD_PHOTO_FULL_SCREEN, 220, 176, 1013},
	{IPOD_PHOTO_TV_SCREEN,   720, 480, 1019},
	{-1,                      -1,  -1,   -1}
};

static const Itdb_ArtworkFormat ipod_nano_artwork_info[] = {
	{IPOD_COVER_SMALL,        42,  42, 1031},
	{IPOD_COVER_LARGE,       100, 100, 1027},
	{IPOD_PHOTO_LARGE,        42,  37, 1032},
	{IPOD_PHOTO_FULL_SCREEN, 176, 132, 1023},
	{-1,                      -1,  -1,   -1}
};

static const Itdb_ArtworkFormat ipod_video_artwork_info[] = {
	{IPOD_COVER_SMALL,       100, 100, 1028},
	{IPOD_COVER_LARGE,       200, 200, 1029},
	{IPOD_PHOTO_SMALL,        50,  41, 1036},
	{IPOD_PHOTO_LARGE,       130,  88, 1015},
	{IPOD_PHOTO_FULL_SCREEN, 320, 240, 1024},
	{IPOD_PHOTO_TV_SCREEN,   720, 480, 1019},
	{-1,                      -1,  -1,   -1}
};

static const Itdb_ArtworkFormat ipod_mobile_1_artwork_info[] = {
	{IPOD_COVER_SMALL,        50,  50, 2002},
	{IPOD_COVER_LARGE,       150, 150, 2003},
	{-1,                      -1,  -1,   -1}
};

/* This will be indexed using a value from the MODEL_TYPE enum */
static const Itdb_ArtworkFormat *ipod_artwork_info_table[] = {
        NULL,                    /* Invalid       */
	NULL,                    /* Unknown       */
	ipod_color_artwork_info, /* Color         */
	ipod_color_artwork_info, /* Color U2      */
	NULL,                    /* Grayscale     */ 
	NULL,                    /* Grayscale U2  */
	NULL,                    /* Mini (Silver) */
	NULL,                    /* Mini (Blue)   */
	NULL,                    /* Mini (Pink)   */
	NULL,                    /* Mini (Green)  */
	NULL,                    /* Mini (Gold)   */
	NULL,                    /* Shuffle       */
	ipod_nano_artwork_info,  /* Nano (White)  */
	ipod_nano_artwork_info,  /* Nano (Black)  */
	ipod_video_artwork_info, /* Video (White) */
	ipod_video_artwork_info, /* Video (Black) */
	ipod_mobile_1_artwork_info /* Mobile (1)    */
};


/* Reset or create the SysInfo hash table */
static void itdb_device_reset_sysinfo (Itdb_Device *device)
{
    if (device->sysinfo)
	g_hash_table_destroy (device->sysinfo);
    device->sysinfo = g_hash_table_new_full (g_str_hash, g_str_equal,
					     g_free, g_free);
}


/**
 * itdb_device_new:
 * 
 * Creates a new #Itdb_Device structure
 *
 * Return value: a newly allocated #Itdb_Device which must be freed with
 * itdb_device_free() when no longer needed
 **/
Itdb_Device *itdb_device_new ()
{
    Itdb_Device *dev;

    dev = g_new0 (Itdb_Device, 1);
    itdb_device_reset_sysinfo (dev);
    return dev;
}

/**
 * itdb_device_free:
 * @device: an #Itdb_Device
 *
 * Frees memory used by @device
 **/
void itdb_device_free (Itdb_Device *device)
{
    if (device)
    {
	g_free (device->mountpoint);
	if (device->sysinfo)
	    g_hash_table_destroy (device->sysinfo);
	g_free (device);
    }
}


/**
 * itdb_device_set_mountpoint:
 * @device: an #Itdb_Device
 * @mp: the new mount point
 *
 * Sets the mountpoint of @device to @mp and update the cached device 
 * information (in particular, re-read the SysInfo file)
 **/
void itdb_device_set_mountpoint (Itdb_Device *device, const gchar *mp)
{
    g_return_if_fail (device);

    g_free (device->mountpoint);
    device->mountpoint = g_strdup (mp);
    if (mp)
	itdb_device_read_sysinfo (device);
}


/** 
 * itdb_device_read_sysinfo:
 * @device: an #Itdb_Device
 *
 * Reads the SysInfo file and stores information in device->sysinfo for
 * later use.
 *
 * Return value: TRUE if file could be read, FALSE otherwise 
 **/
gboolean itdb_device_read_sysinfo (Itdb_Device *device)
{
    const gchar *p_sysinfo[] = {"SysInfo", NULL};
    gchar *dev_path, *sysinfo_path;
    FILE *fd;
    gboolean result = FALSE;
    gchar buf[1024];

    g_return_val_if_fail (device, FALSE);
    g_return_val_if_fail (device->mountpoint, FALSE);

    itdb_device_reset_sysinfo (device);

    g_return_val_if_fail (device->sysinfo, FALSE);

    dev_path = itdb_get_device_dir (device->mountpoint);

    if (!dev_path) return FALSE;

    sysinfo_path = itdb_resolve_path (dev_path, p_sysinfo);

    if (sysinfo_path)
    {
	fd = fopen (sysinfo_path, "r");
	if (fd)
	{
	    result = TRUE;
	    while(fgets(buf, sizeof(buf), fd))
	    {
		gchar *ptr;
		gint len = strlen (buf);
		/* suppress newline at end of line */
		if ((len>0) && (buf[len-1]==0x0a))
		{
		    buf[len-1] = 0;
		    --len;
		}
		ptr = strchr (buf, ':');
		if (ptr && (ptr!=buf))
		{
		    gchar *key, *value;
		    *ptr = 0;
		    ++ptr;
		    key = g_strdup (buf);
		    g_strstrip (ptr);
		    value = g_strdup (ptr);
		    g_hash_table_insert (device->sysinfo, key, value);
		}
	    }
	    fclose (fd);
	}
	g_free (sysinfo_path);
    }
    g_free (dev_path);
    return result;
}


/**
 * itdb_device_get_sysinfo:
 * @device: an #Itdb_Device
 * @field: field to retrive information from
 *
 * Retrieve specified field from the SysInfo file.
 *
 * Return value: the information associated with @field, or NULL if @field
 * couldn't be found. g_free() after use
 **/
gchar *itdb_device_get_sysinfo (Itdb_Device *device, const gchar *field)
{
    g_return_val_if_fail (device, NULL);
    g_return_val_if_fail (device->sysinfo, NULL);
    g_return_val_if_fail (field, NULL);

    return g_strdup (g_hash_table_lookup (device->sysinfo, field));
}


/* Return Itdb_IpodModel entry for this iPod */
G_GNUC_INTERNAL const Itdb_IpodModel *
itdb_device_get_ipod_model (Itdb_Device *device)
{
    gint i;
    gchar *model_num, *p;

    model_num = itdb_device_get_sysinfo (device, "ModelNumStr");

    if (!model_num)
	return &ipod_model_table[0];

    p = model_num;

    if(isalpha(model_num[0])) 
	p++;
	
    for(i=2; ipod_model_table[i].model_number != NULL; i++)
    {
	if(g_strncasecmp(p, ipod_model_table[i].model_number, 4) == 0)
	{
	    g_free(model_num);
	    return &ipod_model_table[i];
	}
    }	
    g_free(model_num);
    return &ipod_model_table[1];
}


/* Return supported artwork formats supported by this iPod */
G_GNUC_INTERNAL const Itdb_ArtworkFormat *
itdb_device_get_artwork_formats (Itdb_Device *device)
{
    const Itdb_IpodModel *model;

    g_return_val_if_fail (device, NULL);

    model = itdb_device_get_ipod_model (device);

    g_return_val_if_fail (model, NULL);

    return ipod_artwork_info_table[model->model_type];
}


/* Determine the number of F.. directories in iPod_Control/Music.

   If device->musicdirs is already set, simply return the previously
   determined number. Otherwise count the directories first and set
   itdb->musicdirs. */
G_GNUC_INTERNAL gint
itdb_device_musicdirs_number (Itdb_Device *device)
{
    gchar *dir_filename = NULL;
    gint dir_num;

    g_return_val_if_fail (device, 0);

    if (device->musicdirs <= 0)
    {
	gchar *music_dir = itdb_get_music_dir (device->mountpoint);
	if (!music_dir) return 0;
	/* count number of dirs */
	for (dir_num=0; ;++dir_num)
	{
	    gchar dir_num_str[5];

	    g_snprintf (dir_num_str, 5, "F%02d", dir_num);
  
	    dir_filename = itdb_get_path (music_dir, dir_num_str);

	    if (!dir_filename)  break;
	    g_free (dir_filename);
	}
	device->musicdirs = dir_num;
	g_free (music_dir);
    }
    return device->musicdirs;
}


/* Attempt to guess the endianess used by this iPod.
 *
 * It will overwrite the previous setting.
 *
 */

G_GNUC_INTERNAL void
itdb_device_autodetect_endianess (Itdb_Device *device)
{
    g_return_if_fail (device);

    /* default: non-reversed */
    device->byte_order = G_LITTLE_ENDIAN;

    if (device->mountpoint)
    {
	gchar *control_dir = itdb_get_control_dir (device->mountpoint);
    
	if (control_dir)
	{
	    gchar *cd_l = g_ascii_strdown (control_dir, -1);
	    if (strstr (cd_l, "itunes/itunes_control") ==
		(cd_l + strlen (cd_l) - strlen ("itunes/itunes_control")))
	    {
		device->byte_order = G_BIG_ENDIAN;
	    }
	    g_free (cd_l);
	    g_free (control_dir);
	}
    }
}

/* Time-stamp: <2006-06-06 00:13:09 jcs>
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

#include "itdb_device.h"
#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <glib/gi18n-lib.h>

static const Itdb_IpodInfo ipod_info_table [] = {
    /* Handle idiots who hose their iPod file system, or lucky people
       with iPods we don't yet know about*/
    {"Invalid", 0,  ITDB_IPOD_MODEL_INVALID,     ITDB_IPOD_GENERATION_UNKNOWN, 0},
    {"Unknown", 0,  ITDB_IPOD_MODEL_UNKNOWN,     ITDB_IPOD_GENERATION_UNKNOWN, 0},

    /* First Generation */
    /* Mechanical buttons arranged around rotating "scroll wheel".
       8513, 8541 and 8709 are Mac types, 8697 is PC */
    {"8513",  5, ITDB_IPOD_MODEL_REGULAR,     ITDB_IPOD_GENERATION_FIRST,  20},
    {"8541",  5, ITDB_IPOD_MODEL_REGULAR,     ITDB_IPOD_GENERATION_FIRST,  20},
    {"8697",  5, ITDB_IPOD_MODEL_REGULAR,     ITDB_IPOD_GENERATION_FIRST,  20},
    {"8709", 10, ITDB_IPOD_MODEL_REGULAR,     ITDB_IPOD_GENERATION_FIRST,  20},

    /* Second Generation */
    /* Same buttons as First Generation but around touch-sensitive
       "touch wheel". 8737 and 8738 are Mac types, 8740 and 8741 * are
       PC */
    {"8737", 10, ITDB_IPOD_MODEL_REGULAR,     ITDB_IPOD_GENERATION_SECOND, 20},
    {"8740", 10, ITDB_IPOD_MODEL_REGULAR,     ITDB_IPOD_GENERATION_SECOND, 20},
    {"8738", 20, ITDB_IPOD_MODEL_REGULAR,     ITDB_IPOD_GENERATION_SECOND, 50},
    {"8741", 20, ITDB_IPOD_MODEL_REGULAR,     ITDB_IPOD_GENERATION_SECOND, 50},

    /* Third Generation */
    /* Touch sensitive buttons and arranged in a line above "touch
       wheel". Docking connector was introduced here, same models for
       Mac and PC from now on. */
    {"8976", 10, ITDB_IPOD_MODEL_REGULAR,     ITDB_IPOD_GENERATION_THIRD,  20},
    {"8946", 15, ITDB_IPOD_MODEL_REGULAR,     ITDB_IPOD_GENERATION_THIRD,  50},
    {"9460", 15, ITDB_IPOD_MODEL_REGULAR,     ITDB_IPOD_GENERATION_THIRD,  50},
    {"9244", 20, ITDB_IPOD_MODEL_REGULAR,     ITDB_IPOD_GENERATION_THIRD,  50},
    {"8948", 30, ITDB_IPOD_MODEL_REGULAR,     ITDB_IPOD_GENERATION_THIRD,  50},
    {"9245", 40, ITDB_IPOD_MODEL_REGULAR,     ITDB_IPOD_GENERATION_THIRD,  50},

    /* Fourth Generation */
    /* Buttons are now integrated into the "touch wheel". */
    {"9282", 20, ITDB_IPOD_MODEL_REGULAR,     ITDB_IPOD_GENERATION_FOURTH, 50},
    {"9787", 25, ITDB_IPOD_MODEL_REGULAR_U2,  ITDB_IPOD_GENERATION_FOURTH, 50},
    {"9268", 40, ITDB_IPOD_MODEL_REGULAR,     ITDB_IPOD_GENERATION_FOURTH, 50},
    {"A079", 20, ITDB_IPOD_MODEL_COLOR,       ITDB_IPOD_GENERATION_FOURTH, 50},
    {"A127", 20, ITDB_IPOD_MODEL_COLOR_U2,    ITDB_IPOD_GENERATION_FOURTH, 50},
    {"9830", 60, ITDB_IPOD_MODEL_COLOR,       ITDB_IPOD_GENERATION_FOURTH, 50},

    /* First Generation Mini */
    {"9160",  4, ITDB_IPOD_MODEL_MINI,        ITDB_IPOD_GENERATION_FIRST,   6},
    {"9436",  4, ITDB_IPOD_MODEL_MINI_BLUE,   ITDB_IPOD_GENERATION_FIRST,   6},
    {"9435",  4, ITDB_IPOD_MODEL_MINI_PINK,   ITDB_IPOD_GENERATION_FIRST,   6},
    {"9434",  4, ITDB_IPOD_MODEL_MINI_GREEN,  ITDB_IPOD_GENERATION_FIRST,   6},
    {"9437",  4, ITDB_IPOD_MODEL_MINI_GOLD,   ITDB_IPOD_GENERATION_FIRST,   6},	

    /* Second Generation Mini */
    {"9800",  4, ITDB_IPOD_MODEL_MINI,        ITDB_IPOD_GENERATION_SECOND,  6},
    {"9802",  4, ITDB_IPOD_MODEL_MINI_BLUE,   ITDB_IPOD_GENERATION_SECOND,  6},
    {"9804",  4, ITDB_IPOD_MODEL_MINI_PINK,   ITDB_IPOD_GENERATION_SECOND,  6},
    {"9806",  4, ITDB_IPOD_MODEL_MINI_GREEN,  ITDB_IPOD_GENERATION_SECOND,  6},
    {"9801",  6, ITDB_IPOD_MODEL_MINI,        ITDB_IPOD_GENERATION_SECOND, 20},
    {"9803",  6, ITDB_IPOD_MODEL_MINI_BLUE,   ITDB_IPOD_GENERATION_SECOND, 20},
    {"9805",  6, ITDB_IPOD_MODEL_MINI_PINK,   ITDB_IPOD_GENERATION_SECOND, 20},
    {"9807",  6, ITDB_IPOD_MODEL_MINI_GREEN,  ITDB_IPOD_GENERATION_SECOND, 20},	

    /* Photo / Fourth Generation */
    /* Buttons are integrated into the "touch wheel". */
    {"9829", 30, ITDB_IPOD_MODEL_COLOR,       ITDB_IPOD_GENERATION_FOURTH, 50},
    {"9585", 40, ITDB_IPOD_MODEL_COLOR,       ITDB_IPOD_GENERATION_FOURTH, 50},
    {"9586", 60, ITDB_IPOD_MODEL_COLOR,       ITDB_IPOD_GENERATION_FOURTH, 50},
    {"9830", 60, ITDB_IPOD_MODEL_COLOR,       ITDB_IPOD_GENERATION_FOURTH, 50},

    /* Shuffle / Fourth Generation */
    {"9724", 0.5,ITDB_IPOD_MODEL_SHUFFLE,     ITDB_IPOD_GENERATION_FOURTH,  3},
    {"9725", 1,  ITDB_IPOD_MODEL_SHUFFLE,     ITDB_IPOD_GENERATION_FOURTH,  3},

    /* Nano / Fifth Generation */
    /* Buttons are integrated into the "touch wheel". */
    {"A350",  1, ITDB_IPOD_MODEL_NANO_WHITE,  ITDB_IPOD_GENERATION_FIFTH,   3},
    {"A352",  1, ITDB_IPOD_MODEL_NANO_BLACK,  ITDB_IPOD_GENERATION_FIFTH,   3},
    {"A004",  2, ITDB_IPOD_MODEL_NANO_WHITE,  ITDB_IPOD_GENERATION_FIFTH,   3},
    {"A099",  2, ITDB_IPOD_MODEL_NANO_BLACK,  ITDB_IPOD_GENERATION_FIFTH,   3},
    {"A005",  4, ITDB_IPOD_MODEL_NANO_WHITE,  ITDB_IPOD_GENERATION_FIFTH,   6},
    {"A107",  4, ITDB_IPOD_MODEL_NANO_BLACK,  ITDB_IPOD_GENERATION_FIFTH,   6},

    /* Video / Fifth Generation */
    /* Buttons are integrated into the "touch wheel". */
    {"A002", 30, ITDB_IPOD_MODEL_VIDEO_WHITE, ITDB_IPOD_GENERATION_FIFTH,  50},
    {"A146", 30, ITDB_IPOD_MODEL_VIDEO_BLACK, ITDB_IPOD_GENERATION_FIFTH,  50},
    {"A003", 60, ITDB_IPOD_MODEL_VIDEO_WHITE, ITDB_IPOD_GENERATION_FIFTH,  50},
    {"A147", 60, ITDB_IPOD_MODEL_VIDEO_BLACK, ITDB_IPOD_GENERATION_FIFTH,  50},

    /* HP iPods, need contributions for this table */
    /* Buttons are integrated into the "touch wheel". */
    {"E436", 40, ITDB_IPOD_MODEL_REGULAR,     ITDB_IPOD_GENERATION_FOURTH, 50},
    {"S492", 30, ITDB_IPOD_MODEL_COLOR,       ITDB_IPOD_GENERATION_FOURTH, 50},

    /* No known model number -- create a Device/SysInfo file with
     * one entry, e.g.:
       ModelNumStr: Mmobile1
    */
    {"mobile1", -1, ITDB_IPOD_MODEL_MOBILE_1, ITDB_IPOD_GENERATION_MOBILE},

    {NULL, 0, 0, 0, 0}
};

static const gchar *ipod_model_name_table [] = {
	N_("Invalid"),
	N_("Unknown"),
	N_("Color"),
	N_("Color U2"),
	N_("Grayscale"),
	N_("Grayscale U2"),
	N_("Mini (Silver)"),
	N_("Mini (Blue)"),
	N_("Mini (Pink)"),
	N_("Mini (Green)"),
	N_("Mini (Gold)"),
	N_("Shuffle"),
	N_("Nano (White)"),
	N_("Nano (Black)"),
	N_("Video (White)"),
	N_("Video (Black)"),
	N_("Mobile (1)"),
	NULL
};

static const gchar *ipod_generation_name_table [] = {
	N_("Unknown"),
	N_("First Generation"),
	N_("Second Generation"),
	N_("Third Generation"),
	N_("Fourth Generation"),
	N_("Fifth Generation"),
	N_("Mobile Phone"),
	NULL
};

static const Itdb_ArtworkFormat ipod_color_artwork_info[] = {
	{ITDB_THUMB_COVER_SMALL,        56,  56, 1017},
	{ITDB_THUMB_COVER_LARGE,       140, 140, 1016},
	{ITDB_THUMB_PHOTO_SMALL,        42,  30, 1009},
	{ITDB_THUMB_PHOTO_LARGE,       130,  88, 1015},
	{ITDB_THUMB_PHOTO_FULL_SCREEN, 220, 176, 1013},
	{ITDB_THUMB_PHOTO_TV_SCREEN,   720, 480, 1019},
	{-1,                            -1,  -1,   -1}
};

static const Itdb_ArtworkFormat ipod_nano_artwork_info[] = {
	{ITDB_THUMB_COVER_SMALL,        42,  42, 1031},
	{ITDB_THUMB_COVER_LARGE,       100, 100, 1027},
	{ITDB_THUMB_PHOTO_LARGE,        42,  37, 1032},
	{ITDB_THUMB_PHOTO_FULL_SCREEN, 176, 132, 1023},
	{-1,                            -1,  -1,   -1}
};

static const Itdb_ArtworkFormat ipod_video_artwork_info[] = {
	{ITDB_THUMB_COVER_SMALL,       100, 100, 1028},
	{ITDB_THUMB_COVER_LARGE,       200, 200, 1029},
	{ITDB_THUMB_PHOTO_SMALL,        50,  41, 1036},
	{ITDB_THUMB_PHOTO_LARGE,       130,  88, 1015},
	{ITDB_THUMB_PHOTO_FULL_SCREEN, 320, 240, 1024},
	{ITDB_THUMB_PHOTO_TV_SCREEN,   720, 480, 1019},
	{-1,                            -1,  -1,   -1}
};

static const Itdb_ArtworkFormat ipod_mobile_1_artwork_info[] = {
	{ITDB_THUMB_COVER_SMALL,        50,  50, 2002},
	{ITDB_THUMB_COVER_LARGE,       150, 150, 2003},
	{-1,                            -1,  -1,   -1}
};

/* This will be indexed using a value from the ITDB_IPOD_MODEL enum */
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


/**
 * itdb_device_get_ipod_info:
 * @device: an #Itdb_Device
 *
 * Retrieve the #Itdb_IpodInfo entry for this iPod
 *
 * Return value: the #Itdb_IpodInfo entry for this iPod
 **/
const Itdb_IpodInfo *
itdb_device_get_ipod_info (Itdb_Device *device)
{
    gint i;
    gchar *model_num, *p;

    model_num = itdb_device_get_sysinfo (device, "ModelNumStr");

    if (!model_num)
	return &ipod_info_table[0];

    p = model_num;

    if(isalpha(model_num[0])) 
	p++;
	
    for(i=2; ipod_info_table[i].model_number != NULL; i++)
    {
	if(g_strncasecmp(p, ipod_info_table[i].model_number, 4) == 0)
	{
	    g_free(model_num);
	    return &ipod_info_table[i];
	}
    }	
    g_free(model_num);
    return &ipod_info_table[1];
}



/* Return supported artwork formats supported by this iPod */
G_GNUC_INTERNAL const Itdb_ArtworkFormat *
itdb_device_get_artwork_formats (Itdb_Device *device)
{
    const Itdb_IpodInfo *info;

    g_return_val_if_fail (device, NULL);

    info = itdb_device_get_ipod_info (device);

    g_return_val_if_fail (info, NULL);

    return ipod_artwork_info_table[info->ipod_model];
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

/**
 * endianess_check_path:
 *
 * Check if endianess can be determined by looking at header of @path.
 *
 * @path:   the file to look at.
 * @hdr: the header string (4 bytes) in case of LITTLE_ENDIAN
 *
 * Return value:
 * G_LITTLE_ENDIAN, G_BIG_ENDIAN or 0 if endianess could not be
 * determined.
 */
static guint endianess_check_path (const gchar *path, const gchar *hdr)
{
    guint byte_order = 0;

    if (path)
    {
	int fd = open (path, O_RDONLY);
	if (fd != -1)
	{
	    gchar buf[4];
	    if (read (fd, buf, 4) == 4)
	    {
		if (strncmp (buf, hdr, 4) == 0)
		{
		    byte_order = G_LITTLE_ENDIAN;
		}
		else
		{
		    if ((buf[0] == hdr[3]) &&
			(buf[1] == hdr[2]) &&
			(buf[2] == hdr[1]) &&
			(buf[3] == hdr[0]))
		    {
			byte_order = G_LITTLE_ENDIAN;
		    }
		}
	    }
	    close (fd);
	}
    }
    return byte_order;
}

/* Attempt to guess the endianess used by this iPod.
 *
 * It will overwrite the previous setting.
 *
 */

G_GNUC_INTERNAL void
itdb_device_autodetect_endianess (Itdb_Device *device)
{
    guint byte_order = 0;

    g_return_if_fail (device);

    if (device->mountpoint)
    {
	gchar *path;
	if (byte_order == 0)
	{
	    /* First try reading the iTunesDB */
	    path = itdb_get_itunesdb_path (device->mountpoint);
	    byte_order = endianess_check_path (path, "mhbd");
	    g_free (path);
	}
	if (byte_order == 0)
	{
	    /* Try reading the ArtworkDB */
	    path = itdb_get_artworkdb_path (device->mountpoint);
	    byte_order = endianess_check_path (path, "mhfd");
	    g_free (path);
	}
	if (byte_order == 0)
	{
	    /* Try reading the Photos Database */
	    path = itdb_get_photodb_path (device->mountpoint);
	    byte_order = endianess_check_path (path, "mhfd");
	    g_free (path);
	}
	if (byte_order == 0)
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

    /* default: non-reversed */
    if (byte_order == 0)
	byte_order = G_LITTLE_ENDIAN;

    device->byte_order = byte_order;
}

/*------------------------------------------------------------------*\
 *                                                                  *
 * Functions for applications to obtain general infos about various *
 * iPods.                                                           *
 *                                                                  *
\*------------------------------------------------------------------*/

/**
 * itdb_info_get_ipod_info_table:
 *
 * Return a pointer to the start of valid iPod model descriptions,
 * which is an array of #Itdb_IpodInfo entries.
 *
 * Return value: a pointer to the array of #Itdb_IpodInfo entries.
 **/
const Itdb_IpodInfo *itdb_info_get_ipod_info_table (void)
{
    return &ipod_info_table[2];
}


/**
 * itdb_info_get_ipod_model_string:
 *
 * Return the iPod's generic model name, like "Color", "Nano"...
 *
 * Return value: a pointer to the model name. This is a static string
 * and must not be g_free()d.
 **/
const gchar *itdb_info_get_ipod_model_name_string (Itdb_IpodModel model)
{
    gint i=0;

    /* Instead of returning ipod_model_name_table[model] directly,
       verify if @model is valid */
    while (ipod_model_name_table[i])
    {
	if (i==model)  return gettext (ipod_model_name_table[i]);
	++i;
    }
    return NULL;
}


/**
 * itdb_info_get_ipod_generation_string:
 *
 * Return the iPod's generic generation name, like "First Generation",
 * "Mobile Phone"...
 *
 * Return value: a pointer to the generation name. This is a static
 * string and must not be g_free()d.
 **/
const gchar *itdb_info_get_ipod_generation_string (Itdb_IpodGeneration generation)
{
    gint i=0;

    /* Instead of returning ipod_generation_name_table[generation]
       directly, verify if @generation is valid */
    while (ipod_generation_name_table[i])
    {
	if (i==generation)
	    return gettext (ipod_generation_name_table[i]);
	++i;
    }
    return NULL;
}

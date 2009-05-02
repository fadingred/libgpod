/* Copyright (c) 2007, Christophe Fergeau  <teuf@gnome.org>
 * Part of the libgpod project.
 * 
 * URL: http://www.gtkpod.org/
 * URL: http://gtkpod.sourceforge.net/
 *
 * The code contained in this file is free software; you can redistribute
 * it and/or modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either version
 * 2.1 of the License, or (at your option) any later version.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this code; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * iTunes and iPod are trademarks of Apple
 *
 * This product is not supported/written/published by Apple!
 *
 */

#include <errno.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <glib-object.h>

#include <libhal.h>
#ifndef __USE_BSD
  #define __USE_BSD /* for mkdtemp */
#endif
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <sys/mount.h>
#include <itdb.h>
#include <itdb_device.h>
extern char *read_sysinfo_extended (const char *device);


static char *
get_icon_name (const Itdb_IpodInfo *info)
{
	if (info == NULL) {
		return g_strdup ("multimedia-player-apple-ipod");
	}
	switch (info->ipod_generation) {
	case ITDB_IPOD_GENERATION_UNKNOWN:
	case ITDB_IPOD_GENERATION_FIRST:
	case ITDB_IPOD_GENERATION_SECOND:
	case ITDB_IPOD_GENERATION_THIRD:
	case ITDB_IPOD_GENERATION_FOURTH:
		return g_strdup ("multimedia-player-apple-ipod");

	case ITDB_IPOD_GENERATION_PHOTO:
		return g_strdup ("multimedia-player-apple-ipod-color");

	case ITDB_IPOD_GENERATION_MINI_1:
	case ITDB_IPOD_GENERATION_MINI_2:
		switch (info->ipod_model) {
		case ITDB_IPOD_MODEL_MINI_BLUE:
			return g_strdup ("multimedia-player-apple-ipod-mini-blue");
		case ITDB_IPOD_MODEL_MINI_PINK:
			return g_strdup ("multimedia-player-apple-ipod-mini-pink");
		case ITDB_IPOD_MODEL_MINI_GOLD:
			return g_strdup ("multimedia-player-apple-ipod-mini-gold");
		case ITDB_IPOD_MODEL_MINI_GREEN:
			return g_strdup ("multimedia-player-apple-ipod-mini-green");
		case ITDB_IPOD_MODEL_MINI:
			return g_strdup ("multimedia-player-apple-ipod-mini-silver");
		default:
			g_assert_not_reached ();
		}

	case ITDB_IPOD_GENERATION_SHUFFLE_1:
		return g_strdup ("multimedia-player-apple-ipod-shuffle");

	case ITDB_IPOD_GENERATION_SHUFFLE_2:
	case ITDB_IPOD_GENERATION_SHUFFLE_3:
		switch (info->ipod_model) {
		case ITDB_IPOD_MODEL_SHUFFLE_SILVER:
			return g_strdup ("multimedia-player-apple-ipod-shuffle-clip-silver");
		case ITDB_IPOD_MODEL_SHUFFLE_GREEN:
			return g_strdup ("multimedia-player-apple-ipod-shuffle-clip-green");
		case ITDB_IPOD_MODEL_SHUFFLE_ORANGE:
			return g_strdup ("multimedia-player-apple-ipod-shuffle-clip-orange");
		case ITDB_IPOD_MODEL_SHUFFLE_PURPLE:
			return g_strdup ("multimedia-player-apple-ipod-shuffle-clip-purple");
		case ITDB_IPOD_MODEL_SHUFFLE_PINK:
			return g_strdup ("multimedia-player-apple-ipod-shuffle-clip-pink");
		case ITDB_IPOD_MODEL_SHUFFLE_BLUE:
			return g_strdup ("multimedia-player-apple-ipod-shuffle-clip-blue");
		default:
			g_assert_not_reached ();
		}

	case ITDB_IPOD_GENERATION_SHUFFLE_4:
		return g_strdup ("multimedia-player-apple-ipod-shuffle");

	case ITDB_IPOD_GENERATION_NANO_1:
		if (info->ipod_model == ITDB_IPOD_MODEL_NANO_BLACK) {
			return g_strdup ("multimedia-player-apple-ipod-nano-black");
		} else {
			return g_strdup ("multimedia-player-apple-ipod-nano-white");
		}

	case ITDB_IPOD_GENERATION_NANO_2:
		return g_strdup ("multimedia-player-apple-ipod-nano-white");

	case ITDB_IPOD_GENERATION_NANO_3:
		switch (info->ipod_model) {
		case ITDB_IPOD_MODEL_NANO_SILVER:
			return g_strdup ("multimedia-player-apple-ipod-nano-video");
		case ITDB_IPOD_MODEL_NANO_BLACK:
			return g_strdup ("multimedia-player-apple-ipod-nano-video-black");
		case ITDB_IPOD_MODEL_NANO_BLUE:
			return g_strdup ("multimedia-player-apple-ipod-nano-video-turquoise");
		case ITDB_IPOD_MODEL_NANO_GREEN:
			return g_strdup ("multimedia-player-apple-ipod-nano-video-green");
		case ITDB_IPOD_MODEL_NANO_RED:
			return g_strdup ("multimedia-player-apple-ipod-nano-video-red");
		default:
			g_assert_not_reached ();
		}

	case ITDB_IPOD_GENERATION_NANO_4:
		/* FIXME: set the correct icon name once it's added to
		 * gnome-icon-theme-extras 
		 */
		return g_strdup ("multimedia-player-apple-ipod-nano-white");

	case ITDB_IPOD_GENERATION_VIDEO_1:
	case ITDB_IPOD_GENERATION_VIDEO_2:
		if (info->ipod_model == ITDB_IPOD_MODEL_VIDEO_BLACK) {
			return g_strdup ("multimedia-player-apple-ipod-video-black");
		} else {
			return g_strdup ("multimedia-player-apple-ipod-video-white");
		}

	case ITDB_IPOD_GENERATION_CLASSIC_1:
	case ITDB_IPOD_GENERATION_CLASSIC_2:
		if (info->ipod_model == ITDB_IPOD_MODEL_CLASSIC_BLACK) {
			return g_strdup ("multimedia-player-apple-ipod-classic-black");
		} else {
			return g_strdup ("multimedia-player-apple-ipod-classic-white");
		}

	case ITDB_IPOD_GENERATION_TOUCH_1:
	case ITDB_IPOD_GENERATION_IPHONE_1:
	case ITDB_IPOD_GENERATION_MOBILE:
		return g_strdup ("multimedia-player-apple-ipod");
	}

	return g_strdup ("multimedia-player-apple-ipod");
}

/* taken from libipoddevice proper */
static LibHalContext *
hal_ipod_initialize(void)
{
	LibHalContext *hal_context;
	DBusError error;
	DBusConnection *dbus_connection;

	hal_context = libhal_ctx_new();
if(hal_context == NULL) {
		return NULL;
	}

	dbus_error_init(&error);
	dbus_connection = dbus_bus_get(DBUS_BUS_SYSTEM, &error);
	if (dbus_error_is_set(&error)) {
		dbus_error_free(&error);
		libhal_ctx_free(hal_context);
		return NULL;
	}

	libhal_ctx_set_dbus_connection(hal_context, dbus_connection);

	if(!libhal_ctx_init(hal_context, &error)) {
		if (dbus_error_is_set(&error)) {
			dbus_error_free(&error);
		}
		libhal_ctx_free(hal_context);
		return NULL;
	}

	return hal_context;
}

#define LIBGPOD_HAL_NS "org.libgpod."

static gboolean hal_ipod_set_properties (const SysInfoIpodProperties *props)
{
	LibHalContext *ctx;
	const char *udi;
	const char *serial_number;
	char *icon_name;
	const Itdb_IpodInfo *info;

        ctx = hal_ipod_initialize ();
        if (ctx == NULL) {
                return FALSE;
        }
	udi = g_getenv ("UDI");
	if (udi == NULL) {
		return FALSE;
	}
        libhal_device_set_property_int (ctx, udi, 
			                LIBGPOD_HAL_NS"version", 1, NULL);

	serial_number = itdb_sysinfo_properties_get_serial_number (props);

	info = itdb_ipod_info_from_serial (serial_number);

	if ((info == NULL) || (info->ipod_generation == ITDB_IPOD_GENERATION_UNKNOWN)) {
		libhal_device_set_property_bool (ctx, udi,
				                 LIBGPOD_HAL_NS"ipod.is_unknown", 
						 TRUE, NULL);
		return TRUE;
	} else {
		libhal_device_set_property_bool (ctx, udi,
				                 LIBGPOD_HAL_NS"ipod.is_unknown", 
						 FALSE, NULL);
	}

	icon_name = get_icon_name (info);
	libhal_device_set_property_string (ctx, udi, "info.desktop.icon",
					   icon_name, NULL);
	g_free (icon_name);

	if (itdb_sysinfo_properties_get_firewire_id (props) != NULL) {
		const char *fwid;
		fwid = itdb_sysinfo_properties_get_firewire_id (props);
		libhal_device_set_property_string (ctx, udi,
		  	  	     		   LIBGPOD_HAL_NS"ipod.firewire_id",
						   fwid, NULL);
	}

	if (serial_number != NULL) {
		libhal_device_set_property_string (ctx, udi,
						   LIBGPOD_HAL_NS"ipod.serial_number",
						   serial_number,
						   NULL);
	}
	libhal_device_set_property_bool (ctx, udi,
				         LIBGPOD_HAL_NS"ipod.images.album_art_supported",
					 (itdb_sysinfo_properties_get_cover_art_formats (props) != NULL),
					 NULL);
	libhal_device_set_property_bool (ctx, udi,
			   	         LIBGPOD_HAL_NS"ipod.images.photos_supported",
					 (itdb_sysinfo_properties_get_photo_formats (props) != NULL),
					 NULL);
	libhal_device_set_property_bool (ctx, udi,
			   	         LIBGPOD_HAL_NS"ipod.images.chapter_images_supported",
					 (itdb_sysinfo_properties_get_chapter_image_formats (props) != NULL),
					 NULL);
        libhal_ctx_free (ctx);

	return TRUE;
}

static char *mount_ipod (const char *dev_path)
{
        char *filename;
        char *tmpname;
        const char *fstype;
        int result;

        fstype = g_getenv ("HAL_PROP_VOLUME_FSTYPE");
        if (fstype == NULL) {
                return NULL;
        }
        filename = g_build_filename (g_get_tmp_dir (), "ipodXXXXXX", NULL);
        if (filename == NULL) {
                return NULL;
        }
        tmpname = mkdtemp (filename);
        if (tmpname == NULL) {
                g_free (filename);
                return NULL;
        }
        g_assert (tmpname == filename);
        result = mount (dev_path, tmpname, fstype, 0, NULL);
        if (result != 0) {
                g_rmdir (filename);
                g_free (filename);
                return NULL;
        }

        return tmpname;
}

static gboolean write_sysinfo_extended (const char *mountpoint, 
                                        const char *data)
{
        char *filename;
        char *devdirpath;
        gboolean result;

        devdirpath = itdb_get_device_dir (mountpoint);
        if (devdirpath == NULL) {
                return FALSE;
        }
        filename = g_build_filename (devdirpath, "SysInfoExtended", NULL);
        g_free (devdirpath);
        if (filename == NULL) {
                return FALSE;
        }

        result = g_file_set_contents (filename, data, -1, NULL);
        g_free (filename);

        return result;
}

int main (int argc, char **argv)
{
        char *ipod_mountpoint;
        char *xml;
	SysInfoIpodProperties *props;

	g_type_init ();

        xml = read_sysinfo_extended (g_getenv ("HAL_PROP_BLOCK_DEVICE"));
        if (xml == NULL) {
                return -1;
        }

	props = itdb_sysinfo_extended_parse_from_xml (xml, NULL);
	hal_ipod_set_properties (props);
	itdb_sysinfo_properties_free (props);


        ipod_mountpoint = mount_ipod (g_getenv ("HAL_PROP_BLOCK_DEVICE"));
        if (ipod_mountpoint == NULL) {
                g_free (xml);
                return -1;
        }
        write_sysinfo_extended (ipod_mountpoint, xml); 
        g_free (xml);

        umount (ipod_mountpoint);
        g_rmdir (ipod_mountpoint);
        g_free (ipod_mountpoint);

        return 0;
}

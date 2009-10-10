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
#ifdef HAVE_SGUTILS
extern char *read_sysinfo_extended (const char *device);
#endif
#ifdef HAVE_LIBIPHONE
extern char *read_sysinfo_extended_by_uuid (const char *uuid);
#endif

struct _ProductionInfo {
	gchar *factory_id;
	guint production_year;
	guint production_week;
	guint production_index;
	char *model_id;
};
typedef struct _ProductionInfo ProductionInfo;

static void 
production_info_free (ProductionInfo *info)
{
	g_return_if_fail (info != NULL);
	g_free (info->factory_id);
	g_free (info->model_id);
	g_free (info);
}

static ProductionInfo *
parse_serial_number (const char *serial_number)
{
	ProductionInfo *info;
	char int_str[4];

	if (serial_number == NULL) {
		return NULL;
	}
	if (strlen (serial_number) < 11) {
		return NULL;
	}
	info = g_new0 (ProductionInfo, 1);
	info->factory_id = g_strndup (serial_number, 2);
	serial_number += 2;

	strncpy (int_str, serial_number, 1);
	serial_number += 1;
	info->production_year = 2000 + g_ascii_strtoull (int_str, NULL, 10);

	strncpy (int_str, serial_number, 2);
	serial_number += 2;
	info->production_week = g_ascii_strtoull (int_str, NULL, 10);

	strncpy (int_str, serial_number, 3);
	serial_number += 3;
	info->production_index = g_ascii_strtoull (int_str, NULL, 36);

	info->model_id = g_strdup (serial_number);

	return info;
}

enum ArtworkType {
	UNKNOWN,
	PHOTO,
	ALBUM,
	CHAPTER
};

static char *
get_format_string (const Itdb_ArtworkFormat *format, enum ArtworkType type)
{
	const char *format_name;
	const char *artwork_type;

	g_return_val_if_fail (format != NULL, NULL);
	switch (format->format) {
            case THUMB_FORMAT_UYVY_BE:
		    format_name = "iyuv";
		    break;
            case THUMB_FORMAT_RGB565_BE:
		    format_name = "rgb565_be";
		    break;		 
	    case THUMB_FORMAT_RGB565_LE:
		    format_name = "rgb565";
		    break;
	    case THUMB_FORMAT_I420_LE:
		    format_name = "iyuv420";
		    break;
	    default:
		    g_return_val_if_reached (NULL);
	}

	switch (type) {
            case UNKNOWN:
		    artwork_type = "unknown";
		    break;
            case PHOTO:
	            artwork_type = "photo";
		    break;
	    case ALBUM:
		    artwork_type = "album";
		    break;
	    case CHAPTER:
		    artwork_type = "chapter";
		    break;
	    default:
		    g_return_val_if_reached (NULL);
	}

	return g_strdup_printf ("corr_id=%u,width=%u,height=%u,rotation=%u,pixel_format=%s,image_type=%s",
  		                format->format_id, 
				format->width, format->height, 
			        format->rotation, format_name, artwork_type);
}

static char *
get_model_name (const Itdb_IpodInfo *info)
{
	if (info == NULL) {
		return NULL;
	}
	switch (info->ipod_generation) {
	case ITDB_IPOD_GENERATION_UNKNOWN:
		return g_strdup ("unknown");
	case ITDB_IPOD_GENERATION_FIRST:
	case ITDB_IPOD_GENERATION_SECOND:
	case ITDB_IPOD_GENERATION_THIRD:
	case ITDB_IPOD_GENERATION_FOURTH:
		return g_strdup ("grayscale");
	case ITDB_IPOD_GENERATION_PHOTO:
		return g_strdup ("color");
	case ITDB_IPOD_GENERATION_MINI_1:
	case ITDB_IPOD_GENERATION_MINI_2:
		return g_strdup ("mini");
	case ITDB_IPOD_GENERATION_SHUFFLE_1:
	case ITDB_IPOD_GENERATION_SHUFFLE_2:
	case ITDB_IPOD_GENERATION_SHUFFLE_3:
	case ITDB_IPOD_GENERATION_SHUFFLE_4:
		return g_strdup ("shuffle");
	case ITDB_IPOD_GENERATION_NANO_1:
	case ITDB_IPOD_GENERATION_NANO_2:
	case ITDB_IPOD_GENERATION_NANO_3:
	case ITDB_IPOD_GENERATION_NANO_4:
	case ITDB_IPOD_GENERATION_NANO_5:
		return g_strdup ("nano");
	case ITDB_IPOD_GENERATION_VIDEO_1:
	case ITDB_IPOD_GENERATION_VIDEO_2:
		return g_strdup ("video");
	case ITDB_IPOD_GENERATION_CLASSIC_1:
	case ITDB_IPOD_GENERATION_CLASSIC_2:
	case ITDB_IPOD_GENERATION_CLASSIC_3:
		return g_strdup ("classic");
	case ITDB_IPOD_GENERATION_TOUCH_1:
	case ITDB_IPOD_GENERATION_TOUCH_2:
		return g_strdup ("touch");
	case ITDB_IPOD_GENERATION_IPHONE_1:
	case ITDB_IPOD_GENERATION_IPHONE_2:
	case ITDB_IPOD_GENERATION_IPHONE_3:
		return g_strdup ("phone");
	case ITDB_IPOD_GENERATION_MOBILE:
		return g_strdup ("rokr");
	}

	g_assert_not_reached ();
}

static double
get_generation (const Itdb_IpodInfo *info)
{
	if (info == NULL) {
		return 0.0;
	}
	switch (info->ipod_generation) {
	case ITDB_IPOD_GENERATION_UNKNOWN:
		return 0.0;
	case ITDB_IPOD_GENERATION_FIRST:
		return 1.0;
	case ITDB_IPOD_GENERATION_SECOND:
		return 2.0;
	case ITDB_IPOD_GENERATION_THIRD:
		return 3.0;
	case ITDB_IPOD_GENERATION_FOURTH:
		return 4.0;
	case ITDB_IPOD_GENERATION_PHOTO:
		return 4.0;
	case ITDB_IPOD_GENERATION_MINI_1:
		return 1.0;
	case ITDB_IPOD_GENERATION_MINI_2:
		return 2.0;
	case ITDB_IPOD_GENERATION_SHUFFLE_1:
		return 1.0;
	case ITDB_IPOD_GENERATION_SHUFFLE_2:
		return 2.0;
	case ITDB_IPOD_GENERATION_SHUFFLE_3:
		return 3.0;
	case ITDB_IPOD_GENERATION_SHUFFLE_4:
		return 4.0;
	case ITDB_IPOD_GENERATION_NANO_1:
		return 1.0;
	case ITDB_IPOD_GENERATION_NANO_2:
		return 2.0;
	case ITDB_IPOD_GENERATION_NANO_3:
		return 3.0;
	case ITDB_IPOD_GENERATION_NANO_4:
		return 4.0;
	case ITDB_IPOD_GENERATION_NANO_5:
		return 5.0;
	case ITDB_IPOD_GENERATION_VIDEO_1:
		return 5.0;
	case ITDB_IPOD_GENERATION_VIDEO_2:
		return 5.5;
	case ITDB_IPOD_GENERATION_CLASSIC_1:
		return 6.0;
	case ITDB_IPOD_GENERATION_CLASSIC_2:
	case ITDB_IPOD_GENERATION_CLASSIC_3:
		return 6.5;
	case ITDB_IPOD_GENERATION_TOUCH_1:
		return 1.0;
	case ITDB_IPOD_GENERATION_TOUCH_2:
		return 2.0;
	case ITDB_IPOD_GENERATION_IPHONE_1:
		return 1.0;
	case ITDB_IPOD_GENERATION_IPHONE_2:
		return 2.0;
	case ITDB_IPOD_GENERATION_IPHONE_3:
		return 3.0;
	case ITDB_IPOD_GENERATION_MOBILE:
		return 1.0;
	}

	g_assert_not_reached ();
}
static char *
get_color_name (const Itdb_IpodInfo *info)
{
	if (info == NULL) {
		return NULL;
	}	
	switch (info->ipod_model) {
	case ITDB_IPOD_MODEL_INVALID:
	case ITDB_IPOD_MODEL_UNKNOWN:
		return NULL;
	case ITDB_IPOD_MODEL_COLOR:
	case ITDB_IPOD_MODEL_COLOR_U2:
	case ITDB_IPOD_MODEL_REGULAR:
	case ITDB_IPOD_MODEL_REGULAR_U2:
	case ITDB_IPOD_MODEL_NANO_WHITE:
	case ITDB_IPOD_MODEL_VIDEO_WHITE:
	case ITDB_IPOD_MODEL_SHUFFLE:
	case ITDB_IPOD_MODEL_MOBILE_1:
	case ITDB_IPOD_MODEL_IPHONE_WHITE:
		return g_strdup ("white");
	case ITDB_IPOD_MODEL_MINI:
	case ITDB_IPOD_MODEL_NANO_SILVER:
	case ITDB_IPOD_MODEL_SHUFFLE_SILVER:
	case ITDB_IPOD_MODEL_CLASSIC_SILVER:
	case ITDB_IPOD_MODEL_TOUCH_SILVER:
	case ITDB_IPOD_MODEL_IPHONE_1:
		return g_strdup ("silver");
	case ITDB_IPOD_MODEL_VIDEO_U2:
	case ITDB_IPOD_MODEL_NANO_BLACK:
	case ITDB_IPOD_MODEL_VIDEO_BLACK:
	case ITDB_IPOD_MODEL_CLASSIC_BLACK:
	case ITDB_IPOD_MODEL_SHUFFLE_BLACK:
	case ITDB_IPOD_MODEL_IPHONE_BLACK:
		return g_strdup ("black");
	case ITDB_IPOD_MODEL_MINI_PINK:
	case ITDB_IPOD_MODEL_NANO_PINK:
	case ITDB_IPOD_MODEL_SHUFFLE_PINK:
		return g_strdup ("pink");
	case ITDB_IPOD_MODEL_MINI_GREEN:
	case ITDB_IPOD_MODEL_NANO_GREEN:
	case ITDB_IPOD_MODEL_SHUFFLE_GREEN:
		return g_strdup ("green");
	case ITDB_IPOD_MODEL_MINI_GOLD:
		return g_strdup ("gold");
	case ITDB_IPOD_MODEL_NANO_BLUE:
	case ITDB_IPOD_MODEL_MINI_BLUE:
	case ITDB_IPOD_MODEL_SHUFFLE_BLUE:
		return g_strdup ("blue");
	case ITDB_IPOD_MODEL_SHUFFLE_RED:
	case ITDB_IPOD_MODEL_NANO_RED:
		return g_strdup ("red");
	case ITDB_IPOD_MODEL_SHUFFLE_ORANGE:
	case ITDB_IPOD_MODEL_NANO_ORANGE:
		return g_strdup ("orange");
	case ITDB_IPOD_MODEL_SHUFFLE_PURPLE:
	case ITDB_IPOD_MODEL_NANO_PURPLE:
		return g_strdup ("purple");
	case ITDB_IPOD_MODEL_NANO_YELLOW:
		return g_strdup ("yellow");
	}

	g_assert_not_reached ();
}

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
	case ITDB_IPOD_GENERATION_NANO_5:
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
	case ITDB_IPOD_GENERATION_CLASSIC_3:
		if (info->ipod_model == ITDB_IPOD_MODEL_CLASSIC_BLACK) {
			return g_strdup ("multimedia-player-apple-ipod-classic-black");
		} else {
			return g_strdup ("multimedia-player-apple-ipod-classic-white");
		}

	case ITDB_IPOD_GENERATION_TOUCH_1:
		return g_strdup ("multimedia-player-apple-ipod-touch");
	case ITDB_IPOD_GENERATION_TOUCH_2:
		return g_strdup ("multimedia-player-apple-ipod-touch-2g");
	case ITDB_IPOD_GENERATION_IPHONE_1:
		return g_strdup ("phone-apple-iphone");
	case ITDB_IPOD_GENERATION_IPHONE_2:
		return g_strdup ("phone-apple-iphone-3g");
	case ITDB_IPOD_GENERATION_IPHONE_3:
		return g_strdup ("phone-apple-iphone-3gs");
	case ITDB_IPOD_GENERATION_MOBILE:
		return g_strdup ("multimedia-player-apple-ipod");
	}

	g_assert_not_reached ();
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

static void set_format_info (LibHalContext *ctx, const char *udi,
                             const char *hal_propname,
                             const GList *formats,
                             enum ArtworkType type)
{
        const GList *it;

        libhal_device_set_property_bool (ctx, udi,
                                         hal_propname, (formats != NULL),
                                         NULL);
        for (it = formats; it != NULL; it = it->next) {
                char *format_str;

                format_str = get_format_string (it->data, type);
                libhal_device_property_strlist_append (ctx, udi,
                                                       LIBGPOD_HAL_NS"ipod.images.formats",
                                                       format_str, 
                                                       NULL);
                g_free (format_str);
        }
}

static void hal_set_artwork_information (const SysInfoIpodProperties *props,
                                         LibHalContext *ctx,
                                         const char *udi)
{
        const GList *formats;

        /* Cover art */
        formats = itdb_sysinfo_properties_get_cover_art_formats (props);
        set_format_info (ctx, udi, 
                         LIBGPOD_HAL_NS"ipod.images.album_art_supported",
                         formats, ALBUM);

        /* Photos */
        formats = itdb_sysinfo_properties_get_photo_formats (props);
        set_format_info (ctx, udi,
                         LIBGPOD_HAL_NS"ipod.images.photos_supported",
                         formats, PHOTO);

        /* Chapter images */
        formats = itdb_sysinfo_properties_get_chapter_image_formats (props);
        set_format_info (ctx, udi, 
                         LIBGPOD_HAL_NS"ipod.images.chapter_images_supported",
                         formats, CHAPTER);
}

static gboolean hal_ipod_set_properties (LibHalContext *ctx, 
					 const char *udi,
					 const SysInfoIpodProperties *props)
{
	const char *serial_number;
	const char *firmware_version;
	char *icon_name;
	const Itdb_IpodInfo *info;
	char *model_name;
	char *color_name;
	double generation;
	ProductionInfo *prod_info;

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

	firmware_version = itdb_sysinfo_properties_get_firmware_version (props);
	if (firmware_version != NULL) {
		libhal_device_set_property_string (ctx, udi,
						   LIBGPOD_HAL_NS"ipod.firmware_version",
						   firmware_version,
						   NULL);
	}

	hal_set_artwork_information (props, ctx, udi);

	model_name = get_model_name (info);
	if (model_name != NULL) {
		libhal_device_set_property_string (ctx, udi,
						   LIBGPOD_HAL_NS"ipod.model.device_class",
						   model_name,
						   NULL);
		g_free (model_name);
	}

	generation = get_generation (info);
	if (generation != 0.0) {
		libhal_device_set_property_double (ctx, udi,
						   LIBGPOD_HAL_NS"ipod.model.generation",
						   generation,
						   NULL);
	}

	color_name = get_color_name (info);
	if (color_name != NULL) {
		libhal_device_set_property_string (ctx, udi,
						   LIBGPOD_HAL_NS"ipod.model.shell_color",
						   color_name,
						   NULL);
		g_free (color_name);
	}

	if (serial_number != NULL) {
		prod_info = parse_serial_number (serial_number);
		if (prod_info != NULL) {
			if (prod_info->factory_id != NULL) {
				libhal_device_set_property_string (ctx, udi,
								   LIBGPOD_HAL_NS"ipod.production.factory_id",
								   prod_info->factory_id,
								   NULL);

			}
			if (prod_info->production_year != 0) {
				libhal_device_set_property_int (ctx, udi, 
			        			        LIBGPOD_HAL_NS"ipod.production.year",
							       	prod_info->production_year,
							       	NULL);

			}
			if (prod_info->production_week != 0) {
				libhal_device_set_property_int (ctx, udi, 
								LIBGPOD_HAL_NS"ipod.production.week",
								prod_info->production_week,
							       	NULL);

			}
			if (prod_info->production_index != 0) {
				libhal_device_set_property_int (ctx, udi, 
								LIBGPOD_HAL_NS"ipod.production.number",
								prod_info->production_index,
							       	NULL);
			}
		}
		production_info_free (prod_info);
	}

	return TRUE;
}

static gboolean hal_mounted_ipod_set_properties (LibHalContext *ctx,
						 const char *udi,
						 const char *ipod_mountpoint)
{
        Itdb_iTunesDB *itdb;
        Itdb_Playlist *mpl;
        char *control_path;

        itdb = itdb_parse (ipod_mountpoint, NULL);
        if (itdb == NULL) {
               return FALSE;
        }
        control_path = itdb_get_control_dir (ipod_mountpoint);
        if (control_path != NULL) {
	   	if (strlen (control_path) >= strlen (ipod_mountpoint)) {
			libhal_device_set_property_string (ctx, udi,
					LIBGPOD_HAL_NS"ipod.model.control_path",
					control_path + strlen (ipod_mountpoint),
					NULL);
			g_free (control_path);
		}
        }

        mpl = itdb_playlist_mpl (itdb);
        if (mpl == NULL) {
                return FALSE;
        }
	if (mpl->name != NULL) {
	    libhal_device_set_property_string (ctx, udi, "info.desktop.name",
					       mpl->name, NULL);
	}

        libhal_device_set_property_string (ctx, udi, "info.product",
                                           mpl->name, NULL);
        return FALSE;
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
	char *ipod_mountpoint = NULL;
	char *xml = NULL;
	SysInfoIpodProperties *props;
	LibHalContext *ctx; 
	const char *udi;
#ifdef HAVE_LIBIPHONE
	char *caps = NULL;
#endif

	g_type_init ();

#ifdef HAVE_LIBIPHONE
        caps = (char *)g_getenv ("HAL_PROP_INFO_CAPABILITIES");
        if (caps == NULL) {
                return -1;
        }
        else
        {
                /* iPhone/iPod Touch devices with custom usb protocol */
                if (strstr (caps, "afc") != NULL) {
                        xml = read_sysinfo_extended_by_uuid(g_getenv ("HAL_PROP_USB_SERIAL"));
                }
                else /* regular usb mass storage ipod */
                {
#endif
#ifdef HAVE_SGUTILS
                        xml = read_sysinfo_extended (g_getenv ("HAL_PROP_BLOCK_DEVICE"));
#else
                        return -1;
#endif
#ifdef HAVE_LIBIPHONE
                }
        }
#endif
        if (xml == NULL) {
                return -1;
        }
        props = itdb_sysinfo_extended_parse_from_xml (xml, NULL);

        ctx = hal_ipod_initialize ();
        if (ctx == NULL) {
		g_free (xml);
                itdb_sysinfo_properties_free (props);
                return FALSE;
        }
        udi = g_getenv ("UDI");
        if (udi == NULL) {
		g_free (xml);
                libhal_ctx_free (ctx);
                itdb_sysinfo_properties_free (props);
                return FALSE;
        }

        hal_ipod_set_properties (ctx, udi, props);
        itdb_sysinfo_properties_free (props);

        ipod_mountpoint = mount_ipod (g_getenv ("HAL_PROP_BLOCK_DEVICE"));
        if (ipod_mountpoint == NULL) {
                g_free (xml);
                libhal_ctx_free (ctx);
                return -1;
        }
        write_sysinfo_extended (ipod_mountpoint, xml); 
        g_free (xml);

        /* hal_mounted_ipod_set_properties will call itdb_parse on the ipod
         * which we just mounted, which will create an ItdbDevice
         * containing most of what 'dev' had above. For now, I'm leaving
         * this kind of duplication since I want the hal information to be
         * added even if for some reason we don't manage to mount the ipod
         */
        hal_mounted_ipod_set_properties (ctx, udi, ipod_mountpoint);
        libhal_ctx_free (ctx);

        umount (ipod_mountpoint);
        g_rmdir (ipod_mountpoint);
        g_free (ipod_mountpoint);

        return 0;
}

/* Copyright (c) 2007, 2010 Christophe Fergeau  <teuf@gnome.org>
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
#include "config.h"

#include "backends.h"

#include <errno.h>
#include <glib.h>
#include <glib/gstdio.h>

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
#ifdef HAVE_LIBUSB
extern char *read_sysinfo_extended_from_usb (guint bus_number, guint device_address);
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
	case ITDB_IPOD_GENERATION_NANO_6:
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
	case ITDB_IPOD_GENERATION_TOUCH_3:
	case ITDB_IPOD_GENERATION_TOUCH_4:
		return g_strdup ("touch");
	case ITDB_IPOD_GENERATION_IPHONE_1:
	case ITDB_IPOD_GENERATION_IPHONE_2:
	case ITDB_IPOD_GENERATION_IPHONE_3:
	case ITDB_IPOD_GENERATION_IPHONE_4:
		return g_strdup ("phone");
	case ITDB_IPOD_GENERATION_IPAD_1:
		return g_strdup ("ipad");
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
	case ITDB_IPOD_GENERATION_NANO_6:
		return 6.0;
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
	case ITDB_IPOD_GENERATION_TOUCH_3:
		return 3.0;
	case ITDB_IPOD_GENERATION_TOUCH_4:
		return 4.0;
	case ITDB_IPOD_GENERATION_IPHONE_1:
		return 1.0;
	case ITDB_IPOD_GENERATION_IPHONE_2:
		return 2.0;
	case ITDB_IPOD_GENERATION_IPHONE_3:
		return 3.0;
	case ITDB_IPOD_GENERATION_IPHONE_4:
		return 4.0;
	case ITDB_IPOD_GENERATION_IPAD_1:
		return 1.0;
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
	case ITDB_IPOD_MODEL_IPAD:
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
	case ITDB_IPOD_MODEL_SHUFFLE_GOLD:
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
	case ITDB_IPOD_MODEL_SHUFFLE_STAINLESS:
		return g_strdup ("stainless");
	}

	g_assert_not_reached ();
}

static char *
get_icon_name (const Itdb_IpodInfo *info)
{
	const char prefix[] = "multimedia-player-apple-";
	if (info == NULL) {
		return g_strconcat (prefix, "ipod", NULL);
	}
	switch (info->ipod_generation) {
	case ITDB_IPOD_GENERATION_UNKNOWN:
	case ITDB_IPOD_GENERATION_FIRST:
	case ITDB_IPOD_GENERATION_SECOND:
	case ITDB_IPOD_GENERATION_THIRD:
	case ITDB_IPOD_GENERATION_FOURTH:
		return g_strconcat (prefix, "ipod", NULL);

	case ITDB_IPOD_GENERATION_PHOTO:
		return g_strconcat (prefix, "ipod-color", NULL);

	case ITDB_IPOD_GENERATION_MINI_1:
	case ITDB_IPOD_GENERATION_MINI_2:
		switch (info->ipod_model) {
		case ITDB_IPOD_MODEL_MINI_BLUE:
			return g_strconcat (prefix, "ipod-mini-blue", NULL);
		case ITDB_IPOD_MODEL_MINI_PINK:
			return g_strconcat (prefix, "ipod-mini-pink", NULL);
		case ITDB_IPOD_MODEL_MINI_GOLD:
			return g_strconcat (prefix, "ipod-mini-gold", NULL);
		case ITDB_IPOD_MODEL_MINI_GREEN:
			return g_strconcat (prefix, "ipod-mini-green", NULL);
		case ITDB_IPOD_MODEL_MINI:
			return g_strconcat (prefix, "ipod-mini-silver", NULL);
		default:
			g_assert_not_reached ();
		}

	case ITDB_IPOD_GENERATION_SHUFFLE_1:
		return g_strconcat (prefix, "ipod-shuffle", NULL);

	case ITDB_IPOD_GENERATION_SHUFFLE_2:
	case ITDB_IPOD_GENERATION_SHUFFLE_3:
		switch (info->ipod_model) {
		case ITDB_IPOD_MODEL_SHUFFLE_SILVER:
			return g_strconcat (prefix, "ipod-shuffle-clip-silver", NULL);
		case ITDB_IPOD_MODEL_SHUFFLE_GREEN:
			return g_strconcat (prefix, "ipod-shuffle-clip-green", NULL);
		case ITDB_IPOD_MODEL_SHUFFLE_GOLD:
			return g_strconcat (prefix, "ipod-shuffle-clip-gold", NULL);
		case ITDB_IPOD_MODEL_SHUFFLE_ORANGE:
			return g_strconcat (prefix, "ipod-shuffle-clip-orange", NULL);
		case ITDB_IPOD_MODEL_SHUFFLE_PURPLE:
			return g_strconcat (prefix, "ipod-shuffle-clip-purple", NULL);
		case ITDB_IPOD_MODEL_SHUFFLE_PINK:
			return g_strconcat (prefix, "ipod-shuffle-clip-pink", NULL);
		case ITDB_IPOD_MODEL_SHUFFLE_BLUE:
			return g_strconcat (prefix, "ipod-shuffle-clip-blue", NULL);
		default:
			g_assert_not_reached ();
		}

	case ITDB_IPOD_GENERATION_SHUFFLE_4:
		return g_strconcat (prefix, "ipod-shuffle", NULL);

	case ITDB_IPOD_GENERATION_NANO_1:
		if (info->ipod_model == ITDB_IPOD_MODEL_NANO_BLACK) {
			return g_strconcat (prefix, "ipod-nano-black", NULL);
		} else {
			return g_strconcat (prefix, "ipod-nano-white", NULL);
		}

	case ITDB_IPOD_GENERATION_NANO_2:
		return g_strconcat (prefix, "ipod-nano-white", NULL);

	case ITDB_IPOD_GENERATION_NANO_3:
		switch (info->ipod_model) {
		case ITDB_IPOD_MODEL_NANO_SILVER:
			return g_strconcat (prefix, "ipod-nano-video", NULL);
		case ITDB_IPOD_MODEL_NANO_BLACK:
			return g_strconcat (prefix, "ipod-nano-video-black", NULL);
		case ITDB_IPOD_MODEL_NANO_BLUE:
			return g_strconcat (prefix, "ipod-nano-video-turquoise", NULL);
		case ITDB_IPOD_MODEL_NANO_GREEN:
			return g_strconcat (prefix, "ipod-nano-video-green", NULL);
		case ITDB_IPOD_MODEL_NANO_RED:
			return g_strconcat (prefix, "ipod-nano-video-red", NULL);
		default:
			g_assert_not_reached ();
		}

	case ITDB_IPOD_GENERATION_NANO_4:
	case ITDB_IPOD_GENERATION_NANO_5:
	case ITDB_IPOD_GENERATION_NANO_6:
		/* FIXME: set the correct icon name once it's added to
		 * gnome-icon-theme-extras 
		 */
		return g_strconcat (prefix, "ipod-nano-white", NULL);

	case ITDB_IPOD_GENERATION_VIDEO_1:
	case ITDB_IPOD_GENERATION_VIDEO_2:
		if (info->ipod_model == ITDB_IPOD_MODEL_VIDEO_BLACK) {
			return g_strconcat (prefix, "ipod-video-black", NULL);
		} else {
			return g_strconcat (prefix, "ipod-video-white", NULL);
		}

	case ITDB_IPOD_GENERATION_CLASSIC_1:
	case ITDB_IPOD_GENERATION_CLASSIC_2:
	case ITDB_IPOD_GENERATION_CLASSIC_3:
		if (info->ipod_model == ITDB_IPOD_MODEL_CLASSIC_BLACK) {
			return g_strconcat (prefix, "ipod-classic-black", NULL);
		} else {
			return g_strconcat (prefix, "ipod-classic-white", NULL);
		}

	case ITDB_IPOD_GENERATION_TOUCH_1:
		return g_strconcat (prefix, "ipod-touch", NULL);
	case ITDB_IPOD_GENERATION_TOUCH_2:
	case ITDB_IPOD_GENERATION_TOUCH_3:
	case ITDB_IPOD_GENERATION_TOUCH_4:
		return g_strconcat (prefix, "ipod-touch-2g", NULL);
	case ITDB_IPOD_GENERATION_IPHONE_1:
		return g_strdup ("phone-apple-iphone");
	case ITDB_IPOD_GENERATION_IPHONE_2:
		return g_strdup ("phone-apple-iphone-3g");
	case ITDB_IPOD_GENERATION_IPHONE_3:
		return g_strdup ("phone-apple-iphone-3gs");
	case ITDB_IPOD_GENERATION_IPHONE_4:
		return g_strdup ("phone-apple-iphone-4g");
	case ITDB_IPOD_GENERATION_IPAD_1:
		return g_strdup ("computer-apple-ipad");
	case ITDB_IPOD_GENERATION_MOBILE:
		return g_strconcat (prefix, "ipod", NULL);
	}

	g_assert_not_reached ();
}

static void set_artwork_information (const SysInfoIpodProperties *props,
				     ItdbBackend *backend)
{
        const GList *formats;

        /* Cover art */
        formats = itdb_sysinfo_properties_get_cover_art_formats (props);
        backend->set_artwork_formats (backend, ALBUM, formats);

        /* Photos */
        formats = itdb_sysinfo_properties_get_photo_formats (props);
        backend->set_artwork_formats (backend, PHOTO, formats);

        /* Chapter images */
        formats = itdb_sysinfo_properties_get_chapter_image_formats (props);
        backend->set_artwork_formats (backend, CHAPTER, formats);
}

static gboolean ipod_set_properties (ItdbBackend *backend,
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

	backend->set_version (backend, 1);

	serial_number = itdb_sysinfo_properties_get_serial_number (props);

	info = itdb_ipod_info_from_serial (serial_number);

	if ((info == NULL) || (info->ipod_generation == ITDB_IPOD_GENERATION_UNKNOWN)) {
		backend->set_is_unknown (backend, TRUE);
		return TRUE;
	} else {
		backend->set_is_unknown (backend, FALSE);
	}

	icon_name = get_icon_name (info);
	backend->set_icon_name (backend, icon_name);
	g_free (icon_name);

	if (itdb_sysinfo_properties_get_firewire_id (props) != NULL) {
		const char *fwid;
		fwid = itdb_sysinfo_properties_get_firewire_id (props);
		backend->set_firewire_id (backend, fwid);
	}

	if (serial_number != NULL) {
		backend->set_serial_number (backend, serial_number);
	}

	firmware_version = itdb_sysinfo_properties_get_firmware_version (props);
	if (firmware_version != NULL) {
		backend->set_firmware_version (backend, firmware_version);
	}

	set_artwork_information (props, backend);

	model_name = get_model_name (info);
	if (model_name != NULL) {
		backend->set_model_name (backend, model_name);
		g_free (model_name);
	}

	generation = get_generation (info);
	if (generation != 0.0) {
		backend->set_generation (backend, generation);
	}

	color_name = get_color_name (info);
	if (color_name != NULL) {
		backend->set_color (backend, color_name);
		g_free (color_name);
	}

	if (serial_number != NULL) {
		prod_info = parse_serial_number (serial_number);
		if (prod_info != NULL) {
			if (prod_info->factory_id != NULL) {
				backend->set_factory_id (backend,
							prod_info->factory_id);

			}
			if (prod_info->production_year != 0) {
				backend->set_production_year (backend,
							      prod_info->production_year);

			}
			if (prod_info->production_week != 0) {
				backend->set_production_week (backend,
							      prod_info->production_week);

			}
			if (prod_info->production_index != 0) {
				backend->set_production_index (backend,
							       prod_info->production_index);
			}
		}
		production_info_free (prod_info);
	}

	return TRUE;
}

static gboolean mounted_ipod_set_properties (ItdbBackend *backend,
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
			backend->set_control_path (backend,
						  control_path + strlen (ipod_mountpoint));
			g_free (control_path);
		}
        }

        mpl = itdb_playlist_mpl (itdb);
        if (mpl == NULL) {
                return FALSE;
        }
	if (mpl->name != NULL) {
	    backend->set_name (backend, mpl->name);
	}

        return FALSE;
}


static char *mount_ipod (const char *dev_path, const char *fstype)
{
        char *filename;
        char *tmpname;
        int result;

        filename = g_build_filename (TMPMOUNTDIR, "ipodXXXXXX", NULL);
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
        /* Make sure the device dir exists (not necessarily true on
         * Shuffles */
        if (devdirpath == NULL) {
            gchar *itunesdirpath;

            itunesdirpath = itdb_get_itunes_dir (mountpoint);
            if (itunesdirpath == NULL) {
                return FALSE;
            }
            devdirpath = g_build_filename (itunesdirpath, "Device", NULL);
            g_free (itunesdirpath);
            g_mkdir (devdirpath, 0777);
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


static char *get_info_from_usb (usb_bus_number, usb_device_number)
{
#ifdef HAVE_LIBUSB
        return read_sysinfo_extended_from_usb (usb_bus_number,
                                               usb_device_number);
#else
	return NULL;
#endif
}

static char *get_info_from_sg (const char *dev)
{
#ifdef HAVE_SGUTILS
	return read_sysinfo_extended (dev);
#else
	return NULL;
#endif
}

int itdb_callout_set_ipod_properties (ItdbBackend *backend, const char *dev,
                                      gint usb_bus_number,
                                      gint usb_device_number,
				      const char *fstype)
{
	char *ipod_mountpoint = NULL;
	char *xml = NULL;
	SysInfoIpodProperties *props;

	if (usb_bus_number != 0) {
		xml = get_info_from_usb (usb_bus_number, usb_device_number);
        }
        if (xml == NULL) {
		xml = get_info_from_sg (dev);
	}

        if (xml == NULL) {
                return -1;
        }
        props = itdb_sysinfo_extended_parse_from_xml (xml, NULL);

        ipod_set_properties (backend, props);
        itdb_sysinfo_properties_free (props);

        ipod_mountpoint = mount_ipod (dev, fstype);
        if (ipod_mountpoint == NULL) {
                g_free (xml);
                return -1;
        }
        write_sysinfo_extended (ipod_mountpoint, xml); 
        g_free (xml);

        /* mounted_ipod_set_properties will call itdb_parse on the ipod
         * which we just mounted, which will create an ItdbDevice
         * containing most of what 'dev' had above. For now, I'm leaving
         * this kind of duplication since I want the hal information to be
         * added even if for some reason we don't manage to mount the ipod
         */
        mounted_ipod_set_properties (backend, ipod_mountpoint);

        umount (ipod_mountpoint);
        g_rmdir (ipod_mountpoint);
        g_free (ipod_mountpoint);

        return 0;
}

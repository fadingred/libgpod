/* Copyright (c) 2010, Christophe Fergeau  <teuf@gnome.org>
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

#include <stdlib.h>
#include <itdb_device.h>
#include <glib-object.h>

#define LIBGPOD_UDEV_NS "LIBGPOD_"

struct _UdevBackend {
	ItdbBackend parent;
};
typedef struct _UdevBackend UdevBackend;


static void udev_backend_destroy (ItdbBackend *itdb_backend)
{
	UdevBackend *backend = (UdevBackend *)itdb_backend;
	g_free (backend);
}

static gboolean
udev_backend_set_version (ItdbBackend *itdb_backend, unsigned int version)
{
#ifdef UPODSLEUTH
	g_print (LIBGPOD_UDEV_NS"VERSION=%u\n", version);
#endif
	return TRUE;
}

static gboolean
udev_backend_set_is_unknown (ItdbBackend *itdb_backend, gboolean unknown)
{
#ifdef UPODSLEUTH
	g_print (LIBGPOD_UDEV_NS"IS_UNKNOWN=%u\n", !!unknown);
#endif
	return TRUE;
}

static gboolean
udev_backend_set_icon_name (ItdbBackend *itdb_backend, const char *icon_name)
{
	g_print ("UDISKS_PRESENTATION_ICON_NAME=%s\n", icon_name);
	return TRUE;
}

static gboolean
udev_backend_set_firewire_id (ItdbBackend *itdb_backend, const char *fwid)
{
#ifdef UPODSLEUTH
	g_print (LIBGPOD_UDEV_NS"FIREWIRE_ID=%s\n", fwid);
#endif
	return TRUE;
}

static gboolean
udev_backend_set_serial_number (ItdbBackend *itdb_backend, const char *serial_number)
{
#ifdef UPODSLEUTH
	g_print (LIBGPOD_UDEV_NS"SERIAL_NUMBER=%s\n", serial_number);
#endif
	return TRUE;
}

static gboolean udev_backend_set_firmware_version (ItdbBackend *itdb_backend,
						  const char *firmware_version)
{
#ifdef UPODSLEUTH
	g_print (LIBGPOD_UDEV_NS"FIRMWARE_VERSION=%s\n", firmware_version);
#endif
	return TRUE;
}

static gboolean
udev_backend_set_model_name (ItdbBackend *itdb_backend, const char *model_name)
{
#ifdef UPODSLEUTH
	g_print (LIBGPOD_UDEV_NS"DEVICE_CLASS=%s\n", model_name);
#endif
	return TRUE;
}

static gboolean
udev_backend_set_generation (ItdbBackend *itdb_backend, gdouble generation)
{
#ifdef UPODSLEUTH
	g_print (LIBGPOD_UDEV_NS"MODEL_GENERATION=%f\n", generation);
#endif
	return TRUE;
}

static gboolean
udev_backend_set_color (ItdbBackend *itdb_backend, const char *color_name)
{
#ifdef UPODSLEUTH
	g_print (LIBGPOD_UDEV_NS"MODEL_SHELL_COLOR=%s\n", color_name);
#endif
	return TRUE;
}

static gboolean
udev_backend_set_factory_id (ItdbBackend *itdb_backend,
			    const char *factory_id)
{
#ifdef UPODSLEUTH
	g_print (LIBGPOD_UDEV_NS"PRODUCTION_FACTORY_ID=%s\n", factory_id);
#endif
	return TRUE;
}

static gboolean
udev_backend_set_production_year (ItdbBackend *itdb_backend, guint year)
{
#ifdef UPODSLEUTH
	g_print (LIBGPOD_UDEV_NS"PRODUCTION_YEAR=%u\n", year);
#endif
	return TRUE;
}

static gboolean 
udev_backend_set_production_week (ItdbBackend *itdb_backend, guint week)
{
#ifdef UPODSLEUTH
	g_print (LIBGPOD_UDEV_NS"PRODUCTION_WEEK=%u\n", week);
#endif
	return TRUE;
}

static gboolean
udev_backend_set_production_index (ItdbBackend *itdb_backend, guint index)
{
#ifdef UPODSLEUTH
	g_print (LIBGPOD_UDEV_NS"PRODUCTION_INDEX=%u\n", index);
#endif
	return TRUE;
}

static gboolean
udev_backend_set_control_path (ItdbBackend *itdb_backend, const char *control_path)
{
#ifdef UPODSLEUTH
	g_print (LIBGPOD_UDEV_NS"MODEL_CONTROL_PATH=%s\n", control_path);
#endif
	return TRUE;
}

static gboolean
udev_backend_set_name (ItdbBackend *itdb_backend, const char *name)
{
	g_print ("UDISKS_PRESENTATION_NAME=%s\n", name);
	return TRUE;
}

#ifdef UPODSLEUTH
static gboolean udev_backend_set_artwork_type_supported (ItdbBackend *itdb_backend,
							enum ArtworkType type,
							gboolean supported)
{
    	char *propname;
	switch (type) {
	    case ALBUM:
		propname = LIBGPOD_UDEV_NS"IMAGES_ALBUM_ART_SUPPORTED=%d\n";
		break;
	    case PHOTO:
		propname = LIBGPOD_UDEV_NS"IMAGES_PHOTOS_SUPPORTED=%d\n";
		break;
	    case CHAPTER:
		propname = LIBGPOD_UDEV_NS"IMAGES_CHAPTER_IMAGES_SUPPORTED=%d\n";
		break;
	    default:
		g_return_val_if_reached (FALSE);
	}
	g_print (propname, !!supported);
	return TRUE;
}
#endif

static gboolean udev_backend_set_artwork_formats (ItdbBackend *itdb_backend,
						 enum ArtworkType type,
						 const GList *formats)
{
#ifdef UPODSLEUTH
	udev_backend_set_artwork_type_supported (itdb_backend, type,
					       	(formats != NULL));
#endif

	return TRUE;
}

static ItdbBackend *udev_backend_new (void) 
{
	UdevBackend *backend;

	backend = g_new0 (UdevBackend, 1);

	backend->parent.destroy = udev_backend_destroy;
	backend->parent.set_version = udev_backend_set_version;
	backend->parent.set_is_unknown = udev_backend_set_is_unknown;
	backend->parent.set_icon_name = udev_backend_set_icon_name;
	backend->parent.set_firewire_id = udev_backend_set_firewire_id;
	backend->parent.set_serial_number = udev_backend_set_serial_number;
	backend->parent.set_firmware_version = udev_backend_set_firmware_version;
	backend->parent.set_model_name = udev_backend_set_model_name;
	backend->parent.set_generation = udev_backend_set_generation;
	backend->parent.set_color = udev_backend_set_color;
	backend->parent.set_factory_id = udev_backend_set_factory_id;
	backend->parent.set_production_year = udev_backend_set_production_year;
	backend->parent.set_production_week = udev_backend_set_production_week;
	backend->parent.set_production_index = udev_backend_set_production_index;
	backend->parent.set_control_path = udev_backend_set_control_path;
	backend->parent.set_name = udev_backend_set_name;
	backend->parent.set_artwork_formats = udev_backend_set_artwork_formats;

	return (ItdbBackend *)backend;
}

int main (int argc, char **argv)
{
	ItdbBackend *backend;
	int result;
	const char *fstype;
        gint usb_bus_number;
        gint usb_dev_number;

	if (argc != 4) {
		return -1;
	}
	g_type_init ();

	backend = udev_backend_new ();
        if (backend == NULL) {
                return -1;
        }

        fstype = g_getenv ("ID_FS_TYPE");
        if (fstype == NULL) {
                return -1;
        }

        usb_bus_number = atoi (argv[2]);
        usb_dev_number = atoi (argv[3]);
	
	result = itdb_callout_set_ipod_properties (backend, argv[1],
                                                   usb_bus_number,
                                                   usb_dev_number,
                                                   fstype);
	backend->destroy (backend);

	return result;
}

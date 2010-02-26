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

#include <libhal.h>
#include <itdb_device.h>
#include <glib-object.h>
#include <string.h>

#define LIBGPOD_HAL_NS "org.libgpod."

struct _HalBackend {
	ItdbBackend parent;
	LibHalContext *ctx;
	char *udi;
};
typedef struct _HalBackend HalBackend;


static void hal_backend_destroy (ItdbBackend *itdb_backend)
{
	HalBackend *backend = (HalBackend *)itdb_backend;
	g_free (backend->udi);
	libhal_ctx_free(backend->ctx);
	g_free (backend);
}

static gboolean
hal_backend_set_version (ItdbBackend *itdb_backend, unsigned int version)
{
	HalBackend *backend = (HalBackend *)itdb_backend;
        libhal_device_set_property_int (backend->ctx, backend->udi, 
			                LIBGPOD_HAL_NS"version", 1, NULL);
	return TRUE;
}

static gboolean
hal_backend_set_is_unknown (ItdbBackend *itdb_backend, gboolean unknown)
{
	HalBackend *backend = (HalBackend *)itdb_backend;
	libhal_device_set_property_bool (backend->ctx, backend->udi,
					 LIBGPOD_HAL_NS"ipod.is_unknown", 
					 TRUE, NULL);
	return TRUE;
}

static gboolean
hal_backend_set_icon_name (ItdbBackend *itdb_backend, const char *icon_name)
{
	HalBackend *backend = (HalBackend *)itdb_backend;
	libhal_device_set_property_string (backend->ctx, backend->udi,
					   "info.desktop.icon",
					   icon_name, NULL);

	return TRUE;
}

static gboolean
hal_backend_set_firewire_id (ItdbBackend *itdb_backend, const char *fwid)
{
	HalBackend *backend = (HalBackend *)itdb_backend;
     	libhal_device_set_property_string (backend->ctx, backend->udi,
					   LIBGPOD_HAL_NS"ipod.firewire_id",
					   fwid, NULL);
	return TRUE;
}

static gboolean
hal_backend_set_serial_number (ItdbBackend *itdb_backend, const char *serial_number)
{
	HalBackend *backend = (HalBackend *)itdb_backend;
    	libhal_device_set_property_string (backend->ctx, backend->udi,
					   LIBGPOD_HAL_NS"ipod.serial_number",
					   serial_number,
					   NULL);
	return TRUE;
}

static gboolean hal_backend_set_firmware_version (ItdbBackend *itdb_backend,
						  const char *firmware_version)
{
	HalBackend *backend = (HalBackend *)itdb_backend;
    	libhal_device_set_property_string (backend->ctx, backend->udi,
					   LIBGPOD_HAL_NS"ipod.firmware_version",
					   firmware_version,
					   NULL);
	return TRUE;
}

static gboolean
hal_backend_set_model_name (ItdbBackend *itdb_backend, const char *model_name)
{
	HalBackend *backend = (HalBackend *)itdb_backend;
    	libhal_device_set_property_string (backend->ctx, backend->udi,
					   LIBGPOD_HAL_NS"ipod.model.device_class",
					   model_name,
					   NULL);
	return TRUE;
}

static gboolean
hal_backend_set_generation (ItdbBackend *itdb_backend, gdouble generation)
{
	HalBackend *backend = (HalBackend *)itdb_backend;
    	libhal_device_set_property_double (backend->ctx, backend->udi,
					   LIBGPOD_HAL_NS"ipod.model.generation",
					   generation,
					   NULL);
	return TRUE;
}

static gboolean
hal_backend_set_color (ItdbBackend *itdb_backend, const char *color_name)
{
	HalBackend *backend = (HalBackend *)itdb_backend;
    	libhal_device_set_property_string (backend->ctx, backend->udi,
					   LIBGPOD_HAL_NS"ipod.model.shell_color",
					   color_name,
					   NULL);
	return TRUE;
}

static gboolean
hal_backend_set_factory_id (ItdbBackend *itdb_backend,
			    const char *factory_id)
{
	HalBackend *backend = (HalBackend *)itdb_backend;
    	libhal_device_set_property_string (backend->ctx, backend->udi,
					   LIBGPOD_HAL_NS"ipod.production.factory_id",
					   factory_id,
					   NULL);
	return TRUE;
}

static gboolean
hal_backend_set_production_year (ItdbBackend *itdb_backend, guint year)
{
	HalBackend *backend = (HalBackend *)itdb_backend;
    	libhal_device_set_property_int (backend->ctx, backend->udi, 
					LIBGPOD_HAL_NS"ipod.production.year",
					year, NULL);
	return TRUE;
}

static gboolean 
hal_backend_set_production_week (ItdbBackend *itdb_backend, guint week)
{
	HalBackend *backend = (HalBackend *)itdb_backend;
    	libhal_device_set_property_int (backend->ctx, backend->udi, 
					LIBGPOD_HAL_NS"ipod.production.week",
					week, NULL);
	return TRUE;
}

static gboolean
hal_backend_set_production_index (ItdbBackend *itdb_backend, guint index)
{
	HalBackend *backend = (HalBackend *)itdb_backend;
    	libhal_device_set_property_int (backend->ctx, backend->udi, 
					LIBGPOD_HAL_NS"ipod.production.number",
					index, NULL);
	return TRUE;
}

static gboolean
hal_backend_set_control_path (ItdbBackend *itdb_backend, const char *control_path)
{
	HalBackend *backend = (HalBackend *)itdb_backend;
    	libhal_device_set_property_string (backend->ctx, backend->udi,
					   LIBGPOD_HAL_NS"ipod.model.control_path",
					   control_path, NULL);
	return TRUE;
}

static gboolean
hal_backend_set_name (ItdbBackend *itdb_backend, const char *name)
{
	HalBackend *backend = (HalBackend *)itdb_backend;
    	libhal_device_set_property_string (backend->ctx, backend->udi,
					   "info.desktop.name",
					   name, NULL);
	return TRUE;
}

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

static gboolean hal_backend_set_artwork_type_supported (ItdbBackend *itdb_backend,
							enum ArtworkType type,
							gboolean supported)
{
	HalBackend *backend = (HalBackend *)itdb_backend;
    	char *propname;
	switch (type) {
	    case ALBUM:
		propname = LIBGPOD_HAL_NS"ipod.images.album_art_supported";
		break;
	    case PHOTO:
		propname = LIBGPOD_HAL_NS"ipod.images.photos_supported";
		break;
	    case CHAPTER:
		propname = LIBGPOD_HAL_NS"ipod.images.chapter_images_supported";
		break;
	    default:
		g_return_val_if_reached (FALSE);
	}
        libhal_device_set_property_bool (backend->ctx, backend->udi,
                                         propname, supported, NULL);
	return TRUE;
}

static gboolean hal_backend_set_artwork_formats (ItdbBackend *itdb_backend,
						 enum ArtworkType type,
						 const GList *formats)
{
	HalBackend *backend = (HalBackend *)itdb_backend;

	const GList *it;
	hal_backend_set_artwork_type_supported (itdb_backend, type,
					       	(formats != NULL));

        for (it = formats; it != NULL; it = it->next) {
                char *format_str;

                format_str = get_format_string (it->data, type);
                libhal_device_property_strlist_append (backend->ctx, backend->udi,
                                                       LIBGPOD_HAL_NS"ipod.images.formats",
                                                       format_str, 
                                                       NULL);
                g_free (format_str);
        }

	return TRUE;
}

/* taken from libipoddevice proper */
static ItdbBackend *hal_backend_new (void) 
{
	LibHalContext *hal_context;
	HalBackend *backend;
	const char *udi;
	DBusError error;
	DBusConnection *dbus_connection;

        udi = g_getenv ("UDI");
	if (udi == NULL) {
	    	return NULL;
	}

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

	backend = g_new0 (HalBackend, 1);
	backend->ctx = hal_context;
	backend->udi = g_strdup (udi);

	backend->parent.destroy = hal_backend_destroy;
	backend->parent.set_version = hal_backend_set_version;
	backend->parent.set_is_unknown = hal_backend_set_is_unknown;
	backend->parent.set_icon_name = hal_backend_set_icon_name;
	backend->parent.set_firewire_id = hal_backend_set_firewire_id;
	backend->parent.set_serial_number = hal_backend_set_serial_number;
	backend->parent.set_firmware_version = hal_backend_set_firmware_version;
	backend->parent.set_model_name = hal_backend_set_model_name;
	backend->parent.set_generation = hal_backend_set_generation;
	backend->parent.set_color = hal_backend_set_color;
	backend->parent.set_factory_id = hal_backend_set_factory_id;
	backend->parent.set_production_year = hal_backend_set_production_year;
	backend->parent.set_production_week = hal_backend_set_production_week;
	backend->parent.set_production_index = hal_backend_set_production_index;
	backend->parent.set_control_path = hal_backend_set_control_path;
	backend->parent.set_name = hal_backend_set_name;
	backend->parent.set_artwork_formats = hal_backend_set_artwork_formats;

	return (ItdbBackend *)backend;
}

#ifdef HAVE_LIBUSB
static gboolean hal_get_ipod_usb_position (LibHalContext *ctx, const char *udi,
                                           int *usb_bus_number, int *usb_device_number)
{
	char *parent_udi;
	char *subsystem;
	gboolean found_ids;
	DBusError error;


	parent_udi = NULL;
	subsystem = NULL;
	found_ids = FALSE;
	dbus_error_init (&error);
	while (TRUE) {
		parent_udi = libhal_device_get_property_string (ctx, udi,
				"info.parent", &error);
		if (parent_udi == NULL || dbus_error_is_set (&error))
			goto end;
		udi = parent_udi;
		subsystem = libhal_device_get_property_string (ctx, udi,
							       "linux.subsystem",
							       &error);
		if (subsystem == NULL || dbus_error_is_set (&error)) {
			dbus_error_free (&error);
			dbus_error_init (&error);
			continue;
		}
		if (strcmp (subsystem, "usb") == 0) {
			*usb_bus_number = libhal_device_get_property_int (ctx, udi,
									  "usb.bus_number", &error);
			if (dbus_error_is_set (&error)) {
				goto end;
			}
			*usb_device_number = libhal_device_get_property_int (ctx, udi,
									     "usb.linux.device_number", &error);
			if (dbus_error_is_set (&error)) {
				goto end;
			}
			found_ids = TRUE;
			goto end;
		}
	}
end:
	libhal_free_string (parent_udi);
	libhal_free_string (subsystem);
	if (dbus_error_is_set (&error)) {
	    g_print ("Error: %s\n", error.message);
	    dbus_error_free (&error);
	}

	return found_ids;
}
#endif

int main (int argc, char **argv)
{
	ItdbBackend *backend;
	int result;
	const char *dev;
	const char *fstype;
        gint usb_bus_number = 0;
        gint usb_device_number = 0;
        HalBackend *hal_backend;

	g_type_init ();

	backend = hal_backend_new ();
        if (backend == NULL) {
                return -1;
        }

	dev = g_getenv ("HAL_PROP_BLOCK_DEVICE");
	if (dev == NULL) {
		return -1;
	}

        fstype = g_getenv ("HAL_PROP_VOLUME_FSTYPE");
        if (fstype == NULL) {
                return -1;
        }

        hal_backend = (HalBackend *)backend;
#ifdef HAVE_LIBUSB
        hal_get_ipod_usb_position (hal_backend->ctx, hal_backend->udi,
                                   &usb_bus_number, &usb_device_number);
#endif

	result = itdb_callout_set_ipod_properties (backend, dev,
                                                   usb_bus_number,
                                                   usb_device_number,
                                                   fstype);
	backend->destroy (backend);

	return result;
}

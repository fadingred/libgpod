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
#ifndef LIBGPOD_BACKEND_H
#define LIBGPOD_BACKEND_H

#include <glib.h>

G_BEGIN_DECLS

enum ArtworkType {
	UNKNOWN,
	PHOTO,
	ALBUM,
	CHAPTER
};

typedef struct _ItdbBackend ItdbBackend;
struct _ItdbBackend {
	void (*destroy) (ItdbBackend *backend);
	gboolean (*set_version) (ItdbBackend *backend, unsigned int version);
	gboolean (*set_is_unknown) (ItdbBackend *backend, gboolean unknown);
	gboolean (*set_icon_name) (ItdbBackend *backend, const char *icon_name);
	gboolean (*set_firewire_id) (ItdbBackend *backend, const char *fwid);
	gboolean (*set_serial_number) (ItdbBackend *backend, const char *serial_number);
	gboolean (*set_firmware_version) (ItdbBackend *backend, const char *firmware_version);
	gboolean (*set_model_name) (ItdbBackend *backend, const char *model_name);
	gboolean (*set_generation) (ItdbBackend *backend, gdouble generation);
	gboolean (*set_color) (ItdbBackend *backend, const char *color_name);
	gboolean (*set_factory_id) (ItdbBackend *backend, const char *factory_id);
	gboolean (*set_production_year) (ItdbBackend *backend, guint year);
	gboolean (*set_production_week) (ItdbBackend *backend, guint week);
	gboolean (*set_production_index) (ItdbBackend *backend, guint index);
	gboolean (*set_control_path) (ItdbBackend *backend, const char *control_path);
	gboolean (*set_name) (ItdbBackend *backend, const char *name);
	gboolean (*set_artwork_formats) (ItdbBackend *backend, enum ArtworkType type, const GList *formats);
};

int itdb_callout_set_ipod_properties (ItdbBackend *backend, const char *dev,
                                      gint usb_bus_number,
                                      gint usb_device_number,
				      const char *fstype);

G_END_DECLS

#endif

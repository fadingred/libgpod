/*
 *  Copyright (C) 2005 Christophe Fergeau
 *
 * 
 *  The code contained in this file is free software; you can redistribute
 *  it and/or modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either version
 *  2.1 of the License, or (at your option) any later version.
 *  
 *  This file is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *  
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this code; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 *  iTunes and iPod are trademarks of Apple
 * 
 *  This product is not supported/written/published by Apple!
 *
 */
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>

#include <glib.h>
#include <glib-object.h>

#include "itdb_device.h"
#include "itdb_endianness.h"
#include "db-image-parser.h"
#include <glib/gi18n-lib.h>

static const Itdb_ArtworkFormat *
find_format (GList *formats, gint16 format_id)
{
        GList *it;

        for (it = formats; it != NULL; it = it->next) {
                const Itdb_ArtworkFormat *format;
                format = (const Itdb_ArtworkFormat *)it->data;
                if (format->format_id == format_id) {
                        return format;
                }
        }

        return NULL;
}

static const Itdb_ArtworkFormat * 
image_format_from_id (Itdb_Device *device, gint16 format_id)
{
        GList *formats;
        const Itdb_ArtworkFormat *format;

	if (device == NULL) {
		return NULL;
	}

	formats = itdb_device_get_cover_art_formats (device);
        format = find_format (formats, format_id);
        g_list_free (formats);
        if (format != NULL) {
                return format;
        }

	formats = itdb_device_get_photo_formats (device);
        format = find_format (formats, format_id);
        g_list_free (formats);
	return format;
}

G_GNUC_INTERNAL Itdb_Thumb_Ipod_Item *
ipod_image_new_from_mhni (MhniHeader *mhni, Itdb_DB *db)
{
	Itdb_Thumb_Ipod_Item *img;
        const Itdb_ArtworkFormat *format;
	gint16 format_id;
	Itdb_Device *device = NULL;

	device = db_get_device (db);
	g_return_val_if_fail (device, NULL);

	format_id = get_gint32_db (db, mhni->format_id);
        format = image_format_from_id (device, format_id);
        if (format == NULL) {
            g_warning (_("Unexpected image type in mhni: %d, offset: %d\n"), 
                       format_id, get_guint32_db (db, mhni->ithmb_offset));
            return NULL;
        }

        img = itdb_thumb_new_item_from_ipod (format);
	if (img == NULL) {
		return NULL;
	}
	img->size   = get_guint32_db (db, mhni->image_size);
	img->offset = get_guint32_db (db, mhni->ithmb_offset);
	img->width  = get_gint16_db (db, mhni->image_width);
	img->height = get_gint16_db (db, mhni->image_height);
	img->horizontal_padding  =
	    get_gint16_db (db, mhni->horizontal_padding);
	img->vertical_padding =
	    get_gint16_db (db, mhni->vertical_padding);

#if DEBUG_ARTWORK
	printf ("format_id: %d, of: %6d sz: %6d, x: %3d, y: %3d, xpad: %3d, ypad: %3d\n",
		format_id, img->offset, img->size, img->width, img->height, img->horizontal_padding, img->vertical_padding);
#endif

	return img;
}

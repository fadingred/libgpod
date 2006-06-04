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
#include "db-artwork-parser.h"
#include "db-image-parser.h"
#include <glib/gi18n-lib.h>

static int
image_type_from_corr_id (Itdb_Device *device, gint16 corr_id)
{
	const Itdb_ArtworkFormat *formats;

	if (device == NULL) {
		return -1;
	}

	formats = itdb_device_get_artwork_formats (device);

	if (formats == NULL) {
		return -1;
	}
	
	while (formats->type != -1) {
		if (formats->correlation_id == corr_id) {
			return formats->type;
		}
		formats++;
	}

	return -1;
}


G_GNUC_INTERNAL const Itdb_ArtworkFormat *
itdb_get_artwork_info_from_type (Itdb_Device *device,
				 ItdbThumbType image_type)
{
	const Itdb_ArtworkFormat *formats;
	
	if (device == NULL) {
		return NULL;
	}

	formats = itdb_device_get_artwork_formats (device);
	if (formats == NULL) {
		return NULL;
	}
	
	while ((formats->type != -1) && (formats->type != image_type)) {
		formats++;
	}

	if (formats->type == -1) {
		return NULL;
	}

	return formats;
}

G_GNUC_INTERNAL Itdb_Thumb *
ipod_image_new_from_mhni (MhniHeader *mhni, Itdb_DB *db)
{
	Itdb_Thumb *img;
	gint16 corr_id;
	Itdb_Device *device = NULL;

	img = g_new0 (Itdb_Thumb, 1);
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

	device = db_get_device (db);
	g_return_val_if_fail (device, NULL);

	corr_id = get_gint32_db (db, mhni->correlation_id);
	img->type = image_type_from_corr_id (device, corr_id);

	if (img->type == -1)
	{
	    g_warning (_("Unexpected image type in mhni: size: %ux%u (%d), offset: %d\n"), 
		       img->width, img->height, 
		       corr_id, img->offset);
	    g_free (img);
	    return NULL;
	}

	return img;
}

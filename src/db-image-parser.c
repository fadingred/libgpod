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

#include "db-artwork-parser.h"
#include "db-image-parser.h"
#include <glib/gi18n-lib.h>

static int
image_type_from_corr_id (IpodDevice *ipod, int corr_id)
{
	const IpodArtworkFormat *formats;

	if (ipod == NULL) {
		return -1;
	}

	g_object_get (G_OBJECT (ipod), "artwork-formats", &formats, NULL);
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


G_GNUC_INTERNAL const IpodArtworkFormat *
ipod_get_artwork_info_from_type (IpodDevice *ipod, int image_type)
{
	const IpodArtworkFormat *formats;
	
	if (ipod == NULL) {
		return NULL;
	}

	g_object_get (G_OBJECT (ipod), "artwork-formats", &formats, NULL);
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
ipod_image_new_from_mhni (MhniHeader *mhni, Itdb_iTunesDB *db)
{

	Itdb_Thumb *img;
	img = g_new0 (Itdb_Thumb, 1);
	if (img == NULL) {
		return NULL;
	}
	img->size   = GUINT32_FROM_LE (mhni->image_size);
	img->offset = GUINT32_FROM_LE (mhni->ithmb_offset);
	img->width  = GINT16_FROM_LE (mhni->image_width);
	img->height = GINT16_FROM_LE (mhni->image_height);

	img->type = image_type_from_corr_id (db->device, mhni->correlation_id);
	if ((img->type != IPOD_COVER_SMALL) && (img->type != IPOD_COVER_LARGE)) {
		g_warning ("Unexpected cover type in mhni: type %d, size: %ux%u (%d), offset: %d\n", 
			   img->type, img->width, img->height, mhni->correlation_id, img->offset);
		g_free (img);
		return NULL;
	}

	return img;
}

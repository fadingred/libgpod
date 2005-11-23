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

static unsigned char *
unpack_RGB_565 (gushort *pixels, unsigned int bytes_len)
{
	unsigned char *result;
	unsigned int i;

	result = g_malloc ((bytes_len/2) * 3);
	if (result == NULL) {
		return NULL;
	}
	for (i = 0; i < bytes_len/2; i++) {
		gushort cur_pixel;

		cur_pixel = GINT16_FROM_LE (pixels[i]);
		/* Unpack pixels */
		result[3*i] = (cur_pixel & RED_MASK) >> RED_SHIFT;
		result[3*i+1] = (cur_pixel & GREEN_MASK) >> GREEN_SHIFT;
		result[3*i+2] = (cur_pixel & BLUE_MASK) >> BLUE_SHIFT;
		
		/* Normalize color values so that they use a [0..255] range */
		result[3*i] <<= (8 - RED_BITS);
		result[3*i+1] <<= (8 - GREEN_BITS);
		result[3*i+2] <<= (8 - BLUE_BITS);
	}

	return result;
}


static unsigned char *
get_pixel_data (Itdb_Image *image)
{
	unsigned char *result;
	FILE *f;
	int res;

	f = NULL;
	result = g_malloc (image->size);
	if (result == NULL) {
		return NULL;
	}

	f = fopen (image->filename, "r");
	if (f == NULL) {
		g_print ("Failed to open %s: %s\n", 
			 image->filename, strerror (errno));
		goto end;
	}

	res = fseek (f, image->offset, SEEK_SET);
	if (res != 0) {
		g_print ("Seek to %d failed on %s: %s\n",
			 image->offset, image->filename, strerror (errno));
		goto end;
	}

	res = fread (result, image->size, 1, f);
	if (res != 1) {
		g_print ("Failed to read %u bytes from %s: %s\n", 
			 image->size, image->filename, strerror (errno));
		goto end;
	}
	fclose (f);

	return result;

 end:
	if (f != NULL) {
		fclose (f);
	}
	g_free (result);

	return NULL;
}

unsigned char *
itdb_image_get_rgb_data (Itdb_Image *image)
{
	void *pixels565;
	void *pixels;

	pixels565 = get_pixel_data (image);
	if (pixels565 == NULL) {
		return NULL;
	}
	
	pixels = unpack_RGB_565 (pixels565, image->size);
	g_free (pixels565);

	return pixels;

	/*	return  gdk_pixbuf_new_from_data (pixels, GDK_COLORSPACE_RGB, FALSE,
					  8, image->width, image->height, 
					  image->width * 3, 
					  (GdkPixbufDestroyNotify)g_free, 
					  NULL);
	*/
}

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

G_GNUC_INTERNAL Itdb_Image *
ipod_image_new_from_mhni (MhniHeader *mhni, Itdb_iTunesDB *db)
{

	Itdb_Image *img;
	img = g_new0 (Itdb_Image, 1);
	if (img == NULL) {
		return NULL;
	}
	img->size   = GUINT32_FROM_LE (mhni->image_size);
	img->offset = GUINT32_FROM_LE (mhni->ithmb_offset);
	img->width  = GINT16_FROM_LE (mhni->image_width);
	img->height = GINT16_FROM_LE (mhni->image_height);

	img->type = image_type_from_corr_id (db->device, mhni->correlation_id);
	if ((img->type != IPOD_COVER_SMALL) && (img->type != IPOD_COVER_LARGE)) {
		g_warning ("Unexpected cover type in mhni: %ux%u (%d)\n", 
			   img->width, img->height, mhni->correlation_id);
		g_free (img);
		return NULL;
	}

	return img;
}

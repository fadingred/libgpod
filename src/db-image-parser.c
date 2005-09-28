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
		result[3*i] = (pixels[i] & RED_MASK) >> RED_SHIFT;
		result[3*i+1] = (pixels[i] & GREEN_MASK) >> GREEN_SHIFT;
		result[3*i+2] = (pixels[i] & BLUE_MASK) >> BLUE_SHIFT;
		
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
		g_print ("Seek to %lu failed on %s: %s\n",
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

G_GNUC_INTERNAL char *
ipod_image_get_ithmb_filename (const char *mount_point, gint correlation_id) 
{
	char *paths[] = {"iPod_Control", "Artwork", NULL, NULL};
	char *filename;

	paths[2] = g_strdup_printf ("F%04u_1.ithmb", correlation_id);
	filename = itdb_resolve_path (mount_point, (const char **)paths);
	g_free (paths[2]);
	return filename;
}

G_GNUC_INTERNAL Itdb_Image *
ipod_image_new_from_mhni (MhniHeader *mhni, const char *mount_point)
{
	Itdb_Image *img;

	img = g_new0 (Itdb_Image, 1);
	if (img == NULL) {
		return NULL;
	}
	img->filename = ipod_image_get_ithmb_filename (mount_point,
						       GINT_FROM_LE (mhni->correlation_id));
	img->size = GINT_FROM_LE (mhni->image_size);
	img->offset = GINT_FROM_LE (mhni->ithmb_offset);
	img->width = (GINT_FROM_LE (mhni->image_dimensions) & 0xffff0000) >> 16;
	img->height = (GINT_FROM_LE (mhni->image_dimensions) & 0x0000ffff);

	switch (mhni->correlation_id) {
	case IPOD_THUMBNAIL_FULL_SIZE_CORRELATION_ID:
	case IPOD_NANO_THUMBNAIL_FULL_SIZE_CORRELATION_ID:
		img->type = ITDB_IMAGE_FULL_SCREEN;
		break;
	case IPOD_THUMBNAIL_NOW_PLAYING_CORRELATION_ID:
	case IPOD_NANO_THUMBNAIL_NOW_PLAYING_CORRELATION_ID:
		img->type = ITDB_IMAGE_NOW_PLAYING;
		break;
	default:
		g_print ("Unrecognized image size: %08x\n", 
			 GINT_FROM_LE (mhni->image_dimensions));
		g_free (img);
		return NULL;
	}

	return img;
}

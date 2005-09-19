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

#include "db-image-parser.h"

#define RED_BITS   5
#define RED_SHIFT 11
#define RED_MASK  (((1 << RED_BITS)-1) << RED_SHIFT)

#define GREEN_BITS 6
#define GREEN_SHIFT 5
#define GREEN_MASK (((1 << GREEN_BITS)-1) << GREEN_SHIFT)

#define BLUE_BITS 5
#define BLUE_SHIFT 0
#define BLUE_MASK (((1 << BLUE_BITS)-1) << BLUE_SHIFT)


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

#if 0
G_GNUC_UNUSED static void
pack_RGB_565 (GdkPixbuf *pixbuf, gushort **pixels565, unsigned int *bytes_len)
{
	guchar *pixels;
	gushort *result;
	gint row_stride;
	gint channels;
	gint width;
	gint height;
	gint w;
	gint h;

	g_object_get (G_OBJECT (pixbuf), 
		      "rowstride", &row_stride, "n-channels", &channels,
		      "height", &height, "width", &width,
		      "pixels", &pixels, NULL);

	result = g_malloc0 (width * height * 2);

	for (h = 0; h < height; h++) {
		for (w = 0; w < width; w++) {
			gint r;
			gint g;
			gint b;

			r = pixels[(h*row_stride + w)*channels];
			g = pixels[(h*row_stride + w)*channels + 1]; 
			b = pixels[(h*row_stride + w)*channels + 2]; 
			r >>= (8 - RED_BITS);
			g >>= (8 - GREEN_BITS);
			b >>= (8 - BLUE_BITS);
			r = (r << RED_SHIFT) & RED_MASK;
			g = (g << GREEN_SHIFT) & GREEN_MASK;
			b = (b << BLUE_SHIFT) & BLUE_MASK;
			result[h*height + w] =  (GINT16_TO_LE (r | g | b));
		}
	}

	*pixels565 = result;
	*bytes_len = width * height * 2;
}
#endif

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

G_GNUC_INTERNAL Itdb_Image *
ipod_image_new_from_mhni (MhniHeader *mhni, const char *mount_point)
{
	Itdb_Image *img;

	img = g_new0 (Itdb_Image, 1);
	if (img == NULL) {
		return NULL;
	}
	img->filename = g_strdup_printf ("%s/iPod_Control/Artwork/F%04u_1.ithmb", mount_point, GINT_FROM_LE (mhni->correlation_id));
	img->size = GINT_FROM_LE (mhni->image_size);
	img->offset = GINT_FROM_LE (mhni->ithmb_offset);
	img->width = (GINT_FROM_LE (mhni->image_dimensions) & 0xffff0000) >> 16;
	img->height = (GINT_FROM_LE (mhni->image_dimensions) & 0x0000ffff);

	return img;
}

G_GNUC_INTERNAL Itdb_Image *
ipod_image_new_from_mhii (MhiiHeader *mhii)
{
	Itdb_Image *img;

	img = g_new0 (Itdb_Image, 1);
	if (img == NULL) {
		return NULL;
	}
	img->size = GINT_FROM_LE (mhii->orig_img_size);
	img->id = GINT_FROM_LE (mhii->image_id);

	return img;	
}

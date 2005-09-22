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

#include "itdb.h"
#include "itdb_private.h"
#include "db-image-parser.h"

#include <errno.h>
#include <locale.h>
#include <string.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#define FULL_THUMB_SIDE_LEN 0x8c
#define NOW_PLAYING_THUMB_SIDE_LEN 0x38

#define IPOD_THUMBNAIL_FULL_SIZE_CORRELATION_ID 1016
#define IPOD_THUMBNAIL_NOW_PLAYING_CORRELATION_ID 1017

struct _iThumbWriter {
	off_t cur_offset;
	FILE *f;
	guint correlation_id;
	enum ItdbImageType type;
	int size;
	GHashTable *cache;
};
typedef struct _iThumbWriter iThumbWriter;

/* The iPod expect square thumbnails with 2 specific side sizes (0x38 and 0x8c
 * respectively for small and fullscreen thumbnails), the 'size' parameter is
 * here to specify which size we are interested in in case the pixbuf is non
 * square
 */
static void
pack_RGB_565 (GdkPixbuf *pixbuf, int size,
	      gushort **pixels565, unsigned int *bytes_len)
{
	guchar *pixels;
	gushort *result;
	gint row_stride;
	gint channels;
	gint width;
	gint height;
	gint w;
	gint h;

	g_return_if_fail (pixels565 != NULL);
	*pixels565 = NULL;
	g_return_if_fail (bytes_len != NULL);

	g_object_get (G_OBJECT (pixbuf), 
		      "rowstride", &row_stride, "n-channels", &channels,
		      "height", &height, "width", &width,
		      "pixels", &pixels, NULL);
	g_return_if_fail ((width <= size) && (height <= size));
	result = g_malloc0 (size * size * 2);

	for (h = 0; h < height; h++) {
		for (w = 0; w < width; w++) {
			gint r;
			gint g;
			gint b;

			r = pixels[h*row_stride + w*channels];
			g = pixels[h*row_stride + w*channels + 1]; 
			b = pixels[h*row_stride + w*channels + 2]; 
			r >>= (8 - RED_BITS);
			g >>= (8 - GREEN_BITS);
			b >>= (8 - BLUE_BITS);
			r = (r << RED_SHIFT) & RED_MASK;
			g = (g << GREEN_SHIFT) & GREEN_MASK;
			b = (b << BLUE_SHIFT) & BLUE_MASK;
			result[h*size + w] =  (GINT16_TO_LE (r | g | b));
		}
	}

	*pixels565 = result;
	*bytes_len = size * size * 2;
}



static Itdb_Image *
itdb_image_dup (Itdb_Image *image)
{
	Itdb_Image *result;

	result = g_new0 (Itdb_Image, 1);
	if (result == NULL) {
		return NULL;
	}
	result->type = image->type;
	result->height = image->height;
	result->width = image->width;
	result->offset = image->offset;
	result->size = image->size;

	return result;
}

static Itdb_Image *
ithumb_writer_write_thumbnail (iThumbWriter *writer, 
			       const char *filename)
{
	GdkPixbuf *thumb;
	gushort *pixels;
	Itdb_Image *image;

	image = g_hash_table_lookup (writer->cache, filename);
	if (image != NULL) {
		return itdb_image_dup (image);
	}

	image = g_new0 (Itdb_Image, 1);
	if (image == NULL) {
		return NULL;
	}

	thumb = gdk_pixbuf_new_from_file_at_scale (filename, writer->size, -1,
						   TRUE, NULL);
	if (thumb == NULL) {
		g_free (image);
		return NULL;
	}
	g_object_get (G_OBJECT (thumb), "height", &image->height, NULL);
	if (image->height > writer->size) {
		g_object_unref (thumb);
		thumb = gdk_pixbuf_new_from_file_at_scale (filename, 
							   -1, writer->size,
							   TRUE, NULL);
		if (thumb == NULL) {
			g_free (image);
			return NULL;
		}
	}
	g_object_get (G_OBJECT (thumb), 
		      "height", &image->height, 
		      "width", &image->width,
		      NULL);
	image->offset = writer->cur_offset;
	image->type = writer->type;
	pack_RGB_565 (thumb, writer->size, &pixels, &image->size);
	g_object_unref (G_OBJECT (thumb));
	if (pixels == NULL) {
		g_free (image);
		return NULL;
	}
	if (fwrite (pixels, image->size, 1, writer->f) != 1) {
		g_free (image);		
		g_free (pixels);
		g_print ("Error writing to file: %s\n", strerror (errno));
		return NULL;
	}
	g_free (pixels);
	writer->cur_offset += image->size;
	g_hash_table_insert (writer->cache, g_strdup (filename), image);

	return image;
}


#define FULL_THUMB_SIDE_LEN 0x8c
#define NOW_PLAYING_THUMB_SIDE_LEN 0x38

#define FULL_THUMB_CORRELATION_ID 1016
#define NOW_PLAYING_THUMB_CORRELATION_ID 1017



static iThumbWriter *
ithumb_writer_new (const char *mount_point, enum ItdbImageType type, 
		   int correlation_id, int size)
{
	char *filename;
	iThumbWriter *writer;
	writer = g_new0 (iThumbWriter, 1);
	if (writer == NULL) {
		return NULL;
	}
	writer->correlation_id = correlation_id;
	writer->size = size;
	writer->type = type;
	writer->cache = g_hash_table_new_full (g_str_hash, g_str_equal, 
					       g_free, NULL);
	if (writer->cache == NULL) {
		g_free (writer);
		return NULL;
	}
	filename = ipod_image_get_ithmb_filename (mount_point, correlation_id);
	writer->f = fopen (filename, "w");
	if (writer->f == NULL) {
		g_print ("Error opening %s: %s\n", filename, strerror (errno));
		g_free (filename);
		g_hash_table_destroy (writer->cache);
		g_free (writer);
		return NULL;
	}
	g_free (filename);
	
	return writer;
}

static void
ithumb_writer_free (iThumbWriter *writer)
{
	g_return_if_fail (writer != NULL);
	g_hash_table_destroy (writer->cache);
	fclose (writer->f);
	g_free (writer);
}

G_GNUC_INTERNAL int
itdb_write_ithumb_files (Itdb_iTunesDB *db, const char *mount_point) 
{
	GList *it;
	iThumbWriter *fullsize_writer;
	iThumbWriter *nowplaying_writer;
	g_print ("%s\n", G_GNUC_FUNCTION);

	fullsize_writer = ithumb_writer_new (mount_point,
					     ITDB_IMAGE_FULL_SCREEN,
					     FULL_THUMB_CORRELATION_ID,
					     FULL_THUMB_SIDE_LEN);
	if (fullsize_writer == NULL) {
		return -1;
	}

	nowplaying_writer = ithumb_writer_new (mount_point,
					       ITDB_IMAGE_NOW_PLAYING,
					       NOW_PLAYING_THUMB_CORRELATION_ID,
					       NOW_PLAYING_THUMB_SIDE_LEN);
	if (nowplaying_writer == NULL) {
		ithumb_writer_free (fullsize_writer);
		return -1;
	}

	for (it = db->tracks; it != NULL; it = it->next) {
		Itdb_Track *song;
		Itdb_Image *thumb;

		song = (Itdb_Track *)it->data;
		song->artwork_count = 0;
		itdb_track_free_generated_thumbnails (song);
		if (song->orig_image_filename == NULL) {
			continue;
		}
		thumb = ithumb_writer_write_thumbnail (nowplaying_writer,
						       song->orig_image_filename);
		if (thumb != NULL) {
			song->thumbnails = g_list_append (song->thumbnails, 
							  thumb);
			song->artwork_count++;
		}
		thumb = ithumb_writer_write_thumbnail (fullsize_writer,
						       song->orig_image_filename);
		if (thumb != NULL) {			
			song->thumbnails = g_list_append (song->thumbnails, 
							  thumb);
			song->artwork_count++;
		}		
	}

	ithumb_writer_free (nowplaying_writer);
	ithumb_writer_free (fullsize_writer);

	return 0;
}

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

#include <config.h>
#include "itdb.h"
#include "db-image-parser.h"

#ifdef HAVE_GDKPIXBUF

#include "itdb_private.h"

#include <errno.h>
#include <locale.h>
#include <string.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

struct _iThumbWriter {
	off_t cur_offset;
	FILE *f;
	IpodArtworkFormat *img_info;
	GHashTable *cache;
};
typedef struct _iThumbWriter iThumbWriter;

/* The iPod expect square thumbnails with 2 specific side sizes (0x38 and 0x8c
 * respectively for small and fullscreen thumbnails), the 'size' parameter is
 * here to specify which size we are interested in in case the pixbuf is non
 * square
 */
static gushort *
pack_RGB_565 (GdkPixbuf *pixbuf, int dst_width, int dst_height)
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
	g_return_val_if_fail ((width <= dst_width) && (height <= dst_height), NULL);
	result = g_malloc0 (dst_width * dst_height * 2);
	if (result == NULL) {
		return NULL;
	}
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
			result[h*dst_width + w] =  (GINT16_TO_LE (r | g | b));
		}
	}
	return result;
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

	thumb = gdk_pixbuf_new_from_file_at_size (filename, 
						  writer->img_info->width, 
						  writer->img_info->height,
						  NULL);
	if (thumb == NULL) {
		g_free (image);
		return NULL;
	}
	g_object_get (G_OBJECT (thumb), 
		      "height", &image->height, 
		      "width", &image->width,
		      NULL);
	image->offset = writer->cur_offset;
	image->type = writer->img_info->type;
	image->size = writer->img_info->width * writer->img_info->height * 2;
	/* FIXME: under certain conditions (probably related to writer->offset
	 * getting too big), this should be :F%04u_2.ithmb and so on
	 */
	image->filename = g_strdup_printf (":F%04u_1.ithmb", 
					   writer->img_info->correlation_id);
	pixels = pack_RGB_565 (thumb, writer->img_info->width, 
			       writer->img_info->height);
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


static char *
ipod_image_get_ithmb_filename (const char *mount_point, gint correlation_id) 
{
	char *paths[] = {"iPod_Control", "Artwork", NULL, NULL};
	char *filename;

	paths[2] = g_strdup_printf ("F%04u_1.ithmb", correlation_id);
	filename = itdb_resolve_path (mount_point, (const char **)paths);
	g_free (paths[2]);
	return filename;
}


static iThumbWriter *
ithumb_writer_new (const char *mount_point, const IpodArtworkFormat *info)
{
	char *filename;
	iThumbWriter *writer;
	writer = g_new0 (iThumbWriter, 1);
	if (writer == NULL) {
		return NULL;
	}
	writer->img_info = g_memdup (info, sizeof (IpodArtworkFormat));
	if (writer->img_info == NULL) {
		g_free (writer);
		return NULL;
	}
	writer->cache = g_hash_table_new_full (g_str_hash, g_str_equal, 
					       g_free, NULL);
	if (writer->cache == NULL) {
		g_free (writer->img_info);
		g_free (writer);
		return NULL;
	}
	filename = ipod_image_get_ithmb_filename (mount_point, 
						  info->correlation_id);
	if (filename == NULL) {
		g_hash_table_destroy (writer->cache);
		g_free (writer->img_info);
		g_free (writer);
		return NULL;
	}
	writer->f = fopen (filename, "w");
	if (writer->f == NULL) {
		g_print ("Error opening %s: %s\n", filename, strerror (errno));
		g_free (filename);
		g_hash_table_destroy (writer->cache);
		g_free (writer->img_info);
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
	g_free (writer->img_info);
	fclose (writer->f);
	g_free (writer);
}


static void
write_thumbnail (gpointer data, gpointer user_data)
{
	iThumbWriter *writer;
	Itdb_Track *song;
	Itdb_Image *thumb;

	song = (Itdb_Track *)user_data;
	writer = (iThumbWriter *)data;

	thumb = ithumb_writer_write_thumbnail (writer,
					       song->orig_image_filename);
	if (thumb != NULL) {
		song->thumbnails = g_list_append (song->thumbnails, thumb);
		song->artwork_count++;
	}
}


G_GNUC_INTERNAL int
itdb_write_ithumb_files (Itdb_iTunesDB *db, const char *mount_point) 
{
	GList *writers;
	GList *it;
	const IpodArtworkFormat *format;
/*	g_print ("%s\n", G_GNUC_FUNCTION);*/

	
	if (db->device == NULL) {
		return -1;
	}

	g_object_get (G_OBJECT (db->device), "artwork-formats", 
		      &format, NULL);
	if (format == NULL) {
		return -1;
	}

	writers = NULL;
	while (format->type != -1) {
		iThumbWriter *writer;

		switch (format->type) {
		case IPOD_COVER_SMALL:
		case IPOD_COVER_LARGE:
			writer = ithumb_writer_new (mount_point, format);
			if (writer != NULL) {
				writers = g_list_prepend (writers, writer);
			}
			break;
		default:
			break;
		}
		format++;
	}

	if (writers == NULL) {
		return -1;
	}

	for (it = db->tracks; it != NULL; it = it->next) {
		Itdb_Track *song;

		song = (Itdb_Track *)it->data;
		song->artwork_count = 0;
		itdb_track_free_generated_thumbnails (song);
		if (song->orig_image_filename == NULL) {
			continue;
		}
		g_list_foreach (writers, write_thumbnail, song);
	}

	g_list_foreach (writers, (GFunc)ithumb_writer_free, NULL);
	g_list_free (writers);

	return 0;
}
#else
G_GNUC_INTERNAL int
itdb_write_ithumb_files (Itdb_iTunesDB *db, const char *mount_point) 
{
    return -1;
}
#endif

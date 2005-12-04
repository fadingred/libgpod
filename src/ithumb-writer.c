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

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


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
static guint16 *
pack_RGB_565 (GdkPixbuf *pixbuf, int dst_width, int dst_height)
{
	guchar *pixels;
	guint16 *result;
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
	/* dst_width and dst_height come from a width/height database 
	 * hardcoded in libipoddevice code, so dst_width * dst_height * 2 can't
	 * overflow, even on an iPod containing malicious data
	 */
	result = g_malloc0 (dst_width * dst_height * 2);

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




static char *
ipod_image_get_ithmb_filename (const char *mount_point, gint correlation_id) 
{
	char *paths[] = {"iPod_Control", "Artwork", NULL, NULL};
	char *filename, *buf;

	buf = g_strdup_printf ("F%04u_1.ithmb", correlation_id);

	paths[2] = buf;

	filename = itdb_resolve_path (mount_point, (const char **)paths);

	/* itdb_resolve_path() only returns existing paths */
	if (!filename)
	{
	    gchar *path;
	    paths[2] = NULL;
	    path = itdb_resolve_path (mount_point, (const char **)paths);
	    if (path)
	    {
		filename = g_build_filename (path, buf, NULL);
	    }
	    g_free (path);
	}

	g_free (buf);

	return filename;
}



static gboolean
ithumb_writer_write_thumbnail (iThumbWriter *writer, 
			       Itdb_Thumb *thumb)
{
    GdkPixbuf *pixbuf;
    guint16 *pixels;
    gchar *filename;
    gint width, height;

    Itdb_Thumb *old_thumb;

    g_return_val_if_fail (writer, FALSE);
    g_return_val_if_fail (thumb, FALSE);

    /* If the same filename was written before, just use the old
       thumbnail to save space on the iPod */
    old_thumb = g_hash_table_lookup (writer->cache, thumb->filename);
    if (old_thumb != NULL)
    {
	g_free (thumb->filename);
	memcpy (thumb, old_thumb, sizeof (Itdb_Thumb));
	thumb->filename = g_strdup (old_thumb->filename);
	return TRUE;
    }

    filename = g_strdup (thumb->filename);

    pixbuf = gdk_pixbuf_new_from_file_at_size (filename, 
					       writer->img_info->width, 
					       writer->img_info->height,
					       NULL);
    if (pixbuf == NULL) {
	return FALSE;
    }

    /* !! cannot write directly to &thumb->width/height because
       g_object_get() returns a gint, but thumb->width/height are
       gint16 !! */
    g_object_get (G_OBJECT (pixbuf), 
		  "width", &width,
		  "height", &height,
		  NULL);

    thumb->width = width;
    thumb->height = height;
    thumb->offset = writer->cur_offset;
    thumb->size = writer->img_info->width * writer->img_info->height * 2;
/*     printf("offset: %d type: %d, size: %d\n", thumb->offset, thumb->type, thumb->size); */
    /* FIXME: under certain conditions (probably related to
     * writer->offset getting too big), this should be :F%04u_2.ithmb
     * and so on
     */
    thumb->filename = g_strdup_printf (":F%04u_1.ithmb", 
				       writer->img_info->correlation_id);
    pixels = pack_RGB_565 (pixbuf, writer->img_info->width, 
			   writer->img_info->height);
    g_object_unref (G_OBJECT (pixbuf));

    if (pixels == NULL)
    {
	return FALSE;
    }
    if (fwrite (pixels, thumb->size, 1, writer->f) != 1) {
	g_free (pixels);
	g_print ("Error writing to file: %s\n", strerror (errno));
	return FALSE;
    }
    g_free (pixels);
    writer->cur_offset += thumb->size;
    g_hash_table_insert (writer->cache, filename, thumb);

    /* !! filename is g_free()d when destroying the hash table. Do not
       do it here */

    return TRUE;
}

static void
write_thumbnail (gpointer _writer, gpointer _artwork)
{
	iThumbWriter *writer = _writer;
	Itdb_Artwork *artwork = _artwork;
 	Itdb_Thumb *thumb;

	thumb = itdb_artwork_get_thumb_by_type (artwork,
						writer->img_info->type);

	/* size == 0 indicates a thumbnail not yet written to the
	   thumbnail file */
	if (thumb && (thumb->size == 0))
	{
	    ithumb_writer_write_thumbnail (writer, thumb);
	}
}

static iThumbWriter *
ithumb_writer_new (const char *mount_point, const IpodArtworkFormat *info)
{
	char *filename;
	iThumbWriter *writer;

	writer = g_new0 (iThumbWriter, 1);

	writer->img_info = g_memdup (info, sizeof (IpodArtworkFormat));

	writer->cache = g_hash_table_new_full (g_str_hash, g_str_equal, 
					       g_free, NULL);

	filename = ipod_image_get_ithmb_filename (mount_point, 
						  info->correlation_id);
	if (filename == NULL) {
		g_hash_table_destroy (writer->cache);
		g_free (writer->img_info);
		g_free (writer);
		return NULL;
	}
	writer->f = fopen (filename, "ab");
	if (writer->f == NULL) {
		g_print ("Error opening %s: %s\n", filename, strerror (errno));
		g_free (filename);
		g_hash_table_destroy (writer->cache);
		g_free (writer->img_info);
		g_free (writer);
		return NULL;
	}
	writer->cur_offset = ftell (writer->f);
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


static gboolean ithumb_rearrange_thumbnail_file (gpointer _key,
						 gpointer _thumbs,
						 gpointer _user_data)
{
    auto gint offset_sort (gconstpointer a, gconstpointer b);
    gint offset_sort (gconstpointer a, gconstpointer b)
	{
	    return (((Itdb_Thumb *)a)->offset - 
		    ((Itdb_Thumb *)b)->offset);
	}
    gchar *filename = _key;
    GList *thumbs = _thumbs;
    gboolean *result = _user_data;
    gint fd = -1;
    guint32 size = 0;
    guint32 tnf_num, tn_num, i;
    GList *gl;
    struct stat statbuf;
    void *buf = NULL;

/*     printf ("%s: %d\n", filename, g_list_length (thumbs)); */

    /* check if an error occured */
    if (*result == FALSE)
	goto out;

    /* check if all thumbnails have the same size */
    for (gl=thumbs; gl; gl=gl->next)
    {
	Itdb_Thumb *img = gl->data;

	if (size == 0)
	    size = img->size;
	if (size != img->size)
	{
	    *result = FALSE;
	    goto out;
	}
    }
    /* OK, all thumbs are the same size @size, let's see how many
     * thumbnails are in the actual file */
/*     printf ("  %d\n", size); */
    if (g_stat  (filename, &statbuf) != 0)
    {
	*result = FALSE;
	goto out;
    }
    tnf_num = statbuf.st_size / size;

    /* check if the file size is a multiple of @size */
    if (tnf_num*size != statbuf.st_size)
    {
	*result = FALSE;
	goto out;
    }

    tn_num = g_list_length (thumbs);

    /* We're finished if the number here and the number of thumbnails
     * in our list is the same */
    if (tn_num == tnf_num)
	goto out;

    fd = open (filename, O_RDWR, 0);
    if (fd == -1)
    {
	*result = FALSE;
	goto out;
    }

    /* Performance note: for performance reaons the list should be
       ordered in reverse order of offsets because of frequent use
       g_list_last(), and instead of using g_list_nth_data() the list
       should be crawled by element from the end -- I will do that
       eventually unless someone beats me to it. */

    /* Sort the list of thumbs according to img->offset */
    thumbs = g_list_sort (thumbs, offset_sort);

    /* size is either a value coming from a hardcoded const array from 
     * libipoddevice, or a guint32 read from an iPod file, so no overflow
     * can occur here
     */
    buf = g_malloc (size);

    for (i=0; i<tn_num; ++i)
    {
	guint offset = i * size;
	Itdb_Thumb *img = g_list_nth_data (thumbs, i);
	if (offset != img->offset)
	{   /* We found an open space -> copy the last element here */
	    gl = g_list_last (thumbs);
	    img = gl->data;
	    thumbs = g_list_delete_link (thumbs, gl);
	    thumbs = g_list_insert (thumbs, img, i);

	    /* actually copy the data */
	    if (lseek (fd, img->offset, SEEK_SET) != img->offset)
	    {
		*result = FALSE;
		goto out;
	    }
	    if (read (fd, buf, size) != size)
	    {
		*result = FALSE;
		goto out;
	    }
	    if (lseek (fd, offset, SEEK_SET) != offset)
	    {
		*result = FALSE;
		goto out;
	    }
	    if (write (fd, buf, size) != size)
	    {
		*result = FALSE;
		goto out;
	    }

	    img->offset = offset;
	}
    }
    /* truncate the file */
    if (ftruncate (fd, tn_num*size) == -1)
    {
	*result = FALSE;
	goto out;
    }

  out:
    if (fd != -1) close (fd);
    if (buf) g_free (buf);
    g_list_free (thumbs);
    return TRUE;
}


/* The actual image data of thumbnails is not read into memory. As a
   consequence, writing the thumbnail file is not as straight-forward
   as e.g. writing the iTunesDB where all data is held in memory.

   To avoid the need to read large amounts from the iPod and back, or
   have to large files exist on the iPod (reading from the original
   thumbnail fail and writing to the new thumbnail file), the
   modifications are done in place.

   It is assumed that all thumbnails have the same data size. If not,
   FALSE is returned.

   If a thumbnail has been removed, a slot in the file is opened. This
   slot is filled by copying data from the end of the file and
   adjusting the corresponding Itdb_Image offset pointer. When all
   slots are filled, the file is truncated to the new length.
*/
static gboolean
ithmb_rearrange_existing_thumbnails (Itdb_iTunesDB *itdb,
				     const IpodArtworkFormat *info)
{
    GList *gl;
    GHashTable *filenamehash;
    gboolean result = TRUE;

    g_return_val_if_fail (itdb, FALSE);
    g_return_val_if_fail (info, FALSE);
    g_return_val_if_fail (itdb->mountpoint, FALSE);

    filenamehash = g_hash_table_new_full (g_str_hash, g_str_equal, 
					  g_free, NULL);

    /* Create a hash with all filenames used for thumbnails.
       This will usually be a number of "F%04d_%d.ithmb" files. A
       GList is kept with pointers to all images in a given file which
       allows to adjust the offset pointers */
    for (gl=itdb->tracks; gl; gl=gl->next)
    {
	Itdb_Thumb *thumb;
	Itdb_Track *track = gl->data;
	g_return_val_if_fail (track, FALSE);

	thumb = itdb_artwork_get_thumb_by_type (track->artwork,
						info->type);
	if (thumb && thumb->filename && (thumb->size != 0))
	{
	    GList *thumbs;
	    gchar *filename = itdb_thumb_get_filename (itdb->device,
						       thumb);
	    if (filename)
	    {
		thumbs = g_hash_table_lookup (filenamehash, filename);
		thumbs = g_list_append (thumbs, thumb);
		g_hash_table_insert (filenamehash, filename, thumbs);
	    }
	}
    }

    g_hash_table_foreach_remove (filenamehash,
				 ithumb_rearrange_thumbnail_file, &result);
    g_hash_table_destroy (filenamehash);

    return result;
}

#endif

G_GNUC_INTERNAL int
itdb_write_ithumb_files (Itdb_iTunesDB *db) 
{
#ifdef HAVE_GDKPIXBUF
	GList *writers;
	GList *it;
	gchar *mount_point;
	const IpodArtworkFormat *format;
/*	g_print ("%s\n", G_GNUC_FUNCTION);*/

	g_return_val_if_fail (db, -1);

	mount_point = db->mountpoint;
	/* FIXME: support writing to directory rather than writing to
	   iPod */
	if (mount_point == NULL)
	    return -1;
	
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
		        ithmb_rearrange_existing_thumbnails (db,
							     format);
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
		Itdb_Track *track;

		track = it->data;
		g_return_val_if_fail (track, -1);

		g_list_foreach (writers, write_thumbnail, track->artwork);
	}

	g_list_foreach (writers, (GFunc)ithumb_writer_free, NULL);
	g_list_free (writers);

	return 0;
#else
    return -1;
#endif
}

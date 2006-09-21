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
#include "itdb_endianness.h"

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
        gchar *filename;
	Itdb_ArtworkFormat *img_info;
        DbType db_type;
        guint byte_order;
};
typedef struct _iThumbWriter iThumbWriter;

/* The iPod expect square thumbnails with 2 specific side sizes (0x38 and 0x8c
 * respectively for small and fullscreen thumbnails), the 'size' parameter is
 * here to specify which size we are interested in in case the pixbuf is non
 * square
 */
static guint16 *
pack_RGB_565 (GdkPixbuf *pixbuf, iThumbWriter *writer,
	      gint horizontal_padding, gint vertical_padding)
{
	guchar *pixels;
	guint16 *result;
	gint row_stride;
	gint channels;
	gint width;
	gint height;
	gint w;
	gint h;
	gint byte_order;

	g_object_get (G_OBJECT (pixbuf), 
		      "rowstride", &row_stride, "n-channels", &channels,
		      "height", &height, "width", &width,
		      "pixels", &pixels, NULL);
	g_return_val_if_fail ((width <= writer->img_info->width) && (height <= writer->img_info->height), NULL);
	/* dst_width and dst_height come from a width/height database 
	 * hardcoded in libipoddevice code, so dst_width * dst_height * 2 can't
	 * overflow, even on an iPod containing malicious data
	 */
	result = g_malloc0 (writer->img_info->width * writer->img_info->height * 2);

	/* Swap the byte order on full screen nano photos */
	byte_order = writer->byte_order;
	if ( writer->img_info->correlation_id == 1023) {
		if (byte_order == G_LITTLE_ENDIAN)
			byte_order = G_BIG_ENDIAN;
		else
			byte_order = G_LITTLE_ENDIAN; 
	}

	for (h = 0; h < height; h++) {
	        gint line = (h+vertical_padding)*writer->img_info->width;
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
			result[line + w + horizontal_padding] =
			    get_gint16 (r | g | b, byte_order);
		}
	}
	return result;
}


static char *
ipod_image_get_ithmb_filename (const char *mount_point, gint correlation_id, gint index, DbType db_type ) 
{
	gchar *artwork_dir = NULL, *filename, *buf;

	g_return_val_if_fail (mount_point, NULL);
	switch( db_type ) {
	case DB_TYPE_PHOTO:
		artwork_dir = itdb_get_photos_thumb_dir (mount_point);
		if (!artwork_dir)
		{
			/* attempt to create Thumbs dir */
			gchar *photos_dir = itdb_get_photos_dir (mount_point);
			gchar *dir;
			if (!photos_dir)
			{   /* give up */
				return NULL;
			}
			dir = g_build_filename (photos_dir, "Thumbs", NULL);
			mkdir (dir, 0777);
			g_free (dir);
			g_free (photos_dir);

			/* try again */
			artwork_dir = itdb_get_photos_thumb_dir (mount_point);
			if (!artwork_dir)
			{   /* give up */
				return NULL;
			}
		}
		break;
	case DB_TYPE_ITUNES:
	artwork_dir = itdb_get_artwork_dir (mount_point);
	if (!artwork_dir)
	{
		/* attempt to create Artwork dir */
		gchar *control_dir = itdb_get_control_dir (mount_point);
		gchar *dir;
		if (!control_dir)
		{   /* give up */
			return NULL;
		}
		dir = g_build_filename (control_dir, "Artwork", NULL);
		mkdir (dir, 0777);
		g_free (dir);
		g_free (control_dir);

		/* try again */
		artwork_dir = itdb_get_artwork_dir (mount_point);
		if (!artwork_dir)
		{   /* give up */
			return NULL;
		}
	}
	}

	buf = g_strdup_printf ("F%04u_%d.ithmb", correlation_id, index);

	filename = itdb_get_path (artwork_dir, buf);

	/* itdb_get_path() only returns existing paths */
	if (!filename)
	{
	    filename = g_build_filename (artwork_dir, buf, NULL);
	}
/*	printf ("%s %s\n", buf, filename);*/

	g_free (buf);

	return filename;
}



static gboolean
ithumb_writer_write_thumbnail (iThumbWriter *writer, 
			       Itdb_Thumb *thumb)
{
    GdkPixbuf *pixbuf = NULL;
    guint16 *pixels;
    gint width, height;

    g_return_val_if_fail (writer, FALSE);
    g_return_val_if_fail (thumb, FALSE);

    if (thumb->filename)
    {   /* read image from filename */
	pixbuf = gdk_pixbuf_new_from_file_at_size (thumb->filename, 
						   writer->img_info->width, 
						   writer->img_info->height,
						   NULL);

    }
    else if (thumb->image_data)
    {   /* image data is stored in image_data and image_data_len */
	GdkPixbufLoader *loader = gdk_pixbuf_loader_new ();
	g_return_val_if_fail (loader, FALSE);
	gdk_pixbuf_loader_set_size (loader,
				    writer->img_info->width, 
				    writer->img_info->height);
	gdk_pixbuf_loader_write (loader,
				 thumb->image_data,
				 thumb->image_data_len,
				 NULL);
	gdk_pixbuf_loader_close (loader, NULL);
	pixbuf = gdk_pixbuf_loader_get_pixbuf (loader);
	g_object_ref (pixbuf);
	g_object_unref (loader);

	g_free (thumb->image_data);
	thumb->image_data = NULL;
	thumb->image_data_len = 0;
    }

    if (pixbuf == NULL)
    {
	return FALSE;
    }

    /* !! cannot write directly to &thumb->width/height because
       g_object_get() returns a gint, but thumb->width/height are
       gint16 !! */
    g_object_get (G_OBJECT (pixbuf), 
		  "width", &width,
		  "height", &height,
		  NULL);

    g_free (thumb->filename);
    thumb->filename = NULL;

    /* FIXME: under certain conditions (probably related to
     * writer->offset getting too big), this should be :F%04u_2.ithmb
     * and so on
     */
    if( thumb->type == ITDB_THUMB_PHOTO_LARGE ||
	thumb->type == ITDB_THUMB_PHOTO_SMALL ||
	thumb->type == ITDB_THUMB_PHOTO_FULL_SCREEN )
    {
	thumb->filename = g_strdup_printf (":Thumbs:F%04u_1.ithmb", 
					   writer->img_info->correlation_id);
	
    } else {
	thumb->filename = g_strdup_printf (":F%04u_1.ithmb", 
					   writer->img_info->correlation_id);
    }

    switch (writer->db_type)
    {
    case DB_TYPE_PHOTO:
	thumb->horizontal_padding = (writer->img_info->width - width)/2;
	thumb->vertical_padding = (writer->img_info->height - height)/2;
	break;
    case DB_TYPE_ITUNES:
	/* IPOD_COVER_LARGE will be centered automatically using
	   the info in mhni->width/height. Free space around
	   IPOD_COVER_SMALL will be used to display track
	   information -> no padding (tested on iPod
	   Nano). mhni->hor_/ver_padding is working */
	thumb->horizontal_padding = 0;
	thumb->vertical_padding = 0;
	break;
    default:
	g_return_val_if_reached (FALSE);
    }

    /* The thumbnail width/height is inclusive padding */
    thumb->width = thumb->horizontal_padding + width;
    thumb->height = thumb->vertical_padding + height;
    thumb->offset = writer->cur_offset;
    thumb->size = writer->img_info->width * writer->img_info->height * 2;

    pixels = pack_RGB_565 (pixbuf, writer,
			   thumb->horizontal_padding,
			   thumb->vertical_padding);
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
ithumb_writer_new (const char *mount_point,
		   const Itdb_ArtworkFormat *info,
		   DbType db_type,
		   guint byte_order)
{
	char *filename;
	iThumbWriter *writer;

	writer = g_new0 (iThumbWriter, 1);

	writer->img_info = g_memdup (info, sizeof (Itdb_ArtworkFormat));

	writer->byte_order = byte_order;
	writer->db_type = db_type;

	filename = ipod_image_get_ithmb_filename (mount_point, 
						  info->correlation_id,
						  1, 
						  db_type);
	if (filename == NULL) {
		g_free (writer->img_info);
		g_free (writer);
		return NULL;
	}
	writer->f = fopen (filename, "ab");
	if (writer->f == NULL) {
		g_print ("Error opening %s: %s\n", filename, strerror (errno));
		g_free (filename);
		g_free (writer->img_info);
		g_free (writer);
		return NULL;
	}
	writer->cur_offset = ftell (writer->f);
	writer->filename=filename;
	
	return writer;
}

static void
ithumb_writer_free (iThumbWriter *writer)
{
	g_return_if_fail (writer != NULL);
	fclose (writer->f);
	if (writer->cur_offset == 0)
	{   /* Remove empty file */
	    unlink (writer->filename);
	}
	g_free (writer->img_info);
	g_free (writer->filename);
	g_free (writer);
}


gint offset_sort (gconstpointer a, gconstpointer b);
gint offset_sort (gconstpointer a, gconstpointer b)
{
    return (-(((Itdb_Thumb *)a)->offset - ((Itdb_Thumb *)b)->offset));
}

static gboolean ithumb_rearrange_thumbnail_file (gpointer _key,
						 gpointer _thumbs,
						 gpointer _user_data)
{
    const gchar *filename = _key;
    GList *thumbs = _thumbs;
    gboolean *result = _user_data;
    gint fd = -1;
    guint32 size = 0;
    GList *gl;
    struct stat statbuf;
    guint32 offset;
    void *buf = NULL;

/*     printf ("%s: %d\n", filename, g_list_length (thumbs)); */

    /* check if an error occured */
    if (*result == FALSE)
	goto out;

    if (thumbs == NULL)
    {   /* no thumbnails for this file --> remove altogether */
	if (unlink (filename) == -1)
	{
	    *result = FALSE;
	    goto out;
	}
    }

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
    if (size == 0)
    {
	*result = FALSE;
	goto out;
    }

    /* OK, all thumbs are the same size @size, let's see how many
     * thumbnails are in the actual file */
    if (g_stat  (filename, &statbuf) != 0)
    {
	*result = FALSE;
	goto out;
    }

    /* check if the file size is a multiple of @size */
    if (((guint32)(statbuf.st_size / size))*size != statbuf.st_size)
    {
	*result = FALSE;
	goto out;
    }

    fd = open (filename, O_RDWR, 0);
    if (fd == -1)
    {
	*result = FALSE;
	goto out;
    }

    /* size is either a value coming from a hardcoded const array from 
     * libipoddevice, or a guint32 read from an iPod file, so no overflow
     * can occur here
     */
    buf = g_malloc (size);

    /* Sort the list of thumbs in reverse order of img->offset */
    thumbs = g_list_sort (thumbs, offset_sort);

    gl = g_list_last (thumbs);

    /* check each thumbnail slot */
    for (offset=0; gl && (offset<statbuf.st_size); offset+=size)
    {
	Itdb_Thumb *thumb = gl->data;
	g_return_val_if_fail (thumb, FALSE);

	/* Try to find a thumbnail that uses this slot */
	while ((gl != NULL) && (thumb->offset < offset))
	{
	    gl = gl->prev;
	    if (gl)
		thumb = gl->data;
	    g_return_val_if_fail (thumb, FALSE);
	}

	if (!gl)
	    break;  /* offset now indicates new length of file */

	if (thumb->offset > offset)
	{
	    /* did not find a thumbnail with matching offset -> copy
	       data from last slot (== first element) */
	    GList *first_gl = g_list_first (thumbs);
	    Itdb_Thumb *first_thumb = first_gl->data;
	    guint32 first_offset;

	    g_return_val_if_fail (first_thumb, FALSE);
	    first_offset = first_thumb->offset;

	    /* actually copy the data */
	    if (lseek (fd, first_offset, SEEK_SET) != first_offset)
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

	    /* Adjust offset of all thumbnails whose offset is
	       first_offset. Since the list is sorted, they are all at
	       the beginning of the list. */
	    while (first_thumb->offset == first_offset)
	    {
		first_thumb->offset = offset;
		/* There's a possibility that gl is the first
		   element. In that case don't attempt to move it (it
		   wouldn't work as intended because we access
		   gl->next after removing it from the list) */
		if (gl != first_gl)
		{
		    thumbs = g_list_delete_link (thumbs, first_gl);
		    /* Insert /behind/ gl */
		    thumbs = g_list_insert_before (thumbs,
						   gl->next, first_thumb);
		    first_gl = g_list_first (thumbs);
		    first_thumb = first_gl->data;
		    g_return_val_if_fail (first_thumb, FALSE);
		}
	    }
	}
    }
    /* offset corresponds to the new length of the file */
    if (offset > 0)
    {   /* Truncate */
	if (ftruncate (fd, offset) == -1)
	{
	    *result = FALSE;
	    goto out;
	}
    }
    else
    {   /* Remove file altogether */
	close (fd);
	fd = -1;
	if (unlink (filename) == -1)
	{
	    *result = FALSE;
	    goto out;
	}
    }

  out:
    if (fd != -1) close (fd);
    g_free (buf);
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
ithmb_rearrange_existing_thumbnails (Itdb_DB *db,
				     const Itdb_ArtworkFormat *info)
{
    GList *gl;
    GHashTable *filenamehash;
    gboolean result = TRUE;
    GList *thumbs;
    gint i;
    gchar *filename;
    const gchar *mountpoint;

    g_return_val_if_fail (db, FALSE);
    g_return_val_if_fail (info, FALSE);
    g_return_val_if_fail (db_get_device(db), FALSE);

    mountpoint = db_get_mountpoint (db);

    g_return_val_if_fail (mountpoint, FALSE);

    filenamehash = g_hash_table_new_full (g_str_hash, g_str_equal, 
					  g_free, NULL);

    /* Create a hash with all filenames used for thumbnails.
       This will usually be a number of "F%04d_%d.ithmb" files. A
       GList is kept with pointers to all images in a given file which
       allows to adjust the offset pointers */
	switch (db->db_type) {
	case DB_TYPE_ITUNES:
		for (gl=db_get_itunesdb(db)->tracks; gl; gl=gl->next)
		{
			Itdb_Thumb *thumb;
			Itdb_Track *track = gl->data;
			g_return_val_if_fail (track, FALSE);

			thumb = itdb_artwork_get_thumb_by_type (track->artwork,
					info->type);
			if (thumb && thumb->filename && (thumb->size != 0))
			{
				filename = itdb_thumb_get_filename (
				        db_get_device(db),
				        thumb);
				if (filename)
				{
					thumbs = g_hash_table_lookup (filenamehash, filename);
					thumbs = g_list_append (thumbs, thumb);
					g_hash_table_insert (filenamehash, filename, thumbs);
				}
			}
		}
		break;
    case DB_TYPE_PHOTO:
	for (gl=db_get_photodb(db)->photos; gl; gl=gl->next)
	{
		Itdb_Thumb *thumb;
		Itdb_Artwork *artwork = gl->data;

		thumb = itdb_artwork_get_thumb_by_type (artwork,
				info->type);
		if (thumb && thumb->filename && (thumb->size != 0))
		{
			filename = itdb_thumb_get_filename (
			        db_get_device (db),
			        thumb);
			if (filename)
			{
				thumbs = g_hash_table_lookup (filenamehash, filename);
				thumbs = g_list_append (thumbs, thumb);
				g_hash_table_insert (filenamehash, filename, thumbs);
			}
		}
	}
	break;
    default:
	g_return_val_if_reached (FALSE);
    }

    /* Check for files present on the iPod but no longer referenced by
       thumbs */

    for (i=0; i<10; ++i)
    {
	filename = ipod_image_get_ithmb_filename (mountpoint,
						  info->correlation_id,
						  i,
						  db->db_type);
	if (g_file_test (filename, G_FILE_TEST_EXISTS))
	{
	    if (g_hash_table_lookup (filenamehash, filename) == NULL)
	    {
		g_hash_table_insert (filenamehash,
				     g_strdup (filename), NULL);
	    }
	}
	g_free (filename);
    }

    g_hash_table_foreach_remove (filenamehash,
				 ithumb_rearrange_thumbnail_file, &result);
    g_hash_table_destroy (filenamehash);

    return result;
}

#endif

G_GNUC_INTERNAL int
itdb_write_ithumb_files (Itdb_DB *db) 
{
#ifdef HAVE_GDKPIXBUF
	GList *writers;
	GList *it;
	Itdb_Device *device;
	const Itdb_ArtworkFormat *format;
	const gchar *mount_point;

	g_return_val_if_fail (db, -1);
	device = db_get_device(db);
	g_return_val_if_fail (device, -1);

	mount_point = db_get_mountpoint (db);
	/* FIXME: support writing to directory rather than writing to
	   iPod */
	if (mount_point == NULL)
	    return -1;
	
	format = itdb_device_get_artwork_formats (device);
	if (format == NULL) {
		return -1;
	}
	writers = NULL;
	while (format->type != -1) {
		iThumbWriter *writer;

		switch (format->type) {
		case ITDB_THUMB_COVER_SMALL:
		case ITDB_THUMB_COVER_LARGE:
		case ITDB_THUMB_PHOTO_SMALL:
		case ITDB_THUMB_PHOTO_LARGE:
		case ITDB_THUMB_PHOTO_FULL_SCREEN:
		    ithmb_rearrange_existing_thumbnails (db,
							     format );
			writer = ithumb_writer_new (mount_point, 
						    format,
						    db->db_type, 
						    device->byte_order);
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
	switch (db->db_type) {
	case DB_TYPE_ITUNES:
		for (it = db_get_itunesdb(db)->tracks; it != NULL; it = it->next) {
			Itdb_Track *track;

			track = it->data;
			g_return_val_if_fail (track, -1);

			g_list_foreach (writers, write_thumbnail, track->artwork);
		}
		break;
	case DB_TYPE_PHOTO:
		for (it = db_get_photodb(db)->photos; it != NULL; it = it->next) {
			Itdb_Artwork *photo;

			photo = it->data;
			g_return_val_if_fail (photo, -1);

			g_list_foreach (writers, write_thumbnail, photo);
		}
		break;
	default:
	        g_return_val_if_reached (-1);
	}
	
	g_list_foreach (writers, (GFunc)ithumb_writer_free, NULL);
	g_list_free (writers);

	return 0;
#else
    return -1;
#endif
}

/* Time-stamp: <2007-10-27 21:54:23 jcs>
|
|  Copyright (C) 2002-2005 Jorg Schuler <jcsjcs at users sourceforge net>
|  Part of the gtkpod project.
| 
|  URL: http://www.gtkpod.org/
|  URL: http://gtkpod.sourceforge.net/
|
|  The code contained in this file is free software; you can redistribute
|  it and/or modify it under the terms of the GNU Lesser General Public
|  License as published by the Free Software Foundation; either version
|  2.1 of the License, or (at your option) any later version.
|
|  This file is distributed in the hope that it will be useful,
|  but WITHOUT ANY WARRANTY; without even the implied warranty of
|  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
|  Lesser General Public License for more details.
|
|  You should have received a copy of the GNU Lesser General Public
|  License along with this code; if not, write to the Free Software
|  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
|
|  iTunes and iPod are trademarks of Apple
|
|  This product is not supported/written/published by Apple!
|
|  $Id$
*/

#include <config.h>

#include "itdb_device.h"
#include "itdb_private.h"
#include "db-image-parser.h"
#include "itdb_endianness.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>
#if HAVE_GDKPIXBUF
#include <gdk-pixbuf/gdk-pixbuf.h>
#endif
#include <glib/gi18n-lib.h>
#include <stdlib.h>

/**
 * itdb_artwork_new:
 * 
 * Creates a new #Itdb_Artwork 
 *
 * Return value: a new #Itdb_Artwork to be freed with itdb_artwork_free() when
 * no longer needed
 **/
Itdb_Artwork *itdb_artwork_new (void)
{
    Itdb_Artwork *artwork = g_new0 (Itdb_Artwork, 1);
    return artwork;
}

/**
 * itdb_artwork_free:
 * @artwork: an #Itdb_Artwork
 *
 * Frees memory used by @artwork
 **/
void itdb_artwork_free (Itdb_Artwork *artwork)
{
    g_return_if_fail (artwork);
    itdb_artwork_remove_thumbnails (artwork);
    if (artwork->userdata && artwork->userdata_destroy)
	(*artwork->userdata_destroy) (artwork->userdata);
    g_free (artwork);
}


static GList *dup_thumbnails (GList *thumbnails)
{
    GList *it;
    GList *result;
    
    result = NULL;
    for (it = thumbnails; it != NULL; it = it->next)
    {
	Itdb_Thumb *new_thumb;
	Itdb_Thumb *thumb;

	thumb = (Itdb_Thumb *)it->data;
	g_return_val_if_fail (thumb, NULL);

	new_thumb = itdb_thumb_duplicate (thumb);
	
	result = g_list_prepend (result, new_thumb);
    }

    return g_list_reverse (result);
}

/**
 * itdb_artwork_duplicate:
 * @artwork: an #Itdb_Artwork
 *
 * Duplicates @artwork
 *
 * Return value: a new copy of @artwork
 **/
Itdb_Artwork *itdb_artwork_duplicate (Itdb_Artwork *artwork)
{
    Itdb_Artwork *dup;
    g_return_val_if_fail (artwork, NULL);

    dup = g_new0 (Itdb_Artwork, 1);

    memcpy (dup, artwork, sizeof (Itdb_Artwork));

    dup->thumbnails = dup_thumbnails (artwork->thumbnails);

    return dup;
}


/**
 * itdb_artwork_remove_thumbnail:
 * @artwork: an #Itdb_Artwork
 * @thumb: an #Itdb_Thumb
 *
 * Removes @thumb from @artwork. The memory used by @thumb is freed.
 **/
void
itdb_artwork_remove_thumbnail (Itdb_Artwork *artwork, Itdb_Thumb *thumb)
{
    g_return_if_fail (artwork);
    g_return_if_fail (thumb);

    artwork->thumbnails = g_list_remove (artwork->thumbnails, thumb);
    itdb_thumb_free (thumb);
}


/**
 * itdb_artwork_remove_thumbnails:
 * @artwork: an #Itdb_Artwork
 *
 * Removes all thumbnails from @artwork 
 **/
void
itdb_artwork_remove_thumbnails (Itdb_Artwork *artwork)
{
    g_return_if_fail (artwork);

    while (artwork->thumbnails)
    {
	Itdb_Thumb *thumb = artwork->thumbnails->data;
	g_return_if_fail (thumb);
	itdb_artwork_remove_thumbnail (artwork, thumb);
    }
    artwork->artwork_size = 0;
    artwork->id = 0;
}




/**
 * itdb_artwork_add_thumbnail
 * @artwork: an #Itdb_Thumbnail
 * @type: thumbnail size
 * @filename: image file to use to create the thumbnail
 * @rotation: angle by which the image should be rotated
 * counterclockwise. Valid values are 0, 90, 180 and 270.
 * @error: return location for a #GError or NULL
 *
 * Appends a thumbnail of type @type to existing thumbnails in @artwork. No 
 * data is read from @filename yet, the file will be when @artwork is saved to
 * disk. @filename must still exist when that happens.
 *
 * For the rotation angle you can also use the gdk constants
 * GDK_PIXBUF_ROTATE_NONE, ..._COUNTERCLOCKWISE, ..._UPSIDEDOWN AND
 * ..._CLOCKWISE.
 *
 * Return value: TRUE if the thumbnail could be successfully added, FALSE
 * otherwise. @error is set appropriately.
 **/
gboolean
itdb_artwork_add_thumbnail (Itdb_Artwork *artwork,
			    ItdbThumbType type,
			    const gchar *filename,
			    gint rotation,
			    GError **error)
{
#ifdef HAVE_GDKPIXBUF
/* This operation doesn't make sense when we can't save thumbnail files */
    struct stat statbuf;
    Itdb_Thumb *thumb;

    g_return_val_if_fail (artwork, FALSE);
    g_return_val_if_fail (filename, FALSE);

    if (g_stat  (filename, &statbuf) != 0) {
	g_set_error (error, 0, -1,
		     _("Could not access file '%s'."),
		     filename);
	return FALSE;
    }

    artwork->artwork_size  = statbuf.st_size;
    artwork->creation_date = statbuf.st_mtime;

    thumb = itdb_thumb_new ();
    thumb->filename = g_strdup (filename);
    thumb->type = type;
    thumb->rotation = rotation;
    artwork->thumbnails = g_list_append (artwork->thumbnails, thumb);

    return TRUE;
#else
    g_set_error (error, 0, -1,
		 _("Artwork support not compiled into libgpod."));
    return FALSE;
#endif
}

/**
 * itdb_artwork_add_thumbnail_from_pixbuf
 * @artwork: an #Itdb_Thumbnail
 * @type: thumbnail size
 * @pixbuf: #GdkPixbuf to use to create the thumbnail
 * @rotation: angle by which the image should be rotated
 * counterclockwise. Valid values are 0, 90, 180 and 270.
 * @error: return location for a #GError or NULL
 *
 * Appends a thumbnail of type @type to existing thumbnails in @artwork. No 
 * data is generated from @pixbuf yet, it will be done when @artwork is saved 
 * to disk. @pixbuf is ref'ed by this function, but is not copied, so it should
 * not be modified before the database is saved.
 *
 * For the rotation angle you can also use the gdk constants
 * GDK_PIXBUF_ROTATE_NONE, ..._COUNTERCLOCKWISE, ..._UPSIDEDOWN AND
 * ..._CLOCKWISE.
 *
 * Return value: TRUE if the thumbnail could be successfully added, FALSE
 * otherwise. @error is set appropriately.
 **/
gboolean
itdb_artwork_add_thumbnail_from_pixbuf (Itdb_Artwork *artwork,
                                        ItdbThumbType type,
                                        gpointer pixbuf,
                                        gint rotation,
                                        GError **error)
{
#ifdef HAVE_GDKPIXBUF
/* This operation doesn't make sense when we can't save thumbnail files */
    Itdb_Thumb *thumb;
    GTimeVal time;
    gint rowstride;
    gint height;

    g_return_val_if_fail (artwork, FALSE);
    g_return_val_if_fail (GDK_IS_PIXBUF (pixbuf), FALSE);

    g_get_current_time (&time);
    g_object_get (G_OBJECT (pixbuf),
                  "height", &height,
                  "rowstride", &rowstride,
                  NULL);
    artwork->artwork_size  = rowstride * height;
    artwork->creation_date = time.tv_sec;

    thumb = itdb_thumb_new ();
    g_object_ref (G_OBJECT (pixbuf));
    thumb->pixbuf = pixbuf;
    thumb->type = type;
    thumb->rotation = rotation;
    artwork->thumbnails = g_list_append (artwork->thumbnails, thumb);

    return TRUE;
#else
    g_set_error (error, 0, -1,
		 _("Artwork support not compiled into libgpod."));
    return FALSE;
#endif
}

/**
 * itdb_artwork_add_thumbnail_from_data
 * @artwork: an #Itdb_Thumbnail
 * @type: thumbnail size
 * @image_data: data used to create the thumbnail (the raw contents of
 *              an image file)
 * @image_data_len: length of above data block
 * @rotation: angle by which the image should be rotated
 * counterclockwise. Valid values are 0, 90, 180 and 270.
 * @error: return location for a #GError or NULL
 *
 * Appends a thumbnail of type @type to existing thumbnails in
 * @artwork. No data is processed yet. This will be done when @artwork
 * is saved to disk.
 *
 * For the rotation angle you can also use the gdk constants
 * GDK_PIXBUF_ROTATE_NONE, ..._COUNTERCLOCKWISE, ..._UPSIDEDOWN AND
 * ..._CLOCKWISE.
 *
 * Return value: TRUE if the thumbnail could be successfully added, FALSE
 * otherwise. @error is set appropriately.
 **/
gboolean
itdb_artwork_add_thumbnail_from_data (Itdb_Artwork *artwork,
				      ItdbThumbType type,
				      const guchar *image_data,
				      gsize image_data_len,
				      gint rotation,
				      GError **error)
{
#ifdef HAVE_GDKPIXBUF
/* This operation doesn't make sense when we can't save thumbnail files */
    Itdb_Thumb *thumb;
    GTimeVal time;

    g_return_val_if_fail (artwork, FALSE);
    g_return_val_if_fail (image_data, FALSE);

    g_get_current_time (&time);

    artwork->artwork_size  = image_data_len;
    artwork->creation_date = time.tv_sec;

    thumb = itdb_thumb_new ();
    thumb->image_data = g_malloc (image_data_len);
    thumb->image_data_len = image_data_len;
    memcpy (thumb->image_data,  image_data, image_data_len);

    thumb->type = type;
    thumb->rotation = rotation;
    artwork->thumbnails = g_list_append (artwork->thumbnails, thumb);

    return TRUE;
#else
    g_set_error (error, 0, -1,
		 _("Artwork support not compiled into libgpod."));
    return FALSE;
#endif
}


/**
 * itdb_artwork_get_thumb_by_type:
 * @artwork: an #Itdb_Artwork
 * @type: type of the #Itdb_Thumb to retrieve
 *
 * Searches @artwork for an #Itdb_Thumb of type @type.
 * 
 * Returns: an #Itdb_Thumb of type @type, or NULL if such a thumbnail couldn't
 * be found
 **/
Itdb_Thumb *itdb_artwork_get_thumb_by_type (Itdb_Artwork *artwork,
					    ItdbThumbType type)
{
    GList *gl;

    g_return_val_if_fail (artwork, NULL);

    for (gl=artwork->thumbnails; gl; gl=gl->next)
    {
	Itdb_Thumb *thumb = gl->data;
	g_return_val_if_fail (thumb, NULL);
	if (thumb->type == type)  return thumb;
    }
    return NULL;
}


/**
 * itdb_thumb_get_filename:
 * @device: an #Itdb_Device
 * @thumb: an #Itdb_Thumb
 *
 * Get filename of thumbnail. If it's a thumbnail on the iPod, return
 * the full path to the ithmb file. Otherwise return the full path to
 * the original file.
 *
 * Return value: newly allocated string containing the absolute path to the 
 * thumbnail file. 
 **/
gchar *itdb_thumb_get_filename (Itdb_Device *device, Itdb_Thumb *thumb)
{
    gchar *artwork_dir, *filename=NULL;

    g_return_val_if_fail (device, NULL);
    g_return_val_if_fail (thumb, NULL);

    /* thumbnail not transferred to the iPod */
    if (thumb->size == 0)
	return g_strdup (thumb->filename);

    if (strlen (thumb->filename) < 2)
    {
	g_print (_("Illegal filename: '%s'.\n"), thumb->filename);
	return NULL;
    }

    if (!device->mountpoint)
    {
	g_print (_("Mountpoint not set.\n"));
	return NULL;
    }
    artwork_dir = itdb_get_artwork_dir (device->mountpoint);
    if (artwork_dir)
    {
	filename = itdb_get_path (artwork_dir, thumb->filename+1);
	g_free (artwork_dir);
    }
    /* FIXME: Hack */
    if( !filename ) {
	artwork_dir = itdb_get_photos_thumb_dir (device->mountpoint);

	if (artwork_dir)
	{
	    const gchar *name_on_disk = strchr( thumb->filename+1, ':');
	    if (name_on_disk)
	    {
		filename = itdb_get_path (artwork_dir, name_on_disk + 1);
	    }
	    g_free (artwork_dir);
	}

    }
    return filename;
}


#if HAVE_GDKPIXBUF
static guchar *
unpack_RGB_565 (guint16 *pixels, guint bytes_len, guint byte_order)
{
	guchar *result;
	guint i;

	g_return_val_if_fail (bytes_len < 2*(G_MAXUINT/3), NULL);

	result = g_malloc ((bytes_len/2) * 3);

	for (i = 0; i < bytes_len/2; i++) {
		guint16 cur_pixel;
		/* FIXME: endianness */
		cur_pixel = get_gint16 (pixels[i], byte_order);
		/* Unpack pixels */
		result[3*i] = (cur_pixel & RED_MASK_565) >> RED_SHIFT_565;
		result[3*i+1] = (cur_pixel & GREEN_MASK_565) >> GREEN_SHIFT_565;
		result[3*i+2] = (cur_pixel & BLUE_MASK_565) >> BLUE_SHIFT_565;
		
		/* Normalize color values so that they use a [0..255] range */
		result[3*i] <<= (8 - RED_BITS_565);
		result[3*i+1] <<= (8 - GREEN_BITS_565);
		result[3*i+2] <<= (8 - BLUE_BITS_565);
	}

	return result;
}

static guchar *
unpack_RGB_555 (guint16 *pixels, guint bytes_len, guint byte_order)
{
	guchar *result;
	guint i;

	g_return_val_if_fail (bytes_len < 2*(G_MAXUINT/3), NULL);

	result = g_malloc ((bytes_len/2) * 3);

	for (i = 0; i < bytes_len/2; i++) {
		guint16 cur_pixel;
		/* FIXME: endianness */
		cur_pixel = get_gint16 (pixels[i], byte_order);
		/* Unpack pixels */
		result[3*i] = (cur_pixel & RED_MASK_555) >> RED_SHIFT_555;
		result[3*i+1] = (cur_pixel & GREEN_MASK_555) >> GREEN_SHIFT_555;
		result[3*i+2] = (cur_pixel & BLUE_MASK_555) >> BLUE_SHIFT_555;
		
		/* Normalize color values so that they use a [0..255] range */
		result[3*i] <<= (8 - RED_BITS_555);
		result[3*i+1] <<= (8 - GREEN_BITS_555);
		result[3*i+2] <<= (8 - BLUE_BITS_555);
	}

	return result;
}

static guchar *
unpack_RGB_888 (guint16 *pixels, guint bytes_len, guint byte_order)
{
	guchar *result;
	guint i;
	guint32 *pixels32;

	result = g_malloc ((bytes_len/4) * 3);

	pixels32 = (guint32 *)pixels;

	for (i = 0; i < bytes_len/4; i++) {
		guint32 cur_pixel;
		/* FIXME: endianness */
		cur_pixel = get_gint32 (pixels32[i], byte_order);
		/* Unpack pixels */
		result[3*i] = (cur_pixel & RED_MASK_888) >> RED_SHIFT_888;
		result[3*i+1] = (cur_pixel & GREEN_MASK_888) >> GREEN_SHIFT_888;
		result[3*i+2] = (cur_pixel & BLUE_MASK_888) >> BLUE_SHIFT_888;
		
		/* Normalize color values so that they use a [0..255] range */
		/* (not necessary for 888 encoding) */
/* 		result[3*i] <<= (8 - RED_BITS_888); */
/* 		result[3*i+1] <<= (8 - GREEN_BITS_888); */
/* 		result[3*i+2] <<= (8 - BLUE_BITS_888); */
	}

	return result;
}


static guint16 *rearrange_pixels (guint16 *pixels_s, guint16 *pixels_d,
				  gint width, gint height, gint row_stride)
{
    g_return_val_if_fail (width == height, pixels_d);

    if (pixels_d == NULL)
    {
	pixels_d = g_malloc0 (sizeof (guint16)*width*height);
    }

    if (width == 1)
    {
	*pixels_d = *pixels_s;
    }
    else
    {
	rearrange_pixels (pixels_s + 0,
			  pixels_d + 0 + 0,
			  width/2, height/2,
			  row_stride);
	rearrange_pixels (pixels_s + (width/2)*(height/2),
			  pixels_d + (height/2)*row_stride + 0,
			  width/2, height/2,
			  row_stride);
	rearrange_pixels (pixels_s + 2*(width/2)*(height/2),
			  pixels_d + width/2,
			  width/2, height/2,
			  row_stride);
	rearrange_pixels (pixels_s + 3*(width/2)*(height/2),
			  pixels_d + (height/2)*row_stride + width/2,
			  width/2, height/2,
			  row_stride);
    }
    
    return pixels_d;
}


static guchar *
unpack_rec_RGB_555 (guint16 *pixels, guint bytes_len, guint byte_order,
		    gint width, gint height)
{
	guchar *result;
	guint16 *use_pixels;
	gboolean free_use_pixels = FALSE;
	guint16 *pixels_arranged;

	guint i;

	g_return_val_if_fail (bytes_len < 2*(G_MAXUINT/3), NULL);
	g_return_val_if_fail (2*width*height < G_MAXUINT, NULL);
	g_return_val_if_fail (width==height, NULL);

	if (2*width*height > bytes_len)
	{
	    use_pixels = g_malloc0 (2*width*height);
	    g_memmove (use_pixels, pixels, bytes_len);
	    free_use_pixels = TRUE;
	}
	else
	{
	    use_pixels = pixels;
	}

	pixels_arranged = rearrange_pixels (use_pixels, NULL,
					    width, height, width);

	if (pixels_arranged == NULL)
	{
	    return NULL;
	}

	result = g_malloc ((bytes_len/2) * 3);

	for (i = 0; i < bytes_len/2; i++) {
		guint16 cur_pixel;
		/* FIXME: endianness */
		cur_pixel = get_gint16 (pixels_arranged[i], byte_order);
		/* Unpack pixels */
		result[3*i] = (cur_pixel & RED_MASK_555) >> RED_SHIFT_555;
		result[3*i+1] = (cur_pixel & GREEN_MASK_555) >> GREEN_SHIFT_555;
		result[3*i+2] = (cur_pixel & BLUE_MASK_555) >> BLUE_SHIFT_555;
		
		/* Normalize color values so that they use a [0..255] range */
		result[3*i] <<= (8 - RED_BITS_555);
		result[3*i+1] <<= (8 - GREEN_BITS_555);
		result[3*i+2] <<= (8 - BLUE_BITS_555);
	}

	g_free (pixels_arranged);
	if (free_use_pixels)
	{
	    g_free (use_pixels);
	}

	return result;
}


#if DEBUG_ARTWORK
static guchar *
unpack_experimental (guint16 *pixels, guint bytes_len, guint byte_order,
		     gint width, gint height)
{
	guchar *result;
	guint i;
	guint32 *rpixels;

	g_return_val_if_fail (bytes_len < (G_MAXUINT/3), NULL);

	result = g_malloc ((bytes_len/4) * 3);

	rpixels = (guint32 *)pixels;

	for (i = 0; i < bytes_len/4; i++) {
		guint32 cur_pixel;
		/* FIXME: endianness */
		cur_pixel = get_gint32 (rpixels[i], byte_order);
printf ("%8x\n", cur_pixel);
		/* Unpack pixels */
		result[3*i] = (cur_pixel & RED_MASK_888) >> RED_SHIFT_888;
		result[3*i+1] = (cur_pixel & GREEN_MASK_888) >> GREEN_SHIFT_888;
		result[3*i+2] = (cur_pixel & BLUE_MASK_888) >> BLUE_SHIFT_888;
		
		/* Normalize color values so that they use a [0..255] range */
		/* (not really necessary for 888 encoding) */
/* 		result[3*i] <<= (8 - RED_BITS_888); */
/* 		result[3*i+1] <<= (8 - GREEN_BITS_888); */
/* 		result[3*i+2] <<= (8 - BLUE_BITS_888); */
	}

	return result;
}
#endif


/* limit8bit() and unpack_UYVY() adapted from imgconvert.c from the
 * GPixPod project (www.gpixpod.org) */
static gint limit8bit (float x)
{
    if(x >= 255)
    {
	return 255;
    }
    if(x <= 0)
    {
	return 0;
    }
    return x;
}
static guchar *
unpack_UYVY (guchar *yuvdata, gint bytes_len, guint byte_order,
	     gint width, gint height)
{
    gint imgsize = width*3*height;
    guchar* rgbdata;
    gint halfimgsize = imgsize/2;
    gint halfyuv = halfimgsize/3*2;
    gint x = 0;
    gint z = 0;
    gint z2 = 0;
    gint u0, y0, v0, y1, u1, y2, v1, y3;
    gint h = 0;

    g_return_val_if_fail (bytes_len < 2*(G_MAXUINT/3), NULL);
/*     printf ("w=%d h=%d s=%d\n", width, height, bytes_len); */
    g_return_val_if_fail (width * height * 2 == bytes_len, NULL);

    rgbdata =  g_malloc(imgsize);

    while(h < height)
    {
	gint w = 0;
	if((h % 2) == 0)
	{
	    while(w < width)
	    {
		u0 = yuvdata[z];
		y0 = yuvdata[z+1];
		v0 = yuvdata[z+2];
		y1 = yuvdata[z+3];
		rgbdata[x] = limit8bit((y0-16)*1.164 + (v0-128)*1.596);/*R0*/
		rgbdata[x+1] = limit8bit((y0-16)*1.164 - (v0-128)*0.813 - (u0-128)*0.391);/*G0*/
		rgbdata[x+2] = limit8bit((y0-16)*1.164 + (u0-128)*2.018);/*B0*/
		rgbdata[x+3] = limit8bit((y0-16)*1.164 + (v0-128)*1.596);/*R1*/
		rgbdata[x+4] = limit8bit((y1-16)*1.164 - (v0-128)*0.813 - (u0-128)*0.391);/*G1*/
		rgbdata[x+5] = limit8bit((y1-16)*1.164 + (u0-128)*2.018);/*B1*/
		w += 2;
		z += 4;
		x += 6;
	    }
	}
	else
	{
	    while(w < width)
	    {
		u1 = yuvdata[halfyuv+z2];
		y2 = yuvdata[halfyuv+z2+1];
		v1 = yuvdata[halfyuv+z2+2];
		y3 = yuvdata[halfyuv+z2+3];
		rgbdata[x] = limit8bit((y2-16)*1.164 + (v1-128)*1.596);
		rgbdata[x+1] = limit8bit((y2-16)*1.164 - (v1-128)*0.813 - (u1-128)*0.391);
		rgbdata[x+2] = limit8bit((y2-16)*1.164 + (u1-128)*2.018);
		rgbdata[x+3] = limit8bit((y2-16)*1.164 + (v1-128)*1.596);
		rgbdata[x+4] = limit8bit((y3-16)*1.164 - (v1-128)*0.813 - (u1-128)*0.391);
		rgbdata[x+5] = limit8bit((y3-16)*1.164 + (u1-128)*2.018);
		w += 2;
		z2 += 4;
		x += 6;
	    }
	}
	h++;
    }
    return rgbdata;
}

static guchar *
get_pixel_data (Itdb_Device *device, Itdb_Thumb *thumb)
{
	gchar *filename = NULL;
	guchar *result = NULL;
	FILE *f = NULL;
	gint res;

	g_return_val_if_fail (thumb, NULL);
	g_return_val_if_fail (thumb->filename, NULL);

	/* thumb->size is read as a guint32 from the iPod, so no overflow
	 * can occur here
	 */
	result = g_malloc (thumb->size);

	filename = itdb_thumb_get_filename (device, thumb);

	if (!filename)
	{
	    g_print (_("Could not find on iPod: '%s'\n"),
		     thumb->filename);
	    goto error;
	}

	f = fopen (filename, "r");
	if (f == NULL) {
		g_print ("Failed to open %s: %s\n", 
			 filename, strerror (errno));
		goto error;
	}

	res = fseek (f, thumb->offset, SEEK_SET);
	if (res != 0) {
		g_print ("Seek to %d failed on %s: %s\n",
			 thumb->offset, thumb->filename, strerror (errno));
		goto error;
	}

	res = fread (result, thumb->size, 1, f);
	if (res != 1) {
		g_print ("Failed to read %u bytes from %s: %s\n", 
			 thumb->size, thumb->filename, strerror (errno));
		goto error;
	}

	goto cleanup;

  error:
	g_free (result);
	result = NULL;
  cleanup:
	if (f != NULL) {
		fclose (f);
	}
	g_free (filename);

	return result;
}

static guchar *
itdb_thumb_get_rgb_data (Itdb_Device *device, Itdb_Thumb *thumb)
{
#if 0
    #include <unistd.h>
    #include <fcntl.h>
    static gint i=0;
    int fd;
    gchar *name;
#endif
	void *pixels_raw;
	guchar *pixels=NULL;
	const Itdb_ArtworkFormat *img_info;

	g_return_val_if_fail (device, NULL);
	g_return_val_if_fail (thumb, NULL);
	g_return_val_if_fail (thumb->size != 0, NULL);
	img_info = itdb_get_artwork_info_from_type (device, thumb->type);
	g_return_val_if_fail (img_info, NULL);
	
	pixels_raw = get_pixel_data (device, thumb);

#if 0
    name = g_strdup_printf ("thumb_%03d.raw", i++);
    fd = creat (name, S_IRWXU|S_IRWXG|S_IRWXO);
    write (fd, pixels_raw, thumb->size);
    close (fd);
    g_free (name);
#endif
	if (pixels_raw == NULL) {
		return NULL;
	}

	switch (img_info->format)
	{
	case THUMB_FORMAT_RGB565_LE_90:
	case THUMB_FORMAT_RGB565_BE_90:
	    /* FIXME: actually the previous two might require
	       different treatment (used on iPod Photo for the full
	       screen photo thumbnail) */
	case THUMB_FORMAT_RGB565_LE:
	case THUMB_FORMAT_RGB565_BE:
	    pixels = unpack_RGB_565 (pixels_raw, thumb->size,
				     itdb_thumb_get_byteorder (img_info->format));
	    break;
	case THUMB_FORMAT_RGB555_LE_90:
	case THUMB_FORMAT_RGB555_BE_90:
	    /* FIXME: actually the previous two might require
	       different treatment (used on iPod Photo for the full
	       screen photo thumbnail) */
	case THUMB_FORMAT_RGB555_LE:
	case THUMB_FORMAT_RGB555_BE:
	    pixels = unpack_RGB_555 (pixels_raw, thumb->size,
				     itdb_thumb_get_byteorder (img_info->format));
	    break;
	case THUMB_FORMAT_RGB888_LE_90:
	case THUMB_FORMAT_RGB888_BE_90:
	    /* FIXME: actually the previous two might require
	       different treatment */
	case THUMB_FORMAT_RGB888_LE:
	case THUMB_FORMAT_RGB888_BE:
	    pixels = unpack_RGB_888 (pixels_raw, thumb->size,
				     itdb_thumb_get_byteorder (img_info->format));
	    break;
	case THUMB_FORMAT_REC_RGB555_LE_90:
	case THUMB_FORMAT_REC_RGB555_BE_90:
	    /* FIXME: actually the previous two might require
	       different treatment (used on iPod Photo for the full
	       screen photo thumbnail) */
	case THUMB_FORMAT_REC_RGB555_LE:
	case THUMB_FORMAT_REC_RGB555_BE:
	    pixels = unpack_rec_RGB_555 (pixels_raw, thumb->size,
					 itdb_thumb_get_byteorder (img_info->format),
					 img_info->width, img_info->height);
	    break;
	case THUMB_FORMAT_EXPERIMENTAL_LE:
	case THUMB_FORMAT_EXPERIMENTAL_BE:
#if DEBUG_ARTWORK
	    pixels = unpack_experimental (pixels_raw, thumb->size,
					  itdb_thumb_get_byteorder (img_info->format),
					  img_info->width, img_info->height);
	    break;
#endif
	case THUMB_FORMAT_UYVY_LE:
	case THUMB_FORMAT_UYVY_BE:
	    pixels = unpack_UYVY (pixels_raw, thumb->size,
				  itdb_thumb_get_byteorder (img_info->format),
				  img_info->width, img_info->height);
	    break;
	}
	g_free (pixels_raw);

	return pixels;

}
#endif



/**
 * itdb_thumb_get_gdk_pixbuf:
 * @device: an #Itdb_Device
 * @thumb: an #Itdb_Thumb
 * 
 * Converts @thumb to a #GdkPixbuf.
 * Since we want to have gdk-pixbuf dependency optional, a generic
 * gpointer is returned which you have to cast to a #GdkPixbuf using 
 * GDK_PIXBUF() yourself. 
 *
 * Return value: a #GdkPixbuf that must be unreffed with gdk_pixbuf_unref()
 * after use, or NULL if the creation of the gdk-pixbuf failed or if 
 * libgpod was compiled without gdk-pixbuf support.
 **/
gpointer
itdb_thumb_get_gdk_pixbuf (Itdb_Device *device, Itdb_Thumb *thumb)
{
#if HAVE_GDKPIXBUF
    GdkPixbuf *pixbuf=NULL;
    guchar *pixels;
    const Itdb_ArtworkFormat *img_info=NULL;

    g_return_val_if_fail (thumb, NULL);

    /* If we are dealing with an iPod (device != NULL), use default
       image dimensions as used on the iPod in question.

       If we are not dealing with an iPod, we can only return a pixmap
       for thumbnails "not transferred to the iPod" (thumb->size ==
       0).
    */
    if (device != NULL)
    {
	img_info = itdb_get_artwork_info_from_type (device, thumb->type);
    }

    if (thumb->size == 0)
    {   /* thumbnail has not yet been transferred to the iPod */
	gint width=0, height=0;

	if (img_info != NULL)
	{   /* use image dimensions from iPod */
	    width = img_info->width;
	    height = img_info->height;
	}
	else
	{   /* use default dimensions */
	    /* FIXME: better way to use the ipod_color dimensions? */
	    switch (thumb->type)
	    {
	    case ITDB_THUMB_COVER_SMALL:
		width =  56;  height =  56;  break;
	    case ITDB_THUMB_COVER_LARGE:
		width = 140;  height = 140;  break;
	    case ITDB_THUMB_PHOTO_SMALL:
		width =  42;  height =  30;  break;
	    case ITDB_THUMB_PHOTO_LARGE:
		width = 130;  height =  88;  break;
	    case ITDB_THUMB_PHOTO_FULL_SCREEN:
		width = 220;  height = 176;  break;
	    case ITDB_THUMB_PHOTO_TV_SCREEN:
		width = 720;  height = 480;  break;
	    case ITDB_THUMB_COVER_XLARGE:
		width = 320;  height = 320;  break;
	    case ITDB_THUMB_COVER_MEDIUM:
		width = 128;  height = 128;  break;
	    case ITDB_THUMB_COVER_SMEDIUM:
		width = 88;  height = 88;  break;
	    case ITDB_THUMB_COVER_XSMALL:
		width = 56;  height = 56;  break;
	    }
	    if (width == 0)
	    {
		width = 140;
		height = 140;
	    }
	}

	if (thumb->filename)
	{   /* read data from filename */
	    pixbuf = gdk_pixbuf_new_from_file_at_size (thumb->filename, 
						       width, height,
						       NULL);
	}
	else if (thumb->image_data)
	{   /* use data stored in image_data */
	    	GdkPixbufLoader *loader = gdk_pixbuf_loader_new ();
		g_return_val_if_fail (loader, FALSE);
		gdk_pixbuf_loader_set_size (loader,
					    width, height);
		gdk_pixbuf_loader_write (loader,
					 thumb->image_data,
					 thumb->image_data_len,
					 NULL);
		gdk_pixbuf_loader_close (loader, NULL);
		pixbuf = gdk_pixbuf_loader_get_pixbuf (loader);
		if (pixbuf)
		    g_object_ref (pixbuf);
		g_object_unref (loader);
	}
	else if (thumb->pixbuf)
	{   /* use pixbuf data */
	    pixbuf = gdk_pixbuf_scale_simple (thumb->pixbuf,
					      width, height,
					      GDK_INTERP_BILINEAR);
	}

	if (!pixbuf)
	{
	    return NULL;
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
    }
    else
    {
	/* pixbuf is already on the iPod -> read from there */
	GdkPixbuf *pixbuf_full;
	GdkPixbuf *pixbuf_sub;
	gint pad_x = thumb->horizontal_padding;
	gint pad_y = thumb->vertical_padding;
	gint width = thumb->width;
	gint height = thumb->height;

/*	printf ("hp%d vp%d w%d h%d\n",
	       pad_x, pad_y, width, height);*/

	if (img_info == NULL)
	{
	    g_print (_("Unable to retrieve thumbnail (appears to be on iPod, but no image info available): type: %d, filename: '%s'\n"),
		     thumb->type, thumb->filename);
	    return NULL;
	}

	pixels = itdb_thumb_get_rgb_data (device, thumb);
	if (pixels == NULL)
	{
	    return NULL;
	}

	pixbuf_full =
	    gdk_pixbuf_new_from_data (pixels,
				      GDK_COLORSPACE_RGB,
				      FALSE, 8,
				      img_info->width, img_info->height,
				      img_info->width*3,
				      (GdkPixbufDestroyNotify)g_free,
				      NULL);
	/* !! do not g_free(pixels) here: it will be freed when doing a
	 * gdk_pixbuf_unref() on the GdkPixbuf !! */

	/* Remove padding from the pixmap and/or cut the pixmap to the
	   right size. */

	/* Negative offsets indicate that part of the image was cut
	   off at the left/top. thumb->width/height include that part
	   of the image. Positive offsets indicate that part of the
	   thumbnail are padded in black. thumb->width/height also
	   include that part of the image -> reduce width and height
	   by the absolute value of the padding */
	width = width - abs (pad_x);
	height = height - abs (pad_y);
	/* And throw out "negative" padding */
	if (pad_x < 0)		pad_x = 0;
	if (pad_y < 0)		pad_y = 0;
	/* Width/height might still be larger than
	   img_info->width/height, indicating that part of the image
	   was cut off at the right/bottom (similar to negative
	   padding above). Adjust width/height accordingly. */
	if (pad_x + width > img_info->width)
	    width = img_info->width - pad_x;
	if (pad_y + height > img_info->height)
	    height = img_info->height - pad_y;

#if DEBUG_ARTWORK
 	printf ("px=%2d py=%2d x=%3d y=%3d\n", pad_x, pad_y, width, height);
#endif

	pixbuf_sub = gdk_pixbuf_new_subpixbuf (pixbuf_full,
					       pad_x, pad_y,
					       width, height);
	pixbuf = gdk_pixbuf_copy (pixbuf_sub);
	gdk_pixbuf_unref (pixbuf_full);
	gdk_pixbuf_unref (pixbuf_sub);
    }

    return pixbuf;
#else
    return NULL;
#endif
}

/**
 * itdb_thumb_new:
 * 
 * Creates a new #Itdb_Thumb
 *
 * Return Value: newly allocated #Itdb_Thumb to be freed with itdb_thumb_free()
 * after use
 **/
Itdb_Thumb *itdb_thumb_new (void)
{
    Itdb_Thumb *thumb = g_new0 (Itdb_Thumb, 1);
    return thumb;
}

/** 
 * itdb_thumb_free:
 * @thumb: an #Itdb_Thumb
 *
 * Frees the memory used by @thumb
 **/
void itdb_thumb_free (Itdb_Thumb *thumb)
{
    g_return_if_fail (thumb);

    g_free (thumb->image_data);
#ifdef HAVE_GDKPIXBUF
    if (thumb->pixbuf) {
        g_object_unref (G_OBJECT (thumb->pixbuf));
    }
#endif
    g_free (thumb->filename);
    g_free (thumb);
}


/** 
 * itdb_thumb_duplicate:
 * @thumb: an #Itdb_Thumb
 *
 * Duplicates the data contained in @thumb
 *
 * Return value: a newly allocated copy of @thumb to be freed with 
 * itdb_thumb_free() after use
 **/
Itdb_Thumb *itdb_thumb_duplicate (Itdb_Thumb *thumb)
{
    Itdb_Thumb *new_thumb;

    g_return_val_if_fail (thumb, NULL);

    new_thumb = itdb_thumb_new ();
    memcpy (new_thumb, thumb, sizeof (Itdb_Thumb));
    new_thumb->filename = g_strdup (thumb->filename);
 
    if (thumb->image_data)
    {
	/* image_data_len already copied with the memcpy above */
	new_thumb->image_data = g_malloc (thumb->image_data_len);
	memcpy (new_thumb->image_data, thumb->image_data,
		new_thumb->image_data_len);
    }
#ifdef HAVE_GDKPIXBUF
    if (thumb->pixbuf) {
        g_object_ref (G_OBJECT (thumb->pixbuf));
    }
#endif
    return new_thumb;
}


G_GNUC_INTERNAL gint
itdb_thumb_get_byteorder (const ItdbThumbFormat format)
{
    switch (format)
    {
    case THUMB_FORMAT_UYVY_LE:
    case THUMB_FORMAT_RGB565_LE:
    case THUMB_FORMAT_RGB565_LE_90:
    case THUMB_FORMAT_RGB555_LE:
    case THUMB_FORMAT_RGB555_LE_90:
    case THUMB_FORMAT_RGB888_LE:
    case THUMB_FORMAT_RGB888_LE_90:
    case THUMB_FORMAT_REC_RGB555_LE:
    case THUMB_FORMAT_REC_RGB555_LE_90:
    case THUMB_FORMAT_EXPERIMENTAL_LE:
	return G_LITTLE_ENDIAN;
    case THUMB_FORMAT_UYVY_BE:
    case THUMB_FORMAT_RGB565_BE:
    case THUMB_FORMAT_RGB565_BE_90:
    case THUMB_FORMAT_RGB555_BE:
    case THUMB_FORMAT_RGB555_BE_90:
    case THUMB_FORMAT_RGB888_BE:
    case THUMB_FORMAT_RGB888_BE_90:
    case THUMB_FORMAT_REC_RGB555_BE:
    case THUMB_FORMAT_REC_RGB555_BE_90:
    case THUMB_FORMAT_EXPERIMENTAL_BE:
	return G_BIG_ENDIAN;
    }
    g_return_val_if_reached (-1);
}

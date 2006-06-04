/* Time-stamp: <2006-06-04 17:30:39 jcs>
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
 * Removes @thumb from @artwork. The memory used by @thumb isn't freed.
 **/
void
itdb_artwork_remove_thumbnail (Itdb_Artwork *artwork, Itdb_Thumb *thumb)
{
    g_return_if_fail (artwork);
    g_return_if_fail (thumb);

    artwork->thumbnails = g_list_remove (artwork->thumbnails, thumb);
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
 *
 * Appends a thumbnail of type @type to existing thumbnails in @artwork. No 
 * data is read from @filename yet, the file will be when @artwork is saved to
 * disk. @filename must still exist when that happens.
 *
 * Return value: TRUE if the thumbnail could be successfully added, FALSE
 * otherwise
 **/
gboolean
itdb_artwork_add_thumbnail (Itdb_Artwork *artwork,
			    ItdbThumbType type,
			    const char *filename)
{
#ifdef HAVE_GDKPIXBUF
/* This operation doesn't make sense when we can't save thumbnail files */
    struct stat statbuf;
    Itdb_Thumb *thumb;

    g_return_val_if_fail (artwork, FALSE);
    g_return_val_if_fail (filename, FALSE);

    if (g_stat  (filename, &statbuf) != 0) {
	return FALSE;
    }

    artwork->artwork_size  = statbuf.st_size;
    artwork->creation_date = statbuf.st_mtime;

    thumb = itdb_thumb_new ();
    thumb->filename = g_strdup (filename);
    thumb->type = type;
    artwork->thumbnails = g_list_append (artwork->thumbnails, thumb);

    return TRUE;
#else
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
	    filename = itdb_get_path (artwork_dir, strchr( thumb->filename+1, ':') + 1);
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
	if (result == NULL) {
		return NULL;
	}
	for (i = 0; i < bytes_len/2; i++) {
		guint16 cur_pixel;
		/* FIXME: endianness */
		cur_pixel = get_gint16 (pixels[i], byte_order);
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
	void *pixels565;
	guchar *pixels;
	guint byte_order;
	const Itdb_ArtworkFormat *img_info;

	g_return_val_if_fail (device, NULL);
	g_return_val_if_fail (thumb, NULL);
	g_return_val_if_fail (thumb->size != 0, NULL);
	img_info = itdb_get_artwork_info_from_type (device, thumb->type);
	g_return_val_if_fail (img_info, NULL);
	
	pixels565 = get_pixel_data (device, thumb);
	if (pixels565 == NULL) {
		return NULL;
	}

	byte_order = device->byte_order; 
	/* Swap the byte order on full screen nano photos */
	if (img_info->correlation_id == 1023)
	{
	    if (byte_order == G_LITTLE_ENDIAN)
		byte_order = G_BIG_ENDIAN;
	    else
		byte_order = G_LITTLE_ENDIAN; 
	}

	pixels = unpack_RGB_565 (pixels565, thumb->size, byte_order);
	g_free (pixels565);

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
	    }
	    if (width == 0)
	    {
		width = 140;
		height = 140;
	    }
	}

	pixbuf = gdk_pixbuf_new_from_file_at_size (thumb->filename, 
						   width, height,
						   NULL);
	if (!pixbuf)
	    return NULL;

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
	    g_print (_("Unable to retreive thumbnail (appears to be on iPod, but no image info available): type: %d, filename: '%s'\n"),
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
 
    return new_thumb;
}

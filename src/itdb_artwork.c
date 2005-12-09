/* Time-stamp: <2005-12-10 00:03:26 jcs>
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
#include <errno.h>
#include <stdio.h>
#include <string.h>
#if HAVE_GDKPIXBUF
#include <gdk-pixbuf/gdk-pixbuf.h>
#endif
#include <glib/gi18n-lib.h>


Itdb_Artwork *itdb_artwork_new (void)
{
    Itdb_Artwork *artwork = g_new0 (Itdb_Artwork, 1);
    return artwork;
}


void itdb_artwork_free (Itdb_Artwork *artwork)
{
    g_return_if_fail (artwork);
    itdb_artwork_remove_thumbnails (artwork);
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

Itdb_Artwork *itdb_artwork_duplicate (Itdb_Artwork *artwork)
{
    Itdb_Artwork *dup;
    g_return_val_if_fail (artwork, NULL);

    dup = itdb_artwork_new ();

    memcpy (dup, artwork, sizeof (Itdb_Artwork));

    dup->thumbnails = dup_thumbnails (artwork->thumbnails);

    return dup;
}


/* Remove @thumb in @artwork */
void
itdb_artwork_remove_thumbnail (Itdb_Artwork *artwork, Itdb_Thumb *thumb)
{
    g_return_if_fail (artwork);
    g_return_if_fail (thumb);

    artwork->thumbnails = g_list_remove (artwork->thumbnails, thumb);
}


/* Remove all thumbnails in @artwork */
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




/* Append thumbnail of type @type to existing thumbnails in @artwork */
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

    artwork->artwork_size = statbuf.st_size;

    thumb = itdb_thumb_new ();
    thumb->filename = g_strdup (filename);
    thumb->type = type;
    artwork->thumbnails = g_list_append (artwork->thumbnails, thumb);

    return TRUE;
#else
    return FALSE;
#endif
}


/* Return a pointer to the Itdb_Thumb of type @type or NULL when it
 * does not exist */
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


/* Get filename of thumbnail. If it's a thumbnail on the iPod, return
   the full path to the ithmb file. Otherwise return the full path to
   the original file.
   g_free() when not needed any more.
*/
gchar *itdb_thumb_get_filename (IpodDevice *device, Itdb_Thumb *thumb)
{
    gchar *paths[] = {"iPod_Control", "Artwork", NULL, NULL};
    gchar *filename, *mountpoint;

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

    g_object_get (G_OBJECT (device),
		  "mount-point", &mountpoint,
		  NULL);

    if (!mountpoint)
    {
	g_print (_("Mountpoint not set.\n"));
	return NULL;
    }

    paths[2] = thumb->filename+1;
    filename = itdb_resolve_path (mountpoint, (const char **)paths);

    return filename;
}


#if HAVE_GDKPIXBUF
static guchar *
unpack_RGB_565 (guint16 *pixels, guint bytes_len)
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


static guchar *
get_pixel_data (IpodDevice *device, Itdb_Thumb *thumb)
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
itdb_thumb_get_rgb_data (IpodDevice *device, Itdb_Thumb *thumb)
{
	void *pixels565;
	guchar *pixels;

	g_return_val_if_fail (device, NULL);
	g_return_val_if_fail (thumb, NULL);

	/* no rgb pixel data available (FIXME: calculate from real
	 * image file) */
	if (thumb->size == 0)  return NULL;

	pixels565 = get_pixel_data (device, thumb);
	if (pixels565 == NULL) {
		return NULL;
	}
	
	pixels = unpack_RGB_565 (pixels565, thumb->size);
	g_free (pixels565);

	return pixels;

}
#endif



/* Convert the pixeldata in @thumb to a GdkPixbuf.
   Since we want to have gdk-pixbuf dependency optional, a generic
   gpointer is returned which you have to cast to (GdkPixbuf *)
   yourself. If gdk-pixbuf is not installed the NULL pointer is
   returned.
   The returned GdkPixbuf must be freed with gdk_pixbuf_unref() after
   use. */
gpointer
itdb_thumb_get_gdk_pixbuf (IpodDevice *device, Itdb_Thumb *thumb)
{
#if HAVE_GDKPIXBUF
    GdkPixbuf *pixbuf;
    guchar *pixels;
    const IpodArtworkFormat *img_info;

    g_return_val_if_fail (device, NULL);
    g_return_val_if_fail (thumb, NULL);

    img_info = ipod_get_artwork_info_from_type (device, thumb->type);

    if (img_info == NULL)
    {
	g_print (_("Unable to obtain image info on thumb (type: %d, filename: '%s'\n)"), thumb->type, thumb->filename);
	return NULL;
    }

    if (thumb->size == 0)
    {   /* pixbuf has not yet been transfered to the iPod */
	gint width, height;

	pixbuf = gdk_pixbuf_new_from_file_at_size (thumb->filename, 
						   img_info->width, 
						   img_info->height,
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

	return pixbuf;
    }

    /* pixbuf is already on the iPod -> read from there */
    pixels = itdb_thumb_get_rgb_data (device, thumb);
    if (pixels == NULL)
    {
	return NULL;
    }

    pixbuf = gdk_pixbuf_new_from_data (pixels, GDK_COLORSPACE_RGB, FALSE,
				       8, thumb->width, thumb->height,
				       img_info->width*3,
				       (GdkPixbufDestroyNotify)g_free,
				       NULL);


    /* !! do not g_free(pixels) here: it will be freed when doing a
     * gdk_pixbuf_unref() on the GdkPixbuf !! */

    return pixbuf;
#else
    return NULL;
#endif
}


Itdb_Thumb *itdb_thumb_new (void)
{
    Itdb_Thumb *thumb = g_new0 (Itdb_Thumb, 1);
    return thumb;
}


void itdb_thumb_free (Itdb_Thumb *thumb)
{
    g_return_if_fail (thumb);

    g_free (thumb->filename);
    g_free (thumb);
}

Itdb_Thumb *itdb_thumb_duplicate (Itdb_Thumb *thumb)
{
    Itdb_Thumb *new_thumb;

    g_return_val_if_fail (thumb, NULL);

    new_thumb = itdb_thumb_new ();
    memcpy (new_thumb, thumb, sizeof (Itdb_Thumb));
    new_thumb->filename = g_strdup (thumb->filename);
 
    return new_thumb;
}

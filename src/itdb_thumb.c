/*
|  Copyright (C) 2007 Christophe Fergeau <teuf at gnome org>
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
*/
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <string.h>
#include <glib/gi18n-lib.h>
#include "itdb_private.h"
#include "itdb_thumb.h"

#ifdef HAVE_GDKPIXBUF
#include <gdk-pixbuf/gdk-pixbuf.h>
#endif

Itdb_Thumb *itdb_thumb_new_from_file (const gchar *filename)
{
    Itdb_Thumb_File *thumb_file;
    Itdb_Thumb *thumb;

    thumb_file = g_new0 (Itdb_Thumb_File, 1);
    thumb = (Itdb_Thumb *)thumb_file;
    thumb->data_type = ITDB_THUMB_TYPE_FILE;
    thumb_file->filename = g_strdup (filename);

    return thumb;
}


Itdb_Thumb *itdb_thumb_new_from_data (const guchar *data, gsize len)
{
    Itdb_Thumb_Memory *thumb_memory;
    Itdb_Thumb *thumb;

    thumb_memory = g_new0 (Itdb_Thumb_Memory, 1);
    thumb = (Itdb_Thumb *)thumb_memory;
    thumb->data_type = ITDB_THUMB_TYPE_MEMORY;
    thumb_memory->image_data = g_memdup (data, len);
    thumb_memory->image_data_len = len;

    return thumb;
}


#ifdef HAVE_GDKPIXBUF
Itdb_Thumb *itdb_thumb_new_from_pixbuf (gpointer pixbuf)
{
    Itdb_Thumb_Pixbuf *thumb_pixbuf;
    Itdb_Thumb *thumb;

    thumb_pixbuf = g_new0 (Itdb_Thumb_Pixbuf, 1);
    thumb = (Itdb_Thumb *)thumb_pixbuf;
    thumb->data_type = ITDB_THUMB_TYPE_PIXBUF;
    thumb_pixbuf->pixbuf = g_object_ref (G_OBJECT (pixbuf));

    return thumb;
}
#else
Itdb_Thumb *itdb_thumb_new_from_pixbuf (gpointer pixbuf)
{
    return NULL;
}
#endif

Itdb_Thumb_Ipod_Item *itdb_thumb_new_item_from_ipod (const Itdb_ArtworkFormat *format)
{
    Itdb_Thumb_Ipod_Item *thumb_ipod;

    thumb_ipod = g_new0 (Itdb_Thumb_Ipod_Item, 1);
    thumb_ipod->format = format;

    return thumb_ipod;
}

G_GNUC_INTERNAL Itdb_Thumb *itdb_thumb_ipod_new (void)
{
    Itdb_Thumb *thumb;

    thumb = (Itdb_Thumb *)g_new0 (Itdb_Thumb_Ipod, 1);
    thumb->data_type = ITDB_THUMB_TYPE_IPOD;

    return thumb;
}

static void itdb_thumb_ipod_item_free (Itdb_Thumb_Ipod_Item *item)
{
    g_free (item->filename);
    g_free (item);
}

/**
 * itdb_thumb_free:
 * @thumb: an #Itdb_Thumb
 *
 * Frees the memory used by @thumb
 *
 * Since: 0.3.0
 */
void itdb_thumb_free (Itdb_Thumb *thumb)
{
    g_return_if_fail (thumb);
    switch (thumb->data_type) {
        case ITDB_THUMB_TYPE_FILE: {
            Itdb_Thumb_File *thumb_file = (Itdb_Thumb_File *)thumb;
            g_free (thumb_file->filename);
            break;
        }
        case ITDB_THUMB_TYPE_MEMORY: {
            Itdb_Thumb_Memory *thumb_memory = (Itdb_Thumb_Memory *)thumb;
            g_free (thumb_memory->image_data);
            break;
        }
#ifdef HAVE_GDKPIXBUF
        case ITDB_THUMB_TYPE_PIXBUF: {
            Itdb_Thumb_Pixbuf *thumb_pixbuf = (Itdb_Thumb_Pixbuf *)thumb;
            if (thumb_pixbuf->pixbuf) {
                g_object_unref (G_OBJECT (thumb_pixbuf->pixbuf));
            }
            break;
        }
#else
	case ITDB_THUMB_TYPE_PIXBUF:
            g_assert_not_reached();
#endif
        case ITDB_THUMB_TYPE_IPOD: {
            Itdb_Thumb_Ipod *thumb_ipod = (Itdb_Thumb_Ipod *)thumb;
            g_list_foreach (thumb_ipod->thumbs,
                            (GFunc)itdb_thumb_ipod_item_free,
                            NULL);
            g_list_free (thumb_ipod->thumbs);
            break;
        }
        case ITDB_THUMB_TYPE_INVALID:
            g_assert_not_reached ();
    }
    g_free (thumb);
}


static Itdb_Thumb_Ipod_Item *
itdb_thumb_ipod_item_duplicate (Itdb_Thumb_Ipod_Item *item)
{
    Itdb_Thumb_Ipod_Item *new_item;

    g_return_val_if_fail (item != NULL, NULL);

    new_item = itdb_thumb_new_item_from_ipod (item->format);

    new_item->filename = g_strdup (item->filename);
    new_item->offset = item->offset;
    new_item->size   = item->size;
    new_item->width  = item->width;
    new_item->height = item->height;
    new_item->horizontal_padding = item->horizontal_padding;
    new_item->vertical_padding = item->vertical_padding;

    return new_item;
}

/**
 * itdb_thumb_duplicate:
 * @thumb: an #Itdb_Thumb
 *
 * Duplicates the data contained in @thumb
 *
 * Returns: a newly allocated copy of @thumb to be freed with
 * itdb_thumb_free() after use
 *
 * Since: 0.3.0
 */
Itdb_Thumb *itdb_thumb_duplicate (Itdb_Thumb *thumb)
{
    switch (thumb->data_type) {
        case ITDB_THUMB_TYPE_FILE: {
            Itdb_Thumb_File *thumb_file = (Itdb_Thumb_File *)thumb;
            return itdb_thumb_new_from_file (thumb_file->filename);
        }
        case ITDB_THUMB_TYPE_MEMORY: {
            Itdb_Thumb_Memory *thumb_memory = (Itdb_Thumb_Memory *)thumb;
            return itdb_thumb_new_from_data (thumb_memory->image_data,
                                             thumb_memory->image_data_len);
        }
#ifdef HAVE_GDKPIXBUF
        case ITDB_THUMB_TYPE_PIXBUF: {
            Itdb_Thumb_Pixbuf *thumb_pixbuf = (Itdb_Thumb_Pixbuf *)thumb;
            return itdb_thumb_new_from_pixbuf (thumb_pixbuf->pixbuf);
        }
#else
        case ITDB_THUMB_TYPE_PIXBUF:
	    return NULL;
#endif
        case ITDB_THUMB_TYPE_IPOD: {
            Itdb_Thumb_Ipod *thumb_ipod = (Itdb_Thumb_Ipod *)thumb;
            Itdb_Thumb_Ipod *new_thumb;
            GList *it;
            new_thumb = (Itdb_Thumb_Ipod *)itdb_thumb_ipod_new ();
            for (it = thumb_ipod->thumbs; it != NULL; it = it->next) {
                Itdb_Thumb_Ipod_Item *item;
                item = itdb_thumb_ipod_item_duplicate (it->data);
                if (item != NULL) {
                    itdb_thumb_ipod_add (new_thumb, item);
                }
            }
	    new_thumb->thumbs = g_list_reverse (new_thumb->thumbs);
            return (Itdb_Thumb *)new_thumb;
        }
        case ITDB_THUMB_TYPE_INVALID:
            g_assert_not_reached ();
    }

       return NULL;
}


G_GNUC_INTERNAL gint
itdb_thumb_get_byteorder (const ItdbThumbFormat format)
{
    switch (format)
    {
      case THUMB_FORMAT_UYVY_LE:
      case THUMB_FORMAT_I420_LE:
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
      case THUMB_FORMAT_I420_BE:
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

guint itdb_thumb_get_rotation (Itdb_Thumb *thumb)
{
    return thumb->rotation;
}

void itdb_thumb_set_rotation (Itdb_Thumb *thumb, guint rotation)
{
    thumb->rotation = rotation;
}

G_GNUC_INTERNAL void itdb_thumb_ipod_add (Itdb_Thumb_Ipod *thumbs,
                                          Itdb_Thumb_Ipod_Item *thumb)
{
    thumbs->thumbs = g_list_prepend (thumbs->thumbs, thumb);
}

Itdb_Thumb_Ipod_Item *itdb_thumb_ipod_get_item_by_type (Itdb_Thumb *thumbs,
                                                        const Itdb_ArtworkFormat *format)
{
    GList *gl;

    g_return_val_if_fail (format != NULL, NULL);
    g_return_val_if_fail (thumbs != NULL, NULL);
    g_return_val_if_fail (thumbs->data_type == ITDB_THUMB_TYPE_IPOD, NULL);

    for (gl=((Itdb_Thumb_Ipod *)thumbs)->thumbs; gl != NULL; gl=gl->next)
    {
	Itdb_Thumb_Ipod_Item *item = gl->data;
	g_return_val_if_fail (item != NULL, NULL);
	if (item->format == format)  return item;
    }
    return NULL;
}

/**
 * itdb_thumb_ipod_get_filename:
 * @device: an #Itdb_Device
 * @thumb:  an #Itdb_Thumb_Ipod_Item
 *
 * Get filename of thumbnail. If it's a thumbnail on the iPod, return
 * the full path to the ithmb file. Otherwise return the full path to
 * the original file.
 *
 * Returns: newly allocated string containing the absolute path to the
 * thumbnail file.
 */
gchar *itdb_thumb_ipod_get_filename (Itdb_Device *device, Itdb_Thumb_Ipod_Item *item)
{
    gchar *artwork_dir;
    char *filename = NULL;

    g_return_val_if_fail (device, NULL);
    g_return_val_if_fail (item, NULL);


    if (strlen (item->filename) < 2) {
        g_print (_("Illegal filename: '%s'.\n"), item->filename);
        return NULL;
    }

    if (!device->mountpoint) {
        g_print (_("Mountpoint not set.\n"));
        return NULL;
    }
    artwork_dir = itdb_get_artwork_dir (device->mountpoint);
    if (artwork_dir) {
        filename = itdb_get_path (artwork_dir, item->filename+1);
        g_free (artwork_dir);
    }
    /* FIXME: Hack */
    if( !filename ) {
        artwork_dir = itdb_get_photos_thumb_dir (device->mountpoint);

        if (artwork_dir) {
            const gchar *name_on_disk = strchr (item->filename+1, ':');
            if (name_on_disk) {
                filename = itdb_get_path (artwork_dir, name_on_disk + 1);
            }
            g_free (artwork_dir);
        }
    }
    return filename;
}

const GList *itdb_thumb_ipod_get_thumbs (Itdb_Thumb_Ipod *thumbs)
{
    return thumbs->thumbs;
}

#ifdef HAVE_GDKPIXBUF
/**
 * itdb_thumb_to_pixbuf_at_size:
 * @device: an #Itdb_Device
 * @thumb:  an #Itdb_Thumb
 * @width:  width of the pixbuf to retrieve, -1 for the biggest
 *          possible size and 0 for the smallest possible size (with no scaling)
 * @height: height of the pixbuf to retrieve, -1 for the biggest possible size
 *          and 0 for the smallest possible size (with no scaling)
 *
 * Converts @thumb to a #GdkPixbuf.
 *
 * <note>
 * Since we want to have gdk-pixbuf dependency optional, a generic
 * gpointer is returned which you have to cast to a #GdkPixbuf using
 * GDK_PIXBUF() yourself.
 * </note>
 *
 * Returns: a #GdkPixbuf that must be unreffed with gdk_pixbuf_unref()
 * after use, or NULL if the creation of the gdk-pixbuf failed or if
 * libgpod was compiled without gdk-pixbuf support.
 *
 * Since: 0.7.0
 */
gpointer itdb_thumb_to_pixbuf_at_size (Itdb_Device *device, Itdb_Thumb *thumb,
                                       gint width, gint height)
{
    GdkPixbuf *pixbuf=NULL;

    switch (thumb->data_type)
    {
    case ITDB_THUMB_TYPE_FILE:
        {
	    Itdb_Thumb_File *thumb_file = (Itdb_Thumb_File *)thumb;
	    if ((width != -1) && (height !=-1) && (width != 0) && (height != 0))
	    {   /* scale */
		pixbuf = gdk_pixbuf_new_from_file_at_size (thumb_file->filename,
							   width, height,
							   NULL);
	    }
	    else
	    {   /* don't scale */
		pixbuf = gdk_pixbuf_new_from_file (thumb_file->filename, NULL);
	    }
	    break;
	}
    case ITDB_THUMB_TYPE_MEMORY:
        {
	    Itdb_Thumb_Memory *thumb_mem = (Itdb_Thumb_Memory *)thumb;
	    GdkPixbufLoader *loader = gdk_pixbuf_loader_new ();
	    g_return_val_if_fail (loader, FALSE);
	    if ((width != -1) && (height !=-1) && (width != 0) && (height != 0))
	    {
		gdk_pixbuf_loader_set_size (loader, width, height);
	    }
	    gdk_pixbuf_loader_write (loader,
				     thumb_mem->image_data,
				     thumb_mem->image_data_len,
				     NULL);
	    gdk_pixbuf_loader_close (loader, NULL);
	    pixbuf = gdk_pixbuf_loader_get_pixbuf (loader);
	    if (pixbuf)
		g_object_ref (pixbuf);
	    g_object_unref (loader);
	    break;
	}
    case ITDB_THUMB_TYPE_PIXBUF:
        {
	    Itdb_Thumb_Pixbuf *thumb_pixbuf = (Itdb_Thumb_Pixbuf*)thumb;
	    if ((width != -1) && (height !=-1) && (width != 0) && (height != 0))
	    {
		pixbuf = gdk_pixbuf_scale_simple (thumb_pixbuf->pixbuf,
						  width, height,
						  GDK_INTERP_BILINEAR);
	    }
	    else
	    {
		pixbuf = g_object_ref (thumb_pixbuf->pixbuf);
	    }
	    break;
	}
    case ITDB_THUMB_TYPE_IPOD:
        {
	    Itdb_Thumb_Ipod *thumb_ipod = (Itdb_Thumb_Ipod *)thumb;
	    const GList *it;
	    Itdb_Thumb_Ipod_Item *chosen;
	    gint w=width;
	    gint h=height;

	    if ((width == -1) || (height == -1))
	    {   /* choose the largest available thumbnail */
		w = G_MAXINT;
	    h = G_MAXINT;
	    }

	    if (device == NULL) {
		/* device is needed to get the ipod mountpoint */
		return NULL;
	    }
	    chosen = NULL;
	    for (it = itdb_thumb_ipod_get_thumbs (thumb_ipod);
		 it != NULL;
		 it = it->next) {
		Itdb_Thumb_Ipod_Item *item = (Itdb_Thumb_Ipod_Item*)it->data;
		if (chosen == NULL)
		{   /* make sure we select *something* */
		    chosen = item;
		}
		if ((chosen->width > w) && (chosen->height > h))
		{   /* try to find a thumb in size between the chosen and
		       the current one */
		    if ((item->width >= w) && (item->height >= h))
		    {
			if ((item->width < chosen->width) || (item->height < chosen->height))
			{
			    chosen = item;
			}
		    }
		}
		if ((chosen->width < w) || (chosen->height < h))
		{   /* try to find something bigger */
		    if ((item->width > chosen->width) || (item->height > chosen->height))
		    {
			chosen = item;
		    }
		}
	    }
	    if (chosen != NULL)
	    {
		GdkPixbuf *pix = itdb_thumb_ipod_item_to_pixbuf (device, chosen);
		if ((width != -1) && (height !=-1) && (width != 0) && (height != 0))
		{   /* scale */
		    gdouble scalex = (gdouble)width/chosen->width;
		    gdouble scaley = (gdouble)height/chosen->height;
		    gdouble scale = MIN (scalex, scaley);
		    pixbuf = gdk_pixbuf_scale_simple (pix,
						      chosen->width*scale,
						      chosen->height*scale,
						      GDK_INTERP_BILINEAR);
		    g_object_unref (pix);
		}
		else
		{   /* don't scale */
		    pixbuf = pix;
		}
	    }
	    break;
	}
    case ITDB_THUMB_TYPE_INVALID:
	g_return_val_if_reached (NULL);
    } /* switch (...) */

    return pixbuf;
}

static GList *itdb_thumb_ipod_to_pixbufs (Itdb_Device *device,
                                          Itdb_Thumb_Ipod *thumb)
{
        const GList *items;
        GList *pixbufs = NULL;

        g_return_val_if_fail (thumb != NULL, NULL);
        g_return_val_if_fail (thumb->parent.data_type == ITDB_THUMB_TYPE_IPOD, NULL);

        for (items = itdb_thumb_ipod_get_thumbs (thumb);
             items != NULL;
             items = items->next) {
            GdkPixbuf *pixbuf;
            pixbuf = itdb_thumb_ipod_item_to_pixbuf (device, items->data);
            if (pixbuf != NULL) {
                pixbufs = g_list_prepend (pixbufs, pixbuf);
            }
        }

        return pixbufs;
}

/**
 * itdb_thumb_to_pixbufs:
 * @device: an #Itdb_Device
 * @thumb:  an #Itdb_Thumb
 *
 * Converts @thumb to a #GList of #GdkPixbuf. The returned #GList will
 * generally contain only 1 element, the full-size pixbuf associated with
 * @thumb, but when the artwork has been read from the ipod and hasn't been
 * modified from the library, then the returned #GList will contain several
 * #GdkPixbuf corresponding to the various thumbnail sizes that were
 * written to the iPod database.
 *
 * Returns: a #GList of #GdkPixbuf which are associated with @thumb, NULL
 * if the pixbuf was invalid or if libgpod is compiled without gdk-pixbuf
 * support. The #GdkPixbuf must be unreffed with gdk_pixbuf_unref() after use
 * and the #GList must be freed with g_list_free().
 *
 * Since: 0.7.0
 */
GList *itdb_thumb_to_pixbufs (Itdb_Device *device, Itdb_Thumb *thumb)
{
    GList *pixbufs = NULL;
    GdkPixbuf *pixbuf;

    switch (thumb->data_type) {
    case ITDB_THUMB_TYPE_IPOD:
        pixbufs = itdb_thumb_ipod_to_pixbufs (device, (Itdb_Thumb_Ipod *)thumb);
        break;
    case ITDB_THUMB_TYPE_FILE:
    case ITDB_THUMB_TYPE_MEMORY:
    case ITDB_THUMB_TYPE_PIXBUF:
        pixbuf = itdb_thumb_to_pixbuf_at_size (device, thumb, -1, -1);
        pixbufs = g_list_append (pixbufs, pixbuf);
        break;
    case ITDB_THUMB_TYPE_INVALID:
        g_assert_not_reached ();
    }

    return pixbufs;
}
#else
gpointer itdb_thumb_to_pixbuf_at_size (Itdb_Device *device, Itdb_Thumb *thumb,
                                       gint width, gint height)
{
    return NULL;
}


GList *itdb_thumb_to_pixbufs (Itdb_Device *device, Itdb_Thumb *thumb)
{
    return NULL;
}
#endif

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


Itdb_Thumb *itdb_thumb_new_from_pixbuf (GdkPixbuf *pixbuf)
{
    Itdb_Thumb_Pixbuf *thumb_pixbuf;
    Itdb_Thumb *thumb;
 
    thumb_pixbuf = g_new0 (Itdb_Thumb_Pixbuf, 1);
    thumb = (Itdb_Thumb *)thumb_pixbuf;
    thumb->data_type = ITDB_THUMB_TYPE_PIXBUF;
    thumb_pixbuf->pixbuf = g_object_ref (G_OBJECT (pixbuf));
   
    return thumb; 
}


Itdb_Thumb_Ipod_Item *itdb_thumb_new_item_from_ipod (const Itdb_ArtworkFormat *format)
{
    Itdb_Thumb_Ipod_Item *thumb_ipod;
 
    thumb_ipod = g_new0 (Itdb_Thumb_Ipod_Item, 1);
    thumb_ipod->format = format;

    return thumb_ipod;
}

Itdb_Thumb *itdb_thumb_ipod_new (void)
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
 **/
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
    if (new_item == NULL) {
        return NULL;
    }
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
 * Return value: a newly allocated copy of @thumb to be freed with 
 * itdb_thumb_free() after use
 **/
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
#endif
        case ITDB_THUMB_TYPE_IPOD: {
            Itdb_Thumb_Ipod *thumb_ipod = (Itdb_Thumb_Ipod *)thumb;
            Itdb_Thumb_Ipod *new_thumb;
            GList *it;
            new_thumb = (Itdb_Thumb_Ipod *)itdb_thumb_ipod_new ();
            if (new_thumb == NULL) {
                return NULL;
            }
            for (it = thumb_ipod->thumbs; it != NULL; it = it->next) {
                Itdb_Thumb_Ipod_Item *item;
                item = itdb_thumb_ipod_item_duplicate (it->data);
                if (item != NULL) {
                    itdb_thumb_ipod_add (new_thumb, item);
                }
                new_thumb->thumbs = g_list_reverse (new_thumb->thumbs);
            }
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
gpointer itdb_thumb_to_pixbuf_at_size (Itdb_Device *device, Itdb_Thumb *thumb, 
                                       gint width, gint height)
{
    GdkPixbuf *pixbuf;
    if (thumb->data_type == ITDB_THUMB_TYPE_FILE)
    {   
        Itdb_Thumb_File *thumb_file = (Itdb_Thumb_File *)thumb;
        pixbuf = gdk_pixbuf_new_from_file_at_size (thumb_file->filename, 
                                                   width, height,
                                                   NULL);
    }
    else if (thumb->data_type == ITDB_THUMB_TYPE_MEMORY)
    {   
        Itdb_Thumb_Memory *thumb_mem = (Itdb_Thumb_Memory *)thumb;
        GdkPixbufLoader *loader = gdk_pixbuf_loader_new ();
        g_return_val_if_fail (loader, FALSE);
        if ((width != -1) && (height != -1)) {
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
    }
    else if (thumb->data_type == ITDB_THUMB_TYPE_PIXBUF)
    {   
        Itdb_Thumb_Pixbuf *thumb_pixbuf = (Itdb_Thumb_Pixbuf*)thumb;
        pixbuf = gdk_pixbuf_scale_simple (thumb_pixbuf->pixbuf,
                                          width, height,
                                          GDK_INTERP_BILINEAR);
    }
    else if (thumb->data_type == ITDB_THUMB_TYPE_IPOD)
    {
        Itdb_Thumb_Ipod *thumb_ipod = (Itdb_Thumb_Ipod *)thumb;
        const GList *thumb;
        Itdb_Thumb_Ipod_Item *chosen;

        if (device == NULL) {
            /* device is needed to get the ipod mountpoint */
            return NULL;
        }
        chosen = NULL;
        for (thumb = itdb_thumb_ipod_get_thumbs (thumb_ipod);
             thumb != NULL;
             thumb = thumb->next) {
            Itdb_Thumb_Ipod_Item *item = (Itdb_Thumb_Ipod_Item*)thumb->data;
            if ((width >= item->width) && (height >= item->height)) {
                if (chosen == NULL) { 
                    chosen = item;
                }
                if ((item->width > chosen->width) 
                        && (item->height > chosen->height)) {
                    chosen = item;
                }
            }
        }
        if (chosen != NULL) {
            GdkPixbuf *pixbuf;
            pixbuf = itdb_thumb_ipod_item_to_pixbuf (device, chosen);
            return pixbuf;
        } else {
            return NULL;
        }
    }
    return pixbuf;
}

GList *itdb_thumb_ipod_to_pixbufs (Itdb_Device *dev, Itdb_Thumb_Ipod *thumb)
{
        g_return_val_if_fail (thumb != NULL, NULL);
        g_return_val_if_fail (thumb->parent.data_type == ITDB_THUMB_TYPE_IPOD, NULL);
        const GList *items;
        GList *pixbufs = NULL;

        for (items = itdb_thumb_ipod_get_thumbs (thumb); 
             items != NULL; 
             items = items->next) {
            GdkPixbuf *pixbuf;
            pixbuf = itdb_thumb_ipod_item_to_pixbuf (dev, items->data);
            if (pixbuf != NULL) {
                pixbufs = g_list_prepend (pixbufs, pixbuf);
            }
        }

        return pixbufs;
}
#else
gpointer itdb_thumb_to_pixbuf_at_size (Itdb_Thumb *thumb, 
                                       gint width, gint height)
{
    return NULL;
}

GList *itdb_thumb_ipod_to_pixbufs (Itdb_Device *dev, Itdb_Thumb_Ipod *thumb)
{
    return NULL;
}
#endif

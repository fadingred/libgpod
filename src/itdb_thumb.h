/*
|  Copyright (C) 2002-2007 Jorg Schuler <jcsjcs at users sourceforge net>
|                2007 Christophe Fergeau <teuf at gnome org>
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
#ifndef __ITDB_THUMB_H__
#define __ITDB_THUMB_H__

#include "itdb.h"
#include "itdb_device.h"

/* Types of thumbnails in Itdb_Image */
typedef enum {
    ITDB_THUMB_TYPE_INVALID,
    ITDB_THUMB_TYPE_FILE,
    ITDB_THUMB_TYPE_MEMORY,
    ITDB_THUMB_TYPE_PIXBUF,
    ITDB_THUMB_TYPE_IPOD
} ItdbThumbDataType;

/* The Itdb_Thumb structure can represent two slightly different
   thumbnails:

  a) a thumbnail before it's transferred to the iPod.

     offset and size are 0

     width and height, if unequal 0, will indicate the size on the
     iPod. width and height are set the first time a pixbuf is
     requested for this thumbnail.

     type is set according to the type this thumbnail represents

     filename point to a 'real' image file OR image_data and
     image_data_len are set.

  b) a thumbnail (big or small) stored on a database in the iPod.  In
     these cases, id corresponds to the ID originally used in the
     database, filename points to a .ithmb file on the iPod
 */

/**
 * Itdb_Thumb:
 * @data_type: The type of data (file, memory, pixbuf, ipod, etc.)
 * @rotation:  Angle by which the image is rotated counterclockwise
 *
 * This is an opaque structure representing a thumbnail to be
 * transferred to the ipod or read from the ipod.
 *
 * Since: 0.3.0
 */
struct _Itdb_Thumb {
    ItdbThumbDataType data_type;
    guint rotation;
};

struct _Itdb_Thumb_File {
    struct _Itdb_Thumb parent;
    gchar *filename;
};
typedef struct _Itdb_Thumb_File Itdb_Thumb_File;

struct _Itdb_Thumb_Memory {
    struct _Itdb_Thumb parent;
    guchar  *image_data;      /* holds the thumbnail data of
				 non-transfered thumbnails when
				 filename == NULL */
    gsize   image_data_len;   /* length of data */
};
typedef struct _Itdb_Thumb_Memory Itdb_Thumb_Memory;

struct _Itdb_Thumb_Pixbuf {
    struct _Itdb_Thumb parent;
    gpointer pixbuf;
};
typedef struct _Itdb_Thumb_Pixbuf Itdb_Thumb_Pixbuf;

struct _Itdb_Thumb_Ipod {
    struct _Itdb_Thumb parent;
    GList *thumbs;
};
typedef struct _Itdb_Thumb_Ipod Itdb_Thumb_Ipod;

struct _Itdb_Thumb_Ipod_Item {
    const Itdb_ArtworkFormat *format;
    gchar   *filename;
    guint32 offset;
    guint32 size;
    gint16  width;
    gint16  height;
    gint16  horizontal_padding;
    gint16  vertical_padding;
};
typedef struct _Itdb_Thumb_Ipod_Item Itdb_Thumb_Ipod_Item;

G_GNUC_INTERNAL Itdb_Thumb *itdb_thumb_new_from_file (const gchar *filename);
G_GNUC_INTERNAL Itdb_Thumb *itdb_thumb_new_from_data (const guchar *data,
                                                      gsize len);
G_GNUC_INTERNAL Itdb_Thumb *itdb_thumb_new_from_pixbuf (gpointer pixbuf);
G_GNUC_INTERNAL Itdb_Thumb_Ipod_Item *itdb_thumb_new_item_from_ipod (const Itdb_ArtworkFormat *format);
G_GNUC_INTERNAL Itdb_Thumb *itdb_thumb_ipod_new (void);
G_GNUC_INTERNAL void itdb_thumb_set_rotation (Itdb_Thumb *thumb,
                                              guint rotation);
G_GNUC_INTERNAL guint itdb_thumb_get_rotation (Itdb_Thumb *thumb);
G_GNUC_INTERNAL void itdb_thumb_ipod_add (Itdb_Thumb_Ipod *thumbs,
                                          Itdb_Thumb_Ipod_Item *thumb);
G_GNUC_INTERNAL const GList *itdb_thumb_ipod_get_thumbs (Itdb_Thumb_Ipod *thumbs);
G_GNUC_INTERNAL char *itdb_thumb_ipod_get_filename (Itdb_Device *device, Itdb_Thumb_Ipod_Item *thumb);
G_GNUC_INTERNAL Itdb_Thumb_Ipod_Item *itdb_thumb_ipod_get_item_by_type (Itdb_Thumb *thumbs,
                                                        const Itdb_ArtworkFormat *format);
G_GNUC_INTERNAL gpointer
itdb_thumb_ipod_item_to_pixbuf (Itdb_Device *device,
                                Itdb_Thumb_Ipod_Item *item);
#endif

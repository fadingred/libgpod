/*
|  Copyright (C) 2002-2007 Jorg Schuler <jcsjcs at users sourceforge net>
|  Part of the gtkpod project.
| 
|  URL: http://www.gtkpod.org/
|  URL: http://gtkpod.sourceforge.net/
|
|  Most of the code in this file has been ported from the perl
|  script "mktunes.pl" (part of the gnupod-tools collection) written
|  by Adrian Ulrich <pab at blinkenlights.ch>.
|
|  gnupod-tools: http://www.blinkenlights.ch/cgi-bin/fm.pl?get=ipod
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

#ifndef __ITDB_DEVICE_H__
#define __ITDB_DEVICE_H__

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "itdb.h"

#include <glib.h>

G_BEGIN_DECLS

typedef struct _Itdb_ArtworkFormat Itdb_ArtworkFormat;
typedef enum _ItdbThumbFormat ItdbThumbFormat;

enum _ItdbThumbFormat
{
    THUMB_FORMAT_UYVY_LE,
    THUMB_FORMAT_UYVY_BE,
    THUMB_FORMAT_RGB565_LE,
    THUMB_FORMAT_RGB565_LE_90,
    THUMB_FORMAT_RGB565_BE,
    THUMB_FORMAT_RGB565_BE_90,
    THUMB_FORMAT_RGB555_LE,
    THUMB_FORMAT_RGB555_LE_90,
    THUMB_FORMAT_RGB555_BE,
    THUMB_FORMAT_RGB555_BE_90,
    THUMB_FORMAT_REC_RGB555_LE,
    THUMB_FORMAT_REC_RGB555_LE_90,
    THUMB_FORMAT_REC_RGB555_BE,
    THUMB_FORMAT_REC_RGB555_BE_90,
    THUMB_FORMAT_RGB888_LE,
    THUMB_FORMAT_RGB888_LE_90,
    THUMB_FORMAT_RGB888_BE,
    THUMB_FORMAT_RGB888_BE_90,
    THUMB_FORMAT_EXPERIMENTAL_LE,
    THUMB_FORMAT_EXPERIMENTAL_BE,
};


struct _Itdb_Device
{
    gchar *mountpoint;    /* mountpoint of the iPod */
    gint   musicdirs;     /* number of /iPod_Control/Music/F.. dirs */
    guint  byte_order;    /* G_LITTLE_ENDIAN "regular" endianness 
			   * G_BIG_ENDIAN "reversed" endianness (e.g. mobile
			   * phone iTunesDBs)
			   */
    GHashTable *sysinfo;  /* hash with value/key pairs of all entries
			   * in Device/SysInfo */
    gboolean sysinfo_changed; /* Has the sysinfo hash been changed by
				 the user (itdb_set_sysinfo) */
    gint timezone_shift;  /* difference in seconds between the current
                           * timezone and UTC
                           */

};

struct _Itdb_ArtworkFormat
{
	ItdbThumbType type;
	gint16 width;
	gint16 height;
	gint16 correlation_id;
        ItdbThumbFormat format;
        gint32 padding;
};

G_GNUC_INTERNAL const Itdb_ArtworkFormat *itdb_device_get_artwork_formats (Itdb_Device *device);
G_GNUC_INTERNAL gint itdb_device_musicdirs_number (Itdb_Device *device);
G_GNUC_INTERNAL void itdb_device_autodetect_endianess (Itdb_Device *device);
G_GNUC_INTERNAL gboolean itdb_device_read_sysinfo_xml (Itdb_Device *device, 
						       GError **error);
G_GNUC_INTERNAL guint64 itdb_device_get_firewire_id (Itdb_Device *device);

G_END_DECLS

#endif

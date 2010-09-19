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
#include "itdb_sysinfo_extended_parser.h"

#include <glib.h>

G_BEGIN_DECLS

typedef enum _ItdbThumbFormat ItdbThumbFormat;

enum _ItdbThumbFormat
{
    THUMB_FORMAT_UYVY_LE,
    THUMB_FORMAT_UYVY_BE,
    THUMB_FORMAT_I420_LE,
    THUMB_FORMAT_I420_BE,
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

typedef enum _ItdbChecksumType ItdbChecksumType;
enum _ItdbChecksumType {
    ITDB_CHECKSUM_UNKNOWN	= -1,
    ITDB_CHECKSUM_NONE		= 0,
    ITDB_CHECKSUM_HASH58	= 1,
    ITDB_CHECKSUM_HASH72	= 2,
    ITDB_CHECKSUM_HASHAB	= 3
};

/**
 * Itdb_Device:
 * @mountpoint:         The mountpoint of the iPod
 * @musicdirs:          The number of /iPod_Control/Music/F.. dirs
 * @byte_order:         G_LITTLE_ENDIAN "regular" endianness G_BIG_ENDIAN
 *                      "reversed" endianness (e.g. mobile phone iTunesDBs)
 * @sysinfo:            A hash with key/value pairs of all entries in
 *                      Device/SysInfo
 * @sysinfo_extended:   The parsed content of SysInfoExtended, which can be NULL
 * @sysinfo_changed:    True if the sysinfo hash been changed by the user, false
 *                      otherwise.  (see itdb_set_sysinfo())
 * @timezone_shift:     The difference in seconds between the current timezone
 *                      and UTC
 * @iphone_sync_context:Private data passed as is to libimobiledevice by 
 *                      itdb_start/stop_sync
 * @iphone_sync_nest_level: Nesting count for itdb_start/stop_sync calls
 *                      itdb_start/stop_sync
 *
 * Structure representing an iPod device
 *
 * Since: 0.4.0
 */
struct _Itdb_Device
{
    gchar *mountpoint;
    gint   musicdirs;
    guint  byte_order;
    GHashTable *sysinfo;
    SysInfoIpodProperties *sysinfo_extended;
    gboolean sysinfo_changed;
    gint timezone_shift;
    void *iphone_sync_context;
    int iphone_sync_nest_level;
};

/**
 * Itdb_ArtworkFormat:
 * @format_id:          Unique ID for the format (generally a 4 digit int)
 * @width:              Width of the thumbnail
 * @height:             Height of the thumbnail
 * @format:             Pixel format of the thumbnail (RGB, YUV, ...)
 * @padding:            Number of bytes of padding to add after the thumbnail
 *                      (not found in SysInfoExtended -- added for
 *                      compatibility with hardcoded artwork formats)
 * @crop:               Indicates if the thumbnail is to be cropped
 * @rotation:           Degrees to rotate the thumbnail
 * @back_color:         Background color for the thumbnail
 * @display_width:      Width at which the thumbnail will be displayed
 *                      (not currently used)
 * @interlaced:         If TRUE, the thumbnails are interlaced
 *                      (not currently used)
 * @color_adjustment:   Color adjustment for the thumbnails
 *                      (not currently used)
 * @gamma:              Gamma value for the thumbails
 *                      (not currently used)
 * @associated_format:  Unknown (not currently used)
 * @row_bytes_alignment: Specifies the number of bytes a pixel row must be aligned to
 *
 * Structure representing the characteristics of the thumbnails to
 * write to a given .ithmb file. The format of the structure is based
 * on the way artwork formats are written to SysInfoExtended.
 */
struct _Itdb_ArtworkFormat {
        gint format_id;
        gint width;
        gint height;
        ItdbThumbFormat format;
        gint32 padding;
        gboolean crop;
        gint rotation;
        guchar back_color[4];

        gint display_width;
        gboolean interlaced;
        gint color_adjustment;
        gdouble gamma;
        gint associated_format;
        gint row_bytes_alignment;
};

/* Error domain */
#define ITDB_DEVICE_ERROR itdb_device_error_quark ()
GQuark     itdb_device_error_quark      (void);

G_GNUC_INTERNAL GList *itdb_device_get_photo_formats (const Itdb_Device *device);
G_GNUC_INTERNAL GList *itdb_device_get_cover_art_formats (const Itdb_Device *device);
G_GNUC_INTERNAL GList *itdb_device_get_chapter_image_formats (const Itdb_Device *device);
G_GNUC_INTERNAL gint itdb_device_musicdirs_number (Itdb_Device *device);
G_GNUC_INTERNAL void itdb_device_autodetect_endianess (Itdb_Device *device);
G_GNUC_INTERNAL const char *itdb_device_get_firewire_id (const Itdb_Device *device);
G_GNUC_INTERNAL gboolean itdb_device_supports_sparse_artwork (const Itdb_Device *device);
G_GNUC_INTERNAL gboolean itdb_device_supports_compressed_itunesdb (const Itdb_Device *device);
G_GNUC_INTERNAL gboolean itdb_device_supports_sqlite_db (const Itdb_Device *device);
G_GNUC_INTERNAL gboolean itdb_device_get_storage_info (Itdb_Device *device, guint64 *capacity, guint64 *free);
G_GNUC_INTERNAL gboolean itdb_device_write_checksum (Itdb_Device *device,
						     unsigned char *itdb_data,
						     gsize itdb_len,
						     GError **error);
G_GNUC_INTERNAL void itdb_device_set_timezone_info (Itdb_Device *device);
G_GNUC_INTERNAL gboolean itdb_device_is_iphone_family (const Itdb_Device *device);
G_GNUC_INTERNAL gboolean itdb_device_is_shuffle (const Itdb_Device *device);
G_GNUC_INTERNAL ItdbChecksumType itdb_device_get_checksum_type (const Itdb_Device *device);
G_GNUC_INTERNAL enum ItdbShadowDBVersion itdb_device_get_shadowdb_version (const Itdb_Device *device);

const Itdb_IpodInfo *
itdb_ipod_info_from_serial (const char *serial);

G_END_DECLS

#endif

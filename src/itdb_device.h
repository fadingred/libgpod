/* Time-stamp: <2006-05-30 21:53:34 jcs>
|
|  Copyright (C) 2002-2005 Jorg Schuler <jcsjcs at users sourceforge net>
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

typedef struct _Itdb_IpodModel Itdb_IpodModel;
typedef struct _Itdb_ArtworkFormat Itdb_ArtworkFormat;

struct _Itdb_Device
{
    gchar *mountpoint;    /* mountpoint of the iPod */
    gint   musicdirs;     /* number of /iPod_Control/Music/F.. dirs */
    guint  byte_order;    /* G_LITTLE_ENDIAN "regular" endianness 
			   * G_BIG_ENDIAN "reversed" endianness (e.g. mobile
			   * phone iTunesDBs)
			   */
    GHashTable *sysinfo;  /* hash with value/key pairs of all entries
			     in Device/SysInfo */
};

struct _Itdb_ArtworkFormat
{
	gint  type;
	gint16 width;
	gint16 height;
	gint16 correlation_id;
};

struct _Itdb_IpodModel {
	const gchar *model_number;
	const guint64 capacity;
	guint model_type;
	guint generation;
        /* number of music (Fnn) dirs created by iTunes. The exact
	   number seems to be version dependent. Therefore, the
	   numbers here represent a mixture of reported values and
	   common sense. */
        guint musicdirs;
};

typedef enum {
	UNKNOWN_GENERATION,
	FIRST_GENERATION,
	SECOND_GENERATION,
	THIRD_GENERATION,
	FOURTH_GENERATION,
	FIFTH_GENERATION,
	MOBILE_GENERATION
} Itdb_Generation;

typedef enum {
	MODEL_TYPE_INVALID,
	MODEL_TYPE_UNKNOWN,
	MODEL_TYPE_COLOR,
	MODEL_TYPE_COLOR_U2,
	MODEL_TYPE_REGULAR,
	MODEL_TYPE_REGULAR_U2,
	MODEL_TYPE_MINI,
	MODEL_TYPE_MINI_BLUE,
	MODEL_TYPE_MINI_PINK,
	MODEL_TYPE_MINI_GREEN,
	MODEL_TYPE_MINI_GOLD,
	MODEL_TYPE_SHUFFLE,
	MODEL_TYPE_NANO_WHITE,
	MODEL_TYPE_NANO_BLACK,
	MODEL_TYPE_VIDEO_WHITE,
	MODEL_TYPE_VIDEO_BLACK,
	MODEL_TYPE_MOBILE_1
} Itdb_ModelType;


enum { 
	IPOD_COVER_SMALL,
	IPOD_COVER_LARGE,
	IPOD_PHOTO_SMALL,
	IPOD_PHOTO_LARGE,
	IPOD_PHOTO_FULL_SCREEN,
	IPOD_PHOTO_TV_SCREEN
};

G_GNUC_INTERNAL const Itdb_ArtworkFormat *itdb_device_get_artwork_formats (Itdb_Device *device);
G_GNUC_INTERNAL const Itdb_IpodModel *itdb_device_get_ipod_model (Itdb_Device *device);
G_GNUC_INTERNAL gint itdb_device_musicdirs_number (Itdb_Device *device);
G_GNUC_INTERNAL void itdb_device_autodetect_endianess (Itdb_Device *device);
G_END_DECLS

#endif

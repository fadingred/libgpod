/*
|  Copyright (C) 2002-2007 Jorg Schuler <jcsjcs at users sourceforge net>
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

#ifndef __ITDB_PRIVATE_H__
#define __ITDB_PRIVATE_H__

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include "itdb_device.h"
#include "itdb.h"

/* always use itdb_playlist_is_mpl() to check for MPL! */
enum ItdbPlType { /* types for playlist->type */
    ITDB_PL_TYPE_NORM = 0,       /* normal playlist, visible in iPod */
    ITDB_PL_TYPE_MPL = 1         /* master playlist, contains all tracks,
				    not visible in iPod */
};

/* always use itdb_playlists_is_podcasts() to check for podcasts PL */
enum ItdbPlFlag { /* types for playlist->podcastflag */
    ITDB_PL_FLAG_NORM = 0,       /* normal playlist, visible under
				    'Playlists  */
    ITDB_PL_FLAG_PODCASTS = 1    /* special podcast playlist visible
				    under 'Music' */
};


/* keeps the contents of one disk file (read) */
typedef struct
{
    gchar *filename;
    gchar *contents;
    /* indicate that endian order is reversed as in the case of the
       iTunesDBs for mobile phones */
    gboolean reversed;
    gsize length;
    GError *error;
} FContents;


/* struct used to hold all necessary information when importing a
   Itdb_iTunesDB */
typedef struct
{
    Itdb_iTunesDB *itdb;
    FContents *fcontents;
    GList *pos_glist;    /* temporary list to store position indicators */
    gint32 pos_len;      /* current length of above list */
    GList *playcounts;   /* contents of Play Counts file */
    GTree *idtree;       /* temporary tree with track id tree */
    GError *error;       /* where to report errors to */
} FImport;

/* data of playcounts GList above */
struct playcount {
    guint32 playcount;
    guint32 skipped;     /* skipped (only for Shuffle's iTunesStats */
    time_t time_played;
    guint32 bookmark_time;
    gint32 rating;
    gint32 pc_unk16;     /* unknown field in Play Counts file */
    guint32 skipcount;
    time_t last_skipped;
    gint32 st_unk06;     /* unknown field in iTunesStats file */
    gint32 st_unk09;     /* unknown field in iTunesStats file */
};

/* value to indicate that playcount was not set in struct playcount
   above */
#define NO_PLAYCOUNT (-1)


/* keeps the contents of the output file (write) */
typedef struct
{
    gchar *filename;
    gchar *contents;     /* pointer to contents */
    /* indicate that endian order is reversed as in the case of the
       iTunesDBs for mobile phones */
    gboolean reversed;
    gulong pos;          /* current write position ("end of file") */
    gulong total;        /* current total size of *contents array  */
    GError *error;       /* place to report errors to */
} WContents;

/* size of memory by which the total size of above WContents gets
 * increased (1.5 MB) */
#define WCONTENTS_STEPSIZE 1572864

/* struct used to hold all necessary information when exporting a
 * Itdb_iTunesDB */
typedef struct
{
    Itdb_iTunesDB *itdb;
    WContents *wcontents;
    guint32 next_id;     /* next free ID to use       */
    GError *error;       /* where to report errors to */
} FExport;


enum _DbType {
    DB_TYPE_ITUNES,
    DB_TYPE_PHOTO
};

typedef enum _DbType DbType;

struct _Itdb_DB{
	DbType db_type;
	union {
		Itdb_PhotoDB *photodb; 
		Itdb_iTunesDB *itdb;
	} db;
};

typedef struct _Itdb_DB Itdb_DB;

G_GNUC_INTERNAL gboolean itdb_spl_action_known (ItdbSPLAction action);
G_GNUC_INTERNAL void itdb_splr_free (Itdb_SPLRule *splr);
G_GNUC_INTERNAL const gchar *itdb_photodb_get_mountpoint (Itdb_PhotoDB *photodb);
G_GNUC_INTERNAL gchar *db_get_mountpoint (Itdb_DB *db);
G_GNUC_INTERNAL Itdb_Device *db_get_device(Itdb_DB *db);
G_GNUC_INTERNAL gint itdb_get_free_photo_id ( Itdb_PhotoDB *db );
G_GNUC_INTERNAL Itdb_iTunesDB *db_get_itunesdb (Itdb_DB *db);
G_GNUC_INTERNAL Itdb_PhotoDB *db_get_photodb (Itdb_DB *db);
G_GNUC_INTERNAL gint itdb_thumb_get_byteorder (ItdbThumbFormat format);
G_GNUC_INTERNAL time_t device_time_mac_to_time_t (Itdb_Device *device, 
						guint64 mactime);
G_GNUC_INTERNAL guint64 device_time_time_t_to_mac (Itdb_Device *device,
						 time_t timet);
G_GNUC_INTERNAL gint itdb_musicdirs_number_by_mountpoint (const gchar *mountpoint);
G_GNUC_INTERNAL gboolean itdb_device_requires_checksum (Itdb_Device *device);
#endif

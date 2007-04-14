/* Time-stamp: <27-mar-2007 10:06:21 teuf>
|
|  Copyright (C) 2002-2005 Jorg Schuler <jcsjcs at users sourceforge net>
|  Part of the gtkpod project.
| 
|  URL: http://www.gtkpod.org/
|  URL: http://gtkpod.sourceforge.net/
|
|  Much of the code in this file has originally been ported from the
|  perl script "mktunes.pl" (part of the gnupod-tools collection)
|  written by Adrian Ulrich <pab at blinkenlights.ch>:
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
|  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
|  USA
|
|  iTunes and iPod are trademarks of Apple
|
|  This product is not supported/written/published by Apple!
|
|  $Id$
*/

/* Some notes on how to use the functions in this file:


   *** Reading the iTunesDB ***

   Itdb_iTunesDB *itunesdb_parse (gchar *path); /+ path to mountpoint +/
   will read an Itdb_iTunesDB and pass the data over to your program.

   The information given in the "Play Counts" file is also read if
   available and the playcounts, star rating and the time last played
   is updated.

   Itdb_iTunesDB is a structure containing a GList for the tracks and a
   GList for the playlists.

   For each track these fields are set as follows:

   "transferred" will be set to TRUE because all tracks read from a
   Itdb_iTunesDB are obviously (or hopefully) already transferred to the
   iPod.

   "recent_playcount" is for information only (it will allow to
   generate playlists with tracks played since the last time) and will
   not be stored to the iPod.

   The master playlist is guaranteed to be the first playlist, and
   this must not be changed by your code.


   *** Writing the Itdb_iTunesDB ***

   gboolean itunesdb_write (gchar *path, Itdb_iTunesDB *itb)
   /+ @path to mountpoint, itb to @write +/
   will write an updated version of the Itdb_iTunesDB.

   The "Play Counts" file is renamed to "Play Counts.bak" if it exists
   to avoid reading it multiple times.

   Please note that non-transferred tracks are not automatically
   transferred to the iPod. A function

   gboolean itunesdb_copy_track_to_ipod (gchar *path, Itdb_Track *track, gchar *pcfile)

   is provided to help you do that, however.

   The following functions most likely will be useful:

   Itdb_Track *itunesdb_new_track (void);
   Use itunesdb_new_track() to get an "initialized" track structure
   (the "unknowns" are initialized with reasonable values).

   gboolean itunesdb_cp (gchar *from_file, gchar *to_file);
   void itunesdb_convert_filename_fs2ipod(gchar *ipod_file);
   void itunesdb_convert_filename_ipod2fs(gchar *ipod_file);

   guint32 itunesdb_time_get_mac_time (void);
   time_t itunesdb_time_mac_to_host (guint32 mactime);
   guint32 itunesdb_time_host_to_mac (time_t time);

   void itunesdb_rename_files (const gchar *dirname);

   (Renames/removes some files on the iPod (Playcounts, OTG
   semaphore). Needs to be called if you write the Itdb_iTunesDB not
   directly to the iPod but to some other location and then manually
   copy the file from there to the iPod. That's much faster in the
   case of using an iPod mounted in sync'ed mode.)

   Jorg Schuler, 29.12.2004 */


/* call itdb_parse () to read the Itdb_iTunesDB  */
/* call itdb_write () to write the Itdb_iTunesDB */


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "db-artwork-parser.h"
#include "itdb_device.h"
#include "itdb_private.h"
#include <errno.h>
#include <fcntl.h>
#include <glib-object.h>
#include <glib/gi18n-lib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define ITUNESDB_DEBUG 0
#define ITUNESDB_MHIT_DEBUG 0

#define ITUNESDB_COPYBLK (1024*1024*4)      /* blocksize for cp () */


/* NOTE for developers:

   Sometimes new MHOD string fields are added by Apple in the
   iTunesDB. In that case you need to modify the code in the following
   places:

   itdb_itunesdb.c:
   - enum MHOD_ID
   - get_mhod(): inside the switch() statement. Currently no compiler
     warning is given.
   - get_mhod_string(): inside the switch() statement. A compiler warning
     will help you find it.
   - get_playlist(): inside the switch() statement. A compiler warning
     will help you find it.
   - get_mhit(): inside the switch() statement. A compiler warning
     will help you find it.
   - mk_mhod(): inside the switch() statement. A compiler warning
     will help you find it.
   - write_mhsd_tracks(): inside the for() loop.

   itdb_track.c:
   - itdb_track_free()
   - itdb_track_duplicate()

   itdb_playlists.c:
   Analogous to itdb_track.c in case the string is part of the playlist
   description.
*/

/* Note: some of the comments for the MHOD_IDs are copied verbatim
 * from http://ipodlinux.org/ITunesDB */
enum MHOD_ID {
  MHOD_ID_TITLE = 1,
  MHOD_ID_PATH = 2,   /* file path on iPod (special format) */
  MHOD_ID_ALBUM = 3,
  MHOD_ID_ARTIST = 4,
  MHOD_ID_GENRE = 5,
  MHOD_ID_FILETYPE = 6,
/* MHOD_ID_EQSETTING = 7, */
  MHOD_ID_COMMENT = 8,
/* Category - This is the category ("Technology", "Music", etc.) where
   the podcast was located. Introduced in db version 0x0d. */
  MHOD_ID_CATEGORY = 9,
  MHOD_ID_COMPOSER = 12,
  MHOD_ID_GROUPING = 13,
/* Description text (such as podcast show notes). Accessible by
   wselecting the center button on the iPod, where this string is
   displayed along with the song title, date, and
   timestamp. Introduced in db version 0x0d. */
  MHOD_ID_DESCRIPTION = 14,
/* Podcast Enclosure URL. Note: this is either a UTF-8 or ASCII
   encoded string (NOT UTF-16). Also, there is no mhod::length value
   for this type. Introduced in db version 0x0d.  */
  MHOD_ID_PODCASTURL = 15,
/* Podcast RSS URL. Note: this is either a UTF-8 or ASCII encoded
   string (NOT UTF-16). Also, there is no mhod::length value for this
   type. Introduced in db version 0x0d. */
  MHOD_ID_PODCASTRSS = 16,
/* Chapter data. This is a m4a-style entry that is used to display
   subsongs within a mhit. Introduced in db version 0x0d. */
  MHOD_ID_CHAPTERDATA = 17,
/* Subtitle (usually the same as Description). Introduced in db
   version 0x0d. */
  MHOD_ID_SUBTITLE = 18,
/* Show (for TV Shows only). Introduced in db version 0x0d? */
  MHOD_ID_TVSHOW = 19, 
/* Episode # (for TV Shows only). Introduced in db version 0x0d? */
  MHOD_ID_TVEPISODE = 20,
/* TV Network (for TV Shows only). Introduced in db version 0x0d? */
  MHOD_ID_TVNETWORK = 21,
/* Album Artist. Introduced in db version 0x13? */
  MHOD_ID_ALBUMARTIST = 22,
/* This only seems to appear if the Artist has "The" in front of it
  (like "The Monkies") in which case it will be in the format "Artist,
  The" and is followed by 5 0x01 UTF-16 characters. Introduced in db
  version 0x13? */
/*  MHOD_ID_ARTISTTHE = 23,   later */
/* Appears to be a list of keywords pertaining to a track. Introduced
   in db version 0x13? */
  MHOD_ID_KEYWORDS = 24,
  MHOD_ID_SPLPREF = 50,  /* settings for smart playlist */
  MHOD_ID_SPLRULES = 51, /* rules for smart playlist     */
  MHOD_ID_LIBPLAYLISTINDEX = 52,  /* Library Playlist Index */
  MHOD_ID_PLAYLIST = 100
};


/* Used as first iPod ID when renumbering (re-assigning) the iPod
   (track) IDs before writing out the iTunesDB. */
static const gint FIRST_IPOD_ID=52;


enum MHOD52_SORTTYPE {
    MHOD52_SORTTYPE_TITLE    = 0x03,
    MHOD52_SORTTYPE_ALBUM    = 0x04,
    MHOD52_SORTTYPE_ARTIST   = 0x05,
    MHOD52_SORTTYPE_GENRE    = 0x07,
    MHOD52_SORTTYPE_COMPOSER = 0x12
};

struct mhod52track
{
    gchar *album;
    gchar *title;
    gchar *artist;
    gchar *genre;
    gchar *composer;
    gint track_nr;
    gint cd_nr;
    gint index;
    gint numtracks;
};

struct _MHODData
{
    gboolean valid;
    gint32 type;
    union
    {
	gint32 track_pos;
	gchar *string;
	gchar *chapterdata_raw;
	Itdb_Track *chapterdata_track; /* for writing chapterdata */
	SPLPref *splpref;
	SPLRules *splrules;
	GList *mhod52coltracks;
    } data;
    union
    {
	enum MHOD52_SORTTYPE mhod52sorttype;
    } data2;
};

typedef struct _MHODData MHODData;

/* Declarations */
static gboolean itdb_create_directories (Itdb_Device *device, GError **error);


/* ID for error domain */
GQuark itdb_file_error_quark (void)
{
    static GQuark q = 0;
    if (q == 0)
	q = g_quark_from_static_string ("itdb-file-error-quark");
    return q;
}

/* Get length of utf16 string in number of characters (words) */
static guint32 utf16_strlen (gunichar2 *utf16)
{
  guint32 i=0;
  if (utf16)
      while (utf16[i] != 0) ++i;
  return i;
}


/* Read the contents of @filename and return a FContents
   struct. Returns NULL in case of error and @error is set
   accordingly */
static FContents *fcontents_read (const gchar *fname, GError **error)
{
    FContents *cts;

    g_return_val_if_fail (fname, NULL);

    cts = g_new0 (FContents, 1);
    cts->reversed = FALSE;

    if (g_file_get_contents (fname, &cts->contents, &cts->length, error))
    {
	cts->filename = g_strdup (fname);
    }
    else
    {
	g_free (cts);
	cts = NULL;
    }
    return cts;
}


/* Frees the memory taken by a FContents structure. NULL pointer will
 * be ignored */
static void fcontents_free (FContents *cts)
{
    if (cts)
    {
	g_free (cts->filename);
	g_free (cts->contents);
	/* must not g_error_free (cts->error) because the error was
	   propagated -> might free the error twice */
	g_free (cts);
    }
}


/* There seems to be a problem with some distributions (kernel
   versions or whatever -- even identical version numbers don't don't
   show identical behaviour...): even though vfat is supposed to be
   case insensitive, a difference is made between upper and lower case
   under some special circumstances. As in "/iPod_Control/Music/F00"
   and "/iPod_Control/Music/f00 "... If the former filename does not
   exist, we try to find an existing case insensitive match for each
   component of the filename.  If we can find such a match, we return
   it.  Otherwise, we return NULL.*/
     
/**
 * itdb_resolve_path:
 * @root: in local encoding
 * @components: in utf8
 *
 * Resolve the path to a track on the iPod
 *
 * We start by assuming that the ipod mount point exists.  Then, for
 * each component c of track-&gt;ipod_path, we try to find an entry d in
 * good_path that is case-insensitively equal to c.  If we find d, we
 * append d to good_path and make the result the new good_path.
 * Otherwise, we quit and return NULL.
 *
 * Return value: path to track on the iPod or NULL.
 **/
gchar * itdb_resolve_path (const gchar *root,
			   const gchar * const * components)
{
  gchar *good_path = g_strdup(root);
  guint32 i;

  if (!root) return NULL;

  for(i = 0 ; components[i] ; i++) {
    GDir *cur_dir;
    gchar *component_as_filename;
    gchar *test_path;
    gchar *component_stdcase;
    const gchar *dir_file=NULL;

    /* skip empty components */
    if (strlen (components[i]) == 0) continue;
    component_as_filename = 
      g_filename_from_utf8(components[i],-1,NULL,NULL,NULL);
    test_path = g_build_filename(good_path,component_as_filename,NULL);
    g_free(component_as_filename);
    if(g_file_test(test_path,G_FILE_TEST_EXISTS)) {
      /* This component does not require fixup */
      g_free(good_path);
      good_path = test_path;
      continue;
    }
    g_free(test_path);
    component_stdcase = g_utf8_casefold(components[i],-1);
    /* Case insensitively compare the current component with each entry
     * in the current directory. */

    cur_dir = g_dir_open(good_path,0,NULL);
    if (cur_dir) while ((dir_file = g_dir_read_name(cur_dir)))
    {
	gchar *file_utf8 = g_filename_to_utf8(dir_file,-1,NULL,NULL,NULL);
	gchar *file_stdcase = g_utf8_casefold(file_utf8,-1);
	gboolean found = !g_utf8_collate(file_stdcase,component_stdcase);
	gchar *new_good_path;
	g_free(file_stdcase);
	if(!found)
	{
	    /* This is not the matching entry */
	    g_free(file_utf8);
	    continue;
	}
      
	new_good_path = dir_file ? g_build_filename(good_path,dir_file,NULL) : NULL;
	g_free(good_path);
	good_path= new_good_path;
	/* This is the matching entry, so we can stop searching */
	break;
    }
    
    if(!dir_file) {
      /* We never found a matching entry */
      g_free(good_path);
      good_path = NULL;
    }

    g_free(component_stdcase);
    if (cur_dir) g_dir_close(cur_dir);
    if(!good_path || !g_file_test(good_path,G_FILE_TEST_EXISTS))
      break; /* We couldn't fix this component, so don't try later ones */
  }
    
  if(good_path && g_file_test(good_path,G_FILE_TEST_EXISTS))
    return good_path;
          
  return NULL;
}


/* Check if the @seek with length @len is legal or out of
 * range. Returns TRUE if legal and FALSE when it is out of range, in
 * which case cts->error is set as well. */
static gboolean check_seek (FContents *cts, glong seek, glong len)
{
    g_return_val_if_fail (cts, FALSE);
    g_return_val_if_fail (cts->contents, FALSE);

    if ((seek+len <= cts->length) && (seek >=0))
    {
	return TRUE;
    }
    else
    {
	g_return_val_if_fail (cts->filename, FALSE);
	g_set_error (&cts->error,
		     ITDB_FILE_ERROR,
		     ITDB_FILE_ERROR_SEEK,
		     _("Illegal seek to offset %ld (length %ld) in file '%s'."),
		     seek, len, cts->filename);
	return FALSE;
    }
}


/* Copies @len bytes from position @seek in @cts->contents to
   @data. Returns FALSE on error and sets cts->error accordingly. */
static gboolean seek_get_n_bytes (FContents *cts, gchar *data,
			      glong seek, glong len)
{
  if (check_seek (cts, seek, len))
  {
      memcpy (data, &cts->contents[seek], len);
      return TRUE;
  }
  return FALSE;
}

/* Compare @n bytes of @cts->contents starting at @seek and
 * @data. Returns TRUE if equal, FALSE if not. Also returns FALSE on
 * error, so you must check cts->error */
static gboolean cmp_n_bytes_seek (FContents *cts, const gchar *data,
				  glong seek, glong len)
{
    if (check_seek (cts, seek, len))
    {
	gint i;
	for (i=0; i<len; ++i)
	{
	    if (cts->contents[seek+i] != data[i]) return FALSE;
	}
	return TRUE;
    }
    return FALSE;
}


/* Compare 4 bytes of @header with 4 bytes at @seek taking into
 * consideration the status of cts->reversed */
static gboolean check_header_seek (FContents *cts, const gchar *data,
				   glong seek)
{
    gchar rdata[4];
    gint i, offset, sign;

    g_return_val_if_fail (cts, FALSE);
    g_return_val_if_fail (data, FALSE);
    /* reverse data for compare if necessary */
    if (cts->reversed)
    {
	offset = 3;
	sign = -1;
    }
    else
    {
	offset = 0;
	sign = 1;
    }
    for (i=0; i<4; ++i)
    {
	    rdata[i] = data[offset + sign*i];
    }

    return cmp_n_bytes_seek (cts, rdata, seek, 4);
}


/* ------------------------------------------------------------
   Not Endian Dependent
   ------------------------------------------------------------ */

/* Returns the 1-byte number stored at position @seek. On error the
 * GError in @cts is set. */
static guint8 get8int (FContents *cts, glong seek)
{
    guint8 n=0;

    if (check_seek (cts, seek, 1))
    {
	n = cts->contents[seek];
    }
    return n;
}


/* ------------------------------------------------------------
   Little Endian
   ------------------------------------------------------------ */

/* Get the 2-byte-number stored at position "seek" in little endian
   encoding. On error the GError in @cts is set. */
static guint16 raw_get16lint (FContents *cts, glong seek)
{
    guint16 n=0;

    if (check_seek (cts, seek, 2))
    {
	g_return_val_if_fail (cts->contents, 0);
	memcpy (&n, &cts->contents[seek], 2);
	n = GUINT16_FROM_LE (n);
    }
    return n;
}

/* Get the 3-byte-number stored at position "seek" in little endian
   encoding. On error the GError in @cts is set. */
static guint32 raw_get24lint (FContents *cts, glong seek)
{
    guint32 n=0;

    if (check_seek (cts, seek, 3))
    {
	g_return_val_if_fail (cts->contents, 0);
	n = ((guint32)get8int (cts, seek+0)) +
	    (((guint32)get8int (cts, seek+1)) >> 8) +
	    (((guint32)get8int (cts, seek+2)) >> 16);
    }
    return n;
}

/* Get the 4-byte-number stored at position "seek" in little endian
   encoding. On error the GError in @cts is set. */
static guint32 raw_get32lint (FContents *cts, glong seek)
{
    guint32 n=0;

    if (check_seek (cts, seek, 4))
    {
	g_return_val_if_fail (cts->contents, 0);
	memcpy (&n, &cts->contents[seek], 4);
	n = GUINT32_FROM_LE (n);
    }
    return n;
}

/* Get 4 byte floating number */
static float raw_get32lfloat (FContents *cts, glong seek)
{
    union
    {
	guint32 i;
	float   f;
    } flt;

    g_return_val_if_fail (sizeof (float) == 4, 0);

    flt.i = raw_get32lint (cts, seek);

    return flt.f;
}


/* Get the 8-byte-number stored at position "seek" in little endian
   encoding. On error the GError in @cts is set. */
static guint64 raw_get64lint (FContents *cts, glong seek)
{
    guint64 n=0;

    if (check_seek (cts, seek, 8))
    {
	g_return_val_if_fail (cts->contents, 0);
	memcpy (&n, &cts->contents[seek], 8);
	n = GUINT64_FROM_LE (n);
    }
    return n;
}


/* ------------------------------------------------------------
   Big Endian
   ------------------------------------------------------------ */

/* Get the 2-byte-number stored at position "seek" in little endian
   encoding. On error the GError in @cts is set. */
static guint16 raw_get16bint (FContents *cts, glong seek)
{
    guint16 n=0;

    if (check_seek (cts, seek, 2))
    {
	g_return_val_if_fail (cts->contents, 0);
	memcpy (&n, &cts->contents[seek], 2);
	n = GUINT16_FROM_BE (n);
    }
    return n;
}

/* Get the 3-byte-number stored at position "seek" in big endian
   encoding. On error the GError in @cts is set. */
static guint32 raw_get24bint (FContents *cts, glong seek)
{
    guint32 n=0;

    if (check_seek (cts, seek, 3))
    {
	g_return_val_if_fail (cts->contents, 0);
	n = ((guint32)get8int (cts, seek+2)) +
	    (((guint32)get8int (cts, seek+1)) >> 8) +
	    (((guint32)get8int (cts, seek+0)) >> 16);
    }
    return n;
}

/* Get the 4-byte-number stored at position "seek" in big endian
   encoding. On error the GError in @cts is set. */
static guint32 raw_get32bint (FContents *cts, glong seek)
{
    guint32 n=0;

    if (check_seek (cts, seek, 4))
    {
	g_return_val_if_fail (cts->contents, 0);
	memcpy (&n, &cts->contents[seek], 4);
	n = GUINT32_FROM_BE (n);
    }
    return n;
}

/* Get 4 byte floating number */
static float raw_get32bfloat (FContents *cts, glong seek)
{
    union
    {
	guint32 i;
	float   f;
    } flt;

    g_return_val_if_fail (sizeof (float) == 4, 0);

    flt.i = raw_get32bint (cts, seek);

    return flt.f;
}

/* Get the 8-byte-number stored at position "seek" in big endian
   encoding. On error the GError in @cts is set. */
static guint64 raw_get64bint (FContents *cts, glong seek)
{
    guint64 n=0;

    if (check_seek (cts, seek, 8))
    {
	g_return_val_if_fail (cts->contents, 0);
	memcpy (&n, &cts->contents[seek], 8);
	n = GUINT64_FROM_BE (n);
    }
    return n;
}


/* ------------------------------------------------------------
   Reversed Endian Sensitive (little endian)
   ------------------------------------------------------------ */

/* The following functions take into consideration the state of
 * cts->reversed and call either raw_getnnlint or raw_getnnbint */
static guint16 get16lint (FContents *cts, glong seek)
{
    g_return_val_if_fail (cts, 0);
    if (!cts->reversed)
	return raw_get16lint (cts, seek);
    else
	return raw_get16bint (cts, seek);
}

static guint32 get24lint (FContents *cts, glong seek)
{
    g_return_val_if_fail (cts, 0);
    if (!cts->reversed)
	return raw_get24lint (cts, seek);
    else
	return raw_get24bint (cts, seek);
}
#if 0
static guint32 get24bint (FContents *cts, glong seek)
{
    g_return_val_if_fail (cts, 0);
    if (!cts->reversed)
	return raw_get24bint (cts, seek);
    else
	return raw_get24lint (cts, seek);
}
#endif
static guint32 get32lint (FContents *cts, glong seek)
{
    g_return_val_if_fail (cts, 0);
    if (!cts->reversed)
	return raw_get32lint (cts, seek);
    else
	return raw_get32bint (cts, seek);
}

static float get32lfloat (FContents *cts, glong seek)
{
    g_return_val_if_fail (cts, 0);
    if (!cts->reversed)
	return raw_get32lfloat (cts, seek);
    else
	return raw_get32bfloat (cts, seek);
}

static guint64 get64lint (FContents *cts, glong seek)
{
    g_return_val_if_fail (cts, 0);
    if (!cts->reversed)
	return raw_get64lint (cts, seek);
    else
	return raw_get64bint (cts, seek);
}



/* ------------------------------------------------------------
   Reversed Endian Sensitive (big endian)
   ------------------------------------------------------------ */

#if 0
static guint16 get16bint (FContents *cts, glong seek)
{
    g_return_val_if_fail (cts, 0);
    if (!cts->reversed)
	return raw_get16bint (cts, seek);
    else
	return raw_get16lint (cts, seek);
}
#endif
static guint32 get32bint (FContents *cts, glong seek)
{
    g_return_val_if_fail (cts, 0);
    if (!cts->reversed)
	return raw_get32bint (cts, seek);
    else
	return raw_get32lint (cts, seek);
}

#if 0
static float get32bfloat (FContents *cts, glong seek)
{
    g_return_val_if_fail (cts, 0);
    if (!cts->reversed)
	return raw_get32bfloat (cts, seek);
    else
	return raw_get32lfloat (cts, seek);
}
#endif

static guint64 get64bint (FContents *cts, glong seek)
{
    g_return_val_if_fail (cts, 0);
    if (!cts->reversed)
	return raw_get64bint (cts, seek);
    else
	return raw_get64lint (cts, seek);
}




/* Fix little endian UTF16 String to correct byteorder if necessary
 * (all strings in the Itdb_iTunesDB are little endian except for the ones
 * in smart playlists). */
static gunichar2 *fixup_little_utf16 (gunichar2 *utf16_string)
{
#   if (G_BYTE_ORDER == G_BIG_ENDIAN)
    gint32 i;
    if (utf16_string)
    {
	for(i=0; i<utf16_strlen(utf16_string); i++)
	{
	    utf16_string[i] = GUINT16_SWAP_LE_BE (utf16_string[i]);
	}
    }
#   endif
    return utf16_string;
}

/* Fix big endian UTF16 String to correct byteorder if necessary (only
 * strings in smart playlists are big endian) */
static gunichar2 *fixup_big_utf16 (gunichar2 *utf16_string)
{
#   if (G_BYTE_ORDER == G_LITTLE_ENDIAN)
    gint32 i;
    if (utf16_string)
    {
	for(i=0; i<utf16_strlen(utf16_string); i++)
	{
	    utf16_string[i] = GUINT16_SWAP_LE_BE (utf16_string[i]);
	}
    }
#   endif
    return utf16_string;
}


#define CHECK_ERROR(imp, val) if (cts->error) { g_propagate_error (&imp->error, cts->error); return (val); }


/* get next playcount, that is the first entry of GList
 * playcounts. This entry is removed from the list. You must free the
 * return value after use */
static struct playcount *playcount_get_next (FImport *fimp)
{
    struct playcount *playcount;
    g_return_val_if_fail (fimp, NULL);

    playcount = g_list_nth_data (fimp->playcounts, 0);

    if (playcount)
	fimp->playcounts = g_list_remove (fimp->playcounts, playcount);
    return playcount;
}

/* delete all entries of GList *playcounts */
static void playcounts_free (FImport *fimp)
{
    struct playcount *playcount;

    g_return_if_fail (fimp);

    while ((playcount=playcount_get_next (fimp))) g_free (playcount);
}


/* called by init_playcounts */
static gboolean playcounts_read (FImport *fimp, FContents *cts)
{
    guint32 header_length, entry_length, entry_num, i=0;

    g_return_val_if_fail (fimp, FALSE);
    g_return_val_if_fail (cts, FALSE);

    if (!check_header_seek (cts, "mhdp", 0))
    {
	if (cts->error)
	{
	    g_propagate_error (&fimp->error, cts->error);
	    return FALSE;
	}
	cts->reversed = TRUE;
	if (!check_header_seek (cts, "mhdp", 0))
	{
	    if (cts->error)
	    {
		g_propagate_error (&fimp->error, cts->error);
		return FALSE;
	    }
	    else
	    {   /* set error */
		g_return_val_if_fail (cts->filename, FALSE);
		g_set_error (&fimp->error,
			     ITDB_FILE_ERROR,
			     ITDB_FILE_ERROR_CORRUPT,
			     _("Not a Play Counts file: '%s' (missing mhdp header)."),
			     cts->filename);
		return FALSE;
	    }
	}
    }
    header_length = get32lint (cts, 4);
    CHECK_ERROR (fimp, FALSE);
    /* all the headers I know are 0x60 long -- if this one is longer
       we can simply ignore the additional information */
    if (header_length < 0x60)
    {
	g_set_error (&fimp->error,
		     ITDB_FILE_ERROR,
		     ITDB_FILE_ERROR_CORRUPT,
		     _("Play Counts file ('%s'): header length smaller than expected (%d<96)."),
		     cts->filename, header_length);
	return FALSE;
    }
    entry_length = get32lint (cts, 8);
    CHECK_ERROR (fimp, FALSE);
    /* all the entries I know are 0x0c (firmware 1.3) or 0x10
     * (firmware 2.0), 0x14 (iTunesDB version 0x0d) or 0x1c (iTunesDB
     * version 0x13) in length */
    if (entry_length < 0x0c)
    {
	g_set_error (&fimp->error,
		     ITDB_FILE_ERROR,
		     ITDB_FILE_ERROR_CORRUPT,
		     _("Play Counts file ('%s'): entry length smaller than expected (%d<12)."),
		     cts->filename, entry_length);
	return FALSE;
    }
    /* number of entries */
    entry_num = get32lint (cts, 12);
    CHECK_ERROR (fimp, FALSE);
    for (i=0; i<entry_num; ++i)
    {
	struct playcount *playcount = g_new0 (struct playcount, 1);
	glong seek = header_length + i*entry_length;

	check_seek (cts, seek, entry_length);
	CHECK_ERROR (fimp, FALSE);	

	fimp->playcounts = g_list_append (fimp->playcounts, playcount);
	playcount->playcount = get32lint (cts, seek);
	playcount->time_played = get32lint (cts, seek+4);
	playcount->bookmark_time = get32lint (cts, seek+8);
	/* NOTE:
	 *
	 * The iPod (firmware 1.3, 2.0, ...?) doesn't seem to use the
	 * timezone information correctly -- no matter what you set
	 * iPod's timezone to, it will always record as if it were set
	 * to UTC -- we need to subtract the difference between
	 * current timezone and UTC to get a correct
	 * display. -- this should be done by the application were
	 * necessary */
	
	/* rating only exists if the entry length is at least 0x10 */
	if (entry_length >= 0x10)
	{
	    playcount->rating = get32lint (cts, seek+12);
	}
	else
	{
	    playcount->rating = NO_PLAYCOUNT;
	}
	/* unk16 only exists if the entry length is at least 0x14 */
	if (entry_length >= 0x14)
	{
	    playcount->pc_unk16 = get32lint (cts, seek+16);
	}
	/* skip_count and last_skipped only exists if the entry length
	   is at least 0x1c */
	if (entry_length >= 0x1c)
	{
	    playcount->skipcount = get32lint (cts, seek+20);
	    playcount->last_skipped = get32lint (cts, seek+24);
	}
    }
    return TRUE;
}


/* called by init_playcounts */
static gboolean itunesstats_read (FImport *fimp, FContents *cts)
{
    guint32 entry_num, i=0;
    glong seek;

    g_return_val_if_fail (fimp, FALSE);
    g_return_val_if_fail (cts, FALSE);

    /* number of entries */
    entry_num = get32lint (cts, 0);
    CHECK_ERROR (fimp, FALSE);

    seek = 6;
    for (i=0; i<entry_num; ++i)
    {
	struct playcount *playcount = g_new0 (struct playcount, 1);
	guint32 entry_length = get24lint (cts, seek+0);
	CHECK_ERROR (fimp, FALSE);
	if (entry_length < 18)
	{
	    g_set_error (&fimp->error,
			 ITDB_FILE_ERROR,
			 ITDB_FILE_ERROR_CORRUPT,
			 _("iTunesStats file ('%s'): entry length smaller than expected (%d<18)."),
			 cts->filename, entry_length);
	    return FALSE;
	}

	fimp->playcounts = g_list_append (fimp->playcounts, playcount);
	/* NOTE:
	 *
	 * The iPod (firmware 1.3, 2.0, ...?) doesn't seem to use the
	 * timezone information correctly -- no matter what you set
	 * iPod's timezone to, it will always record as if it were set
	 * to UTC -- we need to subtract the difference between
	 * current timezone and UTC to get a correct
	 * display. -- this should be done by the application were
	 * necessary */
	playcount->bookmark_time = get24lint (cts, seek+3);
	CHECK_ERROR (fimp, FALSE);
	playcount->st_unk06 = get24lint (cts, seek+6);
	CHECK_ERROR (fimp, FALSE);
	playcount->st_unk09 = get24lint (cts, seek+9);
	CHECK_ERROR (fimp, FALSE);
	playcount->playcount = get24lint (cts, seek+12);
	CHECK_ERROR (fimp, FALSE);
	playcount->skipped = get24lint (cts, seek+15);
	CHECK_ERROR (fimp, FALSE);
	
	playcount->rating = NO_PLAYCOUNT;

	seek += entry_length;
    }
    return TRUE;
}



/* Read the Play Count file (formed by adding "Play Counts" to the
 * directory component of fimp->itdb->itdb_filename) and set up the
 * GList *playcounts. If no Play Count file is present, attempt to
 * read the iTunesStats file instead, which is used on the Shuffle for
 * the same purpose.
 *
 * Returns TRUE on success (also when no Play Count
 * file is found as this is not an error) and FALSE otherwise, in
 * which case fimp->error is set accordingly. */
static gboolean playcounts_init (FImport *fimp)
{
  const gchar *plc[] = {"Play Counts", NULL};
  const gchar *ist[] = {"iTunesStats", NULL};
  gchar *plcname, *dirname, *istname;
  gboolean result=TRUE;
  struct stat filestat;
  FContents *cts;

  g_return_val_if_fail (fimp, FALSE);
  g_return_val_if_fail (!fimp->error, FALSE);
  g_return_val_if_fail (!fimp->playcounts, FALSE);
  g_return_val_if_fail (fimp->itdb, FALSE);
  g_return_val_if_fail (fimp->itdb->filename, FALSE);

  dirname = g_path_get_dirname (fimp->itdb->filename);

  plcname = itdb_resolve_path (dirname, plc);
  istname = itdb_resolve_path (dirname, ist);

  g_free (dirname);

  /* skip if no playcounts file is present */
  if (plcname)
  {
      /* skip if playcounts file has zero-length (often happens after
       * dosfsck) */
      stat (plcname, &filestat);
      if (filestat.st_size >= 0x60)
      {
	  cts = fcontents_read (plcname, &fimp->error);
	  if (cts)
	  {
	      result = playcounts_read (fimp, cts);
	      fcontents_free (cts);
	  }
	  else
	  {
	      result = FALSE;
	  }
      }
  }
  else if (istname)
  {
      /* skip if iTunesStats file has zero-length (often happens after
       * dosfsck) */
      stat (plcname, &filestat);
      if (filestat.st_size >= 0x06)
      {
	  cts = fcontents_read (istname, &fimp->error);
	  if (cts)
	  {
	      result = itunesstats_read (fimp, cts);
	      fcontents_free (cts);
	  }
	  else
	  {
	      result = FALSE;
	  }
      }
  }

  g_free (plcname);
  g_free (istname);

  return result;
}


/* Free the memory taken by @fimp. fimp->itdb must be freed separately
 * before calling this function */
static void itdb_free_fimp (FImport *fimp)
{
    if (fimp)
    {
	if (fimp->fcontents)  fcontents_free (fimp->fcontents);
	g_list_free (fimp->pos_glist);
	playcounts_free (fimp);
	g_free (fimp);
    }
}

/** 
 * itdb_free:
 * @itdb: an #Itdb_iTunesDB
 *
 * Free the memory taken by @itdb. 
 **/
void itdb_free (Itdb_iTunesDB *itdb)
{
    if (itdb)
    {
	g_list_foreach (itdb->playlists,
			(GFunc)(itdb_playlist_free), NULL);
	g_list_free (itdb->playlists);
	g_list_foreach (itdb->tracks,
			(GFunc)(itdb_track_free), NULL);
	g_list_free (itdb->tracks);
	g_free (itdb->filename);
	itdb_device_free (itdb->device);
	if (itdb->userdata && itdb->userdata_destroy)
	    (*itdb->userdata_destroy) (itdb->userdata);
	g_free (itdb);
    }
}

/**
 * itdb_duplicate:
 * @itdb: an #Itdb_iTunesDB
 *
 * Duplicate @itdb 
 * FIXME: not implemented yet 
 *
 * Return value: always return NULL since it's unimplemented
 **/
Itdb_iTunesDB *itdb_duplicate (Itdb_iTunesDB *itdb)
{
    g_return_val_if_fail (itdb, NULL);
    /* FIXME: not yet implemented */
    g_return_val_if_reached (NULL);
}

/**
 * itdb_playlists_number:
 * @itdb: an #Itdb_iTunesDB
 *
 * Counts the number of playlists stored in @itdb
 *
 * Return value: the number of playlists in @itdb (including the master 
 * playlist)
 **/
guint32 itdb_playlists_number (Itdb_iTunesDB *itdb)
{
    g_return_val_if_fail (itdb, 0);

    return g_list_length (itdb->playlists);
}


/**
 * itdb_tracks_number:
 * @itdb: an #Itdb_iTunesDB
 *
 * Counts the number of tracks stored in @itdb
 *
 * Return value: the number of tracks in @itdb
 **/
guint32 itdb_tracks_number (Itdb_iTunesDB *itdb)
{
    g_return_val_if_fail (itdb, 0);

    return g_list_length (itdb->tracks);
}

/**
 * itdb_tracks_number_nontransferred:
 * @itdb: an #Itdb_iTunesDB
 *
 * Counts the number of non-transferred tracks in @itdb
 *
 * Return value: the number of tracks in @itdb that haven't been transferred
 * to the iPod yet (ie the number of #Itdb_Track in which the transferred field
 * is false)
 **/
guint32 itdb_tracks_number_nontransferred (Itdb_iTunesDB *itdb)
{
    guint n = 0;
    GList *gl;
    g_return_val_if_fail (itdb, 0);

    for (gl=itdb->tracks; gl; gl=gl->next)
    {
	Itdb_Track *track = gl->data;
	g_return_val_if_fail (track, 0);
	if (!track->transferred)   ++n;
    }
    return n;
}



/**
 * itdb_new:
 *
 * Creates a new Itdb_iTunesDB with the unknowns filled in to reasonable
 * values.
 *
 * Return value: a newly created Itdb_iTunesDB to be freed with itdb_free()
 * when it's no longer needed
 **/
Itdb_iTunesDB *itdb_new (void)
{
    static GOnce g_type_init_once = G_ONCE_INIT;
    Itdb_iTunesDB *itdb;

    g_once (&g_type_init_once, (GThreadFunc)g_type_init, NULL);
    itdb = g_new0 (Itdb_iTunesDB, 1);
    itdb->device = itdb_device_new ();
    itdb->version = 0x13;
    itdb->id = ((guint64)g_random_int () << 32) |
	((guint64)g_random_int ());
    return itdb;
}

/* Returns the type of the mhod and the length *ml. *ml is set to -1
 * on error (e.g. because there's no mhod at @seek). */
/* A return value of -1 and no error set means that no mhod was found
   at @seek */
static gint32 get_mhod_type (FContents *cts, glong seek, guint32 *ml)
{
    gint32 type = -1;

#if ITUNESDB_DEBUG
    fprintf(stderr, "get_mhod_type seek: %x\n", (int)seek);
#endif

    if (ml) *ml = -1;

    if (check_header_seek (cts, "mhod", seek))
    {
	guint32 len = get32lint (cts, seek+8);    /* total length */
	if (cts->error) return -1;
	if (ml) *ml = len;
	type = get32lint (cts, seek+12);          /* mhod_id      */
	if (cts->error) return -1;
    }
    return type;
}

/* Returns the contents of the mhod at position @mhod_seek. This can
   be a simple string or something more complicated as in the case for
   SPLPREF OR SPLRULES.

   *mhod_len is set to the total length of the mhod (-1 in case an
   *error occured).

   MHODData.valid is set to FALSE in case of any error. cts->error
   will be set accordingly.

   MHODData.type is set to the type of the mhod. The data (or a
   pointer to the data) will be stored in
   .playlist_id/.string/.chapterdata/.splp/.splrs
*/

static MHODData get_mhod (FContents *cts, glong mhod_seek, guint32 *ml)
{
  gunichar2 *entry_utf16 = NULL;
  MHODData result;
  gint32 xl;
  guint32 mhod_len;
  gint32 header_length;
  guint32 string_type;
  gulong seek;

  result.valid = FALSE;
  result.type = -1;
  g_return_val_if_fail (ml, result);
  *ml = -1;

  g_return_val_if_fail (cts, result);
  g_return_val_if_fail (!cts->error, result);

#if ITUNESDB_DEBUG
  fprintf(stderr, "get_mhod seek: %ld\n", mhod_seek);
#endif

  result.type = get_mhod_type (cts, mhod_seek, &mhod_len);

  if (mhod_len == -1)
  {
      if (!cts->error)
      {   /* set error */
	  g_set_error (&cts->error,
		       ITDB_FILE_ERROR,
		       ITDB_FILE_ERROR_CORRUPT,
		       _("iTunesDB corrupt: no MHOD at offset %ld in file '%s'."),
		       mhod_seek, cts->filename);
      }
      return result;
  }

  if (!check_seek (cts, mhod_seek, mhod_len))
      return result;


  header_length = get32lint (cts, mhod_seek+4); /* header length  */

  seek = mhod_seek + header_length;

#if ITUNESDB_DEBUG
  fprintf(stderr, "ml: %x type: %x\n", *ml, result.type);
#endif

  switch ((enum MHOD_ID)result.type)
  {
  case MHOD_ID_LIBPLAYLISTINDEX:
      /* this is not yet supported */
  case MHOD_ID_PLAYLIST:
      /* return the position indicator */
      result.data.track_pos = get32lint (cts, mhod_seek+24);
      break;
  case MHOD_ID_TITLE:
  case MHOD_ID_PATH:
  case MHOD_ID_ALBUM:
  case MHOD_ID_ARTIST:
  case MHOD_ID_GENRE:
  case MHOD_ID_FILETYPE:
  case MHOD_ID_COMMENT:
  case MHOD_ID_CATEGORY:
  case MHOD_ID_COMPOSER:
  case MHOD_ID_GROUPING:
  case MHOD_ID_DESCRIPTION:
  case MHOD_ID_SUBTITLE:
  case MHOD_ID_TVSHOW:
  case MHOD_ID_TVEPISODE:
  case MHOD_ID_TVNETWORK:
  case MHOD_ID_ALBUMARTIST:
  case MHOD_ID_KEYWORDS:
      /* type of string: 0x02: UTF8, 0x01 or 0x00: UTF16 LE */
      string_type = get32lint (cts, seek);
      xl = get32lint (cts, seek+4);   /* length of string */
      g_return_val_if_fail (xl < G_MAXUINT - 2, result);
      if (string_type != 0x02)
      {
	  entry_utf16 = g_new0 (gunichar2, (xl+2)/2);
	  if (seek_get_n_bytes (cts, (gchar *)entry_utf16, seek+16, xl))
	  {
	      fixup_little_utf16 (entry_utf16);
	      result.data.string = g_utf16_to_utf8 (entry_utf16, -1,
						    NULL, NULL, NULL);
	      g_free (entry_utf16);
	  }
	  else
	  {   /* error */
	      g_free (entry_utf16);
	      return result;  /* *ml==-1, result.valid==FALSE */
	  }
      }
      else
      {
	  result.data.string = g_new0 (gchar, xl+1);
	  if (!seek_get_n_bytes (cts, result.data.string, seek+16, xl))
	  {   /* error */
	      g_free (entry_utf16);
	      return result;  /* *ml==-1, result.valid==FALSE */
	  }
      }
      break;
  case MHOD_ID_PODCASTURL:
  case MHOD_ID_PODCASTRSS:
      /* length of string */
      xl = mhod_len - header_length;
      g_return_val_if_fail (xl < G_MAXUINT - 1, result);
      result.data.string = g_new0 (gchar, xl+1);
      if (!seek_get_n_bytes (cts, result.data.string, seek, xl))
      {
	  g_free (result.data.string);
	  return result;  /* *ml==-1, result.valid==FALSE */
      }
      break;
  case MHOD_ID_CHAPTERDATA: 
      /* we'll just copy the entire data section */
      xl = mhod_len - header_length;
      result.data.chapterdata_raw = g_new0 (gchar, xl);
      if (!seek_get_n_bytes (cts, result.data.chapterdata_raw, seek, xl))
      {
	  g_free (result.data.chapterdata_raw);
	  return result;  /* *ml==-1, result.valid==FALSE */
      }
      break;
  case MHOD_ID_SPLPREF:  /* Settings for smart playlist */
      if (!check_seek (cts, seek, 14))
	  return result;  /* *ml==-1, result.valid==FALSE */
      result.data.splpref = g_new0 (SPLPref, 1);
      result.data.splpref->liveupdate = get8int (cts, seek);
      result.data.splpref->checkrules = get8int (cts, seek+1);
      result.data.splpref->checklimits = get8int (cts, seek+2);
      result.data.splpref->limittype = get8int (cts, seek+3);
      result.data.splpref->limitsort = get8int (cts, seek+4);
      result.data.splpref->limitvalue = get32lint (cts, seek+8);
      result.data.splpref->matchcheckedonly = get8int (cts, seek+12);
      /* if the opposite flag is on (seek+13), set limitsort's high
	 bit -- see note in itunesdb.h for more info */
      if (get8int (cts, seek+13))
	  result.data.splpref->limitsort |= 0x80000000;
      break;
  case MHOD_ID_SPLRULES:  /* Rules for smart playlist */
      if (check_header_seek (cts, "SLst", seek))
      {
	  /* !!! for some reason the SLst part is the only part of the
	     iTunesDB with big-endian encoding, including UTF16
	     strings */
	  gint i;
	  guint32 numrules;
	  if (!check_seek (cts, seek, 136))
	      return result;  /* *ml==-1, result.valid==FALSE */
	  result.data.splrules = g_new0 (SPLRules, 1);
	  result.data.splrules->unk004 = get32bint (cts, seek+4);
	  numrules = get32bint (cts, seek+8);
	  result.data.splrules->match_operator = get32bint (cts, seek+12);
	  seek += 136;  /* I can't find this value stored in the
			   iTunesDB :-( */
	  for (i=0; i<numrules; ++i)
	  {
	      guint32 length;
	      SPLFieldType ft;
	      gunichar2 *string_utf16;
	      SPLRule *splr = g_new0 (SPLRule, 1);
	      result.data.splrules->rules = g_list_append (
		  result.data.splrules->rules, splr);
	      if (!check_seek (cts, seek, 56))
		  goto splrules_error;
	      splr->field = get32bint (cts, seek);
	      splr->action = get32bint (cts, seek+4);

	      if (!itdb_spl_action_known (splr->action))
	      {
		  g_warning (_("Unknown smart rule action at %ld: %x. Trying to continue.\n"), seek, splr->action);
	      }

	      seek += 52;
	      length = get32bint (cts, seek);
	      g_return_val_if_fail (length < G_MAXUINT-2, result);

	      ft = itdb_splr_get_field_type (splr);
	      switch (ft)
	      {
	      case splft_string:
		  string_utf16 = g_new0 (gunichar2, (length+2)/2);
		  if (!seek_get_n_bytes (cts, (gchar *)string_utf16,
					 seek+4, length))
		  {
		      g_free (string_utf16);
		      goto splrules_error;
		  }
		  fixup_big_utf16 (string_utf16);
		  splr->string = g_utf16_to_utf8 (
		      string_utf16, -1, NULL, NULL, NULL);
		  g_free (string_utf16);
		  break;
	      case splft_int:
	      case splft_date:
	      case splft_boolean:
	      case splft_playlist:
	      case splft_unknown:
	      case splft_binary_and:
		  if (length != 0x44)
		  {
		      g_warning (_("Length of smart playlist rule field (%d) not as expected. Trying to continue anyhow.\n"), length);
		  }
		  if (!check_seek (cts, seek, 72))
		      goto splrules_error;
		  splr->fromvalue = get64bint (cts, seek+4);
		  splr->fromdate = get64bint (cts, seek+12);
		  splr->fromunits = get64bint (cts, seek+20);
		  splr->tovalue = get64bint (cts, seek+28);
		  splr->todate = get64bint (cts, seek+36);
		  splr->tounits = get64bint (cts, seek+44);
		  /* SPLFIELD_PLAYLIST seems to use these unknowns*/
		  splr->unk052 = get32bint (cts, seek+52);
		  splr->unk056 = get32bint (cts, seek+56);
		  splr->unk060 = get32bint (cts, seek+60);
		  splr->unk064 = get32bint (cts, seek+64);
		  splr->unk068 = get32bint (cts, seek+68);
		  break;
	      }
	      seek += length+4;
	  }
      }
      else
      {
	  if (!cts->error)
	  {   /* set error */
	      g_set_error (&cts->error,
			   ITDB_FILE_ERROR,
			   ITDB_FILE_ERROR_CORRUPT,
			   _("iTunesDB corrupt: no SLst at offset %ld in file '%s'."),
			   seek, cts->filename);
	  }
	  return result;  /* *ml==-1, result.valid==FALSE */
      }
      break;
    splrules_error:
      g_list_foreach (result.data.splrules->rules,
		      (GFunc)(itdb_splr_free), NULL);
      g_list_free (result.data.splrules->rules);
      g_free (result.data.splrules);
      return result;  /* *ml==-1, result.valid==FALSE */
  default:
      g_warning (_("Encountered unknown MHOD type (%d) while parsing the iTunesDB. Ignoring.\n\n"), result.type);
      *ml = mhod_len;
      return result;
  }

  *ml = mhod_len;
  result.valid = TRUE;
  return result;
}

/* Returns the value of a string type mhod. return the length of the
   mhod *ml, the mhod type *mty, and a string with the entry (in
   UTF8). After use you must free the string with g_free(). Returns
   NULL if no string is avaible. *ml is set to -1 in case of error and
   cts->error is set appropriately. */
static gchar *get_mhod_string (FContents *cts, glong seek, guint32 *ml, gint32 *mty)
{
    MHODData mhoddata;

    *mty = get_mhod_type (cts, seek, ml);
    if (cts->error) return NULL;

    if (*ml != -1) switch ((enum MHOD_ID)*mty)
    {
    case MHOD_ID_TITLE:
    case MHOD_ID_PATH:
    case MHOD_ID_ALBUM:
    case MHOD_ID_ARTIST:
    case MHOD_ID_GENRE:
    case MHOD_ID_FILETYPE:
    case MHOD_ID_COMMENT:
    case MHOD_ID_CATEGORY:
    case MHOD_ID_COMPOSER:
    case MHOD_ID_GROUPING:
    case MHOD_ID_DESCRIPTION:
    case MHOD_ID_PODCASTURL:
    case MHOD_ID_PODCASTRSS:
    case MHOD_ID_SUBTITLE:
    case MHOD_ID_TVSHOW:
    case MHOD_ID_TVEPISODE:
    case MHOD_ID_TVNETWORK:
    case MHOD_ID_ALBUMARTIST:
    case MHOD_ID_KEYWORDS:
	mhoddata = get_mhod (cts, seek, ml);
	if ((*ml != -1) && mhoddata.valid)
	    return mhoddata.data.string;
	else
	    return NULL;
    case MHOD_ID_SPLPREF:
    case MHOD_ID_SPLRULES:
    case MHOD_ID_LIBPLAYLISTINDEX:
    case MHOD_ID_PLAYLIST:
    case MHOD_ID_CHAPTERDATA:
	/* these do not have a string entry */
	return NULL;
    }
#if ITUNESDB_MHIT_DEBUG
    fprintf (stderr, "Ignoring unknown MHOD of type %d at offset %ld\n", *mty, seek);
#endif
    return NULL;
}


/* convenience function: set error for zero length hunk */
static void set_error_zero_length_hunk (GError **error, glong seek,
					const gchar *filename)
{
    g_set_error (error,
		 ITDB_FILE_ERROR,
		 ITDB_FILE_ERROR_CORRUPT,
		 _("iTunesDB corrupt: hunk length 0 for hunk at %ld in file '%s'."),
		 seek, filename);
}

/* convenience function: set error for missing hunk */
static void set_error_a_not_found_in_b (GError **error,
					const gchar *a,
					const gchar *b,
					glong b_seek)
{
    g_set_error (error,
		 ITDB_FILE_ERROR,
		 ITDB_FILE_ERROR_CORRUPT,
		 _("iTunesDB corrupt: no section '%s' found in section '%s' starting at %ld."),
		 a, b, b_seek);
}

/* convenience function: set error if header is smaller than expected */
static void set_error_a_header_smaller_than_b (GError **error,
					       const gchar *a,
					       guint32 b, guint32 len,
					       glong a_seek,
					       const gchar *filename)
{
      g_set_error (error,
		   ITDB_FILE_ERROR,
		   ITDB_FILE_ERROR_CORRUPT,
		   _("header length of '%s' smaller than expected (%d < %d) at offset %ld in file '%s'."),
		   a, b, len, a_seek, filename);
}


/* finds next occurence of section @a in section b (@b_seek) starting
   at @start_seek
*/
/* Return value:
   -1 and cts->error not set: section @a could not be found
   -1 and cts->error set: some error occured
   >=0: start of next occurence of section @a
*/
static glong find_next_a_in_b (FContents *cts,
			       const gchar *a,
			       glong b_seek, glong start_seek)
{
    glong b_len;
    glong offset, len;

    g_return_val_if_fail (a, -1);
    g_return_val_if_fail (cts, -1);
    g_return_val_if_fail (strlen (a) == 4, -1);
    g_return_val_if_fail (b_seek>=0, -1);
    g_return_val_if_fail (start_seek >= b_seek, -1);

/*     printf ("%s: b_seek: %lx, start_seek: %lx\n", a, b_seek, start_seek); */

    b_len = get32lint (cts, b_seek+8);
    if (cts->error) return -1;

    offset = start_seek - b_seek;
    len = 0;
    do
    {   /* skip headers inside the b hunk (b_len) until we find header
	   @a */
	len = get32lint (cts, b_seek+offset+4);
	if (cts->error) return -1;
	if (len == 0)
	{   /* This needs to be checked, otherwise we might hang */
	    set_error_zero_length_hunk (&cts->error, b_seek+offset,
					cts->filename);
	    return -1;
	}
	offset += len;
/* 	printf ("offset: %lx, b_len: %lx, bseek+offset: %lx\n",  */
/* 		offset, b_len, b_seek+offset); */
    } while ((offset < b_len-4) && 
	     !check_header_seek (cts, a, b_seek+offset));
    if (cts->error) return -1;

    if (offset >= b_len)	return -1;

/*     printf ("%s found at %lx\n", a, b_seek+offset); */

    return b_seek+offset;
}




/* return the position of mhsd with type @type */
/* Return value:
     -1 if mhsd cannot be found. cts->error will not be set

     0 and cts->error is set if some other error occurs.
     Since the mhsd can never be at position 0 (a mhbd must be there),
     a return value of 0 always indicates an error.
*/
static glong find_mhsd (FContents *cts, guint32 type)
{
    guint32 i, len, mhsd_num;
    glong seek;

    if (!check_header_seek (cts, "mhbd", 0))
    {
	cts->reversed = TRUE;
	if (cts->error) return 0;
	if (!check_header_seek (cts, "mhbd", 0))
	{
	    if (!cts->error)
	    {   /* set error */
		g_set_error (&cts->error,
			     ITDB_FILE_ERROR,
			     ITDB_FILE_ERROR_CORRUPT,
			     _("Not a iTunesDB: '%s' (missing mhdb header)."),
			     cts->filename);
	    }
	    return 0;
	}
    }
    len = get32lint (cts, 4);
    if (cts->error) return 0;
    /* all the headers I know are 0x68 long -- if this one is longer
       we can could simply ignore the additional information */
    /* Since 'we' (parse_fimp()) only need data from the first 32
       bytes, don't complain unless it's smaller than that */
    if (len < 32)
    {
	g_set_error (&cts->error,
		     ITDB_FILE_ERROR,
		     ITDB_FILE_ERROR_CORRUPT,
		     _("iTunesDB ('%s'): header length of mhsd hunk smaller than expected (%d<32). Aborting."),
		     cts->filename, len);
	return FALSE;
    }

    mhsd_num = get32lint (cts, 20);
    if (cts->error) return 0;

    seek = 0;
    for (i=0; i<mhsd_num; ++i)
    {
	guint32 mhsd_type;

	seek += len;
	if (!check_header_seek (cts, "mhsd", seek))
	{
	    if (!cts->error)
	    {   /* set error */
		g_set_error (&cts->error,
			     ITDB_FILE_ERROR,
			     ITDB_FILE_ERROR_CORRUPT,
			     _("iTunesDB '%s' corrupt: mhsd expected at %ld."),
			     cts->filename, seek);
	    }
	    return 0;
	}
	len = get32lint (cts, seek+8);
	if (cts->error) return 0;
	
	mhsd_type = get32lint (cts, seek+12);
	if (cts->error) return 0;

	if (mhsd_type == type) return seek;
    }
    return -1;
}



gint pos_comp (gpointer a, gpointer b);
gint pos_comp (gpointer a, gpointer b)
{
    return (GPOINTER_TO_UINT(a) - GPOINTER_TO_UINT(b));
}


/* Read and process the mhip at @seek. Return a pointer to the next
   possible mhip. */
/* Return value: -1 if no mhip is present at @seek */
static glong get_mhip (FImport *fimp, Itdb_Playlist *plitem,
		       glong mhip_seek)
{
    gboolean first_entry = TRUE;
    FContents *cts;
    guint32 mhip_hlen, mhip_len, mhod_num, mhod_seek;
    Itdb_Track *tr;
    gint32 i, pos=-1;
    gint32 mhod_type;
    guint32 trackid;


    g_return_val_if_fail (fimp, -1);
    g_return_val_if_fail (plitem, -1);

    cts = fimp->fcontents;

    if (!check_header_seek (cts, "mhip", mhip_seek))
    {
	CHECK_ERROR (fimp, -1);
	return -1;
    }

    mhip_hlen = get32lint (cts, mhip_seek+4);
    CHECK_ERROR (fimp, -1);

    if (mhip_hlen < 36)
    {
	set_error_a_header_smaller_than_b (&fimp->error,
					   "mhip",
					   mhip_hlen, 36,
					   mhip_seek, cts->filename);
	return -1;
    }

    /* Check if entire mhip header can be read -- that way we won't
       have to check for read errors every time we access a single
       byte */

    check_seek (cts, mhip_seek, mhip_hlen);
    CHECK_ERROR (fimp, -1);

    mhip_len = get32lint (cts, mhip_seek+8);
    mhod_num = get32lint (cts, mhip_seek+12);
    trackid = get32lint(cts, mhip_seek+24);

    mhod_seek = mhip_seek + mhip_hlen;
 
    /* the mhod that follows gives us the position in the
       playlist (type 100). Just for flexibility, we scan all
       following mhods and pick the type 100 */
    for (i=0; i<mhod_num; ++i)
    {
	guint32 mhod_len;

	mhod_type = get_mhod_type (cts, mhod_seek, &mhod_len);
	CHECK_ERROR (fimp, -1);
	if (mhod_type == MHOD_ID_PLAYLIST)
	{
	    MHODData mhod;
	    mhod = get_mhod (cts, mhod_seek, &mhod_len);
	    CHECK_ERROR (fimp, -1);
	    pos = -1;
	    if (mhod.valid && first_entry)
	    {
		/* The posids don't have to be in numeric order, but our
		   database depends on the playlist members being sorted
		   according to the order they appear in the
		   playlist. Therefore we need to find out at which
		   position to insert the track */
		fimp->pos_glist = g_list_insert_sorted (
		    fimp->pos_glist,
		    GUINT_TO_POINTER(mhod.data.track_pos),
		    (GCompareFunc)pos_comp);
		pos = g_list_index (
		    fimp->pos_glist,
		    GUINT_TO_POINTER(mhod.data.track_pos));
		/* for speedup: pos==-1 is appending at the end */
		if (pos == fimp->pos_len)   pos = -1;
		/* don't call this section more than once (it never
		   should happen except in the case of corrupted
		   iTunesDBs...) */
		first_entry = FALSE;
	    }
	}
	else
	{
	    if (mhod_len == -1)
	    {
		g_warning (_("Number of MHODs in mhip at %ld inconsistent in file '%s'."),
			   mhip_seek, cts->filename);
		break;
	    }
	}
	mhod_seek += mhod_len;
    }

    tr = itdb_track_id_tree_by_id (fimp->idtree, trackid);
    if (tr)
    {
	itdb_playlist_add_track (plitem, tr, pos);
	++fimp->pos_len;
    }
    else
    {
	if (plitem->podcastflag == ITDB_PL_FLAG_NORM)
	{
	g_warning (_("Itdb_Track ID '%d' not found.\n"), trackid);
	}
    }

    /* Up to iTunesd V4.7 or so the mhip_len was set incorrectly
       (mhip_len == mhip_hlen). In that case we need to find the seek
       to the next mhip by going through all mhods.
    */
    if ((mhip_len == mhip_hlen) && (mhod_num > 0))
	return mhod_seek;
    else
	return mhip_seek+mhip_len;
}




/* Get a playlist. Returns the position where the next playlist should
   be. On error -1 is returned and fimp->error is set
   appropriately. */
/* get_mhyp */
static glong get_playlist (FImport *fimp, glong mhyp_seek)
{
  guint32 i, mhipnum, mhod_num;
  glong nextseek, mhod_seek, mhip_seek;
  guint32 header_len;
  Itdb_Playlist *plitem = NULL;
  FContents *cts;

#if ITUNESDB_DEBUG
  fprintf(stderr, "mhyp seek: %x\n", (int)mhyp_seek);
#endif
  g_return_val_if_fail (fimp, -1);
  g_return_val_if_fail (fimp->idtree, -1);
  g_return_val_if_fail (fimp->pos_glist == NULL, -1);
  g_return_val_if_fail (fimp->pos_len == 0, -1);

  cts = fimp->fcontents;

  if (!check_header_seek (cts, "mhyp", mhyp_seek))
  {
      if (cts->error)
	  g_propagate_error (&fimp->error, cts->error);
      return -1;
  }
  header_len = get32lint (cts, mhyp_seek+4); /* length of header */
  CHECK_ERROR (fimp, -1);

  if (header_len < 48)
  {
      set_error_a_header_smaller_than_b (&fimp->error,
					 "mhyp",
					 header_len, 48,
					 mhyp_seek, cts->filename);
      return -1;
  }

  /* Check if entire mhyp can be read -- that way we won't have to
   * check for read errors every time we access a single byte */

  check_seek (cts, mhyp_seek, header_len);
  CHECK_ERROR (fimp, -1);

  nextseek = mhyp_seek + get32lint (cts, mhyp_seek+8);/* possible begin of next PL */  mhod_num = get32lint (cts, mhyp_seek+12); /* number of MHODs we expect */
  mhipnum = get32lint (cts, mhyp_seek+16); /* number of tracks
					       (mhips) in playlist */

  plitem = itdb_playlist_new (NULL, FALSE);

  /* Some Playlists have added 256 to their type -- I don't know what
     it's for, so we just ignore it for now -> & 0xff */
  plitem->type = get8int (cts, mhyp_seek+20);
  plitem->flag1 = get8int (cts, mhyp_seek+21);
  plitem->flag2 = get8int (cts, mhyp_seek+22);
  plitem->flag3 = get8int (cts, mhyp_seek+23);
  plitem->timestamp = get32lint (cts, mhyp_seek+24);
  plitem->id = get64lint (cts, mhyp_seek+28);
/*  plitem->mhodcount = get32lint (cts, mhyp_seek+36);   */
/*  plitem->libmhodcount = get16lint (cts, mhyp_seek+40);*/
  plitem->podcastflag = get16lint (cts, mhyp_seek+42);
  plitem->sortorder = get32lint (cts, mhyp_seek+44);

  mhod_seek = mhyp_seek + header_len;

  for (i=0; i < mhod_num; ++i)
  {
      gint32 type;
      MHODData mhod;

      type = get_mhod_type (cts, mhod_seek, &header_len);
      CHECK_ERROR (fimp, -1);
      if (header_len != -1)
      {
	  switch ((enum MHOD_ID)type)
	  {
	  case MHOD_ID_PLAYLIST:
	      /* here we could do something about the playlist settings */
	      break;
	  case MHOD_ID_TITLE:
	      mhod = get_mhod (cts, mhod_seek, &header_len);
	      CHECK_ERROR (fimp, -1);
	      if (mhod.valid && mhod.data.string)
	      {
		  /* sometimes there seem to be two mhod TITLE headers */
		  g_free (plitem->name);
		  plitem->name = mhod.data.string;
		  mhod.valid = FALSE;
	      }
	      break;
	  case MHOD_ID_SPLPREF:
	      mhod = get_mhod (cts, mhod_seek, &header_len);
	      CHECK_ERROR (fimp, -1);
	      if (mhod.valid && mhod.data.splpref)
	      {
		  plitem->is_spl = TRUE;
		  memcpy (&plitem->splpref, mhod.data.splpref,
			  sizeof (SPLPref));
		  g_free (mhod.data.splpref);
		  mhod.valid = FALSE;
	      }
	      break;
	  case MHOD_ID_SPLRULES:
	      mhod = get_mhod (cts, mhod_seek, &header_len);
	      CHECK_ERROR (fimp, -1);
	      if (mhod.valid && mhod.data.splrules)
	      {
		  plitem->is_spl = TRUE;
		  memcpy (&plitem->splrules, mhod.data.splrules,
			  sizeof (SPLRules));
		  g_free (mhod.data.splrules);
		  mhod.valid = FALSE;
	      }
	      break;
	  case MHOD_ID_PATH:
	  case MHOD_ID_ALBUM:
	  case MHOD_ID_ARTIST:
	  case MHOD_ID_GENRE:
	  case MHOD_ID_FILETYPE:
	  case MHOD_ID_COMMENT:
	  case MHOD_ID_CATEGORY:
	  case MHOD_ID_COMPOSER:
	  case MHOD_ID_GROUPING:
	  case MHOD_ID_DESCRIPTION:
	  case MHOD_ID_PODCASTURL:
	  case MHOD_ID_PODCASTRSS:
	  case MHOD_ID_SUBTITLE:
	  case MHOD_ID_TVSHOW:
	  case MHOD_ID_TVEPISODE:
	  case MHOD_ID_TVNETWORK:
	  case MHOD_ID_ALBUMARTIST:
	  case MHOD_ID_KEYWORDS:
	  case MHOD_ID_CHAPTERDATA:
	      /* these are not expected here */
	      break;
	  case MHOD_ID_LIBPLAYLISTINDEX:
	      /* this I don't know how to handle */
	      break;
	  }
	  mhod_seek += header_len;
      }
      else
      {
	  g_warning (_("Number of MHODs in mhyp at %ld inconsistent in file '%s'."),
		     mhyp_seek, cts->filename);
	  break;
      }
  }

  if (!plitem->name)
  {   /* we did not read a valid mhod TITLE header -> */
      /* we simply make up our own name */
      if (itdb_playlist_is_mpl (plitem))
	  plitem->name = _("Master-PL");
      else
      {
	  if (itdb_playlist_is_podcasts (plitem))
	      plitem->name = _("Podcasts");
	  else
	      plitem->name = _("Playlist");
      }
  }

#if ITUNESDB_DEBUG
  fprintf(stderr, "pln: %s(%d Itdb_Tracks) \n", plitem->name, (int)tracknum);
#endif

  /* add new playlist */
  itdb_playlist_add (fimp->itdb, plitem, -1);

  mhip_seek = mhod_seek;

  i=0; /* tracks read */
  for (i=0; i < mhipnum; ++i)
  {
      mhip_seek = get_mhip (fimp, plitem, mhip_seek);
      if (mhip_seek == -1)
      {
	  g_set_error (&fimp->error,
		       ITDB_FILE_ERROR,
		       ITDB_FILE_ERROR_CORRUPT,
		       _("iTunesDB corrupt: number of mhip sections inconsistent in mhyp starting at %ld in file '%s'."),
		       mhyp_seek, cts->filename);
	  return -1;
      }
  }	  


  g_list_free (fimp->pos_glist);
  fimp->pos_glist = NULL;
  fimp->pos_len = 0;
  return nextseek;
}


/* returns a pointer to the next header or -1 on error. fimp->error is
   set appropriately. If no "mhit" header is found at the location
   specified, -1 is returned but no error is set. */
static glong get_mhit (FImport *fimp, glong mhit_seek)
{
  Itdb_Track *track;
  gchar *entry_utf8;
  gint32 type;
  guint32 header_len;
  guint32 zip;
  struct playcount *playcount;
  guint32 i, mhod_nums;
  FContents *cts;
  glong seek = mhit_seek;

#if ITUNESDB_DEBUG
  fprintf(stderr, "get_mhit seek: %x\n", (int)seek);
#endif

  g_return_val_if_fail (fimp, -1);

  cts = fimp->fcontents;

  if (!check_header_seek (cts, "mhit", seek))
  {
      if (cts->error)
	  g_propagate_error (&fimp->error, cts->error);
      return -1;
  }

  header_len = get32lint (cts, seek+4);
  CHECK_ERROR (fimp, -1);

  /* size of the mhit header: For dbversion <= 0x0b (iTunes 4.7 and
     earlier), the length is 0x9c. As of dbversion 0x0c and 0x0d
     (iTunes 4.7.1 - iTunes 4.9), the size is 0xf4. */
  if (header_len < 0x9c)
  {
      set_error_a_header_smaller_than_b (&fimp->error,
					 "mhit",
					 header_len, 0x9c,
					 seek, cts->filename);
      return -1;
  }

  /* Check if entire mhit can be read -- that way we won't have to
   * check for read errors every time we access a single byte */

  check_seek (cts, seek, header_len);

  mhod_nums = get32lint (cts, seek+12);
  CHECK_ERROR (fimp, -1);


  track = itdb_track_new ();

  if (header_len >= 0x9c)
  {
      guint32 val32;
      track->id = get32lint(cts, seek+16);         /* iPod ID          */
      track->visible = get32lint (cts, seek+20);
      track->filetype_marker = get32lint (cts, seek+24);
      track->type1 = get8int (cts, seek+28);
      track->type2 = get8int (cts, seek+29);
      track->compilation = get8int (cts, seek+30);
      track->rating = get8int (cts, seek+31);
      track->time_modified = get32lint(cts, seek+32); /* time added       */
      track->size = get32lint(cts, seek+36);       /* file size        */
      track->tracklen = get32lint(cts, seek+40);   /* time             */
      track->track_nr = get32lint(cts, seek+44);   /* track number     */
      track->tracks = get32lint(cts, seek+48);     /* nr of tracks     */
      track->year = get32lint(cts, seek+52);       /* year             */
      track->bitrate = get32lint(cts, seek+56);    /* bitrate          */
      val32 = get32lint (cts, seek+60);
      track->samplerate = val32 >> 16;             /* sample rate      */
      track->samplerate_low = val32 & 0xffff;      /* remaining bits   */
      track->volume = get32lint(cts, seek+64);     /* volume adjust    */
      track->starttime = get32lint (cts, seek+68);
      track->stoptime = get32lint (cts, seek+72);
      track->soundcheck = get32lint (cts, seek+76);/* soundcheck       */
      track->playcount = get32lint (cts, seek+80); /* playcount        */
      track->playcount2 = get32lint (cts, seek+84);
      track->time_played = get32lint(cts, seek+88);/* last time played */
      track->cd_nr = get32lint(cts, seek+92);      /* CD nr            */
      track->cds = get32lint(cts, seek+96);        /* CD nr of..       */
      /* Apple Store/Audible User ID (for DRM'ed files only, set to 0
	 otherwise). */
      track->drm_userid = get32lint (cts, seek+100);
      track->time_added = get32lint(cts, seek+104);/* last mod. time */
      track->bookmark_time = get32lint (cts, seek+108);/*time bookmarked*/
      track->dbid = get64lint (cts, seek+112);
      track->checked = get8int (cts, seek+120); /*Checked/Unchecked: 0/1*/
      /* The rating set by the application, as opposed to the rating
	 set on the iPod itself */
      track->app_rating = get8int (cts, seek+121);
      track->BPM = get16lint (cts, seek+122);
      track->artwork_count = get16lint (cts, seek+124);
      track->unk126 = get16lint (cts, seek+126);
      track->artwork_size = get32lint (cts, seek+128);
      track->unk132 = get32lint (cts, seek+132);
      track->samplerate2 = get32lfloat (cts, seek+136);
      track->time_released = get32lint (cts, seek+140);
      track->unk144 = get16lint (cts, seek+144);
      track->unk146 = get16lint (cts, seek+146);
      track->unk148 = get32lint (cts, seek+148);
      track->unk152 = get32lint (cts, seek+152);
  }
  if (header_len >= 0xf4)
  {
      track->skipcount = get32lint (cts, seek+156);
      track->last_skipped = get32lint (cts, seek+160);
      track->has_artwork = get8int (cts, seek+164);
      track->skip_when_shuffling = get8int (cts, seek+165);
      track->remember_playback_position = get8int (cts, seek+166);
      track->flag4 = get8int (cts, seek+167);
      track->dbid2 = get64lint (cts, seek+168);
      track->lyrics_flag = get8int (cts, seek+176);
      track->movie_flag = get8int (cts, seek+177);
      track->mark_unplayed = get8int (cts, seek+178);
      track->unk179 = get8int (cts, seek+179);
      track->unk180 = get32lint (cts, seek+180);
      track->pregap = get32lint (cts, seek+184);
      track->samplecount = get64lint (cts, seek+188);
      track->unk196 = get32lint (cts, seek+196);
      track->postgap = get32lint (cts, seek+200);
      track->unk204 = get32lint (cts, seek+204);
      track->mediatype = get32lint (cts, seek+208);
      track->season_nr = get32lint (cts, seek+212);
      track->episode_nr = get32lint (cts, seek+216);
      track->unk220 = get32lint (cts, seek+220);
      track->unk224 = get32lint (cts, seek+224);
      track->unk228 = get32lint (cts, seek+228);
      track->unk232 = get32lint (cts, seek+232);
      track->unk236 = get32lint (cts, seek+236);
      track->unk240 = get32lint (cts, seek+240);
  }
  if (header_len >= 0x148)
  {
      track->unk244 = get32lint (cts, seek+244);
      track->gapless_data = get32lint (cts, seek+248);
      track->unk252 = get32lint (cts, seek+252);
      track->gapless_track_flag = get16lint (cts, seek+256);
      track->gapless_album_flag = get16lint (cts, seek+258);
  }

  track->transferred = TRUE;                   /* track is on iPod! */

  seek += get32lint (cts, seek+4);             /* 1st mhod starts here! */
  CHECK_ERROR (fimp, -1);

  for (i=0; i<mhod_nums; ++i)
  {
      entry_utf8 = get_mhod_string (cts, seek, &zip, &type);
      CHECK_ERROR (fimp, -1);
      if (entry_utf8 != NULL)
      {
	  switch ((enum MHOD_ID)type)
	  {
	  case MHOD_ID_TITLE:
	      track->title = entry_utf8;
	      break;
	  case MHOD_ID_PATH:
	      track->ipod_path = entry_utf8;
	      break;
	  case MHOD_ID_ALBUM:
	      track->album = entry_utf8;
	      break;
	  case MHOD_ID_ARTIST:
	      track->artist = entry_utf8;
	      break;
	  case MHOD_ID_GENRE:
	      track->genre = entry_utf8;
	      break;
	  case MHOD_ID_FILETYPE:
	      track->filetype = entry_utf8;
	      break;
	  case MHOD_ID_COMMENT:
	      track->comment = entry_utf8;
	      break;
	  case MHOD_ID_CATEGORY:
	      track->category = entry_utf8;
	      break;
	  case MHOD_ID_COMPOSER:
	      track->composer = entry_utf8;
	      break;
	  case MHOD_ID_GROUPING:
	      track->grouping = entry_utf8;
	      break;
	  case MHOD_ID_DESCRIPTION:
	      track->description = entry_utf8;
	      break;
	  case MHOD_ID_PODCASTURL:
	      track->podcasturl = entry_utf8;
	      break;
	  case MHOD_ID_PODCASTRSS:
	      track->podcastrss = entry_utf8;
	      break;
	  case MHOD_ID_SUBTITLE:
	      track->subtitle = entry_utf8;
	      break;
	  case MHOD_ID_TVSHOW:
	      track->tvshow = entry_utf8;
	      break;
	  case MHOD_ID_TVEPISODE:
	      track->tvepisode = entry_utf8;
	      break;
	  case MHOD_ID_TVNETWORK:
	      track->tvnetwork = entry_utf8;
	      break;
	  case MHOD_ID_ALBUMARTIST:
	      track->albumartist = entry_utf8;
	      break;
	  case MHOD_ID_KEYWORDS:
	      track->keywords = entry_utf8;
	      break;
	  case MHOD_ID_SPLPREF:
	  case MHOD_ID_SPLRULES:
	  case MHOD_ID_LIBPLAYLISTINDEX:
	  case MHOD_ID_PLAYLIST:
	  case MHOD_ID_CHAPTERDATA:
	      g_free (entry_utf8);
	      break;
	  }
      }
      else
      {
	  MHODData mhod;
	  switch (type)
	  {
	  case MHOD_ID_CHAPTERDATA:
	      /* we just read the entire chapterdata info until we
		 have a better way to parse and represent it */
	      mhod = get_mhod (cts, seek, &zip);
	      if (mhod.valid && mhod.data.chapterdata_raw)
	      {
		  track->chapterdata_raw = mhod.data.chapterdata_raw;
		  track->chapterdata_raw_length =
		      zip - get32lint (cts, seek+4);
		  mhod.valid = FALSE;
	      }
	      break;
	  default:
/*
	  printf ("found mhod type %d at %lx inside mhit starting at %lx\n",
	  type, seek, mhit_seek);*/
	      break;
	  }
      }
      seek += zip;
  }

  playcount = playcount_get_next (fimp);
  if (playcount)
  {
      if (playcount->rating != NO_PLAYCOUNT)
      {
	  if (track->rating != playcount->rating)
	  {
	      /* backup original rating to app_rating */
	      track->app_rating = track->rating;
	      track->rating = playcount->rating;
	  }
      }
      if (playcount->time_played)
	  track->time_played = playcount->time_played;

      if (playcount->bookmark_time)
	  track->bookmark_time = playcount->bookmark_time;

      track->playcount += playcount->playcount;
      if (playcount->playcount != 0)
      {   /* unmark the 'unplayed' flag */
	  track->mark_unplayed = 0x01;
      }
      track->recent_playcount = playcount->playcount;

      track->skipcount += playcount->skipcount;
      track->recent_skipcount = playcount->skipcount;

      g_free (playcount);
  }
  itdb_track_add (fimp->itdb, track, -1);
  return seek;
}


/* Called by read_OTG_playlists(): OTG playlist stored in @cts by
 * adding a new playlist (named @plname) with the tracks specified in
 * @cts. If @plname is NULL, a standard name will be substituted */
/* Returns FALSE on error, TRUE on success. On error @fimp->error will
 * be set apropriately. */
static gboolean process_OTG_file (FImport *fimp, FContents *cts,
				  const gchar *plname)
{
    guint32 header_length, entry_length, entry_num;

    g_return_val_if_fail (fimp && cts, FALSE);
    g_return_val_if_fail (fimp->itdb, FALSE);

    if (!plname) plname = _("OTG Playlist");

    if (!check_header_seek (cts, "mhpo", 0))
    {
	if (cts->error)
	{
	    g_propagate_error (&fimp->error, cts->error);
	    return FALSE;
	}
	cts->reversed = TRUE;
	if (!check_header_seek (cts, "mhpo", 0))
	{
	    /* cts->error can't be set as already checked above */
	    /* set error */
	    g_return_val_if_fail (cts->filename, FALSE);
	    g_set_error (&fimp->error,
			 ITDB_FILE_ERROR,
			 ITDB_FILE_ERROR_CORRUPT,
			 _("Not a OTG playlist file: '%s' (missing mhpo header)."),
			 cts->filename);
	    return FALSE;
	}
    }
    header_length = get32lint (cts, 4);
    CHECK_ERROR (fimp, FALSE);
    /* all the headers I know are 0x14 long -- if this one is
       longer we can simply ignore the additional information */
    if (header_length < 0x14)
    {
	g_set_error (&fimp->error,
		     ITDB_FILE_ERROR,
		     ITDB_FILE_ERROR_CORRUPT,
		     _("OTG playlist file ('%s'): header length smaller than expected (%d<20)."),
		     cts->filename, header_length);
	return FALSE;
    }
    entry_length = get32lint (cts, 8);
    CHECK_ERROR (fimp, FALSE);
    /* all the entries I know are 0x04 long */
    if (entry_length < 0x04)
    {
	g_set_error (&fimp->error,
		     ITDB_FILE_ERROR,
		     ITDB_FILE_ERROR_CORRUPT,
		     _("OTG playlist file file ('%s'): entry length smaller than expected (%d<4)."),
		     cts->filename, entry_length);
	return FALSE;
    }
    /* number of entries */
    entry_num = get32lint (cts, 12);
    CHECK_ERROR (fimp, FALSE);

    if (entry_num > 0)
    {
	gint i;
	Itdb_Playlist *pl;

	pl = itdb_playlist_new (plname, FALSE);
	/* Add new playlist */
	itdb_playlist_add (fimp->itdb, pl, -1);

	/* Add items */
	for (i=0; i<entry_num; ++i)
	{
	    Itdb_Track *track;
	    guint32 num = get32lint (cts,
				     header_length + entry_length *i);
	    CHECK_ERROR (fimp, FALSE);

	    track = g_list_nth_data (fimp->itdb->tracks, num);
	    if (track)
	    {
		itdb_playlist_add_track (pl, track, -1);
	    }
	    else
	    {
		g_set_error (&fimp->error,
			     ITDB_FILE_ERROR,
			     ITDB_FILE_ERROR_CORRUPT,
			     _("OTG playlist file '%s': reference to non-existent track (%d)."),
			     cts->filename, num);
		return FALSE;
	    }
	}
    }
    return TRUE;
}




/* Add the On-The-Go Playlist(s) to the database */
/* The OTG-Files are located in the directory given by
   fimp->itdb->itdb_filename.
   On error FALSE is returned and fimp->error is set accordingly. */
static gboolean read_OTG_playlists (FImport *fimp)
{
    gchar *db[] = {"OTGPlaylistInfo", NULL};
    gchar *dirname, *otgname;

    g_return_val_if_fail (fimp, FALSE);
    g_return_val_if_fail (fimp->itdb, FALSE);
    g_return_val_if_fail (fimp->itdb->filename, FALSE);

    dirname = g_path_get_dirname (fimp->itdb->filename);

    otgname = itdb_resolve_path (dirname, (const gchar **)db);


    /* only parse if "OTGPlaylistInfo" exists */
    if (otgname)
    {
	gchar *filename;
	gint i=1;
	do
	{
	    db[0] = g_strdup_printf ("OTGPlaylistInfo_%d", i);
	    filename = itdb_resolve_path (dirname, (const gchar **)db);
	    g_free (db[0]);
	    if (filename)
	    {
		FContents *cts = fcontents_read (filename, &fimp->error);
		if (cts)
		{
		    gchar *plname = g_strdup_printf (_("OTG Playlist %d"), i);
		    process_OTG_file (fimp, cts, plname);
		    g_free (plname);
		    fcontents_free (cts);
		}
		g_free (filename);
	    }
	    if (fimp->error) break;
	    ++i;
	} while (filename);
	g_free (otgname);
    }
    g_free (dirname);
    return TRUE;
}


/* Read the tracklist (mhlt). mhsd_seek must point to type 1 mhsd
   (this is treated as a programming error) */
/* Return value:
   TRUE: import successful
   FALSE: error occured, fimp->error is set */
static gboolean parse_tracks (FImport *fimp, glong mhsd_seek)
{
    FContents *cts;
    glong mhlt_seek, seek;
    guint32 nr_tracks, i;

    g_return_val_if_fail (fimp, FALSE);
    g_return_val_if_fail (fimp->itdb, FALSE);
    g_return_val_if_fail (fimp->fcontents, FALSE);
    g_return_val_if_fail (fimp->fcontents->filename, FALSE);
    g_return_val_if_fail (mhsd_seek >= 0, FALSE);

    cts = fimp->fcontents;

    g_return_val_if_fail (check_header_seek (cts, "mhsd", mhsd_seek),
			  FALSE);

   /* The mhlt header should be the next after the mhsd header. In
      order to allow slight changes in the format, we skip headers
      until we find an mhlt inside the given mhsd */

    mhlt_seek = find_next_a_in_b (cts, "mhlt", mhsd_seek, mhsd_seek);
    CHECK_ERROR (fimp, FALSE);

    if (mhlt_seek == -1)
    {
	set_error_a_not_found_in_b (&fimp->error,
				    "mhlt", "mhsd", mhsd_seek);
	return FALSE;
    }

    /* Now we are at the mhlt */
    nr_tracks = get32lint (cts, mhlt_seek+8);
    CHECK_ERROR (fimp, FALSE);

    seek = find_next_a_in_b (cts, "mhit", mhsd_seek, mhlt_seek);
    CHECK_ERROR (fimp, FALSE);
    /* seek should now point to the first mhit */
    for (i=0; i<nr_tracks; ++i)
    {
	/* seek could be -1 if first mhit could not be found */
	if (seek != -1)
	    seek = get_mhit (fimp, seek);
	if (fimp->error) return FALSE;
	if (seek == -1)
	{   /* this should not be -- issue warning */
	    g_warning (_("iTunesDB corrupt: number of tracks (mhit hunks) inconsistent. Trying to continue.\n"));
	    break;
	}
    }
    return TRUE;
}



/* Read the playlists (mhlp). mhsd_seek must point to type 2 or type 3
   mhsd (this is treated as a programming error) */
/* Return value:
   TRUE: import successful
   FALSE: error occured, fimp->error is set */
static gboolean parse_playlists (FImport *fimp, glong mhsd_seek)
{
    FContents *cts;
    glong seek, mhlp_seek;
    guint32 nr_playlists, i;

    g_return_val_if_fail (fimp, FALSE);
    g_return_val_if_fail (fimp->itdb, FALSE);
    g_return_val_if_fail (fimp->fcontents, FALSE);
    g_return_val_if_fail (fimp->fcontents->filename, FALSE);
    g_return_val_if_fail (mhsd_seek >= 0, FALSE);

    cts = fimp->fcontents;

    g_return_val_if_fail (check_header_seek (cts, "mhsd", mhsd_seek),
			  FALSE);

    /* The mhlp header should be the next after the mhsd header. In
       order to allow slight changes in the format, we skip headers
       until we find an mhlp inside the given mhsd */

    mhlp_seek = find_next_a_in_b (cts, "mhlp", mhsd_seek, mhsd_seek);
    CHECK_ERROR (fimp, FALSE);

    if (mhlp_seek == -1)
    {
	set_error_a_not_found_in_b (&fimp->error,
				    "mhlp", "mhsd", mhsd_seek);
	return FALSE;
    }
    /* Now we are at the mhlp */

    nr_playlists = get32lint (cts, mhlp_seek+8);
    CHECK_ERROR (fimp, FALSE);

    /* Create track-id tree for quicker track lookup */
    fimp->idtree = itdb_track_id_tree_create (fimp->itdb);

    seek = find_next_a_in_b (cts, "mhyp", mhsd_seek, mhlp_seek);
    CHECK_ERROR (fimp, FALSE);
    /* seek should now point to the first mhit */
    for (i=0; i<nr_playlists; ++i)
    {
	/* seek could be -1 if first mhyp could not be found */
	if (seek != -1)
	    seek = get_playlist (fimp, seek);
	if (fimp->error) return FALSE;
	if (seek == -1)
	{   /* this should not be -- issue warning */
	    g_warning (_("iTunesDB possibly corrupt: number of playlists (mhyp hunks) inconsistent. Trying to continue.\n"));
	    break;
	}
    }

    itdb_track_id_tree_destroy (fimp->idtree);
    fimp->idtree = NULL;

    return TRUE;
}

static gboolean parse_fimp (FImport *fimp)
{
    glong seek=0;
    FContents *cts;
    glong mhsd_1, mhsd_2, mhsd_3;

    g_return_val_if_fail (fimp, FALSE);
    g_return_val_if_fail (fimp->itdb, FALSE);
    g_return_val_if_fail (fimp->fcontents, FALSE);
    g_return_val_if_fail (fimp->fcontents->filename, FALSE);

    cts = fimp->fcontents;

    /* get the positions of the various mhsd */
    /* type 1: track list */
    mhsd_1 = find_mhsd (cts, 1);
    CHECK_ERROR (fimp, FALSE);
    /* type 2: standard playlist section -- Podcasts playlist will be
       just an ordinary playlist */
    mhsd_2 = find_mhsd (cts, 2);
    CHECK_ERROR (fimp, FALSE);
    /* type 3: playlist section with special version of Podcasts
       playlist (optional) */
    mhsd_3 = find_mhsd (cts, 3);
    CHECK_ERROR (fimp, FALSE);

    fimp->itdb->version = get32lint (cts, seek+16);
    CHECK_ERROR (fimp, FALSE);
    fimp->itdb->id = get64lint (cts, seek+24);
    CHECK_ERROR (fimp, FALSE);

    if (mhsd_1 == -1)
    {   /* Very bad: no type 1 mhsd which should hold the tracklist */
	g_set_error (&fimp->error,
		     ITDB_FILE_ERROR,
		     ITDB_FILE_ERROR_CORRUPT,
		     _("iTunesDB '%s' corrupt: Could not find tracklist (no mhsd type 1 section found)"),
		     cts->filename);
	return FALSE;
    }

    /* copy the 'reversed endian flag' */
    if (cts->reversed) {
      fimp->itdb->device->byte_order = G_BIG_ENDIAN;
    } else {
      fimp->itdb->device->byte_order = G_LITTLE_ENDIAN;
    }
#if 0
    fimp->itdb->device->endianess_set = TRUE;
    fimp->itdb->device->endianess_reversed = cts->reversed;
#endif

    parse_tracks (fimp, mhsd_1);
    if (fimp->error) return FALSE;

    if (mhsd_3 != -1)
	parse_playlists (fimp, mhsd_3);
    else if (mhsd_2 != -1)
	parse_playlists (fimp, mhsd_2);
    else
    {  /* Very bad: no type 2 or type 3 mhsd which should hold the
	  playlists */
	g_set_error (&fimp->error,
		     ITDB_FILE_ERROR,
		     ITDB_FILE_ERROR_CORRUPT,
		     _("iTunesDB '%s' corrupt: Could not find playlists (no mhsd type 2 or type 3 sections found)"),
		     cts->filename);
	return FALSE;
    }

    return TRUE;
}



/* Set @error with standard error message */
static void error_no_itunes_dir (const gchar *mp, GError **error)
{
    gchar *str;

    g_return_if_fail (mp);
    g_return_if_fail (error);

    str = g_build_filename (mp, "iPod_Control", "iTunes", NULL);
    g_set_error (error,
		 ITDB_FILE_ERROR,
		 ITDB_FILE_ERROR_NOTFOUND,
		 _("iTunes directory not found: '%s' (or similar)."),
		 str);
    g_free (str);
}

/* Set @error with standard error message */
static void error_no_music_dir (const gchar *mp, GError **error)
{
    gchar *str;

    g_return_if_fail (mp);
    g_return_if_fail (error);

    str = g_build_filename (mp, "iPod_Control", "Music", NULL);
    g_set_error (error,
		 ITDB_FILE_ERROR,
		 ITDB_FILE_ERROR_NOTFOUND,
		 _("Music directory not found: '%s' (or similar)."),
		 str);
    g_free (str);
}

#if 0
/* Set @error with standard error message */
static void error_no_control_dir (const gchar *mp, GError **error)
{
    gchar *str;

    g_return_if_fail (mp);
    g_return_if_fail (error);

    str = g_build_filename (mp, "iPod_Control", NULL);
    g_set_error (error,
		 ITDB_FILE_ERROR,
		 ITDB_FILE_ERROR_NOTFOUND,
		 _("Control directory not found: '%s' (or similar)."),
		 str);
    g_free (str);
}
#endif

/**
 * itdb_parse:
 * @mp: mount point of the iPod (eg "/mnt/ipod) in local encoding
 * @error: return location for a #GError or NULL
 *
 * Parse the Itdb_iTunesDB of the iPod located at @mp
 *
 * Return value: a newly allocated #Itdb_iTunesDB struct holding the tracks and
 * the playlists present on the iPod at @mp, NULL if @mp isn't an iPod mount
 * point. If non-NULL, the #Itdb_iTunesDB is to be freed with itdb_free() when
 * it's no longer needed
 **/
Itdb_iTunesDB *itdb_parse (const gchar *mp, GError **error)
{
    gchar *filename;
    gchar *itunes_dir;
    Itdb_iTunesDB *itdb = NULL;
    const gchar *db[] = {"iTunesDB", NULL};

    itunes_dir = itdb_get_itunes_dir (mp);

    if (!itunes_dir)
    {
	error_no_itunes_dir (mp, error);
	return NULL;
    }

    filename = itdb_resolve_path (itunes_dir, db);
    if (filename)
    {
	itdb = itdb_parse_file (filename, error);
	if (itdb)
	{
	    itdb_set_mountpoint (itdb, mp);

	    /* We don't test the return value of ipod_parse_artwork_db
	     * since the database content will be consistent even if
	     * we fail to get the various thumbnails, we ignore the
	     * error since older ipods don't have thumbnails.

	     * FIXME: this probably should go into itdb_parse_file,
	     * but I don't understand its purpose, and
	     * ipod_parse_artwork_db needs the mountpoint field from
	     * the itdb, which may not be available in the other
	     * function

	     * JCS: itdb_parse_file is used to read local repositories
	     * (usually repositories stored in
	     * ~/.gtkpod). ipod_parse_artwork_db (and the
	     * corresponding artbook write function) should probably
	     * be expanded to look for (write) the required files into
	     * the same directory as itdb->filename in case
	     * itdb->mountpoint does not exist. Because several local
	     * repositories may exist in the same directory, the names
	     * should be modified by the repository name.
	     */
	    ipod_parse_artwork_db (itdb);

	}
	g_free (filename);
    }
    else
    {
	gchar *str = g_build_filename (mp, db[0], db[1], db[2], db[3], NULL);
	g_set_error (error,
		     ITDB_FILE_ERROR,
		     ITDB_FILE_ERROR_NOTFOUND,
		     _("File not found: '%s'."),
		     str);
	g_free (str);
    }
    g_free (itunes_dir);
    return itdb;
}

/**
 * itdb_parse_file:
 * @filename: path to a file in iTunesDB format
 * @error: return location for a #GError or NULL
 *
 *  Same as itunesdb_parse(), but filename is specified directly.
 *
 * Return value: a newly allocated #Itdb_iTunesDB struct holding the tracks and
 * the playlists present on the iPod at @mp, NULL if @mp isn't an iPod mount
 * point. If non-NULL, the #Itdb_iTunesDB is to be freed with itdb_free() when
 * it's no longer needed
 **/
Itdb_iTunesDB *itdb_parse_file (const gchar *filename, GError **error)
{
    FImport *fimp;
    Itdb_iTunesDB *itdb;
    gboolean success = FALSE;

    g_return_val_if_fail (filename, NULL);

    fimp = g_new0 (FImport, 1);
    itdb = itdb_new ();
    itdb->filename = g_strdup (filename);
    fimp->itdb = itdb;

    fimp->fcontents = fcontents_read (filename, error);

    if (fimp->fcontents)
    {
	if (playcounts_init (fimp))
	{
	    if (parse_fimp (fimp))
	    {
		if (read_OTG_playlists (fimp))
		{
		    success = TRUE;
		}
	    }
	}
    }

    if (!success)
    {
	itdb_free (itdb);
	itdb = NULL;
	if (fimp->error)
	    g_propagate_error (error, fimp->error);
    }
    itdb_free_fimp (fimp);

    return itdb;
}


/* up to here we had the functions for reading the iTunesDB               */
/* ---------------------------------------------------------------------- */
/* from here on we have the functions for writing the iTunesDB            */

/* will expand @cts when necessary in order to accomodate @len bytes
   starting at @seek */
static void wcontents_maybe_expand (WContents *cts, gulong len,
				    gulong seek)
{
    g_return_if_fail (cts);

    while (cts->pos+len > cts->total)
    {
	cts->total += WCONTENTS_STEPSIZE;
	cts->contents = g_realloc (cts->contents, cts->total);
    }
}


/* Write @data, @n bytes long to position @seek. Will always be
 * successful because glib terminates when out of memory */
static void put_data_seek (WContents *cts, gchar *data,
			   gulong len, gulong seek)
{
    g_return_if_fail (cts);

    if (len != 0)
    {
	g_return_if_fail (data);
	wcontents_maybe_expand (cts, len, seek);

	memcpy (&cts->contents[seek], data, len);
	/* adjust end position if necessary */
	if (seek+len > cts->pos)
	    cts->pos = seek+len;
    }
}



/* Write @data, @n bytes long to end of @cts. Will always be
 * successful because glib terminates when out of memory */
static void put_data (WContents *cts, gchar *data, gulong len)
{
    g_return_if_fail (cts);

    put_data_seek (cts, data, len, cts->pos);
}


/* Write @string without trailing Null to end of @cts. Will always be
 * successful because glib terminates when out of memory */
static void put_string (WContents *cts, gchar *string)
{
    g_return_if_fail (cts);
    g_return_if_fail (string);

    put_data (cts, string, strlen(string));
}

/* Write 4-byte long @header identifcation taking into account
 * possible reversed endianess */
static void put_header (WContents *cts, gchar *header)
{
    gchar rdata[4];
    gint i, offset, sign;


    g_return_if_fail (cts);
    g_return_if_fail (header);
    g_return_if_fail (strlen (header) == 4);

    /* reverse data for write if necessary */
    if (cts->reversed)
    {
	offset = 3;
	sign = -1;
    }
    else
    {
	offset = 0;
	sign = 1;
    }
    for (i=0; i<4; ++i)
    {
	    rdata[i] = header[offset + sign*i];
    }

    put_data (cts, rdata, 4);
}



/* ------------------------------------------------------------
   Little Endian
   ------------------------------------------------------------ */

/* Write 2-byte integer @n to @cts in little endian order. */
static void raw_put16lint (WContents *cts, guint16 n)
{
    n = GUINT16_TO_LE (n);
    put_data (cts, (gchar *)&n, 2);
}


/* Write 3-byte integer @n to @cts in big endian order. */
static void raw_put24lint (WContents *cts, guint32 n)
{
    gchar buf[3] ;
    buf[2] = (n >> 16) & 0xff ;
    buf[1] = (n >> 8)  & 0xff ;
    buf[0] = (n >> 0)  & 0xff ;
    put_data (cts, buf, 3);
}


/* Write 4-byte integer @n to @cts in little endian order. */
static void raw_put32lint (WContents *cts, guint32 n)
{
    n = GUINT32_TO_LE (n);
    put_data (cts, (gchar *)&n, 4);
}

/* Write 4 byte floating number */
static void raw_put32lfloat (WContents *cts, float f)
{
    union
    {
	guint32 i;
	float   f;
    } flt;

    if (sizeof (float) != 4)
    {
	raw_put32lint (cts, 0);
	g_return_if_reached ();
    }

    flt.f = f;
    raw_put32lint (cts, flt.i);
}


/* Write 4-byte integer @n to @cts at position @seek in little
   endian order. */
static void raw_put32lint_seek (WContents *cts, guint32 n, gulong seek)
{
    n = GUINT32_TO_LE (n);
    put_data_seek (cts, (gchar *)&n, 4, seek);
}


/* Write 8-byte integer @n to @cts in big little order. */
static void raw_put64lint (WContents *cts, guint64 n)
{
    n = GUINT64_TO_LE (n);
    put_data (cts, (gchar *)&n, 8);
}


/* ------------------------------------------------------------
   Big Endian
   ------------------------------------------------------------ */

/* Write 2-byte integer @n to @cts in big endian order. */
static void raw_put16bint (WContents *cts, guint16 n)
{
    n = GUINT16_TO_BE (n);
    put_data (cts, (gchar *)&n, 2);
}


/* Write 3-byte integer @n to @cts in big endian order. */
static void raw_put24bint (WContents *cts, guint32 n)
{
    gchar buf[3] ;
    buf[0] = (n >> 16) & 0xff ;
    buf[1] = (n >> 8)  & 0xff ;
    buf[2] = (n >> 0)  & 0xff ;
    put_data (cts, buf, 3);
}

/* Write 4-byte integer @n to @cts in big endian order. */
static void raw_put32bint (WContents *cts, guint32 n)
{
    n = GUINT32_TO_BE (n);
    put_data (cts, (gchar *)&n, 4);
}


/* Write 4-byte integer @n to @cts at position @seek in big
   endian order. */
static void raw_put32bint_seek (WContents *cts, guint32 n, gulong seek)
{
    n = GUINT32_TO_BE (n);
    put_data_seek (cts, (gchar *)&n, 4, seek);
}


/* Write 4 byte floating number */
static void raw_put32bfloat (WContents *cts, float f)
{
    union
    {
	guint32 i;
	float   f;
    } flt;

    if (sizeof (float) != 4)
    {
	raw_put32bint (cts, 0);
	g_return_if_reached ();
    }

    flt.f = f;
    raw_put32bint (cts, flt.i);
}


/* Write 8-byte integer @n to cts in big endian order. */
static void raw_put64bint (WContents *cts, guint64 n)
{
    n = GUINT64_TO_BE (n);
    put_data (cts, (gchar *)&n, 8);
}


/* ------------------------------------------------------------
   Not Endian Dependent
   ------------------------------------------------------------ */

/* Write 1-byte integer @n to @cts */
static void put8int (WContents *cts, guint8 n)
{
    put_data (cts, (gchar *)&n, 1);
}


/* Append @n times 2-byte-long zeros */
static void put16_n0 (WContents *cts, gulong n)
{
    g_return_if_fail (cts);

    if (n>0)
    {
	wcontents_maybe_expand (cts, 2*n, cts->pos);
	memset (&cts->contents[cts->pos], 0, 2*n);
	cts->pos += 2*n;
    }
}

/* Append @n times 4-byte-long zeros */
static void put32_n0 (WContents *cts, gulong n)
{
    g_return_if_fail (cts);

    if (n>0)
    {
	wcontents_maybe_expand (cts, 4*n, cts->pos);
	memset (&cts->contents[cts->pos], 0, 4*n);
	cts->pos += 4*n;
    }
}


/* ------------------------------------------------------------
   Reversed Endian Sensitive (little endian)
   ------------------------------------------------------------ */

static void put16lint (WContents *cts, guint16 n)
{
    if (!cts->reversed)
	raw_put16lint (cts, n);
    else
	raw_put16bint (cts, n);
}
#if 0
static void put24lint (WContents *cts, guint32 n)
{
    if (!cts->reversed)
	raw_put24lint (cts, n);
    else
	raw_put24bint (cts, n);
}
#endif
static void put32lint (WContents *cts, guint32 n)
{
    if (!cts->reversed)
	raw_put32lint (cts, n);
    else
	raw_put32bint (cts, n);
}

static void put32lfloat (WContents *cts, float f)
{
    if (!cts->reversed)
	raw_put32lfloat (cts, f);
    else
	raw_put32bfloat (cts, f);
}

static void put32lint_seek (WContents *cts, guint32 n, gulong seek)
{
    if (!cts->reversed)
	raw_put32lint_seek (cts, n, seek);
    else
	raw_put32bint_seek (cts, n, seek);
}


static void put64lint (WContents *cts, guint64 n)
{
    if (!cts->reversed)
	raw_put64lint (cts, n);
    else
	raw_put64bint (cts, n);
}

/* ------------------------------------------------------------
   Reversed Endian Sensitive (big endian)
   ------------------------------------------------------------ */
#if 0
static void put16bint (WContents *cts, guint16 n)
{
    if (cts->reversed)
	raw_put16lint (cts, n);
    else
	raw_put16bint (cts, n);
}
#endif
static void put24bint (WContents *cts, guint32 n)
{
    if (cts->reversed)
	raw_put24lint (cts, n);
    else
	raw_put24bint (cts, n);
}

static void put32bint (WContents *cts, guint32 n)
{
    if (cts->reversed)
	raw_put32lint (cts, n);
    else
	raw_put32bint (cts, n);
}
#if 0
static void put32bfloat (WContents *cts, float f)
{
    if (cts->reversed)
	raw_put32lfloat (cts, f);
    else
	raw_put32bfloat (cts, f);
}

static void put32bint_seek (WContents *cts, guint32 n, gulong seek)
{
    if (cts->reversed)
	raw_put32lint_seek (cts, n, seek);
    else
	raw_put32bint_seek (cts, n, seek);
}
#endif
static void put64bint (WContents *cts, guint64 n)
{
    if (cts->reversed)
	raw_put64lint (cts, n);
    else
	raw_put64bint (cts, n);
}



/* Write out the mhbd header. Size will be written later */
static void mk_mhbd (FExport *fexp, guint32 children)
{
  WContents *cts;

  g_return_if_fail (fexp);
  g_return_if_fail (fexp->itdb);
  g_return_if_fail (fexp->wcontents);

  cts = fexp->wcontents;

  put_header (cts, "mhbd");
  put32lint (cts, 104); /* header size */
  put32lint (cts, -1);  /* size of whole mhdb -- fill in later */
  put32lint (cts, 1);   /* ? */
  /* Version number: 0x01: iTunes 2
                     0x02: iTunes 3
		     0x09: iTunes 4.2
		     0x0a: iTunes 4.5
		     0x0b: iTunes 4.7
		     0x0c: iTunes 4.71/4.8 (required for shuffle)
                     0x0d: iTunes 4.9
                     0x0e: iTunes 5
		     0x0f: iTunes 6
		     0x10: iTunes 6.0.1(?)
		     0x11: iTunes 6.0.2
                     0x12 = iTunes 6.0.5.
		     0x13 = iTunes 7 */
  fexp->itdb->version = 0x13;
  put32lint (cts, fexp->itdb->version);
  put32lint (cts, children);
  put64lint (cts, fexp->itdb->id);
  put32lint (cts, 2);   /* ? */
  put32_n0 (cts, 17);   /* dummy space */
}

/* Fill in the length of a standard header */
static void fix_header (WContents *cts, gulong header_seek)
{
  put32lint_seek (cts, cts->pos-header_seek, header_seek+8);
}


/* Write out the mhsd header. Size will be written later */
static void mk_mhsd (FExport *fexp, guint32 type)
{
  WContents *cts;

  g_return_if_fail (fexp);
  g_return_if_fail (fexp->itdb);
  g_return_if_fail (fexp->wcontents);

  cts = fexp->wcontents;

  put_header (cts, "mhsd");
  put32lint (cts, 96);   /* Headersize */
  put32lint (cts, -1);   /* size of whole mhsd -- fill in later */
  put32lint (cts, type); /* type: 1 = track, 2 = playlist */
  put32_n0 (cts, 20);      /* dummy space */
}


/* Write out the mhlt header. */
static void mk_mhlt (FExport *fexp, guint32 num)
{
  WContents *cts;

  g_return_if_fail (fexp);
  g_return_if_fail (fexp->itdb);
  g_return_if_fail (fexp->wcontents);

  cts = fexp->wcontents;

  put_header (cts, "mhlt");
  put32lint (cts, 92);       /* Headersize */
  put32lint (cts, num);      /* tracks in this itunesdb */
  put32_n0 (cts, 20);        /* dummy space */
}


/* Write out the mhit header. Size will be written later */
static void mk_mhit (WContents *cts, Itdb_Track *track)
{
  g_return_if_fail (cts);
  g_return_if_fail (track);

  put_header (cts, "mhit");
  put32lint (cts, 0x148);/* header size */
  put32lint (cts, -1);   /* size of whole mhit -- fill in later */
  put32lint (cts, -1);   /* nr of mhods in this mhit -- later   */
  put32lint (cts, track->id); /* track index number */

  put32lint (cts, track->visible);
  put32lint (cts, track->filetype_marker);
  put8int (cts, track->type1);
  put8int (cts, track->type2);
  put8int   (cts, track->compilation);
  put8int   (cts, track->rating);
  put32lint (cts, track->time_modified); /* timestamp               */
  put32lint (cts, track->size);    /* filesize                   */
  put32lint (cts, track->tracklen);/* length of track in ms      */
  put32lint (cts, track->track_nr);/* track number               */
  put32lint (cts, track->tracks);  /* number of tracks           */
  put32lint (cts, track->year);    /* the year                   */
  put32lint (cts, track->bitrate); /* bitrate                    */
  put32lint (cts, (((guint32)track->samplerate)<<16) |
	     ((guint32)track->samplerate_low));
  put32lint (cts, track->volume);  /* volume adjust              */
  put32lint (cts, track->starttime);
  put32lint (cts, track->stoptime);
  put32lint (cts, track->soundcheck);
  put32lint (cts, track->playcount);/* playcount                 */
  track->playcount2 = track->playcount;
  put32lint (cts, track->playcount2);
  put32lint (cts, track->time_played); /* last time played       */
  put32lint (cts, track->cd_nr);   /* CD number                  */
  put32lint (cts, track->cds);     /* number of CDs              */
  put32lint (cts, track->drm_userid);
  put32lint (cts, track->time_added); /* timestamp            */
  put32lint (cts, track->bookmark_time);
  put64lint (cts, track->dbid);
  if (track->checked)   put8int (cts, 1);
  else                  put8int (cts, 0);
  put8int (cts, track->app_rating);
  put16lint (cts, track->BPM);
  put16lint (cts, track->artwork_count);
  put16lint (cts, track->unk126);
  put32lint (cts, track->artwork_size);
  put32lint (cts, track->unk132);
  put32lfloat (cts, track->samplerate2);
  put32lint (cts, track->time_released);
  put16lint (cts, track->unk144);
  put16lint (cts, track->unk146);
  put32lint (cts, track->unk148);
  put32lint (cts, track->unk152);
  /* since iTunesDB version 0x0c */
  put32lint (cts, track->skipcount);
  put32lint (cts, track->last_skipped);
  put8int (cts, track->has_artwork);
  put8int (cts, track->skip_when_shuffling);
  put8int (cts, track->remember_playback_position);
  put8int (cts, track->flag4);
  put64lint (cts, track->dbid2);
  put8int (cts, track->lyrics_flag);
  put8int (cts, track->movie_flag);
  put8int (cts, track->mark_unplayed);
  put8int (cts, track->unk179);
  put32lint (cts, track->unk180);
  put32lint (cts, track->pregap);
  put64lint (cts, track->samplecount);
  put32lint (cts, track->unk196);
  put32lint (cts, track->postgap);
  put32lint (cts, track->unk204);
  put32lint (cts, track->mediatype);
  put32lint (cts, track->season_nr);
  put32lint (cts, track->episode_nr);
  put32lint (cts, track->unk220);
  put32lint (cts, track->unk224);
  put32lint (cts, track->unk228);
  put32lint (cts, track->unk232);
  put32lint (cts, track->unk236);
  put32lint (cts, track->unk240);
  put32lint (cts, track->unk244);
  put32lint (cts, track->gapless_data);
  put32lint (cts, track->unk252);
  put16lint (cts, track->gapless_track_flag);
  put16lint (cts, track->gapless_album_flag);
  put32_n0 (cts, 17); /* padding */
}


/* Fill in the missing items of the mhit header:
   total size and number of mhods */
static void fix_mhit (WContents *cts, gulong mhit_seek, guint32 mhod_num)
{
  g_return_if_fail (cts);

  /* size of whole mhit */
  put32lint_seek (cts, cts->pos-mhit_seek, mhit_seek+8);
  /* nr of mhods */
  put32lint_seek (cts, mhod_num, mhit_seek+12);
}

static gint mhod52_sort_title (const struct mhod52track *a, const struct mhod52track *b)
{
    return strcmp (a->title, b->title);
}


static gint mhod52_sort_album (const struct mhod52track *a, const struct mhod52track *b)
{
    gint result;

    result = strcmp (a->album, b->album);
    if (result == 0)
	result = a->cd_nr - b->cd_nr;
    if (result == 0)
	result = a->track_nr - b->track_nr;
    if (result == 0)
	result = strcmp (a->title, b->title);
    return result;
}

static gint mhod52_sort_artist (struct mhod52track *a, struct mhod52track *b)
{
    gint result;

    result = strcmp (a->artist, b->artist);
    if (result == 0)
	result = strcmp (a->album, b->album);
    if (result == 0)
	result = a->cd_nr - b->cd_nr;
    if (result == 0)
	result = a->track_nr - b->track_nr;
    if (result == 0)
	result = strcmp (a->title, b->title);
    return result;
}

static gint mhod52_sort_genre (struct mhod52track *a, struct mhod52track *b)
{
    gint result;

    result = strcmp (a->genre, b->genre);
    if (result == 0)
	result = strcmp (a->artist, b->artist);
    if (result == 0)
	result = strcmp (a->album, b->album);
    if (result == 0)
	result = a->cd_nr - b->cd_nr;
    if (result == 0)
	result = a->track_nr - b->track_nr;
    if (result == 0)
	result = strcmp (a->title, b->title);
    return result;
}

static gint mhod52_sort_composer (struct mhod52track *a, struct mhod52track *b)
{
    gint result;

    result = strcmp (a->composer, b->composer);
    if (result == 0)
	result = strcmp (a->title, b->title);
    return result;
}


/* Create a new list containing all tracks but with collate keys
   instead of the actual strings (title, artist, album, genre,
   composer) */
static GList *mhod52_make_collate_keys (GList *tracks)
{
    gint numtracks = g_list_length (tracks);
    GList *gl, *coltracks = NULL;
    gint index=0;

    for (gl=tracks; gl; gl=gl->next)
    {
	struct mhod52track *ct;
	Itdb_Track *tr = gl->data;
	g_return_val_if_fail (tr, NULL);

	ct = g_new0 (struct mhod52track, 1);
	coltracks = g_list_prepend (coltracks, ct);

	if (tr->album)
	    ct->album = g_utf8_collate_key (tr->album, -1);
	else
	    ct->album = g_strdup ("");
	if (tr->title)
	    ct->title = g_utf8_collate_key (tr->title, -1);
	else
	    ct->title = g_strdup ("");
	if (tr->artist)
	    ct->artist = g_utf8_collate_key (tr->artist, -1);
	else
	    ct->artist = g_strdup ("");
	if (tr->genre)
	    ct->genre = g_utf8_collate_key (tr->genre, -1);
	else
	    ct->genre = g_strdup ("");
	if (tr->composer)
	    ct->composer = g_utf8_collate_key (tr->composer, -1);
	else
	    ct->composer = g_strdup ("");
	ct->track_nr = tr->track_nr;
	ct->cd_nr = tr->cd_nr;
	ct->numtracks = numtracks;
	ct->index = index++;
    }
    return coltracks;
}


/* Free all memory used up by the collate keys */
static void mhod52_free_collate_keys (GList *coltracks)
{
    GList *gl;

    for (gl=coltracks; gl; gl=gl->next)
    {
	struct mhod52track *ct = gl->data;
	g_return_if_fail (ct);
	g_free (ct->album);
	g_free (ct->title);
	g_free (ct->artist);
	g_free (ct->genre);
	g_free (ct->composer);
	g_free (ct);
    }
    g_list_free (coltracks);
}


/* Write out one mhod header.
     type: see enum of MHMOD_IDs;
     data: utf8 string for text items
           position indicator for MHOD_ID_PLAYLIST
           SPLPref for MHOD_ID_SPLPREF
	   SPLRules for MHOD_ID_SPLRULES */
static void mk_mhod (WContents *cts, MHODData *mhod)
{
  g_return_if_fail (cts);
  g_return_if_fail (mhod->valid);

  switch ((enum MHOD_ID)mhod->type)
  {
  case MHOD_ID_TITLE:
  case MHOD_ID_PATH:
  case MHOD_ID_ALBUM:
  case MHOD_ID_ARTIST:
  case MHOD_ID_GENRE:
  case MHOD_ID_FILETYPE:
  case MHOD_ID_COMMENT:
  case MHOD_ID_CATEGORY:
  case MHOD_ID_COMPOSER:
  case MHOD_ID_GROUPING:
  case MHOD_ID_DESCRIPTION:
  case MHOD_ID_SUBTITLE:
  case MHOD_ID_TVSHOW:
  case MHOD_ID_TVEPISODE:
  case MHOD_ID_TVNETWORK:
  case MHOD_ID_ALBUMARTIST:
  case MHOD_ID_KEYWORDS:
      g_return_if_fail (mhod->data.string);
      /* normal iTunesDBs seem to take utf16 strings), endian-inversed
	 iTunesDBs seem to take utf8 strings */
      if (!cts->reversed)
      {
	  /* convert to utf16  */
	  gunichar2 *entry_utf16 = g_utf8_to_utf16 (mhod->data.string, -1,
						    NULL, NULL, NULL);
	  guint32 len = utf16_strlen (entry_utf16);
	  fixup_little_utf16 (entry_utf16);
	  put_header (cts, "mhod");   /* header                     */
	  put32lint (cts, 24);        /* size of header             */
	  put32lint (cts, 2*len+40);  /* size of header + body      */
	  put32lint (cts, mhod->type);/* type of the mhod           */
	  put32_n0 (cts, 2);          /* unknown                    */
	  /* end of header, start of data */
	  put32lint (cts, 1);         /* string type UTF16          */
	  put32lint (cts, 2*len);     /* size of string             */
	  put32_n0 (cts, 2);          /* unknown                    */
	  put_data (cts, (gchar *)entry_utf16, 2*len);/* the string */
	  g_free (entry_utf16);
      }
      else
      {
	  guint32 len = strlen (mhod->data.string);
	  put_header (cts, "mhod");   /* header                     */
	  put32lint (cts, 24);        /* size of header             */
	  put32lint (cts, len+40);    /* size of header + body      */
	  put32lint (cts, mhod->type);/* type of the mhod           */
	  put32_n0 (cts, 2);          /* unknown                    */
	  /* end of header, start of data */
	  put32lint (cts, 2);         /* string type UTF 8          */
	  put32lint (cts, len);       /* size of string             */
	  put8int (cts, 1);           /* unknown                    */
	  put8int (cts, 0);           /* unknown                    */
	  put8int (cts, 0);           /* unknown                    */
	  put8int (cts, 0);           /* unknown                    */
	  put32lint (cts, 0);         /* unknown                    */
	  put_string (cts, mhod->data.string);/* the string         */
      }
      break;
  case MHOD_ID_PODCASTURL:
  case MHOD_ID_PODCASTRSS:
      g_return_if_fail (mhod->data.string);
      {
	  guint32 len = strlen (mhod->data.string);
	  put_header (cts, "mhod");   /* header                     */
	  put32lint (cts, 24);        /* size of header             */
	  put32lint (cts, 24+len);    /* size of header + data      */
	  put32lint (cts, mhod->type);/* type of the mhod           */
	  put32_n0 (cts, 2);          /* unknown                    */
	  put_string (cts, mhod->data.string);/* the string         */
      }
      break;
  case MHOD_ID_PLAYLIST:
      put_header (cts, "mhod");   /* header                     */
      put32lint (cts, 24);        /* size of header             */
      put32lint (cts, 44);        /* size of header + body      */
      put32lint (cts, mhod->type); /* type of the entry          */
      put32_n0 (cts, 2);          /* unknown                    */
      /* end of header, start of data */
      put32lint (cts, mhod->data.track_pos);/* position of track in playlist */
      put32_n0 (cts, 4);             /* unknown                    */
      break;
  case MHOD_ID_CHAPTERDATA:
      g_return_if_fail (mhod->data.chapterdata_track);
      {
	  Itdb_Track *track = mhod->data.chapterdata_track;
	  put_header (cts, "mhod");   /* header                     */
	  put32lint (cts, 24);        /* size of header             */
	  put32lint (cts, 24+track->chapterdata_raw_length); /*size */
	  put32lint (cts, mhod->type); /* type of the entry          */
	  put32_n0 (cts, 2);          /* unknown                    */
	  put_data (cts, track->chapterdata_raw,
		    track->chapterdata_raw_length);
      }
      break;
  case MHOD_ID_SPLPREF:
      g_return_if_fail (mhod->data.splpref);
      put_header (cts, "mhod");  /* header                 */
      put32lint (cts, 24);       /* size of header         */
      put32lint (cts, 96);       /* size of header + body  */
      put32lint (cts, mhod->type);/* type of the entry      */
      put32_n0 (cts, 2);         /* unknown                */
      /* end of header, start of data */
      put8int (cts, mhod->data.splpref->liveupdate);
      put8int (cts, mhod->data.splpref->checkrules? 1:0);
      put8int (cts, mhod->data.splpref->checklimits);
      put8int (cts, mhod->data.splpref->limittype);
      put8int (cts, mhod->data.splpref->limitsort & 0xff);
      put8int (cts, 0);          /* unknown                */
      put8int (cts, 0);          /* unknown                */
      put8int (cts, 0);          /* unknown                */
      put32lint (cts, mhod->data.splpref->limitvalue);
      put8int (cts, mhod->data.splpref->matchcheckedonly);
      /* for the following see note at definitions of limitsort
	 types in itunesdb.h */
      put8int (cts, (mhod->data.splpref->limitsort & 0x80000000) ? 1:0);
      put8int (cts, 0);          /* unknown                */
      put8int (cts, 0);          /* unknown                */
      put32_n0 (cts, 14);        /* unknown                */
      break;
  case MHOD_ID_SPLRULES:
      g_return_if_fail (mhod->data.splrules);
      {
	  gulong header_seek = cts->pos; /* needed to fix length */
	  GList *gl;
	  gint numrules = g_list_length (mhod->data.splrules->rules);

	  put_header (cts, "mhod");  /* header                   */
	  put32lint (cts, 24);       /* size of header           */
	  put32lint (cts, -1);       /* total length, fix later  */
	  put32lint (cts, mhod->type);/* type of the entry        */
	  put32_n0 (cts, 2);         /* unknown                  */
	  /* end of header, start of data */
	  /* For some reason this is the only part of the iTunesDB
	     that uses big endian */
	  put_header (cts, "SLst");  /* header                   */
	  put32bint (cts, mhod->data.splrules->unk004); /* unknown*/
	  put32bint (cts, numrules);
	  put32bint (cts, mhod->data.splrules->match_operator);
	  put32_n0 (cts, 30);            /* unknown              */
	  /* end of header, now follow the rules */
	  for (gl=mhod->data.splrules->rules; gl; gl=gl->next)
	  {
	      SPLRule *splr = gl->data;
	      SPLFieldType ft;
	      gint len;
	      gunichar2 *entry_utf16;
	      g_return_if_fail (splr);
	      ft = itdb_splr_get_field_type (splr);
/*	      printf ("%p: field: %d ft: %d\n", splr, splr->field, ft);*/
	      itdb_splr_validate (splr);
	      put32bint (cts, splr->field);
	      put32bint (cts, splr->action);
	      put32_n0 (cts, 11);          /* unknown              */
	      switch (ft)
	      {
	      case splft_string:
		  /* write string-type rule */
		  entry_utf16 = NULL;
		  /* splr->string may be NULL */
		  if (splr->string)
		      entry_utf16 = g_utf8_to_utf16 (splr->string,
						     -1,NULL,NULL,NULL);
		  len = utf16_strlen (entry_utf16);
		  fixup_big_utf16 (entry_utf16);
		  put32bint (cts, 2*len); /* length of string     */
		  put_data (cts, (gchar *)entry_utf16, 2*len);
		  g_free (entry_utf16);
		  break;
	      case splft_int:
	      case splft_date:
	      case splft_boolean:
	      case splft_playlist:
	      case splft_unknown:
	      case splft_binary_and:
		  /* write non-string-type rule */
		  put32bint (cts, 0x44); /* length of data        */
		  /* data */
		  put64bint (cts, splr->fromvalue);
		  put64bint (cts, splr->fromdate);
		  put64bint (cts, splr->fromunits);
		  put64bint (cts, splr->tovalue);
		  put64bint (cts, splr->todate);
		  put64bint (cts, splr->tounits);
		  put32bint (cts, splr->unk052);
		  put32bint (cts, splr->unk056);
		  put32bint (cts, splr->unk060);
		  put32bint (cts, splr->unk064);
		  put32bint (cts, splr->unk068);
		  break;
	      }
	  }
	  /* insert length of mhod junk */
	  fix_header (cts, header_seek);
      }
      break;
  case MHOD_ID_LIBPLAYLISTINDEX:
      g_return_if_fail (mhod->data.mhod52coltracks);
      g_return_if_fail (mhod->data.mhod52coltracks->data);
      {
	  struct mhod52track *ct = mhod->data.mhod52coltracks->data;
	  gint numtracks = ct->numtracks;
	  GList *gl;
	  gpointer compfunc = NULL;

	  /* sort list */
	  switch (mhod->data2.mhod52sorttype)
	  {
	  case MHOD52_SORTTYPE_TITLE:
	      compfunc = mhod52_sort_title;
	      break;
	  case MHOD52_SORTTYPE_ALBUM:
	      compfunc = mhod52_sort_album;
	      break;
	  case MHOD52_SORTTYPE_ARTIST:
	      compfunc = mhod52_sort_artist;
	      break;
	  case MHOD52_SORTTYPE_GENRE:
	      compfunc = mhod52_sort_genre;
	      break;
	  case MHOD52_SORTTYPE_COMPOSER:
	      compfunc = mhod52_sort_composer;
	      break;
	  }
	  g_return_if_fail (compfunc);

	  /* sort the tracks */
	  mhod->data.mhod52coltracks = g_list_sort (mhod->data.mhod52coltracks,
						    compfunc);
	  /* Write the MHOD */
	  put_header (cts, "mhod");         /* header                     */
	  put32lint (cts, 24);              /* size of header             */
	  put32lint (cts, 4*numtracks+72);  /* size of header + body      */
	  put32lint (cts, mhod->type);      /* type of the mhod           */
	  put32_n0 (cts, 2);                /* unknown                    */
	  /* end of header, start of data */
	  put32lint (cts, mhod->data2.mhod52sorttype);   /* sort type     */
	  put32lint (cts, numtracks);       /* number of entries          */
	  put32_n0 (cts, 10);               /* unknown                    */
	  for (gl=mhod->data.mhod52coltracks; gl; gl=gl->next)
	  {
	      ct = gl->data;
	      g_return_if_fail (ct);
	      put32lint (cts, ct->index);
	  }
      }
      break;
  }
}


/* Write out the mhlp header. Size will be written later */
static void mk_mhlp (FExport *fexp)
{
  WContents *cts;

  g_return_if_fail (fexp);
  g_return_if_fail (fexp->wcontents);

  cts = fexp->wcontents;

  put_header (cts, "mhlp");        /* header                   */
  put32lint (cts, 92);             /* size of header           */
  /* playlists on iPod (including main!) */
  put32lint (cts, g_list_length (fexp->itdb->playlists));
  put32_n0 (cts, 20);               /* dummy space              */
}


/* Write out the long MHOD_ID_PLAYLIST mhod header.
   This seems to be an itunespref thing.. dunno know this
   but if we set everything to 0, itunes doesn't show any data
   even if you drag an mp3 to your ipod: nothing is shown, but itunes
   will copy the file!
   .. so we create a hardcoded-pref.. this will change in future
   Seems to be a Preferences mhod, every PL has such a thing
   FIXME !!! */
static void mk_long_mhod_id_playlist (FExport *fexp, Itdb_Playlist *pl)
{
  WContents *cts;

  g_return_if_fail (fexp);
  g_return_if_fail (fexp->wcontents);
  g_return_if_fail (pl);

  cts = fexp->wcontents;

  put_header (cts, "mhod");          /* header                   */
  put32lint (cts, 0x18);             /* size of header  ?        */
  put32lint (cts, 0x0288);           /* size of header + body    */
  put32lint (cts, MHOD_ID_PLAYLIST); /* type of the entry        */
  put32_n0 (cts, 6);
  put32lint (cts, 0x010084);         /* ? */
  put32lint (cts, 0x05);             /* ? */
  put32lint (cts, 0x09);             /* ? */
  put32lint (cts, 0x03);             /* ? */
  put32lint (cts, 0x120001);         /* ? */
  put32_n0 (cts, 3);
  put32lint (cts, 0xc80002);         /* ? */
  put32_n0 (cts, 3);
  put32lint (cts, 0x3c000d);         /* ? */
  put32_n0 (cts, 3);
  put32lint (cts, 0x7d0004);         /* ? */
  put32_n0 (cts, 3);
  put32lint (cts, 0x7d0003);         /* ? */
  put32_n0 (cts, 3);
  put32lint (cts, 0x640008);         /* ? */
  put32_n0 (cts, 3);
  put32lint (cts, 0x640017);         /* ? */
  put32lint (cts, 0x01);             /* bool? (visible? / colums?) */
  put32_n0 (cts, 2);
  put32lint (cts, 0x500014);         /* ? */
  put32lint (cts, 0x01);             /* bool? (visible?) */
  put32_n0 (cts, 2);
  put32lint (cts, 0x7d0015);         /* ? */
  put32lint (cts, 0x01);             /* bool? (visible?) */
  put32_n0 (cts, 114);
}



/* Header for new PL item */
/* @pos: position in playlist */
static void mk_mhip (FExport *fexp,
		     guint32 childcount,
		     guint32 podcastgroupflag,
		     guint32 podcastgroupid,
		     guint32 trackid,
		     guint32 timestamp,
		     guint32 podcastgroupref)
{
  WContents *cts;

  g_return_if_fail (fexp);
  g_return_if_fail (fexp->wcontents);

  cts = fexp->wcontents;

  put_header (cts, "mhip");
  put32lint (cts, 76);                              /*  4 */
  put32lint (cts, -1);  /* fill in later */         /*  8 */
  put32lint (cts, childcount);                      /* 12 */
  put32lint (cts, podcastgroupflag);                /* 16 */
  put32lint (cts, podcastgroupid);                  /* 20 */
  put32lint (cts, trackid);                         /* 24 */
  put32lint (cts, timestamp);                       /* 28 */
  put32lint (cts, podcastgroupref);                 /* 32 */
  put32_n0 (cts, 10);                               /* 36 */
}


/* Write first mhsd hunk. Return FALSE in case of error and set
 * fexp->error */
static gboolean write_mhsd_tracks (FExport *fexp)
{
    GList *gl;
    gulong mhsd_seek;
    WContents *cts;

    g_return_val_if_fail (fexp, FALSE);
    g_return_val_if_fail (fexp->itdb, FALSE);
    g_return_val_if_fail (fexp->wcontents, FALSE);

    cts = fexp->wcontents;
    
    mhsd_seek = cts->pos;      /* get position of mhsd header  */
    mk_mhsd (fexp, 1);         /* write header: type 1: tracks */
    /* write header with nr. of tracks */
    mk_mhlt (fexp, g_list_length (fexp->itdb->tracks));
    for (gl=fexp->itdb->tracks; gl; gl=gl->next)  /* Write each track */
    {
	Itdb_Track *track = gl->data;
	guint32 mhod_num = 0;
	gulong mhit_seek = cts->pos;
	MHODData mhod;

	g_return_val_if_fail (track, FALSE);

	mhod.valid = TRUE;

	mk_mhit (cts, track);
	if (track->title && *track->title)
	{
	    mhod.type = MHOD_ID_TITLE;
	    mhod.data.string = track->title;
	    mk_mhod (cts, &mhod);
	    ++mhod_num;
	}
	if (track->ipod_path && *track->ipod_path)
	{
	    mhod.type = MHOD_ID_PATH;
	    mhod.data.string = track->ipod_path;
	    mk_mhod (cts, &mhod);
	    ++mhod_num;
	}
	if (track->album && *track->album)
	{
	    mhod.type = MHOD_ID_ALBUM;
	    mhod.data.string = track->album;
	    mk_mhod (cts, &mhod);
	    ++mhod_num;
	}
	if (track->artist && *track->artist)
	{
	    mhod.type = MHOD_ID_ARTIST;
	    mhod.data.string = track->artist;
	    mk_mhod (cts, &mhod);
	    ++mhod_num;
	}
	if (track->genre && *track->genre)
	{
	    mhod.type = MHOD_ID_GENRE;
	    mhod.data.string = track->genre;
	    mk_mhod (cts, &mhod);
	    ++mhod_num;
	}
	if (track->filetype && *track->filetype)
	{
	    mhod.type = MHOD_ID_FILETYPE;
	    mhod.data.string = track->filetype;
	    mk_mhod (cts, &mhod);
	    ++mhod_num;
	}
	if (track->comment && *track->comment)
	{
	    mhod.type = MHOD_ID_COMMENT;
	    mhod.data.string = track->comment;
	    mk_mhod (cts, &mhod);
	    ++mhod_num;
	}
	if (track->category && *track->category)
	{
	    mhod.type = MHOD_ID_CATEGORY;
	    mhod.data.string = track->category;
	    mk_mhod (cts, &mhod);
	    ++mhod_num;
	}
	if (track->composer && *track->composer)
	{
	    mhod.type = MHOD_ID_COMPOSER;
	    mhod.data.string = track->composer;
	    mk_mhod (cts, &mhod);
	    ++mhod_num;
	}
	if (track->grouping && *track->grouping)
	{
	    mhod.type = MHOD_ID_GROUPING;
	    mhod.data.string = track->grouping;
	    mk_mhod (cts, &mhod);
	    ++mhod_num;
	}
	if (track->description && *track->description)
	{
	    mhod.type = MHOD_ID_DESCRIPTION;
	    mhod.data.string = track->description;
	    mk_mhod (cts, &mhod);
	    ++mhod_num;
	}
	if (track->subtitle && *track->subtitle)
	{
	    mhod.type = MHOD_ID_SUBTITLE;
	    mhod.data.string = track->subtitle;
	    mk_mhod (cts, &mhod);
	    ++mhod_num;
	}
	if (track->tvshow && *track->tvshow)
	{
	    mhod.type = MHOD_ID_TVSHOW;
	    mhod.data.string = track->tvshow;
	    mk_mhod (cts, &mhod);
	    ++mhod_num;
	}
	if (track->tvepisode && *track->tvepisode)
	{
	    mhod.type = MHOD_ID_TVEPISODE;
	    mhod.data.string = track->tvepisode;
	    mk_mhod (cts, &mhod);
	    ++mhod_num;
	}
	if (track->tvnetwork && *track->tvnetwork)
	{
	    mhod.type = MHOD_ID_TVNETWORK;
	    mhod.data.string = track->tvnetwork;
	    mk_mhod (cts, &mhod);
	    ++mhod_num;
	}
	if (track->albumartist && *track->albumartist)
	{
	    mhod.type = MHOD_ID_ALBUMARTIST;
	    mhod.data.string = track->albumartist;
	    mk_mhod (cts, &mhod);
	    ++mhod_num;
	}
	if (track->keywords && *track->keywords)
	{
	    mhod.type = MHOD_ID_KEYWORDS;
	    mhod.data.string = track->keywords;
	    mk_mhod (cts, &mhod);
	    ++mhod_num;
	}
	if (track->podcasturl && *track->podcasturl)
	{
	    mhod.type = MHOD_ID_PODCASTURL;
	    mhod.data.string = track->podcasturl;
	    mk_mhod (cts, &mhod);
	    ++mhod_num;
	}
	if (track->podcastrss && *track->podcastrss)
	{
	    mhod.type = MHOD_ID_PODCASTRSS;
	    mhod.data.string = track->podcastrss;
	    mk_mhod (cts, &mhod);
	    ++mhod_num;
	}
	if (track->chapterdata_raw && track->chapterdata_raw_length)
	{
	    mhod.type = MHOD_ID_CHAPTERDATA;
	    mhod.data.chapterdata_track = track;
	    mk_mhod (cts, &mhod);
	    ++mhod_num;
	}
        /* Fill in the missing items of the mhit header */
	fix_mhit (cts, mhit_seek, mhod_num);
    }
    fix_header (cts, mhsd_seek);
    return TRUE;
}


/* write out the mhip/mhod pairs for playlist @pl. @mhyp_seek points
   to the start of the corresponding mhyp, but is not used yet. */
static gboolean write_playlist_mhips (FExport *fexp,
				      Itdb_Playlist *pl,
				      glong mhyp_seek)
{
    GList *gl;
    WContents *cts;
    guint32 i=0;
    guint32 mhip_num;

    g_return_val_if_fail (fexp, FALSE);
    g_return_val_if_fail (fexp->itdb, FALSE);
    g_return_val_if_fail (fexp->wcontents, FALSE);
    g_return_val_if_fail (pl, FALSE);

    cts = fexp->wcontents;

    for (gl=pl->members; gl; gl=gl->next)
    {
	Itdb_Track *track = gl->data;
	glong mhip_seek = cts->pos;
	MHODData mhod;

	g_return_val_if_fail (track, FALSE);

	mk_mhip (fexp, 1, 0, 0, track->id, 0, 0);
	mhod.valid = TRUE;
	mhod.type = MHOD_ID_PLAYLIST;
	mhod.data.track_pos = i;
	mk_mhod (cts, &mhod);
	/* note: with iTunes 4.9 the mhod is counted as a child to
	   mhip, so we have put the total length of the mhip and mhod
	   into the mhip header */
	fix_header (cts, mhip_seek);
	++i;
    }

    /* set number of mhips */
    mhip_num = g_list_length (pl->members);
    put32lint_seek (cts, mhip_num, mhyp_seek+16);

    return TRUE;
}


void free_memberlist (gpointer data);
void write_one_podcast_group (gpointer key, gpointer value, gpointer userdata);
void free_memberlist (gpointer data)
{
    GList **memberlist = data;
    if (memberlist)
    {
	g_list_free (*memberlist);
	g_free (memberlist);
    }
}
void write_one_podcast_group (gpointer key, gpointer value,
			      gpointer userdata)
{
    gchar *album = key;
    GList **memberlist = value;
    FExport *fexp = userdata;
    GList *gl;
    WContents *cts;
    glong mhip_seek;
    guint32 groupid;
    Itdb_iTunesDB *itdb;
    MHODData mhod;

    g_return_if_fail (album);
    g_return_if_fail (memberlist);
    g_return_if_fail (fexp);
    g_return_if_fail (fexp->itdb);
    g_return_if_fail (fexp->wcontents);

    cts = fexp->wcontents;
    itdb = fexp->itdb;
    mhip_seek = cts->pos;

    groupid = fexp->next_id++;
    mk_mhip (fexp, 1, 256, groupid, 0, 0, 0);

    mhod.valid = TRUE;
    mhod.type = MHOD_ID_TITLE;
    mhod.data.string = album;
    mk_mhod (cts, &mhod);
    fix_header (cts, mhip_seek);

    /* write members */
    for (gl=*memberlist; gl; gl=gl->next)
    {
	Itdb_Track *track = gl->data;
	guint32 mhip_id;

	g_return_if_fail (track);

	mhip_seek = cts->pos;
	mhip_id = fexp->next_id++;
	mk_mhip (fexp, 1, 0, mhip_id, track->id, 0, groupid);
	mhod.type = MHOD_ID_PLAYLIST;
	mhod.data.track_pos = mhip_id;
	mk_mhod (cts, &mhod);
	fix_header (cts, mhip_seek);
    }
}



/* write out the mhip/mhod pairs for the podcast playlist
   @pl. @mhyp_seek points to the start of the corresponding mhyp, but
   is not used yet. */
static gboolean write_podcast_mhips (FExport *fexp,
				     Itdb_Playlist *pl,
				     glong mhyp_seek)
{
    GList *gl;
    WContents *cts;
    guint32 mhip_num;
    GHashTable *album_hash;

    g_return_val_if_fail (fexp, FALSE);
    g_return_val_if_fail (fexp->itdb, FALSE);
    g_return_val_if_fail (fexp->wcontents, FALSE);
    g_return_val_if_fail (pl, FALSE);

    cts = fexp->wcontents;

    /* Create a list wit all available album names because we have to
       group the podcasts according to albums */

    album_hash = g_hash_table_new_full (g_str_hash, g_str_equal,
					NULL, free_memberlist);
    for (gl=pl->members; gl; gl=gl->next)
    {
	GList **memberlist;
	gchar *album;
	Itdb_Track *track = gl->data;
	g_return_val_if_fail (track, FALSE);

	if (track->album)
	    album = track->album;
	else
	    album = "";

	memberlist = g_hash_table_lookup (album_hash, album);

	if (!memberlist)
	{
	    memberlist = g_new0 (GList *, 1);
	    g_hash_table_insert (album_hash, album, memberlist);
	}
	*memberlist = g_list_append (*memberlist, track);
    }

    g_hash_table_foreach (album_hash, write_one_podcast_group, fexp);

    /* set number of mhips */
    mhip_num = g_list_length (pl->members)+g_hash_table_size (album_hash);
    put32lint_seek (cts, mhip_num, mhyp_seek+16);

    g_hash_table_destroy (album_hash);

    return TRUE;
}


/* corresponds to mk_mhyp */
/* mhsd_type: 2: write normal playlists
              3: write podcast playlist in special format */
/* Return FALSE in case of error and set fexp->error */
static gboolean write_playlist (FExport *fexp,
				Itdb_Playlist *pl, guint32 mhsd_type)
{
    gulong mhyp_seek;
    WContents *cts;
    gboolean result = TRUE;
    MHODData mhod;
    gint mhodnum;

    g_return_val_if_fail (fexp, FALSE);
    g_return_val_if_fail (fexp->itdb, FALSE);
    g_return_val_if_fail (fexp->wcontents, FALSE);
    g_return_val_if_fail (pl, FALSE);

    cts = fexp->wcontents;
    mhyp_seek = cts->pos;

#if ITUNESDB_DEBUG
  fprintf(stderr, "Playlist: %s (%d tracks)\n", pl->name, g_list_length (pl->members));
#endif

    put_header (cts, "mhyp");      /* header                    */
    put32lint (cts, 108);          /* length		        */
    put32lint (cts, -1);           /* size -> later             */
    mhodnum = 2;
    if (pl->is_spl)
    {   /* 2 more SPL MHODs */
	mhodnum += 2;
    }
    else
    {
	if ((pl->type == ITDB_PL_TYPE_MPL) && pl->members)
	{   /* 5 more MHOD 52 lists */
	    mhodnum += 5;
	}
    }
    put32lint (cts, mhodnum);      /* nr of mhods               */
    /* number of tracks in plist */
    put32lint (cts, -1);           /* number of mhips -> later  */
    put8int (cts, pl->type);       /* 1 = main, 0 = visible     */
    put8int (cts, pl->flag1);      /* unknown                   */
    put8int (cts, pl->flag2);      /* unknown                   */
    put8int (cts, pl->flag3);      /* unknown                   */
    put32lint (cts, pl->timestamp);/* some timestamp            */
    put64lint (cts, pl->id);       /* 64 bit ID                 */
    put32lint (cts, 0);            /* unknown, always 0?        */
    put16lint (cts, 1);            /* string mhod count (1)     */
    put16lint (cts, pl->podcastflag);
    put32lint (cts, pl->sortorder);
    put32_n0 (cts, 15);            /* ?                         */

    mhod.valid = TRUE;
    mhod.type = MHOD_ID_TITLE;
    mhod.data.string = pl->name;
    mk_mhod (cts, &mhod);
    mk_long_mhod_id_playlist (fexp, pl);

    if ((pl->type == ITDB_PL_TYPE_MPL) && pl->members)
    {   /* write out the MHOD 52 lists */
	/* We have to sort all tracks five times. To speed this up,
	   translate the utf8 keys into collate_keys and use the
	   faster strcmp() for comparison */
	mhod.valid = TRUE;
	mhod.type = MHOD_ID_LIBPLAYLISTINDEX;
	mhod.data.mhod52coltracks = mhod52_make_collate_keys (pl->members);
	mhod.data2.mhod52sorttype = MHOD52_SORTTYPE_TITLE;
	mk_mhod (cts, &mhod);
	mhod.data2.mhod52sorttype = MHOD52_SORTTYPE_ALBUM;
	mk_mhod (cts, &mhod);
	mhod.data2.mhod52sorttype = MHOD52_SORTTYPE_ARTIST;
	mk_mhod (cts, &mhod);
	mhod.data2.mhod52sorttype = MHOD52_SORTTYPE_GENRE;
	mk_mhod (cts, &mhod);
	mhod.data2.mhod52sorttype = MHOD52_SORTTYPE_COMPOSER;
	mk_mhod (cts, &mhod);
	mhod52_free_collate_keys (mhod.data.mhod52coltracks);
    }
    else  if (pl->is_spl)
    {  /* write the smart rules */
	mhod.type = MHOD_ID_SPLPREF;
	mhod.data.splpref = &pl->splpref;
	mk_mhod (cts, &mhod);

	mhod.type = MHOD_ID_SPLRULES;
	mhod.data.splrules = &pl->splrules;
	mk_mhod (cts, &mhod);
    }

    if (itdb_playlist_is_podcasts(pl) && (mhsd_type == 3))
    {
	/* write special podcast playlist */
	result = write_podcast_mhips (fexp, pl, mhyp_seek);
    }
    else
    {   /* write standard playlist hard-coded tracks */
	result = write_playlist_mhips (fexp, pl, mhyp_seek);
    }
    fix_header (cts, mhyp_seek);
    return result;
}


/* Expects the master playlist to be the first in the list */
/* Return FALSE in case of error and set fexp->error */
static gboolean write_mhsd_playlists (FExport *fexp, guint32 mhsd_type)
{
    GList *gl;
    glong mhsd_seek;
    WContents *cts;

    g_return_val_if_fail (fexp, FALSE);
    g_return_val_if_fail (fexp->itdb, FALSE);
    g_return_val_if_fail (fexp->wcontents, FALSE);
    g_return_val_if_fail ((mhsd_type == 2) || (mhsd_type == 3), FALSE);

    cts = fexp->wcontents;
    mhsd_seek = cts->pos;      /* get position of mhsd header */
    mk_mhsd (fexp, mhsd_type); /* write header */
    /* write header with nr. of playlists */
    mk_mhlp (fexp);
    for (gl=fexp->itdb->playlists; gl; gl=gl->next)
    {
	Itdb_Playlist *pl = gl->data;
	g_return_val_if_fail (pl, FALSE);

	write_playlist (fexp, pl, mhsd_type);
	if (fexp->error)  return FALSE;
    }
    fix_header (cts, mhsd_seek);
    return TRUE;
}


/* create a WContents structure */
static WContents *wcontents_new (const gchar *filename)
{
    WContents *cts;

    g_return_val_if_fail (filename, NULL);

    cts = g_new0 (WContents, 1);
    cts->filename = g_strdup (filename);

    return cts;
}


/* write the contents of WContents. Return FALSE on error and set
 * cts->error accordingly. */
static gboolean wcontents_write (WContents *cts)
{
    int fd;

    g_return_val_if_fail (cts, FALSE);
    g_return_val_if_fail (cts->filename, FALSE);

    fd = creat (cts->filename, S_IRWXU|S_IRWXG|S_IRWXO);

    if (fd == -1)
    {
	cts->error = g_error_new (G_FILE_ERROR,
				  g_file_error_from_errno (errno),
				  _("Opening of '%s' for writing failed (%s)."),
				  cts->filename, g_strerror (errno));
	return FALSE;
    }
    if (cts->contents && cts->pos)
    {
	ssize_t written = write (fd, cts->contents, cts->pos);
	if (written == -1)
	{
	    cts->error = g_error_new (G_FILE_ERROR,
				      g_file_error_from_errno (errno),
				      _("Writing to '%s' failed (%s)."),
				      cts->filename, g_strerror (errno));
	    close (fd);
	    return FALSE;
	}
    }
    fd = close (fd);
    if (fd == -1)
    {
	cts->error = g_error_new (G_FILE_ERROR,
				  g_file_error_from_errno (errno),
				  _("Writing to '%s' failed (%s)."),
				  cts->filename, g_strerror (errno));
	return FALSE;
    }
    return TRUE;
}


/* Free memory associated with WContents @cts */
static void wcontents_free (WContents *cts)
{
    if (cts)
    {
	g_free (cts->filename);
	g_free (cts->contents);
	/* must not g_error_free (cts->error) because the error was
	   propagated -> might free the error twice */
	g_free (cts);
    }
}


/* reassign the iPod IDs and make sure the itdb->tracks are in the
   same order as the mpl */
static void reassign_ids (FExport *fexp)
{
    GList *gl;
    Itdb_iTunesDB *itdb;
    Itdb_Playlist *mpl;

    g_return_if_fail (fexp);
    itdb = fexp->itdb;
    g_return_if_fail (itdb);

    /* Arrange itdb->tracks in the same order as mpl->members
       (otherwise On-The-Go Playlists will not show the correct
       content. Unfortunately we cannot just copy mpl->members to
       itdb->tracks because podcasts do not show up in the MPL. */

    mpl = itdb_playlist_mpl (itdb);
    g_return_if_fail (mpl);

    for (gl=g_list_last(mpl->members); gl; gl=gl->prev)
    {
	Itdb_Track *track = gl->data;
	g_return_if_fail (track);
	g_return_if_fail (g_list_find (itdb->tracks, track));

	/* move this track to the beginning of itdb_tracks */
	itdb->tracks = g_list_remove (itdb->tracks, track);
	itdb->tracks = g_list_prepend (itdb->tracks, track);
    }

    fexp->next_id = FIRST_IPOD_ID;

    /* assign unique IDs */
    for (gl=itdb->tracks; gl; gl=gl->next)
    {
	Itdb_Track *track = gl->data;
	g_return_if_fail (track);
	track->id = fexp->next_id++;
    }
}


/**
 * itdb_write_file:
 * @itdb: the #Itdb_iTunesDB to save
 * @filename: filename to save @itdb to
 * @error: return location for a #GError or NULL
 *
 * Write the content of @itdb to @filename. If @filename is NULL, it attempts
 * to write to itdb-&gt;filename.
 *
 * Return value: TRUE if all went well, FALSE otherwise
 **/
gboolean itdb_write_file (Itdb_iTunesDB *itdb, const gchar *filename,
			  GError **error)
{
    FExport *fexp;
    gulong mhbd_seek = 0;
    WContents *cts;
    gboolean result = TRUE;;

    g_return_val_if_fail (itdb, FALSE);
    g_return_val_if_fail (itdb->device, FALSE);
    g_return_val_if_fail (filename || itdb->filename, FALSE);

    if (!filename) filename = itdb->filename;

    /* set endianess flag */
    if (!itdb->device->byte_order)
	itdb_device_autodetect_endianess (itdb->device);

#if HAVE_GDKPIXBUF
    /* only write ArtworkDB if we deal with an iPod
       FIXME: figure out a way to store the artwork data when storing
       to local directories. At the moment it's the application's task
       to handle this. */
    if (itdb_device_supports_artwork (itdb->device)) {
		ipod_write_artwork_db (itdb);
    }
#endif

    fexp = g_new0 (FExport, 1);
    fexp->itdb = itdb;
    fexp->wcontents = wcontents_new (filename);
    cts = fexp->wcontents;

    cts->reversed = (itdb->device->byte_order == G_BIG_ENDIAN);

    reassign_ids (fexp);

    mk_mhbd (fexp, 3);   /* three mhsds */
    /* write tracklist */
    if (write_mhsd_tracks (fexp))
    {   /* write special podcast version mhsd */
	if (write_mhsd_playlists (fexp, 3))
	{   /* write standard playlist mhsd */
	    if (write_mhsd_playlists (fexp, 2))
	    {
		fix_header (cts, mhbd_seek);
	    }
	}
    }
    if (!fexp->error)
    {
	if (!wcontents_write (cts))
	    g_propagate_error (&fexp->error, cts->error);
    }
    if (fexp->error)
    {
	g_propagate_error (error, fexp->error);
	result = FALSE;
    }
    wcontents_free (cts);
    g_free (fexp);
    if (result == TRUE)
    {
	gchar *fn = g_strdup (filename);
	g_free (itdb->filename);
	itdb->filename = fn;
    }

    /* make sure all buffers are flushed as some people tend to
       disconnect as soon as gtkpod returns */
    sync ();

    return result;
}

/**
 * itdb_write:
 * @itdb: the #Itdb_iTunesDB to write to disk
 * @error: return location for a #GError or NULL
 *
 * Write out an iTunesDB. It reassigns unique IDs to all tracks. 
 * An existing "Play Counts" file is renamed to "Play Counts.bak" if
 * the export was successful.
 * An existing "OTGPlaylistInfo" file is removed if the export was
 * successful.
 *
 * Return value: TRUE on success, FALSE on error, in which case @error is
 * set accordingly.
 **/
gboolean itdb_write (Itdb_iTunesDB *itdb, GError **error)
{
    gchar *itunes_filename, *itunes_path;
    gboolean result = FALSE;

    g_return_val_if_fail (itdb, FALSE);
    g_return_val_if_fail (itdb_get_mountpoint (itdb), FALSE);

    /* First, let's try to write the .ithmb files containing the artwork data
     * since this operation modifies the 'artwork_count' and 'artwork_size' 
     * field in the Itdb_Track contained in the database.
     * Errors happening during that operation are considered non fatal since
     * they shouldn't corrupt the main database.
     */


    itunes_path = itdb_get_itunes_dir (itdb_get_mountpoint (itdb));

    if(!itunes_path)
    {
	error_no_itunes_dir (itdb_get_mountpoint (itdb), error);
	return FALSE;
    }

    itunes_filename = g_build_filename (itunes_path, "iTunesDB", NULL);

    result = itdb_write_file (itdb, itunes_filename, error);

    g_free (itunes_filename);
    g_free (itunes_path);

    if (result == TRUE)
    {
	/* Write SysInfo file if it has changed */
	if (itdb->device->sysinfo_changed)
	{
	    itdb_device_write_sysinfo (itdb->device, error);
	}
	result = itdb_rename_files (itdb_get_mountpoint (itdb), error);
    }

    /* make sure all buffers are flushed as some people tend to
       disconnect as soon as gtkpod returns */
    sync ();

    return result;
}


/* from here on we have the functions for writing the iTunesDB          */
/* -------------------------------------------------------------------- */
/* up to here we had the functions for writing the iTunesSD             */

/*
|  Copyright (C) 2005 Jorg Schuler <jcsjcs at users.sourceforge.net>
|  Part of the gtkpod project.
|
|  Based on itunessd.c written by Steve Wahl for gtkpod-0.88:
|
|  Copyright 2005 Steve Wahl <steve at pro-ns dot net>
|
|  This file contains routines to create the iTunesSD file, as
|  used by the ipod shuffle.
|
|  Like itunesdb.c, it is derived from the perl script "mktunes.pl"
|  (part of the gnupod-tools collection) written by Adrian
|  Ulrich <pab at blinkenlights.ch>.
|
|  Small(?) portions derived from itunesdb.c, so Jorg Schuler probably
|  has some copyright ownership in this file as well.
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
*/

/* notes:

  All software currently seems to write iTunesDB as well as iTunesSD
  on the iPod shuffle.  I assume that reading from the iTunesSD file
  is not necessary.  The iTunesStats file is different, but I leave
  that for another day.

  The iTunesSD file format is as follows (taken from WikiPodLinux, feb
  '05):

    Offset Field         Bytes   Value
     (hex)               (dec)

  iTunesSD header (occurs once, at beginning of file):

    00      num_songs     3       number of song entries in the file

    03      unknown       3       always(?) 0x010600
    06      header size   3       size of the header (0x12, 18 bytes)
    09      unknown       3       possibly zero padding

  iTunesSD song entry format (occurs once for each song)

   000      size of entry 3       always(?) 0x00022e (558 bytes)
   003      unk1          3       unknown, always(?) 0x5aa501
   006      starttime     3       Start Time, in 256 ms increments
                                  e.g. 60s = 0xea (234 dec)
   009      unk2          3       unknown (always 0?)
   00C      unk3          3       unknown, some relationship to starttime
   00F      stoptime      3       Stop Time, also in 256 ms increments.
                                  Zero means play to end of file.
   012      unk4          3       Unknown.
   015      unk5          3       Unknown, but associated with stoptime?
   018      volume        3       Volume - ranges from 0x00 (-100%) to 0x64
                                  (0%) to 0xc8 (100%)
   01B      file_type     3       0x01 = MP3, 0x02 = AAC, 0x04=WAV
   01E      unk6          3       unknown (always 0x200?)
   021      filename      522     filename of the song, padded at the end
                                  with 0's.  Note: forward slashes are used
                                  here, not colons like in the iTunesDB --
                                  for example,
                                  "/iPod_Control/Music/F00/Song.mp3"
   22B      shuffleflag   1       If this value is 0x00, the song will be
                                  skipped while the player is in shuffle
                                  mode.  Any other value will allow it to be
                                  played in both normal and shuffle modes.
                                  iTunes 4.7.1 sets this to 0 for audio books.
   22C      bookmarkflag  1       If this flag is 0x00, the song will not be
                                  bookmarkable (i.e. its playback position
                                  won't be saved when switching to a different
                                  song). Any other value wil make it
                                  Bookmarkable.  Unlike hard drive based iPods,
                                  all songs can be marked as bookmarkable,
                                  not just .m4b and .aa
   22D      unknownflag   1       unknown, always? 0x00.

All integers in the iTunesSD file are in BIG endian form...

*/


/**
 * itdb_shuffle_write:
 * @itdb: the #Itdb_iTunesDB to write to disk
 * @error: return location for a #GError or NULL
 *
 * Write out an iTunesSD for the Shuffle.
 * First reassigns unique IDs to all tracks.
 * An existing "Play Counts" file is renamed to "Play Counts.bak" if
 * the export was successful.
 * An existing "OTGPlaylistInfo" file is removed if the export was
 * successful.
 * @itdb->mountpoint must point to the mount point of the iPod,
 * e.g. "/mnt/ipod" and be in local encoding.
 *
 * Return value: TRUE on success, FALSE on error, in which case @error is
 * set accordingly.
 **/
gboolean itdb_shuffle_write (Itdb_iTunesDB *itdb, GError **error)
{
    gchar *itunes_filename, *itunes_path;
    gboolean result = FALSE;

    g_return_val_if_fail (itdb, FALSE);
    g_return_val_if_fail (itdb_get_mountpoint (itdb), FALSE);

    itunes_path = itdb_get_itunes_dir (itdb_get_mountpoint (itdb));
    
    if(!itunes_path)
    {
	gchar *str = g_build_filename (itdb_get_mountpoint (itdb),
				       "iPod_Control", "iTunes", NULL);
	g_set_error (error,
		     ITDB_FILE_ERROR,
		     ITDB_FILE_ERROR_NOTFOUND,
		     _("Path not found: '%s' (or similar)."),
		     str);
	g_free (str);
	return FALSE;
    }

    itunes_filename = g_build_filename (itunes_path, "iTunesSD", NULL);

    result = itdb_shuffle_write_file (itdb, itunes_filename, error);

    g_free(itunes_filename);
    g_free(itunes_path);

    if (result == TRUE)
	result = itdb_rename_files (itdb_get_mountpoint (itdb), error);

    /* make sure all buffers are flushed as some people tend to
       disconnect as soon as gtkpod returns */
    sync ();

    return result;
}


/**
 * itdb_shuffle_write_file:
 * @itdb: the #Itdb_iTunesDB to write to disk
 * @filename: file to write to, cannot be NULL
 * @error: return location for a #GError or NULL
 *
 * Do the actual writing to the iTunesSD 
 *
 * Return value: TRUE on success, FALSE on error, in which case @error is
 * set accordingly.
 **/
gboolean itdb_shuffle_write_file (Itdb_iTunesDB *itdb,
				  const gchar *filename, GError **error)
{
    auto gboolean haystack (gchar *filetype, gchar **desclist);
    gboolean haystack (gchar *filetype, gchar **desclist)
    {
	gchar **dlp;
	if (!filetype || !desclist) return FALSE;
	for (dlp=desclist; *dlp; ++dlp)
	{
	    if (strstr (filetype, *dlp)) return TRUE;
	}
	return FALSE;
    }

    FExport *fexp;
    GList *gl;
    WContents *cts;
    gboolean result = TRUE;;

    g_return_val_if_fail (itdb, FALSE);
    g_return_val_if_fail (filename, FALSE);

    fexp = g_new0 (FExport, 1);
    fexp->itdb = itdb;
    fexp->wcontents = wcontents_new (filename);
    cts = fexp->wcontents;

    reassign_ids (fexp);

    put24bint (cts, itdb_tracks_number (itdb));
    put24bint (cts, 0x010600);
    put24bint (cts, 0x12);	/* size of header */
    put24bint (cts, 0x0);	/* padding? */
    put24bint (cts, 0x0);
    put24bint (cts, 0x0);

    for (gl=itdb->tracks; gl; gl=gl->next)
    {
	Itdb_Track *tr = gl->data;
	gchar *path;
	gunichar2 *path_utf16;
	guint32 pathlen;
	gchar *mp3_desc[] = {"MPEG", "MP3", "mpeg", "mp3", NULL};
	gchar *mp4_desc[] = {"AAC", "MP4", "aac", "mp4", NULL};
	gchar *wav_desc[] = {"WAV", "wav", NULL};

	g_return_val_if_fail (tr, FALSE);

	put24bint (cts, 0x00022e);
	put24bint (cts, 0x5aa501);
	/* starttime is in 256 ms incr. for shuffle */
	put24bint (cts, tr->starttime / 256);
	put24bint (cts, 0);
	put24bint (cts, 0);
	put24bint (cts, tr->stoptime / 256);
	put24bint (cts, 0);
	put24bint (cts, 0);
	/* track->volume ranges from -255 to +255 */
	/* we want 0 - 200 */
	put24bint (cts, ((tr->volume + 255) * 201) / 511);

	/* The next one should be 0x01 for MP3,
	** 0x02 for AAC, and 0x04 for WAV, but I can't find
	** a suitable indicator within the track structure? */
	/* JCS: let's do heuristic on tr->filetype which would contain
	   "MPEG audio file", "AAC audio file", "Protected AAC audio
	   file", "AAC audio book file", "WAV audio file" (or similar
	   if not written by gtkpod) */

	if (haystack (tr->filetype, mp3_desc))
	    put24bint (cts, 0x01);
	else if (haystack (tr->filetype, mp4_desc))
	    put24bint (cts, 0x02);
	else if (haystack (tr->filetype, wav_desc))
	    put24bint (cts, 0x04);
	else
	    put24bint (cts, 0x01);  /* default to mp3 */

	put24bint (cts, 0x200);
		
	/* shuffle uses forward slash separator, not colon */
	path = g_strdup (tr->ipod_path);
	itdb_filename_ipod2fs (path);
	path_utf16 = g_utf8_to_utf16 (path, -1, NULL, NULL, NULL);
	pathlen = utf16_strlen (path_utf16);
	if (pathlen > 261) pathlen = 261;
	fixup_little_utf16 (path_utf16);
	put_data (cts, (gchar *)path_utf16, 2*pathlen);
	/* pad to 522 bytes */
	put16_n0 (cts, 261-pathlen);
	g_free(path);
	g_free(path_utf16);

	/* XXX FIXME: should depend on something, not hardcoded */
	put8int (cts, 0x1); /* song used in shuffle mode */
	put8int (cts, 0);   /* song will not be bookmarkable */
	put8int (cts, 0);
    }
    if (!fexp->error)
    {
	if (!wcontents_write (cts))
	    g_propagate_error (&fexp->error, cts->error);
    }
    if (fexp->error)
    {
	g_propagate_error (error, fexp->error);
	result = FALSE;
    }
    wcontents_free (cts);
    g_free (fexp);

    /* make sure all buffers are flushed as some people tend to
       disconnect as soon as gtkpod returns */
    sync ();

    return result;
}











/*------------------------------------------------------------------*\
 *                                                                  *
 *                  Other file/filename stuff                       *
 *                                                                  *
\*------------------------------------------------------------------*/


/**
 * itdb_rename_files:
 * @mp: mount point of the iPod
 * @error: return location for a #GError or NULL
 *
 * Renames/removes some files on the iPod (Playcounts, OTG
 * semaphore). May have to be called if you write the iTunesDB not
 * directly to the iPod but to some other location and then manually
 * copy the file from there to the iPod. 
 *
 * Return value: FALSE on error and sets @error accordingly
 **/
gboolean itdb_rename_files (const gchar *mp, GError **error)
{
    const gchar *db_plc_o[] = {"Play Counts", NULL};
    const gchar *db_otg[] = {"OTGPlaylistInfo", NULL};
    const gchar *db_shu[] = {"iTunesShuffle", NULL};
    gchar *itunesdir;
    gchar *plcname_o;
    gchar *plcname_n;
    gchar *otgname;
    gchar *shuname;
    gboolean result = TRUE;

    g_return_val_if_fail (mp, FALSE);

    itunesdir = itdb_get_itunes_dir (mp);

    if(!itunesdir)
    {
	error_no_itunes_dir (mp, error);
	return FALSE;
    }

    plcname_o = itdb_resolve_path (itunesdir, db_plc_o);
    plcname_n = g_build_filename (itunesdir, 
					 "Play Counts.bak", NULL);
    otgname = itdb_resolve_path (itunesdir, db_otg);
    shuname = itdb_resolve_path (itunesdir, db_shu);

    /* rename "Play Counts" to "Play Counts.bak" */
    if (plcname_o)
    {
	if (rename (plcname_o, plcname_n) == -1)
	{   /* an error occured */
	    g_set_error (error,
			 G_FILE_ERROR,
			 g_file_error_from_errno (errno),
			 _("Error renaming '%s' to '%s' (%s)."),
			 plcname_o, plcname_n, g_strerror (errno));
	    result = FALSE;
	}
    }

    /* remove "OTGPlaylistInfo" (the iPod will remove the remaining
     * files */
    if (otgname)
    {
	if (unlink (otgname) == -1)
	{
	    if (error && !*error)
	    {   /* don't overwrite previous error */
	    g_set_error (error,
			 G_FILE_ERROR,
			 g_file_error_from_errno (errno),
			 _("Error removing '%s' (%s)."),
			 otgname, g_strerror (errno));
	    }
	    result = FALSE;
	}
    }

    /* remove some Shuffle files */
    if (shuname)
    {
	if (unlink (shuname) == -1)
	{
	    if (error && !*error)
	    {   /* don't overwrite previous error */
	    g_set_error (error,
			 G_FILE_ERROR,
			 g_file_error_from_errno (errno),
			 _("Error removing '%s' (%s)."),
			 shuname, g_strerror (errno));
	    }
	    result = FALSE;
	}
    }

    g_free (plcname_o);
    g_free (plcname_n);
    g_free (otgname);
    g_free (shuname);
    g_free (itunesdir);

    return result;
}


/**
 * itdb_filename_fs2ipod:
 * @filename: a filename 'PC-style' (eg /iPod_Control/Music/f00/test.mp3)
 *
 * Convert string from casual PC file name to iPod iTunesDB format using ':' 
 * instead of G_DIR_SEPARATOR_S (ie slashes on Unix-like systems). @ipod_file
 * is modified in place.
 **/
void itdb_filename_fs2ipod (gchar *ipod_file)
{
    g_strdelimit (ipod_file, G_DIR_SEPARATOR_S, ':');
}

/**
 * itdb_filename_ipod2fs:
 * @ipod_file: a filename 'PC-style' (eg /iPod_Control/Music/f00/test.mp3)
 *
 * Convert string from from iPod iTunesDB format to casual PC file name
 * using G_DIR_SEPARATOR (ie slashes on Unix-like systems) instead of ':'.
 * @ipod_file is modified in place.
 **/
void itdb_filename_ipod2fs (gchar *ipod_file)
{
    g_strdelimit (ipod_file, ":", G_DIR_SEPARATOR);
}



/**
 * itdb_set_mountpoint:
 * @itdb: an #Itdb_iTunesDB
 * @mp: new mount point
 *
 * Sets the mountpoint of @db. Always use this function to set the mountpoint
 * of an #Itdb_iTunesDB as it will reset the number of available 
 * /iPod_Control/Music/F.. dirs. It doesn't attempt to parse an iPod database
 * that may be present on the iPod at @mp
 **/
void itdb_set_mountpoint (Itdb_iTunesDB *itdb, const gchar *mp)
{
    g_return_if_fail (itdb);
    g_return_if_fail (itdb->device);

    itdb_device_set_mountpoint (itdb->device, mp);
    itdb->device->musicdirs = 0;
}

/**
 * itdb_get_mountpoint:
 * @itdb: an #Itdb_iTunesDB
 *
 * Retrieve a reference to the mountpoint of @itdb
 *
 * Return value: the @itdb mountpoint, this string shouldn't be freed nor 
 * modified
 **/
const gchar *itdb_get_mountpoint (Itdb_iTunesDB *itdb)
{
    g_return_val_if_fail (itdb, NULL);
    g_return_val_if_fail (itdb->device, NULL);
    return itdb->device->mountpoint;
}

/* Retrieve a reference to the mountpoint */
const gchar *itdb_photodb_get_mountpoint (Itdb_PhotoDB *photodb)
{
    g_return_val_if_fail (photodb, NULL);
    g_return_val_if_fail (photodb->device, NULL);
    return photodb->device->mountpoint;
}

/**
 * itdb_musicdirs_number:
 * @itdb: an #Itdb_iTunesDB
 *
 * Determine the number of F.. directories in iPod_Control/Music.
 *
 * If itdb->musicdirs is already set, simply return the previously
 * determined number. Otherwise count the directories first and set
 * itdb->musicdirs. 
 *
 * Return value: max number of directories in iPod_Control/Music
 **/
gint itdb_musicdirs_number (Itdb_iTunesDB *itdb)
{
    g_return_val_if_fail (itdb, 0);
    g_return_val_if_fail (itdb->device, 0);

    return itdb_device_musicdirs_number (itdb->device);
}



/**
 * itdb_cp_track_to_ipod:
 * @track: the #Itdb_Track to copy (containing @filename metadata)
 * @filename: the source file
 * @error: return location for a #GError or NULL
 * 
 * Copy one track to the iPod. The PC filename is @filename
 * and is taken literally.
 *
 * The mountpoint of the iPod (in local encoding) is expected in
 * track-&gt;itdb-&gt;mountpoint.
 *
 * If @track-&gt;transferred is set to TRUE, nothing is done. Upon
 * successful transfer @track-&gt;transferred is set to TRUE.
 *
 * For storage, the directories "f00 ... fnn" will be
 * cycled through.
 *
 * The filename is constructed as "gtkpod"&lt;random number&gt; and copied
 * to @track-&gt;ipod_path. If this file already exists, &lt;random number&gt;
 * is adjusted until an unused filename is found.
 *
 * If @track-&gt;ipod_path is already set, this one will be used
 * instead. If a file with this name already exists, it will be
 * overwritten.
 *
 * @track-&gt;filetype_marker is set according to the filename extension
 *
 * Return value: TRUE on success, FALSE on error, in which case @error is
 * set accordingly.
 **/
gboolean itdb_cp_track_to_ipod (Itdb_Track *track,
				const gchar *filename, GError **error)
{
  static gint dir_num = -1;
  gchar *track_db_path, *ipod_fullfile;
  gboolean success;
  gint mplen = 0;
  gint i;
  const gchar *mountpoint;
  Itdb_iTunesDB *itdb;

  g_return_val_if_fail (track, FALSE);
  g_return_val_if_fail (track->itdb, FALSE);
  g_return_val_if_fail (itdb_get_mountpoint (track->itdb), FALSE);
  g_return_val_if_fail (filename, FALSE);

  if(track->transferred)  return TRUE; /* nothing to do */ 

  mountpoint = itdb_get_mountpoint (track->itdb);
  itdb = track->itdb;

  /* If track->ipod_path exists, we use that one instead. */
  ipod_fullfile = itdb_filename_on_ipod (track);

  if (!ipod_fullfile)
  {
      gchar *dest_components[] = {NULL, NULL, NULL};
      gchar *parent_dir_filename, *music_dir;
      gchar *original_suffix;
      gchar dir_num_str[5];
      gint32 oops = 0;
      gint32 rand = g_random_int_range (0, 899999); /* 0 to 900000 */

      music_dir = itdb_get_music_dir (mountpoint);
      if (!music_dir)
      {
	  error_no_music_dir (mountpoint, error);
	  return FALSE;
      }

      if (itdb_musicdirs_number (itdb) <= 0)
      {
	  g_set_error (error,
		       ITDB_FILE_ERROR,
		       ITDB_FILE_ERROR_NOTFOUND,
		       _("No 'F..' directories found in '%s'."),
		       music_dir);
	  g_free (music_dir);
	  return FALSE;
      }

      if (dir_num == -1)
      {
	  dir_num = g_random_int_range (0, itdb_musicdirs_number (itdb));
      }
      else
      {
	  dir_num = (dir_num + 1) % itdb_musicdirs_number (itdb);
      }

      g_snprintf (dir_num_str, 5, "F%02d", dir_num);
      dest_components[0] = dir_num_str;
  
      parent_dir_filename =
	  itdb_resolve_path (music_dir, (const gchar **)dest_components);
      if(parent_dir_filename == NULL)
      {
	  /* Can't find the F%02d directory */
	  gchar *str = g_build_filename (music_dir,
					 dest_components[0], NULL);
	  g_set_error (error,
		       ITDB_FILE_ERROR,
		       ITDB_FILE_ERROR_NOTFOUND,
		       _("Path not found: '%s'."),
		       str);
	  g_free (str);
	  g_free (music_dir);
	  return FALSE;
      }

      /* we need the original suffix of pcfile to construct a correct ipod
	 filename */
      original_suffix = strrchr (filename, '.');
      /* If there is no ".mp3", ".m4a" etc, set original_suffix to empty
	 string. Note: the iPod will most certainly ignore this file... */
      if (!original_suffix) original_suffix = "";

      /* use lower-case version of extension as some iPods seem to
	 choke on upper-case extension. */
      original_suffix = g_ascii_strdown (original_suffix, -1);

      /* set filetype from the suffix, e.g. '.mp3' -> 'MP3 ' */
      track->filetype_marker = 0;
      for (i=1; i<=4; ++i)   /* start with i=1 to skip the '.' */
      {
	  track->filetype_marker = track->filetype_marker << 8;
	  if (strlen (original_suffix) > i)
	      track->filetype_marker |= g_ascii_toupper (original_suffix[i]);
	  else
	      track->filetype_marker |= ' ';
      }
      do
      {   /* we need to loop until we find an unused filename */
	  dest_components[1] = 
	      g_strdup_printf("gtkpod%06d%s",
			      rand + oops, original_suffix);
	  ipod_fullfile = itdb_resolve_path (
	      parent_dir_filename,
	      (const gchar **)&dest_components[1]);
	  if(ipod_fullfile)
	  {   /* already exists -- try next */
              g_free(ipod_fullfile);
              ipod_fullfile = NULL;
	  }
	  else
	  {   /* found unused file -- build filename */
	      ipod_fullfile = g_build_filename (parent_dir_filename,
					        dest_components[1], NULL);
	  }
	  g_free (dest_components[1]);
	  ++oops;
      } while (!ipod_fullfile);
      g_free(parent_dir_filename);
      g_free (music_dir);
      g_free (original_suffix);
  }
  /* now extract filepath for track->ipod_path from ipod_fullfile */
  /* ipod_path must begin with a '/' */
  mplen = strlen (mountpoint); /* length of mountpoint in bytes */
  if (ipod_fullfile[mplen] == G_DIR_SEPARATOR)
  {
      track_db_path = g_strdup (&ipod_fullfile[mplen]);
  }
  else
  {
      track_db_path = g_strdup_printf ("%c%s", G_DIR_SEPARATOR,
				       &ipod_fullfile[mplen]);
  }
  /* convert to iPod type */
  itdb_filename_fs2ipod (track_db_path);
  
/* 	printf ("ff: %s\ndb: %s\n", ipod_fullfile, track_db_path); */

  success = itdb_cp (filename, ipod_fullfile, error);
  if (success)
  {
      track->transferred = TRUE;
      g_free (track->ipod_path);
      track->ipod_path = g_strdup (track_db_path);
  }

  g_free (track_db_path);
  g_free (ipod_fullfile);
  return success;
}


/**
 * itdb_filename_on_ipod:
 * @track: an #Itdb_Track
 *
 * Return the full iPod filename as stored in @track.
 *
 * NOTE: NULL is returned when the file does not exist.
 *
 * NOTE: this code works around a problem on some systems (see
 * itdb_resolve_path() ) and might return a filename with different
 * case than the original filename. Don't copy it back to @track
 * unless you must 
 *
 * Return value: full filename to @track on the iPod or NULL if no
 * filename is set in @track. Must be freed with g_free() after use.
 **/
gchar *itdb_filename_on_ipod (Itdb_Track *track)
{
    gchar *result = NULL;
    const gchar *mp;

    g_return_val_if_fail (track, NULL);
    g_return_val_if_fail (track->itdb, NULL);

    if (!itdb_get_mountpoint (track->itdb)) return NULL;

    mp = itdb_get_mountpoint (track->itdb);

    if(track->ipod_path && *track->ipod_path)
    {
	gchar *buf = g_strdup (track->ipod_path);
	itdb_filename_ipod2fs (buf);
	result = g_build_filename (mp, buf, NULL);
	g_free (buf);
	if (!g_file_test (result, G_FILE_TEST_EXISTS))
	{
	    gchar **components = g_strsplit (track->ipod_path,":",10);
	    g_free (result);
	    result = itdb_resolve_path (mp, (const gchar **)components);
	    g_strfreev (components);
	}
    }
    return result;
}


/**
 * itdb_cp:
 * @from_file: source file
 * @to_file: destination file
 * @error: return location for a #GError or NULL
 *
 * Copy file "from_file" to "to_file".
 *
 * Return value: TRUE on success, FALSE on error, in which case @error is
 * set accordingly.
 **/
gboolean itdb_cp (const gchar *from_file, const gchar *to_file,
		  GError **error)
{
    gchar *data;
    glong bread, bwrite;
    FILE *file_in = NULL;
    FILE *file_out = NULL;

#if ITUNESDB_DEBUG
    fprintf(stderr, "Entered itunesdb_cp: '%s', '%s'\n", from_file, to_file);
#endif

    g_return_val_if_fail (from_file, FALSE);
    g_return_val_if_fail (to_file, FALSE);

    data = g_malloc (ITUNESDB_COPYBLK);

    file_in = fopen (from_file, "r");
    if (file_in == NULL)
    {
	g_set_error (error,
		     G_FILE_ERROR,
		     g_file_error_from_errno (errno),
		     _("Error opening '%s' for reading (%s)."),
		     from_file, g_strerror (errno));
	goto err_out;
    }

    file_out = fopen (to_file, "w");
    if (file_out == NULL)
    {
	g_set_error (error,
		     G_FILE_ERROR,
		     g_file_error_from_errno (errno),
		     _("Error opening '%s' for writing (%s)."),
		     to_file, g_strerror (errno));
	goto err_out;
    }

    do {
	bread = fread (data, 1, ITUNESDB_COPYBLK, file_in);
#if ITUNESDB_DEBUG
	fprintf(stderr, "itunesdb_cp: read %ld bytes\n", bread);
#endif
	if (bread == 0)
	{
	    if (feof (file_in) == 0)
	    {   /* error -- not end of file! */
		g_set_error (error,
			     G_FILE_ERROR,
			     g_file_error_from_errno (errno),
			     _("Error while reading from '%s' (%s)."),
			     from_file, g_strerror (errno));
		goto err_out;
	    }
	}
	else
	{
	    bwrite = fwrite (data, 1, bread, file_out);
#if ITUNESDB_DEBUG
	    fprintf(stderr, "itunesdb_cp: wrote %ld bytes\n", bwrite);
#endif
	    if (bwrite != bread)
	    {
		g_set_error (error,
			     G_FILE_ERROR,
			     g_file_error_from_errno (errno),
			     _("Error while writing to '%s' (%s)."),
			     to_file, g_strerror (errno));
		goto err_out;
	    }
	}
    } while (bread != 0);

    if (fclose (file_in) != 0)
    {
	file_in = NULL;
	g_set_error (error,
		     G_FILE_ERROR,
		     g_file_error_from_errno (errno),
		     _("Error when closing '%s' (%s)."),
		     from_file, g_strerror (errno));
	goto err_out;
    }
    if (fclose (file_out) != 0)
    {
	file_out = NULL;
	g_set_error (error,
		     G_FILE_ERROR,
		     g_file_error_from_errno (errno),
		     _("Error when closing '%s' (%s)."),
		     to_file, g_strerror (errno));
	goto err_out;
    }
    g_free (data);
    return TRUE;

  err_out:
    if (file_in)  fclose (file_in);
    if (file_out) fclose (file_out);
    remove (to_file);
    g_free (data);
    return FALSE;
}




/**
 * itdb_get_control_dir:
 * @mountpoint: the iPod mountpoint
 *
 * Get the i*_Control directory. Observed values are 'iPod_Control'
 * for standard iPods and 'iTunes/iTunes_Control' for mobile
 * applications.
 *
 * Return value: path to the control dir or NULL of non-existent. Must
 * g_free() after use.
 **/
gchar *itdb_get_control_dir (const gchar *mountpoint)
{
    gchar *p_ipod[] = {"iPod_Control", NULL};
    gchar *p_mobile[] = {"iTunes", "iTunes_Control", NULL};
    /* Use an array with all possibilities, so further extension will
       be easy */
    gchar **paths[] = {p_ipod, p_mobile, NULL};
    gchar ***ptr;
    gchar *result = NULL;

    g_return_val_if_fail (mountpoint, NULL);

    for (ptr=paths; *ptr && !result; ++ptr)
    {
	result = itdb_resolve_path (mountpoint, (const gchar **)*ptr);
    }
    return result;
}

/**
 * itdb_get_dir:
 * @mountpoint: the iPod mountpoint
 * @dir: a directory
 *
 * Retrieve the directory @dir by first calling itdb_get_control_dir()
 * and then adding @dir
 *
 * Return value: path to @dir or NULL if non-existent. Must g_free()
 * after use.
 **/
static gchar *itdb_get_dir (const gchar *mountpoint, const gchar *dir)
{
    gchar *control_dir;
    gchar *result = NULL;

    g_return_val_if_fail (mountpoint, NULL);
    g_return_val_if_fail (dir, NULL);

    control_dir = itdb_get_control_dir (mountpoint);
    if (control_dir)
    {
	const gchar *p_dir[] = {NULL, NULL};
	p_dir[0] = dir;
	result = itdb_resolve_path (control_dir, p_dir);
	g_free (control_dir);
    }
    return result;
}

/**
 * itdb_get_path:
 * @dir: a directory
 * @file: a file
 *
 * Retrieve a path to the @file in @dir
 *
 * Return value: path to the @file or NULL if non-existent. Must be g_free()'d 
 * after use.
 **/
gchar *itdb_get_path (const gchar *dir, const gchar *file)
{
    const gchar *p_file[] = {NULL, NULL};

    g_return_val_if_fail (dir, NULL);

    p_file[0] = file;

    return itdb_resolve_path (dir, p_file);
}

/**
 * itdb_get_itunes_dir:
 * @mountpoint: the iPod mountpoint
 *
 * Retrieve the iTunes directory (containing the iTunesDB) by first
 * calling itdb_get_control_dir() and then adding 'iTunes'
 *
 * Return value: path to the iTunes directory or NULL of non-existent.
 * Must g_free() after use.
 **/
gchar *itdb_get_itunes_dir (const gchar *mountpoint)
{
    g_return_val_if_fail (mountpoint, NULL);

    return itdb_get_dir (mountpoint, "iTunes");
}

/**
 * itdb_get_music_dir:
 * @mountpoint: the iPod mountpoint
 *
 * Retrieve the Music directory (containing the Fnn dirs) by first
 * calling itdb_get_control_dir() and then adding 'Music'
 *
 * Return value: path to the Music directory or NULL of
 * non-existent. Must g_free() after use.
 **/
gchar *itdb_get_music_dir (const gchar *mountpoint)
{
    g_return_val_if_fail (mountpoint, NULL);

    return itdb_get_dir (mountpoint, "Music");
}

/**
 * itdb_get_device_dir:
 * @mountpoint: the iPod mountpoint
 *
 * Retrieve the Device directory (containing the SysInfo file) by
 * first calling itdb_get_control_dir() and then adding 'Device'
 *
 * Return value: path to the Device directory or NULL of
 * non-existent. Must g_free() after use.
 **/
gchar *itdb_get_device_dir (const gchar *mountpoint)
{
    g_return_val_if_fail (mountpoint, NULL);

    return itdb_get_dir (mountpoint, "Device");
}

/**
 * itdb_get_artwork_dir:
 * @mountpoint: the iPod mountpoint
 *
 * Retrieve the Artwork directory (containing the ArtworDB) by
 * first calling itdb_get_control_dir() and then adding 'Artwork'
 *
 * Return value: path to the Artwork directory or NULL of
 * non-existent. Must g_free() after use.
 **/
gchar *itdb_get_artwork_dir (const gchar *mountpoint)
{
    g_return_val_if_fail (mountpoint, NULL);

    return itdb_get_dir (mountpoint, "Artwork");
}

/**
 * itdb_get_itunesdb_path:
 * @mountpoint: the iPod mountpoint
 *
 * Retrieve a path to the iTunesDB
 *
 * Return value: path to the iTunesDB or NULL if non-existent. Must g_free()
 * after use.
 **/
gchar *itdb_get_itunesdb_path (const gchar *mountpoint)
{
    gchar *itunes_dir, *path=NULL;

    g_return_val_if_fail (mountpoint, NULL);

    itunes_dir = itdb_get_itunes_dir (mountpoint);

    if (itunes_dir)
    {
	path = itdb_get_path (itunes_dir, "iTunesDB");
	g_free (itunes_dir);
    }

    return path;
}

/**
 * itdb_get_itunessd_path:
 * @mountpoint: the iPod mountpoint
 *
 * Retrieve a path to the iTunesSD
 *
 * Return value: path to the iTunesSD or NULL if non-existent. Must g_free()
 * after use.
 **/
gchar *itdb_get_itunessd_path (const gchar *mountpoint)
{
    gchar *itunes_dir, *path=NULL;

    g_return_val_if_fail (mountpoint, NULL);

    itunes_dir = itdb_get_itunes_dir (mountpoint);

    if (itunes_dir)
    {
	path = itdb_get_path (itunes_dir, "iTunesSD");
	g_free (itunes_dir);
    }

    return path;
}

/**
 * itdb_get_artworkdb_path:
 * @mountpoint: the iPod mountpoint
 *
 * Retrieve a path to the ArtworkDB
 *
 * Return value: path to the ArtworkDB or NULL if non-existent. Must g_free() 
 * after use.
 **/
gchar *itdb_get_artworkdb_path (const gchar *mountpoint)
{
    gchar *itunes_dir, *path=NULL;

    g_return_val_if_fail (mountpoint, NULL);

    itunes_dir = itdb_get_artwork_dir (mountpoint);

    if (itunes_dir)
    {
	path = itdb_get_path (itunes_dir, "ArtworkDB");
	g_free (itunes_dir);
    }

    return path;
}


/*------------------------------------------------------------------*\
 *                                                                  *
 *                       Timestamp stuff                            *
 *                                                                  *
\*------------------------------------------------------------------*/

/**
 * itdb_time_get_mac_time:
 *
 * Gets the current time expressed in 'Mac' unit (ie in number of seconds since
 * 1/1/1904).
 * 
 * Return value: current time in 'Mac' unit.
 **/
guint64 itdb_time_get_mac_time (void)
{
    GTimeVal time;

    g_get_current_time (&time);
    return itdb_time_host_to_mac (time.tv_sec);
}


/**
 * itdb_time_mac_to_host:
 * @mactime: time expressed in 'Mac' unit
 *
 * Convert a Mac timestamp to host system time stamp -- modify
 * this function if necessary to port to host systems with different
 * start of Epoch.
 * A "0" time will not be converted.
 *
 * Return value: timestamp for the host system
 **/
time_t itdb_time_mac_to_host (guint64 mactime)
{
    if (mactime != 0)  return (time_t)(mactime - 2082844800);
    else               return (time_t)mactime;
}


/**
 * itdb_time_host_to_mac:
 * @time: time expressed in host unit
 *
 * Convert host system timestamp to Mac time stamp -- modify
 * this function if necessary to port to host systems with different
 * start of Epoch 
 *
 * Return value: a Mac timestamp
 **/
guint64 itdb_time_host_to_mac (time_t time)
{
    return (guint64)(((gint64)time) + 2082844800);
}


/**
 * itdb_init_ipod:
 * @mountpoint:   the iPod mountpoint
 * @model_number: the iPod model number
 * @ipod_name:    the name to give to the iPod. Will be displayed in
 *                gtkpod or itunes
 * @error:        return location for a #GError or NULL
 *
 * Initialise an iPod device from scratch. The function attempts to
 * create a blank database, complete with master playlist and device
 * information as well as the directory structure required for the
 * type of iPod.
 * 
 * Return value: TRUE when successful, FALSE if a failure has occurred.
 * 
 **/
gboolean itdb_init_ipod (const gchar *mountpoint,
			 const gchar *model_number,
			 const gchar *ipod_name,
			 GError **error)
{
	gboolean writeok;
	Itdb_iTunesDB *itdb = NULL;
	Itdb_Playlist *mpl = NULL;
	Itdb_IpodInfo const *info = NULL;
	gchar *path;
	
	g_return_val_if_fail (mountpoint, FALSE);
						
	/* Create new blank itdb database for writing to iPod */
	itdb = itdb_new();
					
	/* Assign iPod device reference to new database */
	itdb_set_mountpoint(itdb, mountpoint);
	
	/* Insert model_number into sysinfo file if present
	 * The model number can be extracted in a couple of ways:
	 *		- use the read_sysinfo_file function
	 * 		- use libipoddevice and hal to get the model
	 *                (as far as I know, libipoddevice will also
         *                read the sysinfo file, complemented by some
	 *	          guessing).
	 */
	if (model_number)
	{
	    itdb_device_set_sysinfo (itdb->device,
				     "ModelNumStr", model_number);
	}
	
	/* Create the remaining directories resident on blank ipod */
	writeok = itdb_create_directories(itdb->device, error);
	if(! writeok)
	{
		return FALSE;
	}

	/* Create a new playlist with the desired name of the ipod
	 * and set it as the mpl */
	if (ipod_name == NULL)
	{
	    mpl = itdb_playlist_new(_("iPod"), FALSE);
	}
	else
	{
	    mpl = itdb_playlist_new(ipod_name, FALSE);
	}
	itdb_playlist_set_mpl(mpl);
	itdb_playlist_add(itdb, mpl, -1);
	
	/* Write both the iTunesDB and iTunesSD files to the new ipod,
	 * unless they already exist */
	path = itdb_get_itunesdb_path (mountpoint);
	if (!path)
	{
	    writeok = itdb_write(itdb, error);
	    if(! writeok)
	    {
	        itdb_free (itdb);
		return FALSE;
	    }
	}
	g_free (path);

	/* Retrieve the model from the device information */
	info = itdb_device_get_ipod_info(itdb->device);
    
	/* If model is a shuffle or the model is undetermined,
	 * ie. @model_number is NULL, then create the itunesSD database
	 */
	if(!model_number || info->ipod_model == ITDB_IPOD_MODEL_SHUFFLE)
	{
	    path = itdb_get_itunessd_path (mountpoint);
	    if (!path)
	    {
	    	writeok = itdb_shuffle_write(itdb, error);
	    	if(! writeok)
	    	{
	    	    itdb_free (itdb);
		    return FALSE;
	    	}
	    }
	    g_free (path);
	}
	itdb_free (itdb);
	return TRUE;
}



/*------------------------------------------------------------------*\
 *                                                                  *
 *             Some functions to access Itdb_DB safely              *
 *                                                                  *
\*------------------------------------------------------------------*/
G_GNUC_INTERNAL
Itdb_iTunesDB *db_get_itunesdb (Itdb_DB *db)
{
    g_return_val_if_fail (db, NULL);
    g_return_val_if_fail (db->db_type == DB_TYPE_ITUNES, NULL);

    return db->db.itdb;
}

G_GNUC_INTERNAL
Itdb_PhotoDB *db_get_photodb (Itdb_DB *db)
{
    g_return_val_if_fail (db, NULL);
    g_return_val_if_fail (db->db_type == DB_TYPE_PHOTO, NULL);

    return db->db.photodb;
}

G_GNUC_INTERNAL
Itdb_Device *db_get_device(Itdb_DB *db)
{
    g_return_val_if_fail (db, NULL);

    switch (db->db_type) {
    case DB_TYPE_ITUNES:
	g_return_val_if_fail (db_get_itunesdb(db), NULL);
	return db_get_itunesdb(db)->device;
    case DB_TYPE_PHOTO:
	g_return_val_if_fail (db_get_photodb(db), NULL);
	return db_get_photodb(db)->device;
    }
    g_return_val_if_reached (NULL);
}

G_GNUC_INTERNAL
gchar *db_get_mountpoint(Itdb_DB *db)
{
    Itdb_Device *device;
    g_return_val_if_fail (db, NULL);

    device = db_get_device (db);
    g_return_val_if_fail (device, NULL);

    return device->mountpoint;
}



/*------------------------------------------------------------------*\
 *                                                                  *
 *             Create iPod directory hierarchy                      *
 *                                                                  *
\*------------------------------------------------------------------*/

/* mkdir_with_parents is copied from GLIB2.8 (gfileutils.c, V1.78), as
 * it is new to V2.8. May be replaced with g_mkdir_with_parents() in a
 * couple of years. */

/**
 * mkdir_with_parents:
 * @pathname: a pathname in the GLib file name encoding
 * @mode: permissions to use for newly created directories
 *
 * Create a directory if it doesn't already exist. Create intermediate
 * parent directories as needed, too.
 *
 * Returns: 0 if the directory already exists, or was successfully
 * created. Returns -1 if an error occurred, with errno set.
 *
 */
static int
mkdir_with_parents (const gchar *pathname,
		      int          mode)
{
  gchar *fn, *p;

  if (pathname == NULL || *pathname == '\0')
    {
      errno = EINVAL;
      return -1;
    }

  fn = g_strdup (pathname);

  if (g_path_is_absolute (fn))
    p = (gchar *) g_path_skip_root (fn);
  else
    p = fn;

  do
    {
      while (*p && !G_IS_DIR_SEPARATOR (*p))
	p++;
      
      if (!*p)
	p = NULL;
      else
	*p = '\0';
      
      if (!g_file_test (fn, G_FILE_TEST_EXISTS))
	{
	  if (g_mkdir (fn, mode) == -1)
	    {
	      int errno_save = errno;
	      g_free (fn);
	      errno = errno_save;
	      return -1;
	    }
	}
      else if (!g_file_test (fn, G_FILE_TEST_IS_DIR))
	{
	  g_free (fn);
	  errno = ENOTDIR;
	  return -1;
	}
      if (p)
	{
	  *p++ = G_DIR_SEPARATOR;
	  while (*p && G_IS_DIR_SEPARATOR (*p))
	    p++;
	}
    }
  while (p);

  g_free (fn);

  return 0;
}

	
static gboolean itdb_create_directories (Itdb_Device *device, GError **error)
{
    const gchar *mp;
    gboolean result;
    gchar *pbuf;
    gint i, dirnum;
    Itdb_IpodInfo const *info = NULL;
    gboolean calconnotes, devicefile;
    gchar *podpath;

    g_return_val_if_fail (device, FALSE);

    mp = device->mountpoint;
    g_return_val_if_fail (mp, FALSE);

    /* Retrieve the model from the device information */
    info = itdb_device_get_ipod_info(device);

    /* Set up special treatment for shuffle and mobile */
    switch(info->ipod_model)
    {
    case ITDB_IPOD_MODEL_SHUFFLE:
	podpath = g_strdup ("iPod_Control");
	calconnotes = FALSE;
	devicefile = TRUE;
	break;
    case ITDB_IPOD_MODEL_MOBILE_1:
	podpath = g_build_filename ("iTunes", "iTunes_Control", NULL);
	calconnotes = FALSE;
	devicefile = TRUE;
	break;
    case ITDB_IPOD_MODEL_UNKNOWN:
	podpath = g_strdup ("iPod_Control");
	calconnotes = TRUE;
	devicefile = TRUE;
	break;
    default:
	podpath = g_strdup ("iPod_Control");
	calconnotes = TRUE;
	devicefile = TRUE;
	break;
    }

    /* Construct the Control directory */
    pbuf = g_build_filename (mp, podpath, NULL);
    if (!g_file_test (pbuf, G_FILE_TEST_EXISTS))
    {
	if (mkdir_with_parents(pbuf, 0777) != 0)
	{
	    goto error_dir;
	}
    }
    g_free (pbuf);

    /* Construct the Music directory inside the Control directory */
    pbuf = g_build_filename (mp, podpath, "Music", NULL);
    if (!g_file_test (pbuf, G_FILE_TEST_EXISTS))
    {
	if((mkdir(pbuf, 0777) != 0))
	{
	    goto error_dir;
	}
    }
    g_free (pbuf);

    /* Construct the iTunes directory inside the Control directory */
    pbuf = g_build_filename (mp, podpath, "iTunes", NULL);
    if (!g_file_test (pbuf, G_FILE_TEST_EXISTS))
    {
	if((mkdir(pbuf, 0777) != 0))
	{
	    goto error_dir;
	}
    }
    g_free (pbuf);

    /* Build Artwork directory only for devices requiring artwork
     * (assume that 'unknown models' are new and will support
     * artwork) */
    if (ipod_supports_cover_art(device) ||
	(info->ipod_model == ITDB_IPOD_MODEL_UNKNOWN))
    {
	pbuf = g_build_filename (mp, podpath, "Artwork", NULL);
	if (!g_file_test (pbuf, G_FILE_TEST_EXISTS))
	{
	    if((mkdir(pbuf, 0777) != 0)) {
		goto error_dir;
	    }
	}
	g_free (pbuf);
    }

    /* Build Photo directory only for devices requiring it */
    if (ipod_supports_photos(device) ||
	(info->ipod_model == ITDB_IPOD_MODEL_UNKNOWN))
    {
	pbuf = g_build_filename (mp, "Photos", "Thumbs", NULL);
	if (!g_file_test (pbuf, G_FILE_TEST_EXISTS))
	{
	    if (mkdir_with_parents(pbuf, 0777) != 0)
	    {
		goto error_dir;
	    }
	}
	g_free (pbuf);
    }
    

    /* Build the directories that hold the music files */
    dirnum = info->musicdirs;
    if (dirnum == 0)
    {   /* do a guess */
	struct statvfs stat;
	if (statvfs (mp, &stat) != 0)
	{   /* why should this fail !? */
	    dirnum = 20;
	}
	else
	{
	    gdouble size = ((gdouble)stat.f_blocks * stat.f_frsize) / 1073741824;
	    if (size < 20)  dirnum = 20;
	    else            dirnum = 50;
	}
    }

    for(i = 0; i < dirnum; i++)
    {
	gchar *num = g_strdup_printf ("F%02d", i);
	pbuf = g_build_filename (mp, podpath, "Music", num, NULL);
	g_free (num);
	if (!g_file_test (pbuf, G_FILE_TEST_EXISTS))
	{
	    if((mkdir(pbuf, 0777) != 0))
	    {
		goto error_dir;
	    }
	}
	g_free (pbuf);
    }

    /* Build Calendar directory for models requiring it */
    if (calconnotes)
    {
	pbuf = g_build_filename (mp, "Calendars", NULL);
	if (!g_file_test (pbuf, G_FILE_TEST_EXISTS))
	{
	    if((mkdir(pbuf, 0777) != 0))
	    {
		goto error_dir;
	    }
	}
	g_free (pbuf);

	/* Build Contacts directory for models requiring it */
	pbuf = g_build_filename (mp, "Contacts", NULL);
	if (!g_file_test (pbuf, G_FILE_TEST_EXISTS))
	{
	    if((mkdir(pbuf, 0777) != 0))
	    {
		goto error_dir;
	    }
	}
	g_free (pbuf);

	pbuf = g_build_filename (mp, "Notes", NULL);
	if (!g_file_test (pbuf, G_FILE_TEST_EXISTS))
	{
	    if((mkdir(pbuf, 0777) != 0))
	    {
		goto error_dir;
	    }
	}
	g_free (pbuf);
    }

    /* Construct a Device directory file for special models */
    if (devicefile)
    {
	gchar *model_number;

	pbuf = g_build_filename (mp, podpath, "Device", NULL);
	if (!g_file_test (pbuf, G_FILE_TEST_EXISTS))
	{
	    if((mkdir(pbuf, 0777) != 0))
	    {
		goto error_dir;
	    }
   	}
	g_free (pbuf);

	model_number = itdb_device_get_sysinfo (device, "ModelNumStr");
   	/* Construct a SysInfo file */
	if (model_number && (strlen (model_number) != 0))
	{
	    pbuf = NULL;
	    if (!itdb_device_write_sysinfo (device, error))
	    {
		g_free (model_number);
		goto error_dir;
	    }
	}
	g_free (model_number);
    }
    pbuf = NULL;

  error_dir:
    if (pbuf)
    {
	g_set_error (error, 0, -1,
		     _("Problem creating iPod directory or file: '%s'."), 
		     pbuf);
	result = FALSE;
    } else
    {
	result = TRUE;
    }
    g_free (pbuf);
    g_free (podpath);
    return result;
}

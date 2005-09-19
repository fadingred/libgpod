/* Time-stamp: <2005-09-19 21:31:28 jcs>
|
|  Copyright (C) 2002-2005 Jorg Schuler <jcsjcs at users sourceforge net>
|  Part of the gtkpod project.
| 
|  URL: http://www.gtkpod.org/
|  URL: http://gtkpod.sourceforge.net/
|
|  Much of the code in this file has originally been ported from the
|  perl script "mktunes.pl" (part of the gnupod-tools collection)
|  written by Adrian Ulrich <pab at blinkenlights.ch>.
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

#include <time.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include "itdb_private.h"
#include "db-artwork-parser.h"
#include <glib/gi18n-lib.h>

#define ITUNESDB_DEBUG 0
#define ITUNESDB_MHIT_DEBUG 0

#define ITUNESDB_COPYBLK 262144      /* blocksize for cp () */


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
/*  MHOD_ID_CHAPTERDATA = 17,*/
/* Subtitle (usually the same as Description). Introduced in db
   version 0x0d. */
  MHOD_ID_SUBTITLE = 18,
  MHOD_ID_SPLPREF = 50,  /* settings for smart playlist */
  MHOD_ID_SPLRULES = 51, /* rules for smart playlist     */
  MHOD_ID_MHYP = 52,     /* unknown                     */
  MHOD_ID_PLAYLIST = 100
};


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
     
/* We start by assuming that the ipod mount point exists.  Then, for
 * each component c of track->ipod_path, we try to find an entry d in
 * good_path that is case-insensitively equal to c.  If we find d, we
 * append d to good_path and make the result the new good_path.
 * Otherwise, we quit and return NULL.  @root: in local encoding,
 * @components: in utf8 */
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
static gboolean cmp_n_bytes_seek (FContents *cts, gchar *data,
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

/* Get the 2-byte-number stored at position "seek" in little endian
   encoding. On error the GError in @cts is set. */
static guint16 get16lint (FContents *cts, glong seek)
{
    guint32 n=0;

    if (check_seek (cts, seek, 2))
    {
	g_return_val_if_fail (cts->contents, 0);
	memcpy (&n, &cts->contents[seek], 4);
#       if (G_BYTE_ORDER == G_BIG_ENDIAN)
	  n = GUINT16_SWAP_LE_BE (n);
#       endif
    }
    return n;
}

/* Get the 3-byte-number stored at position "seek" in little endian
   encoding. On error the GError in @cts is set. */
static guint32 get24lint (FContents *cts, glong seek)
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
static guint32 get32lint (FContents *cts, glong seek)
{
    guint32 n=0;

    if (check_seek (cts, seek, 4))
    {
	g_return_val_if_fail (cts->contents, 0);
	memcpy (&n, &cts->contents[seek], 4);
#       if (G_BYTE_ORDER == G_BIG_ENDIAN)
	  n = GUINT32_SWAP_LE_BE (n);
#       endif
    }
    return n;
}

/* Get 4 byte floating number */
static float get32lfloat (FContents *cts, glong seek)
{
    union
    {
	guint32 i;
	float   f;
    } flt;

    g_return_val_if_fail (sizeof (float) == 4, 0);

    flt.i = get32lint (cts, seek);

    return flt.f;
}


/* Get the 4-byte-number stored at position "seek" in big endian
   encoding. On error the GError in @cts is set. */
static guint32 get32bint (FContents *cts, glong seek)
{
    guint32 n=0;

    if (check_seek (cts, seek, 4))
    {
	g_return_val_if_fail (cts->contents, 0);
	memcpy (&n, &cts->contents[seek], 4);
#       if (G_BYTE_ORDER == G_LITTLE_ENDIAN)
	  n = GUINT32_SWAP_LE_BE (n);
#       endif
    }
    return n;
}

/* Get the 8-byte-number stored at position "seek" in little endian
   encoding. On error the GError in @cts is set. */
static guint64 get64lint (FContents *cts, glong seek)
{
    guint64 n=0;

    if (check_seek (cts, seek, 8))
    {
	g_return_val_if_fail (cts->contents, 0);
	memcpy (&n, &cts->contents[seek], 8);
#       if (G_BYTE_ORDER == G_BIG_ENDIAN)
	  n = GUINT64_SWAP_LE_BE (n);
#       endif
    }
    return n;
}


/* Get the 8-byte-number stored at position "seek" in big endian
   encoding. On error the GError in @cts is set. */
static guint64 get64bint (FContents *cts, glong seek)
{
    guint64 n=0;

    if (check_seek (cts, seek, 8))
    {
	g_return_val_if_fail (cts->contents, 0);
	memcpy (&n, &cts->contents[seek], 8);
#       if (G_BYTE_ORDER == G_LITTLE_ENDIAN)
	  n = GUINT64_SWAP_LE_BE (n);
#       endif
    }
    return n;
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

    if (!cmp_n_bytes_seek (cts, "mhdp", 0, 4))
    {
	if (cts->error)
	{
	    g_propagate_error (&fimp->error, cts->error);
	}
	else
	{   /* set error */
	    g_return_val_if_fail (cts->filename, FALSE);
	    g_set_error (&fimp->error,
			 ITDB_FILE_ERROR,
			 ITDB_FILE_ERROR_CORRUPT,
			 _("Not a Play Counts file: '%s' (missing mhdp header)."),
			 cts->filename);
	}
	return FALSE;
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
     * (firmware 2.0) or 0x14 (iTunesDB version 0x0d) in length */
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

	fimp->playcounts = g_list_append (fimp->playcounts, playcount);
	playcount->playcount = get32lint (cts, seek);
	CHECK_ERROR (fimp, FALSE);
	playcount->time_played = get32lint (cts, seek+4);
	CHECK_ERROR (fimp, FALSE);
	playcount->bookmark_time = get32lint (cts, seek+8);
	CHECK_ERROR (fimp, FALSE);
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
	    CHECK_ERROR (fimp, FALSE);
	}
	else
	{
	    playcount->rating = NO_PLAYCOUNT;
	}
	/* unk16 only exists if the entry length is at least 0x14 */
	if (entry_length >= 0x14)
	{
	    playcount->pc_unk16 = get32lint (cts, seek+16);
	    CHECK_ERROR (fimp, FALSE);
	}
	else
	{
	    playcount->pc_unk16 = 0;
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
	if (entry_length < 0x18)
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
	if (fimp->itunesdb)  fcontents_free (fimp->itunesdb);
	g_list_free (fimp->pos_glist);
	playcounts_free (fimp);
	g_free (fimp);
    }
}

/* Free the memory taken by @itdb. */
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
	g_free (itdb->mountpoint);
	if (itdb->userdata && itdb->userdata_destroy)
	    (*itdb->userdata_destroy) (itdb->userdata);
	g_free (itdb);
    }
}

/* Free the memory taken by @itdb. */
Itdb_iTunesDB *itdb_duplicate (Itdb_iTunesDB *itdb)
{
    g_return_val_if_fail (itdb, NULL);
    g_return_val_if_fail (!itdb->userdata ||
			  itdb->userdata_duplicate, NULL);
    /* FIXME: not yet implemented */
    g_return_val_if_reached (NULL);
}

/* return number of playlists */
guint32 itdb_playlists_number (Itdb_iTunesDB *itdb)
{
    g_return_val_if_fail (itdb, 0);

    return g_list_length (itdb->playlists);
}


/* return total number of tracks */
guint32 itdb_tracks_number (Itdb_iTunesDB *itdb)
{
    g_return_val_if_fail (itdb, 0);

    return g_list_length (itdb->tracks);
}


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



/* Creates a new Itdb_iTunesDB with the unknowns filled in to reasonable
   values */
Itdb_iTunesDB *itdb_new (void)
{
    GRand *grand = g_rand_new ();

    Itdb_iTunesDB *itdb = g_new0 (Itdb_iTunesDB, 1);
    itdb->version = 0x09;
    itdb->id = ((guint64)g_rand_int (grand) << 32) |
	((guint64)g_rand_int (grand));
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

    if (cmp_n_bytes_seek (cts, "mhod", seek, 4))
    {
	guint32 len = get32lint (cts, seek+8);    /* total length */
	if (cts->error) return -1;
	if (ml) *ml = len;
	type = get32lint (cts, seek+12);          /* mhod_id      */
	if (cts->error) return -1;
    }
    return type;
}

/* Returns a pointer to the data contained in the mhod at position
   @seek. This can be a simple string or something more complicated as
   in the case for SPLPREF or SPLRULES. *ml is set to the total length
   of the mhod (-1 in case of an error), *mty is set to the type of
   the mhod.
   On error NULL is returned and cts->error is set appropriately. */
static void *get_mhod (FContents *cts, gulong mhod_seek,
		       guint32 *ml, gint32 *mty)
{
  gunichar2 *entry_utf16 = NULL;
  SPLPref *splp = NULL;
  guint8 limitsort_opposite;
  void *result = NULL;
  gint32 xl;
  guint32 len;
  gint32 header_length;
  gulong seek;

  g_return_val_if_fail (ml, NULL);
  g_return_val_if_fail (mty, NULL);
  g_return_val_if_fail (cts, NULL);

#if ITUNESDB_DEBUG
  fprintf(stderr, "get_mhod seek: %ld\n", mhod_seek);
#endif

  g_return_val_if_fail (cts, NULL);

  *ml = -1;

  g_return_val_if_fail (!cts->error, NULL);

  *mty = get_mhod_type (cts, mhod_seek, &len);
  if (*mty == -1)
  {
      if (!cts->error)
      {   /* set error */
	  g_set_error (&cts->error,
		       ITDB_FILE_ERROR,
		       ITDB_FILE_ERROR_CORRUPT,
		       _("iTunesDB corrupt: no MHOD at offset %ld in file '%s'."),
		       mhod_seek, cts->filename);
      }
      return NULL;
  }
  header_length = get32lint (cts, mhod_seek+4); /* header length  */
  if (cts->error) return NULL;

  seek = mhod_seek + header_length;

#if ITUNESDB_DEBUG
  fprintf(stderr, "ml: %x mty: %x\n", *ml, *mty);
#endif

  switch ((enum MHOD_ID)*mty)
  {
  case MHOD_ID_MHYP:
      /* this is not yet supported */
  case MHOD_ID_PLAYLIST:
      /* return the position indicator */
      result = (void *)get32lint (cts, mhod_seek+24);
      if (cts->error) return NULL;
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
      xl = get32lint (cts, seek+4);   /* length of string */
      if (cts->error) return NULL;
      entry_utf16 = g_new0 (gunichar2, (xl+2)/2);
      if (seek_get_n_bytes (cts, (gchar *)entry_utf16, seek+16, xl))
      {
	  fixup_little_utf16 (entry_utf16);
	  result = g_utf16_to_utf8 (entry_utf16, -1, NULL, NULL, NULL);
      }
      else
      {   /* error */
	  result = NULL;
      }
      g_free (entry_utf16);
      break;
  case MHOD_ID_PODCASTURL:
  case MHOD_ID_PODCASTRSS:
      /* length of string */
      xl = get32lint (cts, mhod_seek+8) - header_length;
      if (cts->error) return NULL;
      result = g_new0 (gchar, xl+1);
      if (!seek_get_n_bytes (cts, (gchar *)result, seek, xl))
      {
	  g_free (result);
	  result = NULL;
      }
      break;
  case MHOD_ID_SPLPREF:  /* Settings for smart playlist */
      splp = g_new0 (SPLPref, 1);
      splp->liveupdate = get8int (cts, seek);
      if (cts->error) return NULL;
      splp->checkrules = get8int (cts, seek+1);
      if (cts->error) return NULL;
      splp->checklimits = get8int (cts, seek+2);
      if (cts->error) return NULL;
      splp->limittype = get8int (cts, seek+3);
      if (cts->error) return NULL;
      splp->limitsort = get8int (cts, seek+4);
      if (cts->error) return NULL;
      splp->limitvalue = get32lint (cts, seek+8);
      if (cts->error) return NULL;
      splp->matchcheckedonly = get8int (cts, seek+12);
      if (cts->error) return NULL;
      limitsort_opposite = get8int (cts, seek+13);
      if (cts->error) return NULL;
      /* if the opposite flag is on, set limitsort's high bit -- see
	 note in itunesdb.h for more info */
      if (limitsort_opposite)
	  splp->limitsort |= 0x80000000;
      result = splp;
      break;
  case MHOD_ID_SPLRULES:  /* Rules for smart playlist */
      if (cmp_n_bytes_seek (cts, "SLst", seek, 4))
      {
	  /* !!! for some reason the SLst part is the only part of the
	     iTunesDB with big-endian encoding, including UTF16
	     strings */
	  gint i;
	  guint32 numrules;
	  SPLRules *splrs = g_new0 (SPLRules, 1);
	  splrs->unk004 = get32bint (cts, seek+4);
	  if (cts->error) return NULL;
	  numrules = get32bint (cts, seek+8);
	  if (cts->error) return NULL;
	  splrs->match_operator = get32bint (cts, seek+12);
	  if (cts->error) return NULL;
	  seek += 136;  /* I can't find this value stored in the
			   iTunesDB :-( */
	  for (i=0; i<numrules; ++i)
	  {
	      guint32 length;
	      SPLRule *splr = g_new0 (SPLRule, 1);
	      splr->field = get32bint (cts, seek);
	      if (cts->error) return NULL;
	      splr->action = get32bint (cts, seek+4);
	      if (cts->error) return NULL;
	      seek += 52;
	      length = get32bint (cts, seek);
	      if (cts->error) return NULL;
	      if (itdb_spl_action_known (splr->action))
	      {
		  gint ft = itdb_splr_get_field_type (splr);
		  if (ft == splft_string)
		  {
		      gunichar2 *string_utf16 = g_new0 (gunichar2,
							(length+2)/2);
		      if (!seek_get_n_bytes (cts, (gchar *)string_utf16,
					     seek+4, length))
		      {
			  g_free (string_utf16);
			  g_free (splr);
			  return NULL;
		      }
		      fixup_big_utf16 (string_utf16);
		      splr->string = g_utf16_to_utf8 (
			  string_utf16, -1, NULL, NULL, NULL);
		      g_free (string_utf16);
		  }
		  else
		  {
		      if (length != 0x44)
		      {
			  g_warning (_("Length of smart playlist rule field (%d) not as expected. Trying to continue anyhow.\n"), length);
		      }
		      splr->fromvalue = get64bint (cts, seek+4);
		      if (cts->error) return NULL;
		      splr->fromdate = get64bint (cts, seek+12);
		      if (cts->error) return NULL;
		      splr->fromunits = get64bint (cts, seek+20);
		      if (cts->error) return NULL;
		      splr->tovalue = get64bint (cts, seek+28);
		      if (cts->error) return NULL;
		      splr->todate = get64bint (cts, seek+36);
		      if (cts->error) return NULL;
		      splr->tounits = get64bint (cts, seek+44);
		      if (cts->error) return NULL;
		      /* SPLFIELD_PLAYLIST seem to use these unknowns*/
		      splr->unk052 = get32bint (cts, seek+52);
		      if (cts->error) return NULL;
		      splr->unk056 = get32bint (cts, seek+56);
		      if (cts->error) return NULL;
		      splr->unk060 = get32bint (cts, seek+60);
		      if (cts->error) return NULL;
		      splr->unk064 = get32bint (cts, seek+64);
		      if (cts->error) return NULL;
		      splr->unk068 = get32bint (cts, seek+68);
		      if (cts->error) return NULL;
		  }  
		  seek += length+4;
	      }
	      else
	      {
		  g_free (splr);
		  splr = NULL;
	      }
	      if (splr)
	      {
		  splrs->rules = g_list_append (splrs->rules, splr);
	      }
	  }
	  result = splrs;
      }
      else
      {
	  if (!cts->error)
	      g_warning (_("Did not find SLst hunk as expected. Trying to continue.\n"));
	  else
	      return NULL;
      }
      break;
  default:
      g_warning (_("Encountered unknown MHOD type (%d) while parsing the iTunesDB. Ignoring.\n\n"), *mty);
      break;
  }
  *ml = len;
  return result;
}

/* Returns the value of a string type mhod. return the length of the
   mhod *ml, the mhod type *mty, and a string with the entry (in
   UTF8). After use you must free the string with g_free(). Returns
   NULL if no string is avaible. *ml is set to -1 in case of error and
   cts->error is set appropriately. */
static gchar *get_mhod_string (FContents *cts, glong seek, guint32 *ml, gint32 *mty)
{
    gchar *result = NULL;

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
	result = get_mhod (cts, seek, ml, mty);
	break;
    case MHOD_ID_SPLPREF:
    case MHOD_ID_SPLRULES:
    case MHOD_ID_MHYP:
    case MHOD_ID_PLAYLIST:
	/* these do not have a string entry */
	break;
    }
    return result;
}


/* Get a playlist. Returns the position where the next playlist should
   be. On error -1 is returned and fimp->error is set appropriately. */
static glong get_playlist (FImport *fimp, glong seek)
{
  auto gint pos_comp (gpointer a, gpointer b);
  gint pos_comp (gpointer a, gpointer b)
  {
    return ((gint)a - (gint)b);
  }

  gchar *plname_utf8 = NULL;
  guint32 i, tracknum, mhod_num;
  glong nextseek;
  guint32 hlen;
  Itdb_Playlist *plitem = NULL;
  FContents *cts;

#if ITUNESDB_DEBUG
  fprintf(stderr, "mhyp seek: %x\n", (int)seek);
#endif
  g_return_val_if_fail (fimp, -1);
  g_return_val_if_fail (fimp->idtree, -1);

  cts = fimp->itunesdb;

  if (!cmp_n_bytes_seek (cts, "mhyp", seek, 4))
  {
      if (cts->error)
	  g_propagate_error (&fimp->error, cts->error);
      return -1;
  }
  hlen = get32lint (cts, seek+4); /* length of header */
  CHECK_ERROR (fimp, -1);
  if (hlen == 0)
  {
      g_set_error (&fimp->error,
		   ITDB_FILE_ERROR,
		   ITDB_FILE_ERROR_CORRUPT,
		   _("iTunesDB corrupt: hunk length 0 for hunk at %ld in file '%s'."),
		   seek, cts->filename);
      return -1;
  }
  nextseek = seek + get32lint (cts, seek+8);/* possible begin of next PL */
  CHECK_ERROR (fimp, -1);
  mhod_num = get32lint (cts, seek+12); /* number of MHODs we expect */
  CHECK_ERROR (fimp, -1);
  tracknum = get32lint (cts, seek+16); /* number of tracks in playlist */
  CHECK_ERROR (fimp, -1);
  plitem = itdb_playlist_new (NULL, FALSE);
  /* Some Playlists have added 256 to their type -- I don't know what
     it's for, so we just ignore it for now -> & 0xff */
  plitem->type = get32lint (cts, seek+20) & 0xff;
  CHECK_ERROR (fimp, -1);
  plitem->id = get64lint (cts, seek+28);
  CHECK_ERROR (fimp, -1);
  plitem->unk036 = get32lint (cts, seek+36);
  CHECK_ERROR (fimp, -1);
  plitem->unk040 = get32lint (cts, seek+40);
  CHECK_ERROR (fimp, -1);
  plitem->unk044 = get32lint (cts, seek+44);
  CHECK_ERROR (fimp, -1);
  for (i=0; i < mhod_num; ++i)
  {
      gchar *plname_utf8_maybe;
      SPLPref *splpref = NULL;
      SPLRules *splrules = NULL;
      gint32 type;

      seek += hlen;
      type = get_mhod_type (cts, seek, &hlen);
      CHECK_ERROR (fimp, -1);
      if (hlen != -1) switch ((enum MHOD_ID)type)
      {
      case MHOD_ID_PLAYLIST:
	  /* here we could do something about the playlist settings */
	  break;
      case MHOD_ID_TITLE:
	  plname_utf8_maybe = get_mhod (cts, seek, &hlen, &type);
	  CHECK_ERROR (fimp, -1);
	  if (plname_utf8_maybe)
	  {
	      /* sometimes there seem to be two mhod TITLE headers */
	      g_free (plname_utf8);
	      plname_utf8  = plname_utf8_maybe;
	  }
	  break;
      case MHOD_ID_SPLPREF:
	  splpref = get_mhod (cts, seek, &hlen, &type);
	  CHECK_ERROR (fimp, -1);
	  if (splpref)
	  {
	      plitem->is_spl = TRUE;
	      memcpy (&plitem->splpref, splpref, sizeof (SPLPref));
	      g_free (splpref);
	      splpref = NULL;
	  }
	  break;
      case MHOD_ID_SPLRULES:
	  splrules = get_mhod (cts, seek, &hlen, &type);
	  CHECK_ERROR (fimp, -1);
	  if (splrules)
	  {
	      plitem->is_spl = TRUE;
	      memcpy (&plitem->splrules, splrules, sizeof (SPLRules));
	      g_free (splrules);
	      splrules = NULL;
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
	  /* these are not expected here */
	  break;
      case MHOD_ID_MHYP:
	  /* this I don't know how to handle */
	  break;
      }
  }

  if (plname_utf8)
  {
      plitem->name = plname_utf8;
  }
  else
  {   /* we did not read a valid mhod TITLE header -> */
      /* we simply make up our own name */
      if (plitem->type == ITDB_PL_TYPE_MPL)
	  plitem->name = _("Master-PL");
      else
	  plitem->name = _("Playlist");
  }

#if ITUNESDB_DEBUG
  fprintf(stderr, "pln: %s(%d Itdb_Tracks) \n", plitem->name, (int)tracknum);
#endif

  /* add new playlist */
  itdb_playlist_add (fimp->itdb, plitem, -1);

  i=0; /* tracks read */
  while (i<tracknum)
  {
      guint32 len = get32lint (cts, seek+8);
#if ITUNESDB_DEBUG
      fprintf(stderr, "  %lx: seeking track %d of %d\n", seek, i+1, (int)tracknum);
#endif
      CHECK_ERROR (fimp, -1);
      /* We read the mhip headers and skip everything else (the mhips
       * seem to come in pairs: mhip/mhod mhip/mhod ...). */
      if (cmp_n_bytes_seek (cts, "mhyp", seek, 4))
      {   /* This cannot be, let's abort... */
	  g_set_error (&fimp->error,
		       ITDB_FILE_ERROR,
		       ITDB_FILE_ERROR_CORRUPT,
		       _("iTunesDB corrupt: found mhyp at %ld in file '%s'."),
		       seek, cts->filename);
	  return -1;
      }
      if (cmp_n_bytes_seek (cts, "mhip", seek, 4))
      {
#if ITUNESDB_DEBUG
      fprintf(stderr, "  %lx: mhit\n", seek);
#endif
	  Itdb_Track *tr;
	  gint32 pos = -1;
	  guint32 posid;
	  gint32 mhod_type;
	  guint32 mhod_len;
	  guint32 mhit_len;
	  guint32 ref;
	  mhit_len = get32lint(cts, seek+4);
	  CHECK_ERROR (fimp, -1);
	  ref = get32lint(cts, seek+24);
	  CHECK_ERROR (fimp, -1);
	  /* the mhod that follows gives us the position in the
	     playlist (type 100) */
	  mhod_type = get_mhod_type (cts, seek+mhit_len, NULL);
	  CHECK_ERROR (fimp, -1);
	  if (mhod_type == MHOD_ID_PLAYLIST)
	  {
	      posid = (guint32)get_mhod (cts, seek+mhit_len,
					 &mhod_len, &mhod_type);
	      CHECK_ERROR (fimp, -1);
	      /* The posids don't have to be in numeric order, but our
		 database depends on the playlist members being sorted
		 according to the order they appear in the
		 playlist. Therefore we need to find out at which
		 position to insert the track */
	      fimp->pos_glist = g_list_insert_sorted (
		  fimp->pos_glist, (gpointer)posid,
		  (GCompareFunc)pos_comp);
	      pos = g_list_index (fimp->pos_glist, (gpointer)posid);
	      /* For performance reasons set pos to -1 if position is
		 end of list */
	      if (pos == i) pos = -1;
	  }
	  tr = itdb_track_id_tree_by_id (fimp->idtree, ref);
	  if (tr)
	  {
	      itdb_playlist_add_track (plitem, tr, pos);
	  }
	  else
	      g_warning (_("Itdb_Track ID '%d' not found.\n"), ref);
	  ++i;
      }
      CHECK_ERROR (fimp, -1);
      seek += len;
  }
  g_list_free (fimp->pos_glist);
  fimp->pos_glist = NULL;
  return nextseek;
}


/* returns a pointer to the next header or -1 on error. fimp->error is
   set appropriately. If no "mhit" header is found at the location
   specified, -1 is returned but no error is set. */
static glong get_mhit (FImport *fimp, glong seek)
{
  Itdb_Track *track;
  gchar *entry_utf8;
  gint32 type;
  guint32 header_len;
  guint32 zip;
  struct playcount *playcount;
  guint32 i, mhod_nums;
  FContents *cts;

#if ITUNESDB_DEBUG
  fprintf(stderr, "get_mhit seek: %x\n", (int)seek);
#endif

  g_return_val_if_fail (fimp, -1);

  cts = fimp->itunesdb;

  if (!cmp_n_bytes_seek (cts, "mhit", seek, 4))
  {
      if (cts->error)
	  g_propagate_error (&fimp->error, cts->error);
      return -1;
  }

  header_len = get32lint (cts, seek+4);
  CHECK_ERROR (fimp, -1);

  /* Check if entire mhit can be read -- that way we won't have to
   * check for read errors every time we access a single byte */

  check_seek (cts, seek, header_len);

  mhod_nums = get32lint (cts, seek+12);
  CHECK_ERROR (fimp, -1);

  track = itdb_track_new ();

  /* size of the mhit header: For dbversion <= 0x0b (iTunes 4.7 and
     earlier), the length is 0x9c. As of dbversion 0x0c and 0x0d
     (iTunes 4.7.1 - iTunes 4.9), the size is 0xf4. */
  if (header_len < 0x9c)
  {
      g_return_val_if_fail (cts->filename, FALSE);
      g_set_error (&fimp->error,
		   ITDB_FILE_ERROR,
		   ITDB_FILE_ERROR_CORRUPT,
		   _("mhit header length smaller than expected (%x < 0x9c) at offset %ld in file '%s'."),
		   header_len, seek, cts->filename);
      return -1;
  }

  if (header_len >= 0x9c)
  {
      track->id = get32lint(cts, seek+16);         /* iPod ID          */
      track->visible = get32lint (cts, seek+20);
      seek_get_n_bytes (cts, track->filetype_marker, seek+24, 4);
      track->type = get16lint (cts, seek+28);
      track->compilation = get8int (cts, seek+30);
      track->rating = get8int (cts, seek+31);
      track->time_added = get32lint(cts, seek+32); /* time added       */
      track->size = get32lint(cts, seek+36);       /* file size        */
      track->tracklen = get32lint(cts, seek+40);   /* time             */
      track->track_nr = get32lint(cts, seek+44);   /* track number     */
      track->tracks = get32lint(cts, seek+48);     /* nr of tracks     */
      track->year = get32lint(cts, seek+52);       /* year             */
      track->bitrate = get32lint(cts, seek+56);    /* bitrate          */
      track->unk060 = get32lint(cts, seek+60);     /* unknown          */
      track->samplerate = get16lint(cts,seek+62);  /* sample rate      */
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
      track->time_modified = get32lint(cts, seek+104);/* last mod. time */
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
      track->unk140 = get32lint (cts, seek+140);
      track->unk144 = get32lint (cts, seek+144);
      track->unk148 = get32lint (cts, seek+148);
      track->unk152 = get32lint (cts, seek+152);
  }
  if (header_len >= 0xf4)
  {
      track->unk156 = get32lint (cts, seek+156);
      track->unk160 = get32lint (cts, seek+160);
      track->unk164 = get32lint (cts, seek+164);
      track->dbid2 = get64lint (cts, seek+168);
      track->unk176 = get32lint (cts, seek+176);
      track->unk180 = get32lint (cts, seek+180);
      track->unk184 = get32lint (cts, seek+184);
      track->samplecount = get32lint (cts, seek+188);
      track->unk192 = get32lint (cts, seek+192);
      track->unk196 = get32lint (cts, seek+196);
      track->unk200 = get32lint (cts, seek+200);
      track->unk204 = get32lint (cts, seek+204);
      track->unk208 = get32lint (cts, seek+208);
      track->unk212 = get32lint (cts, seek+212);
      track->unk216 = get32lint (cts, seek+216);
      track->unk220 = get32lint (cts, seek+220);
      track->unk224 = get32lint (cts, seek+224);
      track->unk228 = get32lint (cts, seek+228);
      track->unk232 = get32lint (cts, seek+232);
      track->unk236 = get32lint (cts, seek+236);
      track->unk240 = get32lint (cts, seek+240);
  }

  track->transferred = TRUE;                   /* track is on iPod! */

#if ITUNESDB_MHIT_DEBUG
time_t time_mac_to_host (guint32 mactime);
gchar *time_time_to_string (time_t time);
#define printf_mhit(sk, str)  printf ("%3d: %d (%s)\n", sk, get32lint (file, seek+sk), str);
#define printf_mhit_time(sk, str) { gchar *buf = time_time_to_string (itunesdb_time_mac_to_host (get32lint (file, seek+sk))); printf ("%3d: %s (%s)\n", sk, buf, str); g_free (buf); }
  {
      printf ("\nmhit: seek=%lu\n", seek);
      printf_mhit (  4, "header size");
      printf_mhit (  8, "mhit size");
      printf_mhit ( 12, "nr of mhods");
      printf_mhit ( 16, "iPod ID");
      printf_mhit ( 20, "?");
      printf_mhit ( 24, "?");
      printf (" 28: %u (type)\n", get32lint (file, seek+28) & 0xffffff);
      printf (" 28: %u (rating)\n", get32lint (file, seek+28) >> 24);
      printf_mhit ( 32, "timestamp file");
      printf_mhit_time ( 32, "timestamp file");
      printf_mhit ( 36, "size");
      printf_mhit ( 40, "tracklen (ms)");
      printf_mhit ( 44, "track_nr");
      printf_mhit ( 48, "total tracks");
      printf_mhit ( 52, "year");
      printf_mhit ( 56, "bitrate");
      printf_mhit ( 60, "sample rate");
      printf (" 60: %u (sample rate LSB)\n", get32lint (file, seek+60) & 0xffff);
      printf (" 60: %u (sample rate HSB)\n", (get32lint (file, seek+60) >> 16));
      printf_mhit ( 64, "?");
      printf_mhit ( 68, "?");
      printf_mhit ( 72, "?");
      printf_mhit ( 76, "?");
      printf_mhit ( 80, "playcount");
      printf_mhit ( 84, "?");
      printf_mhit ( 88, "last played");
      printf_mhit_time ( 88, "last played");
      printf_mhit ( 92, "CD");
      printf_mhit ( 96, "total CDs");
      printf_mhit (100, "?");
      printf_mhit (104, "?");
      printf_mhit_time (104, "?");
      printf_mhit (108, "?");
      printf_mhit (112, "?");
      printf_mhit (116, "?");
      printf_mhit (120, "?");
      printf_mhit (124, "?");
      printf_mhit (128, "?");
      printf_mhit (132, "?");
      printf_mhit (136, "?");
      printf_mhit (140, "?");
      printf_mhit (144, "?");
      printf_mhit (148, "?");
      printf_mhit (152, "?");
  }
#undef printf_mhit_time
#undef printf_mhit
#endif

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
	  default: /* unknown entry -- discard */
	      g_free (entry_utf8);
	      break;
	  }
      }
      seek += zip;
  }

  playcount = playcount_get_next (fimp);
  if (playcount)
  {
      if (playcount->rating != NO_PLAYCOUNT)
	  track->rating = playcount->rating;
	  
      if (playcount->time_played)
	  track->time_played = playcount->time_played;

      if (playcount->bookmark_time)
	  track->bookmark_time = playcount->bookmark_time;

      track->playcount += playcount->playcount;
      track->recent_playcount = playcount->playcount;
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

    if (!cmp_n_bytes_seek (cts, "mhpo", 0, 4))
    {
	if (cts->error)
	{
	    g_propagate_error (&fimp->error, cts->error);
	}
	else
	{   /* set error */
	    g_return_val_if_fail (cts->filename, FALSE);
	    g_set_error (&fimp->error,
			 ITDB_FILE_ERROR,
			 ITDB_FILE_ERROR_CORRUPT,
			 _("Not a OTG playlist file: '%s' (missing mhpo header)."),
			 cts->filename);
	}
	return FALSE;
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



static gboolean parse_fimp (FImport *fimp)
{
    glong seek=0, pl_mhsd=0;
    guint32 i, zip, nr_tracks=0, nr_playlists=0;
    gboolean swapped_mhsd = FALSE;
    FContents *cts;

    g_return_val_if_fail (fimp, FALSE);
    g_return_val_if_fail (fimp->itdb, FALSE);
    g_return_val_if_fail (fimp->itunesdb, FALSE);
    g_return_val_if_fail (fimp->itunesdb->filename, FALSE);

    cts = fimp->itunesdb;

    if (!cmp_n_bytes_seek (cts, "mhbd", 0, 4))
    {
	if (cts->error)
	{
	    g_propagate_error (&fimp->error, cts->error);
	}
	else
	{   /* set error */
	    g_set_error (&fimp->error,
			 ITDB_FILE_ERROR,
			 ITDB_FILE_ERROR_CORRUPT,
			 _("Not a iTunesDB: '%s' (missing mhdb header)."),
			 cts->filename);
	}
	return FALSE;
    }
    seek = get32lint (cts, 4);
    CHECK_ERROR (fimp, FALSE);
    /* all the headers I know are 0x68 long -- if this one is longer
       we can could simply ignore the additional information */
    /* Since we only need data from the first 32 bytes, don't complain
     * unless it's smaller than that */
    if (seek < 32)
    {
	g_set_error (&fimp->error,
		     ITDB_FILE_ERROR,
		     ITDB_FILE_ERROR_CORRUPT,
		     _("iTunesDB ('%s'): header length of mhsd hunk smaller than expected (%ld<32). Aborting."),
		     cts->filename, seek);
	return FALSE;
    }

    fimp->itdb->version = get32lint (cts, seek+16);
    CHECK_ERROR (fimp, FALSE);
    fimp->itdb->id = get64lint (cts, seek+24);
    CHECK_ERROR (fimp, FALSE);

    for (;;)
    {
	if (cmp_n_bytes_seek (cts, "mhsd", seek, 4))
	{   /* mhsd header -> determine start of playlists */
	    guint32 sth;
	    guint32 len;
	    len = get32lint (cts, seek+8);
	    CHECK_ERROR (fimp, FALSE);
	    sth = get32lint (cts, seek+12);
	    CHECK_ERROR (fimp, FALSE);
	    if (sth == 1)
	    {   /* OK, tracklist, save start of playlists */
		if (!swapped_mhsd)
		    pl_mhsd = seek + len;
	    }
	    else if (sth == 2)
	    {   /* bad: these are playlists... switch */
		if (swapped_mhsd)
		{ /* already switched once -> forget it */
		    g_set_error (&fimp->error,
				 ITDB_FILE_ERROR,
				 ITDB_FILE_ERROR_CORRUPT,
				 _("iTunesDB '%s' corrupt: already found two playlist mhsds -- giving up."),
				 cts->filename);
		    return FALSE;
		}
		else
		{
		    pl_mhsd = seek;
		    seek += len;
		    swapped_mhsd = TRUE;
		}
	    }
	    else
	    { /* neither playlist nor track MHSD --> skip it */
		seek += len;
	    }
	}
	else
	{   /* if the cmp_n_bytes_seek() failed we must check if it is
	       because of an error */
	    CHECK_ERROR (fimp, FALSE);
	}
	if (cmp_n_bytes_seek (cts, "mhlt", seek, 4))
	{   /* mhlt header -> number of tracks */
	    nr_tracks = get32lint (cts, seek+8);
	    CHECK_ERROR (fimp, FALSE);
	    if (nr_tracks == 0)
	    {   /* no tracks -- skip directly to next mhsd */
		break;
	    }
	}
	else
	{   /* if the cmp_n_bytes_seek() failed we must check if it is
	       because of an error */
	    CHECK_ERROR (fimp, FALSE);
	}
	if (cmp_n_bytes_seek (cts, "mhit", seek, 4))
	{   /* mhit header -> start of tracks*/
	    break;
	}
	zip = get32lint (cts, seek+4);
	CHECK_ERROR (fimp, FALSE);
	if (zip == 0)
	{
	    g_set_error (&fimp->error,
			 ITDB_FILE_ERROR,
			 ITDB_FILE_ERROR_CORRUPT,
			 _("iTunesDB corrupt: hunk length 0 for hunk at %ld in file '%s'."),
			 seek, cts->filename);
	    return FALSE;
	}
	seek += zip;
    }
    /* now we should be at the first MHIT */

    /* get every file entry */
    for (i=0; i<nr_tracks; ++i)
    {
	seek = get_mhit (fimp, seek);
	if (fimp->error)  return FALSE;
	if (seek == -1)
	{   /* this should not be -- issue warning */
	    g_warning (_("iTunesDB possibly corrupt: number of tracks (mhit hunks) inconsistent. Trying to continue.\n"));
	    break;
	}
    }

    /* next: playlists */
    seek = pl_mhsd;
    for (;;)
    {   /* this is all a bit of magic to make sure we can handle
	   slightly "off-standard" iTunesDBs as well. Normally we
	   would expect hunks in the following order: <mhsd type 2>,
	   <mhlp> containing the number of playlists, <mhyp>: first
	   playlist header. Here we just scan everything until we find
	   the first <mhyp> ignoring everything we don't know. */
	zip = get32lint (cts, seek+4);
	if (zip == 0)
	{
	    g_set_error (&fimp->error,
			 ITDB_FILE_ERROR,
			 ITDB_FILE_ERROR_CORRUPT,
			 _("iTunesDB corrupt: hunk length 0 for hunk at %ld in file '%s'."),
			 seek, cts->filename);
	    return FALSE;
	}
	CHECK_ERROR (fimp, FALSE);
	if (cmp_n_bytes_seek (cts, "mhsd", seek, 4))
	{   /* We just check if it's actually a playlist mhsd (type=2)
	       or not (type = 1, should not be...) */
	    guint32 type;
	    guint32 len = get32lint (cts, seek+8);
	    CHECK_ERROR (fimp, FALSE);
	    type = get32lint (cts, seek+12);
	    CHECK_ERROR (fimp, FALSE);
	    if (type != 2)
	    {  /* this is not a playlist MHSD -> skip it */
		seek += len;
		continue;
	    }
	    else
	    {   /* jump to next hunk */
		seek += zip;
		continue;
	    }
	}
	else
	{
	    CHECK_ERROR (fimp, FALSE);
	}
	if (cmp_n_bytes_seek (cts, "mhlp", seek, 4))
	{ /* mhlp header -> number of playlists */
	    nr_playlists = get32lint (cts, seek+8);
	    CHECK_ERROR (fimp, FALSE);
	    seek += zip;
	    continue;
	}
	else
	{
	    CHECK_ERROR (fimp, FALSE);
	}
	if (cmp_n_bytes_seek (cts, "mhyp", seek, 4))
	{ /* mhyp header -> start of playlists */
	    break;
	}
	else
	{
	    CHECK_ERROR (fimp, FALSE);
	}
	seek += zip;
    }

#if ITUNESDB_DEBUG
    fprintf(stderr, "iTunesDB part2 starts at: %x\n", (int)seek);
#endif

    /* Create track-id tree for quicker track lookup */
    fimp->idtree = itdb_track_id_tree_create (fimp->itdb);
    for (i=0; i<nr_playlists; ++i)
    {
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


/* Parse the Itdb_iTunesDB.
   Returns a pointer to the Itdb_iTunesDB struct holding the tracks and the
   playlists.
   "mp" should point to the mount point of the iPod,
   e.g. "/mnt/ipod" and be in local encoding */
Itdb_iTunesDB *itdb_parse (const gchar *mp, GError **error)
{
    gchar *filename;
    Itdb_iTunesDB *itdb = NULL;
    const gchar *db[] = {"iPod_Control","iTunes","iTunesDB",NULL};

    filename = itdb_resolve_path (mp, db);
    if (filename)
    {
	itdb = itdb_parse_file (filename, error);
	if (itdb)
	{
	    itdb->mountpoint = g_strdup (mp);
	}
	g_free (filename);

	/* We don't test the return value of ipod_parse_artwork_db since the 
	 * database content will be consistent even if we fail to get the 
	 * various thumbnails, we ignore the error since older ipods don't have
	 * thumbnails.
	 * FIXME: this probably should go into itdb_parse_file, but I don't
	 * understand its purpose, and ipod_parse_artwork_db needs the 
	 * mountpoint field from the itdb, which may not be available in the 
	 * other function
	 */
	ipod_parse_artwork_db (itdb);


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
    return itdb;
}


/* Same as itunesdb_parse(), but filename is specified directly. */
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

    fimp->itunesdb = fcontents_read (filename, error);

    if (fimp->itunesdb)
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
    g_return_if_fail (data);

    if (len != 0)
    {
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


/* Write 1-byte integer @n to @cts */
static void put8int (WContents *cts, guint8 n)
{
    put_data (cts, (gchar *)&n, 1);
}


/* Write 2-byte integer @n to @cts in little endian order. */
static void put16lint (WContents *cts, guint16 n)
{
#   if (G_BYTE_ORDER == G_BIG_ENDIAN)
    n = GUINT16_SWAP_LE_BE (n);
#   endif
    put_data (cts, (gchar *)&n, 2);
}


/* Write 4-byte integer @n to @cts in little endian order. */
static void put32lint (WContents *cts, guint32 n)
{
#   if (G_BYTE_ORDER == G_BIG_ENDIAN)
    n = GUINT32_SWAP_LE_BE (n);
#   endif
    put_data (cts, (gchar *)&n, 4);
}

/* Write 4 byte floating number */
static void put32lfloat (WContents *cts, float f)
{
    union
    {
	guint32 i;
	float   f;
    } flt;

    if (sizeof (float) != 4)
    {
	put32lint (cts, 0);
	g_return_if_reached ();
    }

    flt.f = f;
    put32lint (cts, flt.i);
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

/* Write 3-byte integer @n to @cts in big endian order. */
static void put24bint (WContents *cts, guint32 n)
{
    gchar buf[3] ;
    buf[0] = (n >> 16) & 0xff ;
    buf[1] = (n >> 8)  & 0xff ;
    buf[2] = (n >> 0)  & 0xff ;
    put_data (cts, buf, 3);
}


/* Write 4-byte integer @n to @cts at position @seek in little
   endian order. */
static void put32lint_seek (WContents *cts, guint32 n, gulong seek)
{
#   if (G_BYTE_ORDER == G_BIG_ENDIAN)
    n = GUINT32_SWAP_LE_BE (n);
#   endif
    put_data_seek (cts, (gchar *)&n, 4, seek);
}


/* Write 8-byte integer @n to @cts in big little order. */
static void put64lint (WContents *cts, guint64 n)
{
#   if (G_BYTE_ORDER == G_BIG_ENDIAN)
    n = GUINT64_SWAP_LE_BE (n);
#   endif
    put_data (cts, (gchar *)&n, 8);
}


/* Write 4-byte integer @n to @cts in big endian order. */
static void put32bint (WContents *cts, guint32 n)
{
#   if (G_BYTE_ORDER == G_LITTLE_ENDIAN)
    n = GUINT32_SWAP_LE_BE (n);
#   endif
    put_data (cts, (gchar *)&n, 4);
}


/* Write 8-byte integer @n to cts in big endian order. */
static void put64bint (WContents *cts, guint64 n)
{
#   if (G_BYTE_ORDER == G_LITTLE_ENDIAN)
    n = GUINT64_SWAP_LE_BE (n);
#   endif
    put_data (cts, (gchar *)&n, 8);
}


#if 0
/* Write 4-byte integer @n to @cts at position @seek in big endian
   order. */
static void put32bint_seek (WContents *cts, guint32 n, gulong seek)
{
#   if (G_BYTE_ORDER == G_LITTLE_ENDIAN)
    n = GUINT32_SWAP_LE_BE (n);
#   endif
    put_data_seek (cts, (gchar *)&n, 4, seek);
}

/* Write 8-byte integer @n to @cts at position @seek in big endian
   order. */
static void put64bint_seek (WContents *cts, guint64 n, gulong seek)
{
#   if (G_BYTE_ORDER == G_LITTLE_ENDIAN)
    n = GUINT64_SWAP_LE_BE (n);
#   endif
    put_data_seek (cts, (gchar *)&n, 8, seek);
}
#endif


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



/* Write out the mhbd header. Size will be written later */
static void mk_mhbd (FExport *fexp)
{
  WContents *cts;

  g_return_if_fail (fexp);
  g_return_if_fail (fexp->itdb);
  g_return_if_fail (fexp->itunesdb);

  cts = fexp->itunesdb;

  put_data (cts, "mhbd", 4);
  put32lint (cts, 104); /* header size */
  put32lint (cts, -1);  /* size of whole mhdb -- fill in later */
  put32lint (cts, 1);   /* ? */
  /* Version number: 0x01: iTunes 2
                     0x02: iTunes 3
		     0x09: iTunes 4.2
		     0x0a: iTunes 4.5
		     0x0b: iTunes 4.7
		     0x0c: iTunes 4.71/4.8 (required for shuffle)
                     0x0d: iTunes 4.9 */
  fexp->itdb->version = 0x0d;
  put32lint (cts, fexp->itdb->version);
  put32lint (cts, 2);   /* 2 children (track list and playlist list) */
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
  g_return_if_fail (fexp->itunesdb);

  cts = fexp->itunesdb;

  put_data (cts, "mhsd", 4);
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
  g_return_if_fail (fexp->itunesdb);

  cts = fexp->itunesdb;

  put_data (cts, "mhlt", 4);
  put32lint (cts, 92);       /* Headersize */
  put32lint (cts, num);      /* tracks in this itunesdb */
  put32_n0 (cts, 20);        /* dummy space */
}


/* Write out the mhit header. Size will be written later */
static void mk_mhit (WContents *cts, Itdb_Track *track)
{
  g_return_if_fail (cts);
  g_return_if_fail (track);

  put_data (cts, "mhit", 4);
  put32lint (cts, 0xf4);  /* header size */
  put32lint (cts, -1);   /* size of whole mhit -- fill in later */
  put32lint (cts, -1);   /* nr of mhods in this mhit -- later   */
  put32lint (cts, track->id); /* track index number */

  put32lint (cts, track->visible);
  put_data  (cts, track->filetype_marker, 4);
  put16lint (cts, track->type);
  put8int   (cts, track->compilation);
  put8int   (cts, track->rating);
  put32lint (cts, track->time_added); /* timestamp               */
  put32lint (cts, track->size);    /* filesize                   */
  put32lint (cts, track->tracklen);/* length of track in ms      */
  put32lint (cts, track->track_nr);/* track number               */
  put32lint (cts, track->tracks);  /* number of tracks           */
  put32lint (cts, track->year);    /* the year                   */
  put32lint (cts, track->bitrate); /* bitrate                    */
  put16lint (cts, track->unk060);  /* unknown                    */
  put16lint (cts, track->samplerate);
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
  put32lint (cts, track->time_modified); /* timestamp            */
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
  put32lint (cts, track->unk140);
  put32lint (cts, track->unk144);
  put32lint (cts, track->unk148);
  put32lint (cts, track->unk152);
  /* since iTunesDB version 0x0c */
  put32lint (cts, track->unk156);
  put32lint (cts, track->unk160);
  put32lint (cts, track->unk164);
  put64lint (cts, track->dbid2);
  put32lint (cts, track->unk176);
  put32lint (cts, track->unk180);
  put32lint (cts, track->unk184);
  put32lint (cts, track->samplecount);
  put32lint (cts, track->unk192);
  put32lint (cts, track->unk196);
  put32lint (cts, track->unk200);
  put32lint (cts, track->unk204);
  put32lint (cts, track->unk208);
  put32lint (cts, track->unk212);
  put32lint (cts, track->unk216);
  put32lint (cts, track->unk220);
  put32lint (cts, track->unk224);
  put32lint (cts, track->unk228);
  put32lint (cts, track->unk232);
  put32lint (cts, track->unk236);
  put32lint (cts, track->unk240);
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


/* Write out one mhod header.
     type: see enum of MHMOD_IDs;
     data: utf8 string for text items
           position indicator for MHOD_ID_PLAYLIST
           SPLPref for MHOD_ID_SPLPREF
	   SPLRules for MHOD_ID_SPLRULES */
static void mk_mhod (WContents *cts, enum MHOD_ID type, void *data)
{
  g_return_if_fail (cts);

  switch (type)
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
      g_return_if_fail (data);
      {
	  /* convert to utf16  */
	  gunichar2 *entry_utf16 = g_utf8_to_utf16 ((gchar *)data, -1,
						    NULL, NULL, NULL);
	  guint32 len = utf16_strlen (entry_utf16);
	  fixup_little_utf16 (entry_utf16);
	  put_data (cts, "mhod", 4);  /* header                     */
	  put32lint (cts, 24);        /* size of header             */
	  put32lint (cts, 2*len+40);  /* size of header + body      */
	  put32lint (cts, type);      /* type of the mhod           */
	  put32_n0 (cts, 2);          /* unknown                    */
	  /* end of header, start of data */
	  put32lint (cts, 1);         /* always 1 for these MHOD_IDs*/
	  put32lint (cts, 2*len);     /* size of string             */
	  put32_n0 (cts, 2);          /* unknown                    */
	  put_data (cts, (gchar *)entry_utf16, 2*len);/* the string */
	  g_free (entry_utf16);
      }
      break;
  case MHOD_ID_PODCASTURL:
  case MHOD_ID_PODCASTRSS:
      g_return_if_fail (data);
      {
	  guint32 len = strlen ((gchar *)data);
	  put_data (cts, "mhod", 4);  /* header                     */
	  put32lint (cts, 24);        /* size of header             */
	  put32lint (cts, 24+len);    /* size of header + data      */
	  put32lint (cts, type);      /* type of the mhod           */
	  put32_n0 (cts, 2);          /* unknown                    */
	  put_data (cts, (gchar *)data, len); /* the string         */
      }
      break;
  case MHOD_ID_PLAYLIST:
      put_data (cts, "mhod", 4);  /* header                     */
      put32lint (cts, 24);        /* size of header             */
      put32lint (cts, 44);        /* size of header + body      */
      put32lint (cts, type);      /* type of the entry          */
      put32_n0 (cts, 2);          /* unknown                    */
      /* end of header, start of data */
      put32lint (cts, (guint32)data);/* position of track in playlist */
      put32_n0 (cts, 4);             /* unknown                    */
      break;
  case MHOD_ID_SPLPREF:
      g_return_if_fail (data);
      {
	  SPLPref *splp = data;
	  put_data (cts, "mhod", 4); /* header                 */
	  put32lint (cts, 24);       /* size of header         */
	  put32lint (cts, 96);       /* size of header + body  */
	  put32lint (cts, type);     /* type of the entry      */
	  put32_n0 (cts, 2);           /* unknown                */
	  /* end of header, start of data */
	  put8int (cts, splp->liveupdate);
	  put8int (cts, splp->checkrules? 1:0);
	  put8int (cts, splp->checklimits);
	  put8int (cts, splp->limittype);
	  put8int (cts, splp->limitsort & 0xff);
	  put8int (cts, 0);          /* unknown                */
	  put8int (cts, 0);          /* unknown                */
	  put8int (cts, 0);          /* unknown                */
	  put32lint (cts, splp->limitvalue);
	  put8int (cts, splp->matchcheckedonly);
	  /* for the following see note at definitions of limitsort
	     types in itunesdb.h */
	  put8int (cts, (splp->limitsort & 0x80000000) ? 1:0);
	  put8int (cts, 0);          /* unknown                */
	  put8int (cts, 0);          /* unknown                */
	  put32_n0 (cts, 14);          /* unknown                */
      }
      break;
  case MHOD_ID_SPLRULES:
      g_return_if_fail (data);
      {
	  SPLRules *splrs = data;
	  gulong header_seek = cts->pos; /* needed to fix length */
	  GList *gl;
	  gint numrules = g_list_length (splrs->rules);

	  put_data (cts, "mhod", 4); /* header                   */
	  put32lint (cts, 24);       /* size of header           */
	  put32lint (cts, -1);       /* total length, fix later  */
	  put32lint (cts, type);     /* type of the entry        */
	  put32_n0 (cts, 2);           /* unknown                  */
	  /* end of header, start of data */
	  /* For some reason this is the only part of the iTunesDB
	     that uses big endian */
	  put_data (cts, "SLst", 4);      /* header               */
	  put32bint (cts, splrs->unk004); /* unknown              */
	  put32bint (cts, numrules);
	  put32bint (cts, splrs->match_operator);
	  put32_n0 (cts, 30);              /* unknown              */
	  /* end of header, now follow the rules */
	  for (gl=splrs->rules; gl; gl=gl->next)
	  {
	      SPLRule *splr = gl->data;
	      gint ft;
	      g_return_if_fail (splr);
	      ft = itdb_splr_get_field_type (splr);
/*	      printf ("%p: field: %d ft: %d\n", splr, splr->field, ft);*/
	      itdb_splr_validate (splr);
	      put32bint (cts, splr->field);
	      put32bint (cts, splr->action);
	      put32_n0 (cts, 11);          /* unknown              */
	      if (ft == splft_string)
	      {   /* write string-type rule */
		  gunichar2 *entry_utf16 =
		      g_utf8_to_utf16 (splr->string, -1,NULL,NULL,NULL);
		  gint len = utf16_strlen (entry_utf16);
		  fixup_big_utf16 (entry_utf16);
		  put32bint (cts, 2*len); /* length of string     */
		  put_data (cts, (gchar *)entry_utf16, 2*len);
		  g_free (entry_utf16);
	      }
	      else
	      {   /* write non-string-type rule */
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
	      }
	  }
	  /* insert length of mhod junk */
	  fix_header (cts, header_seek);
      }
      break;
  case MHOD_ID_MHYP:
      g_warning (_("Cannot write mhod of type %d\n"), type);
      break;
  }
}


/* Write out the mhlp header. Size will be written later */
static void mk_mhlp (FExport *fexp)
{
  WContents *cts;

  g_return_if_fail (fexp);
  g_return_if_fail (fexp->itunesdb);

  cts = fexp->itunesdb;

  put_data (cts, "mhlp", 4);       /* header                   */
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
  g_return_if_fail (fexp->itunesdb);
  g_return_if_fail (pl);

  cts = fexp->itunesdb;

  put_data (cts, "mhod", 4);         /* header                   */
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
static void mk_mhip (FExport *fexp, guint32 pos, guint32 id)
{
  WContents *cts;

  g_return_if_fail (fexp);
  g_return_if_fail (fexp->itunesdb);

  cts = fexp->itunesdb;

  put_data (cts, "mhip", 4);
  put32lint (cts, 76);                              /*  4 */
  put32lint (cts, -1);  /* fill in later */         /*  8 */
  put32lint (cts, 1);   /* number of children */    /* 12 */
  put32lint (cts, 0);   /* unknown */               /* 16 */
  put32lint (cts, 0);   /* unknown */               /* 20 */
  put32lint (cts, id);  /* id */                    /* 24 */
  put32_n0 (cts, 12);                               /* 28 */
}


/* Write first mhsd hunk. Return FALSE in case of error and set
 * fexp->error */
static gboolean write_mhsd_one(FExport *fexp)
{
    GList *gl;
    gulong mhsd_seek;
    WContents *cts;

    g_return_val_if_fail (fexp, FALSE);
    g_return_val_if_fail (fexp->itdb, FALSE);
    g_return_val_if_fail (fexp->itunesdb, FALSE);

    cts = fexp->itunesdb;
    
    mhsd_seek = cts->pos;      /* get position of mhsd header  */
    mk_mhsd (fexp, 1);         /* write header: type 1: tracks */
    /* write header with nr. of tracks */
    mk_mhlt (fexp, g_list_length (fexp->itdb->tracks));
    for (gl=fexp->itdb->tracks; gl; gl=gl->next)  /* Write each track */
    {
	Itdb_Track *track = gl->data;
	guint32 mhod_num = 0;
	gulong mhit_seek = cts->pos;
	if (!track)
	{
	    g_set_error (&fexp->error,
			 ITDB_FILE_ERROR,
			 ITDB_FILE_ERROR_ITDB_CORRUPT,
			 _("Database in memory corrupt (track pointer == NULL). Aborting export."));
	    return FALSE;
	}
	mk_mhit (cts, track);
	if (track->title && *track->title)
	{
	    mk_mhod (cts, MHOD_ID_TITLE, track->title);
	    ++mhod_num;
	}
	if (track->ipod_path && *track->ipod_path)
	{
	    mk_mhod (cts, MHOD_ID_PATH, track->ipod_path);
	    ++mhod_num;
	}
	if (track->album && *track->album)
	{
	    mk_mhod (cts, MHOD_ID_ALBUM, track->album);
	    ++mhod_num;
	}
	if (track->artist && *track->artist)
	{
	    mk_mhod (cts, MHOD_ID_ARTIST, track->artist);
	    ++mhod_num;
	}
	if (track->genre && *track->genre)
	{
	    mk_mhod (cts, MHOD_ID_GENRE, track->genre);
	    ++mhod_num;
	}
	if (track->filetype && *track->filetype)
	{
	    mk_mhod (cts, MHOD_ID_FILETYPE, track->filetype);
	    ++mhod_num;
	}
	if (track->comment && *track->comment)
	{
	    mk_mhod (cts, MHOD_ID_COMMENT, track->comment);
	    ++mhod_num;
	}
	if (track->category && *track->category)
	{
	    mk_mhod (cts, MHOD_ID_CATEGORY, track->category);
	    ++mhod_num;
	}
	if (track->composer && *track->composer)
	{
	    mk_mhod (cts, MHOD_ID_COMPOSER, track->composer);
	    ++mhod_num;
	}
	if (track->grouping && *track->grouping)
	{
	    mk_mhod (cts, MHOD_ID_GROUPING, track->grouping);
	    ++mhod_num;
	}
	if (track->description && *track->description)
	{
	    mk_mhod (cts, MHOD_ID_DESCRIPTION, track->description);
	    ++mhod_num;
	}
	if (track->subtitle && *track->subtitle)
	{
	    mk_mhod (cts, MHOD_ID_SUBTITLE, track->subtitle);
	    ++mhod_num;
	}
	if (track->podcasturl && *track->podcasturl)
	{
	    mk_mhod (cts, MHOD_ID_PODCASTURL, track->podcasturl);
	    ++mhod_num;
	}
	if (track->podcastrss && *track->podcastrss)
	{
	    mk_mhod (cts, MHOD_ID_PODCASTRSS, track->podcastrss);
	    ++mhod_num;
	}
        /* Fill in the missing items of the mhit header */
	fix_mhit (cts, mhit_seek, mhod_num);
    }
    fix_header (cts, mhsd_seek);
    return TRUE;
}

/* corresponds to mk_mhyp */
/* Return FALSE in case of error and set fexp->error */
static gboolean write_playlist(FExport *fexp, Itdb_Playlist *pl)
{
    GList *gl;
    gulong mhyp_seek, mhip_seek;
    guint32 i;
    WContents *cts;

    g_return_val_if_fail (fexp, FALSE);
    g_return_val_if_fail (fexp->itdb, FALSE);
    g_return_val_if_fail (fexp->itunesdb, FALSE);
    g_return_val_if_fail (pl, FALSE);

    cts = fexp->itunesdb;
    mhyp_seek = cts->pos;

#if ITUNESDB_DEBUG
  fprintf(stderr, "Playlist: %s (%d tracks)\n", pl->name, g_list_length (pl->members));
#endif

    put_data (cts, "mhyp", 4);    /* header                    */
    put32lint (cts, 108);          /* length		        */
    put32lint (cts, -1);           /* size -> later             */
    if (pl->is_spl)
	put32lint (cts, 4);        /* nr of mhods               */
    else
	put32lint (cts, 2);        /* nr of mhods               */
    /* number of tracks in plist */
    put32lint (cts, g_list_length (pl->members));
    put32lint (cts, pl->type);     /* 1 = main, 0 = visible     */
    put32lint (cts, 0);            /* some timestamp            */
    put64lint (cts, pl->id);       /* 64 bit ID                 */
    put32lint (cts, pl->unk036);
    put32lint (cts, pl->unk040);
    put32lint (cts, pl->unk044);
    put32_n0 (cts, 15);            /* ?                         */

    mk_mhod (cts, MHOD_ID_TITLE, pl->name);
    mk_long_mhod_id_playlist (fexp, pl);

    if (pl->is_spl)
    {  /* write the smart rules */
	mk_mhod (cts, MHOD_ID_SPLPREF, &pl->splpref);
	mk_mhod (cts, MHOD_ID_SPLRULES, &pl->splrules);
    }

    /* write hard-coded tracks */
    i=0;
    for (gl=pl->members; gl; gl=gl->next)
    {
	Itdb_Track *track = gl->data;
	if (!track)
	{
	    g_set_error (&fexp->error,
			 ITDB_FILE_ERROR,
			 ITDB_FILE_ERROR_ITDB_CORRUPT,
			 _("Database in memory corrupt (track pointer == NULL). Aborting export."));
	    return FALSE;
	}
	mhip_seek = cts->pos;
	mk_mhip (fexp, i, track->id);
	mk_mhod (cts, MHOD_ID_PLAYLIST, (void *)i);
	/* note: with iTunes 4.9 the mhod is counted as a child to
	   mhip, so we fill have put the total length of the mhip and
	   mhod into the mhip header */
	fix_header (cts, mhip_seek);
	++i;
    }
    fix_header (cts, mhyp_seek);
    return TRUE;
}



/* Expects the master playlist to be the first in the list */
/* Return FALSE in case of error and set fexp->error */
static gboolean write_mhsd_two(FExport *fexp)
{
    GList *gl;
    glong mhsd_seek;
    WContents *cts;

    g_return_val_if_fail (fexp, FALSE);
    g_return_val_if_fail (fexp->itdb, FALSE);
    g_return_val_if_fail (fexp->itunesdb, FALSE);

    cts = fexp->itunesdb;
    mhsd_seek = cts->pos;      /* get position of mhsd header */
    mk_mhsd (fexp, 2);         /* write header: type 2: playlists  */
    /* write header with nr. of playlists */
    mk_mhlp (fexp);
    for(gl=fexp->itdb->playlists; gl; gl=gl->next)
    {
	Itdb_Playlist *pl = gl->data;
	if (!pl)
	{
	    g_set_error (&fexp->error,
			 ITDB_FILE_ERROR,
			 ITDB_FILE_ERROR_ITDB_CORRUPT,
			 _("Database in memory corrupt (playlist pointer == NULL). Aborting export."));
	    return FALSE;
	}
	write_playlist (fexp, pl);
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
				  _("Opening of '%s' for writing failed."),
				  cts->filename);
	return FALSE;
    }
    if (cts->contents && cts->pos)
    {
	ssize_t written = write (fd, cts->contents, cts->pos);
	if (written == -1)
	{
	    cts->error = g_error_new (G_FILE_ERROR,
				      g_file_error_from_errno (errno),
				      _("Writing to '%s' failed."),
				      cts->filename);
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
static void reassign_ids (Itdb_iTunesDB *itdb)
{
    guint32 id = 52;
    GList *gl;
    Itdb_Playlist *mpl;

    g_return_if_fail (itdb);

    /* copy mpl->members to itdb->tracks to make sure they are in the
       same order (otherwise On-The-Go Playlists will not show the
       correct content) */
    mpl = itdb_playlist_mpl (itdb);
    g_return_if_fail (mpl);
    g_return_if_fail (g_list_length (mpl->members) == g_list_length (itdb->tracks));
    g_list_free (itdb->tracks);
    itdb->tracks = g_list_copy (mpl->members);

    /* assign unique IDs */
    for (gl=itdb->tracks; gl; gl=gl->next)
    {
	Itdb_Track *track = gl->data;
	g_return_if_fail (track);
	track->id = id++;
    }
}



/* Do the actual writing to the iTunesDB */
/* If @filename==NULL, itdb->filename is tried */
gboolean itdb_write_file (Itdb_iTunesDB *itdb, const gchar *filename,
			  GError **error)
{
    FExport *fexp;
    gulong mhbd_seek = 0;
    WContents *cts;
    gboolean result = TRUE;;

    g_return_val_if_fail (itdb, FALSE);
    g_return_val_if_fail (filename || itdb->filename, FALSE);

    if (!filename) filename = itdb->filename;

    reassign_ids (itdb);

    fexp = g_new0 (FExport, 1);
    fexp->itdb = itdb;
    fexp->itunesdb = wcontents_new (filename);
    cts = fexp->itunesdb;

    mk_mhbd (fexp);
    if (write_mhsd_one(fexp))
    {   /* write playlists mhsd */
	if (write_mhsd_two(fexp))
	{
	    fix_header (cts, mhbd_seek);
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

/* Write out an iTunesDB.

   First reassigns unique IDs to all tracks.

   An existing "Play Counts" file is renamed to "Play Counts.bak" if
   the export was successful.

   An existing "OTGPlaylistInfo" file is removed if the export was
   successful.

   Returns TRUE on success, FALSE on error, in which case @error is
   set accordingly.

   @mp must point to the mount point of the iPod, e.g. "/mnt/ipod" and
   be in local encoding. If mp==NULL, itdb->mountpoint is tried. */
gboolean itdb_write (Itdb_iTunesDB *itdb, const gchar *mp, GError **error)
{
    gchar *itunes_filename, *itunes_path;
    const gchar *db[] = {"iPod_Control","iTunes",NULL};
    gboolean result = FALSE;

    g_return_val_if_fail (itdb, FALSE);
    g_return_val_if_fail (mp || itdb->mountpoint, FALSE);

    if (!mp) mp = itdb->mountpoint;

    itunes_path = itdb_resolve_path (mp, db);
    
    if(!itunes_path)
    {
	gchar *str = g_build_filename (mp, db[0], db[1], db[2], NULL);
	g_set_error (error,
		     ITDB_FILE_ERROR,
		     ITDB_FILE_ERROR_NOTFOUND,
		     _("Path not found: '%s'."),
		     str);
	g_free (str);
	return FALSE;
    }

    itunes_filename = g_build_filename (itunes_path, "iTunesDB", NULL);

    result = itdb_write_file (itdb, itunes_filename, error);

    g_free(itunes_filename);
    g_free(itunes_path);

    if (result == TRUE)
	result = itdb_rename_files (mp, error);

    if (result == TRUE)
    {
	gchar *mnp = g_strdup (mp);
	g_free (itdb->mountpoint);
	itdb->mountpoint = mnp;
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


/* Write out an iTunesSD for the Shuffle.

   First reassigns unique IDs to all tracks.

   An existing "Play Counts" file is renamed to "Play Counts.bak" if
   the export was successful.

   An existing "OTGPlaylistInfo" file is removed if the export was
   successful.

   Returns TRUE on success, FALSE on error, in which case @error is
   set accordingly.

   @mp must point to the mount point of the iPod, e.g. "/mnt/ipod" and
   be in local encoding. If mp==NULL, itdb->mountpoint is tried. */


gboolean itdb_shuffle_write (Itdb_iTunesDB *itdb,
			     const gchar *mp, GError **error)
{
    gchar *itunes_filename, *itunes_path;
    const gchar *db[] = {"iPod_Control","iTunes",NULL};
    gboolean result = FALSE;

    g_return_val_if_fail (itdb, FALSE);
    g_return_val_if_fail (mp || itdb->mountpoint, FALSE);

    if (!mp) mp = itdb->mountpoint;

    itunes_path = itdb_resolve_path (mp, db);
    
    if(!itunes_path)
    {
	gchar *str = g_build_filename (mp, db[0], db[1], db[2], NULL);
	g_set_error (error,
		     ITDB_FILE_ERROR,
		     ITDB_FILE_ERROR_NOTFOUND,
		     _("Path not found: '%s'."),
		     str);
	g_free (str);
	return FALSE;
    }

    itunes_filename = g_build_filename (itunes_path, "iTunesSD", NULL);

    result = itdb_shuffle_write_file (itdb, itunes_filename, error);

    g_free(itunes_filename);
    g_free(itunes_path);

    if (result == TRUE)
	result = itdb_rename_files (mp, error);

    if (result == TRUE)
    {
	gchar *mnp = g_strdup (mp);
	g_free (itdb->mountpoint);
	itdb->mountpoint = mnp;
    }

    /* make sure all buffers are flushed as some people tend to
       disconnect as soon as gtkpod returns */
    sync ();

    return result;
}


/* Do the actual writing to the iTunesSD */
/* If @filename cannot be NULL */
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

    reassign_ids (itdb);

    fexp = g_new0 (FExport, 1);
    fexp->itdb = itdb;
    fexp->itunesdb = wcontents_new (filename);
    cts = fexp->itunesdb;

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


/* (Renames/removes some files on the iPod (Playcounts, OTG
   semaphore). May have to be called if you write the iTunesDB not
   directly to the iPod but to some other location and then manually
   copy the file from there to the iPod. */
/* Returns FALSE on error and sets @error accordingly */
gboolean itdb_rename_files (const gchar *mp, GError **error)
{
    const gchar *db_itd[] =  {"iPod_Control", "iTunes", NULL};
    const gchar *db_plc_o[] = {"Play Counts", NULL};
    const gchar *db_otg[] = {"OTGPlaylistInfo", NULL};
    gchar *itunesdir;
    gchar *plcname_o;
    gchar *plcname_n;
    gchar *otgname;
    gboolean result = TRUE;

    itunesdir = itdb_resolve_path (mp, db_itd);
    if(!itunesdir)
    {
	gchar *str = g_build_filename (mp, db_itd[0],
				       db_itd[1], db_itd[2], NULL);
	g_set_error (error,
		     ITDB_FILE_ERROR,
		     ITDB_FILE_ERROR_NOTFOUND,
		     _("Path not found: '%s'."),
		     str);
	g_free (str);
	return FALSE;
    }


    plcname_o = itdb_resolve_path (itunesdir, db_plc_o);
    plcname_n = g_build_filename (itunesdir, 
					 "Play Counts.bak", NULL);
    otgname = itdb_resolve_path (itunesdir, db_otg);

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

    g_free (plcname_o);
    g_free (plcname_n);
    g_free (otgname);
    g_free (itunesdir);

    return result;
}


/* Convert string from casual PC file name to iPod iTunesDB format
 * using ':' instead of slashes
 */
void itdb_filename_fs2ipod (gchar *ipod_file)
{
    g_strdelimit (ipod_file, G_DIR_SEPARATOR_S, ':');
}

/* Convert string from iPod iTunesDB format to casual PC file name
 * using slashes instead of ':'
 */
void itdb_filename_ipod2fs (gchar *ipod_file)
{
    g_strdelimit (ipod_file, ":", G_DIR_SEPARATOR);
}



/* Set the mountpoint.
 *
 * Always use this function to set the mountpoint as it will reset the
 * number of available /iPod_Control/Music/F.. dirs
*/
void itdb_set_mountpoint (Itdb_iTunesDB *itdb, const gchar *mp)
{
    g_return_if_fail (itdb);

    g_free (itdb->mountpoint);
    itdb->mountpoint = g_strdup (mp);
    itdb->musicdirs = 0;
}


/* Determine the number of F.. directories in iPod_Control/Music.

   If itdb->musicdirs is already set, simply return the previously
   determined number. Otherwise count the directories first and set
   itdb->musicdirs. */
gint itdb_musicdirs_number (Itdb_iTunesDB *itdb)
{
    gchar *dest_components[] = {"iPod_Control", "Music",
				NULL, NULL, NULL};
    gchar *dir_filename = NULL;
    gint dir_num;

    g_return_val_if_fail (itdb, 0);
    g_return_val_if_fail (itdb->mountpoint, 0);

    if (itdb->musicdirs <= 0)
    {   /* count number of dirs */
	for (dir_num=0; ;++dir_num)
	{
	    gchar dir_num_str[5];

	    g_snprintf (dir_num_str, 5, "F%02d", dir_num);
	    dest_components[2] = dir_num_str;
  
	    dir_filename =
		itdb_resolve_path (itdb->mountpoint,
				   (const gchar **)dest_components);

	    if (!dir_filename)  break;
	    g_free (dir_filename);
	}
	itdb->musicdirs = dir_num;
    }
    return itdb->musicdirs;
}




/* Copy one track to the iPod. The PC filename is @filename
   and is taken literally.

   The mountpoint of the iPod (in local encoding) is expected in
   track->itdb->mountpoint.

   If @track->transferred is set to TRUE, nothing is done. Upon
   successful transfer @track->transferred is set to TRUE.

   For storage, the directories "f00 ... f19" will be
   cycled through.

   The filename is constructed as "gtkpod"<random number> and copied
   to @track->ipod_path. If this file already exists, <random number>
   is adjusted until an unused filename is found.

   If @track->ipod_path is already set, this one will be used
   instead. If a file with this name already exists, it will be
   overwritten. */
gboolean itdb_cp_track_to_ipod (Itdb_Track *track,
				gchar *filename, GError **error)
{
  static gint dir_num = -1;
  gchar *track_db_path, *ipod_fullfile;
  gboolean success;
  gint mplen = 0;
  const gchar *mp;
  Itdb_iTunesDB *itdb;

  g_return_val_if_fail (track, FALSE);
  g_return_val_if_fail (track->itdb, FALSE);
  g_return_val_if_fail (track->itdb->mountpoint, FALSE);
  g_return_val_if_fail (filename, FALSE);

  if(track->transferred)  return TRUE; /* nothing to do */ 

  mp = track->itdb->mountpoint;
  itdb = track->itdb;

  /* If track->ipod_path exists, we use that one instead. */
  ipod_fullfile = itdb_filename_on_ipod (track);

  if (!ipod_fullfile)
  {
      gchar *dest_components[] = {"iPod_Control", "Music",
				  NULL, NULL, NULL};
      gchar *parent_dir_filename;
      gchar *original_suffix;
      gchar dir_num_str[5];
      gint32 oops = 0;
      gint32 rand = g_random_int_range (0, 899999); /* 0 to 900000 */

      if (itdb_musicdirs_number (itdb) <= 0)
      {
	  gchar *str = g_build_filename (mp, dest_components[0],
					 dest_components[1], NULL);

	  g_set_error (error,
		       ITDB_FILE_ERROR,
		       ITDB_FILE_ERROR_NOTFOUND,
		       _("No 'F..' directories found in '%s'."),
		       str);
	  g_free (str);
	  return FALSE;
      }
		       
      if (dir_num == -1) dir_num = g_random_int_range (0, itdb->musicdirs);
      else dir_num = (dir_num + 1) % itdb_musicdirs_number (itdb);

      g_snprintf (dir_num_str, 5, "F%02d", dir_num);
      dest_components[2] = dir_num_str;
  
      parent_dir_filename =
	  itdb_resolve_path (mp, (const gchar **)dest_components);

      if(parent_dir_filename == NULL)
      {
	  /* Can't find the F%02d directory */
	  gchar *str = g_build_filename (mp, dest_components[0],
					 dest_components[1],
					 dest_components[2],
					 dest_components[3], NULL);
	  g_set_error (error,
		       ITDB_FILE_ERROR,
		       ITDB_FILE_ERROR_NOTFOUND,
		       _("Path not found: '%s'."),
		       str);
	  g_free (str);
	  return FALSE;
      }

      /* we need the original suffix of pcfile to construct a correct ipod
	 filename */
      original_suffix = strrchr (filename, '.');
      /* If there is no ".mp3", ".m4a" etc, set original_suffix to empty
	 string. Note: the iPod will most certainly ignore this file... */
      if (!original_suffix) original_suffix = "";

      do
      {   /* we need to loop until we find an unused filename */
	  dest_components[3] = 
	      g_strdup_printf("gtkpod%06d%s",
			      rand + oops, original_suffix);
	  ipod_fullfile = itdb_resolve_path (
	      parent_dir_filename,
	      (const gchar **)&dest_components[3]);
	  if(ipod_fullfile)
	  {   /* already exists -- try next */
              g_free(ipod_fullfile);
              ipod_fullfile = NULL;
	  }
	  else
	  {   /* found unused file -- build filename */
	      ipod_fullfile = g_build_filename (parent_dir_filename,
					        dest_components[3], NULL);
	  }
	  g_free (dest_components[3]);
	  ++oops;
      } while (!ipod_fullfile);
      g_free(parent_dir_filename);
  }
  /* now extract filepath for track->ipod_path from ipod_fullfile */
  /* ipod_path must begin with a '/' */
  mplen = strlen (mp); /* length of mountpoint in bytes */
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


/* Return the full iPod filename as stored in @track. Return value
   must be g_free()d after use.

   mount point of the iPod file system (in local encoding) is expected
   in track->itdb->mountpoint

   @track: track
   Return value: full filename to @track on the iPod or NULL if no
   filename is set in @track.

   NOTE: NULL is returned when the file does not exist.

   NOTE: this code works around a problem on some systems (see
   itdb_resolve_path() ) and might return a filename with different
   case than the original filename. Don't copy it back to @track
   unless you must */
gchar *itdb_filename_on_ipod (Itdb_Track *track)
{
    gchar *result = NULL;
    const gchar *mp;

    g_return_val_if_fail (track, NULL);
    g_return_val_if_fail (track->itdb, NULL);

    if (!track->itdb->mountpoint) return NULL;

    mp = track->itdb->mountpoint;

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


/* Copy file "from_file" to "to_file".
   Returns TRUE on success, FALSE otherwise */
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






/*------------------------------------------------------------------*\
 *                                                                  *
 *                       Timestamp stuff                            *
 *                                                                  *
\*------------------------------------------------------------------*/

guint64 itdb_time_get_mac_time (void)
{
    GTimeVal time;

    g_get_current_time (&time);
    return itdb_time_host_to_mac (time.tv_sec);
}


/* convert Macintosh timestamp to host system time stamp -- modify
 * this function if necessary to port to host systems with different
 * start of Epoch */
/* A "0" time will not be converted */
time_t itdb_time_mac_to_host (guint64 mactime)
{
    if (mactime != 0)  return (time_t)(mactime - 2082844800);
    else               return (time_t)mactime;
}


/* convert host system timestamp to Macintosh time stamp -- modify
 * this function if necessary to port to host systems with different
 * start of Epoch */
guint64 itdb_time_host_to_mac (time_t time)
{
    return (guint64)(((gint64)time) + 2082844800);
}

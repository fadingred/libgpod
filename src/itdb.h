/* Time-stamp: <2005-09-11 17:51:54 jcs>
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

#ifndef __ITUNESDB_H__
#define __ITUNESDB_H__

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <time.h>
#include <glib.h>


/* one star is how much (track->rating) */
#define ITDB_RATING_STEP 20

enum ItdbPlType { /* types for playlist->type */
    ITDB_PL_TYPE_NORM = 0,       /* normal playlist, visible in iPod */
    ITDB_PL_TYPE_MPL = 1         /* master playlist, contains all tracks,
				    not visible in iPod */
};



/* Most of the knowledge about smart playlists has been provided by
   Samuel "Otto" Wood (sam dot wood at gmail dot com) who let me dig
   in his impressive C++ class. Contact him for a complete
   copy. Further, all enums and #defines below, SPLRule, SPLRules, and
   SPLPref may also be used under a FreeBSD license. */

#define SPL_STRING_MAXLEN 255

/* Definitions for smart playlists */
enum { /* types for match_operator */
    SPLMATCH_AND = 0, /* AND rule - all of the rules must be true in
			 order for the combined rule to be applied */
    SPLMATCH_OR = 1   /* OR rule */
};

/* Limit Types.. like limit playlist to 100 minutes or to 100 songs */
enum {
    LIMITTYPE_MINUTES = 0x01,
    LIMITTYPE_MB      = 0x02,
    LIMITTYPE_SONGS   = 0x03,
    LIMITTYPE_HOURS   = 0x04,
    LIMITTYPE_GB      = 0x05
};

/* Limit Sorts.. Like which songs to pick when using a limit type
   Special note: the values for LIMITSORT_LEAST_RECENTLY_ADDED,
   LIMITSORT_LEAST_OFTEN_PLAYED, LIMITSORT_LEAST_RECENTLY_PLAYED, and
   LIMITSORT_LOWEST_RATING are really 0x10, 0x14, 0x15, 0x17, with the
   'limitsort_opposite' flag set.  This is the same value as the
   "positive" value (i.e. LIMITSORT_LEAST_RECENTLY_ADDED), and is
   really very terribly awfully weird, so we map the values to iPodDB
   specific values with the high bit set.

   On writing, we check the high bit and write the limitsort_opposite
   from that. That way, we don't have to deal with programs using the
   class needing to set the wrong limit and then make it into the
   "opposite", which would be frickin' annoying. */
enum {
    LIMITSORT_RANDOM = 0x02,
    LIMITSORT_SONG_NAME = 0x03,
    LIMITSORT_ALBUM = 0x04,
    LIMITSORT_ARTIST = 0x05,
    LIMITSORT_GENRE = 0x07,
    LIMITSORT_MOST_RECENTLY_ADDED = 0x10,
    LIMITSORT_LEAST_RECENTLY_ADDED = 0x80000010, /* See note above */
    LIMITSORT_MOST_OFTEN_PLAYED = 0x14,
    LIMITSORT_LEAST_OFTEN_PLAYED = 0x80000014,   /* See note above */
    LIMITSORT_MOST_RECENTLY_PLAYED = 0x15,
    LIMITSORT_LEAST_RECENTLY_PLAYED = 0x80000015,/* See note above */
    LIMITSORT_HIGHEST_RATING = 0x17,
    LIMITSORT_LOWEST_RATING = 0x80000017,        /* See note above */
};

/* Smartlist Actions - Used in the rules.
Note by Otto (Samuel Wood):
 really this is a bitmapped field...
 high byte
 bit 0 = "string" values if set, "int" values if not set
 bit 1 = "not", or to negate the check.
 lower 2 bytes
 bit 0 = simple "IS" query
 bit 1 = contains
 bit 2 = begins with
 bit 3 = ends with
 bit 4 = greater than
 bit 5 = unknown, but probably greater than or equal to
 bit 6 = less than
 bit 7 = unknown, but probably less than or equal to
 bit 8 = a range selection
 bit 9 = "in the last"
*/
typedef enum {
    SPLACTION_IS_INT = 0x00000001,          /* "Is Set" in iTunes */
    SPLACTION_IS_GREATER_THAN = 0x00000010, /* "Is After" in iTunes */
    SPLACTION_IS_LESS_THAN = 0x00000040,    /* "Is Before" in iTunes */
    SPLACTION_IS_IN_THE_RANGE = 0x00000100,
    SPLACTION_IS_IN_THE_LAST = 0x00000200,

    SPLACTION_IS_STRING = 0x01000001,
    SPLACTION_CONTAINS = 0x01000002,
    SPLACTION_STARTS_WITH = 0x01000004,
    SPLACTION_ENDS_WITH = 0x01000008,

    SPLACTION_IS_NOT_INT = 0x02000001,     /* "Is Not Set" in iTunes */

    /* Note: Not available in iTunes 4.5 (untested on iPod) */
    SPLACTION_IS_NOT_GREATER_THAN = 0x02000010,
    /* Note: Not available in iTunes 4.5 (untested on iPod) */
    SPLACTION_IS_NOT_LESS_THAN = 0x02000040,
    /* Note: Not available in iTunes 4.5 (seems to work on iPod) */
    SPLACTION_IS_NOT_IN_THE_RANGE = 0x02000100,

    SPLACTION_IS_NOT_IN_THE_LAST = 0x02000200,
    SPLACTION_IS_NOT = 0x03000001,
    SPLACTION_DOES_NOT_CONTAIN = 0x03000002,

    /* Note: Not available in iTunes 4.5 (seems to work on iPod) */
    SPLACTION_DOES_NOT_START_WITH = 0x03000004,
    /* Note: Not available in iTunes 4.5 (seems to work on iPod) */
    SPLACTION_DOES_NOT_END_WITH = 0x03000008,
} SPLAction;

typedef enum
{
    splft_string = 1,
    splft_int,
    splft_boolean,
    splft_date,
    splft_playlist,
    splft_unknown
} SPLFieldType;

typedef enum
{
    splat_string = 1,
    splat_int,
    splat_date,
    splat_range_int,
    splat_range_date,
    splat_inthelast,
    splat_playlist,
    splat_none,
    splat_invalid,
    splat_unknown
} SPLActionType;


/* These are to pass to AddRule() when you need a unit for the two "in
   the last" action types Or, in theory, you can use any time
   range... iTunes might not like it, but the iPod shouldn't care. */
enum {
    SPLACTION_LAST_DAYS_VALUE = 86400,    /* nr of secs in 24 hours */
    SPLACTION_LAST_WEEKS_VALUE = 604800,  /* nr of secs in 7 days   */
    SPLACTION_LAST_MONTHS_VALUE = 2628000,/* nr of secs in 30.4167
					     days ~= 1 month */
} ;

#if 0
// Hey, why limit ourselves to what iTunes can do? If the iPod can deal with it, excellent!
#define SPLACTION_LAST_HOURS_VALUE		3600		// number of seconds in 1 hour
#define SPLACTION_LAST_MINUTES_VALUE	60			// number of seconds in 1 minute
#define SPLACTION_LAST_YEARS_VALUE		31536000 	// number of seconds in 365 days

/* fun ones.. Near as I can tell, all of these work. It's open like that. :)*/
#define SPLACTION_LAST_LUNARCYCLE_VALUE	2551443		// a "lunar cycle" is the time it takes the moon to circle the earth
#define SPLACTION_LAST_SIDEREAL_DAY		86164		// a "sidereal day" is time in one revolution of earth on its axis
#define SPLACTION_LAST_SWATCH_BEAT		86			// a "swatch beat" is 1/1000th of a day.. search for "internet time" on google
#define SPLACTION_LAST_MOMENT			90			// a "moment" is 1/40th of an hour, or 1.5 minutes
#define SPLACTION_LAST_OSTENT			600			// an "ostent" is 1/10th of an hour, or 6 minutes
#define SPLACTION_LAST_FORTNIGHT		1209600 	// a "fortnight" is 14 days
#define SPLACTION_LAST_VINAL			1728000 	// a "vinal" is 20 days
#define SPLACTION_LAST_QUARTER			7889231		// a "quarter" is a quarter year
#define SPLACTION_LAST_SOLAR_YEAR       31556926 	// a "solar year" is the time it takes the earth to go around the sun
#define SPLACTION_LAST_SIDEREAL_YEAR 	31558150	// a "sidereal year" is the time it takes the earth to reach the same point in space again, compared to the stars
#endif

/* Smartlist fields - Used for rules. */
typedef enum {
    SPLFIELD_SONG_NAME = 0x02,    /* String */
    SPLFIELD_ALBUM = 0x03,        /* String */
    SPLFIELD_ARTIST = 0x04,       /* String */
    SPLFIELD_BITRATE = 0x05,      /* Int (e.g. from/to = 128) */
    SPLFIELD_SAMPLE_RATE = 0x06,  /* Int  (e.g. from/to = 44100) */
    SPLFIELD_YEAR = 0x07,         /* Int  (e.g. from/to = 2004) */
    SPLFIELD_GENRE = 0x08,        /* String */
    SPLFIELD_KIND = 0x09,         /* String */
    SPLFIELD_DATE_MODIFIED = 0x0a,/* Int/Mac Timestamp (e.g. from/to =
                                     bcf93280 == is before 6/19/2004)*/
    SPLFIELD_TRACKNUMBER = 0x0b,  /* Int (e.g. from = 1, to = 2) */
    SPLFIELD_SIZE = 0x0c,         /* Int (e.g. from/to = 0x00600000
				     for 6MB) */
    SPLFIELD_TIME = 0x0d,         /* Int (e.g. from/to = 83999 for
				     1:23/83 seconds) */
    SPLFIELD_COMMENT = 0x0e,      /* String */
    SPLFIELD_DATE_ADDED = 0x10,   /* Int/Mac Timestamp (e.g. from/to =
                                     bcfa83ff == is after 6/19/2004) */
    SPLFIELD_COMPOSER = 0x12,     /* String */
    SPLFIELD_PLAYCOUNT = 0x16,    /* Int  (e.g. from/to = 1) */
    SPLFIELD_LAST_PLAYED = 0x17,  /* Int/Mac Timestamp (e.g. from =
                                     bcfa83ff (6/19/2004) to =
                                     0xbcfbd57f (6/20/2004)) */
    SPLFIELD_DISC_NUMBER = 0x18,  /* Int  (e.g. from/to = 1) */
    SPLFIELD_RATING = 0x19,       /* Int/Stars Rating (e.g. from/to =
                                     60 (3 stars)) */
    SPLFIELD_COMPILATION = 0x1f,  /* Int (e.g. is set ->
				     SPLACTION_IS_INT/from=1,
				     is not set ->
				     SPLACTION_IS_NOT_INT/from=1) */
    SPLFIELD_BPM = 0x23,          /* Int  (e.g. from/to = 60) */
    SPLFIELD_GROUPING = 0x27,     /* String */
    SPLFIELD_PLAYLIST = 0x28,     /* XXX - Unknown...not parsed
				     correctly...from/to = 0xb6fbad5f
				     for * "Purchased Music".  Extra
				     data after * "to"... */
} SPLField;

#define SPLDATE_IDENTIFIER (G_GINT64_CONSTANT (0x2dae2dae2dae2daeU))

/* Maximum string length that iTunes writes to the database */
#define SPL_MAXSTRINGLENGTH 255

typedef struct SPLPref
{
    guint8  liveupdate;        /* "live Updating" check box */
    guint8  checkrules;        /* "Match X of the following
				  conditions" check box */
    guint8  checklimits;       /* "Limit To..." check box */
    guint32 limittype;         /* See types defined above */
    guint32 limitsort;         /* See types defined above */
    guint32 limitvalue;        /* The value typed next to "Limit
				  type" */
    guint8  matchcheckedonly;  /* "Match only checked songs" check
				  box */
} SPLPref;

typedef struct SPLRule
{
    guint32 field;
    guint32 action;
    gchar *string;             /* data in UTF8  */
    /* from and to are pretty stupid.. if it's a date type of field,
       then
         value = 0x2dae2dae2dae2dae,
         date = some number, like 2 or -2
         units = unit in seconds, like 604800 = a week
       but if this is actually some kind of integer comparison, like
       rating = 60 (3 stars)
         value = the value we care about
	 date = 0
	 units = 1 */
    guint64 fromvalue;
    gint64 fromdate;
    guint64 fromunits;
    guint64 tovalue;
    gint64 todate;
    guint64 tounits;
    guint32 unk052;
    guint32 unk056;
    guint32 unk060;
    guint32 unk064;
    guint32 unk068;
} SPLRule;


typedef struct SPLRules
{
    guint32 unk004;
    guint32 match_operator;  /* "All" (logical AND): SPLMATCH_AND,
				"Any" (logical OR): SPLMATCH_OR */
    GList *rules;
} SPLRules;


typedef void (* ItdbUserDataDestroyFunc) (gpointer userdata);
typedef gpointer (* ItdbUserDataDuplicateFunc) (gpointer userdata);

typedef struct
{
    GList *tracks;
    GList *playlists;
    gchar *filename;    /* filename of iTunesDB */
    gchar *mountpoint;  /* mountpoint of iPod (if available) */
    guint32 version;
    guint64 id;
    /* below is for use by application */
    guint64 usertype;
    gpointer userdata;
    /* function called to duplicate userdata */
    ItdbUserDataDuplicateFunc userdata_duplicate;
    /* function called to free userdata */
    ItdbUserDataDestroyFunc userdata_destroy;
} Itdb_iTunesDB;


typedef struct
{
    Itdb_iTunesDB *itdb;  /* pointer to iTunesDB (for convenience) */
    gchar *name;          /* name of playlist in UTF8      */
    guint32 type;         /* PL_TYPE_MPL: master play list */
    gint  num;            /* number of tracks in playlist  */
    GList *members;       /* tracks in playlist (Track *)  */
    gboolean is_spl;      /* smart playlist?               */
    guint32 timestamp;    /* some timestamp                */
    guint64 id;           /* playlist ID                   */
    guint32 unk036, unk040, unk044;
    SPLPref splpref;      /* smart playlist prefs          */
    SPLRules splrules;    /* rules for smart playlists     */
    /* below is for use by application */
    guint64 usertype;
    gpointer userdata;
    /* function called to duplicate userdata */
    ItdbUserDataDuplicateFunc userdata_duplicate;
    /* function called to free userdata */
    ItdbUserDataDestroyFunc userdata_destroy;
} Itdb_Playlist;


typedef struct
{
  Itdb_iTunesDB *itdb;       /* pointer to iTunesDB (for convenience) */
  gchar   *album;            /* album (utf8)           */
  gchar   *artist;           /* artist (utf8)          */
  gchar   *title;            /* title (utf8)           */
  gchar   *genre;            /* genre (utf8)           */
  gchar   *comment;          /* comment (utf8)         */
  gchar   *composer;         /* Composer (utf8)        */
  gchar   *fdesc;            /* eg. "MP3-File"...(utf8)*/
  gchar   *grouping;         /* ? (utf8)               */
  gchar   *ipod_path;        /* name of file on iPod: uses ":"
				instead of "/"                        */
  guint32 id;                /* unique ID of track     */
  gint32  size;              /* size of file in bytes  */
  gint32  tracklen;          /* Length of track in ms  */
  gint32  cd_nr;             /* CD number              */
  gint32  cds;               /* number of CDs          */
  gint32  track_nr;          /* track number           */
  gint32  tracks;            /* number of tracks       */
  gint32  bitrate;           /* bitrate                */
  guint16 samplerate;        /* samplerate (CD: 44100) */
  gint32  year;              /* year                   */
  gint32  volume;            /* volume adjustment              */
  guint32 soundcheck;        /* volume adjustment "soundcheck" */
  guint32 time_added;        /* time when added (Mac type)          */
  guint32 time_played;       /* time of last play (Mac type)        */
  guint32 time_modified;     /* time of last modification (Mac type)*/
  guint32 bookmark_time;     /* bookmark set for (AudioBook) in ms  */
  guint32 rating;            /* star rating (stars * RATING_STEP (20))     */
  guint32 playcount;         /* number of times track was played    */
  guint32 recent_playcount;  /* times track was played since last sync     */
  gboolean transferred;      /* has file been transferred to iPod?  */
  gint16 BPM;                /* supposed to vary the playback speed */
  guint8  app_rating;        /* star rating set by appl. (not iPod) */
  guint16 type;
  guint8  compilation;
  guint32 starttime;
  guint32 stoptime;
  guint8  checked;
  guint64 dbid;              /* unique database ID */
/* present in the mhit but not used by gtkpod yet */
  guint32 unk020, unk024, unk084, unk100, unk124;
  guint32 unk128, unk132, unk136, unk140, unk144, unk148, unk152;
  /* below is for use by application */
  guint64 usertype;
  gpointer userdata;
  /* function called to duplicate userdata */
  ItdbUserDataDuplicateFunc userdata_duplicate;
  /* function called to free userdata */
  ItdbUserDataDestroyFunc userdata_destroy;
} Itdb_Track;
/* (gtkpod note: don't forget to add fields read from the file to
 * copy_new_info() in file.c!) */

/* Error codes */
typedef enum
{
    ITDB_FILE_ERROR_SEEK,      /* file corrupt: illegal seek occured */
    ITDB_FILE_ERROR_CORRUPT,   /* file corrupt   */
    ITDB_FILE_ERROR_NOTFOUND,  /* file not found */
    ITDB_FILE_ERROR_RENAME,    /* file could not be renamed    */
    ITDB_FILE_ERROR_ITDB_CORRUPT /* iTunesDB in memory corrupt */
} ItdbFileError;

/* Error domain */
#define ITDB_FILE_ERROR itdb_file_error_quark ()
GQuark     itdb_file_error_quark      (void);

/* functions for reading/writing database, general itdb functions */
Itdb_iTunesDB *itdb_parse (const gchar *mp, GError **error);
Itdb_iTunesDB *itdb_parse_file (const gchar *filename, GError **error);
gboolean itdb_write (Itdb_iTunesDB *itdb, const gchar *mp, GError **error);
gboolean itdb_write_file (Itdb_iTunesDB *itdb, const gchar *filename,
			  GError **error);
gboolean itdb_shuffle_write (Itdb_iTunesDB *itdb,
			     const gchar *mp, GError **error);
gboolean itdb_shuffle_write_file (Itdb_iTunesDB *itdb,
				  const gchar *filename, GError **error);
Itdb_iTunesDB *itdb_new (void);
void itdb_free (Itdb_iTunesDB *itdb);
Itdb_iTunesDB *itdb_duplicate (Itdb_iTunesDB *itdb);
guint32 itdb_tracks_number (Itdb_iTunesDB *itdb);
guint32 itdb_tracks_number_nontransferred (Itdb_iTunesDB *itdb);
guint32 itdb_playlists_number (Itdb_iTunesDB *itdb);

/* general file functions */
gchar * itdb_resolve_path (const gchar *root,
			   const gchar * const * components);
gboolean itdb_rename_files (const gchar *mp, GError **error);
gboolean itdb_cp_track_to_ipod (const gchar *mp, Itdb_Track *track,
				gchar *filename, GError **error);
gboolean itdb_cp (const gchar *from_file, const gchar *to_file,
		  GError **error);
void itdb_filename_fs2ipod (gchar *filename);
void itdb_filename_ipod2fs (gchar *ipod_file);
gchar *itdb_filename_on_ipod (const gchar *mp, Itdb_Track *track);

/* track functions */
Itdb_Track *itdb_track_new (void);
void itdb_track_free (Itdb_Track *track);
void itdb_track_add (Itdb_iTunesDB *itdb, Itdb_Track *track, gint32 pos);
void itdb_track_remove (Itdb_Track *track);
void itdb_track_unlink (Itdb_Track *track);
Itdb_Track *itdb_track_duplicate (Itdb_Track *tr);
Itdb_Track *itdb_track_by_id (Itdb_iTunesDB *itdb, guint32 id);
GTree *itdb_track_id_tree_create (Itdb_iTunesDB *itdb);
void itdb_track_id_tree_destroy (GTree *idtree);
Itdb_Track *itdb_track_id_tree_by_id (GTree *idtree, guint32 id);

/* playlist functions */
Itdb_Playlist *itdb_playlist_new (const gchar *title, gboolean spl);
void itdb_playlist_free (Itdb_Playlist *pl);
void itdb_playlist_add (Itdb_iTunesDB *itdb, Itdb_Playlist *pl, gint32 pos);
void itdb_playlist_move (Itdb_Playlist *pl, guint32 pos);
void itdb_playlist_remove (Itdb_Playlist *pl);
void itdb_playlist_unlink (Itdb_Playlist *pl);
Itdb_Playlist *itdb_playlist_duplicate (Itdb_Playlist *pl);
gboolean itdb_playlist_exists (Itdb_iTunesDB *itdb, Itdb_Playlist *pl);
void itdb_playlist_add_track (Itdb_Playlist *pl,
			      Itdb_Track *track, gint32 pos);
Itdb_Playlist *itdb_playlist_by_id (Itdb_iTunesDB *itdb, guint64 id);
Itdb_Playlist *itdb_playlist_by_nr (Itdb_iTunesDB *itdb, guint32 num);
Itdb_Playlist *itdb_playlist_by_name (Itdb_iTunesDB *itdb, gchar *name);
Itdb_Playlist *itdb_playlist_mpl (Itdb_iTunesDB *itdb);
gboolean itdb_playlist_contains_track (Itdb_Playlist *pl, Itdb_Track *track);
guint32 itdb_playlist_contain_track_number (Itdb_Track *tr);
void itdb_playlist_remove_track (Itdb_Playlist *pl, Itdb_Track *track);
guint32 itdb_playlist_tracks_number (Itdb_Playlist *pl);
void itdb_playlist_randomize (Itdb_Playlist *pl);

/* smart playlist functions */
SPLFieldType itdb_splr_get_field_type (const SPLRule *splr);
SPLActionType itdb_splr_get_action_type (const SPLRule *splr);
void itdb_splr_validate (SPLRule *splr);
void itdb_splr_remove (Itdb_Playlist *pl, SPLRule *splr);
SPLRule *itdb_splr_new (void);
void itdb_splr_add (Itdb_Playlist *pl, SPLRule *splr, gint pos);
SPLRule *itdb_splr_add_new (Itdb_Playlist *pl, gint pos);
void itdb_spl_copy_rules (Itdb_Playlist *dest, Itdb_Playlist *src);
gboolean itdb_splr_eval (Itdb_iTunesDB *itdb, SPLRule *splr, Itdb_Track *track);
void itdb_spl_update (Itdb_iTunesDB *itdb, Itdb_Playlist *spl);
void itdb_spl_update_all (Itdb_iTunesDB *itdb);

/* time functions */
guint64 itdb_time_get_mac_time (void);
time_t itdb_time_mac_to_host (guint64 mactime);
guint64 itdb_time_host_to_mac (time_t time);

#endif

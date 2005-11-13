/* Time-stamp: <2005-11-13 14:11:50 jcs>
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

#include "ipod-device.h"

#include <sys/types.h>
#include <time.h>
#include <glib.h>

G_BEGIN_DECLS

/* one star is how much (track->rating) */
#define ITDB_RATING_STEP 20

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

enum ItdbImageType {
	ITDB_IMAGE_FULL_SCREEN,
	ITDB_IMAGE_NOW_PLAYING
};


/* This structure can represent two slightly different images:
 *   - an image before it's transferred to the iPod (it will then be scaled
 *     as necessary to generate the 2 thumbnails needed by the iPod), 
 *     for such images, filename points to a 'real' image file, offset is
 *     not significant, size, width and height may or may not be set
 *     and id corresponds to the image id to write in mhii records of the 
 *     photo database
 * 
 *   - a thumbnail (big or small) stored on a database in the iPod.
 *     For such images, id isn't significant, filename point to a .ithmb file
 *     on the iPod
 */
struct _Itdb_Image {
	enum ItdbImageType type;
  	char *filename;
	guint32 offset;
	guint32 size;
	gint16 width;
	gint16 height;
};

typedef struct _Itdb_Image Itdb_Image;

typedef void (* ItdbUserDataDestroyFunc) (gpointer userdata);
typedef gpointer (* ItdbUserDataDuplicateFunc) (gpointer userdata);

typedef struct
{
    GList *tracks;
    GList *playlists;
    gchar *filename;    /* filename of iTunesDB */
    gchar *mountpoint;  /* mountpoint of iPod (if available) */
    IpodDevice *device;
    gint   musicdirs;   /* number of /iPod_Control/Music/F.. dirs */
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
    gchar *name;          /* name of playlist in UTF8              */
    guint32 type;         /* ITDB_PL_TYPE_NORM/_MPL                */
    gint  num;            /* number of tracks in playlist          */
    GList *members;       /* tracks in playlist (Track *)          */
    gboolean is_spl;      /* smart playlist?                       */
    guint32 timestamp;    /* some timestamp                        */
    guint64 id;           /* playlist ID                           */
    guint32 mhodcount;    /* This appears to be the number of string
			     MHODs (type < 50) associated with this
			     playlist (typically 0x01). Doesn't seem
			     to be signficant unless you include Type
			     52 MHODs. libgpod sets this to 1 when
			     syncing */
    guint16 libmhodcount; /* The number of Type 52 MHODs associated
			     with this playlist. If you don't create
			     Type 52 MHODs, this can be
			     zero. Otherwise, if you have Type 52
			     MHODs associated with this playlist and
			     set this to zero, no songs appear on the
			     iPod. jcsjcs: with iTunes 4.9 this seems
			     to be set to 1 even without any Type 52
			     MHODs present. libgpod sets this to 1
			     when syncing */
    guint32 sortorder;    /* How to sort playlist -- see below     */
    guint32 podcastflag;  /* ITDB_PL_FLAG_NORM/_PODCAST            */
    SPLPref splpref;      /* smart playlist prefs                  */
    SPLRules splrules;    /* rules for smart playlists             */
    /* below is for use by application */
    guint64 usertype;
    gpointer userdata;
    /* function called to duplicate userdata */
    ItdbUserDataDuplicateFunc userdata_duplicate;
    /* function called to free userdata */
    ItdbUserDataDestroyFunc userdata_destroy;
} Itdb_Playlist;


/*
Playlist Sort Order

1 - playlist order (manual sort order)
2 - ???
3 - songtitle
4 - album
5 - artist
6 - bitrate
7 - genre
8 - kind
9 - date modified
10 - track nr
11 - size
12 - time
13 - year
14 - sample rate
15 - comment
16 - date added
17 - equalizer
18 - composer
19 - ???
20 - play count
21 - last played
22 - disc nr
23 - my rating
24 - release date (I guess, it's the value for the "Podcasts" list)
25 - BPM
26 - grouping
27 - category
28 - description
*/

typedef enum
{
    ITDB_PSO_MANUAL = 1,
/*    ITDB_PSO_UNKNOWN = 2, */
    ITDB_PSO_TITLE = 3,
    ITDB_PSO_ALBUM = 4,
    ITDB_PSO_ARTIST = 5,
    ITDB_PSO_BIRATE = 6,
    ITDB_PSO_GENRE = 7,
    ITDB_PSO_FILETYPE = 8,
    ITDB_PSO_TIME_MODIFIED = 9,
    ITDB_PSO_TRACK_NR = 10,
    ITDB_PSO_SIZE = 11,
    ITDB_PSO_TIME = 12,  /* ? */
    ITDB_PSO_YEAR = 13,
    ITDB_PSO_SAMPLERATE = 14,
    ITDB_PSO_COMMENT = 15,
    ITDB_PSO_TIME_ADDED = 16,
    ITDB_PSO_EQUALIZER = 17, /* ? */
    ITDB_PSO_COMPOSER = 18,
/*    ITDB_PSO_UNKNOWN = 19, */
    ITDB_PSO_PLAYCOUNT = 20,
    ITDB_PSO_TIME_PLAYED = 21,
    ITDB_PSO_CD_NR = 22,
    ITDB_PSO_RATING = 23,
    ITDB_PSO_RELEASE_DATE = 24, /* used by podcasts */
    ITDB_PSO_BPM = 25,
    ITDB_PSO_GROUPING = 26,
    ITDB_PSO_CATEGORY = 27,
    ITDB_PSO_DESCRIPTION = 28
} ItdbPlaylistSortOrder;


/* some of the descriptive comments below are copied verbatim from
   http://ipodlinux.org/ITunesDB. 
   http://ipodlinux.org/ITunesDB is the best source for information
   about the iTunesDB and related files. */
typedef struct
{
  Itdb_iTunesDB *itdb;       /* pointer to iTunesDB (for convenience) */
  gchar   *title;            /* title (utf8)           */
  gchar   *ipod_path;        /* name of file on iPod: uses ":"
				instead of "/"                        */
  gchar   *album;            /* album (utf8)           */
  gchar   *artist;           /* artist (utf8)          */
  gchar   *genre;            /* genre (utf8)           */
  gchar   *filetype;         /* eg. "MP3-File"...(utf8)*/
  gchar   *comment;          /* comment (utf8)         */
  gchar   *category;         /* Category for podcast   */
  gchar   *composer;         /* Composer (utf8)        */
  gchar   *grouping;         /* ? (utf8)               */
  gchar   *description;      /* see note for MHOD_ID in itdb_itunesdb.c */
  gchar   *podcasturl;       /* see note for MHOD_ID in itdb_itunesdb.c */
  gchar   *podcastrss;       /* see note for MHOD_ID in itdb_itunesdb.c */
  gchar   *chapterdata;      /* see note for MHOD_ID in itdb_itunesdb.c */
  gchar   *subtitle;         /* see note for MHOD_ID in itdb_itunesdb.c */
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
  guint32 playcount2;        /* Also stores the play count of the
				song.  Don't know if it ever differs
				from the above value. During sync itdb
				sets playcount2 to the same value as
				playcount. */
  guint32 recent_playcount;  /* times track was played since last sync */
  gboolean transferred;      /* has file been transferred to iPod?  */
  gint16 BPM;                /* supposed to vary the playback speed */
  guint8  app_rating;        /* star rating set by appl. (not
			      * iPod). If the rating set on the iPod
			        and the rating field above differ, the
				original rating is copied here and the
				new rating is stored above. */
  guint16 type;              /* CBR MP3s are type 0x100, VBR MP3s are
			        type 0x101, AAC are type 0x0 */
  guint8  compilation;
  guint32 starttime;
  guint32 stoptime;
  guint8  checked;
  guint64 dbid;              /* unique database ID */
  guint32 drm_userid;        /* Apple Store/Audible User ID (for DRM'ed
				files only, set to 0 otherwise). */
  guint32 visible;           /*  If this value is 1, the song is visible
				 on the iPod. All other values cause
				 the file to be hidden. */
  gchar filetype_marker[4];  /* This appears to always be 0 on hard
                                drive based iPods, but for the
                                iTunesDB that is written to an iPod
                                Shuffle, iTunes 4.7.1 writes out the
                                file's type as an ANSI string(!). For
                                example, a MP3 file has a filetype of
                                0x4d503320 -> 0x4d = 'M', 0x50 = 'P',
                                0x33 = '3', 0x20 = <space>. (always
                                left set to 0 by itdb)*/
  guint16 artwork_count;     /*  The number of album artwork items
				 associated with this song. */
  guint32 artwork_size;      /* The total size of artwork (in bytes)
				attached to this song, when it is
				converted to JPEG format. Observed in
				iPodDB version 0x0b and with an iPod
				Photo. */
  float samplerate2;         /* The sample rate of the song expressed
				as an IEEE 32 bit floating point
				number.  It's uncertain why this is
				here.  itdb will set this when adding
				a track */

  guint16 unk060;     /* unknown */
  guint16 unk126;     /* unknown, but always seems to be 0xffff for
			 MP3/AAC songs, 0x0 for uncompressed songs
			 (like WAVE format), 0x1 for Audible. itdb
			 will try to set this when adding a new track */
  guint32 unk132;     /* unknown */
  guint32 time_released;/* date/time added to music store? definitely a
			 timestamp, always appears to be a time of
			 0700 GMT. For podcasts: release date as
			 displayed next to the title in the Podcast
			 playlist  */
  guint32 unk144;     /* unknown, but MP3 songs appear to be always
			 0x0000000c or 0x0100000c (if played one or
			 more times in iTunes), AAC songs are always
			 0x01000033, Audible files are 0x01000029, WAV
			 files are 0x0. itdb will attempt to set this
			 value when adding a track. */  
  guint32 unk148;     /* unknown - used for Apple Store DRM songs
			 (always 0x01010100?), zero otherwise */
  guint32 unk152;     /* unknown */
  guint32 unk156, unk160;
  guint32 unk164;     /* unknown (0x02?) in dbversion 0x0d, for
			 podcasts this seems to be set to
			 0x01010102. The 0x10000 bit seems to control
			 whether or not iTunes remembers the last
			 played position of this song. This will work
			 on any song, not just podcasts.
			 Assumption:
			 Audiobooks also get this bit set by
			 default. Assumption2: This is really four
			 single byte flags, perhaps? Might want to try
			 find various examples and compare them. */
  guint64 dbid2;      /* not clear. if not set, itdb will set this to
			 the same value as dbid when adding a track */
  guint32 unk176;     /* unknown - added in dbversion 0x0c, first
			 values observed in 0x0d. Observed to be
			 0x00010000 for non-podcasts and 0x00020000
			 for a podcast. */
  guint32 unk180, unk184;
  guint32 samplecount;/* Number of samples in the song. First observed
			 in dbversion 0x0d, and only for AAC and WAV
			 files (not MP3?!?). */
  guint32 unk192, unk196, unk200;
  guint32 unk204;     /*  unknown - added in dbversion 0x0c, first
			  values observed in 0x0d. Observed to be 0x0
			  or 0x1. */
  guint32 unk208, unk212, unk216, unk220, unk224;
  guint32 unk228, unk232, unk236, unk240;

    /* Chapter data: defines where the chapter stops are in the track,
       as well as what info should be displayed for each section of
       the track. Until it can be parsed and interpreted, the
       chapterdata will just be read as a block and written back on
       sync. This will be changed at a later time */
  void *chapterdata_raw;
  guint32 chapterdata_raw_length;

  /* This is for Cover Art support */
  GList *thumbnails;
  unsigned int image_id;
  char *orig_image_filename;

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
gboolean itdb_write (Itdb_iTunesDB *itdb, GError **error);
gboolean itdb_write_file (Itdb_iTunesDB *itdb, const gchar *filename,
			  GError **error);
gboolean itdb_shuffle_write (Itdb_iTunesDB *itdb, GError **error);
gboolean itdb_shuffle_write_file (Itdb_iTunesDB *itdb,
				  const gchar *filename, GError **error);
Itdb_iTunesDB *itdb_new (void);
void itdb_free (Itdb_iTunesDB *itdb);
Itdb_iTunesDB *itdb_duplicate (Itdb_iTunesDB *itdb); /* not implemented */
guint32 itdb_tracks_number (Itdb_iTunesDB *itdb);
guint32 itdb_tracks_number_nontransferred (Itdb_iTunesDB *itdb);
guint32 itdb_playlists_number (Itdb_iTunesDB *itdb);

/* general file functions */
gint itdb_musicdirs_number (Itdb_iTunesDB *itdb);
gchar * itdb_resolve_path (const gchar *root,
			   const gchar * const * components);
gboolean itdb_rename_files (const gchar *mp, GError **error);
gboolean itdb_cp_track_to_ipod (Itdb_Track *track,
				gchar *filename, GError **error);
gboolean itdb_cp (const gchar *from_file, const gchar *to_file,
		  GError **error);
void itdb_filename_fs2ipod (gchar *filename);
void itdb_filename_ipod2fs (gchar *ipod_file);
gchar *itdb_filename_on_ipod (Itdb_Track *track);
void itdb_set_mountpoint (Itdb_iTunesDB *itdb, const gchar *mp);

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
gboolean itdb_playlist_contains_track (Itdb_Playlist *pl, Itdb_Track *track);
guint32 itdb_playlist_contain_track_number (Itdb_Track *tr);
void itdb_playlist_remove_track (Itdb_Playlist *pl, Itdb_Track *track);
guint32 itdb_playlist_tracks_number (Itdb_Playlist *pl);
void itdb_playlist_randomize (Itdb_Playlist *pl);

/* playlist functions for master playlist */
Itdb_Playlist *itdb_playlist_mpl (Itdb_iTunesDB *itdb);
gboolean itdb_playlist_is_mpl (Itdb_Playlist *pl);
void itdb_playlist_set_mpl (Itdb_Playlist *pl);

/* playlist functions for podcast playlist */
Itdb_Playlist *itdb_playlist_podcasts (Itdb_iTunesDB *itdb);
gboolean itdb_playlist_is_podcasts (Itdb_Playlist *pl);
void itdb_playlist_set_podcasts (Itdb_Playlist *pl);

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

/* thumbnails functions */
unsigned char *itdb_image_get_rgb_data (Itdb_Image *image);
int itdb_track_set_thumbnail (Itdb_Track *song, const char *filename);
void itdb_track_remove_thumbnail (Itdb_Track *song);
void itdb_track_free_generated_thumbnails (Itdb_Track *track);

/* time functions */
guint64 itdb_time_get_mac_time (void);
time_t itdb_time_mac_to_host (guint64 mactime);
guint64 itdb_time_host_to_mac (time_t time);

G_END_DECLS

#endif

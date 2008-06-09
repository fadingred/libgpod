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

#ifndef __ITUNESDB_H__
#define __ITUNESDB_H__

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <sys/types.h>
#include <time.h>
#include <glib.h>

G_BEGIN_DECLS

/* G_GNUC_INTERNAL is defined in glib 2.6 */
#ifndef G_GNUC_INTERNAL
#define G_GNUC_INTERNAL
#endif 

typedef void (* ItdbUserDataDestroyFunc) (gpointer userdata);
typedef gpointer (* ItdbUserDataDuplicateFunc) (gpointer userdata);

/* public structures */
typedef struct _Itdb_Device Itdb_Device;
typedef struct _Itdb_IpodInfo Itdb_IpodInfo;
typedef struct _Itdb_Artwork Itdb_Artwork;
typedef struct _Itdb_ArtworkFormat Itdb_ArtworkFormat;
typedef struct _Itdb_Thumb Itdb_Thumb;
typedef struct _Itdb_SPLPref Itdb_SPLPref;
typedef struct _Itdb_SPLRule Itdb_SPLRule;
typedef struct _Itdb_SPLRules Itdb_SPLRules;
typedef struct _Itdb_iTunesDB Itdb_iTunesDB;
typedef struct _Itdb_PhotoDB Itdb_PhotoDB;
typedef struct _Itdb_Playlist Itdb_Playlist;
typedef struct _Itdb_PhotoAlbum Itdb_PhotoAlbum;
typedef struct _Itdb_Track Itdb_Track;
typedef struct _Itdb_Chapter Itdb_Chapter;
typedef struct _Itdb_Chapterdata Itdb_Chapterdata;


/* ------------------------------------------------------------ *\
 *
 * iPod model-relevant definitions
 *
\* ------------------------------------------------------------ */

typedef enum {
    ITDB_IPOD_GENERATION_UNKNOWN,
    ITDB_IPOD_GENERATION_FIRST,
    ITDB_IPOD_GENERATION_SECOND,
    ITDB_IPOD_GENERATION_THIRD,
    ITDB_IPOD_GENERATION_FOURTH,
    ITDB_IPOD_GENERATION_PHOTO,
    ITDB_IPOD_GENERATION_MOBILE,
    ITDB_IPOD_GENERATION_MINI_1,
    ITDB_IPOD_GENERATION_MINI_2,
    ITDB_IPOD_GENERATION_SHUFFLE_1,
    ITDB_IPOD_GENERATION_SHUFFLE_2,
    ITDB_IPOD_GENERATION_SHUFFLE_3,
    ITDB_IPOD_GENERATION_NANO_1,
    ITDB_IPOD_GENERATION_NANO_2,
    ITDB_IPOD_GENERATION_NANO_3,
    ITDB_IPOD_GENERATION_VIDEO_1,
    ITDB_IPOD_GENERATION_VIDEO_2,
    ITDB_IPOD_GENERATION_CLASSIC_1,
    ITDB_IPOD_GENERATION_TOUCH_1,
    ITDB_IPOD_GENERATION_IPHONE_1,
} Itdb_IpodGeneration;

typedef enum {
    ITDB_IPOD_MODEL_INVALID,
    ITDB_IPOD_MODEL_UNKNOWN,
    ITDB_IPOD_MODEL_COLOR,
    ITDB_IPOD_MODEL_COLOR_U2,
    ITDB_IPOD_MODEL_REGULAR,
    ITDB_IPOD_MODEL_REGULAR_U2,
    ITDB_IPOD_MODEL_MINI,
    ITDB_IPOD_MODEL_MINI_BLUE,
    ITDB_IPOD_MODEL_MINI_PINK,
    ITDB_IPOD_MODEL_MINI_GREEN,
    ITDB_IPOD_MODEL_MINI_GOLD,
    ITDB_IPOD_MODEL_SHUFFLE,
    ITDB_IPOD_MODEL_NANO_WHITE,
    ITDB_IPOD_MODEL_NANO_BLACK,
    ITDB_IPOD_MODEL_VIDEO_WHITE,
    ITDB_IPOD_MODEL_VIDEO_BLACK,
    ITDB_IPOD_MODEL_MOBILE_1,
    ITDB_IPOD_MODEL_VIDEO_U2,
    ITDB_IPOD_MODEL_NANO_SILVER,
    ITDB_IPOD_MODEL_NANO_BLUE,
    ITDB_IPOD_MODEL_NANO_GREEN,
    ITDB_IPOD_MODEL_NANO_PINK,
    ITDB_IPOD_MODEL_NANO_RED,
    ITDB_IPOD_MODEL_IPHONE_1,
    ITDB_IPOD_MODEL_SHUFFLE_SILVER,
    ITDB_IPOD_MODEL_SHUFFLE_PINK,
    ITDB_IPOD_MODEL_SHUFFLE_BLUE,
    ITDB_IPOD_MODEL_SHUFFLE_GREEN,
    ITDB_IPOD_MODEL_SHUFFLE_ORANGE,
    ITDB_IPOD_MODEL_SHUFFLE_PURPLE,
    ITDB_IPOD_MODEL_CLASSIC_SILVER,
    ITDB_IPOD_MODEL_CLASSIC_BLACK,
    ITDB_IPOD_MODEL_TOUCH_BLACK,
} Itdb_IpodModel;

struct _Itdb_IpodInfo {
    /* model_number is abbreviated: if the first character is not
       numeric, it is ommited. e.g. "MA350 -> A350", "M9829 -> 9829" */
    const gchar *model_number;
    const double capacity;  /* in GB */
    const Itdb_IpodModel ipod_model;
    const Itdb_IpodGeneration ipod_generation;
    /* Number of music (Fnn) dirs created by iTunes. The exact number
       seems to be version dependent. Therefore, the numbers here
       represent a mixture of reported values and common sense. Note:
       this number does not necessarily represent the number of dirs
       present on a particular iPod. It is used when setting up a new
       iPod from scratch. */
    const guint musicdirs;
    /* reserved for future use */
    const gint32 reserved_int1;
    const gint32 reserved_int2;
    gconstpointer reserved1;
    gconstpointer reserved2;
};


/* ------------------------------------------------------------ *\
 *
 * Smart Playlists (Rules)
 *
\* ------------------------------------------------------------ */


/* Most of the knowledge about smart playlists has been provided by
   Samuel "Otto" Wood (sam dot wood at gmail dot com) who let me dig
   in his impressive C++ class. Contact him for a complete
   copy. Further, all enums and #defines below, Itdb_SPLRule, Itdb_SPLRules, and
   Itdb_SPLPref may also be used under a FreeBSD license. */

#define ITDB_SPL_STRING_MAXLEN 255
#define ITDB_SPL_DATE_IDENTIFIER (G_GINT64_CONSTANT (0x2dae2dae2dae2daeU))

/* Definitions for smart playlists */
typedef enum { /* types for match_operator */
    ITDB_SPLMATCH_AND = 0, /* AND rule - all of the rules must be true in
			 order for the combined rule to be applied */
    ITDB_SPLMATCH_OR = 1   /* OR rule */
} ItdbSPLMatch;

/* Limit Types.. like limit playlist to 100 minutes or to 100 songs */
typedef enum {
    ITDB_LIMITTYPE_MINUTES = 0x01,
    ITDB_LIMITTYPE_MB      = 0x02,
    ITDB_LIMITTYPE_SONGS   = 0x03,
    ITDB_LIMITTYPE_HOURS   = 0x04,
    ITDB_LIMITTYPE_GB      = 0x05
} ItdbLimitType;

/* Limit Sorts.. Like which songs to pick when using a limit type
   Special note: the values for ITDB_LIMITSORT_LEAST_RECENTLY_ADDED,
   ITDB_LIMITSORT_LEAST_OFTEN_PLAYED, ITDB_LIMITSORT_LEAST_RECENTLY_PLAYED, and
   ITDB_LIMITSORT_LOWEST_RATING are really 0x10, 0x14, 0x15, 0x17, with the
   'limitsort_opposite' flag set.  This is the same value as the
   "positive" value (i.e. ITDB_LIMITSORT_LEAST_RECENTLY_ADDED), and is
   really very terribly awfully weird, so we map the values to iPodDB
   specific values with the high bit set.

   On writing, we check the high bit and write the limitsort_opposite
   from that. That way, we don't have to deal with programs using the
   class needing to set the wrong limit and then make it into the
   "opposite", which would be frickin' annoying. */
typedef enum {
    ITDB_LIMITSORT_RANDOM = 0x02,
    ITDB_LIMITSORT_SONG_NAME = 0x03,
    ITDB_LIMITSORT_ALBUM = 0x04,
    ITDB_LIMITSORT_ARTIST = 0x05,
    ITDB_LIMITSORT_GENRE = 0x07,
    ITDB_LIMITSORT_MOST_RECENTLY_ADDED = 0x10,
    ITDB_LIMITSORT_LEAST_RECENTLY_ADDED = 0x80000010, /* See note above */
    ITDB_LIMITSORT_MOST_OFTEN_PLAYED = 0x14,
    ITDB_LIMITSORT_LEAST_OFTEN_PLAYED = 0x80000014,   /* See note above */
    ITDB_LIMITSORT_MOST_RECENTLY_PLAYED = 0x15,
    ITDB_LIMITSORT_LEAST_RECENTLY_PLAYED = 0x80000015,/* See note above */
    ITDB_LIMITSORT_HIGHEST_RATING = 0x17,
    ITDB_LIMITSORT_LOWEST_RATING = 0x80000017,        /* See note above */
} ItdbLimitSort;

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
    ITDB_SPLACTION_IS_INT = 0x00000001,          /* "Is Set" in iTunes */
    ITDB_SPLACTION_IS_GREATER_THAN = 0x00000010, /* "Is After" in iTunes */
    ITDB_SPLACTION_IS_LESS_THAN = 0x00000040,    /* "Is Before" in iTunes */
    ITDB_SPLACTION_IS_IN_THE_RANGE = 0x00000100,
    ITDB_SPLACTION_IS_IN_THE_LAST = 0x00000200,
    ITDB_SPLACTION_BINARY_AND = 0x00000400,

    ITDB_SPLACTION_IS_STRING = 0x01000001,
    ITDB_SPLACTION_CONTAINS = 0x01000002,
    ITDB_SPLACTION_STARTS_WITH = 0x01000004,
    ITDB_SPLACTION_ENDS_WITH = 0x01000008,

    ITDB_SPLACTION_IS_NOT_INT = 0x02000001,     /* "Is Not Set" in iTunes */

    /* Note: Not available in iTunes 4.5 (untested on iPod) */
    ITDB_SPLACTION_IS_NOT_GREATER_THAN = 0x02000010,
    /* Note: Not available in iTunes 4.5 (untested on iPod) */
    ITDB_SPLACTION_IS_NOT_LESS_THAN = 0x02000040,
    /* Note: Not available in iTunes 4.5 (seems to work on iPod) */
    ITDB_SPLACTION_IS_NOT_IN_THE_RANGE = 0x02000100,

    ITDB_SPLACTION_IS_NOT_IN_THE_LAST = 0x02000200,
    ITDB_SPLACTION_IS_NOT = 0x03000001,
    ITDB_SPLACTION_DOES_NOT_CONTAIN = 0x03000002,

    /* Note: Not available in iTunes 4.5 (seems to work on iPod) */
    ITDB_SPLACTION_DOES_NOT_START_WITH = 0x03000004,
    /* Note: Not available in iTunes 4.5 (seems to work on iPod) */
    ITDB_SPLACTION_DOES_NOT_END_WITH = 0x03000008,
} ItdbSPLAction;

typedef enum
{
    ITDB_SPLFT_STRING = 1,
    ITDB_SPLFT_INT,
    ITDB_SPLFT_BOOLEAN,
    ITDB_SPLFT_DATE,
    ITDB_SPLFT_PLAYLIST,
    ITDB_SPLFT_UNKNOWN,
    ITDB_SPLFT_BINARY_AND
} ItdbSPLFieldType;

typedef enum
{
    ITDB_SPLAT_STRING = 1,
    ITDB_SPLAT_INT,
    ITDB_SPLAT_DATE,
    ITDB_SPLAT_RANGE_INT,
    ITDB_SPLAT_RANGE_DATE,
    ITDB_SPLAT_INTHELAST,
    ITDB_SPLAT_PLAYLIST,
    ITDB_SPLAT_NONE,
    ITDB_SPLAT_INVALID,
    ITDB_SPLAT_UNKNOWN,
    ITDB_SPLAT_BINARY_AND
} ItdbSPLActionType;


/* These are to pass to AddRule() when you need a unit for the two "in
   the last" action types Or, in theory, you can use any time
   range... iTunes might not like it, but the iPod shouldn't care. */
typedef enum {
    ITDB_SPLACTION_LAST_DAYS_VALUE = 86400,    /* nr of secs in 24 hours */
    ITDB_SPLACTION_LAST_WEEKS_VALUE = 604800,  /* nr of secs in 7 days   */
    ITDB_SPLACTION_LAST_MONTHS_VALUE = 2628000,/* nr of secs in 30.4167
					     days ~= 1 month */
} ItdbSPLActionLast;

#if 0
// Hey, why limit ourselves to what iTunes can do? If the iPod can deal with it, excellent!
#define ITDB_SPLACTION_LAST_HOURS_VALUE		3600		// number of seconds in 1 hour
#define ITDB_SPLACTION_LAST_MINUTES_VALUE	60			// number of seconds in 1 minute
#define ITDB_SPLACTION_LAST_YEARS_VALUE		31536000 	// number of seconds in 365 days

/* fun ones.. Near as I can tell, all of these work. It's open like that. :)*/
#define ITDB_SPLACTION_LAST_LUNARCYCLE_VALUE	2551443		// a "lunar cycle" is the time it takes the moon to circle the earth
#define ITDB_SPLACTION_LAST_SIDEREAL_DAY		86164		// a "sidereal day" is time in one revolution of earth on its axis
#define ITDB_SPLACTION_LAST_SWATCH_BEAT		86			// a "swatch beat" is 1/1000th of a day.. search for "internet time" on google
#define ITDB_SPLACTION_LAST_MOMENT			90			// a "moment" is 1/40th of an hour, or 1.5 minutes
#define ITDB_SPLACTION_LAST_OSTENT			600			// an "ostent" is 1/10th of an hour, or 6 minutes
#define ITDB_SPLACTION_LAST_FORTNIGHT		1209600 	// a "fortnight" is 14 days
#define ITDB_SPLACTION_LAST_VINAL			1728000 	// a "vinal" is 20 days
#define ITDB_SPLACTION_LAST_QUARTER			7889231		// a "quarter" is a quarter year
#define ITDB_SPLACTION_LAST_SOLAR_YEAR       31556926 	// a "solar year" is the time it takes the earth to go around the sun
#define ITDB_SPLACTION_LAST_SIDEREAL_YEAR 	31558150	// a "sidereal year" is the time it takes the earth to reach the same point in space again, compared to the stars
#endif

/* Smartlist fields - Used for rules. */
typedef enum {
    ITDB_SPLFIELD_SONG_NAME = 0x02,    /* String */
    ITDB_SPLFIELD_ALBUM = 0x03,        /* String */
    ITDB_SPLFIELD_ARTIST = 0x04,       /* String */
    ITDB_SPLFIELD_BITRATE = 0x05,      /* Int (e.g. from/to = 128) */
    ITDB_SPLFIELD_SAMPLE_RATE = 0x06,  /* Int  (e.g. from/to = 44100) */
    ITDB_SPLFIELD_YEAR = 0x07,         /* Int  (e.g. from/to = 2004) */
    ITDB_SPLFIELD_GENRE = 0x08,        /* String */
    ITDB_SPLFIELD_KIND = 0x09,         /* String */
    ITDB_SPLFIELD_DATE_MODIFIED = 0x0a,/* Int (e.g. from/to = bcf93280 ==
				     is before 6/19/2004)*/
    ITDB_SPLFIELD_TRACKNUMBER = 0x0b,  /* Int (e.g. from = 1, to = 2) */
    ITDB_SPLFIELD_SIZE = 0x0c,         /* Int (e.g. from/to = 0x00600000
				     for 6MB) */
    ITDB_SPLFIELD_TIME = 0x0d,         /* Int (e.g. from/to = 83999 for
				     1:23/83 seconds) */
    ITDB_SPLFIELD_COMMENT = 0x0e,      /* String */
    ITDB_SPLFIELD_DATE_ADDED = 0x10,   /* Int (e.g. from/to = bcfa83ff ==
				     is after 6/19/2004) */
    ITDB_SPLFIELD_COMPOSER = 0x12,     /* String */
    ITDB_SPLFIELD_PLAYCOUNT = 0x16,    /* Int  (e.g. from/to = 1) */
    ITDB_SPLFIELD_LAST_PLAYED = 0x17,  /* Int/ (e.g. from = bcfa83ff (6/19/2004)
				     to = 0xbcfbd57f (6/20/2004)) */
    ITDB_SPLFIELD_DISC_NUMBER = 0x18,  /* Int  (e.g. from/to = 1) */
    ITDB_SPLFIELD_RATING = 0x19,       /* Int/Stars Rating (e.g. from/to =
                                     60 (3 stars)) */
    ITDB_SPLFIELD_COMPILATION = 0x1f,  /* Int (e.g. is set ->
				     ITDB_SPLACTION_IS_INT/from=1,
				     is not set ->
				     ITDB_SPLACTION_IS_NOT_INT/from=1) */
    ITDB_SPLFIELD_BPM = 0x23,          /* Int  (e.g. from/to = 60) */
    ITDB_SPLFIELD_GROUPING = 0x27,     /* String */
    ITDB_SPLFIELD_PLAYLIST = 0x28,     /* FIXME - Unknown...not parsed
				     correctly...from/to = 0xb6fbad5f
				     for "Purchased Music".  Extra
				     data after "to"... */
    ITDB_SPLFIELD_VIDEO_KIND = 0x3c,   /* Logic Int */
    ITDB_SPLFIELD_TVSHOW = 0x3e,       /* String */
    ITDB_SPLFIELD_SEASON_NR = 0x3f,    /* Int */
    ITDB_SPLFIELD_SKIPCOUNT = 0x44,    /* Int */
    ITDB_SPLFIELD_LAST_SKIPPED = 0x45, /* Int */
    ITDB_SPLFIELD_ALBUMARTIST = 0x47   /* String */
} ItdbSPLField;

struct _Itdb_SPLPref
{
    guint8  liveupdate;        /* "live Updating" check box */
    guint8  checkrules;        /* "Match X of the following
				  conditions" check box */
    guint8  checklimits;       /* "Limit To..." check box */
    guint32 limittype;         /* See types defined above */
    guint32 limitsort;         /* See types defined above */
    guint32 limitvalue;        /* The value typed next to "Limit
				  type" */
    guint8  matchcheckedonly;  /* "Match only checked songs" check box */
    /* reserved for future use */
    gint32 reserved_int1;
    gint32 reserved_int2;
    gpointer reserved1;
    gpointer reserved2;
};

struct _Itdb_SPLRule
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
    /* reserved for future use */
    gint32 reserved_int1;
    gint32 reserved_int2;
    gpointer reserved1;
    gpointer reserved2;
};


struct _Itdb_SPLRules
{
    guint32 unk004;
    guint32 match_operator;  /* "All" (logical AND): Itdb_SPLMATCH_AND,
				"Any" (logical OR): Itdb_SPLMATCH_OR */
    GList *rules;
    /* reserved for future use */
    gint32 reserved_int1;
    gint32 reserved_int2;
    gpointer reserved1;
    gpointer reserved2;
};


/* ------------------------------------------------------------ *\
 *
 * Chapters
 *
\* ------------------------------------------------------------ */

struct _Itdb_Chapter
{
    guint32 startpos;
    gchar *chaptertitle;          /* data in UTF8  */
    /* reserved for future use */
    gint32 reserved_int1;
    gint32 reserved_int2;
    gpointer reserved1;
    gpointer reserved2;
};


struct _Itdb_Chapterdata
{
    GList *chapters;
    guint32 unk024;
    guint32 unk028;
    guint32 unk032;
    /* reserved for future use */
    gint32 reserved_int1;
    gint32 reserved_int2;
    gpointer reserved1;
    gpointer reserved2;
};


/* ------------------------------------------------------------ *\
 *
 * iTunesDB, Playlists, Tracks, PhotoDB, Artwork, Thumbnails
 *
\* ------------------------------------------------------------ */

/* one star is how much (track->rating) */
#define ITDB_RATING_STEP 20

/* Types of thumbnails in Itdb_Image */
enum _ItdbThumbDataType {
    ITDB_THUMB_TYPE_INVALID,
    ITDB_THUMB_TYPE_FILE,
    ITDB_THUMB_TYPE_MEMORY,
    ITDB_THUMB_TYPE_PIXBUF,
    ITDB_THUMB_TYPE_IPOD
};
typedef enum _ItdbThumbDataType ItdbThumbDataType;



struct _Itdb_Artwork {
    Itdb_Thumb *thumbnail;
    guint32 id;            /* Artwork id used by photoalbums, starts at
			    * 0x40... libgpod will set this on sync. */
    guint64 dbid;          /* dbid of associated track. used
			      internally by libgpod */
    gint32  unk028;
    guint32 rating;        /* Rating from iPhoto * 20 (PhotoDB only) */
    gint32  unk036;
    time_t  creation_date;  /* Date the image file was created
			      (creation date of image file (PhotoDB only) */
    time_t  digitized_date;/* Date the image was taken (EXIF information,
			      PhotoDB only) */
    guint32 artwork_size;  /* Size in bytes of the original source
			      image (PhotoDB only -- don't touch in
			      case of ArtworkDB!) */
    /* reserved for future use */
    gint32 reserved_int1;
    gint32 reserved_int2;
    gpointer reserved1;
    gpointer reserved2;
    /* below is for use by application */
    guint64 usertype;
    gpointer userdata;
    /* functions called to duplicate/free userdata */
    ItdbUserDataDuplicateFunc userdata_duplicate;
    ItdbUserDataDestroyFunc userdata_destroy;
};


struct _Itdb_PhotoDB
{
    GList *photos;      /* (Itdb_Artwork *)     */
    GList *photoalbums; /* (Itdb_PhotoAlbum *)  */
    Itdb_Device *device;/* iPod device info     */
    /* reserved for future use */
    gint32 reserved_int1;
    gint32 reserved_int2;
    gpointer reserved1;
    gpointer reserved2;
    /* below is for use by application */
    guint64 usertype;
    gpointer userdata;
    /* functions called to duplicate/free userdata */
    ItdbUserDataDuplicateFunc userdata_duplicate;
    ItdbUserDataDestroyFunc userdata_destroy;
};

struct _Itdb_iTunesDB
{
    GList *tracks;
    GList *playlists;
    gchar *filename;    /* filename of iTunesDB */
    Itdb_Device *device;/* iPod device info     */
    guint32 version;
    guint64 id;
    /* reserved for future use */
    gint32 reserved_int1;
    gint32 reserved_int2;
    gpointer reserved1;
    gpointer reserved2;
    /* below is for use by application */
    guint64 usertype;
    gpointer userdata;
    /* functions called to duplicate/free userdata */
    ItdbUserDataDuplicateFunc userdata_duplicate;
    ItdbUserDataDestroyFunc userdata_destroy;
};

struct _Itdb_PhotoAlbum
{
    gchar *name;                 /* name of photoalbum in UTF8            */
    GList *members;              /* photos in album (Itdb_Artwork *)      */
    guint8 album_type;           /* 0x01 for master (Photo Library),
				    0x02 otherwise (sometimes 4 and 5)    */
    guint8 playmusic;            /* play music during slideshow (from
				    iPhoto setting)                       */
    guint8 repeat;               /* repeat the slideshow (from iPhoto
				    setting)                              */
    guint8 random;               /* show the slides in random order
			            (from iPhoto setting)                 */
    guint8 show_titles;          /* show slide captions (from iPhoto
				    setting)                              */
    guint8 transition_direction; /* 0=none, 1=left-to-right,
				    2=right-to-left, 3=top-to-bottom,
				    4=bottom-to-top (from iPhoto setting) */
    gint32 slide_duration;       /* in seconds (from iPhoto setting)      */
    gint32 transition_duration;  /* in milliseconds (from iPhoto setting) */
    gint64 song_id;              /* dbid2 of track in iTunesDB to play
				    during slideshow (from iPhoto setting)*/
    gint32 unk024;               /* unknown, seems to be always 0         */
    gint16 unk028;               /* unknown, seems to be always 0         */
    gint32 unk044;               /* unknown, seems to always be 0         */
    gint32 unk048;               /* unknown, seems to always be 0         */
    /* set automatically at time of writing the PhotoDB */
    gint32  album_id;
    gint32  prev_album_id;
    /* reserved for future use */
    gint32 reserved_int1;
    gint32 reserved_int2;
    gpointer reserved1;
    gpointer reserved2;
    /* below is for use by application */
    guint64 usertype;
    gpointer userdata;
    /* functions called to duplicate/free userdata */
    ItdbUserDataDuplicateFunc userdata_duplicate;
    ItdbUserDataDestroyFunc userdata_destroy;
};

struct _Itdb_Playlist
{
    Itdb_iTunesDB *itdb;  /* pointer to iTunesDB (for convenience) */
    gchar *name;          /* name of playlist in UTF8              */
    guint8 type;          /* ITDB_PL_TYPE_NORM/_MPL                */
    guint8 flag1;         /* unknown, usually set to 0             */
    guint8 flag2;         /* unknown, always set to 0              */
    guint8 flag3;         /* unknown, always set to 0              */
    gint  num;            /* number of tracks in playlist          */
    GList *members;       /* tracks in playlist (Track *)          */
    gboolean is_spl;      /* smart playlist?                       */
    time_t timestamp;     /* timestamp of playlist creation        */
    guint64 id;           /* playlist ID                           */
    guint32 sortorder;    /* How to sort playlist -- see below     */
    guint32 podcastflag;  /* ITDB_PL_FLAG_NORM/_PODCAST            */
    Itdb_SPLPref splpref;      /* smart playlist prefs                  */
    Itdb_SPLRules splrules;    /* rules for smart playlists             */
    gpointer reserved100; /* reserved for MHOD100 implementation   */
    gpointer reserved101; /* reserved for MHOD100 implementation   */
    /* reserved for future use */
    gint32 reserved_int1;
    gint32 reserved_int2;
    gpointer reserved1;
    gpointer reserved2;
    /* below is for use by application */
    guint64 usertype;
    gpointer userdata;
    /* functions called to duplicate/free userdata */
    ItdbUserDataDuplicateFunc userdata_duplicate;
    ItdbUserDataDestroyFunc userdata_destroy;
};


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


/* Mediatype definitions */
typedef enum
{
    ITDB_MEDIATYPE_AUDIO      = 0x0001,
    ITDB_MEDIATYPE_MOVIE      = 0x0002,
    ITDB_MEDIATYPE_PODCAST    = 0x0004,
    ITDB_MEDIATYPE_AUDIOBOOK  = 0x0008,
    ITDB_MEDIATYPE_MUSICVIDEO = 0x0020,
    ITDB_MEDIATYPE_TVSHOW     = 0x0040,
} Itdb_Mediatype;

/* some of the descriptive comments below are copied verbatim from
   http://ipodlinux.org/ITunesDB. 
   http://ipodlinux.org/ITunesDB is the best source for information
   about the iTunesDB and related files. */
struct _Itdb_Track
{
  Itdb_iTunesDB *itdb;       /* pointer to iTunesDB (for convenience)   */
  gchar   *title;            /* title (utf8)                            */
  gchar   *ipod_path;        /* name of file on iPod: uses ":" instead
				of "/" and is relative to mountpoint    */
  gchar   *album;            /* album (utf8)                            */
  gchar   *artist;           /* artist (utf8)                           */
  gchar   *genre;            /* genre (utf8)                            */
  gchar   *filetype;         /* eg. "MP3-File"...(utf8)                 */
  gchar   *comment;          /* comment (utf8)                          */
  gchar   *category;         /* Category for podcast                    */
  gchar   *composer;         /* Composer (utf8)                         */
  gchar   *grouping;         /* ? (utf8)                                */
  gchar   *description;      /* see note for MHOD_ID in itdb_itunesdb.c */
  gchar   *podcasturl;       /* see note for MHOD_ID in itdb_itunesdb.c */
  gchar   *podcastrss;       /* see note for MHOD_ID in itdb_itunesdb.c */
  Itdb_Chapterdata *chapterdata; /* see note for MHOD_ID in itdb_itunesdb.c */
  gchar   *subtitle;         /* see note for MHOD_ID in itdb_itunesdb.c */
/* the following 6 are new in libgpod 0.4.2... */
  gchar   *tvshow;           /* see note for MHOD_ID in itdb_itunesdb.c */
  gchar   *tvepisode;        /* see note for MHOD_ID in itdb_itunesdb.c */
  gchar   *tvnetwork;        /* see note for MHOD_ID in itdb_itunesdb.c */
  gchar   *albumartist;      /* see note for MHOD_ID in itdb_itunesdb.c */
  gchar   *keywords;         /* see note for MHOD_ID in itdb_itunesdb.c */
/* the following 6 are new in libgpod 0.5.0... */
  /* You can set these strings to override the standard
     sortorder. When set they take precedence over the default
     'artist', 'album'... fields.

     For example, in the case of an artist name like "The Artist",
     iTunes will set sort_artist to "Artist, The" followed by five
     0x01 characters. Why five 0x01 characters are added is not
     completely understood.

     If you do not set the sort_artist field, libgpod will pre-sort
     the lists displayed by the iPod according to "Artist, The",
     without setting the field.
  */
  gchar   *sort_artist;      /* artist (for sorting)                    */
  gchar   *sort_title;       /* title (for sorting)                     */
  gchar   *sort_album;       /* album (for sorting)                     */
  gchar   *sort_albumartist; /* album artist (for sorting)              */
  gchar   *sort_composer;    /* composer (for sorting)                  */
  gchar   *sort_tvshow;      /* tv show (for sorting)                   */
/* new fields in libgpod 0.5.0 up to here */
  guint32 id;                /* unique ID of track                      */
  gint32  size;              /* size of file in bytes                   */
  gint32  tracklen;          /* Length of track in ms                   */
  gint32  cd_nr;             /* CD number                               */
  gint32  cds;               /* number of CDs                           */
  gint32  track_nr;          /* track number                            */
  gint32  tracks;            /* number of tracks                        */
  gint32  bitrate;           /* bitrate                                 */
  guint16 samplerate;        /* samplerate (CD: 44100)                  */
  guint16 samplerate_low;    /* in the iTunesDB the samplerate is
                                multiplied by 0x10000 -- these are the
				lower 16 bit, which are usually 0       */
  gint32  year;              /* year                                    */
  gint32  volume;            /* volume adjustment                       */
  guint32 soundcheck;        /* volume adjustment "soundcheck"          */
  time_t  time_added;        /* time when added                         */
  time_t  time_modified;     /* time of last modification               */
  time_t  time_played;       /* time of last play                       */
  guint32 bookmark_time;     /* bookmark set for (AudioBook) in ms      */
  guint32 rating;            /* star rating (stars * RATING_STEP (20))  */
  guint32 playcount;         /* number of times track was played        */
  guint32 playcount2;        /* Also stores the play count of the
				song.  Don't know if it ever differs
				from the above value. During sync itdb
				sets playcount2 to the same value as
				playcount.                              */
  guint32 recent_playcount;  /* times track was played since last sync  */
  gboolean transferred;      /* has file been transferred to iPod?      */
  gint16  BPM;               /* BPM (beats per minute) of this track    */
  guint8  app_rating;        /* star rating set by appl. (not
			      * iPod). If the rating set on the iPod
			        and the rating field above differ, the
				original rating is copied here and the
				new rating is stored above. */
  guint8  type1;             /* CBR MP3s and AAC are 0x00, VBR MP3s are
			        0x01                                    */
  guint8  type2;             /* MP3s are 0x01, AAC are 0x00             */
  guint8  compilation;
  guint32 starttime;
  guint32 stoptime;
  guint8  checked;           /* 0x0: checkmark on track is set 0x1: not set */
  guint64 dbid;              /* unique database ID                      */
  guint32 drm_userid;        /* Apple Store/Audible User ID (for DRM'ed
				files only, set to 0 otherwise).        */
  guint32 visible;           /*  If this value is 1, the song is visible
				 on the iPod. All other values cause
				 the file to be hidden.                 */
  guint32 filetype_marker;   /* This appears to always be 0 on hard
                                drive based iPods, but for the
                                iTunesDB that is written to an iPod
                                Shuffle, iTunes 4.7.1 writes out the
                                file's type as an ANSI string(!). For
                                example, a MP3 file has a filetype of
                                0x4d503320 -> 0x4d = 'M', 0x50 = 'P',
                                0x33 = '3', 0x20 = <space>. (set to
				the filename extension by itdb when
				copying track to iPod)                  */
  guint16 artwork_count;     /* The number of album artwork items
				associated with this song. libgpod
				updates this value when syncing */
  guint32 artwork_size;      /* The total size of artwork (in bytes)
				attached to this song, when it is
				converted to JPEG format. Observed in
				iPodDB version 0x0b and with an iPod
				Photo. libgpod updates this value when
				syncing                                 */
  float samplerate2;         /* The sample rate of the song expressed
				as an IEEE 32 bit floating point
				number.  It's uncertain why this is
				here.  itdb will set this when adding
				a track                                 */

  guint16 unk126;     /* unknown, but always seems to be 0xffff for
			 MP3/AAC songs, 0x0 for uncompressed songs
			 (like WAVE format), 0x1 for Audible. itdb
			 will try to set this when adding a new track */
  guint32 unk132;     /* unknown */
  time_t  time_released;/* date/time added to music store? 
			   For podcasts: release date as displayed next to the 
			   title in the Podcast playlist */
  guint16 unk144;     /* unknown, but MP3 songs appear to be always
			 0x000c, AAC songs are always 0x0033, Audible
			 files are 0x0029, WAV files are 0x0. itdb
			 will attempt to set this value when adding a
			 track. */ 
  guint16 unk146;     /* unknown, but appears to be 1 if played at
			 least once in iTunes and 0 otherwise. */
  guint32 unk148;     /* unknown - used for Apple Store DRM songs
			 (always 0x01010100?), zero otherwise */
  guint32 unk152;     /* unknown */
  guint32 skipcount;  /* Number of times the track has been skipped.
			 Formerly unk156 (added in dbversion 0x0c) */
  guint32 recent_skipcount; /* number of times track was skipped since
			       last sync */
  guint32 last_skipped;/* Date/time last skipped. Formerly unk160
			 (added in dbversion 0x0c) */
  guint8 has_artwork; /* 0x01: artwork is present. 0x02: no artwork is
			 present for this track (used by the iPod to
			 decide whether to display Artwork or not) */
  guint8 skip_when_shuffling;/* "Skip when shuffling" when set to
			 0x01, set to 0x00 otherwise. .m4b and .aa
			 files always seem to be skipped when
			 shuffling, however */
  guint8 remember_playback_position;/* "Remember playback position"
			 when set to 0x01, set to 0x00 otherwise. .m4b
			 and .aa files always seem to remember the
			 playback position, however. */
  guint8 flag4;       /* Used for podcasts, 0x00 otherwise.  If set to
			 0x01 the "Now Playing" page will show
			 Title/Album, when set to 0x00 it will also
			 show the Artist. When set to 0x02 a sub-page
			 (middle button) with further information
			 about the track will be shown. */
  guint64 dbid2;      /* not clear. if not set, itdb will set this to
			 the same value as dbid when adding a
			 track. (With iTunes, since V0x12, this field
			 value differs from the dbid one.) */
  guint8 lyrics_flag; /* set to 0x01 if lyrics are present in MP3 tag
			 ("ULST"), 0x00 otherwise */
  guint8 movie_flag;  /* set to 0x01 if it's a movie file, 0x00
		         otherwise */
  guint8 mark_unplayed; /* A value of 0x02 marks a podcast as unplayed
			   on the iPod (bullet) once played it is set to
			   0x01. Non-podcasts have this set to 0x01. */
  guint8 unk179;      /* unknown (always 0x00 so far) */
  guint32 unk180;
  guint32 pregap;     /* Number of samples of silence before the songs
			 starts (for gapless playback). */
  guint64 samplecount;/* Number of samples in the song. First observed
			 in dbversion 0x0d, and only for AAC and WAV
			 files (for gapless playback). */
  guint32 unk196;
  guint32 postgap;    /* Number of samples of silence at the end of
			 the song (for gapless playback). */
  guint32 unk204;     /* unknown - added in dbversion 0x0c, first
			 values observed in 0x0d. Observed to be 0x0
			 or 0x1. */
  guint32 mediatype;  /* It seems that this field denotes the type of
		         the file on (e.g.) the 5g video iPod. It must
			 be set to 0x00000001 for audio files, and set
			 to 0x00000002 for video files. If set to
			 0x00, the files show up in both, the audio
			 menus ("Songs", "Artists", etc.) and the
			 video menus ("Movies", "Music Videos",
			 etc.). It appears to be set to 0x20 for music
			 videos, and if set to 0x60 the file shows up
			 in "TV Shows" rather than "Movies". 

			 The following list summarizes all observed types:

			 * 0x00 00 00 00 - Audio/Video
			 * 0x00 00 00 01 - Audio
			 * 0x00 00 00 02 - Video
			 * 0x00 00 00 04 - Podcast
			 * 0x00 00 00 06 - Video Podcast
			 * 0x00 00 00 08 - Audiobook
			 * 0x00 00 00 20 - Music Video
			 * 0x00 00 00 40 - TV Show (shows up ONLY in TV Shows
			 * 0x00 00 00 60 - TV Show (shows up in the
			                            Music lists as well) */
  guint32 season_nr;  /* the season number of the track, for TV shows only. */
  guint32 episode_nr; /* the episode number of the track, for TV shows
			 only - although not displayed on the iPod,
			 the episodes are sorted by episode number. */
  guint32 unk220;     /* Has something to do with protected files -
			 set to 0x0 for non-protected files. */
  guint32 unk224;
  guint32 unk228, unk232, unk236, unk240, unk244;
  guint32 gapless_data;/* some magic number needed for gapless playback
			  (added in dbversion 0x13) It has been observed
			  that gapless playback does not work if this is
			  set to zero. This number is related to the the
			  filesize in bytes, but it is a couple of bytes
			  less than the filesize. Maybe ID3 tags
			  etc... taken off? */
  guint32 unk252;
  guint16 gapless_track_flag; /* if 1, this track has gapless playback data
			         (added in dbversion 0x13) */
  guint16 gapless_album_flag; /* if 1, this track does not use crossfading
			         in iTunes (added in dbversion 0x13) */

    /* Chapter data: defines where the chapter stops are in the track,
       as well as what info should be displayed for each section of
       the track. Until it can be parsed and interpreted, the
       chapterdata will just be read as a block and written back on
       sync. This will be changed at a later time */
  void *chapterdata_raw;
  guint32 chapterdata_raw_length;

  /* This is for Cover Art support */
  struct _Itdb_Artwork *artwork;

    /* 200805 */
  guint32 mhii_link;

  /* reserved for future use */
  gint32 reserved_int1;
  gint32 reserved_int2;
  gint32 reserved_int3;
  gint32 reserved_int4;
  gint32 reserved_int5;
  gint32 reserved_int6;
  gpointer reserved1;
  gpointer reserved2;
  gpointer reserved3;
  gpointer reserved4;
  gpointer reserved5;
  gpointer reserved6;

  /* +++***+++***+++***+++***+++***+++***+++***+++***+++***+++***
     When adding string fields don't forget to add them in
     itdb_track_duplicate as well
     +++***+++***+++***+++***+++***+++***+++***+++***+++***+++*** */

  /* below is for use by application */
  guint64 usertype;
  gpointer userdata;
  /* functions called to duplicate/free userdata */
  ItdbUserDataDuplicateFunc userdata_duplicate;
  ItdbUserDataDestroyFunc userdata_destroy;
};
/* (gtkpod note: don't forget to add fields read from the file to
 * copy_new_info() in file.c!) */


/* ------------------------------------------------------------ *\
 *
 * Error codes
 *
\* ------------------------------------------------------------ */
typedef enum
{
    ITDB_FILE_ERROR_SEEK,        /* file corrupt: illegal seek occured */
    ITDB_FILE_ERROR_CORRUPT,     /* file corrupt                       */
    ITDB_FILE_ERROR_NOTFOUND,    /* file not found                     */
    ITDB_FILE_ERROR_RENAME,      /* file could not be renamed          */
    ITDB_FILE_ERROR_ITDB_CORRUPT /* iTunesDB in memory corrupt         */
} ItdbFileError;


/* Error domain */
#define ITDB_FILE_ERROR itdb_file_error_quark ()
GQuark     itdb_file_error_quark      (void);


/* ------------------------------------------------------------ *\
 *
 * Public functions
 *
\* ------------------------------------------------------------ */

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
gchar *itdb_resolve_path (const gchar *root,
			  const gchar * const * components);
gboolean itdb_rename_files (const gchar *mp, GError **error);
gchar *itdb_cp_get_dest_filename (Itdb_Track *track,
                                  const gchar *mountpoint,
				  const gchar *filename,
				  GError **error);
gboolean itdb_cp (const gchar *from_file, const gchar *to_file,
		  GError **error);
Itdb_Track *itdb_cp_finalize (Itdb_Track *track,
			      const gchar *mountpoint,
			      const gchar *dest_filename,
			      GError **error);
gboolean itdb_cp_track_to_ipod (Itdb_Track *track,
				const gchar *filename, GError **error);
void itdb_filename_fs2ipod (gchar *filename);
void itdb_filename_ipod2fs (gchar *ipod_file);
gchar *itdb_filename_on_ipod (Itdb_Track *track);
void itdb_set_mountpoint (Itdb_iTunesDB *itdb, const gchar *mp);
const gchar *itdb_get_mountpoint (Itdb_iTunesDB *itdb);
gchar *itdb_get_control_dir (const gchar *mountpoint);
gchar *itdb_get_itunes_dir (const gchar *mountpoint);
gchar *itdb_get_music_dir (const gchar *mountpoint);
gchar *itdb_get_artwork_dir (const gchar *mountpoint);
gchar *itdb_get_photos_dir (const gchar *mountpoint);
gchar *itdb_get_photos_thumb_dir (const gchar *mountpoint);
gchar *itdb_get_device_dir (const gchar *mountpoint);
gchar *itdb_get_itunesdb_path (const gchar *mountpoint);
gchar *itdb_get_itunessd_path (const gchar *mountpoint);
gchar *itdb_get_artworkdb_path (const gchar *mountpoint);
gchar *itdb_get_photodb_path (const gchar *mountpoint);
gchar *itdb_get_path (const gchar *dir, const gchar *file);

/* itdb_device functions */
Itdb_Device *itdb_device_new (void);
void itdb_device_free (Itdb_Device *device);
void itdb_device_set_mountpoint (Itdb_Device *device, const gchar *mp);
gboolean itdb_device_read_sysinfo (Itdb_Device *device);
gboolean itdb_device_write_sysinfo (Itdb_Device *device, GError **error);
gchar *itdb_device_get_sysinfo (const Itdb_Device *device, const gchar *field);
void itdb_device_set_sysinfo (Itdb_Device *device,
			      const gchar *field, const gchar *value);
const Itdb_IpodInfo *itdb_device_get_ipod_info (const Itdb_Device *device);
const Itdb_IpodInfo *itdb_info_get_ipod_info_table (void);
gboolean itdb_device_supports_artwork (const Itdb_Device *device);
gboolean itdb_device_supports_video (const Itdb_Device *device);
gboolean itdb_device_supports_photo (const Itdb_Device *device);
const gchar *itdb_info_get_ipod_model_name_string (Itdb_IpodModel model);
const gchar *itdb_info_get_ipod_generation_string (Itdb_IpodGeneration generation);

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

/* playlist functions for podcasts playlist */
Itdb_Playlist *itdb_playlist_podcasts (Itdb_iTunesDB *itdb);
gboolean itdb_playlist_is_podcasts (Itdb_Playlist *pl);
void itdb_playlist_set_podcasts (Itdb_Playlist *pl);

/* smart playlist functions */
ItdbSPLFieldType itdb_splr_get_field_type (const Itdb_SPLRule *splr);
ItdbSPLActionType itdb_splr_get_action_type (const Itdb_SPLRule *splr);
void itdb_splr_validate (Itdb_SPLRule *splr);
void itdb_splr_remove (Itdb_Playlist *pl, Itdb_SPLRule *splr);
Itdb_SPLRule *itdb_splr_new (void);
void itdb_splr_add (Itdb_Playlist *pl, Itdb_SPLRule *splr, gint pos);
Itdb_SPLRule *itdb_splr_add_new (Itdb_Playlist *pl, gint pos);
void itdb_spl_copy_rules (Itdb_Playlist *dest, Itdb_Playlist *src);
gboolean itdb_splr_eval (Itdb_SPLRule *splr, Itdb_Track *track);
void itdb_spl_update (Itdb_Playlist *spl);
void itdb_spl_update_all (Itdb_iTunesDB *itdb);
void itdb_spl_update_live (Itdb_iTunesDB *itdb);

/* thumbnails functions for coverart */
/* itdb_track_... */
gboolean itdb_track_set_thumbnails (Itdb_Track *track,
				    const gchar *filename);
gboolean itdb_track_set_thumbnails_from_data (Itdb_Track *track,
					      const guchar *image_data,
					      gsize image_data_len);
gboolean itdb_track_set_thumbnails_from_pixbuf (Itdb_Track *track,
                                                gpointer pixbuf);
gboolean itdb_track_has_thumbnails (Itdb_Track *track);
void itdb_track_remove_thumbnails (Itdb_Track *track);
gpointer itdb_track_get_thumbnail (Itdb_Track *track, gint width, gint height);

/* photoalbum functions -- see itdb_photoalbum.c for instructions on
 * how to use. */
Itdb_PhotoDB *itdb_photodb_parse (const gchar *mp, GError **error);
Itdb_Artwork *itdb_photodb_add_photo (Itdb_PhotoDB *db, const gchar *filename,
				      gint position, gint rotation,
				      GError **error);
Itdb_Artwork *itdb_photodb_add_photo_from_data (Itdb_PhotoDB *db,
						const guchar *image_data,
						gsize image_data_len,
						gint position,
						gint rotation,
						GError **error);
Itdb_Artwork *itdb_photodb_add_photo_from_pixbuf (Itdb_PhotoDB *db,
						  gpointer pixbuf,
						  gint position,
						  gint rotation,
						  GError **error);
void itdb_photodb_photoalbum_add_photo (Itdb_PhotoDB *db,
					Itdb_PhotoAlbum *album,
					Itdb_Artwork *photo,
					gint position);
Itdb_PhotoAlbum *itdb_photodb_photoalbum_create (Itdb_PhotoDB *db,
						 const gchar *albumname,
						 gint pos);
Itdb_PhotoDB *itdb_photodb_create (const gchar *mountpoint);
void itdb_photodb_free (Itdb_PhotoDB *photodb);
gboolean itdb_photodb_write (Itdb_PhotoDB *photodb, GError **error);
void itdb_photodb_remove_photo (Itdb_PhotoDB *db,
				Itdb_PhotoAlbum *album,
				Itdb_Artwork *photo);
void itdb_photodb_photoalbum_remove (Itdb_PhotoDB *db,
				     Itdb_PhotoAlbum *album,
				     gboolean remove_pics);
Itdb_PhotoAlbum *itdb_photodb_photoalbum_by_name(Itdb_PhotoDB *db,
						 const gchar *albumname );

/* itdb_artwork_... -- you probably won't need many of these (probably
 * with the exception of itdb_artwork_get_thumb_by_type() and
 * itdb_thumb_get_gdk_pixbuf() probably). Use the itdb_photodb_...()
 * functions when adding photos, and the itdb_track_...() functions
 * when adding coverart to audio. */
Itdb_Artwork *itdb_artwork_new (void);
Itdb_Artwork *itdb_artwork_duplicate (Itdb_Artwork *artwork);
void itdb_artwork_free (Itdb_Artwork *artwork);
gboolean itdb_artwork_set_thumbnail (Itdb_Artwork *artwork,
				     const gchar *filename,
				     gint rotation, GError **error);
gboolean itdb_artwork_set_thumbnail_from_data (Itdb_Artwork *artwork,
					       const guchar *image_data,
					       gsize image_data_len,
					       gint rotation, GError **error);
gboolean itdb_artwork_set_thumbnail_from_pixbuf (Itdb_Artwork *artwork,
                                                 gpointer pixbuf,
                                                 gint rotation,
                                                 GError **error);
void itdb_artwork_remove_thumbnails (Itdb_Artwork *artwork);
gpointer itdb_artwork_get_pixbuf (Itdb_Device *device, Itdb_Artwork *artwork, 
                                  gint width, gint height);

/* itdb_thumb_... */
/* the following function returns a pointer to a GdkPixbuf if
   gdk-pixbuf is installed -- a NULL pointer otherwise. */
gpointer itdb_thumb_get_gdk_pixbuf (Itdb_Device *device,
				    Itdb_Thumb *thumb);
Itdb_Thumb *itdb_thumb_duplicate (Itdb_Thumb *thumb);
void itdb_thumb_free (Itdb_Thumb *thumb);

/* itdb_chapterdata_... */
Itdb_Chapterdata *itdb_chapterdata_new (void);
void itdb_chapterdata_free (Itdb_Chapterdata *chapterdata);
Itdb_Chapterdata *itdb_chapterdata_duplicate (Itdb_Chapterdata *chapterdata);
void itdb_chapterdata_remove_chapter (Itdb_Chapterdata *chapterdata, Itdb_Chapter *chapter);
void itdb_chapterdata_remove_chapters (Itdb_Chapterdata *chapterdata);
Itdb_Chapter *itdb_chapter_new (void);
void itdb_chapter_free (Itdb_Chapter *chapter);
Itdb_Chapter *itdb_chapter_duplicate (Itdb_Chapter *chapter);
gboolean itdb_chapterdata_add_chapter (Itdb_Chapterdata *chapterdata,
				       gint32 startpos,
				       gchar *chaptertitle);

#ifndef LIBGPOD_DISABLE_DEPRECATED
/* time functions */
time_t itdb_time_get_mac_time (void);
time_t itdb_time_mac_to_host (time_t time);
time_t itdb_time_host_to_mac (time_t time);
#endif

/* Initialize a blank ipod */
gboolean itdb_init_ipod (const gchar *mountpoint,
			 const gchar *model_number,
			 const gchar *ipod_name,
			 GError **error);

G_END_DECLS

#endif

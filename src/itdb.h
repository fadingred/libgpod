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

/**
 * ItdbUserDataDestroyFunc:
 * @userdata: A #gpointer to user data
 *
 * Function called to free userdata
 */
typedef void (* ItdbUserDataDestroyFunc) (gpointer userdata);

/**
 * ItdbUserDataDuplicateFunc:
 * @userdata: A #gpointer to user data
 *
 * Function called to duplicate userdata
 *
 * Returns: A #gpointer
 */
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

/**
 * Itdb_IpodGeneration:
 * @ITDB_IPOD_GENERATION_UNKNOWN:   Unknown iPod
 * @ITDB_IPOD_GENERATION_FIRST:     First Generation iPod
 * @ITDB_IPOD_GENERATION_SECOND:    Second Generation iPod
 * @ITDB_IPOD_GENERATION_THIRD:     Third Generation iPod
 * @ITDB_IPOD_GENERATION_FOURTH:    Fourth Generation iPod
 * @ITDB_IPOD_GENERATION_PHOTO:     Photo iPod
 * @ITDB_IPOD_GENERATION_MOBILE:    Mobile iPod
 * @ITDB_IPOD_GENERATION_MINI_1:    First Generation iPod Mini
 * @ITDB_IPOD_GENERATION_MINI_2:    Second Generation iPod Mini
 * @ITDB_IPOD_GENERATION_SHUFFLE_1: First Generation iPod Shuffle
 * @ITDB_IPOD_GENERATION_SHUFFLE_2: Second Generation iPod Shuffle
 * @ITDB_IPOD_GENERATION_SHUFFLE_3: Third Generation iPod Shuffle
 * @ITDB_IPOD_GENERATION_SHUFFLE_4: Third Generation iPod Shuffle
 * @ITDB_IPOD_GENERATION_NANO_1:    First Generation iPod Nano
 * @ITDB_IPOD_GENERATION_NANO_2:    Second Generation iPod Nano
 * @ITDB_IPOD_GENERATION_NANO_3:    Third Generation iPod Nano
 * @ITDB_IPOD_GENERATION_NANO_4:    Fourth Generation iPod Nano
 * @ITDB_IPOD_GENERATION_NANO_5:    Fifth Generation iPod Nano (with camera)
 * @ITDB_IPOD_GENERATION_VIDEO_1:   First Generation iPod Video (aka 5g)
 * @ITDB_IPOD_GENERATION_VIDEO_2:   Second Generation iPod Video (aka 5.5g)
 * @ITDB_IPOD_GENERATION_CLASSIC_1: First Generation iPod Classic
 * @ITDB_IPOD_GENERATION_CLASSIC_2: Second Generation iPod Classic
 * @ITDB_IPOD_GENERATION_CLASSIC_3: Third Generation iPod Classic
 * @ITDB_IPOD_GENERATION_TOUCH_1:   First Generation iPod Touch
 * @ITDB_IPOD_GENERATION_TOUCH_2:   Second Generation iPod Touch
 * @ITDB_IPOD_GENERATION_TOUCH_3:   Third Generation iPod Touch
 * @ITDB_IPOD_GENERATION_TOUCH_4:   Fourth Generation iPod Touch
 * @ITDB_IPOD_GENERATION_IPHONE_1:  First Generation iPhone
 * @ITDB_IPOD_GENERATION_IPHONE_2:  Second Generation iPhone (aka iPhone 3G)
 * @ITDB_IPOD_GENERATION_IPHONE_3:  Third Generation iPhone (aka iPhone 3GS)
 * @ITDB_IPOD_GENERATION_IPHONE_4:  Fourth Generation iPhone
 *
 * iPod generation information
 *
 * See http://support.apple.com/kb/HT1353 and http://en.wikipedia.org/wiki/IPod
 * for more details.
 *
 * Since: 0.4.0
 */
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
    ITDB_IPOD_GENERATION_NANO_4,
    ITDB_IPOD_GENERATION_VIDEO_1,
    ITDB_IPOD_GENERATION_VIDEO_2,
    ITDB_IPOD_GENERATION_CLASSIC_1,
    ITDB_IPOD_GENERATION_CLASSIC_2,
    ITDB_IPOD_GENERATION_TOUCH_1,
    ITDB_IPOD_GENERATION_IPHONE_1,
    ITDB_IPOD_GENERATION_SHUFFLE_4,
    ITDB_IPOD_GENERATION_TOUCH_2,
    ITDB_IPOD_GENERATION_IPHONE_2,
    ITDB_IPOD_GENERATION_IPHONE_3,
    ITDB_IPOD_GENERATION_CLASSIC_3,
    ITDB_IPOD_GENERATION_NANO_5,
    ITDB_IPOD_GENERATION_TOUCH_3,
    ITDB_IPOD_GENERATION_IPAD_1,
    ITDB_IPOD_GENERATION_IPHONE_4,
    ITDB_IPOD_GENERATION_TOUCH_4,
    ITDB_IPOD_GENERATION_NANO_6
} Itdb_IpodGeneration;

/**
 * Itdb_IpodModel:
 * @ITDB_IPOD_MODEL_INVALID:        Invalid model
 * @ITDB_IPOD_MODEL_UNKNOWN:        Unknown model
 * @ITDB_IPOD_MODEL_COLOR:          Color iPod
 * @ITDB_IPOD_MODEL_COLOR_U2:       Color iPod (U2)
 * @ITDB_IPOD_MODEL_REGULAR:        Regular iPod
 * @ITDB_IPOD_MODEL_REGULAR_U2:     Regular iPod (U2)
 * @ITDB_IPOD_MODEL_MINI:           iPod Mini
 * @ITDB_IPOD_MODEL_MINI_BLUE:      iPod Mini (Blue)
 * @ITDB_IPOD_MODEL_MINI_PINK:      iPod Mini (Pink)
 * @ITDB_IPOD_MODEL_MINI_GREEN:     iPod Mini (Green)
 * @ITDB_IPOD_MODEL_MINI_GOLD:      iPod Mini (Gold)
 * @ITDB_IPOD_MODEL_SHUFFLE:        iPod Shuffle
 * @ITDB_IPOD_MODEL_NANO_WHITE:     iPod Nano (White)
 * @ITDB_IPOD_MODEL_NANO_BLACK:     iPod Nano (Black)
 * @ITDB_IPOD_MODEL_VIDEO_WHITE:    iPod Video (White)
 * @ITDB_IPOD_MODEL_VIDEO_BLACK:    iPod Video (Black)
 * @ITDB_IPOD_MODEL_MOBILE_1:       Mobile iPod
 * @ITDB_IPOD_MODEL_VIDEO_U2:       iPod Video (U2)
 * @ITDB_IPOD_MODEL_NANO_SILVER:    iPod Nano (Silver)
 * @ITDB_IPOD_MODEL_NANO_BLUE:      iPod Nano (Blue)
 * @ITDB_IPOD_MODEL_NANO_GREEN:     iPod Nano (Green)
 * @ITDB_IPOD_MODEL_NANO_PINK:      iPod Nano (Pink)
 * @ITDB_IPOD_MODEL_NANO_RED:       iPod Nano (Red)
 * @ITDB_IPOD_MODEL_NANO_YELLOW:    iPod Nano (Yellow)
 * @ITDB_IPOD_MODEL_NANO_PURPLE:    iPod Nano (Purple)
 * @ITDB_IPOD_MODEL_NANO_ORANGE:    iPod Nano (Orange)
 * @ITDB_IPOD_MODEL_IPHONE_1:       iPhone
 * @ITDB_IPOD_MODEL_SHUFFLE_SILVER: iPod Shuffle (Silver)
 * @ITDB_IPOD_MODEL_SHUFFLE_BLACK:  iPod Shuffle (Black)
 * @ITDB_IPOD_MODEL_SHUFFLE_PINK:   iPod Shuffle (Pink)
 * @ITDB_IPOD_MODEL_SHUFFLE_BLUE:   iPod Shuffle (Blue)
 * @ITDB_IPOD_MODEL_SHUFFLE_GREEN:  iPod Shuffle (Green)
 * @ITDB_IPOD_MODEL_SHUFFLE_ORANGE: iPod Shuffle (Orange)
 * @ITDB_IPOD_MODEL_SHUFFLE_PURPLE: iPod Shuffle (Purple)
 * @ITDB_IPOD_MODEL_SHUFFLE_RED:    iPod Shuffle (Red)
 * @ITDB_IPOD_MODEL_CLASSIC_SILVER: iPod Classic (Silver)
 * @ITDB_IPOD_MODEL_CLASSIC_BLACK:  iPod Classic (Black)
 * @ITDB_IPOD_MODEL_TOUCH_SILVER:   iPod Touch (Silver)
 * @ITDB_IPOD_MODEL_IPHONE_WHITE:   iPhone (White)
 * @ITDB_IPOD_MODEL_IPHONE_BLACK:   iPhone (Black)
 *
 * iPod model information
 *
 * Since: 0.4.0
 */
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
    ITDB_IPOD_MODEL_NANO_YELLOW,
    ITDB_IPOD_MODEL_NANO_PURPLE,
    ITDB_IPOD_MODEL_NANO_ORANGE,
    ITDB_IPOD_MODEL_IPHONE_1,
    ITDB_IPOD_MODEL_SHUFFLE_SILVER,
    ITDB_IPOD_MODEL_SHUFFLE_PINK,
    ITDB_IPOD_MODEL_SHUFFLE_BLUE,
    ITDB_IPOD_MODEL_SHUFFLE_GREEN,
    ITDB_IPOD_MODEL_SHUFFLE_ORANGE,
    ITDB_IPOD_MODEL_SHUFFLE_PURPLE,
    ITDB_IPOD_MODEL_SHUFFLE_RED,
    ITDB_IPOD_MODEL_CLASSIC_SILVER,
    ITDB_IPOD_MODEL_CLASSIC_BLACK,
    ITDB_IPOD_MODEL_TOUCH_SILVER,
    ITDB_IPOD_MODEL_SHUFFLE_BLACK,
    ITDB_IPOD_MODEL_IPHONE_WHITE,
    ITDB_IPOD_MODEL_IPHONE_BLACK,
    ITDB_IPOD_MODEL_SHUFFLE_GOLD,
    ITDB_IPOD_MODEL_SHUFFLE_STAINLESS,
    ITDB_IPOD_MODEL_IPAD
} Itdb_IpodModel;

/**
 * Itdb_IpodInfo:
 * @model_number:    The model number.  This is abbreviated.  If the first
 *                   character is not numeric, it is ommited. e.g.
 *                   "MA350 -> A350", "M9829 -> 9829"
 * @capacity:        The iPod's capacity in gigabytes
 * @ipod_model:      The iPod model
 * @ipod_generation: The iPod generation
 * @musicdirs:       The number of music (Fnn) dirs created by iTunes. The
 *                   exact number seems to be version dependent. Therefore, the
 *                   numbers here represent a mixture of reported values and
 *                   common sense.  Note: this number does not necessarily
 *                   represent the number of dirs present on a particular iPod.
 *                   It is used when setting up a new iPod from scratch.
 * @reserved_int1:   Reserved for future use
 * @reserved_int2:   Reserved for future use
 * @reserved1:       Reserved for future use
 * @reserved2:       Reserved for future use
 *
 * Structure representing information about an iPod
 *
 * Since: 0.4.0
 */
struct _Itdb_IpodInfo {
    const gchar *model_number;
    const double capacity;
    const Itdb_IpodModel ipod_model;
    const Itdb_IpodGeneration ipod_generation;
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

/**
 * ITDB_SPL_STRING_MAXLEN:
 *
 * Maximum string length for smart playlists
 *
 * Since: 0.5.0
 */
#define ITDB_SPL_STRING_MAXLEN 255

/**
 * ITDB_SPL_DATE_IDENTIFIER:
 *
 * Identifier for smart playlist date fields
 *
 * Since: 0.5.0
 */
#define ITDB_SPL_DATE_IDENTIFIER (G_GINT64_CONSTANT (0x2dae2dae2dae2daeU))

/* Definitions for smart playlists */

/**
 * ItdbSPLMatch:
 * @ITDB_SPLMATCH_AND: Logical AND - all of the rules must be true in order for
 *                     the combined rule to be applied
 * @ITDB_SPLMATCH_OR:  Logical OR - any of the rules may be true
 *
 * Types for smart playlist rules match_operator
 */
typedef enum {
    ITDB_SPLMATCH_AND = 0,
    ITDB_SPLMATCH_OR = 1
} ItdbSPLMatch;

/**
 * ItdbLimitType:
 * @ITDB_LIMITTYPE_MINUTES: Limit in minutes
 * @ITDB_LIMITTYPE_MB:      Limit in megabytes
 * @ITDB_LIMITTYPE_SONGS:   Limit in number of songs
 * @ITDB_LIMITTYPE_HOURS:   Limit in hours
 * @ITDB_LIMITTYPE_GB:      Limit in gigabytes
 *
 * The type of unit to use when limiting a playlist
 *
 * Since: 0.5.0
 */
typedef enum {
    ITDB_LIMITTYPE_MINUTES = 0x01,
    ITDB_LIMITTYPE_MB      = 0x02,
    ITDB_LIMITTYPE_SONGS   = 0x03,
    ITDB_LIMITTYPE_HOURS   = 0x04,
    ITDB_LIMITTYPE_GB      = 0x05
} ItdbLimitType;

/**
 * ItdbLimitSort:
 * @ITDB_LIMITSORT_RANDOM:                Sort randomly
 * @ITDB_LIMITSORT_SONG_NAME:             Sort by track name
 * @ITDB_LIMITSORT_ALBUM:                 Sort by album name
 * @ITDB_LIMITSORT_ARTIST:                Sort by artist name
 * @ITDB_LIMITSORT_GENRE:                 Sort by genre
 * @ITDB_LIMITSORT_MOST_RECENTLY_ADDED:   Sort by most recently added
 * @ITDB_LIMITSORT_LEAST_RECENTLY_ADDED:  Sort by least recently added
 * @ITDB_LIMITSORT_MOST_OFTEN_PLAYED:     Sort by most often played
 * @ITDB_LIMITSORT_LEAST_OFTEN_PLAYED:    Sort by least often played
 * @ITDB_LIMITSORT_MOST_RECENTLY_PLAYED:  Sort by most recently played
 * @ITDB_LIMITSORT_LEAST_RECENTLY_PLAYED: Sort by least recently played
 * @ITDB_LIMITSORT_HIGHEST_RATING:        Sort by highest rating
 * @ITDB_LIMITSORT_LOWEST_RATING:         Sort by lowest rating
 *
 * Which songs to pick when using a limit type
 *
 * Note: the values for #ITDB_LIMITSORT_LEAST_RECENTLY_ADDED,
 * #ITDB_LIMITSORT_LEAST_OFTEN_PLAYED, #ITDB_LIMITSORT_LEAST_RECENTLY_PLAYED,
 * and #ITDB_LIMITSORT_LOWEST_RATING are really 0x10, 0x14, 0x15, 0x17, with the
 * 'limitsort_opposite' flag set.  This is the same value as the "positive"
 * value (i.e. #ITDB_LIMITSORT_LEAST_RECENTLY_ADDED), and is really very
 * terribly awfully weird, so we map the values to iPodDB specific values with
 * the high bit set.
 *
 * On writing, we check the high bit and write the limitsort_opposite from that.
 * That way, we don't have to deal with programs using the class needing to set
 * the wrong limit and then make it into the "opposite", which would be frickin'
 * annoying.
 *
 * Since: 0.5.0
 */
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

/**
 * ItdbSPLAction:
 * @ITDB_SPLACTION_IS_INT:              is integer ("Is Set" in iTunes)
 * @ITDB_SPLACTION_IS_GREATER_THAN:     is greater than ("Is after" in iTunes)
 * @ITDB_SPLACTION_IS_LESS_THAN:        is less than ("Is Before" in iTunes)
 * @ITDB_SPLACTION_IS_IN_THE_RANGE:     is in the range
 * @ITDB_SPLACTION_IS_IN_THE_LAST:      is in the last
 * @ITDB_SPLACTION_BINARY_AND:          binary AND
 * @ITDB_SPLACTION_IS_STRING:           is a string
 * @ITDB_SPLACTION_CONTAINS:            contains
 * @ITDB_SPLACTION_STARTS_WITH:         starts with
 * @ITDB_SPLACTION_ENDS_WITH:           ends with
 * @ITDB_SPLACTION_IS_NOT_INT:          is not an integer ("Is Not Set" in iTunes)
 * @ITDB_SPLACTION_IS_NOT_GREATER_THAN: is not greater than (not in iTunes)
 * @ITDB_SPLACTION_IS_NOT_LESS_THAN:    is not less than (not in iTunes)
 * @ITDB_SPLACTION_IS_NOT_IN_THE_RANGE: is not in the range (not in iTunes)
 * @ITDB_SPLACTION_IS_NOT_IN_THE_LAST:  is not in the last
 * @ITDB_SPLACTION_IS_NOT:              is not
 * @ITDB_SPLACTION_DOES_NOT_CONTAIN:    does not contain
 * @ITDB_SPLACTION_DOES_NOT_START_WITH: does not start with (not in iTunes)
 * @ITDB_SPLACTION_DOES_NOT_END_WITH:   does not end with (not in iTunes)
 *
 * Smartlist Actions used in smart playlist rules.
 *
 * Note by Otto (Samuel Wood):
 * <informalexample>
 *  <programlisting>
 *  really this is a bitmapped field...
 *  high byte
 *  bit 0 = "string" values if set, "int" values if not set
 *  bit 1 = "not", or to negate the check.
 *  lower 2 bytes
 *  bit 0 = simple "IS" query
 *  bit 1 = contains
 *  bit 2 = begins with
 *  bit 3 = ends with
 *  bit 4 = greater than
 *  bit 5 = unknown, but probably greater than or equal to
 *  bit 6 = less than
 *  bit 7 = unknown, but probably less than or equal to
 *  bit 8 = a range selection
 *  bit 9 = "in the last"
 *  bit 10 = binary AND
 *  bit 11 = unknown
 *  </programlisting>
 * </informalexample>
 *
 * Since: 0.5.0
*/
typedef enum {
    ITDB_SPLACTION_IS_INT = 0x00000001,
    ITDB_SPLACTION_IS_GREATER_THAN = 0x00000010,
    ITDB_SPLACTION_IS_LESS_THAN = 0x00000040,
    ITDB_SPLACTION_IS_IN_THE_RANGE = 0x00000100,
    ITDB_SPLACTION_IS_IN_THE_LAST = 0x00000200,
    ITDB_SPLACTION_BINARY_AND = 0x00000400,
    /* This action has been seen in the smart playlists stored in mhsd5
     * on recent ipods. It operates on the "video kind" field. 
     * It uses integer values, and the from/to values are different.
     * The "from" value might be the list of bits that are allowed to be set
     * in the value we are matching, and the "to" value might be a value
     * that must be set in the value we are matching. For example, there's 
     * a TVShow playlist using that action, from is 0x208044, to is 0x40. 
     * "video kind" is a bitfield so the 1st value let us filter out
     * entries with unwanted bits, and the second one ensures we have a
     * TVShow. A match could be tested with:
     * is_match = (((val & from) == val) && (val & to))
     * I'm not sure what this becomes when this action is negated...
     */
    ITDB_SPLACTION_BINARY_UNKNOWN1 = 0x00000800,

    ITDB_SPLACTION_IS_STRING = 0x01000001,
    ITDB_SPLACTION_CONTAINS = 0x01000002,
    ITDB_SPLACTION_STARTS_WITH = 0x01000004,
    ITDB_SPLACTION_ENDS_WITH = 0x01000008,

    ITDB_SPLACTION_IS_NOT_INT = 0x02000001,
    ITDB_SPLACTION_IS_NOT_GREATER_THAN = 0x02000010,
    ITDB_SPLACTION_IS_NOT_LESS_THAN = 0x02000040,
    ITDB_SPLACTION_IS_NOT_IN_THE_RANGE = 0x02000100,
    ITDB_SPLACTION_IS_NOT_IN_THE_LAST = 0x02000200,
    ITDB_SPLACTION_NOT_BINARY_AND = 0x02000400,
    ITDB_SPLACTION_BINARY_UNKNOWN2 = 0x02000800,


    ITDB_SPLACTION_IS_NOT = 0x03000001,
    ITDB_SPLACTION_DOES_NOT_CONTAIN = 0x03000002,
    ITDB_SPLACTION_DOES_NOT_START_WITH = 0x03000004,
    ITDB_SPLACTION_DOES_NOT_END_WITH = 0x03000008,
} ItdbSPLAction;

/**
 * ItdbSPLFieldType:
 * @ITDB_SPLFT_STRING:      string
 * @ITDB_SPLFT_INT:         integer
 * @ITDB_SPLFT_BOOLEAN:     boolean
 * @ITDB_SPLFT_DATE:        date
 * @ITDB_SPLFT_PLAYLIST:    playlist
 * @ITDB_SPLFT_UNKNOWN:     unknown
 * @ITDB_SPLFT_BINARY_AND:  binary AND
 *
 * Smart Playlist Field Types
 *
 * Since: 0.5.0
 */
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

/**
 * ItdbSPLActionType:
 * @ITDB_SPLAT_STRING:      string
 * @ITDB_SPLAT_INT:         from integer
 * @ITDB_SPLAT_DATE:        from date ...
 * @ITDB_SPLAT_RANGE_INT:   an integer range ...
 * @ITDB_SPLAT_RANGE_DATE:  a date range ...
 * @ITDB_SPLAT_INTHELAST:   in the last ...
 * @ITDB_SPLAT_PLAYLIST:    in playlist
 * @ITDB_SPLAT_NONE:        none
 * @ITDB_SPLAT_INVALID:     invalid
 * @ITDB_SPLAT_UNKNOWN:     unknown
 * @ITDB_SPLAT_BINARY_AND:  is / is not (binary AND)
 *
 * Smart Playlist Action Types
 *
 * Since: 0.5.0
 */
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


/**
 * ItdbSPLActionLast:
 * @ITDB_SPLACTION_LAST_DAYS_VALUE:   Seconds in 24 hours
 * @ITDB_SPLACTION_LAST_WEEKS_VALUE:  Seconds in 7 days
 * @ITDB_SPLACTION_LAST_MONTHS_VALUE: Seconds in 1 month (approximately)
 *
 * These are to pass to AddRule() when you need a unit for the two "in the last"
 * action types.  In theory, you can use any time range.  iTunes might not
 * like it, but the iPod shouldn't care.
 *
 * Since: 0.5.0
 */
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

/**
 * ItdbSPLField:
 * @ITDB_SPLFIELD_SONG_NAME:        Song name (string)
 * @ITDB_SPLFIELD_ALBUM:            Album (string)
 * @ITDB_SPLFIELD_ARTIST:           Artist (string)
 * @ITDB_SPLFIELD_BITRATE:          Bitrate (integer, e.g. from/to = 128)
 * @ITDB_SPLFIELD_SAMPLE_RATE:      Sample rate (integer, e.g. from/to = 44100)
 * @ITDB_SPLFIELD_YEAR:             Year (integer, e.g. from/to = 2004)
 * @ITDB_SPLFIELD_GENRE:            Genre (string)
 * @ITDB_SPLFIELD_KIND:             File type (string, e.g. MP3-File)
 * @ITDB_SPLFIELD_DATE_MODIFIED:    Date modified (integer, e.g.
 *                                  from/to = bcf93280 == is before 6/19/2004)
 * @ITDB_SPLFIELD_TRACKNUMBER:      Track number (integer, e.g. from/to = 2)
 * @ITDB_SPLFIELD_SIZE:             Size (integer, e.g.
 *                                  from/to = 0x00600000 for 6MB)
 * @ITDB_SPLFIELD_TIME:             Time (integer, e.g.
 *                                  from/to = 83999 for 1:23/83 seconds)
 * @ITDB_SPLFIELD_COMMENT:          Comment (string)
 * @ITDB_SPLFIELD_DATE_ADDED:       Date added (integer, e.g.
 *                                  from/to = bcfa83ff == is after 6/19/2004)
 * @ITDB_SPLFIELD_COMPOSER:         Composer (string)
 * @ITDB_SPLFIELD_PLAYCOUNT:        Playcount (integer, e.g. from/to = 1)
 * @ITDB_SPLFIELD_LAST_PLAYED:      Date last played (integer, e.g.
 *                                  from = bcfa83ff (6/19/2004)
 *                                  to = 0xbcfbd57f (6/20/2004))
 * @ITDB_SPLFIELD_DISC_NUMBER:      Disc number (integer, e.g. from/to = 1)
 * @ITDB_SPLFIELD_RATING:           Rating (integer, e.g.
 *                                  from/to = 60 (3 stars))
 * @ITDB_SPLFIELD_COMPILATION:      Compilation (integer, e.g.
 *                                  is set -> ITDB_SPLACTION_IS_INT/from=1,
 *                                  not set -> ITDB_SPLACTION_IS_NOT_INT/from=1)
 * @ITDB_SPLFIELD_BPM:              Beats per minute (integer, e.g.
 *                                  from/to = 60)
 * @ITDB_SPLFIELD_GROUPING:         Grouping (string)
 * @ITDB_SPLFIELD_PLAYLIST:         FIXME Unknown...not parsed correctly...
 *                                  from/to = 0xb6fbad5f for "Purchased Music".
 *                                  Extra data after "to"...
 * @ITDB_SPLFIELD_VIDEO_KIND:       Logical integer (works on mediatype)
 * @ITDB_SPLFIELD_TVSHOW:           TV Show (string)
 * @ITDB_SPLFIELD_SEASON_NR:        Season number (integer)
 * @ITDB_SPLFIELD_SKIPCOUNT:        Skipcount (integer)
 * @ITDB_SPLFIELD_LAST_SKIPPED:     Last skipped (integer)
 * @ITDB_SPLFIELD_ALBUMARTIST:      Album artist (string)
 *
 * Smart Playlist Fields, used for Smart Playlist Rules (#Itdb_SPLRule).
 *
 * Since: 0.5.0
 */
typedef enum {
    ITDB_SPLFIELD_SONG_NAME = 0x02,
    ITDB_SPLFIELD_ALBUM = 0x03,
    ITDB_SPLFIELD_ARTIST = 0x04,
    ITDB_SPLFIELD_BITRATE = 0x05,
    ITDB_SPLFIELD_SAMPLE_RATE = 0x06,
    ITDB_SPLFIELD_YEAR = 0x07,
    ITDB_SPLFIELD_GENRE = 0x08,
    ITDB_SPLFIELD_KIND = 0x09,
    ITDB_SPLFIELD_DATE_MODIFIED = 0x0a,
    ITDB_SPLFIELD_TRACKNUMBER = 0x0b,
    ITDB_SPLFIELD_SIZE = 0x0c,
    ITDB_SPLFIELD_TIME = 0x0d,
    ITDB_SPLFIELD_COMMENT = 0x0e,
    ITDB_SPLFIELD_DATE_ADDED = 0x10,
    ITDB_SPLFIELD_COMPOSER = 0x12,
    ITDB_SPLFIELD_PLAYCOUNT = 0x16,
    ITDB_SPLFIELD_LAST_PLAYED = 0x17,
    ITDB_SPLFIELD_DISC_NUMBER = 0x18,
    ITDB_SPLFIELD_RATING = 0x19,
    ITDB_SPLFIELD_COMPILATION = 0x1f,
    ITDB_SPLFIELD_BPM = 0x23,
    ITDB_SPLFIELD_GROUPING = 0x27,
    ITDB_SPLFIELD_PLAYLIST = 0x28,
    ITDB_SPLFIELD_PURCHASE = 0x29,
    ITDB_SPLFIELD_DESCRIPTION = 0x36,
    ITDB_SPLFIELD_CATEGORY = 0x37,
    ITDB_SPLFIELD_PODCAST = 0x39,
    ITDB_SPLFIELD_VIDEO_KIND = 0x3c,
    ITDB_SPLFIELD_TVSHOW = 0x3e,
    ITDB_SPLFIELD_SEASON_NR = 0x3f,
    ITDB_SPLFIELD_SKIPCOUNT = 0x44,
    ITDB_SPLFIELD_LAST_SKIPPED = 0x45,
    ITDB_SPLFIELD_ALBUMARTIST = 0x47,
    ITDB_SPLFIELD_SORT_SONG_NAME = 0x4e,
    ITDB_SPLFIELD_SORT_ALBUM = 0x4f,
    ITDB_SPLFIELD_SORT_ARTIST = 0x50,
    ITDB_SPLFIELD_SORT_ALBUMARTIST = 0x51,
    ITDB_SPLFIELD_SORT_COMPOSER = 0x52,
    ITDB_SPLFIELD_SORT_TVSHOW = 0x53,
    ITDB_SPLFIELD_ALBUM_RATING = 0x5a
} ItdbSPLField;

/**
 * Itdb_SPLPref:
 * @liveupdate:         Live Updating
 * @checkrules:         Match this number of rules.  If set to 0, ignore rules.
 * @checklimits:        Limit to this number of @limittype.  If 0, no limits.
 * @limittype:          an #ItdbLimitType
 * @limitsort:          an #ItdbLimitSort
 * @limitvalue:         The value typed next to "Limit type"
 * @matchcheckedonly:   Match only checked songs
 * @reserved_int1:      Reserved for future use
 * @reserved_int2:      Reserved for future use
 * @reserved1:          Reserved for future use
 * @reserved2:          Reserved for future use
 *
 * Smart Playlist preferences are for various flags that are not strictly smart
 * playlist "rules."
 *
 * Since: 0.5.0
 */
struct _Itdb_SPLPref
{
    guint8  liveupdate;
    guint8  checkrules;
    guint8  checklimits;
    guint32 limittype;
    guint32 limitsort;
    guint32 limitvalue;
    guint8  matchcheckedonly;
    gint32 reserved_int1;
    gint32 reserved_int2;
    gpointer reserved1;
    gpointer reserved2;
};

/**
 * Itdb_SPLRule:
 * @field:          an #ItdbSPLFieldType
 * @action:         an #ItdbSPLActionType
 * @string:         data in UTF8
 * @fromvalue:      from value
 * @fromdate:       from date
 * @fromunits:      from units
 * @tovalue:        to value
 * @todate:         to date
 * @tounits:        to units
 * @unk052:         Unknown
 * @unk056:         Unknown
 * @unk060:         Unknown
 * @unk064:         Unknown
 * @unk068:         Unknown
 * @reserved_int1:  Reserved for future use
 * @reserved_int2:  Reserved for future use
 * @reserved1:      Reserved for future use
 * @reserved2:      Reserved for future use
 *
 * Smart Playlist Rule
 *
 * The from and to fields require some explanation.  If @field is a date type,
 * then @value would be set to 0x2dae2dae2dae2dae, @date would be a number,
 * (e.g. 2 or -2), and @units would be a time unit in seconds (e.g. one week
 * would be 604800).  If @field is an integer comparison, like rating = 60 (i.e.
 * 3 stars), then @value would be the value we care about (e.g. 60), @date would
 * be 0, and @units would be 1.  Binary AND types are similar, @value is the
 * important part, with @date = 0 and @units = 1.  Clear as mud, right?
 *
 * For more details see <ulink
 * url="http://ipodlinux.org/ITunesDB.html&num;Smart_Playlist_Rule_Values">ipodlinux.org</ulink>.
 *
 * Since: 0.5.0
 */
struct _Itdb_SPLRule
{
    guint32 field;
    guint32 action;
    gchar *string;
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

/**
 * Itdb_SPLRules:
 * @unk004:         Unknown
 * @match_operator: Whether all rules must match (#ITDB_SPLMATCH_AND) or any
 *                  rules may match (#ITDB_SPLMATCH_OR)
 * @rules:          list of #Itdb_SPLRule's
 * @reserved_int1:  Reserved for future use
 * @reserved_int2:  Reserved for future use
 * @reserved1:      Reserved for future use
 * @reserved2:      Reserved for future use
 *
 * Smart Playlist Rules
 *
 * Since: 0.5.0
 */
struct _Itdb_SPLRules
{
    guint32 unk004;
    guint32 match_operator;
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

/**
 * Itdb_Chapter:
 * @startpos:      The start position of the chapter in ms. The first chapter
 *                 begins at 1.
 * @chaptertitle:  The chapter title in UTF8
 * @reserved_int1: Reserved for future use
 * @reserved_int2: Reserved for future use
 * @reserved1:     Reserved for future use
 * @reserved2:     Reserved for future use
 *
 * Structure representing an iTunesDB Chapter
 *
 * Since: 0.7.0
 */
struct _Itdb_Chapter
{
    guint32 startpos;
    gchar *chaptertitle;
    /* reserved for future use */
    gint32 reserved_int1;
    gint32 reserved_int2;
    gpointer reserved1;
    gpointer reserved2;
};

/**
 * Itdb_Chapterdata:
 * @chapters:      A list of chapters (#Itdb_Chapter)
 * @unk024:        Unknown
 * @unk028:        Unknown
 * @unk032:        Unknown
 * @reserved_int1: Reserved for future use
 * @reserved_int2: Reserved for future use
 * @reserved1:     Reserved for future use
 * @reserved2:     Reserved for future use
 *
 * Structure representing iTunesDB Chapter data
 *
 * Since: 0.7.0
 */
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

/**
 * ITDB_RATING_STEP:
 *
 * The multiplier for each star in #track->rating
 */
#define ITDB_RATING_STEP 20

/**
 * Itdb_Artwork:
 * @thumbnail:          An #Itdb_Thumb
 * @id:                 Artwork id used by photoalbums.  This starts at 0x40 and
 *                      is set automatically when the ArtworkDB or PhotoDB is
 *                      written
 * @dbid:               The dbid of associated track.  Used internally by
 *                      libgpod.
 * @unk028:             Unknown
 * @rating:             Rating from iPhoto * 20 (PhotoDB only)
 * @unk036:             Unknown
 * @creation_date:      Date the image file was created (PhotoDB only)
 * @digitized_date:     Date the image was taken (EXIF information, PhotoDB
 *                      only)
 * @artwork_size:       Size in bytes of the original source image (PhotoDB
 *                      only -- don't touch in case of ArtworkDB!)
 * @reserved_int1:      Reserved for future use
 * @reserved_int2:      Reserved for future use
 * @reserved1:          Reserved for future use
 * @reserved2:          Reserved for future use
 * @usertype:           For use by application
 * @userdata:           For use by application
 * @userdata_duplicate: A function to duplicate #userdata
 * @userdata_destroy:   A function to free #userdata
 *
 * Structure representing artwork in an #Itdb_iTunesDB or #Itdb_PhotoDB
 *
 * Since: 0.3.0
 */
struct _Itdb_Artwork {
    Itdb_Thumb *thumbnail;
    guint32 id;
    guint64 dbid;
    gint32  unk028;
    guint32 rating;
    gint32  unk036;
    time_t  creation_date;
    time_t  digitized_date;
    guint32 artwork_size;
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

/**
 * Itdb_PhotoDB:
 * @photos:             A list of photos in the database (#Itdb_Artwork)
 * @photoalbums:        A list of albums in the database (#Itdb_PhotoAlbum)
 * @device:             iPod device info (#Itdb_Device)
 * @reserved_int1:      Reserved for future use
 * @reserved_int2:      Reserved for future use
 * @reserved1:          Reserved for future use
 * @reserved2:          Reserved for future use
 * @usertype:           For use by application
 * @userdata:           For use by application
 * @userdata_duplicate: A function to duplicate #userdata
 * @userdata_destroy:   A function to free #userdata
 *
 * Structure representing an iTunes Photo database
 *
 * Since: 0.4.0
 */
struct _Itdb_PhotoDB
{
    GList *photos;
    GList *photoalbums;
    Itdb_Device *device;
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

typedef struct _Itdb_iTunesDB_Private Itdb_iTunesDB_Private;
/**
 * Itdb_iTunesDB:
 * @tracks:             A list of tracks in the database (#Itdb_Track)
 * @playlists:          A list of playlists in the database (#Itdb_Playlist)
 * @filename:           The filename of the iTunesDB
 * @device:             iPod device info (#Itdb_Device)
 * @version:            The version number of the iTunesDB
 * @id:                 A 64 bit id value for the iTunesDB
 * @tzoffset:           offset in seconds from UTC
 * @reserved_int2:      Reserved for future use
 * @priv:               Private data
 * @reserved2:          Reserved for future use
 * @usertype:           For use by application
 * @userdata:           For use by application
 * @userdata_duplicate: A function to duplicate #userdata
 * @userdata_destroy:   A function to free #userdata
 *
 * Structure representing an iTunes database
 */
struct _Itdb_iTunesDB
{
    GList *tracks;
    GList *playlists;
    gchar *filename;
    Itdb_Device *device;
    guint32 version;
    guint64 id;
    gint32 tzoffset;
    /* reserved for future use */
    gint32 reserved_int2;
    Itdb_iTunesDB_Private *priv;
    gpointer reserved2;
    /* below is for use by application */
    guint64 usertype;
    gpointer userdata;
    /* functions called to duplicate/free userdata */
    ItdbUserDataDuplicateFunc userdata_duplicate;
    ItdbUserDataDestroyFunc userdata_destroy;
};

/**
 * Itdb_PhotoAlbum:
 * @photodb:              A pointer to the #Itdb_PhotoDB (for convenience)
 * @name:                 The name of photoalbum in UTF8
 * @members:              A list of photos in album (#Itdb_Artwork)
 * @album_type:           The album type.  0x01 for master (Photo Library),
 *                        0x02 for a normal album.  (4 and 5 have also been
 *                        seen.)
 * @playmusic:            Play music during slideshow
 * @repeat:               Repeat the slideshow
 * @random:               Show slides in random order
 * @show_titles:          Show slide captions
 * @transition_direction: Transition direction.  0=none, 1=left-to-right,
 *                        2=right-to-left, 3=top-to-bottom, 4=bottom-to-top
 * @slide_duration:       Slide duration in seconds
 * @transition_duration:  Transition duration, in milliseconds
 * @song_id:              The @dbid2 of a track to play during slideshow
 * @unk024:               Unknown, seems to be always 0
 * @unk028:               Unknown, seems to be always 0
 * @unk044:               Unknown, seems to always be 0
 * @unk048:               Unknown, seems to always be 0
 * @album_id:             Unique integer for each playlist. This is set
 *                        automatically when the PhotoDB is written.
 * @prev_album_id:        The id of the previous playlist.  This is set
 *                        automatically when the PhotoDB is written.
 * @reserved_int1:        Reserved for future use
 * @reserved_int2:        Reserved for future use
 * @reserved1:            Reserved for future use
 * @reserved2:            Reserved for future use
 * @usertype:             For use by application
 * @userdata:             For use by application
 * @userdata_duplicate:   A function to duplicate #userdata
 * @userdata_destroy:     A function to free #userdata
 *
 * Structure representing an iTunes Photo Album
 *
 * Since: 0.4.0
 */
struct _Itdb_PhotoAlbum
{
    Itdb_PhotoDB *photodb;
    gchar *name;
    GList *members;
    guint8 album_type;
    guint8 playmusic;
    guint8 repeat;
    guint8 random;
    guint8 show_titles;
    guint8 transition_direction;
    gint32 slide_duration;
    gint32 transition_duration;
    gint64 song_id;
    gint32 unk024;
    gint16 unk028;
    gint32 unk044;
    gint32 unk048;
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

typedef struct _Itdb_Playlist_Private Itdb_Playlist_Private;
/**
 * Itdb_Playlist:
 * @itdb:               A pointer to the #Itdb_iTunesDB (for convenience)
 * @name:               The name of the playlist in UTF8
 * @type:               The playlist type (normal or master)
 * @flag1:              Unknown, usually set to 0
 * @flag2:              Unknown, always set to 0
 * @flag3:              Unknown, always set to 0
 * @num:                The number of tracks in the playlist
 * @members:            A list of tracks in the playlist (#Itdb_Track)
 * @is_spl:             True if the playlist is a smart playlist, otherwise
 *                      false
 * @timestamp:          When the playlist was created
 * @id:                 The playlist ID
 * @sortorder:          The playlist sort order (#ItdbPlaylistSortOrder)
 * @podcastflag:        This is set to 0 on normal playlists and to 1 for the
 *                      Podcast playlist. If set to 1, the playlist will not be
 *                      shown under 'Playlists' on the iPod, but as 'Podcasts'
 *                      under the Music menu. The actual title of the Playlist
 *                      does not matter. If more than one playlist is set to 1,
 *                      they don't show up at all.
 * @splpref:            Smart playlist preferences (#Itdb_SPLPref)
 * @splrules:           Smart playlist rules (#Itdb_SPLRules)
 * @reserved100:        Reserved for MHOD100 implementation
 * @reserved101:        Reserved for MHOD100 implementation
 * @reserved_int1:      Reserved for future use
 * @reserved_int2:      Reserved for future use
 * @priv:               Private data
 * @reserved2:          Reserved for future use
 * @usertype:           For use by application
 * @userdata:           For use by application
 * @userdata_duplicate: A function to duplicate #userdata
 * @userdata_destroy:   A function to free #userdata
 *
 * Structure representing an iTunes Playlist
 */
struct _Itdb_Playlist
{
    Itdb_iTunesDB *itdb;
    gchar *name;
    guint8 type;
    guint8 flag1;
    guint8 flag2;
    guint8 flag3;
    gint  num;
    GList *members;
    gboolean is_spl;
    time_t timestamp;
    guint64 id;
    guint32 sortorder;
    guint32 podcastflag;
    Itdb_SPLPref splpref;
    Itdb_SPLRules splrules;
    gpointer reserved100;
    gpointer reserved101;
    /* reserved for future use */
    gint32 reserved_int1;
    gint32 reserved_int2;
    Itdb_Playlist_Private *priv;
    gpointer reserved2;
    /* below is for use by application */
    guint64 usertype;
    gpointer userdata;
    /* functions called to duplicate/free userdata */
    ItdbUserDataDuplicateFunc userdata_duplicate;
    ItdbUserDataDestroyFunc userdata_destroy;
};


/**
 * ItdbPlaylistSortOrder:
 * @ITDB_PSO_MANUAL:        Sort by playlist order (manual sort)
 * @ITDB_PSO_TITLE:         Sort by track title
 * @ITDB_PSO_ALBUM:         Sort by album
 * @ITDB_PSO_ARTIST:        Sort by artist
 * @ITDB_PSO_BITRATE:       Sort by bitrate
 * @ITDB_PSO_GENRE:         Sort by genre
 * @ITDB_PSO_FILETYPE:      Sort by filetype
 * @ITDB_PSO_TIME_MODIFIED: Sort by date modified
 * @ITDB_PSO_TRACK_NR:      Sort by track number
 * @ITDB_PSO_SIZE:          Sort by track size
 * @ITDB_PSO_TIME:          Sort by track time
 * @ITDB_PSO_YEAR:          Sort by year
 * @ITDB_PSO_SAMPLERATE:    Sort by samplerate
 * @ITDB_PSO_COMMENT:       Sort by comments
 * @ITDB_PSO_TIME_ADDED:    Sort by date added
 * @ITDB_PSO_EQUALIZER:     Sort by equilizer
 * @ITDB_PSO_COMPOSER:      Sort by composer
 * @ITDB_PSO_PLAYCOUNT:     Sort by playcount
 * @ITDB_PSO_TIME_PLAYED:   Sort by date last played
 * @ITDB_PSO_CD_NR:         Sort by disc number
 * @ITDB_PSO_RATING:        Sort by rating
 * @ITDB_PSO_RELEASE_DATE:  Sort by release date
 * @ITDB_PSO_BPM:           Sort by beats per minute
 * @ITDB_PSO_GROUPING:      Sort by grouping
 * @ITDB_PSO_CATEGORY:      Sort by category
 * @ITDB_PSO_DESCRIPTION:   Sort by description
 *
 * Playlist Sort Order
 *
 * Since: 0.1.3
 */
typedef enum
{
    ITDB_PSO_MANUAL = 1,
/*    ITDB_PSO_UNKNOWN = 2, */
    ITDB_PSO_TITLE = 3,
    ITDB_PSO_ALBUM = 4,
    ITDB_PSO_ARTIST = 5,
    ITDB_PSO_BITRATE = 6,
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
/* libgpod 0.7.90 and earlier had a typo in ITDB_PSO_BITRATE, workaround
 * that typo and avoid breaking the API (even if I doubt there are users 
 * of this)
 */
#define ITDB_PSO_BIRATE ITDB_PSO_BITRATE


/**
 * Itdb_Mediatype:
 * @ITDB_MEDIATYPE_AUDIO:      Audio files
 * @ITDB_MEDIATYPE_MOVIE:      Movies
 * @ITDB_MEDIATYPE_PODCAST:    Podcasts
 * @ITDB_MEDIATYPE_VIDEO_PODCAST:    Video Podcasts
 * @ITDB_MEDIATYPE_AUDIOBOOK:  Audio books
 * @ITDB_MEDIATYPE_MUSICVIDEO: Music videos
 * @ITDB_MEDIATYPE_TVSHOW:     TV Shows
 * @ITDB_MEDIATYPE_MUSIC_TVSHOW:     TV Shows (also show in Music)
 * @ITDB_MEDIATYPE_RINGTONE:   Ringtone
 * @ITDB_MEDIATYPE_RENTAL:     Rental
 * @ITDB_MEDIATYPE_ITUNES_EXTRA: ?
 * @ITDB_MEDIATYPE_MEMO:       Memo
 * @ITDB_MEDIATYPE_ITUNES_U:   iTunes U
 *
 * Mediatype definitions
 *
 * The mediatype is used to determine what menu a track appears under.  For
 * example, setting the mediatype to #ITDB_MEDIATYPE_PODCAST makes the track
 * appear on the Podcast menu. Media type is a bitfield, so it can be a
 * binary combination of these constants, make sure to use binary operators
 * when you want to operate on media types (eg use a binary AND in
 * preference over a straight == when you want to test if a track has a
 * given media type).
 *
 * Since: 0.5.0
 */
typedef enum
{
    ITDB_MEDIATYPE_AUDIO        = (1 << 0),
    ITDB_MEDIATYPE_MOVIE        = (1 << 1),
    ITDB_MEDIATYPE_PODCAST      = (1 << 2),
    ITDB_MEDIATYPE_AUDIOBOOK    = (1 << 3),
    ITDB_MEDIATYPE_MUSICVIDEO   = (1 << 5),
    ITDB_MEDIATYPE_TVSHOW       = (1 << 6),
    ITDB_MEDIATYPE_RINGTONE     = (1 << 14),
    ITDB_MEDIATYPE_RENTAL       = (1 << 15),
    ITDB_MEDIATYPE_ITUNES_EXTRA = (1 << 16),
    ITDB_MEDIATYPE_MEMO         = (1 << 20),
    ITDB_MEDIATYPE_ITUNES_U     = (1 << 21),
    ITDB_MEDIATYPE_EPUB_BOOK    = (1 << 22)
} Itdb_Mediatype;

/* Known compound media types which have been observed in iPod databases.
 * This list is in no way exhaustive, and these constants are only helpers,
 * there's nothing wrong with not using them.
 */
#define ITDB_MEDIATYPE_VIDEO_PODCAST (ITDB_MEDIATYPE_MOVIE | ITDB_MEDIATYPE_PODCAST)
#define ITDB_MEDIATYPE_MUSIC_TVSHOW (ITDB_MEDIATYPE_MUSICVIDEO | ITDB_MEDIATYPE_TVSHOW)
typedef struct _Itdb_Track_Private Itdb_Track_Private;
/**
 * Itdb_Track:
 * @itdb:                       A pointer to the #Itdb_iTunesDB (for convenience)
 * @title:                      The title of the track in UTF8
 * @ipod_path:                  The file path on the iPod.  Directories are
 *                              separated with ":" instead of "/".  The path is
 *                              relative to the iPod mountpoint.
 * @album:                      The album name in UTF8
 * @artist:                     The artist name in UTF8
 * @genre:                      The genre in UTF8
 * @filetype:                   A UTF8 string describing the file type.  E.g.
 *                              "MP3-File".
 * @comment:                    A comment in UTF8
 * @category:                   The category ("Technology", "Music", etc.)
 *                              where the podcast was located.  (Added in
 *                              dbversion 0x0d)
 * @composer:                   The composer name in UTF8
 * @grouping:                   ??? (UTF8)
 * @description:                Description text (such as podcast show notes).
 *                              (Added in dbversion 0x0d)
 * @podcasturl:                 Podcast Enclosure URL in UTF-8 or ASCII.
 *                              (Added in dbversion 0x0d)
 * @podcastrss:                 Podcast RSS URL in UTF-8 or ASCII.
 *                              (Added in dbversion 0x0d)
 * @chapterdata:                This is an m4a-style entry that is used to
 *                              display subsongs within a mhit.  (Added in
 *                              dbversion 0x0d)
 * @subtitle:                   Subtitle (usually the same as Description).
 *                              (Added in dbversion 0x0d)
 * @tvshow:                     Name of the TV show (only used for TV Shows).
 *                              (Added in dbversion 0x0d?) (Since libgpod-0.4.2)
 * @tvepisode:                  The episode number (only used for TV Shows).
 *                              (Added in dbversion 0x0d?) (Since libgpod-0.4.2)
 * @tvnetwork:                  The TV network (only used for TV Shows).
 *                              (Added in dbversion 0x0d?) (Since libgpod-0.4.2)
 * @albumartist:                The album artist (Added in dbversion 0x13?)
 *                              (Since libgpod-0.4.2)
 * @keywords:                   List of keywords pertaining to the track.
 *                              (Added in dbversion 0x13?) (Since libgpod-0.4.2)
 * @sort_artist:                The artist name used for sorting.  Artists with
 *                              names like "The Artist" would have "Artist,
 *                              The" here.  If you do not set this field,
 *                              libgpod will pre-sort the lists displayed by
 *                              the iPod according to "Artist, The", without
 *                              setting this field.
 *                              (Added in dbversion 0x13?) (Since libgpod-0.5.0)
 * @sort_title:                 The track title used for sorting.  See
 *                              @sort_artist. (Since libgpod-0.5.0)
 * @sort_album:                 The album name used for sorting.  See
 *                              @sort_artist. (Since libgpod-0.5.0)
 * @sort_albumartist:           The album artist used for sorting.  See
 *                              @sort_artist. (Since libgpod-0.5.0)
 * @sort_composer:              The composer used for sorting.  See
 *                              @sort_artist. (Since libgpod-0.5.0)
 * @sort_tvshow:                The name of the TV show used for sorting.  See
 *                              @sort_artist. (Since libgpod-0.5.0)
 * @id:                         Unique ID of track
 * @size:                       The size of the file in bytes
 * @tracklen:                   The length of the track in ms
 * @cd_nr:                      The CD number the track comes from.
 * @cds:                        The total number of CDs.
 * @track_nr:                   The track number.
 * @tracks:                     The total number of tracks.
 * @bitrate:                    The bitrate at which the file is encoded.
 * @samplerate:                 The samplerate of the track (e.g. CD = 44100)
 * @samplerate_low:             In the iTunesDB the samplerate is
 *                              multiplied by 0x10000 -- these are the
 *                              lower 16 bit, which are usually 0
 * @year:                       The year the track was released
 * @volume:                     Volume adjustment field.  This is a value from
 *                              -255 to 255 that will be applied to the track
 *                              on playback.
 * @soundcheck:                 The SoundCheck value to apply to the song, when
 *                              SoundCheck is switched on in the iPod settings.
 *                              The value for this field can be determined by
 *                              the equation: X = 1000 * 10 ^ (-.1 * Y) where Y
 *                              is the adjustment value in dB and X is the
 *                              value that goes into the SoundCheck field. The
 *                              value 0 is special, the equation is not used
 *                              and it is treated as "no Soundcheck" (basically
 *                              the same as the value 1000). This equation
 *                              works perfectly well with ReplayGain derived
 *                              data instead of the iTunes SoundCheck derived
 *                              information.
 * @time_added:                 The time the track was added.
 * @time_modified:              The time the track was last modified
 * @time_played:                The time the track was last played
 * @bookmark_time:              The time, in milliseconds, that the track will
 *                              start playing at. This is used for AudioBook
 *                              filetypes (.aa and .m4b).  Note that there is
 *                              also a bookmark value in the play counts file
 *                              that will be set by the iPod and can be used
 *                              instead of this value.
 * @rating:                     The track rating (stars * #ITDB_RATING_STEP)
 * @playcount:                  The number of times the track has been played
 * @playcount2:                 This also stores the play count of the
 *                              track.  It is unclear if this ever differs
 *                              from the above value. During sync, this is set
 *                              to the same value as @playcount.
 * @recent_playcount:           The number of times the track was played since
 *                              the last sync.
 * @transferred:                Whether the file been transferred to iPod.
 * @BPM:                        BPM (beats per minute) of the track
 * @app_rating:                 The last rating set by an application (e.g.
 *                              iTunes).  If the rating on the iPod and the
 *                              @rating field above differ, the original
 *                              rating is copied here and the new rating is
 *                              stored in @rating.
 * @type1:                      CBR MP3s and AAC are 0x00, VBR MP3s are 0x01
 * @type2:                      MP3s are 0x01, AAC are 0x00
 * @compilation:                Flag to mark the track as a compilation.  True
 *                              if set to 0x1, false if set to 0x0.
 * @starttime:                  The offset, in milliseconds, at which the song
 *                              will start playing.
 * @stoptime:                   The offset, in milliseconds, at which the song
 *                              will stop playing.
 * @checked:                    Flag for whether the track is checked.  True if
 *                              set to 0x0, false if set to 0x1
 * @dbid:                       Unique database ID that identifies this song
 *                              across the databases on the iPod. For example,
 *                              this id joins an iTunesDB mhit with an
 *                              ArtworkDB mhii.
 * @drm_userid:                 Apple Store/Audible User ID (for DRM'ed files
 *                              only, set to 0 otherwise).
 * @visible:                    If this value is 1, the song is visible on the
 *                              iPod. All other values cause the file to be
 *                              hidden.
 * @filetype_marker:            This appears to always be 0 on hard drive based
 *                              iPods, but for the iTunesDB that is written to
 *                              an iPod Shuffle, iTunes 4.7.1 writes out the
 *                              file's type as an ANSI string(!). For example,
 *                              a MP3 file has a filetype of 0x4d503320 ->
 *                              0x4d = 'M', 0x50 = 'P', 0x33 = '3', 0x20 =
 *                              &lt;space&gt;.  This is set to the filename
 *                              extension by libgpod when copying the track to
 *                              the iPod.
 * @artwork_count:              The number of album artwork items associated
 *                              with this song.  libgpod updates this value
 *                              when syncing.
 * @artwork_size:               The total size of artwork (in bytes) attached
 *                              to this song, when it is converted to JPEG
 *                              format. Observed in dbversion 0x0b and with
 *                              an iPod Photo. libgpod updates this value when
 *                              syncing.
 * @samplerate2:                The sample rate of the song expressed as an
 *                              IEEE 32 bit floating point number.  It is
 *                              uncertain why this is here.  libgpod will set
 *                              this when adding a track.
 * @unk126:                     Unknown, but always seems to be 0xffff for
 *                              MP3/AAC songs, 0x0 for uncompressed songs
 *                              (like WAVE format), 0x1 for Audible.  libgpod
 *                              will try to set this when adding a new track.
 * @unk132:                     Unknown
 * @time_released:              The date/time the track was added to the iTunes
 *                              music store?  For podcasts this is the release
 *                              date that is displayed next to the title in the
 *                              Podcast playlist.
 * @unk144:                     Unknown, but MP3 songs appear to be always
 *                              0x000c, AAC songs are always 0x0033, Audible
 *                              files are 0x0029, WAV files are 0x0.  libgpod
 *                              will attempt to set this value when adding a
 *                              track.
 * @explicit_flag:              Flag to mark a track as "explicit" in iTunes.
 *                              True if to 0x1, false if set to 0x0.
 * @unk148:                     Unknown - used for Apple Store DRM songs
 *                              (always 0x01010100?), zero otherwise
 * @unk152:                     Unknown
 * @skipcount:                  The number of times the track has been skipped.
 *                              (Added in dbversion 0x0c)
 * @recent_skipcount:           The number of times the track was skipped since
 *                              the last sync.
 * @last_skipped:               The time the track was last skipped.  (Added in
 *                              dbversion 0x0c)
 * @has_artwork:                Whether the track has artwork.
 *                              True if set to 0x01, false if set to 0x02.
 * @skip_when_shuffling:        Flag to skip the track when shuffling.  True if
 *                              set to 0x01, false if set to 0x00. Audiobooks
 *                              (.aa and .m4b) always seem to be skipped when
 *                              shuffling, regardless of this flag.
 * @remember_playback_position: Flag to remember playback position.
 *                              True when set to 0x01, false when set to 0x00.
 *                              Audiobooks (.aa and .m4b) always seem to
 *                              remember the playback position, regardless of
 *                              this flag.
 * @flag4:                      Used for podcasts, 0x00 otherwise.  If set to
 *                              0x01 the "Now Playing" page will show
 *                              Title/Album, when set to 0x00 it will also show
 *                              the Artist.  When set to 0x02 a sub-page
 *                              (middle button) with further information about
 *                              the track will be shown.
 * @dbid2:                      The purpose of the field is unclear.  If not
 *                              set, libgpod will set this to the same value as
 *                              @dbid when adding a track.  (With iTunes, since
 *                              dbversion 0x12, this field value differs from
 *                              @dbid.)
 * @lyrics_flag:                Whether the track has lyrics (e.g. it has a
 *                              USLT ID3 tag in an MP3 or a @lyr atom in an
 *                              MP4).  True if set to 0x01, false if set to
 *                              0x00.
 * @movie_flag:                 Whether the track is a movie.  True if set to
 *                              0x01, false if set to 0x00.
 * @mark_unplayed:              A value of 0x02 marks a podcast as unplayed on
 *                              the iPod, with a bullet.  Once played it is set
 *                              to 0x01. Non-podcasts have this set to 0x01.
 *                              (Added in dbversion 0x0c)
 * @unk179:                     Unknown, always 0x00 so far.  (Added in
 *                              dbversion 0x0c)
 * @unk180:                     Unknown.  (Added in dbversion 0x0c)
 * @pregap:                     The number of samples of silence before the
 *                              track starts (for gapless playback).
 * @samplecount:                The number of samples in the track (for gapless
 *                              playback).
 * @unk196:                     Unknown.  (Added in dbversion 0x0c)
 * @postgap:                    The number of samples of silence at the end of
 *                              the track (for gapless playback).
 * @unk204:                     Unknown.  Appears to be 0x1 for files encoded
 *                              using the MP3 encoder, 0x0 otherwise.  (Added
 *                              in dbversion 0x0c, first values observed in
 *                              0x0d.)
 * @mediatype:                  The type of file.  It must be set to 0x00000001
 *                              for audio files, and set to 0x00000002 for
 *                              video files.  If set to 0x00, the files show up
 *                              in both, the audio menus ("Songs", "Artists",
 *                              etc.) and the video menus ("Movies", "Music
 *                              Videos", etc.).  It appears to be set to 0x20
 *                              for music videos, and if set to 0x60 the file
 *                              shows up in "TV Shows" rather than "Movies".
 *                              <para>
 *                              The following list summarizes all observed types:
 *                              </para>
 *                              <itemizedlist>
 *                              <listitem>0x00 00 00 00 - Audio/Video</listitem>
 *                              <listitem>0x00 00 00 01 - Audio</listitem>
 *                              <listitem>0x00 00 00 02 - Video</listitem>
 *                              <listitem>0x00 00 00 04 - Podcast</listitem>
 *                              <listitem>0x00 00 00 06 - Video Podcast</listitem>
 *                              <listitem>0x00 00 00 08 - Audiobook</listitem>
 *                              <listitem>0x00 00 00 20 - Music Video</listitem>
 *                              <listitem>0x00 00 00 40 - TV Show (shows up ONLY
 *                              in TV Shows)</listitem>
 *                              <listitem>0x00 00 00 60 - TV Show (shows up in
 *                              the Music lists as well)</listitem>
 *                              </itemizedlist>
 * @season_nr:                  The season number of the track (only used for
 *                              TV Shows).
 * @episode_nr:                 The episode number of the track (only used for
 *                              TV Shows).  Although this is not displayed on
 *                              the iPod, the episodes are sorted by episode
 *                              number.
 * @unk220:                     Unknown.  This has something to do with
 *                              protected files.  It is set to 0x0 for
 *                              non-protected files.
 * @unk224:                     Unknown.  (Added in dbversion 0x0c)
 * @unk228:                     Unknown.  (Added in dbversion 0x0c)
 * @unk232:                     Unknown.  (Added in dbversion 0x0c)
 * @unk236:                     Unknown.  (Added in dbversion 0x0c)
 * @unk240:                     Unknown.  (Added in dbversion 0x0c)
 * @unk244:                     Unknown.  (Added in dbversion 0x13)
 * @gapless_data:               The size in bytes from first Synch Frame
 *                              (which is usually the XING frame that
 *                              includes the LAME tag) until the 8th before
 *                              the last frame. The gapless playback does not
 *                              work for MP3 files if this is set to zero. For
 *                              AAC tracks, this may be zero.  (Added in
 *                              dbversion 0x13)
 * @unk252:                     Unknown.  (Added in dbversion 0x0c)
 * @gapless_track_flag:         If set to 1, this track has gapless playback
 *                              data.  (Added in dbversion 0x13)
 * @gapless_album_flag:         If set to 1, this track does not use
 *                              crossfading in iTunes.  (Added in dbversion
 *                              0x13)
 * @album_id:                   The Album ID from the album list (currently
 *                              unused by libgpod)
 * @artwork:                    An #Itdb_Artwork for cover art
 * @mhii_link:                  This is set to the id of the corresponding
 *                              ArtworkDB mhii, used for sparse artwork
 *                              support.  (Since libgpod-0.7.0)
 * @reserved_int1:              Reserved for future use
 * @reserved_int2:              Reserved for future use
 * @reserved_int3:              Reserved for future use
 * @reserved_int4:              Reserved for future use
 * @reserved_int5:              Reserved for future use
 * @reserved_int6:              Reserved for future use
 * @reserved1:                  Reserved for future use
 * @reserved2:                  Reserved for future use
 * @reserved3:                  Reserved for future use
 * @reserved4:                  Reserved for future use
 * @reserved5:                  Reserved for future use
 * @reserved6:                  Reserved for future use
 * @usertype:                   For use by application
 * @userdata:                   For use by application
 * @userdata_duplicate:         A function to duplicate #userdata
 * @userdata_destroy:           A function to free #userdata
 * @priv:                       Private data
 *
 * Structure representing a track in an iTunesDB
 *
 * <note><para>When adding string fields don't forget to add them in
 * itdb_track_duplicate() as well.</para></note>
 *
 * Many of the parameter descriptions are copied verbatim from
 * http://ipodlinux.org/ITunesDB, which is the best source for information about
 * the iTunesDB and related files.
 */
struct _Itdb_Track
{
  Itdb_iTunesDB *itdb;
  gchar   *title;
  gchar   *ipod_path;
  gchar   *album;
  gchar   *artist;
  gchar   *genre;
  gchar   *filetype;
  gchar   *comment;
  gchar   *category;
  gchar   *composer;
  gchar   *grouping;
  gchar   *description;
  gchar   *podcasturl;
  gchar   *podcastrss;
  Itdb_Chapterdata *chapterdata;
  gchar   *subtitle;
/* the following 5 are new in libgpod 0.4.2... */
  gchar   *tvshow;
  gchar   *tvepisode;
  gchar   *tvnetwork;
  gchar   *albumartist;
  gchar   *keywords;
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
  gchar   *sort_artist;
  gchar   *sort_title;
  gchar   *sort_album;
  gchar   *sort_albumartist;
  gchar   *sort_composer;
  gchar   *sort_tvshow;
/* end of new fields in libgpod 0.5.0 */
  guint32 id;
  gint32  size;
  gint32  tracklen;
  gint32  cd_nr;
  gint32  cds;
  gint32  track_nr;
  gint32  tracks;
  gint32  bitrate;
  guint16 samplerate;
  guint16 samplerate_low;
  gint32  year;
  gint32  volume;
  guint32 soundcheck;
  time_t  time_added;
  time_t  time_modified;
  time_t  time_played;
  guint32 bookmark_time;
  guint32 rating;
  guint32 playcount;
  guint32 playcount2;
  guint32 recent_playcount;
  gboolean transferred;
  gint16  BPM;
  guint8  app_rating;
  guint8  type1;
  guint8  type2;
  guint8  compilation;
  guint32 starttime;
  guint32 stoptime;
  guint8  checked;
  guint64 dbid;
  guint32 drm_userid;
  guint32 visible;
  guint32 filetype_marker;
  guint16 artwork_count;
  guint32 artwork_size;
  float samplerate2;
  guint16 unk126;
  guint32 unk132;
  time_t  time_released;
  guint16 unk144;
  guint16 explicit_flag;
  guint32 unk148;
  guint32 unk152;
  guint32 skipcount;
  guint32 recent_skipcount;
  guint32 last_skipped;
  guint8 has_artwork;
  guint8 skip_when_shuffling;
  guint8 remember_playback_position;
  guint8 flag4;
  guint64 dbid2;
  guint8 lyrics_flag;
  guint8 movie_flag;
  guint8 mark_unplayed;
  guint8 unk179;
  guint32 unk180;
  guint32 pregap;
  guint64 samplecount;
  guint32 unk196;
  guint32 postgap;
  guint32 unk204;
  guint32 mediatype;
  guint32 season_nr;
  guint32 episode_nr;
  guint32 unk220;
  guint32 unk224;
  guint32 unk228, unk232, unk236, unk240, unk244;
  guint32 gapless_data;
  guint32 unk252;
  guint16 gapless_track_flag;
  guint16 gapless_album_flag;
  guint16 obsolete;

  /* This is for Cover Art support */
  struct _Itdb_Artwork *artwork;

  /* This is for sparse artwork support, new in libgpod-0.7.0 */
  guint32 mhii_link;

  /* reserved for future use */
  gint32 reserved_int1;
  gint32 reserved_int2;
  gint32 reserved_int3;
  gint32 reserved_int4;
  gint32 reserved_int5;
  gint32 reserved_int6;
  Itdb_Track_Private *priv;
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

/**
 * ItdbFileError:
 * @ITDB_FILE_ERROR_SEEK:         file corrupt: illegal seek occured
 * @ITDB_FILE_ERROR_CORRUPT:      file corrupt
 * @ITDB_FILE_ERROR_NOTFOUND:     file not found
 * @ITDB_FILE_ERROR_RENAME:       file could not be renamed
 * @ITDB_FILE_ERROR_ITDB_CORRUPT: iTunesDB in memory corrupt
 *
 * Error codes for iTunesDB file
 */
typedef enum
{
    ITDB_FILE_ERROR_SEEK,
    ITDB_FILE_ERROR_CORRUPT,
    ITDB_FILE_ERROR_NOTFOUND,
    ITDB_FILE_ERROR_RENAME,
    ITDB_FILE_ERROR_ITDB_CORRUPT
} ItdbFileError;

typedef enum
{
    ITDB_ERROR_SEEK,
    ITDB_ERROR_CORRUPT,
    ITDB_ERROR_NOTFOUND,
    ITDB_ERROR_RENAME,
    ITDB_ERROR_ITDB_CORRUPT,
    ITDB_ERROR_SQLITE
} ItdbError;


/* Error domain */
#define ITDB_ERROR itdb_file_error_quark ()
#define ITDB_FILE_ERROR ITDB_ERROR
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
gboolean itdb_start_sync (Itdb_iTunesDB *itdb);
gboolean itdb_stop_sync (Itdb_iTunesDB *itdb);
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
gchar *itdb_get_itunescdb_path (const gchar *mountpoint);
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
gboolean itdb_device_supports_chapter_image (const Itdb_Device *device);
gboolean itdb_device_supports_video (const Itdb_Device *device);
gboolean itdb_device_supports_photo (const Itdb_Device *device);
gboolean itdb_device_supports_podcast (const Itdb_Device *device);
const gchar *itdb_info_get_ipod_model_name_string (Itdb_IpodModel model);
const gchar *itdb_info_get_ipod_generation_string (Itdb_IpodGeneration generation);
gchar *itdb_device_get_uuid(const Itdb_Device *device);

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
void itdb_playlist_move (Itdb_Playlist *pl, gint32 pos);
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

/* playlist functions for audiobooks playlist */
gboolean itdb_playlist_is_audiobooks (Itdb_Playlist *pl);

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
Itdb_PhotoAlbum *itdb_photodb_photoalbum_new (const gchar *albumname);
void itdb_photodb_photoalbum_free (Itdb_PhotoAlbum *album);
void itdb_photodb_photoalbum_add (Itdb_PhotoDB *db, Itdb_PhotoAlbum *album, gint pos);
void itdb_photodb_free (Itdb_PhotoDB *photodb);
gboolean itdb_photodb_write (Itdb_PhotoDB *photodb, GError **error);
void itdb_photodb_remove_photo (Itdb_PhotoDB *db,
				Itdb_PhotoAlbum *album,
				Itdb_Artwork *photo);
void itdb_photodb_photoalbum_remove (Itdb_PhotoDB *db,
				     Itdb_PhotoAlbum *album,
				     gboolean remove_pics);
void itdb_photodb_photoalbum_unlink (Itdb_PhotoAlbum *album);
Itdb_PhotoAlbum *itdb_photodb_photoalbum_by_name(Itdb_PhotoDB *db,
						 const gchar *albumname );

/* itdb_artwork_... -- you probably won't need many of these (with
 * the exception of itdb_artwork_get_pixbuf() probably). Use the
 * itdb_photodb_...() functions when adding photos, and the
 * itdb_track_...() functions when adding coverart to audio. */
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
/* the following function returns a pointer to a GdkPixbuf if
   gdk-pixbuf is installed -- a NULL pointer otherwise. */
gpointer itdb_artwork_get_pixbuf (Itdb_Device *device, Itdb_Artwork *artwork,
                                  gint width, gint height);

/* itdb_thumb_... */
Itdb_Thumb *itdb_thumb_duplicate (Itdb_Thumb *thumb);
gpointer itdb_thumb_to_pixbuf_at_size (Itdb_Device *device, Itdb_Thumb *thumb,
                                       gint width, gint height);
GList *itdb_thumb_to_pixbufs (Itdb_Device *device, Itdb_Thumb *thumb);
void itdb_thumb_free (Itdb_Thumb *thumb);

/* itdb_chapterdata_... */
Itdb_Chapterdata *itdb_chapterdata_new (void);
void itdb_chapterdata_free (Itdb_Chapterdata *chapterdata);
Itdb_Chapterdata *itdb_chapterdata_duplicate (Itdb_Chapterdata *chapterdata);
void itdb_chapterdata_remove_chapter (Itdb_Chapterdata *chapterdata, Itdb_Chapter *chapter);
void itdb_chapterdata_unlink_chapter (Itdb_Chapterdata *chapterdata, Itdb_Chapter *chapter);
void itdb_chapterdata_remove_chapters (Itdb_Chapterdata *chapterdata);
Itdb_Chapter *itdb_chapter_new (void);
void itdb_chapter_free (Itdb_Chapter *chapter);
Itdb_Chapter *itdb_chapter_duplicate (Itdb_Chapter *chapter);
gboolean itdb_chapterdata_add_chapter (Itdb_Chapterdata *chapterdata,
				       guint32 startpos,
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

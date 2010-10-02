/*
 *  Copyright (C) 2005-2007 Christophe Fergeau
 *
 * 
 *  The code contained in this file is free software; you can redistribute
 *  it and/or modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either version
 *  2.1 of the License, or (at your option) any later version.
 *  
 *  This file is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *  
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this code; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 *  iTunes and iPod are trademarks of Apple
 * 
 *  This product is not supported/written/published by Apple!
 *
 * $Id$
 */


#ifndef DB_PARSER_H
#define DB_PARSER_H

#include <glib.h>

struct _MHeader {
	unsigned char header_id[4];
	gint32 header_len;
};

typedef struct _MHeader MHeader;

struct _MhlHeader {
	unsigned char header_id[4];
	gint32 header_len;
	gint32 num_children;
	unsigned char padding[];
};

typedef struct _MhlHeader MhlHeader;


typedef struct _MhbdHeader MhbdHeader;
typedef struct _ArtworkDB_MhsdHeader ArtworkDB_MhsdHeader;
/*typedef struct _MhsdHeader MhsdHeader;*/
typedef struct _MhltHeader MhltHeader;
typedef struct _MhlpHeader MhlpHeader;
typedef struct _MhypHeader MhypHeader;
typedef struct _MhipHeader MhipHeader;
typedef struct _MhitHeader MhitHeader;
typedef struct _ArtworkDB_MhodHeader ArtworkDB_MhodHeader;
/*typedef struct _MhodHeader MhodHeader;*/
typedef struct _MhfdHeader MhfdHeader;
typedef struct _MhliHeader MhliHeader;
typedef struct _MhiiHeader MhiiHeader;
typedef struct _MhniHeader MhniHeader;
typedef struct _MhlaHeader MhlaHeader;
typedef struct _MhbaHeader MhbaHeader;
typedef struct _MhlfHeader MhlfHeader;
typedef struct _MhifHeader MhifHeader;
typedef struct _MhiaHeader MhiaHeader;

typedef struct _MhitHeader471 MhitHeader471;
/* MHOD typedef mess */
typedef struct _ArtworkDB_MhodHeaderString ArtworkDB_MhodHeaderString;
typedef struct _MhodHeaderString MhodHeaderString;
typedef struct _MhodHeaderSmartPlaylistData MhodHeaderSmartPlaylistData;
typedef struct _MhodHeaderSmartPlaylistRuleString MhodHeaderSmartPlaylistRuleString; 
typedef struct _MhodHeaderSmartPlaylistRuleNonString MhodHeaderSmartPlaylistRuleNonString;
typedef struct _MhodHeaderSmartPlaylistRule MhodHeaderSmartPlaylistRule;

struct _MhbdHeader {
	gchar   header_id[4];
	guint32 header_len;
	guint32 total_len;
	guint32 unknown1;
	guint32 version;
	guint32 num_children;
	guint64 db_id;
	guint16 platform;
	guint16 unk_0x22;
	guint64 id_0x24;
	guint32 unk_0x2c;
	guint16 hashing_scheme;
	guchar  unk_0x32[20];
	gchar   language_id[2];
	guint64 db_persistent_id;
	guint32 unk_0x50;
	guint32 unk_0x54;
	guchar  hash58[20];
	gint32  timezone_offset;
	guint16 unk_0x70;
	guchar  hash72[46];
	guint16 audio_language;
	guint16 subtitle_language;
	guint16 unk_0xa4;
	guint16 unk_0xa6;
	guint16 unk_0xa8;
	guchar  padding[];
} __attribute__((__packed__));

enum MhsdIndexType {
	MHSD_TRACK_LIST = 1,
	MHSD_PLAYLIST_LIST = 2
};

enum MhsdPhotoIndexType {
	MHSD_IMAGE_LIST = 1,
	MHSD_ALBUM_LIST = 2,
	MHSD_FILE_LIST = 3
};

struct _ArtworkDB_MhsdHeader {
	unsigned char header_id[4];
	gint32 header_len;
	gint32 total_len;
        /* Strangely, the following field is only 16 bits long in the
	 * ArtworkDB (it's definitely 32 bits in the iTunesDB). This
	 * could well be an error with the first generation of mobile
	 * phones with iPod support).
	 */
	gint16 index;
        gint16 unknown014;
	unsigned char padding[];
};

struct _iTunesDB_MhsdHeader {
	unsigned char header_id[4];
	gint32 header_len;
	gint32 total_len;
	gint32 index;
	unsigned char padding[];
};
struct _MhltHeader {
	unsigned char header_id[4];
	gint32 header_len;
	gint32 num_songs;
	unsigned char padding[];
};

struct _MhlpHeader {
	unsigned char header_id[4];
	gint32 header_len;
	gint32 num_playlists;
	unsigned char padding[];
};

struct _MhypHeader {
	unsigned char header_id[4];
	gint32 header_len;
	gint32 total_len;
	gint32 num_mhod;
	gint32 num_items;
	gint32 hidden;
	gint32 timestamp;
	gint32 playlist_id;
	gint32 unknown3;
	gint32 unknown4;
	gint32 unknown5;
	unsigned char padding[];
};

struct _MhipHeader {
	unsigned char header_id[4];
	gint32 header_len;
	gint32 total_len;
	gint32 unknown1;
	gint32 format_id;
	gint32 unknown2;
	gint32 track_id;
	gint32 timestamp;
	unsigned char padding[];
};

/* MHIT header as written by iTunes 4.7.1, the added fields (from unknown17 to
 * the end of the struct) are pretty useless, so we don't use this struct
 */
struct _MhitHeader471 {
	unsigned char header_id[4];
	gint32 header_len;
	gint32 total_len;
	gint32 num_mhod;
	gint32 track_id;
	gint32 visible;
	gint32 filetype;
	/* FIXME: endianness issue with the order of the 3 fields above ? */
	gint16 type;
	gchar  compilation;
	gchar  rating;
	gint32 date_added;
	gint32 size;
	gint32 length;
	gint32 track_number;
	gint32 track_total;
	gint32 year;
	gint32 bitrate;
	/* FIXME: endianness issue with the order of the 2 fields above ? */
	gint16 unknown;
	gint16 sample_rate;
	gint32 volume;
	gint32 start_time;
	gint32 stop_time;
	gint32 sound_check;
	gint32 play_count;
	gint32 play_count2;
	gint32 last_played;
	gint32 disc_number;
	gint32 disc_total;
	gint32 user_id;
	gint32 last_modified;
	gint32 bookmark_time;
	gint64 song_id;
	/* FIXME: endianness issue with the order of the 5 fields above ? */
	gchar  checked;
	gchar  app_rating;
	gint16 bpm;
	gint16 artwork_count;
	gint16 unknown9;
	gint32 artwork_size;
	gint32 unknown11;
	gint32 sample_rate2;
	gint32 unknown13;
	gint32 unknown14;
	gint32 unknown15;
	gint32 unknown16;
	gint32 unknown17;
	gint32 unknown18;
	gint32 unknown19;
	gint64 song_id2;
	gint32 unknown20;
	gint32 unknown21;
	gint32 unknown22;
	gint32 unknown23;
	gint32 unknown24;
	gint32 unknown25;
	gint32 unknown26;
	gint32 unknown27;
	gint32 unknown28;
	gint32 unknown29;
	gint32 unknown30;
	gint32 unknown31;
	gint32 unknown32;
	gint32 unknown33;
	gint32 unknown34;
	gint32 unknown35;
	gint32 unknown36;
	unsigned char padding[];
};

struct _MhitHeader {
	unsigned char header_id[4];
	gint32 header_len;
	gint32 total_len;
	gint32 num_mhod;
	gint32 track_id;
	gint32 visible;
	gint32 filetype;
	/* FIXME: endianness issue with the order of the 3 fields above ? */
	gint16 type;
	gchar  compilation;
	gchar  rating;
	gint32 date_added;
	gint32 size;
	gint32 length;
	gint32 track_number;
	gint32 track_total;
	gint32 year;
	gint32 bitrate;
	/* FIXME: endianness issue with the order of the 2 fields above ? */
	gint16 unknown;
	gint16 sample_rate;
	gint32 volume;
	gint32 start_time;
	gint32 stop_time;
	gint32 sound_check;
	gint32 play_count;
	gint32 play_count2;
	gint32 last_played;
	gint32 disc_number;
	gint32 disc_total;
	gint32 user_id;
	gint32 last_modified;
	gint32 bookmark_time;
	gint64 song_id;
	/* FIXME: endianness issue with the order of the 5 fields above ? */
	gchar  checked;
	gchar  app_rating;
	gint16 bpm;
	gint16 artwork_count;
	gint16 unknown9;
	gint32 artwork_size;
	gint32 unknown11;
	gint32 sample_rate2;
	gint32 unknown13;
	gint32 unknown14;
	gint32 unknown15;
	gint32 unknown16;
	unsigned char padding[];
};

enum MhodEncoding {
	MHOD_ENCODING_UTF16 = 0,
	MHOD_ENCODING_UTF8 = 1
};

enum MhodType {
	MHOD_TYPE_TITLE = 1,
	MHOD_TYPE_LOCATION = 2,
	MHOD_TYPE_ALBUM = 3,
	MHOD_TYPE_ARTIST = 4,
	MHOD_TYPE_GENRE = 5,
	MHOD_TYPE_FILETYPE = 6,
	MHOD_TYPE_EQ_SETTING = 7,
	MHOD_TYPE_COMMENT = 8,
	MHOD_TYPE_COMPOSER = 12,
	MHOD_TYPE_GROUPING = 13,
	MHOD_TYPE_SMART_PLAYLIST_DATA = 50,
	MHOD_TYPE_SMART_PLAYLIST_RULES = 51,
	MHOD_TYPE_LIBRARY_PLAYLIST_INDEX = 52,
	MHOD_TYPE_100 = 100
};

struct _MhodHeader {
	unsigned char header_id[4];
	gint32 header_len;
	gint32 total_len;
	gint32 type;
	gint32 unknown1;
	gint32 unknown2;
};

struct _ArtworkDB_MhodHeader {
	unsigned char header_id[4];
	gint32 header_len;
	gint32 total_len;
    /* Strangely, the following field is only 16 bits long in the
	 * ArtworkDB (it's definitely 32 bits in the iTunesDB). This
	 * could well be an error with the first generation of mobile
	 * phones with iPod support).
	 */
	gint16 type;
        gint16 unknown014;
	gint32 unknown1;
	gint32 unknown2;
};

struct _MhodHeaderString {
	unsigned char header_id[4];
	gint32 header_len;
	gint32 total_len;
	gint32 type; /* < 50 */
	gint32 unknown1;
	gint32 unknown2;
	gint32 position;
	gint32 string_len;
	gint32 unknown3; /* It was thought that this was string encoding:
			  * 0 == UTF-16, 1 == UTF-8, however, recent iTunesDB
			  * files have had this set to 1 even with UTF-16 strings.
			  * Therefore this is definitely incorrect, and the
			  * correct meaning has not yet been discovered yet. */
	gint32 unknown4;
	unsigned char string[];
};

enum MhodArtworkType {
	MHOD_ARTWORK_TYPE_ALBUM_NAME = 1, /* string:    album name (in the Photo Database) */
	MHOD_ARTWORK_TYPE_THUMBNAIL  = 2, /* container: thumbnail image */
	MHOD_ARTWORK_TYPE_FILE_NAME  = 3, /* string:    file name */
	MHOD_ARTWORK_TYPE_IMAGE      = 5  /* container: full resolution image (in the Photo Database) */
};

struct _ArtworkDB_MhodHeaderString {
	unsigned char header_id[4];
	gint32 header_len;
	gint32 total_len;
        gint16 type;
        gint8  unknown13;
	gint8  padding_len;
	gint32 unknown1;
	gint32 unknown2;
	gint32 string_len;
	gint8 encoding; /* 0,1: string is UTF8, 2: string is UTF16-LE */
        gint8 unknown5;
        gint16 unknown6;
	gint32 unknown4;
        gchar  string[];
};

enum MhodLimitType {
	MHOD_LIMIT_MINUTES = 1,
	MHOD_LIMIT_MEGABYTES = 2,
	MHOD_LIMIT_SONGS = 3,
	MHOD_LIMIT_HOURS = 4,
	MHOD_LIMIT_GIGABYTES = 5
};

enum MhodLimitSortType {
	MHOD_LIMIT_SORT_RANDOM = 0x02,
	MHOD_LIMIT_SORT_SONG_NAME = 0x03,
	MHOD_LIMIT_SORT_ALBUM = 0x04,
	MHOD_LIMIT_SORT_ARTIST = 0x05,
	MHOD_LIMIT_SORT_GENRE = 0x07,
	MHOD_LIMIT_SORT_MOST_RECENTLY_ADDED = 0x10,
	MHOD_LIMIT_SORT_MOST_OFTEN_PLAYED = 0x14,
	MHOD_LIMIT_SORT_MOST_RECENTLY_PLAYED = 0x15,
	MHOD_LIMIT_SORT_HIGHEST_RATING = 0x17
};

struct _MhodHeaderSmartPlaylistData {
	unsigned char header_id[4];
	gint32 header_len;
	gint32 total_len;
	gint32 type; /* 50 */
	gint32 unknown1;
	gint32 unknown2;
	gchar  live_update;
	gchar  rules_enable;
	gchar  limit_enable;
	gchar  limit_type;
	gchar  limit_sort;
	gchar  unknow3[3];
	gint32 limit;
	gchar  match_checked_only;
	gchar  reverse_limit_sort;
	unsigned char padding[];
};


enum MhodSmartPlaylistRuleFieldType {
	MHOD_FIELD_SONG_NAME = 0x02,
	MHOD_FIELD_ALBUM = 0x03,
	MHOD_FIELD_ARTIST = 0x04,
	MHOD_FIELD_BITRATE = 0x05,
	MHOD_FIELD_SAMPLE_RATE = 0x06,
	MHOD_FIELD_YEAR = 0x07,
	MHOD_FIELD_GENRE = 0x08,
	MHOD_FIELD_KIND = 0x09,	
	MHOD_FIELD_DATE_MODIFIED = 0x0a,
	MHOD_FIELD_TRACK_NUMBER = 0x0b,
	MHOD_FIELD_SIZE = 0x0c,
	MHOD_FIELD_TIME = 0x0d,
	MHOD_FIELD_COMMENT = 0x0e,
	MHOD_FIELD_DATE_ADDED = 0x10,
	MHOD_FIELD_COMPOSER = 0x12,
	MHOD_FIELD_PLAY_COUNT = 0x16,
	MHOD_FIELD_LAST_PLAYED = 0x17,
	MHOD_FIELD_DISC_NUMBER = 0x18,
	MHOD_FIELD_RATING = 0x19,
	MHOD_FIELD_COMPILATION = 0x1f,
	MHOD_FIELD_BPM = 0x23,
	MHOD_FIELD_GROUPING = 0x27,
	MHOD_FIELD_PLAYLIST = 0x28
};

enum MhodSmartPlaylistRuleAction {
	MHOD_RULE_IS = 1 << 0, 
	MHOD_RULE_CONTAINS = 1 << 1,
	MHOD_RULE_BEGINS_WITH = 1 << 2,
	MHOD_RULE_ENDS_WITH = 1 << 3,
	MHOD_RULE_GREATER_THAN = 1 << 4,
	MHOD_RULE_GREATER_THAN_OR_EQUAL_TO = 1 << 5,
	MHOD_RULE_LESS_THAN = 1 << 6,
	MHOD_RULE_LESS_THAN_OR_EQUAL_TO = 1 << 7,
	MHOD_RULE_IN_THE_RANGE = 1 << 8,
	MHOD_RULE_IS_THE_LAST = 1 << 9,
	MHOD_RULE_NOT = 1 << 24,
	MHOD_RULE_STRING = 1 << 25
};


struct _MhodHeaderSmartPlaylistRuleString {
	/* Big endian fields */
	gint32 field;
	gint32 action;
	gchar  padding[44];
	gint32 string_len;
	gchar  string[];
};

struct _MhodHeaderSmartPlaylistRuleNonString {
	/* Big endian fields */
	gint32  field;
	gint32  action;
	gchar   padding[44];
	gint32  length;
	guint64 from_value;
	gint64  from_date;
	guint64 from_unit;
	guint64 to_value;
	gint64  to_date;
	guint64 to_unit;
	gchar   unknown[20];
};

struct _MhodHeaderSmartPlaylistRule {
	unsigned char header_id[4];
	gint32 header_len;
	gint32 total_len;
	gint32 type; /* 51 */
	gint32 unknown1;
	gint32 unknown2;
	gchar  rules_id[4];
	/* Fields stored in big-endian from there */
	gint32 unknown5;
	gint32 number_of_rules;
	gint32 rules_operator;
	gchar  padding[120];
	union {
		MhodHeaderSmartPlaylistRuleString rule_string;
		MhodHeaderSmartPlaylistRuleNonString rule_non_string;
	} rule;
};



struct _MhfdHeader {
	unsigned char header_id[4];
	gint32 header_len;
	gint32 total_len;
	gint32 unknown1;
	gint32 unknown2; /* always seem to be 1 for older databases,
			    in an ArtworkDB generated by iTunes 4.9 or
			    above, it's 2. Caution: iTunes7 removes
			    the whole database if this field is 1 */
	gint32 num_children;
	gint32 unknown3;
	gint32 next_id;
	gint64 unknown5;
	gint64 unknown6;
	gint8  unknown_flag1;
	gint8  unknown_flag2;
	gint8  unknown_flag3;
	gint8  unknown_flag4;
	gint32 unknown8;
	gint32 unknown9;
	gint32 unknown10;
	gint32 unknown11;
	unsigned char padding[];
};

struct _MhliHeader {
	unsigned char header_id[4];
	gint32 header_len;
	gint32 num_children;
	unsigned char padding[];
};

struct _MhiiHeader {
	unsigned char header_id[4];
	gint32 header_len;
	gint32 total_len;
	gint32 num_children;
	gint32 image_id;
	gint64 song_id;
	gint32 unknown4;
	gint32 rating;
	gint32 unknown6;
	gint32 orig_date;
	gint32 digitized_date;
	gint32 orig_img_size;
	unsigned char padding[];
} __attribute__((__packed__));
/* I we don't put the 'packed' attribute above, some 64 bit systems
 * will pad the 64 bit integer to be aligned to an 8-byte boundary.
 * Hopefully there won't be any systems around that crash when
 * accessing 64 bit integers not aligned to 8-byte boundaries. */


struct _MhniHeader {
	unsigned char header_id[4];
	gint32 header_len;
	gint32 total_len;
	gint32 num_children;
	gint32 format_id;
	gint32 ithmb_offset;
	gint32 image_size;
	gint16 vertical_padding;
	gint16 horizontal_padding;
	gint16 image_height;
	gint16 image_width;
	unsigned char padding[];
};

struct _MhlaHeader {
	unsigned char header_id[4];
	gint32 header_len;
	gint32 num_children;
	unsigned char padding[];
};

struct _MhbaHeader {
	unsigned char header_id[4];
	gint32 header_len;
	gint32 total_len;
	gint32 num_mhods;  /* number of Data Objects in the List,
			      probably always 1 */
	gint32 num_mhias;  /* number of pictures in the album */
	gint32 album_id;   /* starts out at $64 and increments by 1 */
	gint32 unk024;     /* unknown, seems to be always 0 */
	gint16 unk028;     /* unknown, seems to be always 0 */
	guint8 album_type; /* 1 = master photo list ("Photo Library"),
			      2 = normal album, sometimes 4 and 5 */
	guint8 playmusic;  /* play music during slideshow (from iPhoto setting) */
	guint8 repeat;     /* repeat the slideshow (from iPhoto setting) */
	guint8 random;     /* show the slides in random order (from iPhoto
			      setting) */
	guint8 show_titles;/* show slide captions (from iPhoto setting) */
	guint8 transition_direction; /* 0=none, 1=left-to-right,
			      2=right-to-left, 3=top-to-bottom,
			      4=bottom-to-top (from iPhoto setting) */
	gint32 slide_duration; /* in seconds (from iPhoto setting) */
	gint32 transition_duration; /* in milliseconds (from iPhoto setting) */
	gint32 unk044;     /* unknown, seems to always be 0 */
	gint32 unk048;     /* unknown, seems to always be 0 */
	gint64 song_id;    /* dbid2 of track in iTunesDB to play during
			      slideshow (from iPhoto setting) */
	gint32 prev_album_id; /* the id of the previous photoalbum */
	unsigned char padding[];
};

struct _MhlfHeader {
	unsigned char header_id[4];
	gint32 header_len;
	gint32 num_files;
	unsigned char padding[];
};

struct _MhifHeader {
	unsigned char header_id[4];
	gint32 header_len;
	gint32 total_len;
	gint32 unknown1;
	gint32 format_id;
	gint32 image_size;
	unsigned char padding[];
};

struct _MhiaHeader {
	unsigned char header_id[4];
	gint32 header_len;
	gint32 total_len; /* probably the size of the header and all child records;
			   * as there aren't any child records this is equal to header length */
	gint32 unknown1; /* seems to be zero */
	guint32 image_id; /* the id of the mhii record this mhia refers to */
	unsigned char padding[];
};


#endif /* PARSE_DB_H */

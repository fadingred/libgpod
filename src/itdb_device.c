/*
|  Copyright (C) 2002-2007 Jorg Schuler <jcsjcs at users sourceforge net>
|  Part of the gtkpod project.
| 
|  URL: http://www.gtkpod.org/
|  URL: http://gtkpod.sourceforge.net/
|
|  Part of this code is based on ipod-device.c from the libipoddevice
|  project (http://64.14.94.162/index.php/Libipoddevice).
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

#include "itdb_device.h"
#include "itdb_private.h"
#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <glib/gi18n-lib.h>
#include <glib/gstdio.h>

static const Itdb_IpodInfo ipod_info_table [] = {
    /* Handle idiots who hose their iPod file system, or lucky people
       with iPods we don't yet know about*/
    {"Invalid", 0,  ITDB_IPOD_MODEL_INVALID,     ITDB_IPOD_GENERATION_UNKNOWN, 0},
    {"Unknown", 0,  ITDB_IPOD_MODEL_UNKNOWN,     ITDB_IPOD_GENERATION_UNKNOWN, 0},

    /* First Generation */
    /* Mechanical buttons arranged around rotating "scroll wheel".
       8513, 8541 and 8709 are Mac types, 8697 is PC */
    {"8513",  5, ITDB_IPOD_MODEL_REGULAR,     ITDB_IPOD_GENERATION_FIRST,  20},
    {"8541",  5, ITDB_IPOD_MODEL_REGULAR,     ITDB_IPOD_GENERATION_FIRST,  20},
    {"8697",  5, ITDB_IPOD_MODEL_REGULAR,     ITDB_IPOD_GENERATION_FIRST,  20},
    {"8709", 10, ITDB_IPOD_MODEL_REGULAR,     ITDB_IPOD_GENERATION_FIRST,  20},

    /* Second Generation */
    /* Same buttons as First Generation but around touch-sensitive
       "touch wheel". 8737 and 8738 are Mac types, 8740 and 8741 * are
       PC */
    {"8737", 10, ITDB_IPOD_MODEL_REGULAR,     ITDB_IPOD_GENERATION_SECOND, 20},
    {"8740", 10, ITDB_IPOD_MODEL_REGULAR,     ITDB_IPOD_GENERATION_SECOND, 20},
    {"8738", 20, ITDB_IPOD_MODEL_REGULAR,     ITDB_IPOD_GENERATION_SECOND, 50},
    {"8741", 20, ITDB_IPOD_MODEL_REGULAR,     ITDB_IPOD_GENERATION_SECOND, 50},

    /* Third Generation */
    /* Touch sensitive buttons and arranged in a line above "touch
       wheel". Docking connector was introduced here, same models for
       Mac and PC from now on. */
    {"8976", 10, ITDB_IPOD_MODEL_REGULAR,     ITDB_IPOD_GENERATION_THIRD,  20},
    {"8946", 15, ITDB_IPOD_MODEL_REGULAR,     ITDB_IPOD_GENERATION_THIRD,  50},
    {"9460", 15, ITDB_IPOD_MODEL_REGULAR,     ITDB_IPOD_GENERATION_THIRD,  50},
    {"9244", 20, ITDB_IPOD_MODEL_REGULAR,     ITDB_IPOD_GENERATION_THIRD,  50},
    {"8948", 30, ITDB_IPOD_MODEL_REGULAR,     ITDB_IPOD_GENERATION_THIRD,  50},
    {"9245", 40, ITDB_IPOD_MODEL_REGULAR,     ITDB_IPOD_GENERATION_THIRD,  50},

    /* Fourth Generation */
    /* Buttons are now integrated into the "touch wheel". */
    {"9282", 20, ITDB_IPOD_MODEL_REGULAR,     ITDB_IPOD_GENERATION_FOURTH, 50},
    {"9787", 25, ITDB_IPOD_MODEL_REGULAR_U2,  ITDB_IPOD_GENERATION_FOURTH, 50},
    {"9268", 40, ITDB_IPOD_MODEL_REGULAR,     ITDB_IPOD_GENERATION_FOURTH, 50},

    /* First Generation Mini */
    {"9160",  4, ITDB_IPOD_MODEL_MINI,        ITDB_IPOD_GENERATION_MINI_1,  6},
    {"9436",  4, ITDB_IPOD_MODEL_MINI_BLUE,   ITDB_IPOD_GENERATION_MINI_1,  6},
    {"9435",  4, ITDB_IPOD_MODEL_MINI_PINK,   ITDB_IPOD_GENERATION_MINI_1,  6},
    {"9434",  4, ITDB_IPOD_MODEL_MINI_GREEN,  ITDB_IPOD_GENERATION_MINI_1,  6},
    {"9437",  4, ITDB_IPOD_MODEL_MINI_GOLD,   ITDB_IPOD_GENERATION_MINI_1,  6},	

    /* Second Generation Mini */
    {"9800",  4, ITDB_IPOD_MODEL_MINI,        ITDB_IPOD_GENERATION_MINI_2,  6},
    {"9802",  4, ITDB_IPOD_MODEL_MINI_BLUE,   ITDB_IPOD_GENERATION_MINI_2,  6},
    {"9804",  4, ITDB_IPOD_MODEL_MINI_PINK,   ITDB_IPOD_GENERATION_MINI_2,  6},
    {"9806",  4, ITDB_IPOD_MODEL_MINI_GREEN,  ITDB_IPOD_GENERATION_MINI_2,  6},
    {"9801",  6, ITDB_IPOD_MODEL_MINI,        ITDB_IPOD_GENERATION_MINI_2, 20},
    {"9803",  6, ITDB_IPOD_MODEL_MINI_BLUE,   ITDB_IPOD_GENERATION_MINI_2, 20},
    {"9805",  6, ITDB_IPOD_MODEL_MINI_PINK,   ITDB_IPOD_GENERATION_MINI_2, 20},
    {"9807",  6, ITDB_IPOD_MODEL_MINI_GREEN,  ITDB_IPOD_GENERATION_MINI_2, 20},	

    /* Photo / Fourth Generation */
    /* Buttons are integrated into the "touch wheel". */
    {"A079", 20, ITDB_IPOD_MODEL_COLOR,       ITDB_IPOD_GENERATION_PHOTO,  50},
    {"A127", 20, ITDB_IPOD_MODEL_COLOR_U2,    ITDB_IPOD_GENERATION_PHOTO,  50},
    {"9829", 30, ITDB_IPOD_MODEL_COLOR,       ITDB_IPOD_GENERATION_PHOTO,  50},
    {"9585", 40, ITDB_IPOD_MODEL_COLOR,       ITDB_IPOD_GENERATION_PHOTO,  50},
    {"9830", 60, ITDB_IPOD_MODEL_COLOR,       ITDB_IPOD_GENERATION_PHOTO,  50},
    {"9586", 60, ITDB_IPOD_MODEL_COLOR,       ITDB_IPOD_GENERATION_PHOTO,  50},
    {"9830", 60, ITDB_IPOD_MODEL_COLOR,       ITDB_IPOD_GENERATION_PHOTO,  50},

    /* Shuffle / Fourth Generation */
    {"9724", 0.5,ITDB_IPOD_MODEL_SHUFFLE,     ITDB_IPOD_GENERATION_SHUFFLE_1, 3},
    {"9725", 1,  ITDB_IPOD_MODEL_SHUFFLE,     ITDB_IPOD_GENERATION_SHUFFLE_1, 3},
    /* Shuffle / Sixth Generation */
    /* Square, connected to computer via cable */
    {"A546", 1,  ITDB_IPOD_MODEL_SHUFFLE_SILVER, ITDB_IPOD_GENERATION_SHUFFLE_2, 3},
    {"A947", 1,  ITDB_IPOD_MODEL_SHUFFLE_PINK,   ITDB_IPOD_GENERATION_SHUFFLE_2, 3},
    {"A949", 1,  ITDB_IPOD_MODEL_SHUFFLE_BLUE,   ITDB_IPOD_GENERATION_SHUFFLE_2, 3},
    {"A951", 1,  ITDB_IPOD_MODEL_SHUFFLE_GREEN,  ITDB_IPOD_GENERATION_SHUFFLE_2, 3},
    {"A953", 1,  ITDB_IPOD_MODEL_SHUFFLE_ORANGE, ITDB_IPOD_GENERATION_SHUFFLE_2, 3},
    /* Shuffle / Seventh Generation */
    /* Square, connected to computer via cable -- look identicaly to
     * Sixth Generation*/
    {"B225", 1,  ITDB_IPOD_MODEL_SHUFFLE_SILVER, ITDB_IPOD_GENERATION_SHUFFLE_3, 3},
    {"B233", 1,  ITDB_IPOD_MODEL_SHUFFLE_PURPLE, ITDB_IPOD_GENERATION_SHUFFLE_3, 3},
    {"B225", 1,  ITDB_IPOD_MODEL_SHUFFLE_RED,    ITDB_IPOD_GENERATION_SHUFFLE_3, 3},
    {"B227", 1,  ITDB_IPOD_MODEL_SHUFFLE_BLUE,   ITDB_IPOD_GENERATION_SHUFFLE_3, 3},
    {"B229", 1,  ITDB_IPOD_MODEL_SHUFFLE_GREEN,  ITDB_IPOD_GENERATION_SHUFFLE_3, 3},
    {"B518", 2,  ITDB_IPOD_MODEL_SHUFFLE_SILVER, ITDB_IPOD_GENERATION_SHUFFLE_3, 3},
    {"B520", 2,  ITDB_IPOD_MODEL_SHUFFLE_BLUE,   ITDB_IPOD_GENERATION_SHUFFLE_3, 3},
    {"B522", 2,  ITDB_IPOD_MODEL_SHUFFLE_GREEN,  ITDB_IPOD_GENERATION_SHUFFLE_3, 3},
    {"B524", 2,  ITDB_IPOD_MODEL_SHUFFLE_RED,    ITDB_IPOD_GENERATION_SHUFFLE_3, 3},
    {"B526", 2,  ITDB_IPOD_MODEL_SHUFFLE_PURPLE, ITDB_IPOD_GENERATION_SHUFFLE_3, 3},

    /* Nano / Fifth Generation (first nano generation) */
    /* Buttons are integrated into the "touch wheel". */
    {"A350",  1, ITDB_IPOD_MODEL_NANO_WHITE,  ITDB_IPOD_GENERATION_NANO_1,   3},
    {"A352",  1, ITDB_IPOD_MODEL_NANO_BLACK,  ITDB_IPOD_GENERATION_NANO_1,   3},
    {"A004",  2, ITDB_IPOD_MODEL_NANO_WHITE,  ITDB_IPOD_GENERATION_NANO_1,   3},
    {"A099",  2, ITDB_IPOD_MODEL_NANO_BLACK,  ITDB_IPOD_GENERATION_NANO_1,   3},
    {"A005",  4, ITDB_IPOD_MODEL_NANO_WHITE,  ITDB_IPOD_GENERATION_NANO_1,   6},
    {"A107",  4, ITDB_IPOD_MODEL_NANO_BLACK,  ITDB_IPOD_GENERATION_NANO_1,   6},

    /* Video / Fifth Generation */
    /* Buttons are integrated into the "touch wheel". */
    {"A002", 30, ITDB_IPOD_MODEL_VIDEO_WHITE, ITDB_IPOD_GENERATION_VIDEO_1,  50},
    {"A146", 30, ITDB_IPOD_MODEL_VIDEO_BLACK, ITDB_IPOD_GENERATION_VIDEO_1,  50},
    {"A003", 60, ITDB_IPOD_MODEL_VIDEO_WHITE, ITDB_IPOD_GENERATION_VIDEO_1,  50},
    {"A147", 60, ITDB_IPOD_MODEL_VIDEO_BLACK, ITDB_IPOD_GENERATION_VIDEO_1,  50},

    /* Video / Sixth Generation */
    /* Pretty much identical to fifth generation with better display,
     * extended battery operation time and gap-free playback */
    {"A444", 30, ITDB_IPOD_MODEL_VIDEO_WHITE, ITDB_IPOD_GENERATION_VIDEO_2,  50},
    {"A446", 30, ITDB_IPOD_MODEL_VIDEO_BLACK, ITDB_IPOD_GENERATION_VIDEO_2,  50},
    {"A664", 30, ITDB_IPOD_MODEL_VIDEO_U2,    ITDB_IPOD_GENERATION_VIDEO_2,  50},
    {"A448", 80, ITDB_IPOD_MODEL_VIDEO_WHITE, ITDB_IPOD_GENERATION_VIDEO_2,  50},
    {"A450", 80, ITDB_IPOD_MODEL_VIDEO_BLACK, ITDB_IPOD_GENERATION_VIDEO_2,  50},

    /* Nano / Sixth Generation (second nano generation) */
    /* Pretty much identical to fifth generation with better display,
     * extended battery operation time and gap-free playback */
    {"A477",  2, ITDB_IPOD_MODEL_NANO_SILVER, ITDB_IPOD_GENERATION_NANO_2,   3},
    {"A426",  4, ITDB_IPOD_MODEL_NANO_SILVER, ITDB_IPOD_GENERATION_NANO_2,   6},
    {"A428",  4, ITDB_IPOD_MODEL_NANO_BLUE,   ITDB_IPOD_GENERATION_NANO_2,   6},
    {"A487",  4, ITDB_IPOD_MODEL_NANO_GREEN,  ITDB_IPOD_GENERATION_NANO_2,   6},
    {"A489",  4, ITDB_IPOD_MODEL_NANO_PINK,   ITDB_IPOD_GENERATION_NANO_2,   6},
    {"A725",  4, ITDB_IPOD_MODEL_NANO_RED,    ITDB_IPOD_GENERATION_NANO_2,   6},
    {"A726",  8, ITDB_IPOD_MODEL_NANO_RED,    ITDB_IPOD_GENERATION_NANO_2,   6},
    {"A497",  8, ITDB_IPOD_MODEL_NANO_BLACK,  ITDB_IPOD_GENERATION_NANO_2,  14},

    /* HP iPods, need contributions for this table */
    /* Buttons are integrated into the "touch wheel". */
    {"E436", 40, ITDB_IPOD_MODEL_REGULAR,     ITDB_IPOD_GENERATION_FOURTH, 50},
    {"S492", 30, ITDB_IPOD_MODEL_COLOR,       ITDB_IPOD_GENERATION_PHOTO,  50},

    /* iPod Classic G1 */
    /* First generation with "cover flow" */
    {"B029",  80, ITDB_IPOD_MODEL_CLASSIC_SILVER, ITDB_IPOD_GENERATION_CLASSIC_1, 50},
    {"B147",  80, ITDB_IPOD_MODEL_CLASSIC_BLACK,  ITDB_IPOD_GENERATION_CLASSIC_1, 50},
    {"B155", 160, ITDB_IPOD_MODEL_CLASSIC_SILVER, ITDB_IPOD_GENERATION_CLASSIC_1, 50},
    {"B150", 160, ITDB_IPOD_MODEL_CLASSIC_BLACK,  ITDB_IPOD_GENERATION_CLASSIC_1, 50},

    /* iPod nano video G1 (Third Nano Generation) */
    /* First generation of video support for nano */
    {"A978",   4, ITDB_IPOD_MODEL_NANO_SILVER,    ITDB_IPOD_GENERATION_NANO_3,  6},
    {"A980",   8, ITDB_IPOD_MODEL_NANO_SILVER,    ITDB_IPOD_GENERATION_NANO_3, 14},
    {"B261",   8, ITDB_IPOD_MODEL_NANO_BLACK,     ITDB_IPOD_GENERATION_NANO_3, 14},
    {"B249",   8, ITDB_IPOD_MODEL_NANO_BLUE,      ITDB_IPOD_GENERATION_NANO_3, 14},
    {"B253",   8, ITDB_IPOD_MODEL_NANO_GREEN,     ITDB_IPOD_GENERATION_NANO_3, 14},
    {"B257",   8, ITDB_IPOD_MODEL_NANO_RED,       ITDB_IPOD_GENERATION_NANO_3, 14},

    /* iPod touch G1 */
    /* With touch screen */
    {"A623",   8, ITDB_IPOD_MODEL_TOUCH_BLACK,    ITDB_IPOD_GENERATION_TOUCH_1, 14},
    {"A627",  16, ITDB_IPOD_MODEL_TOUCH_BLACK,    ITDB_IPOD_GENERATION_TOUCH_1, 28},

    /* iPhone G1 */
    /* We used to not have a model number for the iPhone so we had that
     * dummy "iPhone1" model number, we now keep it here for backward
     * compatibility reasons
     */
    {"A501",  -1, ITDB_IPOD_MODEL_IPHONE_1,       ITDB_IPOD_GENERATION_IPHONE_1, 14},
    {"iPhone1", -1, ITDB_IPOD_MODEL_IPHONE_1, ITDB_IPOD_GENERATION_IPHONE_1, 14},

    /* No known model number -- create a Device/SysInfo file with
     * one entry, e.g.:
       ModelNumStr: Mmobile1
    */
    {"mobile1", -1, ITDB_IPOD_MODEL_MOBILE_1, ITDB_IPOD_GENERATION_MOBILE,  6},

    {NULL, 0, 0, 0, 0}
};


/* One entry for each of Itdb_IpodModel (itdb.h) */
static const gchar *ipod_model_name_table [] = {
	N_("Invalid"),
	N_("Unknown"),
	N_("Color"),
	N_("Color U2"),
	N_("Grayscale"),
	N_("Grayscale U2"),
	N_("Mini (Silver)"),
	N_("Mini (Blue)"),
	N_("Mini (Pink)"),
	N_("Mini (Green)"),
	N_("Mini (Gold)"),
	N_("Shuffle"),
	N_("Nano (White)"),
	N_("Nano (Black)"),
	N_("Video (White)"),
	N_("Video (Black)"),
	N_("Mobile (1)"),
	N_("Video U2"),
	N_("Nano (Silver)"),
	N_("Nano (Blue)"),
	N_("Nano (Green)"),
	N_("Nano (Pink)"),
	N_("Nano (Red)"),
	N_("iPhone (1)"),
	N_("Shuffle (Silver)"),
	N_("Shuffle (Pink)"),
	N_("Shuffle (Blue)"),
	N_("Shuffle (Green)"),
	N_("Shuffle (Orange)"),
	N_("Shuffle (Purple)"),
	N_("Classic (Silver)"),
	N_("Classic (Black)"),
	N_("Touch (Black)"),
	NULL
};

/* One entry for each of Itdb_IpodGeneration (itdb.h) */
static const gchar *ipod_generation_name_table [] = {
	N_("Unknown"),
	N_("Regular (1st Gen.)"),
	N_("Regular (2nd Gen.)"),
	N_("Regular (3rd Gen.)"),
	N_("Regular (4th Gen.)"),
	N_("Photo"),
	N_("Mobile Phones"),
	N_("Mini (1st Gen.)"),
	N_("Mini (2nd Gen.)"),
	N_("Shuffle (1st Gen.)"),
	N_("Shuffle (2nd Gen.)"),
	N_("Shuffle (3rd Gen.)"),
	N_("Nano (1st Gen.)"),
	N_("Nano (2nd Gen.)"),
	N_("Nano Video (3rd Gen.)"),
	N_("Video (1st Gen.)"),
	N_("Video (2nd Gen.)"),
	N_("Classic"),
	N_("Touch"),
        N_("iPhone"),
	N_("Unused"),
	N_("Unused"),
	NULL
};


static const Itdb_ArtworkFormat ipod_photo_cover_art_info[] = {
    {1017,  56,  56, THUMB_FORMAT_RGB565_LE},
    {1016, 140, 140, THUMB_FORMAT_RGB565_LE},
    { -1,   -1,  -1, -1}
};

static const Itdb_ArtworkFormat ipod_photo_photo_info[] = {
    {1009,  42,  30, THUMB_FORMAT_RGB565_LE},
    {1015, 130,  88, THUMB_FORMAT_RGB565_LE},
    {1013, 220, 176, THUMB_FORMAT_RGB565_BE_90},
    {1019, 720, 480, THUMB_FORMAT_UYVY_BE},
    {  -1,  -1,  -1, -1}
};

static const Itdb_ArtworkFormat ipod_nano_cover_art_info[] = {
    {1031,  42,  42, THUMB_FORMAT_RGB565_LE},
    {1027, 100, 100, THUMB_FORMAT_RGB565_LE},
    {  -1,  -1,  -1, -1}
};

static const Itdb_ArtworkFormat ipod_nano_photo_info[] = {
    {1032,  42,  37, THUMB_FORMAT_RGB565_LE},
    {1023, 176, 132, THUMB_FORMAT_RGB565_BE},
    {  -1,  -1,  -1, -1}
};

static const Itdb_ArtworkFormat ipod_video_cover_art_info[] = {
    {1028, 100, 100, THUMB_FORMAT_RGB565_LE},
    {1029, 200, 200, THUMB_FORMAT_RGB565_LE},
    {  -1,  -1,  -1, -1}
};

static const Itdb_ArtworkFormat ipod_video_photo_info[] = {
    {1036,  50,  41, THUMB_FORMAT_RGB565_LE},
    {1015, 130,  88, THUMB_FORMAT_RGB565_LE},
    {1024, 320, 240, THUMB_FORMAT_RGB565_LE},
    {1019, 720, 480, THUMB_FORMAT_UYVY_BE},
    {  -1,  -1,  -1, -1}
};

static const Itdb_ArtworkFormat ipod_mobile_1_cover_art_info[] = {
    {2002,  50,  50, THUMB_FORMAT_RGB565_BE},
    {2003, 150, 150, THUMB_FORMAT_RGB565_BE},
    {  -1,  -1,  -1, -1}
};

static const Itdb_ArtworkFormat ipod_touch_1_cover_art_info[] = {
    {3001, 256, 256, THUMB_FORMAT_REC_RGB555_LE},
    {3002, 128, 128, THUMB_FORMAT_REC_RGB555_LE},
    {3003,  64,  64, THUMB_FORMAT_REC_RGB555_LE},
    {3005, 320, 320, THUMB_FORMAT_RGB555_LE},
    {3006,  56,  56, THUMB_FORMAT_RGB555_LE,  8192}, /*pad data to  8192 bytes */
    {3007,  88,  88, THUMB_FORMAT_RGB555_LE, 16364}, /*pad data to 16384 bytes */
    {  -1,  -1,  -1, -1}
};

static const Itdb_ArtworkFormat ipod_touch_1_photo_info[] = {
    /* In the album list, if a photo is being used to represent a whole album,
       PHOTO_SMALL is used.  We specify TRUE for the crop option so we fill
       the square completely. */
    {3004,  56,  55, THUMB_FORMAT_RGB555_LE, 8192, TRUE},
    /* In thumbnail view, PHOTO_LARGE is used.  It's actually 79x79, with a 4px
       white border on the right and bottom.  We specify TRUE for the crop option
       so we fill the square completely. */
    {3011,  80,  79, THUMB_FORMAT_RGB555_LE, 0, TRUE},
    {3009, 160, 120, THUMB_FORMAT_RGB555_LE},
    /* When viewing an individual photo, PHOTO_TV_SCREEN is used.  Note that it
       is acceptable to write a thumbnail less than the specified width or
       height, the iPhone / iTouch will scale it to fit the screen.  This is
       important for images that are taller than they wide. */
    {3008, 640, 480, THUMB_FORMAT_RGB555_LE},
    {  -1,  -1,  -1, -1}
};

static const Itdb_ArtworkFormat ipod_classic_1_cover_art_info[] = {
    /* officially 55x55 -- verify! */
    {1061,  56,  56, THUMB_FORMAT_RGB565_LE},
    {1055, 128, 128, THUMB_FORMAT_RGB565_LE},
    {1060, 320, 320, THUMB_FORMAT_RGB565_LE},
    {  -1,  -1,  -1, -1}
};

static const Itdb_ArtworkFormat ipod_classic_1_photo_info[] = {
    {1067, 720, 480, THUMB_FORMAT_I420_LE},
    {1024, 320, 240, THUMB_FORMAT_RGB565_LE},
    {1066,  64,  64, THUMB_FORMAT_RGB565_LE},
    {  -1,  -1,  -1, -1}
};

static const Itdb_ArtworkFormat ipod_classic_1_chapter_image_info[] = {
    /*  These are the same as for the iPod video... -- labeled by the iPod as
        "chapter images" */
    {1028, 100, 100, THUMB_FORMAT_RGB565_LE},
    {1029, 200, 200, THUMB_FORMAT_RGB565_LE},
    {  -1,  -1,  -1, -1}
};

enum ArtworkType {
    ARTWORK_TYPE_COVER_ART,
    ARTWORK_TYPE_PHOTO,
    ARTWORK_TYPE_CHAPTER_IMAGE
};

struct _ArtworkCapabilities {
    Itdb_IpodGeneration generation;
    const Itdb_ArtworkFormat *cover_art_formats;
    const Itdb_ArtworkFormat *photo_formats;
    const Itdb_ArtworkFormat *chapter_image_formats;
};
typedef struct _ArtworkCapabilities ArtworkCapabilities;

static const ArtworkCapabilities ipod_artwork_capabilities[] = {
    { ITDB_IPOD_GENERATION_PHOTO, ipod_photo_cover_art_info, ipod_photo_photo_info, NULL },
    { ITDB_IPOD_GENERATION_VIDEO_1, ipod_video_cover_art_info, ipod_video_photo_info, NULL },
    { ITDB_IPOD_GENERATION_VIDEO_2, ipod_video_cover_art_info, ipod_video_photo_info, NULL },
    { ITDB_IPOD_GENERATION_NANO_1, ipod_nano_cover_art_info, ipod_nano_photo_info, NULL },
    { ITDB_IPOD_GENERATION_NANO_2, ipod_nano_cover_art_info, ipod_nano_photo_info, NULL },
    { ITDB_IPOD_GENERATION_NANO_3, ipod_classic_1_cover_art_info, ipod_classic_1_photo_info, ipod_classic_1_chapter_image_info },
    { ITDB_IPOD_GENERATION_CLASSIC_1, ipod_classic_1_cover_art_info, ipod_classic_1_photo_info, ipod_classic_1_chapter_image_info },
    { ITDB_IPOD_GENERATION_TOUCH_1, ipod_touch_1_cover_art_info, ipod_touch_1_photo_info, NULL },
    { ITDB_IPOD_GENERATION_IPHONE_1, ipod_touch_1_cover_art_info, ipod_touch_1_photo_info, NULL },
    { ITDB_IPOD_GENERATION_MOBILE, ipod_mobile_1_cover_art_info, NULL, NULL },
    { ITDB_IPOD_GENERATION_UNKNOWN, NULL, NULL, NULL }
};

static void itdb_device_set_timezone_info (Itdb_Device *device);

/* Reset or create the SysInfo hash table */
static void itdb_device_reset_sysinfo (Itdb_Device *device)
{
    if (device->sysinfo)
	g_hash_table_destroy (device->sysinfo);
    device->sysinfo = g_hash_table_new_full (g_str_hash, g_str_equal,
					     g_free, g_free);
    device->sysinfo_changed = FALSE;
}


/**
 * itdb_device_new:
 * 
 * Creates a new #Itdb_Device structure
 *
 * Return value: a newly allocated #Itdb_Device which must be freed with
 * itdb_device_free() when no longer needed
 **/
Itdb_Device *itdb_device_new ()
{
    Itdb_Device *dev;

    dev = g_new0 (Itdb_Device, 1);
    itdb_device_reset_sysinfo (dev);
    return dev;
}

/**
 * itdb_device_free:
 * @device: an #Itdb_Device
 *
 * Frees memory used by @device
 **/
void itdb_device_free (Itdb_Device *device)
{
    if (device)
    {
	g_free (device->mountpoint);
	if (device->sysinfo)
	    g_hash_table_destroy (device->sysinfo);
        if (device->sysinfo_extended)
            itdb_sysinfo_properties_free (device->sysinfo_extended);
	g_free (device);
    }
}


/**
 * itdb_device_set_mountpoint:
 * @device: an #Itdb_Device
 * @mp: the new mount point
 *
 * Sets the mountpoint of @device to @mp and update the cached device 
 * information (in particular, re-read the SysInfo file)
 **/
void itdb_device_set_mountpoint (Itdb_Device *device, const gchar *mp)
{
    g_return_if_fail (device);

    g_free (device->mountpoint);
    device->mountpoint = g_strdup (mp);
    if (mp) {
	itdb_device_read_sysinfo (device);
        itdb_device_set_timezone_info (device);
    }
}


G_GNUC_INTERNAL time_t device_time_mac_to_time_t (Itdb_Device *device, guint64 mactime)
{
    g_return_val_if_fail (device, 0);
    if (mactime != 0)  return (time_t)(mactime - 2082844800 - device->timezone_shift);
    else               return (time_t)mactime;
}

G_GNUC_INTERNAL guint64 device_time_time_t_to_mac (Itdb_Device *device, time_t timet)
{
    g_return_val_if_fail (device, 0);
    if (timet != 0)
	return ((guint64)timet) + 2082844800 + device->timezone_shift;
    else return 0;
}

static void itdb_device_read_sysinfo_extended (Itdb_Device *device)
{
    const gchar *p_sysinfo_ex[] = {"SysInfoExtended", NULL};
    gchar *dev_path, *sysinfo_ex_path;

    if (device->sysinfo_extended != NULL) {
        itdb_sysinfo_properties_free (device->sysinfo_extended);
        device->sysinfo_extended = NULL;
    }

    dev_path = itdb_get_device_dir (device->mountpoint);
    if (!dev_path) return;

    sysinfo_ex_path = itdb_resolve_path (dev_path, p_sysinfo_ex);
    g_free (dev_path);
    if (!sysinfo_ex_path) return;
    device->sysinfo_extended = itdb_sysinfo_extended_parse (sysinfo_ex_path);
    g_free (sysinfo_ex_path);

    if ((device->sysinfo != NULL) && (device->sysinfo_extended != NULL)) {
        const char *fwid;
        fwid = itdb_sysinfo_properties_get_firewire_id (device->sysinfo_extended);
        if (fwid != NULL) {
            g_hash_table_insert (device->sysinfo, g_strdup ("FirewireGuid"),
                                 g_strdup (fwid));
        }
    }
}

/** 
 * itdb_device_read_sysinfo:
 * @device: an #Itdb_Device
 *
 * Reads the SysInfo file and stores information in device->sysinfo for
 * later use.
 *
 * Return value: TRUE if file could be read, FALSE otherwise 
 **/
gboolean itdb_device_read_sysinfo (Itdb_Device *device)
{
    const gchar *p_sysinfo[] = {"SysInfo", NULL};
    gchar *dev_path, *sysinfo_path;
    FILE *fd;
    gboolean result = FALSE;
    gchar buf[1024];

    g_return_val_if_fail (device, FALSE);
    g_return_val_if_fail (device->mountpoint, FALSE);

    itdb_device_reset_sysinfo (device);

    g_return_val_if_fail (device->sysinfo, FALSE);

    dev_path = itdb_get_device_dir (device->mountpoint);

    if (!dev_path) return FALSE;

    sysinfo_path = itdb_resolve_path (dev_path, p_sysinfo);

    if (sysinfo_path)
    {
	fd = fopen (sysinfo_path, "r");
	if (fd)
	{
	    result = TRUE;
	    while(fgets(buf, sizeof(buf), fd))
	    {
		gchar *ptr;
		gint len = strlen (buf);
		/* suppress newline at end of line */
		if ((len>0) && (buf[len-1]==0x0a))
		{
		    buf[len-1] = 0;
		    --len;
		}
		ptr = strchr (buf, ':');
		if (ptr && (ptr!=buf))
		{
		    *ptr = 0;
		    ++ptr;
		    itdb_device_set_sysinfo (device,
					     buf, g_strstrip(ptr));
		}
	    }
	    fclose (fd);
	}
	g_free (sysinfo_path);
    }
    g_free (dev_path);

    itdb_device_read_sysinfo_extended (device);

    /* indicate that sysinfo is identical to what is on the iPod */
    device->sysinfo_changed = FALSE;
    return result;
}

/* used by itdb_device_write_sysinfo() */
static void write_sysinfo_entry (const gchar *key,
				 const gchar *value,
				 FILE *file)
{
    fprintf (file, "%s: %s\n", key, value);
}



/** 
 * itdb_device_write_sysinfo:
 * @device: an #Itdb_Device
 * @error: return location for a #GError or NULL
 *
 * Fills the SysInfo file with information in device->sysinfo. Note:
 * no directories are created if not already existent.
 *
 * Return value: TRUE if file could be read, FALSE otherwise 
 **/
gboolean itdb_device_write_sysinfo (Itdb_Device *device, GError **error)
{
    gchar *devicedir;
    gboolean success = FALSE;

    g_return_val_if_fail (device, FALSE);
    g_return_val_if_fail (device->mountpoint, FALSE);

    devicedir = itdb_get_device_dir (device->mountpoint);
    if (devicedir)
    {
	gchar *sysfile = g_build_filename (devicedir, "SysInfo", NULL);
	FILE *sysinfo = fopen (sysfile, "w");
	if (sysinfo)
	{
	    if (device->sysinfo)
	    {
		g_hash_table_foreach (device->sysinfo,
				      (GHFunc)write_sysinfo_entry,
				      sysinfo);
	    }
	    fclose (sysinfo);
	    success = TRUE;
	}
	else
	{
	    g_set_error (error, 0, -1,
			 _("Could not open '%s' for writing."),
			 sysfile);
	}
	g_free (sysfile);
	g_free (devicedir);
    }
    else
    {
	g_set_error (error, 0, -1,
		     _("Device directory does not exist."));
    }

    if (success)
	device->sysinfo_changed = FALSE;

    return success;
}


/**
 * itdb_device_get_sysinfo:
 * @device: an #Itdb_Device
 * @field: field to retrive information from
 *
 * Retrieve specified field from the SysInfo file.
 *
 * Return value: the information associated with @field, or NULL if @field
 * couldn't be found. g_free() after use
 **/
gchar *itdb_device_get_sysinfo (const Itdb_Device *device, const gchar *field)
{
    g_return_val_if_fail (device, NULL);
    g_return_val_if_fail (device->sysinfo, NULL);
    g_return_val_if_fail (field, NULL);

    return g_strdup (g_hash_table_lookup (device->sysinfo, field));
}

/**
 * itdb_device_set_sysinfo:
 * @device: an #Itdb_Device
 * @field: field to set
 * @value: value to set (or NULL to remove the field).
 *
 * Set specified field. It can later be written to the iPod using
 * itdb_device_write_sysinfo()
 *
 **/
void itdb_device_set_sysinfo (Itdb_Device *device,
			      const gchar *field, const gchar *value)
{
    g_return_if_fail (device);
    g_return_if_fail (device->sysinfo);
    g_return_if_fail (field);

    if (field)
    {
	g_hash_table_insert (device->sysinfo,
			     g_strdup (field), g_strdup (value));
    }
    else
    {
	g_hash_table_remove (device->sysinfo, field);
    }
    device->sysinfo_changed = TRUE;
}


/**
 * itdb_device_get_ipod_info:
 * @device: an #Itdb_Device
 *
 * Retrieve the #Itdb_IpodInfo entry for this iPod
 *
 * Return value: the #Itdb_IpodInfo entry for this iPod
 **/
const Itdb_IpodInfo *
itdb_device_get_ipod_info (const Itdb_Device *device)
{
    gint i;
    gchar *model_num, *p;

    model_num = itdb_device_get_sysinfo (device, "ModelNumStr");

    if (!model_num)
	return &ipod_info_table[0];

    p = model_num;

    if(isalpha(model_num[0])) 
	p++;
	
    for(i=2; ipod_info_table[i].model_number != NULL; i++)
    {
	if(g_strncasecmp(p, ipod_info_table[i].model_number, 
			 strlen (ipod_info_table[i].model_number)) == 0)
	{
	    g_free(model_num);
	    return &ipod_info_table[i];
	}
    }	
    g_free(model_num);
    return &ipod_info_table[1];
}



/* Return supported artwork formats supported by this iPod */
static const Itdb_ArtworkFormat *
itdb_device_get_artwork_formats (const Itdb_Device *device, 
                                 enum ArtworkType type)
{
    const Itdb_IpodInfo *info;
    const ArtworkCapabilities *caps;

    g_return_val_if_fail (device, NULL);

    info = itdb_device_get_ipod_info (device);

    g_return_val_if_fail (info, NULL);
    caps = ipod_artwork_capabilities;
    while (caps->generation != ITDB_IPOD_GENERATION_UNKNOWN) {
        if (caps->generation == info->ipod_generation) {
            break;
        }
        caps++;
    }
    switch (type) {
        case ARTWORK_TYPE_COVER_ART:
            return caps->cover_art_formats;
        case ARTWORK_TYPE_PHOTO:
            return caps->photo_formats;
        case ARTWORK_TYPE_CHAPTER_IMAGE:
            return caps->chapter_image_formats;
    }

    g_return_val_if_reached (NULL);
}

static GList *
itdb_device_get_photo_formats_fallback (const Itdb_Device *device)
{
    const Itdb_ArtworkFormat *formats;
    const Itdb_ArtworkFormat *it;
    GList *photo_formats = NULL;

    formats = itdb_device_get_artwork_formats (device, ARTWORK_TYPE_PHOTO);
    if (formats == NULL) {
        return NULL;
    }

    for (it = formats; it->format_id != -1; it++) {
        photo_formats = g_list_prepend (photo_formats, (gpointer)it);
    }

    return photo_formats;
}

GList *itdb_device_get_photo_formats (const Itdb_Device *device)
{
    g_return_val_if_fail (device != NULL, NULL);

    if (device->sysinfo_extended == NULL) {
        return itdb_device_get_photo_formats_fallback (device);
    } else {
        return g_list_copy ((GList *)itdb_sysinfo_properties_get_photo_formats (device->sysinfo_extended));
    }
    g_return_val_if_reached (NULL);
}

static GList *
itdb_device_get_cover_art_formats_fallback (const Itdb_Device *device)
{
    const Itdb_ArtworkFormat *formats;
    const Itdb_ArtworkFormat *it;
    GList *cover_art_formats = NULL;

    formats = itdb_device_get_artwork_formats (device, ARTWORK_TYPE_COVER_ART);
    if (formats == NULL) {
        return NULL;
    }

    for (it = formats; it->format_id != -1; it++) {
        cover_art_formats = g_list_prepend (cover_art_formats, (gpointer)it);
    }

    return cover_art_formats;
}

GList *itdb_device_get_cover_art_formats (const Itdb_Device *device)
{
    g_return_val_if_fail (device != NULL, NULL);

    if (device->sysinfo_extended == NULL) {
        return itdb_device_get_cover_art_formats_fallback (device);
    } else {
        return g_list_copy ((GList *)itdb_sysinfo_properties_get_cover_art_formats (device->sysinfo_extended));
    }
    g_return_val_if_reached (NULL);
}

G_GNUC_INTERNAL gboolean 
itdb_device_supports_sparse_artwork (const Itdb_Device *device)
{
    gboolean supports_sparse_artwork = FALSE;

    g_return_val_if_fail (device != NULL, FALSE);

    if (device->sysinfo_extended != NULL) {
        supports_sparse_artwork = itdb_sysinfo_properties_supports_sparse_artwork (device->sysinfo_extended);
    }

    if (supports_sparse_artwork == FALSE) {
        const Itdb_IpodInfo *info;
        info = itdb_device_get_ipod_info (device);
        switch (info->ipod_generation) {
            case ITDB_IPOD_GENERATION_UNKNOWN:
            case ITDB_IPOD_GENERATION_FIRST:
            case ITDB_IPOD_GENERATION_SECOND:
            case ITDB_IPOD_GENERATION_THIRD:
            case ITDB_IPOD_GENERATION_FOURTH:
            case ITDB_IPOD_GENERATION_PHOTO:
            case ITDB_IPOD_GENERATION_MOBILE:
            case ITDB_IPOD_GENERATION_MINI_1:
            case ITDB_IPOD_GENERATION_MINI_2:
            case ITDB_IPOD_GENERATION_SHUFFLE_1:
            case ITDB_IPOD_GENERATION_SHUFFLE_2:
            case ITDB_IPOD_GENERATION_SHUFFLE_3:
                supports_sparse_artwork = FALSE;
                break;
            case ITDB_IPOD_GENERATION_NANO_1:
            case ITDB_IPOD_GENERATION_NANO_2:
            case ITDB_IPOD_GENERATION_NANO_3:
            case ITDB_IPOD_GENERATION_VIDEO_1:
            case ITDB_IPOD_GENERATION_VIDEO_2:
            case ITDB_IPOD_GENERATION_CLASSIC_1:
            case ITDB_IPOD_GENERATION_TOUCH_1:
            case ITDB_IPOD_GENERATION_IPHONE_1:
                supports_sparse_artwork = TRUE;
                break;
        }
    }
    return supports_sparse_artwork;
}

/* Determine the number of F.. directories in iPod_Control/Music.*/
G_GNUC_INTERNAL gint
itdb_musicdirs_number_by_mountpoint (const gchar *mountpoint)
{
    gint dir_num;
    gchar *music_dir = itdb_get_music_dir (mountpoint);

    if (!music_dir) return 0;

    /* count number of dirs */
    for (dir_num=0; ;++dir_num)
    {
	gchar *dir_filename;
	gchar dir_num_str[6];

	g_snprintf (dir_num_str, 6, "F%02d", dir_num);
  
	dir_filename = itdb_get_path (music_dir, dir_num_str);

	g_free (dir_filename);
	if (!dir_filename)  break;
    }

    g_free (music_dir);

    return dir_num;
}


/* Determine the number of F.. directories in iPod_Control/Music.

   If device->musicdirs is already set, simply return the previously
   determined number. Otherwise count the directories first and set
   itdb->musicdirs. */
G_GNUC_INTERNAL gint
itdb_device_musicdirs_number (Itdb_Device *device)
{
    g_return_val_if_fail (device, 0);

    if (device->musicdirs <= 0)
    {
	device->musicdirs = itdb_musicdirs_number_by_mountpoint (device->mountpoint);
    }
    return device->musicdirs;
}

/**
 * endianess_check_path:
 *
 * Check if endianess can be determined by looking at header of @path.
 *
 * @path:   the file to look at.
 * @hdr: the header string (4 bytes) in case of LITTLE_ENDIAN
 *
 * Return value:
 * G_LITTLE_ENDIAN, G_BIG_ENDIAN or 0 if endianess could not be
 * determined.
 */
static guint endianess_check_path (const gchar *path, const gchar *hdr)
{
    guint byte_order = 0;

    if (path)
    {
	int fd = open (path, O_RDONLY);
	if (fd != -1)
	{
	    gchar buf[4];
	    if (read (fd, buf, 4) == 4)
	    {
		if (strncmp (buf, hdr, 4) == 0)
		{
		    byte_order = G_LITTLE_ENDIAN;
		}
		else
		{
		    if ((buf[0] == hdr[3]) &&
			(buf[1] == hdr[2]) &&
			(buf[2] == hdr[1]) &&
			(buf[3] == hdr[0]))
		    {
			byte_order = G_BIG_ENDIAN;
		    }
		}
	    }
	    close (fd);
	}
    }
    return byte_order;
}

/* Attempt to guess the endianess used by this iPod.
 *
 * It will overwrite the previous setting.
 *
 */

G_GNUC_INTERNAL void
itdb_device_autodetect_endianess (Itdb_Device *device)
{
    guint byte_order = 0;

    g_return_if_fail (device);

    if (device->mountpoint)
    {
	gchar *path;
	if (byte_order == 0)
	{
	    /* First try reading the iTunesDB */
	    path = itdb_get_itunesdb_path (device->mountpoint);
	    byte_order = endianess_check_path (path, "mhbd");
	    g_free (path);
	}
	if (byte_order == 0)
	{
	    /* Try reading the ArtworkDB */
	    path = itdb_get_artworkdb_path (device->mountpoint);
	    byte_order = endianess_check_path (path, "mhfd");
	    g_free (path);
	}
	if (byte_order == 0)
	{
	    /* Try reading the Photos Database */
	    path = itdb_get_photodb_path (device->mountpoint);
	    byte_order = endianess_check_path (path, "mhfd");
	    g_free (path);
	}
	if (byte_order == 0)
	{
	    gchar *control_dir = itdb_get_control_dir (device->mountpoint);
	    if (control_dir)
	    {
		gchar *cd_l = g_ascii_strdown (control_dir, -1);
		/* check if cd_l ends on "itunes/itunes_control" */
		if (strstr (cd_l, "itunes/itunes_control") ==
		    (cd_l + strlen (cd_l) - strlen ("itunes/itunes_control")))
		{
		    byte_order = G_BIG_ENDIAN;
		}
		g_free (cd_l);
		g_free (control_dir);
	    }
	}
    }

    /* default: non-reversed */
    if (byte_order == 0)
	byte_order = G_LITTLE_ENDIAN;

    device->byte_order = byte_order;
}

/*------------------------------------------------------------------*\
 *                                                                  *
 * Functions for applications to obtain general infos about various *
 * iPods.                                                           *
 *                                                                  *
\*------------------------------------------------------------------*/

/**
 * itdb_info_get_ipod_info_table:
 *
 * Return a pointer to the start of valid iPod model descriptions,
 * which is an array of #Itdb_IpodInfo entries.
 *
 * Return value: a pointer to the array of #Itdb_IpodInfo entries.
 **/
const Itdb_IpodInfo *itdb_info_get_ipod_info_table (void)
{
    return &ipod_info_table[2];
}


/**
 * itdb_info_get_ipod_model_name_string:
 * @model: an #Itdb_IpodModel
 *
 * Return the iPod's generic model name, like "Color", "Nano"...
 *
 * Return value: a pointer to the model name. This is a static string
 * and must not be g_free()d.
 **/
const gchar *itdb_info_get_ipod_model_name_string (Itdb_IpodModel model)
{
    gint i=0;

    /* Instead of returning ipod_model_name_table[model] directly,
       verify if @model is valid */
    while (ipod_model_name_table[i])
    {
	if (i==model)  return gettext (ipod_model_name_table[i]);
	++i;
    }
    return NULL;
}


/**
 * itdb_info_get_ipod_generation_string:
 * @generation: an #Itdb_IpodGeneration
 *
 * Return the iPod's generic generation name, like "First Generation",
 * "Mobile Phone"...
 *
 * Return value: a pointer to the generation name. This is a static
 * string and must not be g_free()d.
 **/
const gchar *itdb_info_get_ipod_generation_string (Itdb_IpodGeneration generation)
{
    gint i=0;

    /* Instead of returning ipod_generation_name_table[generation]
       directly, verify if @generation is valid */
    while (ipod_generation_name_table[i])
    {
	if (i==generation)
	    return gettext (ipod_generation_name_table[i]);
	++i;
    }
    return NULL;
}


/**
 * itdb_device_supports_artwork:
 * @device: an #Itdb_Device
 *
 * Indicates whether @device can display artwork or not. When dealing
 * with a non-art capable iPod, no artwork data will be written to the
 * iPod so you can spare calls to the artwork handling methods.
 *
 * Return value: true if @device can display artwork.
 */
gboolean itdb_device_supports_artwork (const Itdb_Device *device)
{
    if (device == NULL) {
        return FALSE;
    }

    return (itdb_device_get_artwork_formats (device, ARTWORK_TYPE_COVER_ART) != NULL);
}


/**
 * itdb_device_supports_video:
 * @device: an #Itdb_Device
 *
 * Indicates whether @device can play videos or not.
 *
 * Return value: true if @device can play videos.
 */

gboolean itdb_device_supports_video (const Itdb_Device *device)
{
    const Itdb_IpodInfo *info;
    if (device == NULL) {
        return FALSE;
    }

    info = itdb_device_get_ipod_info (device);
    switch (info->ipod_generation) {
        case ITDB_IPOD_GENERATION_UNKNOWN:
        case ITDB_IPOD_GENERATION_FIRST:
        case ITDB_IPOD_GENERATION_SECOND:
        case ITDB_IPOD_GENERATION_THIRD:
        case ITDB_IPOD_GENERATION_FOURTH:
        case ITDB_IPOD_GENERATION_PHOTO:
        case ITDB_IPOD_GENERATION_MOBILE:
        case ITDB_IPOD_GENERATION_MINI_1:
        case ITDB_IPOD_GENERATION_MINI_2:
        case ITDB_IPOD_GENERATION_SHUFFLE_1:
        case ITDB_IPOD_GENERATION_SHUFFLE_2:
        case ITDB_IPOD_GENERATION_SHUFFLE_3:
        case ITDB_IPOD_GENERATION_NANO_1:
        case ITDB_IPOD_GENERATION_NANO_2:
            return FALSE;
        case ITDB_IPOD_GENERATION_NANO_3:
        case ITDB_IPOD_GENERATION_VIDEO_1:
        case ITDB_IPOD_GENERATION_VIDEO_2:
        case ITDB_IPOD_GENERATION_CLASSIC_1:
        case ITDB_IPOD_GENERATION_TOUCH_1:
        case ITDB_IPOD_GENERATION_IPHONE_1:
            return TRUE;
    }
    g_return_val_if_reached (FALSE);
}


/**
 * itdb_device_supports_photo:
 * @device: an #Itdb_Device
 *
 * Indicates whether @device can display photos or not.
 *
 * Return value: true if @device can display photos.
 */

gboolean itdb_device_supports_photo (const Itdb_Device *device)
{
    if (device == NULL) {
        return FALSE;
    }

    return (itdb_device_get_artwork_formats (device, ARTWORK_TYPE_PHOTO) != NULL);
}

static char *
get_preferences_path (const Itdb_Device *device)
{

    const gchar *p_preferences[] = {"Preferences", NULL};
    char *dev_path;
    char *prefs_filename;

    if (device->mountpoint == NULL) {
        return NULL;
    }

    dev_path = itdb_get_device_dir (device->mountpoint);

    if (dev_path == NULL) {
        return NULL;
    }

    prefs_filename = itdb_resolve_path (dev_path, p_preferences);
    g_free (dev_path);

    return prefs_filename;
}

static gboolean itdb_device_read_raw_timezone (const char *prefs_path,
                                               glong offset,
                                               gint16 *timezone)
{
    FILE *f;
    int result;

    if (timezone == NULL) {
        return FALSE;
    }

    f = fopen (prefs_path, "r");
    if (f == NULL) {
        return FALSE;
    }

    result = fseek (f, offset, SEEK_SET);
    if (result != 0) {
        fclose (f);
        return FALSE;
    }

    result = fread (timezone, sizeof (*timezone), 1, f);
    if (result != 1) {
        fclose (f);
        return FALSE;
    }

    fclose (f);

    *timezone = GINT16_FROM_LE (*timezone);

    return TRUE;
}

static gboolean raw_timezone_to_utc_shift_4g (gint16 raw_timezone,
                                              gint *utc_shift)
{
    const int GMT_OFFSET = 0x19;

    if (utc_shift == NULL) {
        return FALSE;
    }

    if ((raw_timezone < 0) || (raw_timezone > (2*12) << 1)) {
        /* invalid timezone */
        return FALSE;
    }

    raw_timezone -= GMT_OFFSET;

    *utc_shift = (raw_timezone >> 1) * 3600;
    if (raw_timezone & 1) {
        /* Adjust for DST */
        *utc_shift += 3600;
    }

    return TRUE;
}

static gboolean raw_timezone_to_utc_shift_5g (gint16 raw_timezone,
                                              gint *utc_shift)
{
    const int TZ_SHIFT = 8;

    if (utc_shift == NULL) {
        return FALSE;
    }
    /* The iPod stores the timezone information as a number of minutes
     * from Tokyo timezone which increases when going eastward (ie
     * going from Tokyo to LA and then to Europe).
     * The calculation below shifts the origin so that 0 corresponds
     * to UTC-12 and the max is 24*60 and corresponds to UTC+12
     * Finally, we substract 12*60 to that value to get a signed number
     * giving the timezone relative to UTC.
     */
    *utc_shift = raw_timezone*60 - TZ_SHIFT*3600;

    return TRUE;
}

static gint get_local_timezone (void)
{
    return timezone; /* global variable defined by time.h, see man tzset */
}

/* This function reads the timezone information from the iPod and sets it in
 * the Itdb_Device structure. If an error occurs, the function returns silently
 * and the timezone shift is set to 0
 */
static void itdb_device_set_timezone_info (Itdb_Device *device)
{
    gint16 raw_timezone;
    gint timezone = 0;
    gboolean result;
    struct stat stat_buf;
    int status;
    char *prefs_path;

    device->timezone_shift = get_local_timezone ();

    prefs_path = get_preferences_path (device);
    status = g_stat (prefs_path, &stat_buf);
    if (status != 0) {
	g_free (prefs_path);
	return;
    }
    switch (stat_buf.st_size) {
	case 2892:
  	    result = itdb_device_read_raw_timezone (prefs_path, 0xb10, 
			    			    &raw_timezone);
	    g_free (prefs_path);
	    if (!result) {
                return;
	    }
	    result = raw_timezone_to_utc_shift_4g (raw_timezone, &timezone);
	    break;
	case 2924:
            result = itdb_device_read_raw_timezone (prefs_path, 0xb22, 
			                            &raw_timezone);
	    g_free (prefs_path);
	    if (!result) {
                return;
	    }
	    result = raw_timezone_to_utc_shift_5g (raw_timezone, &timezone);
	    break;
	case 2952:
	    /* ipod classic, not implemented yet */
	default:
	    /* We don't know how to get the timezone of this ipod model,
	     * assume the computer timezone and the ipod timezone match
	     */
	    return; 
    }

    if ((timezone < -12*3600) || (timezone > 12 * 3600)) {
        return;
    }

    device->timezone_shift = timezone;
}

/**
 * itdb_device_get_firewire_id
 * @device: an #Itdb_Device
 *
 * Returns the Firewire ID for @device, this is useful to calculate the 
 * iTunesDB checksum which is expected by newer iPod models
 * (iPod classic/fat nanos)
 *
 * Return value: the guint64 firewire id, or 0 if we don't know it
 **/
guint64 itdb_device_get_firewire_id (const Itdb_Device *device)
{
    gchar *fwid;

    g_assert (device->sysinfo != NULL);

    fwid = g_hash_table_lookup (device->sysinfo, "FirewireGuid");
    if (fwid == NULL) {
	return 0;
    }
    return g_ascii_strtoull (fwid, NULL, 16);
}

G_GNUC_INTERNAL gboolean itdb_device_requires_checksum (Itdb_Device *device) 
{
    const Itdb_IpodInfo *info;

    if (device == NULL) {
        return FALSE;
    }

    info = itdb_device_get_ipod_info (device);
    if (info == NULL) {
        return FALSE;
    }
    switch (info->ipod_generation) {
    case ITDB_IPOD_GENERATION_CLASSIC_1: 
    case ITDB_IPOD_GENERATION_NANO_3:
    case ITDB_IPOD_GENERATION_TOUCH_1:
    case ITDB_IPOD_GENERATION_IPHONE_1:
      return TRUE;

    case ITDB_IPOD_GENERATION_UNKNOWN:
    case ITDB_IPOD_GENERATION_FIRST:
    case ITDB_IPOD_GENERATION_SECOND:
    case ITDB_IPOD_GENERATION_THIRD:
    case ITDB_IPOD_GENERATION_FOURTH:
    case ITDB_IPOD_GENERATION_PHOTO:
    case ITDB_IPOD_GENERATION_MOBILE:
    case ITDB_IPOD_GENERATION_MINI_1:
    case ITDB_IPOD_GENERATION_MINI_2:
    case ITDB_IPOD_GENERATION_SHUFFLE_1:
    case ITDB_IPOD_GENERATION_SHUFFLE_2:
    case ITDB_IPOD_GENERATION_SHUFFLE_3:
    case ITDB_IPOD_GENERATION_NANO_1:
    case ITDB_IPOD_GENERATION_NANO_2:
    case ITDB_IPOD_GENERATION_VIDEO_1:
    case ITDB_IPOD_GENERATION_VIDEO_2:
            return FALSE;
    }

    return FALSE;
}

#ifdef WIN32
#include <windows.h>
#else
#include <sys/statvfs.h>
#endif

/**
 * itdb_device_get_storage_info:
 *
 * @device: an #Itdb_Device
 * @capacity: returned capacity in bytes
 * @free: returned free space in bytes
 *
 * Return the storage info for this iPod
 *
 * Return value: TRUE if storage info could be obtained, FALSE otherwise
 **/
gboolean itdb_device_get_storage_info (Itdb_Device *device, guint64 *capacity, guint64 *free)
{
#ifdef WIN32
    ULARGE_INTEGER u_free, u_capacity;
#else
    struct statvfs info;
    guint64 block_size;
#endif

    g_return_val_if_fail (device, FALSE);
    g_return_val_if_fail (capacity, FALSE);
    g_return_val_if_fail (free, FALSE);

#ifdef WIN32
    if (GetDiskFreeSpaceEx(device->mountpoint, &u_free, &u_capacity, NULL) == 0) {
    	return FALSE;
    }
    *free = u_free.QuadPart;
    *capacity = u_capacity.QuadPart;
    return TRUE;
#else
    if (statvfs(device->mountpoint, &info))
	return FALSE;

    if (info.f_frsize > 0)
	block_size = info.f_frsize;
    else
	block_size = info.f_bsize;

    *capacity = info.f_blocks * block_size;
    *free = info.f_bfree * block_size;

    return TRUE;
#endif
}


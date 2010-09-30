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
#include <config.h>

#include "db-itunes-parser.h"
#include "itdb_device.h"
#include "itdb_private.h"
#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <glib/gi18n-lib.h>

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
    {"C167", 1,  ITDB_IPOD_MODEL_SHUFFLE_GOLD,   ITDB_IPOD_GENERATION_SHUFFLE_2, 3},
    /* Shuffle / Seventh Generation */
    /* Square, connected to computer via cable -- look identicaly to
     * Sixth Generation*/
    {"B225", 1,  ITDB_IPOD_MODEL_SHUFFLE_SILVER, ITDB_IPOD_GENERATION_SHUFFLE_3, 3},
    {"B233", 1,  ITDB_IPOD_MODEL_SHUFFLE_PURPLE, ITDB_IPOD_GENERATION_SHUFFLE_3, 3},
    {"B231", 1,  ITDB_IPOD_MODEL_SHUFFLE_RED,    ITDB_IPOD_GENERATION_SHUFFLE_3, 3},
    {"B227", 1,  ITDB_IPOD_MODEL_SHUFFLE_BLUE,   ITDB_IPOD_GENERATION_SHUFFLE_3, 3},
    {"B228", 1,  ITDB_IPOD_MODEL_SHUFFLE_BLUE,   ITDB_IPOD_GENERATION_SHUFFLE_3, 3},
    {"B229", 1,  ITDB_IPOD_MODEL_SHUFFLE_GREEN,  ITDB_IPOD_GENERATION_SHUFFLE_3, 3},
    {"B518", 2,  ITDB_IPOD_MODEL_SHUFFLE_SILVER, ITDB_IPOD_GENERATION_SHUFFLE_3, 3},
    {"B520", 2,  ITDB_IPOD_MODEL_SHUFFLE_BLUE,   ITDB_IPOD_GENERATION_SHUFFLE_3, 3},
    {"B522", 2,  ITDB_IPOD_MODEL_SHUFFLE_GREEN,  ITDB_IPOD_GENERATION_SHUFFLE_3, 3},
    {"B524", 2,  ITDB_IPOD_MODEL_SHUFFLE_RED,    ITDB_IPOD_GENERATION_SHUFFLE_3, 3},
    {"B526", 2,  ITDB_IPOD_MODEL_SHUFFLE_PURPLE, ITDB_IPOD_GENERATION_SHUFFLE_3, 3},

    /* Shuffle / Eigth Generation */
    /* Bar, button-less, speaking */
    {"C306", 2,  ITDB_IPOD_MODEL_SHUFFLE_SILVER,    ITDB_IPOD_GENERATION_SHUFFLE_4, 3},
    {"C323", 2,  ITDB_IPOD_MODEL_SHUFFLE_BLACK,     ITDB_IPOD_GENERATION_SHUFFLE_4, 3},
    {"C381", 2,  ITDB_IPOD_MODEL_SHUFFLE_GREEN,     ITDB_IPOD_GENERATION_SHUFFLE_4, 3},
    {"C384", 2,  ITDB_IPOD_MODEL_SHUFFLE_BLUE,      ITDB_IPOD_GENERATION_SHUFFLE_4, 3},
    {"C387", 2,  ITDB_IPOD_MODEL_SHUFFLE_PINK,      ITDB_IPOD_GENERATION_SHUFFLE_4, 3},
    {"B867", 4,  ITDB_IPOD_MODEL_SHUFFLE_SILVER,    ITDB_IPOD_GENERATION_SHUFFLE_4, 3},
    {"C164", 4,  ITDB_IPOD_MODEL_SHUFFLE_BLACK,     ITDB_IPOD_GENERATION_SHUFFLE_4, 3},
    {"C303", 4,  ITDB_IPOD_MODEL_SHUFFLE_STAINLESS, ITDB_IPOD_GENERATION_SHUFFLE_4, 3},
    {"C307", 4,  ITDB_IPOD_MODEL_SHUFFLE_GREEN,     ITDB_IPOD_GENERATION_SHUFFLE_4, 3},
    {"C328", 4,  ITDB_IPOD_MODEL_SHUFFLE_BLUE,      ITDB_IPOD_GENERATION_SHUFFLE_4, 3},
    {"C331", 4,  ITDB_IPOD_MODEL_SHUFFLE_PINK,      ITDB_IPOD_GENERATION_SHUFFLE_4, 3},

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
    {"A452", 30, ITDB_IPOD_MODEL_VIDEO_U2,    ITDB_IPOD_GENERATION_VIDEO_1,  50},

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
    {"B145", 160, ITDB_IPOD_MODEL_CLASSIC_SILVER, ITDB_IPOD_GENERATION_CLASSIC_1, 50},
    {"B150", 160, ITDB_IPOD_MODEL_CLASSIC_BLACK,  ITDB_IPOD_GENERATION_CLASSIC_1, 50},

    /* iPod Classic G2 */
    {"B562", 120, ITDB_IPOD_MODEL_CLASSIC_SILVER, ITDB_IPOD_GENERATION_CLASSIC_2, 50},
    {"B565", 120, ITDB_IPOD_MODEL_CLASSIC_BLACK,  ITDB_IPOD_GENERATION_CLASSIC_2, 50},

    /* iPod Classic G3 */
    {"C293", 160, ITDB_IPOD_MODEL_CLASSIC_SILVER, ITDB_IPOD_GENERATION_CLASSIC_3, 50},
    {"C297", 160, ITDB_IPOD_MODEL_CLASSIC_BLACK,  ITDB_IPOD_GENERATION_CLASSIC_3, 50},

    /* iPod nano video G1 (Third Nano Generation) */
    /* First generation of video support for nano */
    {"A978",   4, ITDB_IPOD_MODEL_NANO_SILVER,    ITDB_IPOD_GENERATION_NANO_3,  6},
    {"A980",   8, ITDB_IPOD_MODEL_NANO_SILVER,    ITDB_IPOD_GENERATION_NANO_3, 14},
    {"B261",   8, ITDB_IPOD_MODEL_NANO_BLACK,     ITDB_IPOD_GENERATION_NANO_3, 14},
    {"B249",   8, ITDB_IPOD_MODEL_NANO_BLUE,      ITDB_IPOD_GENERATION_NANO_3, 14},
    {"B253",   8, ITDB_IPOD_MODEL_NANO_GREEN,     ITDB_IPOD_GENERATION_NANO_3, 14},
    {"B257",   8, ITDB_IPOD_MODEL_NANO_RED,       ITDB_IPOD_GENERATION_NANO_3, 14},

    /* iPod nano video G2 (Fourth Nano Generation) */
    {"B480",   4, ITDB_IPOD_MODEL_NANO_SILVER,    ITDB_IPOD_GENERATION_NANO_4, 14},
    {"B651",   4, ITDB_IPOD_MODEL_NANO_BLUE,      ITDB_IPOD_GENERATION_NANO_4, 14},
    {"B654",   4, ITDB_IPOD_MODEL_NANO_PINK,      ITDB_IPOD_GENERATION_NANO_4, 14},
    {"B657",   4, ITDB_IPOD_MODEL_NANO_PURPLE,    ITDB_IPOD_GENERATION_NANO_4, 14},
    {"B660",   4, ITDB_IPOD_MODEL_NANO_ORANGE,    ITDB_IPOD_GENERATION_NANO_4, 14},
    {"B663",   4, ITDB_IPOD_MODEL_NANO_GREEN,     ITDB_IPOD_GENERATION_NANO_4, 14},
    {"B666",   4, ITDB_IPOD_MODEL_NANO_YELLOW,    ITDB_IPOD_GENERATION_NANO_4, 14},

    {"B598",   8, ITDB_IPOD_MODEL_NANO_SILVER,    ITDB_IPOD_GENERATION_NANO_4, 14},
    {"B732",   8, ITDB_IPOD_MODEL_NANO_BLUE,      ITDB_IPOD_GENERATION_NANO_4, 14},
    {"B735",   8, ITDB_IPOD_MODEL_NANO_PINK,      ITDB_IPOD_GENERATION_NANO_4, 14},
    {"B739",   8, ITDB_IPOD_MODEL_NANO_PURPLE,    ITDB_IPOD_GENERATION_NANO_4, 14},
    {"B742",   8, ITDB_IPOD_MODEL_NANO_ORANGE,    ITDB_IPOD_GENERATION_NANO_4, 14},
    {"B745",   8, ITDB_IPOD_MODEL_NANO_GREEN,     ITDB_IPOD_GENERATION_NANO_4, 14},
    {"B748",   8, ITDB_IPOD_MODEL_NANO_YELLOW,    ITDB_IPOD_GENERATION_NANO_4, 14},
    {"B751",   8, ITDB_IPOD_MODEL_NANO_RED,       ITDB_IPOD_GENERATION_NANO_4, 14},
    {"B754",   8, ITDB_IPOD_MODEL_NANO_BLACK,     ITDB_IPOD_GENERATION_NANO_4, 14},

    {"B903",  16, ITDB_IPOD_MODEL_NANO_SILVER,    ITDB_IPOD_GENERATION_NANO_4, 14},
    {"B905",  16, ITDB_IPOD_MODEL_NANO_BLUE,      ITDB_IPOD_GENERATION_NANO_4, 14},
    {"B907",  16, ITDB_IPOD_MODEL_NANO_PINK,      ITDB_IPOD_GENERATION_NANO_4, 14},
    {"B909",  16, ITDB_IPOD_MODEL_NANO_PURPLE,    ITDB_IPOD_GENERATION_NANO_4, 14},
    {"B911",  16, ITDB_IPOD_MODEL_NANO_ORANGE,    ITDB_IPOD_GENERATION_NANO_4, 14},
    {"B913",  16, ITDB_IPOD_MODEL_NANO_GREEN,     ITDB_IPOD_GENERATION_NANO_4, 14},
    {"B915",  16, ITDB_IPOD_MODEL_NANO_YELLOW,    ITDB_IPOD_GENERATION_NANO_4, 14},
    {"B917",  16, ITDB_IPOD_MODEL_NANO_RED,       ITDB_IPOD_GENERATION_NANO_4, 14},
    {"B918",  16, ITDB_IPOD_MODEL_NANO_BLACK,     ITDB_IPOD_GENERATION_NANO_4, 14},

    /* iPod nano with camera (Fifth Nano Generation) */
    {"C027",   8, ITDB_IPOD_MODEL_NANO_SILVER,    ITDB_IPOD_GENERATION_NANO_5, 14},
    {"C031",   8, ITDB_IPOD_MODEL_NANO_BLACK,     ITDB_IPOD_GENERATION_NANO_5, 14},
    {"C034",   8, ITDB_IPOD_MODEL_NANO_PURPLE,    ITDB_IPOD_GENERATION_NANO_5, 14},
    {"C037",   8, ITDB_IPOD_MODEL_NANO_BLUE,      ITDB_IPOD_GENERATION_NANO_5, 14},
    {"C040",   8, ITDB_IPOD_MODEL_NANO_GREEN,     ITDB_IPOD_GENERATION_NANO_5, 14},
    {"C043",   8, ITDB_IPOD_MODEL_NANO_YELLOW,    ITDB_IPOD_GENERATION_NANO_5, 14},
    {"C046",   8, ITDB_IPOD_MODEL_NANO_ORANGE,    ITDB_IPOD_GENERATION_NANO_5, 14},
    {"C049",   8, ITDB_IPOD_MODEL_NANO_RED,       ITDB_IPOD_GENERATION_NANO_5, 14},
    {"C050",   8, ITDB_IPOD_MODEL_NANO_PINK,      ITDB_IPOD_GENERATION_NANO_5, 14},

    {"C060",  16, ITDB_IPOD_MODEL_NANO_SILVER,    ITDB_IPOD_GENERATION_NANO_5, 14},
    {"C062",  16, ITDB_IPOD_MODEL_NANO_BLACK,     ITDB_IPOD_GENERATION_NANO_5, 14},
    {"C064",  16, ITDB_IPOD_MODEL_NANO_PURPLE,    ITDB_IPOD_GENERATION_NANO_5, 14},
    {"C066",  16, ITDB_IPOD_MODEL_NANO_BLUE,      ITDB_IPOD_GENERATION_NANO_5, 14},
    {"C068",  16, ITDB_IPOD_MODEL_NANO_GREEN,     ITDB_IPOD_GENERATION_NANO_5, 14},
    {"C070",  16, ITDB_IPOD_MODEL_NANO_YELLOW,    ITDB_IPOD_GENERATION_NANO_5, 14},
    {"C072",  16, ITDB_IPOD_MODEL_NANO_ORANGE,    ITDB_IPOD_GENERATION_NANO_5, 14},
    {"C074",  16, ITDB_IPOD_MODEL_NANO_RED,       ITDB_IPOD_GENERATION_NANO_5, 14},
    {"C075",  16, ITDB_IPOD_MODEL_NANO_PINK,      ITDB_IPOD_GENERATION_NANO_5, 14},

    /* iPod nano touch (Sixth Generation) */
    {"C525",   8, ITDB_IPOD_MODEL_NANO_SILVER,    ITDB_IPOD_GENERATION_NANO_6, 14},
    {"C688",   8, ITDB_IPOD_MODEL_NANO_BLACK,     ITDB_IPOD_GENERATION_NANO_6, 14},
    {"C689",   8, ITDB_IPOD_MODEL_NANO_BLUE,      ITDB_IPOD_GENERATION_NANO_6, 14},
    {"C690",   8, ITDB_IPOD_MODEL_NANO_GREEN,     ITDB_IPOD_GENERATION_NANO_6, 14},
    {"C691",   8, ITDB_IPOD_MODEL_NANO_ORANGE,    ITDB_IPOD_GENERATION_NANO_6, 14},
    {"C692",   8, ITDB_IPOD_MODEL_NANO_PINK,      ITDB_IPOD_GENERATION_NANO_6, 14},
    {"C693",   8, ITDB_IPOD_MODEL_NANO_RED,       ITDB_IPOD_GENERATION_NANO_6, 14},

    {"C526",  16, ITDB_IPOD_MODEL_NANO_SILVER,    ITDB_IPOD_GENERATION_NANO_6, 14},
    {"C694",  16, ITDB_IPOD_MODEL_NANO_BLACK,     ITDB_IPOD_GENERATION_NANO_6, 14},
    {"C695",  16, ITDB_IPOD_MODEL_NANO_BLUE,      ITDB_IPOD_GENERATION_NANO_6, 14},
    {"C696",  16, ITDB_IPOD_MODEL_NANO_GREEN,     ITDB_IPOD_GENERATION_NANO_6, 14},
    {"C697",  16, ITDB_IPOD_MODEL_NANO_ORANGE,    ITDB_IPOD_GENERATION_NANO_6, 14},
    {"C698",  16, ITDB_IPOD_MODEL_NANO_PINK,      ITDB_IPOD_GENERATION_NANO_6, 14},
    {"C699",  16, ITDB_IPOD_MODEL_NANO_RED,       ITDB_IPOD_GENERATION_NANO_6, 14},

    /* iPod Touch 1st gen */
    {"A623",   8, ITDB_IPOD_MODEL_TOUCH_SILVER,   ITDB_IPOD_GENERATION_TOUCH_1, 50},
    {"A627",  16, ITDB_IPOD_MODEL_TOUCH_SILVER,   ITDB_IPOD_GENERATION_TOUCH_1, 50},
    {"B376",  32, ITDB_IPOD_MODEL_TOUCH_SILVER,   ITDB_IPOD_GENERATION_TOUCH_1, 50},

    /* iPod Touch 2nd gen */
    {"B528",   8, ITDB_IPOD_MODEL_TOUCH_SILVER,   ITDB_IPOD_GENERATION_TOUCH_2, 50},
    {"B531",  16, ITDB_IPOD_MODEL_TOUCH_SILVER,   ITDB_IPOD_GENERATION_TOUCH_2, 50},
    {"B533",  32, ITDB_IPOD_MODEL_TOUCH_SILVER,   ITDB_IPOD_GENERATION_TOUCH_2, 50},

    /* iPod Touch 3rd gen */
    /* The 8GB model is marked as 2nd gen because it's actually what the 
     * hardware is even if Apple markets it the same as the 2 bigger models
     */
    {"C086",   8, ITDB_IPOD_MODEL_TOUCH_SILVER,   ITDB_IPOD_GENERATION_TOUCH_2, 50},
    {"C008",  32, ITDB_IPOD_MODEL_TOUCH_SILVER,   ITDB_IPOD_GENERATION_TOUCH_3, 50},
    {"C011",  64, ITDB_IPOD_MODEL_TOUCH_SILVER,   ITDB_IPOD_GENERATION_TOUCH_3, 50},

    /* iPod Touch 4th gen */
    {"C540",   8, ITDB_IPOD_MODEL_TOUCH_SILVER,   ITDB_IPOD_GENERATION_TOUCH_4, 50},
    {"C544",  32, ITDB_IPOD_MODEL_TOUCH_SILVER,   ITDB_IPOD_GENERATION_TOUCH_4, 50},
    {"C547",  64, ITDB_IPOD_MODEL_TOUCH_SILVER,   ITDB_IPOD_GENERATION_TOUCH_4, 50},

    /* iPhone */
    {"A501",   4, ITDB_IPOD_MODEL_IPHONE_1,       ITDB_IPOD_GENERATION_IPHONE_1, 50},
    {"A712",   8, ITDB_IPOD_MODEL_IPHONE_1,       ITDB_IPOD_GENERATION_IPHONE_1, 50},
    {"B384",  16, ITDB_IPOD_MODEL_IPHONE_1,       ITDB_IPOD_GENERATION_IPHONE_1, 50},
    /* iPhone G2 aka iPhone 3G (yeah, confusing ;) */
    {"B046",   8, ITDB_IPOD_MODEL_IPHONE_BLACK,   ITDB_IPOD_GENERATION_IPHONE_2, 50},
    {"B500",  16, ITDB_IPOD_MODEL_IPHONE_WHITE,   ITDB_IPOD_GENERATION_IPHONE_2, 50},
    {"B048",  16, ITDB_IPOD_MODEL_IPHONE_BLACK,   ITDB_IPOD_GENERATION_IPHONE_2, 50},
    {"B496",  16, ITDB_IPOD_MODEL_IPHONE_BLACK,   ITDB_IPOD_GENERATION_IPHONE_2, 50},
    /* iPhone 3GS */
    {"C131",  16, ITDB_IPOD_MODEL_IPHONE_BLACK,   ITDB_IPOD_GENERATION_IPHONE_3, 50},
    {"C133",  32, ITDB_IPOD_MODEL_IPHONE_BLACK,   ITDB_IPOD_GENERATION_IPHONE_3, 50},
    {"C134",  32, ITDB_IPOD_MODEL_IPHONE_WHITE,   ITDB_IPOD_GENERATION_IPHONE_3, 50},
    /* iPhone 4G */
    {"C603",  16, ITDB_IPOD_MODEL_IPHONE_BLACK,   ITDB_IPOD_GENERATION_IPHONE_4, 50},
    {"C605",  32, ITDB_IPOD_MODEL_IPHONE_BLACK,   ITDB_IPOD_GENERATION_IPHONE_4, 50},

    /* iPad */
    {"B292",  16, ITDB_IPOD_MODEL_IPAD,           ITDB_IPOD_GENERATION_IPAD_1,   50},
    {"B293",  32, ITDB_IPOD_MODEL_IPAD,           ITDB_IPOD_GENERATION_IPAD_1,   50},
    {"B294",  64, ITDB_IPOD_MODEL_IPAD,           ITDB_IPOD_GENERATION_IPAD_1,   50},
    /* iPad with 3G */
    {"C349",  16, ITDB_IPOD_MODEL_IPAD,           ITDB_IPOD_GENERATION_IPAD_1,   50},
    {"C496",  32, ITDB_IPOD_MODEL_IPAD,           ITDB_IPOD_GENERATION_IPAD_1,   50},
    {"C497",  64, ITDB_IPOD_MODEL_IPAD,           ITDB_IPOD_GENERATION_IPAD_1,   50},

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
	N_("Nano (Yellow)"),
	N_("Nano (Purple)"),
	N_("Nano (Orange)"),
	N_("iPhone (1)"),
	N_("Shuffle (Silver)"),
	N_("Shuffle (Pink)"),
	N_("Shuffle (Blue)"),
	N_("Shuffle (Green)"),
	N_("Shuffle (Orange)"),
	N_("Shuffle (Purple)"),
	N_("Shuffle (Red)"),
	N_("Classic (Silver)"),
	N_("Classic (Black)"),
	N_("Touch (Silver)"),
	N_("Shuffle (Black)"),
	N_("iPhone (White)"),
	N_("iPhone (Black)"),
	N_("Shuffle (Gold)"),
	N_("Shuffle (Stainless)"),
	N_("iPad"),
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
	N_("Nano Video (4th Gen.)"),
	N_("Video (1st Gen.)"),
	N_("Video (2nd Gen.)"),
	N_("Classic"),
	N_("Classic"),
	N_("Touch"),
	N_("iPhone"),
	N_("Shuffle (4th Gen.)"),
	N_("Touch (2nd Gen.)"),
	N_("iPhone 3G"),
	N_("iPhone 3GS"),
	N_("Classic"),
	N_("Nano with camera (5th Gen.)"),
	N_("Touch (3rd Gen.)"),
	N_("iPad"),
	N_("iPhone 4"),
	N_("Touch (4th Gen.)"),
	N_("Nano touch (6th Gen.)"),
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

static const Itdb_ArtworkFormat ipod_nano4g_cover_art_info[] = {
    {1055, 128, 128, THUMB_FORMAT_RGB565_LE},
    {1068, 128, 128, THUMB_FORMAT_RGB565_LE},
    {1071, 240, 240, THUMB_FORMAT_RGB565_LE},
    {1074,  50,  50, THUMB_FORMAT_RGB565_LE},
    {1078,  80,  80, THUMB_FORMAT_RGB565_LE},
    {1084, 240, 240, THUMB_FORMAT_RGB565_LE},
    {  -1,  -1,  -1, -1}
};

static const Itdb_ArtworkFormat ipod_nano4g_photo_info[] = {
    {1024, 320, 240, THUMB_FORMAT_RGB565_LE},
    {1066,  64,  64, THUMB_FORMAT_RGB565_LE},
    {1079,  80,  80, THUMB_FORMAT_RGB565_LE},
    /*
     * To be implemented, THUMB_FORMAT_JPEG 
    {1081, 640, 480, THUMB_FORMAT_JPEG},
    */
    {1083, 240, 320, THUMB_FORMAT_RGB565_LE},
    {  -1,  -1,  -1, -1}
};

static const Itdb_ArtworkFormat ipod_nano4g_chapter_image_info[] = {
    {1071, 240, 240, THUMB_FORMAT_RGB565_LE},
};

static const Itdb_ArtworkFormat ipod_nano5g_cover_art_info[] = {
    {1056, 128, 128, THUMB_FORMAT_RGB565_LE},
    {1078,  80,  80, THUMB_FORMAT_RGB565_LE},
    {1073,  240,  240, THUMB_FORMAT_RGB565_LE},
    {1074,  50,  50, THUMB_FORMAT_RGB565_LE},
    {  -1,  -1,  -1, -1}
};

static const Itdb_ArtworkFormat ipod_nano5g_photo_info[] = {
    {1087, 384, 384, THUMB_FORMAT_RGB565_LE},
    /*
     * To be implemented, THUMB_FORMAT_JPEG 
     {1081,  640,  480, THUMB_FORMAT_JPEG},
     */
    {1079,  80,  80, THUMB_FORMAT_RGB565_LE},
    {1066,  64,  64, THUMB_FORMAT_RGB565_LE},
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
    {3007,  88,  88, THUMB_FORMAT_RGB555_LE, 16384}, /*pad data to 16384 bytes */
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

/* also used for 3G Nano */
static const Itdb_ArtworkFormat ipod_classic_1_cover_art_info[] = {
    /* officially 55x55 -- verify! */
    {1061,  56,  56, THUMB_FORMAT_RGB565_LE},
    {1055, 128, 128, THUMB_FORMAT_RGB565_LE},
    {1068, 128, 128, THUMB_FORMAT_RGB565_LE},
    {1060, 320, 320, THUMB_FORMAT_RGB565_LE},
    {  -1,  -1,  -1, -1}
};

/* also used for 3G Nano */
static const Itdb_ArtworkFormat ipod_classic_1_photo_info[] = {
    {1067, 720, 480, THUMB_FORMAT_I420_LE},
    {1024, 320, 240, THUMB_FORMAT_RGB565_LE},
    {1066,  64,  64, THUMB_FORMAT_RGB565_LE},
    {  -1,  -1,  -1, -1}
};

/* also used for 3G Nano */
static const Itdb_ArtworkFormat ipod_classic_1_chapter_image_info[] = {
    /*  These are the same as for the iPod video... -- labeled by the iPod as
        "chapter images" */
    {1055, 128, 128, THUMB_FORMAT_RGB565_LE},
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
    { ITDB_IPOD_GENERATION_NANO_4, ipod_nano4g_cover_art_info, ipod_nano4g_photo_info, ipod_nano4g_chapter_image_info },
    { ITDB_IPOD_GENERATION_NANO_5, ipod_nano5g_cover_art_info, ipod_nano5g_photo_info, NULL },
    { ITDB_IPOD_GENERATION_CLASSIC_1, ipod_classic_1_cover_art_info, ipod_classic_1_photo_info, ipod_classic_1_chapter_image_info },
    { ITDB_IPOD_GENERATION_CLASSIC_2, ipod_classic_1_cover_art_info, ipod_classic_1_photo_info, ipod_classic_1_chapter_image_info },
    { ITDB_IPOD_GENERATION_CLASSIC_3, ipod_classic_1_cover_art_info, ipod_classic_1_photo_info, ipod_classic_1_chapter_image_info },
    { ITDB_IPOD_GENERATION_TOUCH_1, ipod_touch_1_cover_art_info, ipod_touch_1_photo_info, NULL },
    { ITDB_IPOD_GENERATION_TOUCH_2, ipod_touch_1_cover_art_info, ipod_touch_1_photo_info, NULL },
    { ITDB_IPOD_GENERATION_TOUCH_3, ipod_touch_1_cover_art_info, ipod_touch_1_photo_info, NULL },
    { ITDB_IPOD_GENERATION_IPHONE_1, ipod_touch_1_cover_art_info, ipod_touch_1_photo_info, NULL },
    { ITDB_IPOD_GENERATION_IPHONE_2, ipod_touch_1_cover_art_info, ipod_touch_1_photo_info, NULL },
    { ITDB_IPOD_GENERATION_IPHONE_3, ipod_touch_1_cover_art_info, ipod_touch_1_photo_info, NULL },
    { ITDB_IPOD_GENERATION_MOBILE, ipod_mobile_1_cover_art_info, NULL, NULL },
    { ITDB_IPOD_GENERATION_UNKNOWN, NULL, NULL, NULL }
};

struct _ItdbSerialToModel {
    const char *serial;
    const char *model_number;
};
typedef struct _ItdbSerialToModel ItdbSerialToModel;

/* This table was extracted from ipod-model-table from podsleuth svn trunk
 * on 2008-06-14 (which seems to match podsleuth 0.6.2)
*/
static const ItdbSerialToModel serial_to_model_mapping[] = {
    { "LG6", "8541" },
    { "NAM", "8541" },
    { "MJ2", "8541" },
    { "ML1", "8709" },
    { "MME", "8709" },
    { "MMB", "8737" },
    { "MMC", "8738" },
    { "NGE", "8740" },
    { "NGH", "8740" },
    { "MMF", "8741" },
    { "NLW", "8946" },
    { "NRH", "8976" },
    { "QQF", "9460" },
    { "PQ5", "9244" },
    { "PNT", "9244" },
    { "NLY", "8948" },
    { "NM7", "8948" },
    { "PNU", "9245" },
    { "PS9", "9282" },
    { "Q8U", "9282" },
    { "V9V", "9787" },
    { "S2X", "9787" },
    { "PQ7", "9268" },
    { "TDU", "A079" },
    { "TDS", "A079" },
    { "TM2", "A127" },
    { "SAZ", "9830" },
    { "SB1", "9830" },
    { "SAY", "9829" },
    { "R5Q", "9585" },
    { "R5R", "9586" },
    { "R5T", "9586" },
    { "PFW", "9160" },
    { "PRC", "9160" },
    { "QKL", "9436" },
    { "QKQ", "9436" },
    { "QKK", "9435" },
    { "QKP", "9435" },
    { "QKJ", "9434" },
    { "QKN", "9434" },
    { "QKM", "9437" },
    { "QKR", "9437" },
    { "S41", "9800" },
    { "S4C", "9800" },
    { "S43", "9802" },
    { "S45", "9804" },
    { "S47", "9806" },
    { "S4J", "9806" },
    { "S42", "9801" },
    { "S44", "9803" },
    { "S48", "9807" },
    { "RS9", "9724" },
    { "QGV", "9724" },
    { "TSX", "9724" },
    { "PFV", "9724" },
    { "R80", "9724" },
    { "RSA", "9725" },
    { "TSY", "9725" },
    { "C60", "9725" },
    { "VTE", "A546" },
    { "VTF", "A546" },
    { "XQ5", "A947" },
    { "XQS", "A947" },
    { "XQV", "A949" },
    { "XQX", "A949" },
    { "YX7", "A949" },
    { "XQY", "A951" },
    { "YX8", "A951" },
    { "XR1", "A953" },
    { "YXA", "B233" }, /* 1GB Purple Shuffle 2g */
    { "YX6", "B225" }, /* 1GB Silver Shuffle 2g */
    { "YX7", "B228" }, /* 1GB Blue Shuffle 2g */
    { "YX9", "B225" }, /* 1GB Silver Shuffle 2g */
    { "8CQ", "C167" }, /* 1GB Gold Shuffle 2g */
    { "1ZH", "B518" }, /* 2GB Silver Shuffle 2g */
    { "UNA", "A350" },
    { "UNB", "A350" },
    { "UPR", "A352" },
    { "UPS", "A352" },
    { "SZB", "A004" },
    { "SZV", "A004" },
    { "SZW", "A004" },
    { "SZC", "A005" },
    { "SZT", "A005" },
    { "TJT", "A099" },
    { "TJU", "A099" },
    { "TK2", "A107" },
    { "TK3", "A107" },
    { "VQ5", "A477" },
    { "VQ6", "A477" },
    { "V8T", "A426" },
    { "V8U", "A426" },
    { "V8W", "A428" },
    { "V8X", "A428" },
    { "VQH", "A487" },
    { "VQJ", "A487" },
    { "VQK", "A489" },
    { "VKL", "A489" },
    { "WL2", "A725" },
    { "WL3", "A725" },
    { "X9A", "A726" },
    { "X9B", "A726" },
    { "VQT", "A497" },
    { "VQU", "A497" },
    { "Y0P", "A978" },
    { "Y0R", "A980" },
    { "YXR", "B249" },
    { "YXV", "B257" },
    { "YXT", "B253" },
    { "YXX", "B261" },
    { "SZ9", "A002" },
    { "WEC", "A002" },
    { "WED", "A002" },
    { "WEG", "A002" },
    { "WEH", "A002" },
    { "WEL", "A002" },
    { "TXK", "A146" },
    { "TXM", "A146" },
    { "WEE", "A146" },
    { "WEF", "A146" },
    { "WEJ", "A146" },
    { "WEK", "A146" },
    { "SZA", "A003" },
    { "SZU", "A003" },
    { "TXL", "A147" },
    { "TXN", "A147" },
    { "V9K", "A444" },
    { "V9L", "A444" },
    { "WU9", "A444" },
    { "VQM", "A446" },
    { "V9M", "A446" },
    { "V9N", "A446" },
    { "WEE", "A446" },
    { "V9P", "A448" },
    { "V9Q", "A448" },
    { "V9R", "A450" },
    { "V9S", "A450" },
    { "V95", "A450" },
    { "V96", "A450" },
    { "WUC", "A450" },
    { "W9G", "A664" }, /* 30GB iPod Video U2 5.5g */
    { "Y5N", "B029" }, /* Silver Classic 80GB */
    { "YMV", "B147" }, /* Black Classic 80GB */
    { "YMU", "B145" }, /* Silver Classic 160GB */
    { "YMX", "B150" }, /* Black Classic 160GB */
    { "2C5", "B562" }, /* Silver Classic 120GB */
    { "2C7", "B565" }, /* Black Classic 120GB */
    { "9ZS", "C293" }, /* Silver Classic 160GB (2009) */
    { "9ZU", "C297" }, /* Black Classic 160GB (2009) */

    { "37P", "B663" }, /* 4GB Green Nano 4g */
    { "37Q", "B666" }, /* 4GB Yellow Nano 4g */
    { "37H", "B654" }, /* 4GB Pink Nano 4g */
    { "1P1", "B480" }, /* 4GB Silver Nano 4g */
    { "37K", "B657" }, /* 4GB Purple Nano 4g */
    { "37L", "B660" }, /* 4GB Orange Nano 4g */
    { "2ME", "B598" }, /* 8GB Silver Nano 4g */
    { "3QS", "B732" }, /* 8GB Blue Nano 4g */
    { "3QT", "B735" }, /* 8GB Pink Nano 4g */
    { "3QU", "B739" }, /* 8GB Purple Nano 4g */
    { "3QW", "B742" }, /* 8GB Orange Nano 4g */
    { "3QX", "B745" }, /* 8GB Green Nano 4g */
    { "3QY", "B748" }, /* 8GB Yellow Nano 4g */
    { "3R0", "B754" }, /* 8GB Black Nano 4g */
    { "3QZ", "B751" }, /* 8GB Red Nano 4g */
    { "5B7", "B903" }, /* 16GB Silver Nano 4g */
    { "5B8", "B905" }, /* 16GB Blue Nano 4g */
    { "5B9", "B907" }, /* 16GB Pink Nano 4g */
    { "5BA", "B909" }, /* 16GB Purple Nano 4g */
    { "5BB", "B911" }, /* 16GB Orange Nano 4g */
    { "5BC", "B913" }, /* 16GB Green Nano 4g */
    { "5BD", "B915" }, /* 16GB Yellow Nano 4g */
    { "5BE", "B917" }, /* 16GB Red Nano 4g */
    { "5BF", "B918" }, /* 16GB Black Nano 4g */

    { "71V", "C027" }, /* 8GB Silver Nano 5g */
    { "71Y", "C031" }, /* 8GB Black Nano 5g */
    { "721", "C034" }, /* 8GB Purple Nano 5g */
    { "726", "C037" }, /* 8GB Blue Nano 5g */
    { "72A", "C040" }, /* 8GB Green Nano 5g */
    { "72F", "C046" }, /* 8GB Orange Nano 5g */
    { "72K", "C049" }, /* 8GB Red Nano 5g */
    { "72L", "C050" }, /* 8GB Pink Nano 5g */

    { "72Q", "C060" }, /* 16GB Silver Nano 5g */
    { "72R", "C062" }, /* 16GB Black Nano 5g */
    { "72S", "C064" }, /* 16GB Purple Nano 5g */
    { "72X", "C066" }, /* 16GB Blue Nano 5g */
    { "734", "C068" }, /* 16GB Green Nano 5g */
    { "738", "C070" }, /* 16GB Yellow Nano 5g */
    { "739", "C072" }, /* 16GB Orange Nano 5g */
    { "73A", "C074" }, /* 16GB Red Nano 5g */
    { "73B", "C075" }, /* 16GB Pink Nano 5g */

    { "CMN", "C525" }, /* 8GB Silver Nano 6g */
    { "DVX", "C688" }, /* 8GB Black Nano 6g */
    { "DW2", "C692" }, /* 8GB Pink Nano 6g */
    { "CMP", "C526" }, /* 16GB Silver Nano 6g */
    { "DW9", "C699" }, /* 16GB Red Nano 6g */

    { "A1S", "C306" }, /* 2GB Silver Shuffle 4g */
    { "A78", "C323" }, /* 2GB Black Shuffle 4g */
    { "ALB", "C381" }, /* 2GB Green Shuffle 4g */
    { "ALD", "C384" }, /* 2GB Blue Shuffle 4g */
    { "ALG", "C387" }, /* 2GB Pink Shuffle 4g */
    { "4NZ", "B867" }, /* 4GB Silver Shuffle 4g */
    { "891", "C164" }, /* 4GB Black Shuffle 4g */
    { "A1L", "C303" }, /* 4GB Stainless Shuffle 4g */
    { "A1U", "C307" }, /* 4GB Green Shuffle 4g */
    { "A7B", "C328" }, /* 4GB Blue Shuffle 4g */
    { "A7D", "C331" }, /* 4GB Pink Shuffle 4g */

    { "W4N", "A623" }, /* 8GB Silver iPod Touch (1st gen) */
    { "W4T", "A627" }, /* 16GB Silver iPod Touch (1st gen) */
    { "0JW", "B376" }, /* 32GB Silver iPod Touch (1st gen) */
    { "201", "B528" }, /* 8GB Silver iPod Touch (2nd gen) */
    { "203", "B531" }, /* 16GB Silver iPod Touch (2nd gen) */
    { "75J", "C086" }, /* 8GB iPod Touch (3rd gen) */
    { "6K2", "C008" }, /* 32GB iPod Touch (3rd gen) */
    { "6K4", "C011" }, /* 64GB iPod Touch (3rd gen) */
    { "CP7", "C540" }, /* 8GB iPod Touch (4th gen) */
    { "CP9", "C544" }, /* 32GB iPod Touch (4th gen) */
    { "CPC", "C547" }, /* 64GB iPod Touch (4th gen) */

    { "VR0", "A501" }, /* 4GB Silver iPhone 1st gen */
    { "WH8", "A712" }, /* 8GB Silver iPhone */
    { "0KH", "B384" }, /* 16GB Silver iPhone */
    { "Y7H", "B046" }, /* 8GB Black iPhone 3G */
    { "Y7K", "B496" }, /* 16GB Black iPhone 3G */
    { "3NP", "C131" }, /* 16GB Black iPhone 3GS */
    { "3NR", "C133" }, /* 32GB Black iPhone 3GS */
    { "3NS", "C134" }, /* 32GB White iPhone 3GS */
    { "A4S", "C603" }, /* 16GB Black iPhone 4G */
    { "A4T", "C605" }, /* 32GB Black iPhone 4G */

    { "Z38", "B292" }, /* 16GB iPad with Wifi */
    { "Z39", "B293" }, /* 32GB iPad with Wifi */
    { "Z3A", "B294" }, /* 64GB iPad with Wifi */
    { NULL ,  NULL  }
};

static const Itdb_IpodInfo *get_ipod_info_from_model_number (const char *model_number);

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
 * Returns: a newly allocated #Itdb_Device which must be freed with
 * itdb_device_free() when no longer needed
 *
 * Since: 0.4.0
 */
Itdb_Device *itdb_device_new (void)
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
 *
 * Since: 0.4.0
 */
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
 * @mp:     the new mount point
 *
 * Sets the mountpoint of @device to @mp and update the cached device 
 * information (in particular, re-read the SysInfo file)
 *
 * <warning><para>Calling this function invalidates all the artwork in the
 * #Itdb_iTunesDB database using this #Itdb_Device. Trying to access this
 * artwork will result in memory corruption. It's recommended to use
 * itdb_set_mountpoint() instead which will clean the invalidated artwork
 * for you.</para></warning>.
 *
 * Since: 0.4.0
 */
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
    device->sysinfo_extended = itdb_sysinfo_extended_parse (sysinfo_ex_path, NULL);
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
 * <warning><para>Calling this function invalidates all the artwork in the
 * #Itdb_iTunesDB database using this #Itdb_Device. Trying to access this
 * artwork will result in memory corruption. Directly calling this function
 * shouldn't ever be needed and it will be deprecated
 * soon.</para></warning>.
 *
 * Returns: TRUE if file could be read, FALSE otherwise 
 *
 * Since: 0.4.0
 */
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
 * @error:  return location for a #GError or NULL
 *
 * Fills the SysInfo file with information in device->sysinfo. Note:
 * no directories are created if not already existent.
 *
 * Returns: TRUE if file could be read, FALSE otherwise 
 *
 * Since: 0.4.0
 */
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
 * @field:  field to retrive information from
 *
 * Retrieve specified field from the SysInfo file.
 *
 * Returns: the information associated with @field, or NULL if @field
 * couldn't be found. g_free() after use
 *
 * Since: 0.4.0
 */
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
 * @field:  field to set
 * @value:  value to set (or NULL to remove the field).
 *
 * Set specified field. It can later be written to the iPod using
 * itdb_device_write_sysinfo()
 *
 * Since: 0.4.0
 */
void itdb_device_set_sysinfo (Itdb_Device *device,
			      const gchar *field, const gchar *value)
{
    g_return_if_fail (device);
    g_return_if_fail (device->sysinfo);
    g_return_if_fail (field);

    if (value)
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
 * Returns: the #Itdb_IpodInfo entry for this iPod
 *
 * Since: 0.4.0
 */
const Itdb_IpodInfo *
itdb_device_get_ipod_info (const Itdb_Device *device)
{
    gchar *model_num;
    const Itdb_IpodInfo *info;

    if (device->sysinfo_extended != NULL) {
        const char *serial;

        serial = itdb_sysinfo_properties_get_serial_number (device->sysinfo_extended);
        info = itdb_ipod_info_from_serial (serial);
        if (info != NULL) {
            return info;
        }
    }

    model_num = itdb_device_get_sysinfo (device, "ModelNumStr");

    if (!model_num)
	return &ipod_info_table[0];

    info = get_ipod_info_from_model_number (model_num);
    g_free(model_num);
    if (info != NULL) {
        return info;
    } else {
        return &ipod_info_table[1];
    }
}


/* Return supported artwork formats supported by this iPod */
static const ArtworkCapabilities *
itdb_device_get_artwork_capabilities (const Itdb_Device *device)
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

    if (caps->generation == ITDB_IPOD_GENERATION_UNKNOWN) {
	return NULL;
    }
 
    return caps;
}

static GList *
itdb_device_get_artwork_formats_fallback (const Itdb_Device *device, 
					  enum ArtworkType type)
{
    const ArtworkCapabilities *caps;
    const Itdb_ArtworkFormat *formats = NULL;
    const Itdb_ArtworkFormat *it;
    GList *artwork_formats = NULL;

    caps = itdb_device_get_artwork_capabilities (device);
    if (caps == NULL) {
	return NULL;
    }
    switch (type) {
	case ARTWORK_TYPE_COVER_ART:
	    formats = caps->cover_art_formats;
	    break;
	case ARTWORK_TYPE_PHOTO:
	    formats = caps->photo_formats;
	    break;
	case ARTWORK_TYPE_CHAPTER_IMAGE:
	    formats = caps->chapter_image_formats;
	    break;
    }
    if (formats == NULL) {
        return NULL;
    }

    for (it = formats; it->format_id != -1; it++) {
        artwork_formats = g_list_prepend (artwork_formats, (gpointer)it);
    }

    return artwork_formats;
}

/**
 * itdb_device_get_photo_formats:
 * @device: an #Itdb_Device
 *
 * Returns: a #GList of #Itdb_ArtworkFormat describing the photo formats 
 * supported by the iPod associated with @device. The returned list must
 * be freed with g_list_free() when no longer needed.
 *
 **/
GList *itdb_device_get_photo_formats (const Itdb_Device *device)
{
    g_return_val_if_fail (device != NULL, NULL);

    if (device->sysinfo_extended == NULL) {
        return itdb_device_get_artwork_formats_fallback (device,
							 ARTWORK_TYPE_PHOTO);
    } else {
        return g_list_copy ((GList *)itdb_sysinfo_properties_get_photo_formats (device->sysinfo_extended));
    }
    g_return_val_if_reached (NULL);
}

/**
 * itdb_device_get_cover_art_formats:
 * @device: an #Itdb_Device
 *
 * Returns: a #GList of #Itdb_ArtworkFormat describing the cover art formats 
 * supported by the iPod associated with @device. The returned list must
 * freed with g_list_free() when no longer needed.
 *
 **/
GList *itdb_device_get_cover_art_formats (const Itdb_Device *device)
{
    g_return_val_if_fail (device != NULL, NULL);

    if (device->sysinfo_extended == NULL) {
        return itdb_device_get_artwork_formats_fallback (device,
							 ARTWORK_TYPE_COVER_ART);
    } else {
        return g_list_copy ((GList *)itdb_sysinfo_properties_get_cover_art_formats (device->sysinfo_extended));
    }
    g_return_val_if_reached (NULL);
}

/**
 * itdb_device_get_chapter_image_formats:
 * @device: an #Itdb_Device
 *
 * Returns: a #GList of #Itdb_ArtworkFormat describing the chapter image 
 * formats supported by the iPod associated with @device. The returned list
 * must be freed with g_list_free() when no longer needed.
 *
 **/
GList *itdb_device_get_chapter_image_formats (const Itdb_Device *device)
{
    g_return_val_if_fail (device != NULL, NULL);

    if (device->sysinfo_extended == NULL) {
        return itdb_device_get_artwork_formats_fallback (device,
							 ARTWORK_TYPE_CHAPTER_IMAGE);
    } else {
        return g_list_copy ((GList *)itdb_sysinfo_properties_get_chapter_image_formats (device->sysinfo_extended));
    }
    g_return_val_if_reached (NULL);
}

G_GNUC_INTERNAL gboolean
itdb_device_supports_sqlite_db (const Itdb_Device *device)
{
    if (device->sysinfo_extended != NULL) {
	return itdb_sysinfo_properties_supports_sqlite (device->sysinfo_extended);
    } else {
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
	    case ITDB_IPOD_GENERATION_SHUFFLE_4:
	    case ITDB_IPOD_GENERATION_NANO_1:
	    case ITDB_IPOD_GENERATION_NANO_2:
	    case ITDB_IPOD_GENERATION_VIDEO_1:
	    case ITDB_IPOD_GENERATION_VIDEO_2:
	    case ITDB_IPOD_GENERATION_NANO_3:
	    case ITDB_IPOD_GENERATION_NANO_4:
	    case ITDB_IPOD_GENERATION_CLASSIC_1:
	    case ITDB_IPOD_GENERATION_CLASSIC_2:
	    case ITDB_IPOD_GENERATION_CLASSIC_3:
		return FALSE;
	    case ITDB_IPOD_GENERATION_NANO_5:
	    case ITDB_IPOD_GENERATION_NANO_6:
	    case ITDB_IPOD_GENERATION_TOUCH_1:
	    case ITDB_IPOD_GENERATION_TOUCH_2:
	    case ITDB_IPOD_GENERATION_TOUCH_3:
	    case ITDB_IPOD_GENERATION_TOUCH_4:
	    case ITDB_IPOD_GENERATION_IPHONE_1:
	    case ITDB_IPOD_GENERATION_IPHONE_2:
	    case ITDB_IPOD_GENERATION_IPHONE_3:
	    case ITDB_IPOD_GENERATION_IPHONE_4:
	    case ITDB_IPOD_GENERATION_IPAD_1:
		/* FIXME: needs to check firmware version */
		return TRUE;
	}
	g_return_val_if_reached (FALSE);
    }
}

G_GNUC_INTERNAL gboolean
itdb_device_supports_compressed_itunesdb (const Itdb_Device *device)
{
    return itdb_device_supports_sqlite_db (device);
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
            case ITDB_IPOD_GENERATION_SHUFFLE_4:
            case ITDB_IPOD_GENERATION_NANO_1:
            case ITDB_IPOD_GENERATION_NANO_2:
            case ITDB_IPOD_GENERATION_VIDEO_1:
            case ITDB_IPOD_GENERATION_VIDEO_2:
                supports_sparse_artwork = FALSE;
                break;
            case ITDB_IPOD_GENERATION_NANO_3:
            case ITDB_IPOD_GENERATION_NANO_4:
            case ITDB_IPOD_GENERATION_NANO_5:
            case ITDB_IPOD_GENERATION_NANO_6:
            case ITDB_IPOD_GENERATION_CLASSIC_1:
            case ITDB_IPOD_GENERATION_CLASSIC_2:
            case ITDB_IPOD_GENERATION_CLASSIC_3:
            case ITDB_IPOD_GENERATION_TOUCH_1:
            case ITDB_IPOD_GENERATION_TOUCH_2:
            case ITDB_IPOD_GENERATION_TOUCH_3:
            case ITDB_IPOD_GENERATION_TOUCH_4:
            case ITDB_IPOD_GENERATION_IPHONE_1:
            case ITDB_IPOD_GENERATION_IPHONE_2:
            case ITDB_IPOD_GENERATION_IPHONE_3:
            case ITDB_IPOD_GENERATION_IPHONE_4:
            case ITDB_IPOD_GENERATION_IPAD_1:
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

/*
 * endianess_check_path:
 * @path:   the file to look at.
 * @hdr:    the header string (4 bytes) in case of LITTLE_ENDIAN
 *
 * Check if endianess can be determined by looking at header of @path.
 *
 * Returns: G_LITTLE_ENDIAN, G_BIG_ENDIAN or 0 if endianess could not be
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
 * which is an array of #Itdb_IpodInfo entries. This can be useful if you
 * want to build a list of all iPod models known to the current libgpod.
 *
 * Returns: a pointer to the array of #Itdb_IpodInfo entries.
 *
 * Since: 0.4.0
 */
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
 * Returns: a pointer to the model name. This is a static string
 * and must not be g_free()d.
 *
 * Since: 0.4.0
 */
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
 * Returns: a pointer to the generation name. This is a static
 * string and must not be g_free()d.
 *
 * Since: 0.4.0
 */
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
 * Returns: true if @device can display artwork.
 *
 * Since: 0.5.0
 */
gboolean itdb_device_supports_artwork (const Itdb_Device *device)
{
    GList *formats;
    if (device == NULL) {
        return FALSE;
    }
    formats = itdb_device_get_cover_art_formats (device);
    g_list_free (formats);
    return (formats != NULL);
}


/**
 * itdb_device_supports_video:
 * @device: an #Itdb_Device
 *
 * Indicates whether @device can play videos or not.
 *
 * Returns: true if @device can play videos.
 *
 * Since: 0.7.0
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
        case ITDB_IPOD_GENERATION_SHUFFLE_4:
        case ITDB_IPOD_GENERATION_NANO_1:
        case ITDB_IPOD_GENERATION_NANO_2:
        case ITDB_IPOD_GENERATION_NANO_6:
            return FALSE;
        case ITDB_IPOD_GENERATION_NANO_3:
        case ITDB_IPOD_GENERATION_NANO_4:
        case ITDB_IPOD_GENERATION_NANO_5:
        case ITDB_IPOD_GENERATION_VIDEO_1:
        case ITDB_IPOD_GENERATION_VIDEO_2:
        case ITDB_IPOD_GENERATION_CLASSIC_1:
	case ITDB_IPOD_GENERATION_CLASSIC_2:
	case ITDB_IPOD_GENERATION_CLASSIC_3:
	case ITDB_IPOD_GENERATION_TOUCH_1:
	case ITDB_IPOD_GENERATION_TOUCH_2:
	case ITDB_IPOD_GENERATION_TOUCH_3:
	case ITDB_IPOD_GENERATION_TOUCH_4:
	case ITDB_IPOD_GENERATION_IPHONE_1:
	case ITDB_IPOD_GENERATION_IPHONE_2:
	case ITDB_IPOD_GENERATION_IPHONE_3:
	case ITDB_IPOD_GENERATION_IPHONE_4:
	case ITDB_IPOD_GENERATION_IPAD_1:
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
 * Returns: true if @device can display photos.
 *
 * Since: 0.5.0
 */
gboolean itdb_device_supports_photo (const Itdb_Device *device)
{
    GList *formats;
    if (device == NULL) {
        return FALSE;
    }
    formats = itdb_device_get_photo_formats (device);
    g_list_free (formats);
    return (formats != NULL);
}

/**
 * itdb_device_supports_chapter_image:
 * @device: an #Itdb_Device
 *
 * Indicates whether @device can display chapter images or not.
 *
 * Return value: true if @device can display chapter images.
 *
 * Since: 0.7.2
 */
gboolean itdb_device_supports_chapter_image (const Itdb_Device *device)
{
    GList *formats;
    if (device == NULL) {
        return FALSE;
    }
    formats = itdb_device_get_chapter_image_formats (device);
    g_list_free (formats);
    return (formats != NULL);
}

/**
 * itdb_device_get_firewire_id
 * @device: an #Itdb_Device
 *
 * Returns the Firewire ID for @device, this is useful to calculate the 
 * iTunesDB checksum which is expected by newer iPod models
 * (iPod classic/fat nanos) and is needed on iPod plugged through USB contrary
 * to what the name implies.
 *
 * Returns: a string containing the firewire id in hexadecimal
 * or NULL if we don't know it.
 *
 * Since: 0.6.0
 */
const char *itdb_device_get_firewire_id (const Itdb_Device *device)
{
    if (device->sysinfo_extended != NULL) {
	return itdb_sysinfo_properties_get_firewire_id (device->sysinfo_extended);
    }

    return g_hash_table_lookup (device->sysinfo, "FirewireGuid");
}

ItdbChecksumType itdb_device_get_checksum_type (const Itdb_Device *device)
{

    if (device == NULL) {
        return ITDB_CHECKSUM_NONE;
    }
    
    if (device->sysinfo_extended != NULL) {
	gint db_version;
	db_version = itdb_sysinfo_properties_get_db_version (device->sysinfo_extended);
	switch (db_version) {
	    case 0:
	    case 1:
	    case 2:
		return ITDB_CHECKSUM_NONE;
	    case 3:
		return ITDB_CHECKSUM_HASH58;
	    case 4:
		return ITDB_CHECKSUM_HASH72;
	    case 5:
		return ITDB_CHECKSUM_HASHAB;
	    default:
		return ITDB_CHECKSUM_UNKNOWN;
	}
    } else {
	const Itdb_IpodInfo *info;
	info = itdb_device_get_ipod_info (device);
	if (info == NULL) {
	    return ITDB_CHECKSUM_NONE;
	}

	switch (info->ipod_generation) {
	    case ITDB_IPOD_GENERATION_CLASSIC_1: 
	    case ITDB_IPOD_GENERATION_CLASSIC_2:
	    case ITDB_IPOD_GENERATION_CLASSIC_3:
	    case ITDB_IPOD_GENERATION_NANO_3:
	    case ITDB_IPOD_GENERATION_NANO_4:
		return ITDB_CHECKSUM_HASH58;

	    case ITDB_IPOD_GENERATION_NANO_5:
	    case ITDB_IPOD_GENERATION_TOUCH_1:
	    case ITDB_IPOD_GENERATION_TOUCH_2:
	    case ITDB_IPOD_GENERATION_TOUCH_3:
	    case ITDB_IPOD_GENERATION_IPHONE_1:
	    case ITDB_IPOD_GENERATION_IPHONE_2:
	    case ITDB_IPOD_GENERATION_IPHONE_3:
		return ITDB_CHECKSUM_HASH72;

	    case ITDB_IPOD_GENERATION_IPAD_1:
	    case ITDB_IPOD_GENERATION_IPHONE_4:
	    case ITDB_IPOD_GENERATION_TOUCH_4:
	    case ITDB_IPOD_GENERATION_NANO_6:
		return ITDB_CHECKSUM_HASHAB;

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
	    case ITDB_IPOD_GENERATION_SHUFFLE_4:
	    case ITDB_IPOD_GENERATION_NANO_1:
	    case ITDB_IPOD_GENERATION_NANO_2:
	    case ITDB_IPOD_GENERATION_VIDEO_1:
	    case ITDB_IPOD_GENERATION_VIDEO_2:
		return ITDB_CHECKSUM_NONE;
	}
    }

    return ITDB_CHECKSUM_NONE;
}

G_GNUC_INTERNAL gboolean itdb_device_write_checksum (Itdb_Device *device, 
						     unsigned char *itdb_data, 
						     gsize itdb_len,
						     GError **error)
{
    switch (itdb_device_get_checksum_type (device)) {
	case ITDB_CHECKSUM_NONE:
	    return TRUE;
	case ITDB_CHECKSUM_HASH58:
	    return itdb_hash58_write_hash (device, itdb_data, itdb_len, error);
	case ITDB_CHECKSUM_HASH72:
	    return itdb_hash72_write_hash (device, itdb_data, itdb_len, error);
        case ITDB_CHECKSUM_HASHAB:
	case ITDB_CHECKSUM_UNKNOWN:
            g_set_error (error, 0, -1, "Unsupported checksum type");
	    return FALSE;
    }	
    g_assert_not_reached ();
}

#ifdef WIN32
#include <windows.h>
#else
#include <sys/statvfs.h>
#endif

/**
 * itdb_device_get_storage_info:
 *
 * @device:   an #Itdb_Device
 * @capacity: returned capacity in bytes
 * @free:     returned free space in bytes
 *
 * Return the storage info for this iPod
 *
 * Returns: TRUE if storage info could be obtained, FALSE otherwise
 */
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

struct IpodModelTable {
    GHashTable *serial_hash;
    GHashTable *model_number_hash;
};

static struct IpodModelTable *init_ipod_model_table (void)
{
    const Itdb_IpodInfo *it;
    struct IpodModelTable *model_table;
    const ItdbSerialToModel *serial_it;

    model_table = g_new0 (struct IpodModelTable, 1);
    model_table->serial_hash = g_hash_table_new (g_str_hash, g_str_equal);
    model_table->model_number_hash = g_hash_table_new (g_str_hash, g_str_equal);

    for (it = itdb_info_get_ipod_info_table (); 
         it->model_number != NULL; 
         it++) {
        g_hash_table_insert (model_table->model_number_hash, 
                             (gpointer)it->model_number, (gpointer)it);
    }

    for (serial_it = serial_to_model_mapping; 
         serial_it->serial != NULL; 
         serial_it++) {
        gpointer model_info;
        model_info = g_hash_table_lookup (model_table->model_number_hash,
                                          serial_it->model_number);
        if (model_info != NULL) {
            g_hash_table_insert (model_table->serial_hash, 
                                 (gpointer)serial_it->serial, model_info);
        } else {
            g_warning ("Inconsistent ipod model tables, model info is missing for %s (serial %s)", 
                       serial_it->model_number, serial_it->serial);
        }
    }

    return model_table;
}

static struct IpodModelTable *get_model_table (void)
{
    static GOnce my_once = G_ONCE_INIT;

    g_once (&my_once, (GThreadFunc)init_ipod_model_table, NULL);

    return my_once.retval;
}

static const Itdb_IpodInfo *
get_ipod_info_from_model_number (const char *model_number)
{
    struct IpodModelTable *model_table;

    g_return_val_if_fail (model_number != NULL, NULL);

    model_table = get_model_table ();
    if(isalpha(model_number[0])) {
	model_number++;
    }
    return g_hash_table_lookup (model_table->model_number_hash, model_number);
}

const Itdb_IpodInfo *
itdb_ipod_info_from_serial (const char *serial)
{
    struct IpodModelTable *model_table;
    glong len;

    g_return_val_if_fail (serial != NULL, NULL);

    len = strlen (serial);
    if (len < 3) {
        return NULL;
    }
    model_table = get_model_table ();
    /* The 3 last letters of the ipod serial number can be used to identify 
     * the model
     */
    return g_hash_table_lookup (model_table->serial_hash, serial+len-3);
}

GQuark itdb_device_error_quark (void)
{
    static GQuark quark = 0;
    if (!quark) {
        quark = g_quark_from_static_string ("itdb-device-error-quark");
    }
    return quark;
}

/**
 * itdb_device_supports_podcast:
 * @device: an #Itdb_Device
 *
 * Indicates whether @device can play podcasts or not.
 *
 * Returns: true if @device can play podcasts.
 *
 * Since: 0.7.2
 */
gboolean
itdb_device_supports_podcast (const Itdb_Device *device)
{
    if (device->sysinfo_extended != NULL) {
    	return itdb_sysinfo_properties_supports_podcast (device->sysinfo_extended);
    } else {
	const Itdb_IpodInfo *info;

	info = itdb_device_get_ipod_info (device);
	switch (info->ipod_generation) {
	    case ITDB_IPOD_GENERATION_UNKNOWN:
	    case ITDB_IPOD_GENERATION_FIRST:
	    case ITDB_IPOD_GENERATION_SECOND:
	    case ITDB_IPOD_GENERATION_THIRD:
	    case ITDB_IPOD_GENERATION_MOBILE:
		return FALSE;
	    case ITDB_IPOD_GENERATION_FOURTH:
	    case ITDB_IPOD_GENERATION_PHOTO:
	    case ITDB_IPOD_GENERATION_MINI_1:
	    case ITDB_IPOD_GENERATION_MINI_2:
	    case ITDB_IPOD_GENERATION_NANO_1:
	    case ITDB_IPOD_GENERATION_NANO_2:
	    case ITDB_IPOD_GENERATION_NANO_3:
	    case ITDB_IPOD_GENERATION_NANO_4:
	    case ITDB_IPOD_GENERATION_NANO_5:
	    case ITDB_IPOD_GENERATION_NANO_6:
	    case ITDB_IPOD_GENERATION_SHUFFLE_1:
	    case ITDB_IPOD_GENERATION_SHUFFLE_2:
	    case ITDB_IPOD_GENERATION_SHUFFLE_3:
	    case ITDB_IPOD_GENERATION_SHUFFLE_4:
	    case ITDB_IPOD_GENERATION_VIDEO_1:
	    case ITDB_IPOD_GENERATION_VIDEO_2:
	    case ITDB_IPOD_GENERATION_CLASSIC_1:
	    case ITDB_IPOD_GENERATION_CLASSIC_2:
	    case ITDB_IPOD_GENERATION_CLASSIC_3:
	    case ITDB_IPOD_GENERATION_TOUCH_1:
	    case ITDB_IPOD_GENERATION_TOUCH_2:
	    case ITDB_IPOD_GENERATION_TOUCH_3:
	    case ITDB_IPOD_GENERATION_TOUCH_4:
	    case ITDB_IPOD_GENERATION_IPHONE_1:
	    case ITDB_IPOD_GENERATION_IPHONE_2:
	    case ITDB_IPOD_GENERATION_IPHONE_3:
	    case ITDB_IPOD_GENERATION_IPHONE_4:
	    case ITDB_IPOD_GENERATION_IPAD_1:
		return TRUE;
	}
	g_return_val_if_reached (FALSE);
    }
}

gchar *itdb_device_get_uuid(const Itdb_Device *device)
{
    return g_hash_table_lookup (device->sysinfo, "FirewireGuid");
}

gboolean itdb_device_is_shuffle (const Itdb_Device *device)
{
    const Itdb_IpodInfo *info;

    info = itdb_device_get_ipod_info (device);
    switch (info->ipod_generation) {
        case ITDB_IPOD_GENERATION_UNKNOWN:
        case ITDB_IPOD_GENERATION_FIRST:
        case ITDB_IPOD_GENERATION_SECOND:
        case ITDB_IPOD_GENERATION_THIRD:
        case ITDB_IPOD_GENERATION_MOBILE:
        case ITDB_IPOD_GENERATION_FOURTH:
        case ITDB_IPOD_GENERATION_PHOTO:
        case ITDB_IPOD_GENERATION_MINI_1:
        case ITDB_IPOD_GENERATION_MINI_2:
        case ITDB_IPOD_GENERATION_NANO_1:
        case ITDB_IPOD_GENERATION_NANO_2:
        case ITDB_IPOD_GENERATION_NANO_3:
        case ITDB_IPOD_GENERATION_NANO_4:
        case ITDB_IPOD_GENERATION_NANO_5:
        case ITDB_IPOD_GENERATION_NANO_6:
        case ITDB_IPOD_GENERATION_VIDEO_1:
        case ITDB_IPOD_GENERATION_VIDEO_2:
        case ITDB_IPOD_GENERATION_CLASSIC_1:
        case ITDB_IPOD_GENERATION_CLASSIC_2:
        case ITDB_IPOD_GENERATION_CLASSIC_3:
        case ITDB_IPOD_GENERATION_TOUCH_1:
        case ITDB_IPOD_GENERATION_TOUCH_2:
        case ITDB_IPOD_GENERATION_TOUCH_3:
        case ITDB_IPOD_GENERATION_TOUCH_4:
        case ITDB_IPOD_GENERATION_IPHONE_1:
        case ITDB_IPOD_GENERATION_IPHONE_2:
        case ITDB_IPOD_GENERATION_IPHONE_3:
        case ITDB_IPOD_GENERATION_IPHONE_4:
        case ITDB_IPOD_GENERATION_IPAD_1:
            return FALSE;
        case ITDB_IPOD_GENERATION_SHUFFLE_1:
        case ITDB_IPOD_GENERATION_SHUFFLE_2:
        case ITDB_IPOD_GENERATION_SHUFFLE_3:
        case ITDB_IPOD_GENERATION_SHUFFLE_4:
            return TRUE;
    }
    g_return_val_if_reached (FALSE);
}

gboolean itdb_device_is_iphone_family (const Itdb_Device *device)
{
    if (device->sysinfo_extended != NULL) {
    	return (itdb_sysinfo_properties_get_family_id (device->sysinfo_extended) >= 10000);
    } else {
	const Itdb_IpodInfo *info;

	info = itdb_device_get_ipod_info (device);
	switch (info->ipod_generation) {
	    case ITDB_IPOD_GENERATION_UNKNOWN:
	    case ITDB_IPOD_GENERATION_FIRST:
	    case ITDB_IPOD_GENERATION_SECOND:
	    case ITDB_IPOD_GENERATION_THIRD:
	    case ITDB_IPOD_GENERATION_MOBILE:
	    case ITDB_IPOD_GENERATION_FOURTH:
	    case ITDB_IPOD_GENERATION_PHOTO:
	    case ITDB_IPOD_GENERATION_MINI_1:
	    case ITDB_IPOD_GENERATION_MINI_2:
	    case ITDB_IPOD_GENERATION_NANO_1:
	    case ITDB_IPOD_GENERATION_NANO_2:
	    case ITDB_IPOD_GENERATION_NANO_3:
	    case ITDB_IPOD_GENERATION_NANO_4:
	    case ITDB_IPOD_GENERATION_NANO_5:
	    case ITDB_IPOD_GENERATION_NANO_6:
	    case ITDB_IPOD_GENERATION_SHUFFLE_1:
	    case ITDB_IPOD_GENERATION_SHUFFLE_2:
	    case ITDB_IPOD_GENERATION_SHUFFLE_3:
	    case ITDB_IPOD_GENERATION_SHUFFLE_4:
	    case ITDB_IPOD_GENERATION_VIDEO_1:
	    case ITDB_IPOD_GENERATION_VIDEO_2:
	    case ITDB_IPOD_GENERATION_CLASSIC_1:
	    case ITDB_IPOD_GENERATION_CLASSIC_2:
	    case ITDB_IPOD_GENERATION_CLASSIC_3:
		return FALSE;
	    case ITDB_IPOD_GENERATION_TOUCH_1:
	    case ITDB_IPOD_GENERATION_TOUCH_2:
	    case ITDB_IPOD_GENERATION_TOUCH_3:
	    case ITDB_IPOD_GENERATION_TOUCH_4:
	    case ITDB_IPOD_GENERATION_IPHONE_1:
	    case ITDB_IPOD_GENERATION_IPHONE_2:
	    case ITDB_IPOD_GENERATION_IPHONE_3:
	    case ITDB_IPOD_GENERATION_IPHONE_4:
	    case ITDB_IPOD_GENERATION_IPAD_1:
		return TRUE;
	}
	g_return_val_if_reached (FALSE);
    }
}

enum ItdbShadowDBVersion itdb_device_get_shadowdb_version (const Itdb_Device *device)
{
    int version;

    version = 0;
    if (device->sysinfo_extended != NULL) {
	version = itdb_sysinfo_properties_get_shadow_db_version (device->sysinfo_extended);
    }

    if (version == 0) {
	const Itdb_IpodInfo *info;

	info = itdb_device_get_ipod_info (device);
	switch (info->ipod_generation) {
	    case ITDB_IPOD_GENERATION_UNKNOWN:
	    case ITDB_IPOD_GENERATION_FIRST:
	    case ITDB_IPOD_GENERATION_SECOND:
	    case ITDB_IPOD_GENERATION_THIRD:
	    case ITDB_IPOD_GENERATION_MOBILE:
	    case ITDB_IPOD_GENERATION_FOURTH:
	    case ITDB_IPOD_GENERATION_PHOTO:
	    case ITDB_IPOD_GENERATION_MINI_1:
	    case ITDB_IPOD_GENERATION_MINI_2:
	    case ITDB_IPOD_GENERATION_NANO_1:
	    case ITDB_IPOD_GENERATION_NANO_2:
	    case ITDB_IPOD_GENERATION_NANO_3:
	    case ITDB_IPOD_GENERATION_NANO_4:
	    case ITDB_IPOD_GENERATION_NANO_5:
	    case ITDB_IPOD_GENERATION_NANO_6:
	    case ITDB_IPOD_GENERATION_VIDEO_1:
	    case ITDB_IPOD_GENERATION_VIDEO_2:
	    case ITDB_IPOD_GENERATION_CLASSIC_1:
	    case ITDB_IPOD_GENERATION_CLASSIC_2:
	    case ITDB_IPOD_GENERATION_CLASSIC_3:
	    case ITDB_IPOD_GENERATION_TOUCH_1:
	    case ITDB_IPOD_GENERATION_TOUCH_2:
	    case ITDB_IPOD_GENERATION_TOUCH_3:
	    case ITDB_IPOD_GENERATION_TOUCH_4:
	    case ITDB_IPOD_GENERATION_IPHONE_1:
	    case ITDB_IPOD_GENERATION_IPHONE_2:
	    case ITDB_IPOD_GENERATION_IPHONE_3:
	    case ITDB_IPOD_GENERATION_IPHONE_4:
	    case ITDB_IPOD_GENERATION_IPAD_1:
		version = ITDB_SHADOW_DB_UNKNOWN;
		break;
	    case ITDB_IPOD_GENERATION_SHUFFLE_1:
	    case ITDB_IPOD_GENERATION_SHUFFLE_2:
		version = ITDB_SHADOW_DB_V1;
		break;
	    case ITDB_IPOD_GENERATION_SHUFFLE_3:
	    case ITDB_IPOD_GENERATION_SHUFFLE_4:
		version = ITDB_SHADOW_DB_V2;
		break;
	}
    }

    return version;
}

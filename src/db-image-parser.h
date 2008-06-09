/*
 *  Copyright (C) 2005 Christophe Fergeau
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
 *  $Id$
 */

#ifndef IMAGE_PARSER_H
#define IMAGE_PARSER_H

#include "db-itunes-parser.h"
#include "itdb_device.h"
#include "itdb_private.h"
#include "itdb_thumb.h"
#include "itdb.h"

#define DEBUG_ARTWORK 0

#define RED_BITS_565   5
#define RED_SHIFT_565 11
#define RED_MASK_565  (((1 << RED_BITS_565)-1) << RED_SHIFT_565)

#define GREEN_BITS_565 6
#define GREEN_SHIFT_565 5
#define GREEN_MASK_565 (((1 << GREEN_BITS_565)-1) << GREEN_SHIFT_565)

#define BLUE_BITS_565 5
#define BLUE_SHIFT_565 0
#define BLUE_MASK_565 (((1 << BLUE_BITS_565)-1) << BLUE_SHIFT_565)

#define ALPHA_BITS_555 1
#define ALPHA_SHIFT_555 15
#define ALPHA_MASK_555  (((1 << ALPHA_BITS_555)-1) << ALPHA_SHIFT_555)

#define RED_BITS_555   5
#define RED_SHIFT_555 10
#define RED_MASK_555  (((1 << RED_BITS_555)-1) << RED_SHIFT_555)

#define GREEN_BITS_555 5
#define GREEN_SHIFT_555 5
#define GREEN_MASK_555 (((1 << GREEN_BITS_555)-1) << GREEN_SHIFT_555)

#define BLUE_BITS_555 5
#define BLUE_SHIFT_555 0
#define BLUE_MASK_555 (((1 << BLUE_BITS_555)-1) << BLUE_SHIFT_555)

#define ALPHA_BITS_888 8
#define ALPHA_SHIFT_888 24
#define ALPHA_MASK_888  (((1 << ALPHA_BITS_888)-1) << ALPHA_SHIFT_888)

#define RED_BITS_888   8
#define RED_SHIFT_888 16
#define RED_MASK_888  (((1 << RED_BITS_888)-1) << RED_SHIFT_888)

#define GREEN_BITS_888 8
#define GREEN_SHIFT_888 8
#define GREEN_MASK_888 (((1 << GREEN_BITS_888)-1) << GREEN_SHIFT_888)

#define BLUE_BITS_888 8
#define BLUE_SHIFT_888 0
#define BLUE_MASK_888 (((1 << BLUE_BITS_888)-1) << BLUE_SHIFT_888)

G_GNUC_INTERNAL Itdb_Thumb_Ipod_Item *ipod_image_new_from_mhni (MhniHeader *mhni, 
			        			   Itdb_DB *db);

G_GNUC_INTERNAL int itdb_write_ithumb_files (Itdb_DB *db);
#endif

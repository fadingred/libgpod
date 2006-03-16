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
 */

#ifndef IMAGE_PARSER_H
#define IMAGE_PARSER_H

#include "db-itunes-parser.h"
#include "itdb_device.h"
#include "itdb.h"

#define RED_BITS   5
#define RED_SHIFT 11
#define RED_MASK  (((1 << RED_BITS)-1) << RED_SHIFT)

#define GREEN_BITS 6
#define GREEN_SHIFT 5
#define GREEN_MASK (((1 << GREEN_BITS)-1) << GREEN_SHIFT)

#define BLUE_BITS 5
#define BLUE_SHIFT 0
#define BLUE_MASK (((1 << BLUE_BITS)-1) << BLUE_SHIFT)

G_GNUC_INTERNAL Itdb_Thumb *ipod_image_new_from_mhni (MhniHeader *mhni, 
						      Itdb_iTunesDB *db);

G_GNUC_INTERNAL int itdb_write_ithumb_files (Itdb_iTunesDB *db);

G_GNUC_INTERNAL const Itdb_ArtworkFormat *itdb_get_artwork_info_from_type (
    Itdb_Device *ipod, int image_type);

#endif

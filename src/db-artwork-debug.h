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
#ifndef DB_ARTWORK_DEBUG_H
#define DB_ARTWORK_DEBUG_H

#include "db-itunes-parser.h"

/* #define DEBUG_ARTWORKDB */

#ifdef DEBUG_ARTWORKDB
extern G_GNUC_INTERNAL void dump_mhif (MhifHeader *mhif);
extern G_GNUC_INTERNAL void dump_mhia (MhiaHeader *mhia);
extern G_GNUC_INTERNAL void dump_mhod_string (ArtworkDB_MhodHeaderString *mhod);
extern G_GNUC_INTERNAL void dump_mhni (MhniHeader *mhni);
extern G_GNUC_INTERNAL void dump_mhod (ArtworkDB_MhodHeader *mhod);
extern G_GNUC_INTERNAL void dump_mhii (MhiiHeader *mhii);
extern G_GNUC_INTERNAL void dump_mhl (MhlHeader *mhl, const char *id);
extern G_GNUC_INTERNAL void dump_mhsd (ArtworkDB_MhsdHeader *mhsd);
extern G_GNUC_INTERNAL void dump_mhfd (MhfdHeader *mhfd);
extern G_GNUC_INTERNAL void dump_mhba (MhbaHeader *mhba);
#else 
#define dump_mhif(x)
#define dump_mhia(x)
#define dump_mhod_string(x)
#define dump_mhni(x)
#define dump_mhod(x)
#define dump_mhii(x)
#define dump_mhl(x,y)
#define dump_mhsd(x)
#define dump_mhfd(x)
#define dump_mhba(x)
#endif

#endif

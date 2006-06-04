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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307 USA
 * 
 *  iTunes and iPod are trademarks of Apple
 * 
 *  This product is not supported/written/published by Apple!
 *
 *  $Id$
 */

#ifndef __ITDB_ENDIANNESS_H__
#define __ITDB_ENDIANNESS_H__

#include <glib.h>
#include "itdb.h"
#include "itdb_device.h"
#include "itdb_private.h"

#define DB_TO_CPU_GET(lower_case_type, upper_case_type) \
    static inline lower_case_type \
    get_##lower_case_type (lower_case_type val, guint byte_order) \
    { \
        if (byte_order == G_BIG_ENDIAN) { \
            return upper_case_type##_FROM_BE (val); \
        } else if (byte_order == G_LITTLE_ENDIAN) { \
            return upper_case_type##_FROM_LE (val); \
	} else { \
  	    g_assert_not_reached ();  \
	} \
        return 0;  /* never reached */ \
    }
/*
#define DB_TO_CPU_GET_DB(lower_case_type, upper_case_type) \
    DB_TO_CPU_GET(lower_case_type, upper_case_type) \
    static inline lower_case_type \
    get_##lower_case_type##_db (Itdb_iTunesDB *db, lower_case_type val) \
    { \
        g_assert (db->device != NULL); \
        return get_##lower_case_type (val, db->device->byte_order); \
}
*/

#define DB_TO_CPU_GET_DB(lower_case_type, upper_case_type) \
    DB_TO_CPU_GET(lower_case_type, upper_case_type) \
    static inline lower_case_type \
    get_##lower_case_type##_db (Itdb_DB *db, lower_case_type val) \
    { \
	g_assert (db_get_device(db) != NULL); \
	return get_##lower_case_type (val, db_get_device(db)->byte_order); \
    }

DB_TO_CPU_GET_DB(guint32, GUINT32)
DB_TO_CPU_GET_DB(gint32, GINT32)
DB_TO_CPU_GET_DB(gint16, GINT16)
DB_TO_CPU_GET_DB(gint64, GINT64)

/* OK, for 'normal' people an effective summary of what this file is
 * doing:
 *
 * The following inline functions are defined:
 *
 * guint32 get_guint32 (guint32 val, guint byte_order);
 * guint32 get_gint32 (gint32 val, guint byte_order);
 * guint32 get_gint16 (gint16 val, guint byte_order);
 * guint32 get_gint64 (gint64 val, guint byte_order);
 * guint32 get_guint32_db (guint32 val, guint byte_order);
 * guint32 get_gint32_db (gint32 val, guint byte_order);
 * guint32 get_gint16_db (gint16 val, guint byte_order);
 * guint32 get_gint64_db (gint64 val, guint byte_order);
 *

 * They are used to retrieve integer data from or store integer data
 * to the iPod's databases which may use a different byte order
 * (@byte_order) than the host system on which this library runs on.
 */
#endif

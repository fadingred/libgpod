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


#ifndef DB_PARSE_CONTEXT
#define DB_PARSE_CONTEXT

#include <sys/types.h>
#include "itdb.h"
#include "itdb_private.h"

struct _DBParseContext {
	const unsigned char *buffer;
	const unsigned char *cur_pos;
	off_t header_len;
	off_t total_len;
	guint byte_order;
	Itdb_DB *db;
	GMappedFile *mapped_file;
	GList **artwork;
};

typedef struct _DBParseContext DBParseContext;


#define db_parse_context_get_m_header(ctx, type, id) (type *)db_parse_context_get_m_header_internal (ctx, id, sizeof (type))

G_GNUC_INTERNAL DBParseContext *
db_parse_context_new (const unsigned char *buffer, 
		      off_t len, guint byte_order);

G_GNUC_INTERNAL void 
db_parse_context_set_total_len (DBParseContext *ctx, off_t len);

G_GNUC_INTERNAL off_t 
db_parse_context_get_remaining_length (DBParseContext *ctx);

G_GNUC_INTERNAL DBParseContext *
db_parse_context_get_sub_context (DBParseContext *ctx, off_t offset);


G_GNUC_INTERNAL DBParseContext *
db_parse_context_get_next_child (DBParseContext *ctx);


G_GNUC_INTERNAL void *
db_parse_context_get_m_header_internal (DBParseContext *ctx, 
					const char *id, off_t size);

G_GNUC_INTERNAL DBParseContext *
db_parse_context_new_from_file (const char *filename, Itdb_DB *db);


G_GNUC_INTERNAL void 
db_parse_context_destroy (DBParseContext *ctx);

#endif

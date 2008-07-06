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
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <glib.h>
#include <glib/gstdio.h>
#include "db-parse-context.h"
#include "db-itunes-parser.h"
#include "itdb_endianness.h"

DBParseContext *
db_parse_context_new (const unsigned char *buffer, off_t len, guint byte_order)
{
	DBParseContext *result;

	result = g_new0 (DBParseContext, 1);
	if (result == NULL) {
		return NULL;
	}

	result->buffer = buffer;
	result->cur_pos = buffer;
	result->total_len = len;
	result->byte_order = byte_order;

	return result;
}

void
db_parse_context_destroy (DBParseContext *ctx) 
{
	g_return_if_fail (ctx != NULL);

	if (ctx->mapped_file) {
		g_mapped_file_free(ctx->mapped_file);
	}

	g_free (ctx);
}

static void
db_parse_context_set_header_len (DBParseContext *ctx, off_t len)
{
	/* FIXME: this can probably happen in malformed itunesdb files, 
	 * don't g_assert on this, only output a warning 
	 */
	g_assert ((ctx->cur_pos - ctx->buffer) <= len);
	g_assert (len <= ctx->total_len);
	ctx->header_len = len;
}

void
db_parse_context_set_total_len (DBParseContext *ctx, off_t len)
{
	/* FIXME: this can probably happen in malformed itunesdb files, 
	 * don't g_assert on this, only output a warning 
	 */
	g_assert ((ctx->cur_pos - ctx->buffer) <= len);
	if (ctx->header_len != 0) {
		g_assert (len >= ctx->header_len);
	}
	ctx->total_len = len;
}


off_t
db_parse_context_get_remaining_length (DBParseContext *ctx)
{
	if (ctx->header_len != 0) {
		return ctx->header_len - (ctx->cur_pos - ctx->buffer); 
	} else {
		return ctx->total_len - (ctx->cur_pos - ctx->buffer); 
	}
}

DBParseContext *
db_parse_context_get_sub_context (DBParseContext *ctx, off_t offset)
{
	DBParseContext *sub_ctx;

	if (offset >= ctx->total_len) {
		return NULL;
	}
	sub_ctx = db_parse_context_new (&ctx->buffer[offset], 
				     ctx->total_len - offset, 
				     ctx->byte_order);
	sub_ctx->db = ctx->db;
	sub_ctx->artwork = ctx->artwork;
	return sub_ctx;
}


DBParseContext *
db_parse_context_get_next_child (DBParseContext *ctx)
{
	if (ctx->header_len == 0) {
		return NULL;
	}
	if (ctx->header_len >= ctx->total_len) {
		return NULL;
	}

	return db_parse_context_get_sub_context (ctx, ctx->header_len);
}

void *
db_parse_context_get_m_header_internal (DBParseContext *ctx, const char *id, off_t size) 
{
	MHeader *h;
	char *header_id;

	if (db_parse_context_get_remaining_length (ctx) < 8) {
		return NULL;
	}

	h = (MHeader *)ctx->cur_pos;
	header_id = g_strndup ((char *)h->header_id, 4);
	if (ctx->byte_order == G_BIG_ENDIAN) {
		g_strreverse (header_id);
	}
	if (strncmp (id, header_id, 4) != 0) {
	        g_free (header_id);
		return NULL;
	}

	g_free (header_id);

	/* FIXME: this test sucks for compat: if a field is smaller than 
	 * expected, we probably should create a buffer of the appropriate 
	 * size inited to 0, copy the data that is available in it and use
	 * that buffer in the rest of the code (maybe it's harmful to have
	 * some fields at 0 in some headers though...)
	 */
	if (get_gint32 (h->header_len, ctx->byte_order) < size) {
		return NULL;
	}

	db_parse_context_set_header_len (ctx, get_gint32 (h->header_len, 
							  ctx->byte_order));

	return h;
}

DBParseContext *
db_parse_context_new_from_file (const char *filename, Itdb_DB *db)
{
	DBParseContext *ctx;
	Itdb_Device *device;
	GError* error;
	GMappedFile* mapped_file;
	struct stat stat_buf;

	ctx = NULL;
	error = NULL;
	mapped_file = NULL;

	device = db_get_device (db);
	g_return_val_if_fail (device, NULL);

	if (g_stat (filename, &stat_buf) != 0) {
		return NULL;	
	};
	if (stat_buf.st_size > 64 * 1024 * 1024) {
		g_warning ("%s is too big to be mmapped (%llu bytes)\n",
			   filename, (unsigned long long)stat_buf.st_size);
		return NULL;
	}

	mapped_file = g_mapped_file_new(filename, FALSE, &error);
	
	if (mapped_file == NULL) {
		g_print ("Error while mapping %s: %s\n", filename, 
                    error->message);
		g_error_free(error);
		return NULL;
	}

	if (device->byte_order == 0)
	    itdb_device_autodetect_endianess (device);

	ctx = db_parse_context_new ((guchar *)g_mapped_file_get_contents(mapped_file),
					g_mapped_file_get_length(mapped_file), 
					device->byte_order);

	if (ctx == NULL) {
		g_mapped_file_free(mapped_file);
		return NULL;
	}
	ctx->db = db;
	ctx->mapped_file = mapped_file;

        return ctx;
}

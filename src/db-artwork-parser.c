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

#include "itdb.h"
#include "db-artwork-debug.h"
#include "db-artwork-parser.h"
#include "db-image-parser.h"
#include "db-itunes-parser.h"
#include "db-parse-context.h"

typedef int (*ParseListItem)(DBParseContext *ctx, Itdb_iTunesDB *db, GError *error);

#ifndef DEBUG_ARTWORKDB
static Itdb_Track *
get_song_by_dbid (Itdb_iTunesDB *db, guint64 id)
{
	GList *it;

	for (it = db->tracks; it != NULL; it = it->next) {
		Itdb_Track *song;

		song = (Itdb_Track*)it->data;
		if (song->dbid == id) {
			return song;
		}
	}
	return NULL;
}
#endif


static int
parse_mhif (DBParseContext *ctx, Itdb_iTunesDB *db, GError *error)
{
	MhifHeader *mhif;

	mhif = db_parse_context_get_m_header (ctx, MhifHeader, "mhif");
	if (mhif == NULL) {
		return -1;
	}
	dump_mhif (mhif);
	db_parse_context_set_total_len (ctx, GINT_FROM_LE (mhif->total_len));
	return 0;
}

static int
parse_mhia (DBParseContext *ctx, Itdb_iTunesDB *db, GError *error)
{
	MhiaHeader *mhia;

	mhia = db_parse_context_get_m_header (ctx, MhiaHeader, "mhia");
	if (mhia == NULL) {
		return -1;
	}
	dump_mhia (mhia);
	db_parse_context_set_total_len (ctx, GINT_FROM_LE (mhia->total_len));
	return 0;
}

#ifdef DEBUG_ARTWORKDB
static int
parse_mhod_3 (DBParseContext *ctx, GError *error)
{
	MhodHeader *mhod;
	MhodHeaderArtworkType3 *mhod3;

	mhod = db_parse_context_get_m_header (ctx, MhodHeader, "mhod");
	if (mhod == NULL) {
		return -1;
	}
	db_parse_context_set_total_len (ctx, GINT_FROM_LE (mhod->total_len));
	
	if (GINT_FROM_LE (mhod->total_len) < sizeof (MhodHeaderArtworkType3)){
		return -1;
	}
	mhod3 = (MhodHeaderArtworkType3*)mhod;
	if ((GINT_FROM_LE (mhod3->type) & 0x00FFFFFF) != MHOD_ARTWORK_TYPE_FILE_NAME) {
		return -1;
	}

	dump_mhod_type_3 (mhod3);
	return 0;
}
#endif

static int
parse_mhni (DBParseContext *ctx, iPodSong *song, GError *error)
{
	MhniHeader *mhni;
#ifndef DEBUG_ARTWORKDB
	Itdb_Image *thumb;
#endif

	mhni = db_parse_context_get_m_header (ctx, MhniHeader, "mhni");
	if (mhni == NULL) {
		return -1;
	}
	db_parse_context_set_total_len (ctx, GINT_FROM_LE (mhni->total_len));

	dump_mhni (mhni);
#ifdef DEBUG_ARTWORKDB
	{
		DBParseContext *mhod_ctx; 

		/* No information useful to us in mhod type 3, do not parse
		 * it in non-debug mode 
		 * FIXME: really? it contains the thumbnail file name!
		 * we infer it from the correlation id, but is this
		 * always The Right Thing to do?
		 */
		mhod_ctx = db_parse_context_get_sub_context (ctx, ctx->header_len);
		if (mhod_ctx == NULL) {
			return -1;
		}
		parse_mhod_3 (mhod_ctx, NULL);
		g_free (mhod_ctx);
	}
#else
	thumb = ipod_image_new_from_mhni (mhni, song->itdb->mountpoint);
	if (thumb != NULL) {
		song->thumbnails = g_list_append (song->thumbnails, thumb);
	}
#endif
	return 0;
}

static int
parse_mhod (DBParseContext *ctx, iPodSong *song, GError *error)
{
	MhodHeader *mhod;
	DBParseContext *mhni_ctx;
	int type;

	mhod = db_parse_context_get_m_header (ctx, MhodHeader, "mhod");
	if (mhod == NULL) {
		return -1;
	}
	db_parse_context_set_total_len (ctx, GINT_FROM_LE (mhod->total_len));

	/* The MHODs found in the ArtworkDB and Photo Database files are
	 * significantly different than those found in the iTunesDB.
	 * For example, the "type" is split like this:
	 * - low 3 bytes are actual type;
	 * - high byte is padding length of string (0-3).
	 */
	type = GINT_FROM_LE (mhod->type) & 0x00FFFFFF;
	if (type == MHOD_ARTWORK_TYPE_ALBUM_NAME)
		dump_mhod_type_1 ((MhodHeaderArtworkType1 *)mhod);
	else
		dump_mhod (mhod);

	/* if this is a container... */
	if (type == MHOD_ARTWORK_TYPE_THUMBNAIL) {
		mhni_ctx = db_parse_context_get_sub_context (ctx, ctx->header_len);
		if (mhni_ctx == NULL) {
			return -1;
		}
		parse_mhni (mhni_ctx, song, NULL);
		g_free (mhni_ctx);
	}

	return 0;
}


static int
parse_mhii (DBParseContext *ctx, Itdb_iTunesDB *db, GError *error)
{
	MhiiHeader *mhii;
	DBParseContext *mhod_ctx;
	int num_children;
	off_t cur_offset;
	iPodSong *song;

	mhii = db_parse_context_get_m_header (ctx, MhiiHeader, "mhii");
	if (mhii == NULL) {
		return -1;
	}
	db_parse_context_set_total_len (ctx, GINT_FROM_LE (mhii->total_len));

	dump_mhii (mhii);

#ifdef DEBUG_ARTWORKDB
	song = NULL;
#else
	song = get_song_by_dbid (db, GINT64_FROM_LE (mhii->song_id));
	if (song == NULL) {
		return -1;
	}

	if (song->artwork_size != GINT_FROM_LE (mhii->orig_img_size)-1) {
		g_warning ("iTunesDB and ArtworkDB artwork sizes don't match (%d %d)", song->artwork_size , GINT_FROM_LE (mhii->orig_img_size));
	}

	song->artwork_size = GINT_FROM_LE (mhii->orig_img_size)-1;
	song->image_id = GINT_FROM_LE (mhii->image_id);
#endif

	cur_offset = ctx->header_len;
	mhod_ctx = db_parse_context_get_sub_context (ctx, cur_offset);
	num_children = GINT_FROM_LE (mhii->num_children);
	while ((num_children > 0) && (mhod_ctx != NULL)) {
		parse_mhod (mhod_ctx, song, NULL);
		num_children--;
		cur_offset += mhod_ctx->total_len;
		g_free (mhod_ctx);
		mhod_ctx = db_parse_context_get_sub_context (ctx, cur_offset);
	}

	return 0;
}


static int
parse_mhba (DBParseContext *ctx, Itdb_iTunesDB *db, GError *error)
{
	MhbaHeader *mhba;
	DBParseContext *mhod_ctx;
	DBParseContext *mhia_ctx;
	int num_children;
	off_t cur_offset;

	mhba = db_parse_context_get_m_header (ctx, MhbaHeader, "mhba");
	if (mhba == NULL) {
		return -1;
	}
	db_parse_context_set_total_len (ctx, GINT_FROM_LE (mhba->total_len));

	dump_mhba (mhba);

	cur_offset = ctx->header_len;
	mhod_ctx = db_parse_context_get_sub_context (ctx, cur_offset);
	num_children = GINT_FROM_LE (mhba->num_mhods);
	while ((num_children > 0) && (mhod_ctx != NULL)) {
		parse_mhod (mhod_ctx, NULL, NULL);
		num_children--;
		cur_offset += mhod_ctx->total_len;
		g_free (mhod_ctx);
		mhod_ctx = db_parse_context_get_sub_context (ctx, cur_offset);
	}
	mhia_ctx = db_parse_context_get_sub_context (ctx, cur_offset);
	num_children = GINT_FROM_LE (mhba->num_mhias);
	while ((num_children > 0) && (mhia_ctx != NULL)) {
		parse_mhia (mhia_ctx, db, NULL);
		num_children--;
		cur_offset += mhia_ctx->total_len;
		g_free (mhia_ctx);
		mhia_ctx = db_parse_context_get_sub_context (ctx, cur_offset);
	}

	return 0;
}


static int
parse_mhl (DBParseContext *ctx, Itdb_iTunesDB *db, GError *error, 
	   const char *id, ParseListItem parse_child)
{
	MhlHeader *mhl;
	int num_children;
	DBParseContext *mhi_ctx;
	off_t cur_offset;

	mhl = db_parse_context_get_m_header (ctx, MhlHeader, id);
	if (mhl == NULL) {
		return -1;
	}

	dump_mhl (mhl, id);

	num_children = GINT_FROM_LE (mhl->num_children);
	if (num_children < 0) {
		return -1;
	}

	cur_offset = ctx->header_len;
	mhi_ctx = db_parse_context_get_sub_context (ctx, cur_offset);
	while ((num_children > 0) && (mhi_ctx != NULL)) {
		if (parse_child != NULL) {
			parse_child (mhi_ctx, db, NULL);
		}
		num_children--;
		cur_offset += mhi_ctx->total_len;
		g_free (mhi_ctx);
		mhi_ctx = db_parse_context_get_sub_context (ctx, cur_offset);
	}

	return 0;

}


static int 
parse_mhsd (DBParseContext *ctx, Itdb_iTunesDB *db, GError **error)
{
	MhsdHeader *mhsd;

	mhsd = db_parse_context_get_m_header (ctx, MhsdHeader, "mhsd");
	if (mhsd == NULL) {
		return -1;
	}

	db_parse_context_set_total_len (ctx, GINT_FROM_LE (mhsd->total_len));
	dump_mhsd (mhsd);
	switch (GINT_FROM_LE (mhsd->index)) {
	case MHSD_IMAGE_LIST: {
		DBParseContext *mhli_context;
		mhli_context = db_parse_context_get_next_child (ctx);
		parse_mhl (mhli_context, db, NULL, "mhli", parse_mhii);
		g_free (mhli_context);
		break;
	}
	case MHSD_ALBUM_LIST: {
		DBParseContext *mhla_context;
		mhla_context = db_parse_context_get_next_child (ctx);
		parse_mhl (mhla_context, db, NULL, "mhla", parse_mhba);
		g_free (mhla_context);
		break;
	}
	case MHSD_FILE_LIST: {
		DBParseContext *mhlf_context;
		mhlf_context = db_parse_context_get_next_child (ctx);
		parse_mhl (mhlf_context, db, NULL, "mhlf", parse_mhif);
		g_free (mhlf_context);
		break;
	}
	default:
		g_warning ("Unexpected mhsd index: %d\n", 
			   GINT_FROM_LE (mhsd->index));
		return -1;
		break;
	}

	return 0;
}

/* Database Object */
static int
parse_mhfd (DBParseContext *ctx, Itdb_iTunesDB *db, GError **error)
{
	MhfdHeader *mhfd;
	DBParseContext *mhsd_context;
	unsigned int cur_pos;

	mhfd = db_parse_context_get_m_header (ctx, MhfdHeader, "mhfd");
	if (mhfd == NULL) {
		return -1;
	}

	/* Sanity check */
	g_assert (GINT_FROM_LE (mhfd->total_len) == ctx->total_len);
	dump_mhfd (mhfd);
	cur_pos = ctx->header_len;

	/* The mhfd record always has 3 mhsd children, so it's hardcoded here.
	 * It could be replaced with a loop using the nb_children field from
	 * the mhfd record. [explanation by Christophe]
	 */
	mhsd_context = db_parse_context_get_sub_context (ctx, cur_pos);
	if (mhsd_context == NULL) {
		return -1;
	}
	parse_mhsd (mhsd_context, db, NULL);
	cur_pos += mhsd_context->total_len;
	g_free (mhsd_context);

	mhsd_context = db_parse_context_get_sub_context (ctx, cur_pos);
	if (mhsd_context == NULL) {
		return -1;
	}
	parse_mhsd (mhsd_context, db, NULL);
	cur_pos += mhsd_context->total_len;
	g_free (mhsd_context);

	mhsd_context = db_parse_context_get_sub_context (ctx, cur_pos);
	if (mhsd_context == NULL) {
		return -1;
	}
	parse_mhsd (mhsd_context, db, NULL);
	cur_pos += mhsd_context->total_len;
	g_free (mhsd_context);
	
	return 0;
}


G_GNUC_INTERNAL char *
ipod_db_get_artwork_db_path (const char *mount_point)
{
	const char *paths[] = {"iPod_Control", "Artwork", "ArtworkDB", NULL};
	return itdb_resolve_path (mount_point, paths);
}


static char *
ipod_db_get_photo_db_path (const char *mount_point)
{
	const char *paths[] = {"Photos", "Photo Database", NULL};
	g_return_val_if_fail (mount_point != NULL, NULL);
	return itdb_resolve_path (mount_point, paths);
				  
}


int
ipod_parse_artwork_db (Itdb_iTunesDB *db)
{
	DBParseContext *ctx;
	char *filename;

	g_return_val_if_fail (db, -1);

	ctx = NULL;
	filename = ipod_db_get_artwork_db_path (db->mountpoint);
	if (filename == NULL) {
		goto error;
	}	
	ctx = db_parse_context_new_from_file (filename);	
	g_free (filename);
	if (ctx == NULL) {
		goto error;
	}	

	parse_mhfd (ctx, db, NULL);
	db_parse_context_destroy (ctx, TRUE);
	return 0;

 error:
	if (ctx != NULL) {
		db_parse_context_destroy (ctx, TRUE);
	}
	return -1;
}

int
ipod_parse_photo_db (const char *mount_point)
{
	DBParseContext *ctx;
	char *filename;

	filename = ipod_db_get_photo_db_path (mount_point);
	if (filename == NULL) {
		return -1;
	}       
	ctx = db_parse_context_new_from_file (filename);
	g_free (filename);
	if (ctx == NULL) {
		return  -1;
	}

	parse_mhfd (ctx, NULL, NULL);
	db_parse_context_destroy (ctx, TRUE);

	return 0;
}

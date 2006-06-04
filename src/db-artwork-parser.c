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

#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "itdb.h"
#include "itdb_private.h"
#include "itdb_endianness.h"
#include "db-artwork-debug.h"
#include "db-artwork-parser.h"
#include "db-image-parser.h"
#include "db-itunes-parser.h"
#include "db-parse-context.h"
#include <glib/gi18n-lib.h>

typedef int (*ParseListItem)(DBParseContext *ctx, GError *error);

#ifndef NOT_DEFINED_DEBUG_ARTWORKDB
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
parse_mhif (DBParseContext *ctx, GError *error)
{
	MhifHeader *mhif;

	mhif = db_parse_context_get_m_header (ctx, MhifHeader, "mhif");
	if (mhif == NULL) {
		return -1;
	}
	dump_mhif (mhif);
	db_parse_context_set_total_len (ctx, get_gint32 (mhif->total_len, ctx->byte_order));
	return 0;
}

static int
parse_mhia (DBParseContext *ctx, Itdb_PhotoAlbum *photo_album, GError *error)
{
	MhiaHeader *mhia;
	gint image_id;

	mhia = db_parse_context_get_m_header (ctx, MhiaHeader, "mhia");
	if (mhia == NULL) {
		return -1;
	}
	dump_mhia (mhia);
	image_id = get_gint32 (mhia->image_id, ctx->byte_order);
	photo_album->members = g_list_append (photo_album->members,
					      GINT_TO_POINTER(image_id));
	db_parse_context_set_total_len (ctx,
					get_gint32_db (ctx->db, mhia->total_len));
	return 0;
}

static char *
get_utf16_string (void* buffer, gint length, guint byte_order)
{
	char *result;
	gunichar2 *tmp;
	int i;
	/* Byte-swap the utf16 characters if necessary (I'm relying
	 * on gcc to optimize most of this code away on LE platforms)
	 */
	tmp = g_memdup (buffer, length);
	for (i = 0; i < length/2; i++) {
		tmp[i] = get_gint16 (tmp[i], byte_order);
	}
	result = g_utf16_to_utf8 (tmp, length/2, NULL, NULL, NULL);
	g_free (tmp);

	return result;
	
}

static char *
mhod3_get_ithmb_filename (DBParseContext *ctx, ArtworkDB_MhodHeaderArtworkType3 *mhod3)
{
       char *filename=NULL;

       g_assert (mhod3 != NULL);

       if (mhod3->mhod_version == 2)
	   filename = get_utf16_string ((gunichar2 *)mhod3->string,
					get_gint32 (mhod3->string_len, ctx->byte_order),
					ctx->byte_order);
       else if ((mhod3->mhod_version == 0) ||
		(mhod3->mhod_version == 1))
	   filename = g_strndup (mhod3->string,
				 get_gint32 (mhod3->string_len, ctx->byte_order));
       else
	   g_warning (_("Unexpected mhod3 string type: %d\n"),
		      mhod3->mhod_version);
       return filename;
}


static int
parse_mhod_3 (DBParseContext *ctx,
	      Itdb_Thumb *thumb, GError *error)
{
	ArtworkDB_MhodHeader *mhod;
	ArtworkDB_MhodHeaderArtworkType3 *mhod3;
	gint32 mhod3_type;

	mhod = db_parse_context_get_m_header (ctx, ArtworkDB_MhodHeader, "mhod");
	if (mhod == NULL) {
		return -1;
	}
	db_parse_context_set_total_len (ctx, get_gint32 (mhod->total_len, ctx->byte_order));
	
	if (get_gint32 (mhod->total_len, ctx->byte_order) < sizeof (ArtworkDB_MhodHeaderArtworkType3)){
		return -1;
	}
	mhod3 = (ArtworkDB_MhodHeaderArtworkType3*)mhod;
	mhod3_type = get_gint16 (mhod3->type, ctx->byte_order);
	if (mhod3_type != MHOD_ARTWORK_TYPE_FILE_NAME) {
		return -1;
	}
	thumb->filename = mhod3_get_ithmb_filename (ctx, mhod3);
	dump_mhod_type_3 (mhod3);
	return 0;
}

static int
parse_photo_mhni (DBParseContext *ctx, Itdb_Artwork *artwork, GError *error)
{
	MhniHeader *mhni;
	DBParseContext *mhod_ctx; 
	Itdb_Thumb *thumb;

	mhni = db_parse_context_get_m_header (ctx, MhniHeader, "mhni");
	if (mhni == NULL) {
		return -1;
	}
	db_parse_context_set_total_len (ctx, get_gint32 (mhni->total_len, ctx->byte_order));
	dump_mhni (mhni);

	thumb = ipod_image_new_from_mhni (mhni, ctx->db);
	if (thumb == NULL) {
		return 0;
	}

	artwork->thumbnails = g_list_append (artwork->thumbnails, thumb);

	mhod_ctx = db_parse_context_get_sub_context (ctx, ctx->header_len);
	if (mhod_ctx == NULL) {
	  return -1;
	}
	parse_mhod_3 (mhod_ctx, thumb, error);
	g_free (mhod_ctx);

	return 0;
}

static int
parse_mhni (DBParseContext *ctx, iPodSong *song, GError *error)
{
	MhniHeader *mhni;
	DBParseContext *mhod_ctx; 
	Itdb_Thumb *thumb;

	mhni = db_parse_context_get_m_header (ctx, MhniHeader, "mhni");
	if (mhni == NULL) {
		return -1;
	}
	db_parse_context_set_total_len (ctx, get_gint32 (mhni->total_len, ctx->byte_order));
	dump_mhni (mhni);
	thumb = ipod_image_new_from_mhni (mhni, ctx->db);
	if (thumb == NULL) {
		return 0;
	}

	song->artwork->thumbnails =
		g_list_append (song->artwork->thumbnails, thumb);

	mhod_ctx = db_parse_context_get_sub_context (ctx, ctx->header_len);
	if (mhod_ctx == NULL) {
	  return -1;
	}
	parse_mhod_3 (mhod_ctx, thumb, error);
	g_free (mhod_ctx);

	return 0;
}

static int
parse_photo_mhod (DBParseContext *ctx, Itdb_Artwork *artwork, GError *error)
{
	ArtworkDB_MhodHeader *mhod;
	DBParseContext *mhni_ctx;
	int type;

	mhod = db_parse_context_get_m_header (ctx, ArtworkDB_MhodHeader, "mhod");
	if (mhod == NULL) {
		return -1;
	}
	db_parse_context_set_total_len (ctx, get_gint32 (mhod->total_len, ctx->byte_order));

	/* The MHODs found in the ArtworkDB and Photo Database files are
	 * significantly different than those found in the iTunesDB.
	 * For example, the "type" is split like this:
	 * - low 3 bytes are actual type;
	 * - high byte is padding length of string (0-3).
	 */
	type = get_gint16 (mhod->type, ctx->byte_order) & 0x00FFFFFF;

	if ( type == MHOD_ARTWORK_TYPE_ALBUM_NAME) {
		dump_mhod_type_1 ((MhodHeaderArtworkType1 *)mhod);
	} else {
		dump_mhod (mhod);
	}

	/* if this is a container... */
	if (type == MHOD_ARTWORK_TYPE_THUMBNAIL) {
		mhni_ctx = db_parse_context_get_sub_context (ctx, ctx->header_len);
		if (mhni_ctx == NULL) {
			return -1;
		}
		parse_photo_mhni (mhni_ctx, artwork, NULL);
		g_free (mhni_ctx);
	}

	return 0;
}
static int
parse_mhod (DBParseContext *ctx, iPodSong *song, GError *error)
{
	ArtworkDB_MhodHeader *mhod;
	DBParseContext *mhni_ctx;
	int type;
	gint32 total_len;

	mhod = db_parse_context_get_m_header (ctx, ArtworkDB_MhodHeader, "mhod");
	if (mhod == NULL) {
		return -1;
	}
	total_len = get_gint32 (mhod->total_len, ctx->byte_order);
	db_parse_context_set_total_len (ctx, total_len);

	type = get_gint16 (mhod->type, ctx->byte_order);

	if ( type == MHOD_ARTWORK_TYPE_ALBUM_NAME) {
		dump_mhod_type_1 ((MhodHeaderArtworkType1 *)mhod);
	} else {
		dump_mhod (mhod);
	}

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
parse_mhii (DBParseContext *ctx, GError *error)
{
	MhiiHeader *mhii;
	DBParseContext *mhod_ctx;
	int num_children;
	off_t cur_offset;
	iPodSong *song;
	Itdb_Artwork *artwork;
	Itdb_PhotoDB *photodb;
	Itdb_iTunesDB *itunesdb;

	mhii = db_parse_context_get_m_header (ctx, MhiiHeader, "mhii");
	if (mhii == NULL) {
		return -1;
	}
	db_parse_context_set_total_len (ctx, get_gint32 (mhii->total_len, ctx->byte_order));
	dump_mhii (mhii);

	switch (ctx->db->db_type) {
	case DB_TYPE_PHOTO:
		artwork = g_new0( Itdb_Artwork, 1 );
		artwork->artwork_size = get_gint32 (mhii->orig_img_size, ctx->byte_order);
		artwork->id = get_gint32 (mhii->image_id, ctx->byte_order);
		artwork->creation_date = itdb_time_mac_to_host( get_gint32 (mhii->digitised_date, ctx->byte_order) );

		cur_offset = ctx->header_len;
		mhod_ctx = db_parse_context_get_sub_context (ctx, cur_offset);
		num_children = get_gint32 (mhii->num_children, ctx->byte_order);
		while ((num_children > 0) && (mhod_ctx != NULL)) {
			parse_photo_mhod (mhod_ctx, artwork, NULL);
			num_children--;
			cur_offset += mhod_ctx->total_len;
			g_free (mhod_ctx);
			mhod_ctx = db_parse_context_get_sub_context (ctx, cur_offset);
		}
		photodb = db_get_photodb (ctx->db);
		g_return_val_if_fail (photodb, -1);
		photodb->photos = g_list_append (photodb->photos,
						 artwork);
		break;
	case DB_TYPE_ITUNES:
#ifdef NOT_DEFINED_DEBUG_ARTWORKDB
		song = NULL;
#else
		itunesdb = db_get_itunesdb (ctx->db);
		g_return_val_if_fail (itunesdb, -1);
		song = get_song_by_dbid (itunesdb,
					 get_gint64 (mhii->song_id,
						     ctx->byte_order));
		if (song == NULL) {
			return -1;
		}
		song->artwork->artwork_size = get_gint32 (mhii->orig_img_size, ctx->byte_order);
		if ((song->artwork_size+song->artwork_count) !=
				song->artwork->artwork_size) {
			g_warning (_("iTunesDB and ArtworkDB artwork sizes inconsistent (%d+%d != %d)"), song->artwork_size, song->artwork_count, song->artwork->artwork_size);
		}
		song->artwork->id = get_gint32 (mhii->image_id, ctx->byte_order);
#endif

		cur_offset = ctx->header_len;
		mhod_ctx = db_parse_context_get_sub_context (ctx, cur_offset);
		num_children = get_gint32 (mhii->num_children, ctx->byte_order);
		while ((num_children > 0) && (mhod_ctx != NULL)) {
			parse_mhod (mhod_ctx, song, NULL);
			num_children--;
			cur_offset += mhod_ctx->total_len;
			g_free (mhod_ctx);
			mhod_ctx = db_parse_context_get_sub_context (ctx, cur_offset);
		}
		break;
	default:
	       g_return_val_if_reached (-1);
	}
	return 0;
}

static int
parse_mhba (DBParseContext *ctx, GError *error)
{
	MhbaHeader *mhba;
	ArtworkDB_MhodHeader *mhod;
	DBParseContext *mhod_ctx;
	DBParseContext *mhia_ctx;
	Itdb_PhotoAlbum *photo_album;
	Itdb_PhotoDB *photodb;
	int num_children;
	off_t cur_offset;

	mhba = db_parse_context_get_m_header (ctx, MhbaHeader, "mhba");
	if (mhba == NULL) {
		return -1;
	}
	db_parse_context_set_total_len (ctx, get_gint32 (mhba->total_len, ctx->byte_order));

	dump_mhba (mhba);

	photo_album = g_new0 (Itdb_PhotoAlbum, 1);
	photo_album->num_images = get_gint32( mhba->num_mhias, ctx->byte_order);
	photo_album->album_id = get_gint32( mhba->playlist_id, ctx->byte_order);
	photo_album->master = get_gint32( mhba->master, ctx->byte_order);
	photo_album->prev_album_id = get_gint32( mhba->prev_playlist_id, ctx->byte_order);

	cur_offset = ctx->header_len;
	mhod_ctx = db_parse_context_get_sub_context (ctx, cur_offset);
	num_children = get_gint32 (mhba->num_mhods, ctx->byte_order);
	while ((num_children > 0) && (mhod_ctx != NULL)) {
		/* FIXME: First mhod is album name, whats the others for? */
		mhod = db_parse_context_get_m_header (mhod_ctx, ArtworkDB_MhodHeader, "mhod");
		if (mhod == NULL) {
			return -1;
		}
		db_parse_context_set_total_len (mhod_ctx,  get_gint32(mhod->total_len, ctx->byte_order));
		photo_album->name = g_strdup( (char *)((MhodHeaderArtworkType1*)mhod)->string );
		cur_offset += mhod_ctx->total_len;
		dump_mhod_type_1 ((MhodHeaderArtworkType1*)mhod);
		g_free (mhod_ctx);
		num_children--;
	}

	mhia_ctx = db_parse_context_get_sub_context (ctx, cur_offset);
	num_children = get_gint32 (mhba->num_mhias, ctx->byte_order);
	while ((num_children > 0) && (mhia_ctx != NULL)) {
		parse_mhia (mhia_ctx, photo_album, NULL);
		num_children--;
		cur_offset += mhia_ctx->total_len;
		g_free (mhia_ctx);
		mhia_ctx = db_parse_context_get_sub_context (ctx, cur_offset);
	}
	photodb = db_get_photodb (ctx->db);
	g_return_val_if_fail (photodb, -1);
	photodb->photoalbums = g_list_append (photodb->photoalbums,
					      photo_album);
	return 0;
}


static int
parse_mhl (DBParseContext *ctx, GError *error, 
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

	num_children = get_gint32 (mhl->num_children, ctx->byte_order);
	if (num_children < 0) {
		return -1;
	}

	cur_offset = ctx->header_len;
	mhi_ctx = db_parse_context_get_sub_context (ctx, cur_offset);
	while ((num_children > 0) && (mhi_ctx != NULL)) {
		if (parse_child != NULL) {
			parse_child (mhi_ctx, NULL);
		}
		num_children--;
		cur_offset += mhi_ctx->total_len;
		g_free (mhi_ctx);
		mhi_ctx = db_parse_context_get_sub_context (ctx, cur_offset);
	}

	return 0;

}


static int 
parse_mhsd (DBParseContext *ctx, GError **error)
{
	ArtworkDB_MhsdHeader *mhsd;

	mhsd = db_parse_context_get_m_header (ctx, ArtworkDB_MhsdHeader, "mhsd");
	if (mhsd == NULL) {
		return -1;
	}

	db_parse_context_set_total_len (ctx, get_gint32 (mhsd->total_len, ctx->byte_order));
	dump_mhsd (mhsd);
	switch (get_gint16_db (ctx->db, mhsd->index)) {
	case MHSD_IMAGE_LIST: {
		DBParseContext *mhli_context;
		mhli_context = db_parse_context_get_next_child (ctx);
		parse_mhl (mhli_context, NULL, "mhli", parse_mhii);
		g_free (mhli_context);
		break;
	}
	case MHSD_ALBUM_LIST: {
		DBParseContext *mhla_context;
		mhla_context = db_parse_context_get_next_child (ctx);
		parse_mhl (mhla_context, NULL, "mhla", parse_mhba);
		g_free (mhla_context);
		break;
	}
	case MHSD_FILE_LIST: {
		DBParseContext *mhlf_context;
		mhlf_context = db_parse_context_get_next_child (ctx);
		parse_mhl (mhlf_context, NULL, "mhlf", parse_mhif);
		g_free (mhlf_context);
		break;
	}
	default:
		g_warning (_("Unexpected mhsd index: %d\n"), 
			   get_gint16_db (ctx->db, mhsd->index));
		return -1;
		break;
	}

	return 0;
}

/* Database Object */
static int
parse_mhfd (DBParseContext *ctx, GError **error)
{
	MhfdHeader *mhfd;
	DBParseContext *mhsd_context;
	unsigned int cur_pos;
	gint total_len;

	mhfd = db_parse_context_get_m_header (ctx, MhfdHeader, "mhfd");
	if (mhfd == NULL) {
		return -1;
	}

	/* Sanity check */
	total_len = get_gint32_db (ctx->db, mhfd->total_len);
	g_return_val_if_fail (total_len == ctx->total_len, -1);
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
	parse_mhsd (mhsd_context, NULL);
	cur_pos += mhsd_context->total_len;
	g_free (mhsd_context);

	mhsd_context = db_parse_context_get_sub_context (ctx, cur_pos);
	if (mhsd_context == NULL) {
		return -1;
	}
	parse_mhsd (mhsd_context, NULL);
	cur_pos += mhsd_context->total_len;
	g_free (mhsd_context);

	mhsd_context = db_parse_context_get_sub_context (ctx, cur_pos);
	if (mhsd_context == NULL) {
		return -1;
	}
	parse_mhsd (mhsd_context, NULL);
	cur_pos += mhsd_context->total_len;
	g_free (mhsd_context);
	
	return 0;
}


G_GNUC_INTERNAL char *
ipod_db_get_artwork_db_path (const char *mount_point)
{
        gchar *filename=NULL;

	/* fail silently if no mount point given */
	if (!mount_point)  return NULL;

	filename = itdb_get_artworkdb_path (mount_point);

	/* itdb_resolve_path() only returns existing paths */
	if (!filename)
	{
	    gchar *artwork_dir;

	    artwork_dir = itdb_get_artwork_dir (mount_point);
	    if (!artwork_dir)
	    {
		/* attempt to create Artwork dir */
		gchar *control_dir = itdb_get_control_dir (mount_point);
		if (control_dir)
		{
		    gchar *dir = g_build_filename (control_dir, "Artwork", NULL);
		    mkdir (dir, 0777);
		    g_free (control_dir);
		    g_free (dir);
		    artwork_dir = itdb_get_artwork_dir (mount_point);
		}
	    }
	    if (artwork_dir)
	    {
		filename = g_build_filename (artwork_dir,
					     "ArtworkDB", NULL);
		g_free (artwork_dir);
	    }
	}

	return filename;
}

G_GNUC_INTERNAL gboolean
ipod_supports_cover_art (Itdb_Device *device)
{
	const Itdb_ArtworkFormat *formats;

	if (device == NULL) {
		return FALSE;
	}

	formats = itdb_device_get_artwork_formats (device);
	if (formats == NULL) {
		return FALSE;
	}
	
	while (formats->type != -1)
	{
	    switch (formats->type)
	    {
	    case ITDB_THUMB_COVER_SMALL:
	    case ITDB_THUMB_COVER_LARGE:
		return TRUE;
	    case ITDB_THUMB_PHOTO_SMALL:
	    case ITDB_THUMB_PHOTO_LARGE:
	    case ITDB_THUMB_PHOTO_FULL_SCREEN:
	    case ITDB_THUMB_PHOTO_TV_SCREEN:
		break;
	    }
	    formats++;
	}

	return FALSE;
}

G_GNUC_INTERNAL gboolean
ipod_supports_photos (Itdb_Device *device)
{
	const Itdb_ArtworkFormat *formats;

	if (device == NULL) {
		return FALSE;
	}

	formats = itdb_device_get_artwork_formats (device);
	if (formats == NULL) {
		return FALSE;
	}
	
	while (formats->type != -1)
	{
	    switch (formats->type)
	    {
	    case ITDB_THUMB_COVER_SMALL:
	    case ITDB_THUMB_COVER_LARGE:
		break;
	    case ITDB_THUMB_PHOTO_SMALL:
	    case ITDB_THUMB_PHOTO_LARGE:
	    case ITDB_THUMB_PHOTO_FULL_SCREEN:
	    case ITDB_THUMB_PHOTO_TV_SCREEN:
		return TRUE;
	    }
	    formats++;
	}

	return FALSE;
}


int
ipod_parse_artwork_db (Itdb_iTunesDB *itdb)
{
	DBParseContext *ctx;
	char *filename;
	Itdb_DB db;

	db.db.itdb = itdb;
	db.db_type = DB_TYPE_ITUNES;

	g_return_val_if_fail (itdb, -1);

	if (!ipod_supports_cover_art (itdb->device)) {
		return -1;
	}
	ctx = NULL;
	filename = ipod_db_get_artwork_db_path (itdb_get_mountpoint (itdb));
	if (filename == NULL) {
		goto error;
	}
	if (!g_file_test (filename, G_FILE_TEST_EXISTS))
	{
		goto error;
	}

	ctx = db_parse_context_new_from_file (filename, &db);
	g_free (filename);
	if (ctx == NULL) {
		goto error;
	}	

	parse_mhfd (ctx, NULL);
	db_parse_context_destroy (ctx, TRUE);
	return 0;

 error:
	if (ctx != NULL) {
		db_parse_context_destroy (ctx, TRUE);
	}
	return -1;
}


G_GNUC_INTERNAL char *
ipod_db_get_photos_db_path (const char *mount_point)
{
        gchar *filename=NULL;

	/* fail silently if no mount point given */
	if (!mount_point)  return NULL;

	filename = itdb_get_photodb_path (mount_point);

	/* itdb_resolve_path() only returns existing paths */
	if (!filename)
	{
	    gchar *photos_dir;

	    photos_dir = itdb_get_photos_dir (mount_point);
	    if (!photos_dir)
	    {
		/* attempt to create Photos dir */
		gchar *dir = g_build_filename (mount_point, "Photos", NULL);
		mkdir (dir, 0777);
		g_free (dir);
		photos_dir = itdb_get_photos_dir (mount_point);
	    }
	    if (photos_dir)
	    {
		filename = g_build_filename (photos_dir,
					     "Photo Database", NULL);
		g_free (photos_dir);
	    }
	}

	return filename;
}


int
ipod_parse_photo_db (Itdb_PhotoDB *photodb)
{
	DBParseContext *ctx;
	char *filename;
	Itdb_DB db;

	db.db.photodb = photodb;
	db.db_type = DB_TYPE_PHOTO;


	filename = itdb_get_photodb_path (
	    itdb_photodb_get_mountpoint (photodb));
	if (filename == NULL) {
		return -1;
	}       

	ctx = db_parse_context_new_from_file (filename, &db );
	g_free (filename);
	if (ctx == NULL) {
		return  -1;
	}
	parse_mhfd (ctx, NULL);
	db_parse_context_destroy (ctx, TRUE);

	return 0;
}


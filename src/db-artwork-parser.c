/*
 *  Copyright (C) 2005-2007 Christophe Fergeau
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

#include <stdio.h>
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
#include <glib/gstdio.h>

typedef int (*ParseListItem)(DBParseContext *ctx, GError *error);


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
	guint32 image_id;

	mhia = db_parse_context_get_m_header (ctx, MhiaHeader, "mhia");
	if (mhia == NULL) {
		return -1;
	}
	dump_mhia (mhia);
	image_id = get_gint32 (mhia->image_id, ctx->byte_order);
	photo_album->members = g_list_append (photo_album->members,
					      GUINT_TO_POINTER(image_id));
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

struct ParsedMhodString {
        enum MhodArtworkType mhod_type;
        char *mhod_string;
};

static struct ParsedMhodString *
parse_mhod_string (DBParseContext *ctx, GError *error)
{
	struct ParsedMhodString *result;
	ArtworkDB_MhodHeaderString *mhod_string;
	ArtworkDB_MhodHeader *mhod;
	gint len;
	mhod = db_parse_context_get_m_header (ctx, ArtworkDB_MhodHeader, "mhod");
	if (mhod == NULL) {
		return NULL;
	}
	db_parse_context_set_total_len (ctx, get_gint32 (mhod->total_len, ctx->byte_order));

	if (get_gint32 (mhod->total_len, ctx->byte_order) < sizeof (ArtworkDB_MhodHeaderString)){
		return NULL;
	}

	result = g_new0 (struct ParsedMhodString, 1);
	if (result == NULL) {
		return NULL;
	}

	mhod_string = (ArtworkDB_MhodHeaderString*)mhod;
	result->mhod_type = get_gint16 (mhod_string->type, ctx->byte_order);
	len = get_gint32 (mhod_string->string_len, ctx->byte_order);
	switch (mhod_string->encoding) {
	case 2:
		result->mhod_string = get_utf16_string ((gunichar2 *)mhod_string->string,
							len, ctx->byte_order);
		break;
	case 0:
	case 1: 
		result->mhod_string = g_strndup (mhod_string->string, len);
		break;
	default:
		g_warning (_("Unexpected mhod string type: %d\n"),
			   mhod_string->encoding);
		break;
	}
	dump_mhod_string (mhod_string);
	return result;
}

static int
parse_mhod_3 (DBParseContext *ctx,
	      Itdb_Thumb_Ipod_Item *thumb, GError *error)
{
	struct ParsedMhodString *mhod;
	mhod = parse_mhod_string (ctx, error);
	if (mhod == NULL) {
		return -1;
	}
	if (mhod->mhod_type != MHOD_ARTWORK_TYPE_FILE_NAME) {
		g_free (mhod->mhod_string);
		g_free (mhod);
		return -1;
	}

	thumb->filename = mhod->mhod_string;
	g_free (mhod);
	return 0;
}

static int
parse_photo_mhni (DBParseContext *ctx, Itdb_Thumb_Ipod *thumbs, GError *error)
{
	MhniHeader *mhni;
	DBParseContext *mhod_ctx; 
	Itdb_Thumb_Ipod_Item *thumb;

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
    
        itdb_thumb_ipod_add (thumbs, thumb);

	mhod_ctx = db_parse_context_get_sub_context (ctx, ctx->header_len);
	if (mhod_ctx == NULL) {
	  return -1;
	}
	parse_mhod_3 (mhod_ctx, thumb, error);
	g_free (mhod_ctx);

	return 0;
}

static int
parse_photo_mhod (DBParseContext *ctx, Itdb_Thumb_Ipod *thumbs, GError *error)
{
	ArtworkDB_MhodHeader *mhod;
	DBParseContext *mhni_ctx;
	gint32 type;

	mhod = db_parse_context_get_m_header (ctx, ArtworkDB_MhodHeader, "mhod");
	if (mhod == NULL) {
		return -1;
	}
	db_parse_context_set_total_len (ctx, get_gint32 (mhod->total_len, ctx->byte_order));

	type = get_gint16 (mhod->type, ctx->byte_order);

        dump_mhod (mhod);

	/* if this is a container... */
	if (type == MHOD_ARTWORK_TYPE_THUMBNAIL) {
		mhni_ctx = db_parse_context_get_sub_context (ctx, ctx->header_len);
		if (mhni_ctx == NULL) {
			return -1;
		}
		parse_photo_mhni (mhni_ctx, thumbs, NULL);
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
	Itdb_Artwork *artwork;
	Itdb_PhotoDB *photodb;
	guint64 mactime;
	Itdb_Device *device = db_get_device (ctx->db);
        Itdb_Thumb_Ipod *thumbs;
            
	mhii = db_parse_context_get_m_header (ctx, MhiiHeader, "mhii");
	if (mhii == NULL)
	{
	    return -1;
	}
	db_parse_context_set_total_len (ctx, get_gint32 (mhii->total_len, ctx->byte_order));
	dump_mhii (mhii);

	artwork = itdb_artwork_new ();

	artwork->id = get_gint32 (mhii->image_id, ctx->byte_order);
	artwork->unk028 = get_gint32 (mhii->unknown4, ctx->byte_order);
	artwork->rating = get_gint32 (mhii->rating, ctx->byte_order);
	artwork->unk036 = get_gint32 (mhii->unknown6, ctx->byte_order);
	mactime = get_gint32 (mhii->orig_date, ctx->byte_order);
	artwork->creation_date = device_time_mac_to_time_t (device, mactime);
	mactime = get_gint32 (mhii->digitized_date, ctx->byte_order);
	artwork->digitized_date = device_time_mac_to_time_t (device, mactime);
	artwork->artwork_size = get_gint32 (mhii->orig_img_size, ctx->byte_order);
	artwork->dbid = get_gint64 (mhii->song_id, ctx->byte_order);

        thumbs = (Itdb_Thumb_Ipod *)itdb_thumb_ipod_new ();
        artwork->thumbnail = (Itdb_Thumb *)thumbs;
	cur_offset = ctx->header_len;
	mhod_ctx = db_parse_context_get_sub_context (ctx, cur_offset);
	num_children = get_gint32 (mhii->num_children, ctx->byte_order);
	while ((num_children > 0) && (mhod_ctx != NULL))
	{
	    parse_photo_mhod (mhod_ctx, thumbs, NULL);
	    num_children--;
	    cur_offset += mhod_ctx->total_len;
	    g_free (mhod_ctx);
	    mhod_ctx = db_parse_context_get_sub_context (ctx, cur_offset);
	}
        g_free (mhod_ctx);

	switch (ctx->db->db_type)
	{
	case DB_TYPE_PHOTO:
	    photodb = db_get_photodb (ctx->db);
	    g_return_val_if_fail (photodb, -1);
	    photodb->photos = g_list_append (photodb->photos, artwork);
	    break;
	case DB_TYPE_ITUNES:
	    g_return_val_if_fail (ctx->artwork!=NULL, -1);
	    *ctx->artwork = g_list_prepend (*ctx->artwork, artwork);
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
	DBParseContext *mhod_ctx;
	DBParseContext *mhia_ctx;
	Itdb_PhotoAlbum *album;
	Itdb_PhotoDB *photodb;
	int num_children;
	off_t cur_offset;

	mhba = db_parse_context_get_m_header (ctx, MhbaHeader, "mhba");
	if (mhba == NULL) {
		return -1;
	}
	db_parse_context_set_total_len (ctx, get_gint32 (mhba->total_len, ctx->byte_order));

	dump_mhba (mhba);

	album = g_new0 (Itdb_PhotoAlbum, 1);
	album->album_id = get_gint32(mhba->album_id, ctx->byte_order);
	album->unk024 = get_gint32(mhba->unk024, ctx->byte_order);
	album->unk028 = get_gint16(mhba->unk028, ctx->byte_order);
	album->album_type = mhba->album_type;
	album->playmusic = mhba->playmusic;
	album->repeat = mhba->repeat;
	album->random = mhba->random;
	album->show_titles = mhba->show_titles;
	album->transition_direction = mhba->transition_direction;
	album->slide_duration = get_gint32(mhba->slide_duration,
					   ctx->byte_order);
	album->transition_duration = get_gint32(mhba->transition_duration,
						ctx->byte_order);
	album->unk044 = get_gint32(mhba->unk044, ctx->byte_order);
	album->unk048 = get_gint32(mhba->unk048, ctx->byte_order);
	album->song_id = get_gint64(mhba->song_id, ctx->byte_order);
	album->prev_album_id = get_gint32(mhba->prev_album_id,
					  ctx->byte_order);

	cur_offset = ctx->header_len;
	num_children = get_gint32 (mhba->num_mhods, ctx->byte_order);

        mhod_ctx = db_parse_context_get_sub_context (ctx, cur_offset);
        while ((num_children > 0) && (mhod_ctx != NULL)) {
		struct ParsedMhodString *mhod;

		mhod = parse_mhod_string (mhod_ctx, error);
		if (mhod == NULL) {
			break;
		}
		switch (mhod->mhod_type)
		{  /* FIXME: type==1 is album name. type==2 seems to be
		    * the transtition type between photos,
		    * e.g. "Dissolve". Not handled yet. */
		case MHOD_ARTWORK_TYPE_ALBUM_NAME:
			g_free (album->name);
			album->name = mhod->mhod_string;
			g_free (mhod);
			break;
		default:
			g_free (mhod->mhod_string);
			g_free (mhod);
			break;
		}
		cur_offset += mhod_ctx->total_len;
		g_free (mhod_ctx);
		num_children--;
		mhod_ctx = db_parse_context_get_sub_context (ctx, cur_offset);
        }
	g_free (mhod_ctx);

	mhia_ctx = db_parse_context_get_sub_context (ctx, cur_offset);
	num_children = get_gint32 (mhba->num_mhias, ctx->byte_order);
	while ((num_children > 0) && (mhia_ctx != NULL)) {
		parse_mhia (mhia_ctx, album, NULL);
		num_children--;
		cur_offset += mhia_ctx->total_len;
		g_free (mhia_ctx);
		mhia_ctx = db_parse_context_get_sub_context (ctx, cur_offset);
	}
        g_free (mhia_ctx);
	photodb = db_get_photodb (ctx->db);
	g_return_val_if_fail (photodb, -1);
	album->photodb = photodb;
	photodb->photoalbums = g_list_append (photodb->photoalbums,
					      album);
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
        g_free (mhi_ctx);

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


/* Compares the two guint64 values being pointed to and returns TRUE if
 * they are equal. It can be passed to g_hash_table_new() as the
 *  key_equal_func parameter, when using pointers to guint64 as keys
 * in a GHashTable.
 */
static gboolean
guint64_equal (gconstpointer v1, gconstpointer v2)
{
    guint64 i1 = *(const guint64*)v1;
    guint64 i2 = *(const guint64*)v2;
    return i1 == i2;
}

/* Converts a pointer to a guint64 to a hash value. It can be passed to
 * g_hash_table_new() as the hash_func parameter, when using pointers
 * to guint64 values as keys in a GHashTable.
 */
static guint
guint64_hash(gconstpointer v)
{
    guint64 i = *(const guint64*)v;
    return i ^ (i >> 32);
}

/* Apple introduced a new way to associate artwork. The former way
 * used the dbid to link each artwork (mhii) back to the track. The
 * new way uses the mhii id to link from each track to the mhii. Above
 * we only handled the former way */
static int
mhfd_associate_itunesdb_artwork (DBParseContext *ctx)
{
    GHashTable *mhii_id_hash;
    GHashTable *song_dbid_hash;
    Itdb_iTunesDB *itdb;
    GList *gl;

    g_return_val_if_fail (ctx && ctx->artwork, -1);
    itdb = db_get_itunesdb (ctx->db);
    g_return_val_if_fail (itdb, -1);

    /* make a hash linking the dbid with the songs for faster
       lookup */
    song_dbid_hash = g_hash_table_new (guint64_hash, guint64_equal);

    for (gl = itdb->tracks; gl != NULL; gl = gl->next) {
	Itdb_Track *song = (Itdb_Track*)gl->data;
	g_hash_table_insert (song_dbid_hash, &song->dbid, song);
    }

    /* make a hash linking the mhii with the artwork for faster
       lookup */
    mhii_id_hash = g_hash_table_new_full (g_direct_hash,
					  g_direct_equal,
					  NULL,
					  (GDestroyNotify)itdb_artwork_free);

    for (gl=*ctx->artwork; gl; gl=gl->next)
    {
	Itdb_Track *track;
	Itdb_Artwork *artwork=gl->data;
	g_return_val_if_fail (artwork, -1);

	g_hash_table_insert (mhii_id_hash, GINT_TO_POINTER (artwork->id), artwork);

	/* add Artwork to track indicated by the dbid for backward
	   compatibility */
	track = g_hash_table_lookup (song_dbid_hash, &artwork->dbid);
	if (track == NULL)
	{
	    gchar *strval = g_strdup_printf("%" G_GINT64_FORMAT, artwork->dbid);
	    g_print (_("Could not find corresponding track (dbid: %s) for artwork entry.\n"), strval);
	    g_free (strval);
	}
	else
	{
	    itdb_artwork_free (track->artwork);
	    track->artwork = itdb_artwork_duplicate (artwork);
	}
    }
    /* Now go through all the tracks and add artwork where an
     * mhii_link is available */
    for (gl=itdb->tracks; gl; gl=gl->next)
    {
	Itdb_Track *track = gl->data;
	g_return_val_if_fail (track, -1);
	if (track->mhii_link)
	{
	    Itdb_Artwork *artwork;
	    artwork = g_hash_table_lookup (mhii_id_hash,
					   GINT_TO_POINTER (track->mhii_link));
	    if (artwork)
	    {
		g_return_val_if_fail (track->artwork, -1);
		if (track->artwork->id != track->mhii_link)
		{
		    itdb_artwork_free (track->artwork);
		    track->artwork = itdb_artwork_duplicate (artwork);
		}
		else
		{
		    /* same artwork -- don't copy again */
		}
	    }
	    else
	    {
		/* The user can't do much here, so let's not worry them with
		 * this warning...
		gchar *strval = g_strdup_printf("%" G_GINT64_FORMAT, track->dbid);
		g_print (_("Could not find artwork entry (mhii id: %u) for track (dbid: %s).\n"), track->mhii_link, strval);
		g_free (strval);
		 */

		/* couldn't find artwork -- make sure track data is in
		   a consistent state. */
		itdb_track_remove_thumbnails (track);
	    }
	}
    }
    g_hash_table_destroy (mhii_id_hash);
    g_hash_table_destroy (song_dbid_hash);
    /* The actual ItdbArtwork data was freed through the GHashTable
       value_destroy_func */
    g_list_free (*ctx->artwork);
    ctx->artwork = NULL;

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
	gint32 i;
	GList *artwork_glist = NULL;

	mhfd = db_parse_context_get_m_header (ctx, MhfdHeader, "mhfd");
	if (mhfd == NULL) {
		return -1;
	}

	/* Sanity check */
	total_len = get_gint32_db (ctx->db, mhfd->total_len);
	g_return_val_if_fail (total_len == ctx->total_len, -1);
	dump_mhfd (mhfd);
	cur_pos = ctx->header_len;

	if (ctx->db->db_type == DB_TYPE_ITUNES)
	{   /* we need to collect all artwork in this GList for
	       iTunesDB (see below) */
	    ctx->artwork = &artwork_glist;
	}

	for (i=0; i<mhfd->num_children; ++i)
	{
	    /* so far all mhfd we know have 3 children, but better be
	       safe than sorry */
	    mhsd_context = db_parse_context_get_sub_context (ctx, cur_pos);
	    if (mhsd_context == NULL) {
		return -1;
	    }
	    parse_mhsd (mhsd_context, NULL);
	    cur_pos += mhsd_context->total_len;
	    g_free (mhsd_context);
	}

	if (ctx->db->db_type == DB_TYPE_ITUNES)
	{
	    return mhfd_associate_itunesdb_artwork (ctx);
	}

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
		    g_mkdir (dir, 0777);
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

int
ipod_parse_artwork_db (Itdb_iTunesDB *itdb)
{
	DBParseContext *ctx;
	char *filename;
	Itdb_DB db;

	db.db.itdb = itdb;
	db.db_type = DB_TYPE_ITUNES;

	g_return_val_if_fail (itdb, -1);

	if (!itdb_device_supports_artwork (itdb->device)) {
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
	db_parse_context_destroy (ctx);
	return 0;

 error:
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
		g_mkdir (dir, 0777);
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

	GList *gl;
	GHashTable *hash;

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
	db_parse_context_destroy (ctx);

	/* Now we need to replace references to artwork_ids in the
	 * photo albums with references to the actual artwork
	 * structure. Since we cannot guarantee that the list with the
	 * photos is read before the album list, we cannot safely do
	 * this at the time of reading the ids. */

	/* Create a hash for faster lookup */
	hash = g_hash_table_new (g_int_hash, g_int_equal);
	for (gl=photodb->photos; gl; gl=gl->next)
	{
	    Itdb_Artwork *photo = gl->data;
	    g_return_val_if_fail (photo, -1);
	    g_hash_table_insert (hash, &photo->id, photo);
/*	    printf ("id: %d, photo: %p\n", photo->id, photo);*/
	}
	for (gl=photodb->photoalbums; gl; gl=gl->next)
	{
	    GList *glp;
	    Itdb_PhotoAlbum *album = gl->data;
	    g_return_val_if_fail (album, -1);
	    for (glp=album->members; glp; glp=glp->next)
	    {
		guint image_id = GPOINTER_TO_UINT (glp->data);
		Itdb_Artwork *photo = g_hash_table_lookup (hash, &image_id);
/*		printf ("id: %d, photo: %p\n", image_id, photo);*/
		glp->data = photo;
	    }
	}
	g_hash_table_destroy (hash);
	return 0;
}


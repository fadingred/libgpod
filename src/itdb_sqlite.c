/*
|  Copyright (C) 2009 Nikias Bassen <nikias@gmx.li>
|  Copyright (C) 2009 Christophe Fergeau <cfergeau@mandriva.com>
|  Copyright (C) 2009 Hector Martin <hector@marcansoft.com>
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
*/
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include <glib.h>
#include <glib/gstdio.h>
#include <sqlite3.h>
#include <plist/plist.h>

#ifdef HAVE_LIBIMOBILEDEVICE
#include <libimobiledevice/libimobiledevice.h>
#include <libimobiledevice/lockdown.h>
#endif

#include "itdb.h"
#include "itdb_private.h"
#include "itdb_sqlite_queries.h"


/** time zone offset in seconds */
static uint32_t tzoffset = 0;

static uint32_t timeconv(time_t tv)
{
    /* quite strange. this is time_t - 01.01.2001 01:00:00 - tzoffset */
    return tv - 978307200 - tzoffset;
}

enum sort_order {
	ORDER_TITLE,
	ORDER_ARTIST,
	ORDER_ALBUM,
	ORDER_GENRE,
	ORDER_COMPOSER,
	ORDER_ALBUM_ARTIST,
	ORDER_ALBUM_BY_ARTIST,
	ORDER_SERIES_NAME,
	ORDER_END
};

static glong sort_offsets[][4] = {
    { G_STRUCT_OFFSET(Itdb_Track, sort_title),       G_STRUCT_OFFSET(Itdb_Track, title) },
    { G_STRUCT_OFFSET(Itdb_Track, sort_artist),      G_STRUCT_OFFSET(Itdb_Track, artist) },
    { G_STRUCT_OFFSET(Itdb_Track, sort_album),       G_STRUCT_OFFSET(Itdb_Track, album) },
    { G_STRUCT_OFFSET(Itdb_Track, genre),            0 },
    { G_STRUCT_OFFSET(Itdb_Track, sort_composer),    G_STRUCT_OFFSET(Itdb_Track, composer) },
    { G_STRUCT_OFFSET(Itdb_Track, sort_artist),      G_STRUCT_OFFSET(Itdb_Track, artist) },
    /* TODO: iTunes sorts first by album_artist, then by artist. */
    { G_STRUCT_OFFSET(Itdb_Track, sort_albumartist), G_STRUCT_OFFSET(Itdb_Track, albumartist),
      G_STRUCT_OFFSET(Itdb_Track, sort_artist),      G_STRUCT_OFFSET(Itdb_Track, artist) },
    { 0, 0 } /* TODO: Series Name: ignored for now. */
};

static gchar *sort_field(Itdb_Track *track, guint member)
{
    gchar *field;
    int offset;
    int i;

    for (i = 0; i < 4; i++) {
	offset = sort_offsets[member][i];
	field = G_STRUCT_MEMBER(gchar *, track, offset);
        if (offset != 0 && field != NULL && *field) {
            return field;
        }
    }
    return NULL;
}

static int lookup_order(GHashTable **order_hashes, guint member, Itdb_Track *track)
{
    const char *key;

    g_assert (member < ORDER_END);

    key = sort_field(track, member);
    if (key == NULL) {
        return 100;
    }

    return GPOINTER_TO_UINT(g_hash_table_lookup(order_hashes[member], key));
}

static gint compare_track_fields(gconstpointer lhs, gconstpointer rhs)
{
    const gchar *field_lhs = (const gchar *)lhs;
    const gchar *field_rhs = (const gchar *)rhs;

    if (field_lhs == NULL && field_rhs == NULL) {
        return 0;
    }

    if (field_lhs == NULL) {
        return -1;
    }

    if (field_rhs == NULL) {
        return 1;
    }

    /* FIXME: should probably generate collation keys first */
    return g_utf8_collate(field_lhs, field_rhs);
}

static gboolean traverse_tracks(gpointer key, gpointer value, gpointer data)
{
    GHashTable *hash = (GHashTable *)data;

    if (key != NULL) {
        g_hash_table_insert(hash, key, GUINT_TO_POINTER((g_hash_table_size(hash) + 1) * 100));
    }
    return FALSE;
}

static GHashTable **compute_key_orders(GList *tracks)
{
    GHashTable **result;
    int i;

    result = g_new0(GHashTable *, ORDER_END);
    g_assert (result != NULL);

    for (i = 0; i < ORDER_END; i++) {
        GList *li;
        GTree *tree;

        tree = g_tree_new(compare_track_fields);

        for (li = tracks; li != NULL; li = li->next) {
            Itdb_Track *track = (Itdb_Track *)li->data;

            g_tree_insert(tree, sort_field(track, i), 0);
        }

        result[i] = g_hash_table_new(g_str_hash, g_str_equal);
        g_assert (result[i] != NULL);

        g_tree_foreach(tree, traverse_tracks, (gpointer)result[i]);
        g_tree_destroy(tree);
    }

    return result;
}

static void destroy_key_orders(GHashTable **orders)
{
    int i;

    for (i = 0; i < ORDER_END; i++) {
        g_hash_table_destroy(orders[i]);
    }

    g_free(orders);
}

static int mk_Dynamic(Itdb_iTunesDB *itdb, const char *outpath)
{
    int res = -1;
    gchar *dbf = NULL;
    sqlite3 *db = NULL;
    sqlite3_stmt *stmt = NULL;
    char *errmsg = NULL;
    struct stat fst;
    int idx = 0;
    GList *gl = NULL;

    dbf = g_build_filename(outpath, "Dynamic.itdb", NULL);
    printf("[%s] Processing '%s'\n", __func__, dbf);
    if (stat(dbf, &fst) != 0) {
	if (errno == ENOENT) {
	    /* file is not present. so we'll create it */
	} else {
	    fprintf(stderr, "[%s] Error: stat: %s\n", __func__, strerror(errno));
	    goto leave;
	}
    } else {
	/* file is present. delete it, we'll re-create it. */
	if (unlink(dbf) != 0) {
	    fprintf(stderr, "[%s] could not delete '%s': %s\n", __func__, dbf, strerror(errno));
	    goto leave;
	}
    }

    if (SQLITE_OK != sqlite3_open((const char*)dbf, &db)) {
	fprintf(stderr, "Error opening database '%s': %s\n", dbf, sqlite3_errmsg(db));
	goto leave;
    }

    sqlite3_exec(db, "PRAGMA synchronous = OFF;", NULL, NULL, NULL);

    fprintf(stderr, "[%s] creating table structure\n", __func__);
    /* db structure needs to be created. */
    if (SQLITE_OK != sqlite3_exec(db, Dynamic_create, NULL, NULL, &errmsg)) {
	fprintf(stderr, "[%s] sqlite3_exec error: %s\n", __func__, sqlite3_errmsg(db));
	if (errmsg) {
	    fprintf(stderr, "[%s] additional error information: %s\n", __func__, errmsg);
	    sqlite3_free(errmsg);
	    errmsg = NULL;
	}
	goto leave;
    }

    sqlite3_exec(db, "BEGIN;", NULL, NULL, NULL);

    if (itdb->tracks) {
	printf("[%s] - processing %d tracks\n", __func__, g_list_length(itdb->tracks));
	if (SQLITE_OK != sqlite3_prepare_v2(db, "INSERT INTO \"item_stats\" (item_pid,has_been_played,date_played,play_count_user,play_count_recent,date_skipped,skip_count_user,skip_count_recent,bookmark_time_ms,bookmark_time_ms_common,user_rating,user_rating_common) VALUES(?,?,?,?,?,?,?,?,?,?,?,?);", -1, &stmt, NULL)) {
	    fprintf(stderr, "[%s] sqlite3_prepare error: %s\n", __func__, sqlite3_errmsg(db));
	    goto leave;
	}
	for (gl=itdb->tracks; gl; gl=gl->next) {
	    Itdb_Track *track = gl->data;
	    if (!track->ipod_path) {
		continue;
	    }
	    res = sqlite3_reset(stmt);
	    if (res != SQLITE_OK) {
		fprintf(stderr, "[%s] 1 sqlite3_reset returned %d\n", __func__, res);
	    }
	    idx = 0;
	    /* item_pid */
	    sqlite3_bind_int64(stmt, ++idx, track->dbid);
	    /* has_been_played */
	    sqlite3_bind_int(stmt, ++idx, (track->playcount > 0) ? 1 : 0);
	    /* date_played */
	    sqlite3_bind_int(stmt, ++idx, timeconv(track->time_played));
	    /* play_count_user */
	    sqlite3_bind_int(stmt, ++idx, track->playcount);
	    /* play_count_recent */
	    sqlite3_bind_int(stmt, ++idx, track->recent_playcount);
	    /* date_skipped */
	    sqlite3_bind_int(stmt, ++idx, timeconv(track->last_skipped));
	    /* skip_count_user */
	    sqlite3_bind_int(stmt, ++idx, track->skipcount);
	    /* skip_count_recent */
	    sqlite3_bind_int(stmt, ++idx, track->recent_skipcount);
	    /* bookmark_time_ms */
	    sqlite3_bind_double(stmt, ++idx, track->bookmark_time);
	    /* bookmark_time_ms_common */
	    /* FIXME: is this the same as bookmark_time_ms ??! */
	    sqlite3_bind_double(stmt, ++idx, track->bookmark_time);
	    /* user_rating */
	    sqlite3_bind_int(stmt, ++idx, track->rating);
	    /* user_rating_common */
	    /* FIXME: don't know if this is the right value */
	    sqlite3_bind_int(stmt, ++idx, track->app_rating);

	    res = sqlite3_step(stmt);
	    if (res == SQLITE_DONE) {
		/* expected result */
	    } else {
		fprintf(stderr, "[%s] 1 sqlite3_step returned %d\n", __func__, res);
	    }
	}
	if (stmt) {
	    sqlite3_finalize(stmt);
	    stmt = NULL;
	}
    } else {
	printf("[%s] - No tracks available, none written.\n", __func__);
    }

    /* add playlist info to container_ui */
    if (SQLITE_OK != sqlite3_prepare_v2(db, "INSERT INTO \"container_ui\" VALUES(?,?,?,?,?,?,?);", -1, &stmt, NULL)) {
	    fprintf(stderr, "[%s] 2 sqlite3_prepare error: %s\n", __func__, sqlite3_errmsg(db));
	    goto leave;
    }

    printf("[%s] - processing %d playlists\n", __func__, g_list_length(itdb->playlists));
    for (gl = itdb->playlists; gl; gl = gl->next) {
	Itdb_Playlist *pl = (Itdb_Playlist*)gl->data;

	res = sqlite3_reset(stmt);
	if (res != SQLITE_OK) {
	    fprintf(stderr, "[%s] 2 sqlite3_reset returned %d\n", __func__, res);
	}
	idx = 0;
	/* container_pid */
	sqlite3_bind_int64(stmt, ++idx, pl->id);
	/* play_order TODO: where does this value come from? */
	/* FIXME: 5 --> 7, 24 --> 39 ??!? */
	switch (pl->sortorder) {
	    case 5:
		sqlite3_bind_int(stmt, ++idx, 7);
		break;
	    case 24:
		sqlite3_bind_int(stmt, ++idx, 39);
		break;
	    default:
		sqlite3_bind_int(stmt, ++idx, 7);
		break;
	}
	/* is_reversed TODO where does this value come from? */
	sqlite3_bind_int(stmt, ++idx, 0);
	/* album_field_order TODO where does this value come from? */
	sqlite3_bind_int(stmt, ++idx, 1);
	/* repeat_mode TODO where does this value come from? */
	sqlite3_bind_int(stmt, ++idx, 0);
	/* shuffle_items TODO where does this value come from? */
	sqlite3_bind_int(stmt, ++idx, 0);
	/* has_been_shuffled TODO where does this value come from? */
	sqlite3_bind_int(stmt, ++idx, 0);

	res = sqlite3_step(stmt);
	if (res == SQLITE_DONE) {
	    /* expected result */
	} else {
	    fprintf(stderr, "[%s] 2 sqlite3_step returned %d\n", __func__, res);
	}
    }

    sqlite3_exec(db, "COMMIT;", NULL, NULL, NULL);

    if (stmt) {
	sqlite3_finalize(stmt);
	stmt = NULL;
    }

    res = 0;
    printf("[%s] done.\n", __func__);
leave:
    if (db) {
	sqlite3_close(db);
    }
    if (dbf) {
	g_free(dbf);
    }
    return res;
}

static int mk_Extras(Itdb_iTunesDB *itdb, const char *outpath)
{
    int res = -1;
    int rebuild = 0;
    gchar *dbf = NULL;
    sqlite3 *db = NULL;
    sqlite3_stmt *stmt_chapter = NULL;
    char *errmsg = NULL;
    struct stat fst;
    GList *gl = NULL;

    dbf = g_build_filename(outpath, "Extras.itdb", NULL);
    printf("[%s] Processing '%s'\n", __func__, dbf);
    if (stat(dbf, &fst) != 0) {
	if (errno == ENOENT) {
	    /* file is not present. so we need to create the tables in it ;) */
	    rebuild = 1;
	} else {
	    fprintf(stderr, "[%s] Error: stat: %s\n", __func__, strerror(errno));
	    goto leave;
	}
    }

    if (SQLITE_OK != sqlite3_open((const char*)dbf, &db)) {
	fprintf(stderr, "Error opening database '%s': %s\n", dbf, sqlite3_errmsg(db));
	goto leave;
    }

    sqlite3_exec(db, "PRAGMA synchronous = OFF;", NULL, NULL, NULL);

    if (rebuild) {
	fprintf(stderr, "[%s] re-building table structure\n", __func__);
	/* db structure needs to be created. */
	if (SQLITE_OK != sqlite3_exec(db, Extras_create, NULL, NULL, &errmsg)) {
	    fprintf(stderr, "[%s] sqlite3_exec error: %s\n", __func__, sqlite3_errmsg(db));
	    if (errmsg) {
		fprintf(stderr, "[%s] additional error information: %s\n", __func__, errmsg);
		sqlite3_free(errmsg);
		errmsg = NULL;
	    }
	    goto leave;
	}
    }

    sqlite3_exec(db, "BEGIN;", NULL, NULL, NULL);
    if (SQLITE_OK != sqlite3_prepare_v2(db, "INSERT INTO \"chapter\" VALUES(?,?);", -1, &stmt_chapter, NULL)) {
	fprintf(stderr, "[%s] sqlite3_prepare error: %s\n", __func__, sqlite3_errmsg(db));
	goto leave;
    }

    /* kill all entries in 'chapter' as they will be re-inserted */
    if (SQLITE_OK != sqlite3_exec(db, "DELETE FROM chapter;", NULL, NULL, &errmsg)) {
	fprintf(stderr, "[%s] sqlite3_exec error: %s\n", __func__, sqlite3_errmsg(db));
	if (errmsg) {
	    fprintf(stderr, "[%s] additional error information: %s\n", __func__, errmsg);
	    sqlite3_free(errmsg);
	    errmsg = NULL;
	}
	goto leave;
    }

    /* kill all entries in 'lyrics' as they will be re-inserted */
    /* TODO: we do not support this in the moment, so don't touch this */
    /*if (SQLITE_OK != sqlite3_exec(db, "DELETE FROM lyrics;", NULL, NULL, &errmsg)) {
	fprintf(stderr, "[%s] sqlite3_exec error: %s\n", __func__, sqlite3_errmsg(db));
	if (errmsg) {
	    fprintf(stderr, "[%s] additional error information: %s\n", __func__, errmsg);
	    sqlite3_free(errmsg);
	    errmsg = NULL;
	}
	goto leave;
    }*/

    for (gl = itdb->tracks; gl; gl = gl->next) {
	Itdb_Track *track = gl->data;
	if (track->chapterdata) {
	    int idx = 0;
	    GByteArray *chapter_blob = itdb_chapterdata_build_chapter_blob(track->chapterdata, FALSE);
	    /* printf("[%s] -- inserting into \"chapter\"\n", __func__); */
	    res = sqlite3_reset(stmt_chapter);
	    if (res != SQLITE_OK) {
		fprintf(stderr, "[%s] 1 sqlite3_reset returned %d\n", __func__, res);
	    }
	    /* item_pid INTEGER NOT NULL */
	    sqlite3_bind_int64(stmt_chapter, ++idx, track->dbid);
	    /* data BLOB */
	    sqlite3_bind_blob(stmt_chapter, ++idx, chapter_blob->data, chapter_blob->len, SQLITE_TRANSIENT);

	    res = sqlite3_step(stmt_chapter);
	    if (res == SQLITE_DONE) {
		/* expected result */
	    } else {
		fprintf(stderr, "[%s] 8 sqlite3_step returned %d\n", __func__, res);
	    }
	    g_byte_array_free(chapter_blob, TRUE);
	}
    }
    sqlite3_exec(db, "COMMIT;", NULL, NULL, NULL);

    res = 0;
    printf("[%s] done.\n", __func__);
leave:
    if (stmt_chapter) {
	sqlite3_finalize(stmt_chapter);
    }
    if (db) {
	sqlite3_close(db);
    }
    if (dbf) {
	g_free(dbf);
    }
    return res;
}

static int mk_Genius(Itdb_iTunesDB *itdb, const char *outpath)
{
    int res = -1;
    int rebuild = 0;
    gchar *dbf = NULL;
    sqlite3 *db = NULL;
    char *errmsg = NULL;
    struct stat fst;

    dbf = g_build_filename(outpath, "Genius.itdb", NULL);
    printf("[%s] Processing '%s'\n", __func__, dbf);
    if (stat(dbf, &fst) != 0) {
	if (errno == ENOENT) {
	    /* file is not present. so we need to create the tables in it ;) */
	    rebuild = 1;
	} else {
	    fprintf(stderr, "[%s] Error: stat: %s\n", __func__, strerror(errno));
	    goto leave;
	}
    }

    if (SQLITE_OK != sqlite3_open((const char*)dbf, &db)) {
	fprintf(stderr, "Error opening database '%s': %s\n", dbf, sqlite3_errmsg(db));
	goto leave;
    }

    sqlite3_exec(db, "PRAGMA synchronous = OFF;", NULL, NULL, NULL);

    if (rebuild) {
	fprintf(stderr, "[%s] re-building table structure\n", __func__);
	/* db structure needs to be created. */
	if (SQLITE_OK != sqlite3_exec(db, Genius_create, NULL, NULL, &errmsg)) {
	    fprintf(stderr, "[%s] sqlite3_exec error: %s\n", __func__, sqlite3_errmsg(db));
	    if (errmsg) {
		fprintf(stderr, "[%s] additional error information: %s\n", __func__, errmsg);
		sqlite3_free(errmsg);
		errmsg = NULL;
	    }
	    goto leave;
	}
    }

    sqlite3_exec(db, "BEGIN;", NULL, NULL, NULL);

    /* kill all entries in 'genius_config' as they will be re-inserted */
    /* TODO: we do not support this in the moment, so don't touch this */
    /*if (SQLITE_OK != sqlite3_exec(db, "DELETE FROM genius_config;", NULL, NULL, &errmsg)) {
	fprintf(stderr, "[%s] sqlite3_exec error: %s\n", __func__, sqlite3_errmsg(db));
	if (errmsg) {
	    fprintf(stderr, "[%s] additional error information: %s\n", __func__, errmsg);
	    sqlite3_free(errmsg);
	    errmsg = NULL;
	}
	goto leave;
    }*/

    /* kill all entries in 'genius_metadata' as they will be re-inserted */
    /* TODO: we do not support this in the moment, so don't touch this */
    /*if (SQLITE_OK != sqlite3_exec(db, "DELETE FROM genius_metadata;", NULL, NULL, &errmsg)) {
	fprintf(stderr, "[%s] sqlite3_exec error: %s\n", __func__, sqlite3_errmsg(db));
	if (errmsg) {
	    fprintf(stderr, "[%s] additional error information: %s\n", __func__, errmsg);
	    sqlite3_free(errmsg);
	    errmsg = NULL;
	}
	goto leave;
    }*/

    /* kill all entries in 'genius_similarities' as they will be re-inserted */
    /* TODO: we do not support this in the moment, so don't touch this */
    /*if (SQLITE_OK != sqlite3_exec(db, "DELETE FROM genius_similarities;", NULL, NULL, &errmsg)) {
	fprintf(stderr, "[%s] sqlite3_exec error: %s\n", __func__, sqlite3_errmsg(db));
	if (errmsg) {
	    fprintf(stderr, "[%s] additional error information: %s\n", __func__, errmsg);
	    sqlite3_free(errmsg);
	    errmsg = NULL;
	}
	goto leave;
    }*/

    sqlite3_exec(db, "COMMIT;", NULL, NULL, NULL);

    res = 0;
    printf("[%s] done.\n", __func__);
leave:
    if (db) {
	sqlite3_close(db);
    }
    if (dbf) {
	g_free(dbf);
    }
    return res;
}

static void free_key_val_strings(gpointer key, gpointer value, gpointer user_data)
{
	g_free(key);
	g_free(value);
}

static void bind_first_text(sqlite3_stmt *stmt, int idx, int n, ...)
{
    va_list ap;
    int i;

    va_start(ap, n);
    for (i = 0; i < n; i++) {
        char *str = va_arg(ap, char*);
        if (str && *str) {
	    sqlite3_bind_text(stmt, idx, str, -1, SQLITE_STATIC);
            goto done;
        }
    }
    sqlite3_bind_null(stmt, idx);
done:
    va_end(ap);
}

static int mk_Library(Itdb_iTunesDB *itdb,
		       GHashTable *album_ids, GHashTable *artist_ids,
		       GHashTable *composer_ids, const char *outpath)
{
    int res = -1;
    gchar *dbf = NULL;
    sqlite3 *db = NULL;
    sqlite3_stmt *stmt_version_info = NULL;
    sqlite3_stmt *stmt_db_info = NULL;
    sqlite3_stmt *stmt_container = NULL;
    sqlite3_stmt *stmt_genre_map = NULL;
    sqlite3_stmt *stmt_item = NULL;
    sqlite3_stmt *stmt_location_kind_map = NULL;
    sqlite3_stmt *stmt_avformat_info = NULL;
    sqlite3_stmt *stmt_item_to_container = NULL;
    sqlite3_stmt *stmt_album = NULL;
    sqlite3_stmt *stmt_artist = NULL;
    sqlite3_stmt *stmt_composer = NULL;
    sqlite3_stmt *stmt_video_info = NULL;
    char *errmsg = NULL;
    struct stat fst;
    GList *gl = NULL;
    Itdb_Playlist *dev_playlist = NULL;
    int idx = 0;
    int pos = 0;
    GHashTable *genre_map;
    GHashTable **orders = NULL;
    guint32 genre_index;
    printf("library_persistent_id = 0x%016"G_GINT64_MODIFIER"x\n", itdb->priv->pid);

    dbf = g_build_filename(outpath, "Library.itdb", NULL);
    printf("[%s] Processing '%s'\n", __func__, dbf);
    if (stat(dbf, &fst) != 0) {
	if (errno == ENOENT) {
	    /* file is not present. so we will create it */
	} else {
	    fprintf(stderr, "[%s] Error: stat: %s\n", __func__, strerror(errno));
	    goto leave;
	}
    } else {
	/* file is present. delete it, we'll re-create it. */
	if (unlink(dbf) != 0) {
	    fprintf(stderr, "[%s] could not delete '%s': %s\n", __func__, dbf, strerror(errno));
	    goto leave;
	}
    }

    if (SQLITE_OK != sqlite3_open((const char*)dbf, &db)) {
	fprintf(stderr, "Error opening database '%s': %s\n", dbf, sqlite3_errmsg(db));
	goto leave;
    }

    sqlite3_exec(db, "PRAGMA synchronous = OFF;", NULL, NULL, NULL);

    fprintf(stderr, "[%s] building table structure\n", __func__);
    /* db structure needs to be created. */
    if (SQLITE_OK != sqlite3_exec(db, Library_create, NULL, NULL, &errmsg)) {
	fprintf(stderr, "[%s] sqlite3_exec error: %s\n", __func__, sqlite3_errmsg(db));
	if (errmsg) {
	    fprintf(stderr, "[%s] additional error information: %s\n", __func__, errmsg);
	    sqlite3_free(errmsg);
	    errmsg = NULL;
	}
	goto leave;
    }

    sqlite3_exec(db, "BEGIN;", NULL, NULL, NULL);

    fprintf(stderr, "[%s] compiling SQL statements\n", __func__);
    if (SQLITE_OK != sqlite3_prepare_v2(db, "INSERT INTO \"version_info\" VALUES(?,?,?,?,?,?,?);", -1, &stmt_version_info, NULL)) {
	fprintf(stderr, "[%s] sqlite3_prepare error: %s\n", __func__, sqlite3_errmsg(db));
	goto leave;
    }
    if (SQLITE_OK != sqlite3_prepare_v2(db, "INSERT INTO \"db_info\" VALUES(?,?,?,?,?,?,?,?);", -1, &stmt_db_info, NULL)) {
	fprintf(stderr, "[%s] sqlite3_prepare error: %s\n", __func__, sqlite3_errmsg(db));
	goto leave;
    }
    if (SQLITE_OK != sqlite3_prepare_v2(db, "INSERT INTO \"container\" "
	    "VALUES(?,?,?,?,:name,?,?,?,?,?,?,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL);", -1, &stmt_container, NULL)) {
	fprintf(stderr, "[%s] sqlite3_prepare error: %s\n", __func__, sqlite3_errmsg(db));
	goto leave;
    }
    if (SQLITE_OK != sqlite3_prepare_v2(db, "INSERT INTO \"genre_map\" (id,genre,genre_order) VALUES(?,?,0);", -1, &stmt_genre_map, NULL)) {
	fprintf(stderr, "[%s] sqlite3_prepare error: %s\n", __func__, sqlite3_errmsg(db));
	goto leave;
    }
    if (SQLITE_OK != sqlite3_prepare_v2(db, "INSERT INTO \"item\" "
	    "(pid,media_kind,date_modified,year,is_compilation,remember_bookmark,exclude_from_shuffle,artwork_status,artwork_cache_id,"
	    "start_time_ms,stop_time_ms,total_time_ms,total_burn_time_ms,track_number,track_count,disc_number,disc_count,"
	    "bpm,relative_volume,eq_preset,radio_stream_status,genre_id,album_pid,artist_pid,composer_pid,title,artist,album,"
	    "album_artist,composer,sort_title,sort_artist,sort_album,sort_album_artist,sort_composer,title_order,artist_order,"
	    "album_order,genre_order,composer_order,album_artist_order,album_by_artist_order,series_name_order,comment,grouping,"
	    "description,description_long) "
	    "VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?);", -1, &stmt_item, NULL)) {
	fprintf(stderr, "[%s] sqlite3_prepare error: %s\n", __func__, sqlite3_errmsg(db));
	goto leave;
    }
    if (SQLITE_OK != sqlite3_prepare_v2(db, "INSERT OR IGNORE INTO \"location_kind_map\" VALUES(?,:kind);", -1, &stmt_location_kind_map, NULL)) {
	fprintf(stderr, "[%s] sqlite3_prepare error: %s\n", __func__, sqlite3_errmsg(db));
	goto leave;
    }
    if (SQLITE_OK != sqlite3_prepare_v2(db, "INSERT INTO \"avformat_info\" VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?);", -1, &stmt_avformat_info, NULL)) {
	fprintf(stderr, "[%s] sqlite3_prepare error: %s\n", __func__, sqlite3_errmsg(db));
	goto leave;
    }
    if (SQLITE_OK != sqlite3_prepare_v2(db, "INSERT INTO \"item_to_container\" VALUES(?,?,?,?);", -1, &stmt_item_to_container, NULL)) {
	fprintf(stderr, "[%s] sqlite3_prepare error: %s\n", __func__, sqlite3_errmsg(db));
	goto leave;
    }
    if (SQLITE_OK != sqlite3_prepare_v2(db, "INSERT OR IGNORE INTO \"album\" VALUES(?,?,?,?,?,?,?,?,?,?,?);", -1, &stmt_album, NULL)) {
	fprintf(stderr, "[%s] sqlite3_prepare error: %s\n", __func__, sqlite3_errmsg(db));
	goto leave;
    }
    if (SQLITE_OK != sqlite3_prepare_v2(db, "INSERT OR IGNORE INTO \"artist\" VALUES(?,?,?,?,?,?,?);", -1, &stmt_artist, NULL)) {
	fprintf(stderr, "[%s] sqlite3_prepare error: %s\n", __func__, sqlite3_errmsg(db));
	goto leave;
    }
    if (SQLITE_OK != sqlite3_prepare_v2(db, "INSERT OR IGNORE INTO \"composer\" (pid,name,name_order,sort_name) VALUES(?,?,?,?);", -1, &stmt_composer, NULL)) {
	fprintf(stderr, "[%s] sqlite3_prepare error: %s\n", __func__, sqlite3_errmsg(db));
	goto leave;
    }
    if (SQLITE_OK != sqlite3_prepare_v2(db, "INSERT INTO \"video_info\" VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?);", -1, &stmt_video_info, NULL)) {
	fprintf(stderr, "[%s] sqlite3_prepare error: %s\n", __func__, sqlite3_errmsg(db));
	goto leave;
    }

    printf("[%s] - inserting into \"version_info\"\n", __func__);
    /* INSERT INTO "version_info" VALUES(1,2,40,0,0,0,2); */
    idx = 0;
    /* id */
    sqlite3_bind_int(stmt_version_info, ++idx, 1);
    /* major */
    /* FIXME: the versioning scheme needs to be updated in libgpod using */
    /* major+minor version, this is a workaround */
    sqlite3_bind_int(stmt_version_info, ++idx, (itdb->version >= 0x28) ? 2 : 1);
    /* minor */
    sqlite3_bind_int(stmt_version_info, ++idx, itdb->version);
    /* compatibility */
    /* This has an unknown meaning. use 0. */
    sqlite3_bind_int(stmt_version_info, ++idx, 0);
    /* update_level */
    /* This has an unknown meaning. use 0. */
    sqlite3_bind_int(stmt_version_info, ++idx, 0);
    /* device_update_level, default is 0 */
    sqlite3_bind_int(stmt_version_info, ++idx, 0);
    /* platform, 1 = MacOS, 2 = Windows */
    sqlite3_bind_int(stmt_version_info, ++idx, itdb->priv->platform);

    res = sqlite3_step(stmt_version_info);
    if (res == SQLITE_DONE) {
	/* expected result */
    } else {
	fprintf(stderr, "[%s] 2 sqlite3_step returned %d\n", __func__, res);
    }

    printf("[%s] - inserting into \"genre_map\"\n", __func__);
    genre_map = g_hash_table_new(g_str_hash, g_str_equal);

    /* build genre_map */
    genre_index = 1;
    for (gl = itdb->tracks; gl; gl = gl->next) {
	Itdb_Track *track = gl->data;
	if ((track->genre == NULL) || (g_hash_table_lookup(genre_map, track->genre) != NULL))
		continue;
	g_hash_table_insert(genre_map, track->genre, GUINT_TO_POINTER(genre_index));

	res = sqlite3_reset(stmt_genre_map);
	if (res != SQLITE_OK) {
	    fprintf(stderr, "[%s] 1 sqlite3_reset returned %d\n", __func__, res);
	}

	idx = 0;
	/* id */
	sqlite3_bind_int(stmt_genre_map, ++idx, genre_index++);
	/* genre */
	sqlite3_bind_text(stmt_genre_map, ++idx, track->genre, -1, SQLITE_STATIC);

	res = sqlite3_step(stmt_genre_map);
	if (res != SQLITE_DONE) {
	    fprintf(stderr, "[%s] sqlite3_step returned %d\n", __func__, res);
	    goto leave;
	}
    }

    pos = 0;

    for (gl = itdb->playlists; gl; gl = gl->next) {
	int tpos = 0;
	GList *glt = NULL;
	guint32 types = 0;
	Itdb_Playlist *pl = (Itdb_Playlist*)gl->data;

	if (pl->type == 1) {
	    dev_playlist = pl;
	}

	printf("[%s] - inserting songs into \"item_to_container\"\n", __func__);

	for (glt = pl->members; glt; glt = glt->next) {
	    Itdb_Track *track = glt->data;

	    /* printf("[%s] -- inserting into \"item_to_container\"\n", __func__); */
	    res = sqlite3_reset(stmt_item_to_container);
	    if (res != SQLITE_OK) {
		fprintf(stderr, "[%s] 1 sqlite3_reset returned %d\n", __func__, res);
	    }	/* INSERT INTO "item_to_container" VALUES(-6197982141081478573,959107999841118509,0,NULL); */
	    idx = 0;
	    /* item_pid */
	    sqlite3_bind_int64(stmt_item_to_container, ++idx, track->dbid);
	    /* container_pid */
	    sqlite3_bind_int64(stmt_item_to_container, ++idx, pl->id);
	    /* physical_order */
	    sqlite3_bind_int(stmt_item_to_container, ++idx, tpos++);
	    /* shuffle_order */
	    /* TODO what's this? set to NULL as iTunes does */
	    sqlite3_bind_null(stmt_item_to_container, ++idx);

	    res = sqlite3_step(stmt_item_to_container);
	    if (res == SQLITE_DONE) {
		/* expected result */
	    } else {
		fprintf(stderr, "[%s] 8 sqlite3_step returned %d\n", __func__, res);
	    }

	    types |= track->mediatype;
	}

	printf("[%s] - inserting playlist '%s' into \"container\"\n", __func__, pl->name);
	res = sqlite3_reset(stmt_container);
	if (res != SQLITE_OK) {
	    fprintf(stderr, "[%s] 1 sqlite3_reset returned %d\n", __func__, res);
	}
	/* INSERT INTO "container" VALUES(959107999841118509,0,267295295,'Hamouda',400,0,1,0,1,0,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL); */
	idx = 0;
	/* pid */
	sqlite3_bind_int64(stmt_container, ++idx, pl->id);
	/* distinguished_kind */
	/* TODO: more values? */
	if (pl->podcastflag) {
	    sqlite3_bind_int(stmt_container, ++idx, 11);
	} else {
	    sqlite3_bind_int(stmt_container, ++idx, 0);
	}
	/* date_created */
	sqlite3_bind_int(stmt_container, ++idx, timeconv(pl->timestamp));
	/* date_modified */
	sqlite3_bind_int(stmt_container, ++idx, timeconv(pl->timestamp));
	/* name */
	sqlite3_bind_text(stmt_container, ++idx, pl->name, -1, SQLITE_STATIC);
	/* name order */
	sqlite3_bind_int(stmt_container, ++idx, pos++);
	/* parent_pid */
	/* TODO: unkown meaning, always 0? */
	sqlite3_bind_int(stmt_container, ++idx, 0);
	/* media_kinds */
	sqlite3_bind_int(stmt_container, ++idx, types);
	/* workout_template_id */
	/* TODO: seems to be always 0 */
	sqlite3_bind_int(stmt_container, ++idx, 0);
	/* is_hidden */
	/* 1 for dev and podcasts, 0 elsewhere */
	if (pl->podcastflag || pl->type == 1) {
	    sqlite3_bind_int(stmt_container, ++idx, 1);
	} else {
	    sqlite3_bind_int(stmt_container, ++idx, 0);
	}
	/* smart_is_folder */
	/* TODO: smart playlist stuff? */
	sqlite3_bind_int(stmt_container, ++idx, 0);
	/* iTunes leaves everything else NULL for normal playlists */

	res = sqlite3_step(stmt_container);
	if (res == SQLITE_DONE) {
	    /* expected result */
	} else {
	    fprintf(stderr, "[%s] 4 sqlite3_step returned %d\n", __func__, res);
	}
    }

    if (!dev_playlist) {
	fprintf(stderr, "Could not find special device playlist!\n");
	goto leave;
    }
    printf("library_persistent_id = 0x%016"G_GINT64_MODIFIER"x\n", itdb->priv->pid);

    if (!dev_playlist->name) {
	fprintf(stderr, "Could not fetch device name from itdb!\n");
	goto leave;
    }
    printf("device name = %s\n", dev_playlist->name);

    printf("[%s] - inserting into \"db_info\"\n", __func__);
    idx = 0;
    /* pid */
    sqlite3_bind_int64(stmt_db_info, ++idx, itdb->priv->pid);
    /* primary_container_pid */
    /* ... stored in the playlists where the device name is stored too. */
    sqlite3_bind_int64(stmt_db_info, ++idx, dev_playlist->id);
    /* media_folder_url */
    sqlite3_bind_null(stmt_db_info, ++idx);
    /* audio_language  */
    /*  this is +0xA0 */
    sqlite3_bind_int(stmt_db_info, ++idx, itdb->priv->audio_language);
    /* subtitle_language */
    /*  this is +0xA2 */
    sqlite3_bind_int(stmt_db_info, ++idx, itdb->priv->subtitle_language);
    /* genius cuid */
    /* TODO: unkown meaning, set to NULL */
    sqlite3_bind_null(stmt_db_info, ++idx);
    /* bib */
    /* TODO: unkown meaning, set to NULL */
    sqlite3_bind_null(stmt_db_info, ++idx);
    /* rib */
    /* TODO: unkown meaning, set to NULL */
    sqlite3_bind_null(stmt_db_info, ++idx);

    res = sqlite3_step(stmt_db_info);
    if (res == SQLITE_DONE) {
	/* expected result */
    } else {
	fprintf(stderr, "[%s] 3 sqlite3_step returned %d\n", __func__, res);
    }

    /* for each track: */
    printf("[%s] - processing %d tracks\n", __func__, g_list_length(itdb->tracks));
    orders = compute_key_orders(itdb->tracks);

    for (gl = itdb->tracks; gl; gl = gl->next) {
	Itdb_Track *track = gl->data;
	Itdb_Item_Id *this_album = NULL;
	Itdb_Item_Id *this_artist = NULL;
	Itdb_Item_Id *this_composer = NULL;
	uint64_t album_pid = 0;
	uint64_t artist_pid = 0;
	uint64_t composer_pid = 0;
	gpointer genre_id = NULL;
	int audio_format;
	int aw_id;

	/* printf("[%s] -- inserting into \"item\"\n", __func__); */
	res = sqlite3_reset(stmt_item);
	if (res != SQLITE_OK) {
	    fprintf(stderr, "[%s] 1 sqlite3_reset returned %d\n", __func__, res);
	}
	/* INSERT INTO "item" VALUES(-6197982141081478573,NULL,1,1,0,0,0,0,0,0,0,0,0,0,2004,0,0,0,0,0,0,2,0,0.0,0.0,213812.0,NULL,1,0,0,0,0,0,NULL,NULL,0,0,0,-7670716306765259718,-7576753259813584028,0,'Enjoy the silence 2004 (Reinterpreted by Mike Shinoda)','Depeche Mode','Enjoy The Silence 04 CDS','Depeche Mode',NULL,'Enjoy the silence 2004 (Reinterpreted by Mike Shinoda)','Depeche Mode','Enjoy The Silence 04 CDS','Depeche Mode',NULL,100,100,100,100,100,100,NULL,100,'Lbl MUTE C:MUTE RCDBONG034',NULL,NULL,NULL,1,0,0,0,0,1,1,NULL,NULL,NULL,NULL,NULL,NULL,NULL); */
	idx = 0;
	/* pid */
	sqlite3_bind_int64(stmt_item, ++idx, track->dbid);
	/* media_kind */
	/* NOTE: this is one of  */
	/*  song = 1 */
	/*  audio_book = 8 */
	/*  music_video = 32 */
	/*  movie = 2 */
	/*  tv_show = 64 */
	/*  ringtone = 16384 */
	/*  podcast = 4 */
	/*  rental = 32768 */
	sqlite3_bind_int(stmt_item, ++idx, track->mediatype);
	/* date_modified */
	sqlite3_bind_int(stmt_item, ++idx, timeconv(track->time_modified));
	/* year */
	sqlite3_bind_int(stmt_item, ++idx, track->year);
	/* is_compilation */
	sqlite3_bind_int(stmt_item, ++idx, track->compilation);
	/* remember_bookmark */
	sqlite3_bind_int(stmt_item, ++idx, track->remember_playback_position);
	/* exclude_from_shuffle */
	sqlite3_bind_int(stmt_item, ++idx, track->skip_when_shuffling);
	/* artwork_status 1 = has artwork, 2 = doesn't */
	sqlite3_bind_int(stmt_item, ++idx, track->has_artwork);
	/* artwork_cache_id */
	/* TODO check if this value is correct */
	aw_id = 0;
	if (track->artwork) {
	    aw_id = track->artwork->id;
	}
	sqlite3_bind_int(stmt_item, ++idx, aw_id);
	/* start_time_ms */
	sqlite3_bind_double(stmt_item, ++idx, track->starttime);
	/* stop_time_ms */
	sqlite3_bind_double(stmt_item, ++idx, track->stoptime);
	/* total_time_ms */
	sqlite3_bind_double(stmt_item, ++idx, track->tracklen);
	/* total_burn_time_ms --> set to NULL */
	/* TODO where is this stored? */
	sqlite3_bind_null(stmt_item, ++idx);
	/* track_number */
	sqlite3_bind_int(stmt_item, ++idx, track->track_nr);
	/* track_count */
	sqlite3_bind_int(stmt_item, ++idx, track->tracks);
	/* disc_number */
	sqlite3_bind_int(stmt_item, ++idx, track->cd_nr);
	/* disc_count */
	sqlite3_bind_int(stmt_item, ++idx, track->cds);
	/* bpm */
	sqlite3_bind_int(stmt_item, ++idx, track->BPM);
	/* relative_volume */
	sqlite3_bind_int(stmt_item, ++idx, track->volume);
	/* eq_preset --> set to NULL */
	sqlite3_bind_null(stmt_item, ++idx);
	/* radio_stream_status --> set to NULL */
	sqlite3_bind_null(stmt_item, ++idx);
	/* genre_id */
	genre_id = NULL;
	if (track->genre && *track->genre && (genre_id = g_hash_table_lookup(genre_map, track->genre)) != NULL) {
	    sqlite3_bind_int(stmt_item, ++idx, GPOINTER_TO_UINT(genre_id));
	}
	else
	    sqlite3_bind_int(stmt_item, ++idx, 0);
	/* album_pid -7670716306765259718 */
	this_album = g_hash_table_lookup(album_ids, track);
	if (this_album) {
	    album_pid = this_album->sql_id;
	}
	sqlite3_bind_int64(stmt_item, ++idx, album_pid);
	/* artist_pid -7576753259813584028 */
	this_artist = g_hash_table_lookup(artist_ids, track);
	if (this_artist) {
	    artist_pid = this_artist->sql_id;
	}
	sqlite3_bind_int64(stmt_item, ++idx, artist_pid);
	/* composer_pid */
	this_composer = g_hash_table_lookup(composer_ids, track);
	if (this_composer) {
	    composer_pid = this_composer->sql_id;
	}
	sqlite3_bind_int64(stmt_item, ++idx, composer_pid);
	/* title */
        bind_first_text(stmt_item, ++idx, 1, track->title);
	/* artist */
        bind_first_text(stmt_item, ++idx, 1, track->artist);
	/* album */
        bind_first_text(stmt_item, ++idx, 1, track->album);
	/* album_artist */
        bind_first_text(stmt_item, ++idx, 1, track->albumartist);
	/* composer */
        bind_first_text(stmt_item, ++idx, 1, track->composer);
	/* sort_title */
        bind_first_text(stmt_item, ++idx, 2, track->sort_title, track->title);
	/* sort_artist */
        bind_first_text(stmt_item, ++idx, 2, track->sort_artist, track->artist);
	/* sort_album */
        bind_first_text(stmt_item, ++idx, 2, track->sort_album, track->album);
	/* sort_album_artist */
        bind_first_text(stmt_item, ++idx, 2, track->sort_albumartist, track->albumartist);
	/* sort_composer */
        bind_first_text(stmt_item, ++idx, 2, track->sort_composer, track->composer);

	/* TODO figure out where these values are stored */
	/* title_order */
	sqlite3_bind_int(stmt_item, ++idx, lookup_order(orders, ORDER_TITLE, track));
	/* artist_order */
	sqlite3_bind_int(stmt_item, ++idx, lookup_order(orders, ORDER_ARTIST, track));
	/* album_order */
	sqlite3_bind_int(stmt_item, ++idx, lookup_order(orders, ORDER_ALBUM, track));
	/* genre_order */
	sqlite3_bind_int(stmt_item, ++idx, lookup_order(orders, ORDER_GENRE, track));
	/* composer_order */
	sqlite3_bind_int(stmt_item, ++idx, lookup_order(orders, ORDER_COMPOSER, track));
	/* album_artist_order */
	sqlite3_bind_int(stmt_item, ++idx, lookup_order(orders, ORDER_ALBUM_ARTIST, track));
	/* album_by_artist_order */
	sqlite3_bind_int(stmt_item, ++idx, lookup_order(orders, ORDER_ALBUM_BY_ARTIST, track));
	/* series_name_order */
	sqlite3_bind_int(stmt_item, ++idx, lookup_order(orders, ORDER_SERIES_NAME, track));
	/* comment */
	sqlite3_bind_text(stmt_item, ++idx, track->comment, -1, SQLITE_STATIC);
	/* grouping */
        bind_first_text(stmt_item, ++idx, 1, track->grouping);
	/* description */
        bind_first_text(stmt_item, ++idx, 1, track->description);
	/* description_long */
	/* TODO libgpod doesn't know about it */
	sqlite3_bind_null(stmt_item, ++idx);

	res = sqlite3_step(stmt_item);
	if (res != SQLITE_DONE) {
	    fprintf(stderr, "[%s] 6 sqlite3_step returned %d\n", __func__, res);
	    goto leave;
	}

	/* printf("[%s] -- inserting into \"location_kind_map\" (if required)\n", __func__); */
	res = sqlite3_reset(stmt_location_kind_map);
	if (res != SQLITE_OK) {
	    fprintf(stderr, "[%s] 1 sqlite3_reset returned %d\n", __func__, res);
	}	/* INSERT INTO "location_kind_map" VALUES(1,'MPEG-Audiodatei'); */
	idx = 0;
	/* id */
	sqlite3_bind_int(stmt_location_kind_map, ++idx, track->mediatype);
	/* kind */
	sqlite3_bind_text(stmt_location_kind_map, ++idx, track->filetype, -1, SQLITE_STATIC);

	res = sqlite3_step(stmt_location_kind_map);
	if (res == SQLITE_DONE) {
	    /* expected result */
	} else {
	    fprintf(stderr, "[%s] 5 sqlite3_step returned %d: %s\n", __func__, res, sqlite3_errmsg(db));
	}

	/* printf("[%s] -- inserting into \"avformat_info\"\n", __func__); */
	res = sqlite3_reset(stmt_avformat_info);
	if (res != SQLITE_OK) {
	    fprintf(stderr, "[%s] 1 sqlite3_reset returned %d\n", __func__, res);
	}	/* INSERT INTO "avformat_info" VALUES(-6197982141081478573,0,301,232,44100.0,9425664,1,576,2880,6224207,0,0,0); */
	idx = 0;
	/* item_pid */
	sqlite3_bind_int64(stmt_avformat_info, ++idx, track->dbid);
	/* sub_id */
	/* TODO what is this? set to 0 */
	sqlite3_bind_int64(stmt_avformat_info, ++idx, 0);
	/* audio_format, TODO what's this? */
	switch (track->filetype_marker) {
	    case 0x4d503320:
		audio_format = 301;
		break;
	    case 0x4d344120:
	    case 0x4d345220:
	    case 0x4d503420:
		audio_format = 502;
		break;
	    default:
		audio_format = 0;
	}
	sqlite3_bind_int(stmt_avformat_info, ++idx, audio_format);
	/* bit_rate */
	sqlite3_bind_int(stmt_avformat_info, ++idx, track->bitrate);
	/* sample_rate */
	sqlite3_bind_double(stmt_avformat_info, ++idx, track->samplerate);
	/* duration (in samples) (track->tracklen is in ms) */
	/* iTunes sometimes set it to 0, do that for now since it's easier */
	sqlite3_bind_int(stmt_avformat_info, ++idx, 0);
	/* gapless_heuristic_info */
	sqlite3_bind_int(stmt_avformat_info, ++idx, track->gapless_track_flag);
	/* gapless_encoding_delay */
	sqlite3_bind_int(stmt_avformat_info, ++idx, track->pregap);
	/* gapless_encoding_drain */
	sqlite3_bind_int(stmt_avformat_info, ++idx, track->postgap);
	/* gapless_last_frame_resynch */
	sqlite3_bind_int(stmt_avformat_info, ++idx, track->gapless_data);
	/* analysis_inhibit_flags */
	/* TODO don't know where this belongs to */
	sqlite3_bind_int(stmt_avformat_info, ++idx, 0);
	/* audio_fingerprint */
	/* TODO this either */
	sqlite3_bind_int(stmt_avformat_info, ++idx, 0);
	/* volume_normalization_energy */
	sqlite3_bind_int(stmt_avformat_info, ++idx, track->soundcheck);

	res = sqlite3_step(stmt_avformat_info);
	if (res == SQLITE_DONE) {
	    /* expected result */
	} else {
	    fprintf(stderr, "[%s] 7 sqlite3_step returned %d\n", __func__, res);
	}

	/* this is done by a trigger, so we don't need to do this :-D */
	/* INSERT INTO "ext_item_view_membership" VALUES(-6197982141081478573,0,0); */

	if (this_album) {
	    /* printf("[%s] -- inserting into \"album\" (if required)\n", __func__); */
	    res = sqlite3_reset(stmt_album);
	    if (res != SQLITE_OK) {
		fprintf(stderr, "[%s] 1 sqlite3_reset returned %d\n", __func__, res);
	    }
	    /* INSERT INTO "album" VALUES(-7670716306765259718,2,0,0,-7576753259813584028,0,'Enjoy The Silence 04 CDS',100,0,NULL,0); */
	    idx = 0;
	    /* pid */
	    sqlite3_bind_int64(stmt_album, ++idx, album_pid);
	    /* kind */
	    /* TODO use field from mhia */
	    sqlite3_bind_int(stmt_album, ++idx, 2);
	    /* artwork_status */
	    /* TODO */
	    sqlite3_bind_int(stmt_album, ++idx, 0);
	    /* artwork_item_pid */
	    /* TODO */
	    sqlite3_bind_int(stmt_album, ++idx, 0);
	    /* artist_pid */
	    sqlite3_bind_int64(stmt_album, ++idx, artist_pid);
	    /* user_rating */
	    sqlite3_bind_int(stmt_album, ++idx, 0);
	    /* name */
	    sqlite3_bind_text(stmt_album, ++idx, track->album, -1, SQLITE_STATIC);
	    /* name_order */
	    sqlite3_bind_int(stmt_album, ++idx, lookup_order(orders, ORDER_ALBUM_ARTIST, track));
	    /* all_compilations */
	    /* TODO */
	    sqlite3_bind_int(stmt_album, ++idx, 0);
	    /* feed_url */
	    /* TODO this is for podcasts? */
	    sqlite3_bind_null(stmt_album, ++idx);
	    /* season_number */
	    /* TODO this is for tv shows?! */
	    sqlite3_bind_int(stmt_album, ++idx, 0);

	    res = sqlite3_step(stmt_album);
	    if (res == SQLITE_DONE) {
		/* expected result */
	    } else {
		fprintf(stderr, "[%s] 8 sqlite3_step returned %d\n", __func__, res);
	    }
	}


	if (this_artist) {
	    /* printf("[%s] -- inserting into \"artist\" (if required)\n", __func__); */
	    res = sqlite3_reset(stmt_artist);
	    if (res != SQLITE_OK) {
		fprintf(stderr, "[%s] 1 sqlite3_reset returned %d\n", __func__, res);
	    }
	    /* INSERT INTO "artist" VALUES(-7576753259813584028,2,0,0,'Depeche Mode',100,'Depeche Mode'); */
	    idx = 0;
	    /* pid */
	    sqlite3_bind_int64(stmt_artist, ++idx, artist_pid);
	    /* kind */
	    /* TODO use field from mhii */
	    sqlite3_bind_int(stmt_artist, ++idx, 2);
	    /* artwork_status */
	    /* TODO */
	    sqlite3_bind_int(stmt_artist, ++idx, 0);
	    /* artwork_album_pid */
	    /* TODO */
	    sqlite3_bind_int(stmt_artist, ++idx, 0);
	    /* name */
	    sqlite3_bind_text(stmt_artist, ++idx, track->artist, -1, SQLITE_STATIC);
	    /* name_order */
	    sqlite3_bind_int(stmt_artist, ++idx, lookup_order(orders, ORDER_ARTIST, track));
	    /* sort_name */
	    /* TODO always same as 'name'? */
	    sqlite3_bind_text(stmt_artist, ++idx, track->artist, -1, SQLITE_STATIC);

	    res = sqlite3_step(stmt_artist);
	    if (res == SQLITE_DONE) {
		/* expected result */
	    } else {
		fprintf(stderr, "[%s] 8 sqlite3_step returned %d\n", __func__, res);
	    }
	}

	if (this_composer) {
	    /* printf("[%s] -- inserting into \"composer\" (if required)\n", __func__); */
	    res = sqlite3_reset(stmt_composer);
	    if (res != SQLITE_OK) {
		fprintf(stderr, "[%s] 1 sqlite3_reset returned %d\n", __func__, res);
	    }
	    idx = 0;
	    /* pid */
	    sqlite3_bind_int64(stmt_composer, ++idx, composer_pid);
	    /* name */
	    sqlite3_bind_text(stmt_composer, ++idx, track->composer, -1, SQLITE_STATIC);
	    /* name_order */
	    sqlite3_bind_int(stmt_composer, ++idx, lookup_order(orders, ORDER_COMPOSER, track));
	    /* sort_name */
	    /* TODO always same as 'name'? */
	    sqlite3_bind_text(stmt_composer, ++idx, track->composer, -1, SQLITE_STATIC);

	    res = sqlite3_step(stmt_composer);
	    if (res == SQLITE_DONE) {
		/* expected result */
	    } else {
		fprintf(stderr, "[%s] 8 sqlite3_step returned %d\n", __func__, res);
	    }
	}
	/* if it's a movie, music video or tv show */
	if ((track->mediatype & ITDB_MEDIATYPE_MOVIE)
		|| (track->mediatype & ITDB_MEDIATYPE_MUSICVIDEO)
		|| (track->mediatype & ITDB_MEDIATYPE_TVSHOW)) {
	    /* printf("[%s] -- inserting into \"video_info\"\n", __func__); */
	    res = sqlite3_reset(stmt_video_info);
	    if (res != SQLITE_OK) {
		fprintf(stderr, "[%s] 1 sqlite3_reset returned %d\n", __func__, res);
	    }
	    idx = 0;
	    /* item_pid INTEGER NOT NULL */
	    sqlite3_bind_int64(stmt_video_info, ++idx, track->dbid);
	    /* has_alternate_audio INTEGER */
	    sqlite3_bind_int(stmt_video_info, ++idx, 0);
	    /* has_subtitles INTEGER */
	    sqlite3_bind_int(stmt_video_info, ++idx, 0);
	    /* characteristics_valid INTEGER */
	    sqlite3_bind_int(stmt_video_info, ++idx, 0);
	    /* has_closed_captions INTEGER */
	    sqlite3_bind_int(stmt_video_info, ++idx, 0);
	    /* is_self_contained INTEGER */
	    sqlite3_bind_int(stmt_video_info, ++idx, 0);
	    /* is_compressed INTEGER */
	    sqlite3_bind_int(stmt_video_info, ++idx, 0);
	    /* is_anamorphic INTEGER */
	    sqlite3_bind_int(stmt_video_info, ++idx, 0);
	    /* season_number INTEGER */
	    sqlite3_bind_int(stmt_video_info, ++idx, track->season_nr);
	    /* audio_language INTEGER */
	    sqlite3_bind_int(stmt_video_info, ++idx, 0);
	    /* audio_track_index INTEGER */
	    sqlite3_bind_int(stmt_video_info, ++idx, 0);
	    /* audio_track_id INTEGER */
	    sqlite3_bind_int(stmt_video_info, ++idx, 0);
	    /* subtitle_language INTEGER */
	    sqlite3_bind_int(stmt_video_info, ++idx, 0);
	    /* subtitle_track_index INTEGER */
	    sqlite3_bind_int(stmt_video_info, ++idx, 0);
	    /* subtitle_track_id INTEGER */
	    sqlite3_bind_int(stmt_video_info, ++idx, 0);
	    /* series_name TEXT */
	    sqlite3_bind_text(stmt_video_info, ++idx, track->tvshow, -1, SQLITE_STATIC);
	    /* sort_series_name TEXT */
	    bind_first_text(stmt_item, ++idx, 2, track->sort_tvshow, track->tvshow);
	    /* episode_id TEXT */
	    sqlite3_bind_text(stmt_video_info, ++idx, track->tvepisode, -1, SQLITE_STATIC);
	    /* episode_sort_id INTEGER */
	    sqlite3_bind_int(stmt_video_info, ++idx, track->season_nr << 16 | track->episode_nr);
	    /* network_name TEXT */
	    sqlite3_bind_text(stmt_video_info, ++idx, track->tvnetwork, -1, SQLITE_STATIC);
	    /* extended_content_rating TEXT */
	    sqlite3_bind_null(stmt_video_info, ++idx);
	    /* movie_info TEXT */
	    sqlite3_bind_null(stmt_video_info, ++idx);

	    res = sqlite3_step(stmt_video_info);
	    if (res == SQLITE_DONE) {
		/* expected result */
	    } else {
		fprintf(stderr, "[%s] 8 sqlite3_step returned %d\n", __func__, res);
	    }
	}
    }

    sqlite3_exec(db, "UPDATE album SET artwork_item_pid = (SELECT item.pid FROM item WHERE item.artwork_cache_id != 0 AND item.album_pid = album.pid LIMIT 1);", NULL, NULL, NULL);

    sqlite3_exec(db, "COMMIT;", NULL, NULL, NULL);

    res = 0;
    printf("[%s] done.\n", __func__);
leave:
    if (orders != NULL) {
        destroy_key_orders(orders);
    }

    if (stmt_version_info) {
	sqlite3_finalize(stmt_version_info);
    }
    if (stmt_db_info) {
	sqlite3_finalize(stmt_db_info);
    }
    if (stmt_container) {
	sqlite3_finalize(stmt_container);
    }
    if (stmt_genre_map) {
	sqlite3_finalize(stmt_genre_map);
    }
    if (stmt_item) {
	sqlite3_finalize(stmt_item);
    }
    if (stmt_location_kind_map) {
	sqlite3_finalize(stmt_location_kind_map);
    }
    if (stmt_avformat_info) {
	sqlite3_finalize(stmt_avformat_info);
    }
    if (stmt_item_to_container) {
	sqlite3_finalize(stmt_item_to_container);
    }
    if (stmt_album) {
	sqlite3_finalize(stmt_album);
    }
    if (stmt_artist) {
	sqlite3_finalize(stmt_artist);
    }
    if (stmt_video_info) {
	sqlite3_finalize(stmt_video_info);
    }
    if (genre_map) {
	g_hash_table_destroy(genre_map);
    }
    if (db) {
	sqlite3_close(db);
    }
    if (dbf) {
	g_free(dbf);
    }
    return res;
}

static int mk_Locations(Itdb_iTunesDB *itdb, const char *outpath, const char *uuid)
{
    int res = -1;
    gchar *dbf = NULL;
    sqlite3 *db = NULL;
    sqlite3_stmt *stmt = NULL;
    char *errmsg = NULL;
    int idx = 0;

    dbf = g_build_filename(outpath, "Locations.itdb", NULL);
    printf("[%s] Processing '%s'\n", __func__, dbf);
    /* file is present. delete it, we'll re-create it. */
    if (g_unlink(dbf) != 0) {
	if (errno != ENOENT) {
	    fprintf(stderr, "[%s] could not delete '%s': %s\n", __func__, dbf, strerror(errno));
	    goto leave;
	}
    }

    if (SQLITE_OK != sqlite3_open((const char*)dbf, &db)) {
	fprintf(stderr, "Error opening database '%s': %s\n", dbf, sqlite3_errmsg(db));
	goto leave;
    }

    sqlite3_exec(db, "PRAGMA synchronous = OFF;", NULL, NULL, NULL);

    fprintf(stderr, "[%s] re-building table structure\n", __func__);
    /* db structure needs to be created. */
    if (SQLITE_OK != sqlite3_exec(db, Locations_create, NULL, NULL, &errmsg)) {
	fprintf(stderr, "[%s] sqlite3_exec error: %s\n", __func__, sqlite3_errmsg(db));
	if (errmsg) {
	    fprintf(stderr, "[%s] additional error information: %s\n", __func__, errmsg);
	    sqlite3_free(errmsg);
	    errmsg = NULL;
	}
	goto leave;
    }

    sqlite3_exec(db, "BEGIN;", NULL, NULL, NULL);

    if (SQLITE_OK != sqlite3_prepare_v2(db, "INSERT INTO \"base_location\" (id, path) VALUES (?,?);", -1, &stmt, NULL)) {
	fprintf(stderr, "[%s] sqlite3_prepare error: %s\n", __func__, sqlite3_errmsg(db));
	goto leave;
    }
    idx = 0;
    res = sqlite3_reset(stmt);
    if (res != SQLITE_OK) {
	fprintf(stderr, "[%s] sqlite3_reset returned %d\n", __func__, res);
    }

    sqlite3_bind_int(stmt, ++idx, 1);
    if (itdb_device_is_iphone_family(itdb->device)) {
	sqlite3_bind_text(stmt, ++idx, "iTunes_Control/Music", -1, SQLITE_STATIC);
    } else {
	sqlite3_bind_text(stmt, ++idx, "iPod_Control/Music", -1, SQLITE_STATIC);
    }
    res = sqlite3_step(stmt);
    if (res == SQLITE_DONE) {
	/* expected result */
    } else {
	fprintf(stderr, "[%s] sqlite3_step returned %d\n", __func__, res);
    }

    if (itdb_device_is_iphone_family(itdb->device)) {
	idx = 0;
	res = sqlite3_reset(stmt);
	if (res != SQLITE_OK) {
	    fprintf(stderr, "[%s] sqlite3_reset returned %d\n", __func__, res);
	}
	sqlite3_bind_int(stmt, ++idx, 4);
	sqlite3_bind_text(stmt, ++idx, "Podcasts", -1, SQLITE_STATIC);
	res = sqlite3_step(stmt);
	if (res == SQLITE_DONE) {
	    /* expected result */
	} else {
	    fprintf(stderr, "[%s] sqlite3_step returned %d\n", __func__, res);
	}

	idx = 0;
	res = sqlite3_reset(stmt);
	if (res != SQLITE_OK) {
	    fprintf(stderr, "[%s] sqlite3_reset returned %d\n", __func__, res);
	}
	sqlite3_bind_int(stmt, ++idx, 6);
	sqlite3_bind_text(stmt, ++idx, "iTunes_Control/Ringtones", -1, SQLITE_STATIC);
	res = sqlite3_step(stmt);
	if (res == SQLITE_DONE) {
	    /* expected result */
	} else {
	    fprintf(stderr, "[%s] sqlite3_step returned %d\n", __func__, res);
	}
    }

    if (stmt) {
	sqlite3_finalize(stmt);
    }
 
    if (SQLITE_OK != sqlite3_prepare_v2(db, "INSERT INTO \"location\" (item_pid, sub_id, base_location_id, location_type, location, extension, kind_id, date_created, file_size) VALUES(?,?,?,?,?,?,?,?,?);", -1, &stmt, NULL)) {
	fprintf(stderr, "[%s] sqlite3_prepare error: %s\n", __func__, sqlite3_errmsg(db));
	goto leave;
    }

    if (itdb->tracks) {
	GList *gl = NULL;

	printf("[%s] Processing %d tracks...\n", __func__, g_list_length(itdb->tracks));
	for (gl=itdb->tracks; gl; gl=gl->next) {
	    Itdb_Track *track = gl->data;
	    char *ipod_path;
	    int i = 0;
	    int cnt = 0;
	    int pos = 0;
	    int res;

	    if (!track->ipod_path) {
		continue;
	    }
	    ipod_path = strdup(track->ipod_path);
	    idx = 0;
	    res = sqlite3_reset(stmt);
	    if (res != SQLITE_OK) {
		fprintf(stderr, "[%s] sqlite3_reset returned %d\n", __func__, res);
	    }
	    /* item_pid */
	    sqlite3_bind_int64(stmt, ++idx, track->dbid);
	    /* sub_id */
	    /* TODO subitle id? set to 0. */
	    sqlite3_bind_int64(stmt, ++idx, 0);
	    /* base_location_id */
	    /*  use 1 here as this is 'iTunes_Control/Music' */
	    sqlite3_bind_int(stmt, ++idx, 1);
	    /* location_type */
	    /* TODO this should always be 0x46494C45 = "FILE" for now, */
	    /*  perhaps later libgpod will support more. */
	    sqlite3_bind_int(stmt, ++idx, 0x46494C45);
	    /* location */
	    for (i = 0; i < strlen(ipod_path); i++) {
		/* replace all ':' with '/' so that the path is valid */
		if (ipod_path[i] == ':') {
		    ipod_path[i] = '/';
		    cnt++;
		    if (cnt == 3) {
			pos = i+1;
		    }
		}
	    }
	    sqlite3_bind_text(stmt, ++idx, ipod_path + pos, -1, SQLITE_STATIC);
	    /* extension */
	    sqlite3_bind_int(stmt, ++idx, track->filetype_marker);
	    /* kind_id, should match track->mediatype */
	    sqlite3_bind_int(stmt, ++idx, track->mediatype);
	    /* date_created */
	    sqlite3_bind_int(stmt, ++idx, timeconv(track->time_modified));
	    /* file_size */
	    sqlite3_bind_int(stmt, ++idx, track->size);
#if 0
	    /* file_creator */
	    /* TODO unknown, set to NULL */
	    sqlite3_bind_null(stmt, ++idx);
	    /* file_type */
	    /* TODO unknown, set to NULL */
	    sqlite3_bind_null(stmt, ++idx);
	    /* num_dir_levels_file */
	    /* TODO unknown, set to NULL */
	    sqlite3_bind_null(stmt, ++idx);
	    /* num_dir_levels_lib */
	    /* TODO unknown, set to NULL */
	    sqlite3_bind_null(stmt, ++idx);
#endif
	    res = sqlite3_step(stmt);
	    if (res == SQLITE_DONE) {
		/* expected result */
	    } else {
		fprintf(stderr, "[%s] 10 sqlite3_step returned %d\n", __func__, res);
	    }
	    if (ipod_path) {
		free(ipod_path);
		ipod_path = NULL;
	    }
	}
    } else {
	printf("[%s] No tracks available, none written.\n", __func__);
    }

    sqlite3_exec(db, "COMMIT;", NULL, NULL, NULL);

    res = 0;
    printf("[%s] done.\n", __func__);
leave:
    if (stmt) {
	sqlite3_finalize(stmt);
    }
    if (db) {
	sqlite3_close(db);
    }
    if (dbf) {
	g_free(dbf);
    }
    return res;
}

static void run_post_process_commands(Itdb_iTunesDB *itdb, const char *outpath, const char *uuid)
{
    plist_t plist_node = NULL;
    plist_t ppc_dict = NULL;
    const gchar *basedb = "Library.itdb";
    const gchar *otherdbs[] = {"Dynamic.itdb", "Extras.itdb", "Genius.itdb", "Locations.itdb", NULL};
    int res;
    sqlite3 *db = NULL;

#ifdef HAVE_LIBIMOBILEDEVICE
    if (itdb_device_is_iphone_family(itdb->device)) {
	/* get SQL post process commands via lockdown (iPhone/iPod Touch) */
	lockdownd_client_t client = NULL;
	idevice_t phone = NULL;
	idevice_error_t ret = IDEVICE_E_UNKNOWN_ERROR;
	lockdownd_error_t lockdownerr = LOCKDOWN_E_UNKNOWN_ERROR;

	ret = idevice_new(&phone, uuid);
	if (ret != IDEVICE_E_SUCCESS) {
	    printf("[%s] ERROR: Could not find device with uuid %s, is it plugged in?\n", __func__, uuid);
	    goto leave;
	}

	if (LOCKDOWN_E_SUCCESS != lockdownd_client_new_with_handshake(phone, &client, "libgpod")) {
	    printf("[%s] ERROR: Could not connect to device's lockdownd!\n", __func__);
	    idevice_free(phone);
	    goto leave;
	}

	lockdownerr = lockdownd_get_value(client, "com.apple.mobile.iTunes.SQLMusicLibraryPostProcessCommands", NULL, &plist_node);
	lockdownd_client_free(client);
	idevice_free(phone);

	if (lockdownerr == LOCKDOWN_E_SUCCESS) {
	    ppc_dict = plist_node;
	} else {
	    if (plist_node) {
		plist_free(plist_node);
		plist_node = NULL;
	    }
	}
    } else 
#endif
    if (itdb->device->sysinfo_extended != NULL) {
	/* try to get SQL post process commands via sysinfo_extended */
	gchar *dev_path = itdb_get_device_dir(itdb->device->mountpoint);
	if (dev_path) {
	    const gchar *p_sysinfo_ex[] = {"SysInfoExtended", NULL};
	    gchar *sysinfo_ex_path = itdb_resolve_path(dev_path, p_sysinfo_ex);
	    g_free(dev_path);
	    if (sysinfo_ex_path) {
		/* open plist file */
		char *xml_contents = NULL;
		gsize xml_length = 0;
		if (g_file_get_contents(sysinfo_ex_path, &xml_contents, &xml_length, NULL)) {
		    plist_from_xml(xml_contents, xml_length, &plist_node);
		    if (plist_node) {
			/* locate specific key */
			ppc_dict = plist_dict_get_item(plist_node, "com.apple.mobile.iTunes.SQLMusicLibraryPostProcessCommands");
		    }
		}
		if (xml_contents) {
		    g_free(xml_contents);
		}
		g_free(sysinfo_ex_path);
	    }
	}
    }

    if (ppc_dict) {
	plist_dict_iter iter = NULL;
	plist_t sql_cmds = NULL;
	plist_t user_ver_cmds = NULL;

	printf("[%s] Getting SQL post process commands\n", __func__);

	sql_cmds = plist_dict_get_item(ppc_dict, "SQLCommands");
	user_ver_cmds = plist_dict_get_item(ppc_dict, "UserVersionCommandSets");

	if (sql_cmds && user_ver_cmds) {
	    /* we found the SQLCommands and the UserVersionCommandSets keys */
	    char *key = NULL;
	    unsigned long int maxver = 0;
	    plist_t curnode = user_ver_cmds;
	    plist_t subnode = NULL;

	    user_ver_cmds = NULL;

	    /* now look for numbered subkey in the UserVersionCommandsSets */
	    plist_dict_new_iter(curnode, &iter);
	    if (iter) {
		plist_dict_next_item(curnode, iter, &key, &subnode);
		while (subnode) {
		    unsigned long int intval = strtoul(key, NULL, 0);
		    if ((intval > 0) && (intval > maxver)) {
			user_ver_cmds = subnode;
			maxver = intval;
		    }
		    subnode = NULL;
		    free(key);
		    key = NULL;
		    plist_dict_next_item(curnode, iter, &key, &subnode);
		}
		free(iter);
		iter = NULL;
	    }
	    if (user_ver_cmds) {
		/* found numbered key (usually '8', for Nano 5G it is '9') */
		curnode = user_ver_cmds;
		/* now get the commands array */
		user_ver_cmds = plist_dict_get_item(curnode, "Commands");
		if (user_ver_cmds && (plist_get_node_type(user_ver_cmds) == PLIST_ARRAY)) {
		    /* We found our array with the commands to execute, now
		     * make a hashmap for the SQLCommands to find them faster
		     * when we actually execute them in correct order. */
	    	    GHashTable *sqlcmd_map = g_hash_table_new(g_str_hash, g_str_equal);
		    if (sqlcmd_map) {
			char *val = NULL;
		    	gchar *dbf = NULL;
		    	gchar *attach_str = NULL;
			char *errmsg = NULL;
			guint32 i = 0, cnt = 0, ok_cnt = 0;

			plist_dict_new_iter(sql_cmds, &iter);
			if (iter) {
			    plist_dict_next_item(sql_cmds, iter, &key, &subnode);
			    while (subnode) {
				if (plist_get_node_type(subnode) == PLIST_STRING) {
				    plist_get_string_val(subnode, &val);
				    g_hash_table_insert(sqlcmd_map, key, val);
				    val = NULL;
				} else {
				    printf("[%s] WARNING: ignoring non-string value for key '%s'\n", __func__, key);
				    free(key);
				}
				subnode = NULL;
				key = NULL;
				plist_dict_next_item(sql_cmds, iter, &key, &subnode);
			    }
			    free(iter);
			    iter = NULL;
			}

			/* open Library.itdb first */
			dbf = g_build_filename(outpath, basedb, NULL);

			if (SQLITE_OK != sqlite3_open((const char*)dbf, &db)) {
			    fprintf(stderr, "Error opening database '%s': %s\n", dbf, sqlite3_errmsg(db));
			    g_free(dbf);
			    goto leave;
			}
			g_free(dbf);

			/* now attach other required database files */
			i = 0;
			while (otherdbs[i]) {
			    errmsg = NULL;
			    dbf = g_build_filename(outpath, otherdbs[i], NULL);
			    attach_str = g_strdup_printf("ATTACH DATABASE '%s' AS '%s';", dbf, otherdbs[i]);
			    g_free(dbf);
			    res = sqlite3_exec(db, attach_str, NULL, NULL, &errmsg);
			    g_free(attach_str);
			    if (res != SQLITE_OK) {
				printf("[%s] WARNING: Could not attach database '%s': %s\n", __func__, otherdbs[i], errmsg);
			    }
			    if (errmsg) {
				free(errmsg);
			    }
			    i++;
			}

			cnt = plist_array_get_size(user_ver_cmds);
			printf("[%s] Running %d post process commands now\n", __func__, cnt);

			sqlite3_exec(db, "BEGIN;", NULL, NULL, NULL);
			for (i=0; i<cnt; i++) {
			    subnode = plist_array_get_item(user_ver_cmds, i);
			    plist_get_string_val(subnode, &key);
			    if (key) {
				val = g_hash_table_lookup(sqlcmd_map, key);
				if (val) {
				    char *errmsg = NULL; 
				    if (SQLITE_OK == sqlite3_exec(db, val, NULL, NULL, &errmsg)) {
				        /*printf("[%s] executing '%s': OK", __func__, key);*/
					ok_cnt++;
				    } else {
					printf("[%s] ERROR when executing '%s': %s\n", __func__, key, errmsg);
				    }
				    if (errmsg) {
					sqlite3_free(errmsg);
				    }
				} else {
				    printf("[%s] value for '%s' not found in hashmap!\n", __func__, key);
				}
				free(key);
				key = NULL;
			    }
			}
			g_hash_table_foreach(sqlcmd_map, free_key_val_strings, NULL);
			g_hash_table_destroy(sqlcmd_map);

			printf("[%s] %d out of %d post process commands successfully executed\n", __func__, ok_cnt, cnt);
			/* TODO perhaps we want to roll back when an error has occured ? */
			sqlite3_exec(db, "COMMIT;", NULL, NULL, NULL);
		    } else {
			printf("[%s]: Error: could not create hash table!\n", __func__);
		    }
		} else {
		    printf("[%s]: Error fetching commands array\n", __func__);
		}
	    } else {
		printf("[%s]: Error fetching user version command set\n", __func__);
	    }
	} else {
	    printf("[%s]: Error fetching post process commands from device!\n", __func__);
	}
    }
   
    printf("[%s] done.\n", __func__);

leave:
    if (db) {
	sqlite3_close(db);
    }
    if (plist_node) {
	plist_free(plist_node);
    }

}

static int cbk_calc_sha1_one_block(FILE *f, unsigned char sha1[20])
{
    const guint BLOCK_SIZE = 1024;
    unsigned char block[BLOCK_SIZE];
    size_t read_count;
    GChecksum *checksum;
    gsize sha1_len;

    read_count = fread(block, BLOCK_SIZE, 1, f);
    if ((read_count != 1)) {
	if (feof(f)) {
	    return 1;
	} else {
	    return -1;
	}
    }

    sha1_len = g_checksum_type_get_length(G_CHECKSUM_SHA1);
    g_assert (sha1_len == 20);
    checksum = g_checksum_new(G_CHECKSUM_SHA1);
    g_checksum_update(checksum, block, BLOCK_SIZE);
    g_checksum_get_digest(checksum, sha1, &sha1_len);
    g_checksum_free(checksum);

    return 0;
}

static gboolean cbk_calc_sha1s(const char *filename, GArray *sha1s)
{
    FILE *f;
    int calc_ok;

    f = fopen(filename, "rb");
    if (f == NULL) {
	return FALSE;
    }

    do {
	unsigned char sha1[20];
	calc_ok = cbk_calc_sha1_one_block(f, sha1);
	if (calc_ok != 0) {
	    break;
	}
	g_array_append_vals(sha1s, sha1, sizeof(sha1));
    } while (calc_ok == 0);

    if (calc_ok < 0) {
	goto error;
    }

    fclose(f);
    return TRUE;

error:
    fclose(f);
    return FALSE;
}

static const guint CBK_HEADER_SIZE = 46;

static void cbk_calc_sha1_of_sha1s(GArray *cbk)
{
    GChecksum *checksum;
    unsigned char* final_sha1;
    unsigned char* sha1s;
    gsize final_sha1_len;

    g_assert (cbk->len > CBK_HEADER_SIZE + 20);

    final_sha1 = &g_array_index(cbk, guchar, CBK_HEADER_SIZE);
    sha1s = &g_array_index(cbk, guchar, CBK_HEADER_SIZE + 20);
    final_sha1_len = g_checksum_type_get_length(G_CHECKSUM_SHA1);
    g_assert (final_sha1_len == 20);

    checksum = g_checksum_new(G_CHECKSUM_SHA1);
    g_checksum_update(checksum, sha1s, cbk->len - (CBK_HEADER_SIZE + 20));
    g_checksum_get_digest(checksum, final_sha1, &final_sha1_len);
    g_checksum_free(checksum);
}

static gboolean mk_Locations_cbk(Itdb_iTunesDB *itdb, const char *dirname)
{
    char *locations_filename;
    char *cbk_filename;
    GArray *cbk;
    gboolean success;
    guchar *cbk_hash72;
    guchar *final_sha1;

    cbk = g_array_sized_new(FALSE, TRUE, 1,
			    CBK_HEADER_SIZE + 20);
    g_array_set_size(cbk, CBK_HEADER_SIZE + 20);

    locations_filename = g_build_filename(dirname, "Locations.itdb", NULL);
    success = cbk_calc_sha1s(locations_filename, cbk);
    g_free(locations_filename);
    if (!success) {
	g_array_free(cbk, TRUE);
	return FALSE;
    }
    cbk_calc_sha1_of_sha1s(cbk);
    final_sha1 = &g_array_index(cbk, guchar, CBK_HEADER_SIZE);
    cbk_hash72 = &g_array_index(cbk, guchar, 0);
    success = itdb_hash72_compute_hash_for_sha1 (itdb->device, final_sha1,
						 cbk_hash72, NULL);
    if (!success) {
	g_array_free(cbk, TRUE);
	return FALSE;
    }

    cbk_filename = g_build_filename(dirname, "Locations.itdb.cbk", NULL);
    success = g_file_set_contents(cbk_filename, cbk->data, cbk->len, NULL);
    g_free(cbk_filename);
    g_array_free(cbk, TRUE);
    return success;
}

static int build_itdb_files(Itdb_iTunesDB *itdb,
			     GHashTable *album_ids, GHashTable *artist_ids,
			     GHashTable *composer_ids,
			     const char *outpath, const char *uuid)
{
    int err = 0;

    if (mk_Dynamic(itdb, outpath) != 0) {
	err++;
    }
    if (mk_Extras(itdb, outpath) != 0) {
	err++;
    }
    if (mk_Genius(itdb, outpath) != 0) {
	err++;
    }
    if (mk_Library(itdb, album_ids, artist_ids, composer_ids, outpath) != 0) {
	err++;
    }
    if (mk_Locations(itdb, outpath, uuid) != 0) {
	err++;
    }

    run_post_process_commands(itdb, outpath, uuid);

    if (!mk_Locations_cbk(itdb, outpath)) {
	err++;
    }

    if (err > 0) {
	return -1;
    }
    return 0;
}

static int ensure_itlp_dir_exists(const char *itlpdir, GError **error)
{
    /* check if directory exists */
    if (!g_file_test(itlpdir, G_FILE_TEST_EXISTS)) {
	if (g_mkdir(itlpdir, 0755) != 0) {
	    g_set_error (error, G_FILE_ERROR,
			 g_file_error_from_errno(errno),
			 "Could not create directory '%s': %s",
			 itlpdir, strerror(errno));
	    return FALSE;
	}
    } else if (!g_file_test(itlpdir, G_FILE_TEST_IS_DIR)) {
	g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_NOTDIR,
		     "'%s' is not a directory as it should be",
		     itlpdir);
	return FALSE;
    }

    return TRUE;
}

static int copy_itdb_file(const gchar *from_dir, const gchar *to_dir,
			  const gchar *fname, GError **error)
{
    int res = 0;

    gchar *srcname = g_build_filename(from_dir, fname, NULL);
    gchar *dstname = g_build_filename(to_dir, fname, NULL);

    if (itdb_cp(srcname, dstname, error)) {
	fprintf(stderr, "itdbprep: copying '%s'\n", fname);
	res++;
    }
    if (error && *error) {
	fprintf(stderr, "Error copying '%s' to '%s': %s\n", srcname, dstname, (*error)->message);
    }

    if (srcname) {
	g_free(srcname);
    }

    if (dstname) {
	g_free(dstname);
    }

    return res;
}

static void rmdir_recursive(gchar *path)
{
    GDir *cur_dir;
    const gchar *dir_file;

    cur_dir = g_dir_open(path, 0, NULL);
    if (cur_dir) while ((dir_file = g_dir_read_name(cur_dir)))
    {
	gchar *fpath = g_build_filename(path, dir_file, NULL);
	if (fpath) {
	    if (g_file_test(fpath, G_FILE_TEST_IS_DIR)) {
		rmdir_recursive(fpath);
	    } else {
		g_unlink(fpath);
	    }
	    g_free(fpath);
	}
    }
    if (cur_dir) {
	g_dir_close(cur_dir);
    }
    g_rmdir(path);
}

int itdb_sqlite_generate_itdbs(FExport *fexp)
{
    int res = 0;
    gchar *itlpdir;
    gchar *dirname;
    gchar *tmpdir = NULL;

    printf("libitdbprep: %s called with file %s and uuid %s\n", __func__,
	   fexp->itdb->filename, itdb_device_get_uuid(fexp->itdb->device));

    dirname = itdb_get_itunes_dir(itdb_get_mountpoint(fexp->itdb));
    itlpdir = g_build_filename(dirname, "iTunes Library.itlp", NULL);
    g_free(dirname);

    printf("itlp directory='%s'\n", itlpdir);

    if (!ensure_itlp_dir_exists(itlpdir, &fexp->error)) {
	res = -1;
	goto leave;
    }

    printf("*.itdb files will be stored in '%s'\n", itlpdir);

    g_assert(fexp->itdb != NULL);
    g_assert(fexp->itdb->playlists != NULL);

    tzoffset = fexp->itdb->tzoffset;

    tmpdir = g_build_path(g_get_tmp_dir(), tmpnam(NULL), NULL);
    if (g_mkdir(tmpdir, 0755) != 0) {
	g_set_error (&fexp->error, G_FILE_ERROR, g_file_error_from_errno(errno),
		     "Could not create temporary directory '%s': %s",
		     tmpdir, strerror(errno));
	res = -1;
	goto leave;
    }

    /* generate itdb files in temporary directory */
    if (build_itdb_files(fexp->itdb, fexp->albums, fexp->artists, fexp->composers, tmpdir,
		     itdb_device_get_uuid(fexp->itdb->device)) != 0) {
	g_set_error (&fexp->error, ITDB_ERROR, ITDB_ERROR_SQLITE,
		     "Failed to generate sqlite database");
	res = -1;
	goto leave;
    } else {
	/* copy files */
	const char *itdb_files[] = { "Dynamic.itdb", "Extras.itdb",
				     "Genius.itdb", "Library.itdb",
				     "Locations.itdb", "Locations.itdb.cbk",
				     NULL };
	const char **file;
	GError *error = NULL;
	g_assert (fexp->error == NULL);
	for (file = itdb_files; *file != NULL; file++) {
	    copy_itdb_file(tmpdir, itlpdir, *file, &error);
	    if (error) {
		res = -1;
		/* only the last error will be reported, but this way we
		 * copy as many files as possible even when errors occur
		 */
		g_clear_error (&fexp->error);
		g_propagate_error (&fexp->error, error);
	    }
	}
	if (fexp->error) {
	    goto leave;
	}
    }

    res = 0;
leave:
    if (itlpdir) {
	g_free(itlpdir);
    }
    if (tmpdir) {
	/* cleanup tmpdir, this will also delete tmpdir itself */
	rmdir_recursive(tmpdir);
	g_free(tmpdir);
    }

    return res;
}

/*
|  Copyright (C) 2009 Nikias Bassen <nikias@gmx.li>
|  Copyright (C) 2009 Christophe Fergeau <cfergeau@mandriva.com>

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
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include <glib.h>
#include <glib/gstdio.h>
#include <sqlite3.h>

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


static void mk_Dynamic(Itdb_iTunesDB *itdb, const char *outpath)
{
    gchar *dbf = NULL;
    sqlite3 *db = NULL;
    sqlite3_stmt *stmt = NULL;
    const char *sqltail = NULL;
    char *errmsg = NULL;
    struct stat fst;
    int idx = 0;
   
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

    if (itdb->tracks) {
	GList *gl = NULL;

	printf("[%s] - processing %d tracks\n", __func__, g_list_length(itdb->tracks));
	for (gl=itdb->tracks; gl; gl=gl->next) {
	    Itdb_Track *track = gl->data;
	    if (!track->ipod_path) {
		continue;
	    }

	    printf("[%s] -- inserting into \"item_stats\"\n", __func__);
	    if (SQLITE_OK != sqlite3_prepare_v2(db, "INSERT INTO \"item_stats\" VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?);", -1, &stmt, &sqltail)) {
		fprintf(stderr, "[%s] sqlite3_prepare error: %s\n", __func__, sqlite3_errmsg(db));
	    } else {
		int res;
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
		/* hidden */
		sqlite3_bind_int(stmt, ++idx, 0);
		/* deleted */
		sqlite3_bind_int(stmt, ++idx, 0);
		/* has_changes */
		sqlite3_bind_int(stmt, ++idx, 0);

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
	}
    } else {
	printf("[%s] - No tracks available, none written.\n", __func__);
    }

    printf("[%s] done.\n", __func__);
leave:
    if (db) {
	sqlite3_close(db);
    }
    if (dbf) {
	g_free(dbf);
    }
}

static void mk_Extras(Itdb_iTunesDB *itdb, const char *outpath)
{
    int rebuild = 0;
    gchar *dbf = NULL;
    sqlite3 *db = NULL;
    char *errmsg = NULL;
    struct stat fst; 

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

    /* kill all entries in 'chapter' as they will be re-inserted */
    /* TODO: we do not support this in the moment, so don't touch this */
    /*if (SQLITE_OK != sqlite3_exec(db, "DELETE FROM chapter;", NULL, NULL, &errmsg)) {
	fprintf(stderr, "[%s] sqlite3_exec error: %s\n", __func__, sqlite3_errmsg(db));
	if (errmsg) {
	    fprintf(stderr, "[%s] additional error information: %s\n", __func__, errmsg);
	    sqlite3_free(errmsg);
	    errmsg = NULL;
	}
	goto leave;
    }*/

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

    printf("[%s] done.\n", __func__);
leave:
    if (db) {
	sqlite3_close(db);
    }
    if (dbf) {
	g_free(dbf);
    }
}

static void mk_Genius(Itdb_iTunesDB *itdb, const char *outpath)
{
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

    printf("[%s] done.\n", __func__);
leave:
    if (db) {
	sqlite3_close(db);
    }
    if (dbf) {
	g_free(dbf);
    }
}

static void mk_Library(Itdb_iTunesDB *itdb,
		       GHashTable *album_ids, GHashTable *artist_ids,
		       const char *outpath)
{
    gchar *dbf = NULL;
    sqlite3 *db = NULL;
    sqlite3_stmt *stmt = NULL;
    const char *sqltail = NULL;
    char *errmsg = NULL;
    struct stat fst;
    GList *gl = NULL;
    Itdb_Playlist *dev_playlist = NULL;
    int idx = 0;
    GHashTable *genre_map;
    guint32 genre_index;

    /* get some itdb infos */
    for (gl = itdb->playlists; gl; gl = gl->next) {
	Itdb_Playlist *pl = (Itdb_Playlist*)gl->data;
	if (pl->type == 1) {
	    dev_playlist = pl;
	    break;
	}
    }
    if (!dev_playlist) {
	fprintf(stderr, "Could not find special device playlist!\n");
	goto leave;
    }
    printf("library_persistent_id = 0x%016"G_GINT64_MODIFIER"x\n", itdb->priv->pid);
    printf("primary_container_pid = 0x%016"G_GINT64_MODIFIER"x\n", dev_playlist->id);

    if (!dev_playlist->name) {
	fprintf(stderr, "Could not fetch device name from itdb!\n");
	goto leave;
    }
    printf("device name = %s\n", dev_playlist->name);

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

    printf("[%s] - inserting into \"version_info\"\n", __func__);
    /* INSERT INTO "version_info" VALUES(1,2,40,0,0,2); */
    if (SQLITE_OK != sqlite3_prepare_v2(db, "INSERT INTO \"version_info\" VALUES(?,?,?,?,?,?);", -1, &stmt, &sqltail)) {
	fprintf(stderr, "[%s] sqlite3_prepare error: %s\n", __func__, sqlite3_errmsg(db));
	goto leave;
    } else {
	int res;
	idx = 0;
	/* id */
	sqlite3_bind_int(stmt, ++idx, 1);
	/* major */
	/* FIXME: the versioning scheme needs to be updated in libgpod using */
	/* major+minor version, this is a workaround */
	sqlite3_bind_int(stmt, ++idx, (itdb->version >= 0x28) ? 2 : 1);
	/* minor */
	sqlite3_bind_int(stmt, ++idx, itdb->version);
	/* compatibility */
	/* This has an unknown meaning. use 0. */
	sqlite3_bind_int(stmt, ++idx, 0);
	/* update_level */
	/* This has an unknown meaning. use 0. */
	sqlite3_bind_int(stmt, ++idx, 0);
	/* platform */
	/* TODO: this is a guess: 2 = Windows, 1 = MacOS */
	/* FIXME: needs autoselection based on the itdb library (how to check?) */
	sqlite3_bind_int(stmt, ++idx, 2);

	res = sqlite3_step(stmt);
	if (res == SQLITE_DONE) {
	    /* expected result */
	} else {
	    fprintf(stderr, "[%s] 2 sqlite3_step returned %d\n", __func__, res); 
	}
    }
    if (stmt) {
	sqlite3_finalize(stmt);
	stmt = NULL;
    }

    printf("[%s] - inserting into \"db_info\"\n", __func__);
    /* INSERT INTO "db_info" VALUES(-5483759902971074902,959107999841118509,NULL,5575,-1,NULL,NULL); */
    if (SQLITE_OK != sqlite3_prepare_v2(db, "INSERT INTO \"db_info\" VALUES(?,?,?,?,?,?,?);", -1, &stmt, &sqltail)) {
	fprintf(stderr, "[%s] sqlite3_prepare error: %s\n", __func__, sqlite3_errmsg(db));
	goto leave;
    } else {
	int res;
	idx = 0;
	/* pid */
	sqlite3_bind_int64(stmt, ++idx, itdb->priv->pid);
	/* primary_container_pid */
	/* ... stored in the playlists where the device name is stored too. */
	sqlite3_bind_int64(stmt, ++idx, dev_playlist->id);
	/* media_folder_url */
	sqlite3_bind_null(stmt, ++idx);
	/* audio_language  */
	/*  this is +0xA0 */
	sqlite3_bind_int(stmt, ++idx, itdb->priv->audio_language);
	/* subtitle_language */
	/*  this is +0xA2 */
	sqlite3_bind_int(stmt, ++idx, itdb->priv->subtitle_language);
	/* bib */
	/* TODO: unkown meaning, set to NULL */
	sqlite3_bind_null(stmt, ++idx);
	/* rib */
	/* TODO: unkown meaning, set to NULL */
	sqlite3_bind_null(stmt, ++idx);

	res = sqlite3_step(stmt);
	if (res == SQLITE_DONE) {
	    /* expected result */
	} else {
	    fprintf(stderr, "[%s] 3 sqlite3_step returned %d\n", __func__, res); 
	}
    }
    if (stmt) {
	sqlite3_finalize(stmt);
	stmt = NULL;
    }

    printf("[%s] - inserting into \"container\"\n", __func__);
    /* INSERT INTO "container" VALUES(959107999841118509,0,267295295,'Hamouda',400,0,1,0,1,0,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL); */
    if (SQLITE_OK != sqlite3_prepare_v2(db, "INSERT INTO \"container\" VALUES(?,?,?,:name,?,?,?,?,?,?,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL);", -1, &stmt, &sqltail)) {
	fprintf(stderr, "[%s] sqlite3_prepare error: %s\n", __func__, sqlite3_errmsg(db));
	goto leave;
    } else {
	int res;
	idx = 0;
	/* pid */
	sqlite3_bind_int64(stmt, ++idx, dev_playlist->id);
	/* distinguished_kind */
	/* TODO: whatever this means, set it to 0 */
	sqlite3_bind_int(stmt, ++idx, 0);
	/* date_created */
	sqlite3_bind_int(stmt, ++idx, timeconv(dev_playlist->timestamp));
	/* name */
	sqlite3_bind_text(stmt, ++idx, dev_playlist->name, -1, SQLITE_STATIC);
	/* name order */
	/* This seems to be always 400 */
	sqlite3_bind_int(stmt, ++idx, 400);
	/* parent_pid */
	/* TODO: unkown meaning, set to 0 */
	sqlite3_bind_int(stmt, ++idx, 0);
	/* media_kinds */
	/* TODO: not sure, set to 1 */
	sqlite3_bind_int(stmt, ++idx, 1);
	/* workout_template_id */
	/* seems to be always 0 for this special playlist */
	sqlite3_bind_int(stmt, ++idx, 0);
	/* is_hidden */
	/* seems to be always 1 for this special playlist */
	sqlite3_bind_int(stmt, ++idx, 1);
        /* smart_is_folder */
	/* seems to be always 0 for this special playlist */
	sqlite3_bind_int(stmt, ++idx, 0);

	res = sqlite3_step(stmt);
	if (res == SQLITE_DONE) {
	    /* expected result */
	} else {
	    fprintf(stderr, "[%s] 4 sqlite3_step returned %d\n", __func__, res); 
	}
    }
    if (stmt) {
	sqlite3_finalize(stmt);
	stmt = NULL;
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

	if (SQLITE_OK != sqlite3_prepare_v2(db, "INSERT INTO \"genre_map\" (id,genre,genre_order) VALUES(?,?,0);", -1, &stmt, &sqltail)) {
	    fprintf(stderr, "[%s] sqlite3_prepare error: %s\n", __func__, sqlite3_errmsg(db));
	    goto leave;
	} else {
	    int res;
	    idx = 0;
	    /* id */
	    sqlite3_bind_int(stmt, ++idx, genre_index++);
	    /* genre */
	    sqlite3_bind_text(stmt, ++idx, track->genre, -1, SQLITE_STATIC);

	    res = sqlite3_step(stmt);
	    if (res != SQLITE_DONE) {
		fprintf(stderr, "[%s] sqlite3_step returned %d\n", __func__, res);
		goto leave;
	    }
	}
	if (stmt) {
		sqlite3_finalize(stmt);
		stmt = NULL;
	}
    }

    /* for each track: */
    printf("[%s] - processing %d tracks\n", __func__, g_list_length(itdb->tracks));
    for (gl = itdb->tracks; gl; gl = gl->next) {
	Itdb_Track *track = gl->data;
	Itdb_Item_Id *this_album = NULL;
	Itdb_Item_Id *this_artist = NULL;
	uint64_t album_pid = 0;
	uint64_t artist_pid = 0;
	gpointer genre_id = NULL;

	printf("[%s] -- inserting into \"item\"\n", __func__);
	/* INSERT INTO "item" VALUES(-6197982141081478573,NULL,1,1,0,0,0,0,0,0,0,0,0,0,2004,0,0,0,0,0,0,2,0,0.0,0.0,213812.0,NULL,1,0,0,0,0,0,NULL,NULL,0,0,0,-7670716306765259718,-7576753259813584028,0,'Enjoy the silence 2004 (Reinterpreted by Mike Shinoda)','Depeche Mode','Enjoy The Silence 04 CDS','Depeche Mode',NULL,'Enjoy the silence 2004 (Reinterpreted by Mike Shinoda)','Depeche Mode','Enjoy The Silence 04 CDS','Depeche Mode',NULL,100,100,100,100,100,100,NULL,100,'Lbl MUTE C:MUTE RCDBONG034',NULL,NULL,NULL,1,0,0,0,0,1,1,NULL,NULL,NULL,NULL,NULL,NULL,NULL); */
	if (SQLITE_OK != sqlite3_prepare_v2(db, "INSERT INTO \"item\" (pid,media_kind,date_modified,year,is_compilation,exclude_from_shuffle,artwork_status,artwork_cache_id,start_time_ms,stop_time_ms,total_time_ms,total_burn_time_ms,track_number,track_count,disc_number,disc_count,bpm,relative_volume,eq_preset,radio_stream_status,genre_id,album_pid,artist_pid,composer_pid,title,artist,album,album_artist,composer,sort_title,sort_artist,sort_album,sort_album_artist,sort_composer,title_order,artist_order,album_order,genre_order,composer_order,album_artist_order,album_by_artist_order,series_name_order,comment,grouping,description,description_long,title_section_order,artist_section_order,album_section_order,album_artist_section_order,composer_section_order,genre_section_order,series_name_section_order) VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?);", -1, &stmt, &sqltail)) {
	    fprintf(stderr, "[%s] sqlite3_prepare error: %s\n", __func__, sqlite3_errmsg(db));
	    goto leave;
	} else {
	    int aw_id;
	    int res;
	    idx = 0;
	    /* pid */
	    sqlite3_bind_int64(stmt, ++idx, track->dbid);
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
	    sqlite3_bind_int(stmt, ++idx, track->mediatype);
	    /* date_modified, set to 0 */
	    sqlite3_bind_int(stmt, ++idx, 0);
	    /* year */
	    sqlite3_bind_int(stmt, ++idx, track->year);
	    /* is_compilation */
	    sqlite3_bind_int(stmt, ++idx, track->compilation);
	    /* exclude_from_shuffle */
	    sqlite3_bind_int(stmt, ++idx, track->skip_when_shuffling);
    	    /* artwork_status 1 = has artwork, 2 = doesn't */
	    sqlite3_bind_int(stmt, ++idx, track->has_artwork);
	    /* artwork_cache_id */
	    /* TODO check if this value is correct */
	    aw_id = 0;
	    if (track->artwork) {
		aw_id = track->artwork->id;
	    }
	    sqlite3_bind_int(stmt, ++idx, aw_id);
	    /* start_time_ms */
	    sqlite3_bind_double(stmt, ++idx, track->starttime);
	    /* stop_time_ms */
	    sqlite3_bind_double(stmt, ++idx, track->stoptime);
	    /* total_time_ms */
	    sqlite3_bind_double(stmt, ++idx, track->tracklen);
	    /* total_burn_time_ms --> set to NULL */
	    /* TODO where is this stored? */
	    sqlite3_bind_null(stmt, ++idx);
	    /* track_number */
	    sqlite3_bind_int(stmt, ++idx, track->track_nr);
	    /* track_count */
	    sqlite3_bind_int(stmt, ++idx, track->tracks);
	    /* disc_number */
	    sqlite3_bind_int(stmt, ++idx, track->cd_nr);
	    /* disc_count */
	    sqlite3_bind_int(stmt, ++idx, track->cds);
	    /* bpm */
	    sqlite3_bind_int(stmt, ++idx, track->BPM);
	    /* relative_volume */
	    sqlite3_bind_int(stmt, ++idx, track->volume);
	    /* eq_preset --> set to NULL */
	    sqlite3_bind_null(stmt, ++idx);
	    /* radio_stream_status --> set to NULL */
	    sqlite3_bind_null(stmt, ++idx);
	    /* genre_id */
	    genre_id = NULL;
	    if (track->genre && *track->genre && (genre_id = g_hash_table_lookup(genre_map, track->genre)) != NULL) {
		sqlite3_bind_int(stmt, ++idx, GPOINTER_TO_UINT(genre_id));
	    }
	    else
		sqlite3_bind_int(stmt, ++idx, 0);
	    /* album_pid -7670716306765259718 */
	    this_album = g_hash_table_lookup(album_ids, track);
	    if (this_album) {
		album_pid = this_album->sql_id;
	    }
	    sqlite3_bind_int64(stmt, ++idx, album_pid);
	    /* artist_pid -7576753259813584028 */
	    this_artist = g_hash_table_lookup(artist_ids, track);
	    if (this_artist) {
		artist_pid = this_artist->sql_id;
	    }
	    sqlite3_bind_int64(stmt, ++idx, artist_pid);
	    /* composer_pid */
	    /* FIXME TODO libgpod support missing! */
	    sqlite3_bind_int64(stmt, ++idx, 0);
	    /* title */
	    if (track->title && *track->title) {
		sqlite3_bind_text(stmt, ++idx, track->title, -1, SQLITE_STATIC);
	    } else {
		sqlite3_bind_null(stmt, ++idx);
	    }
	    /* artist */
	    if (track->artist && *track->artist) {
		sqlite3_bind_text(stmt, ++idx, track->artist, -1, SQLITE_STATIC);
	    } else {
		sqlite3_bind_null(stmt, ++idx);
	    }
	    /* album */
	    if (track->album && *track->album) {
		sqlite3_bind_text(stmt, ++idx, track->album, -1, SQLITE_STATIC);
	    } else {
		sqlite3_bind_null(stmt, ++idx);
	    }
	    /* album_artist */
	    if (track->albumartist && *track->albumartist) {
		sqlite3_bind_text(stmt, ++idx, track->albumartist, -1, SQLITE_STATIC);
	    } else {
		sqlite3_bind_null(stmt, ++idx);
	    }
	    /* composer */
	    if (track->composer && *track->composer) {
		sqlite3_bind_text(stmt, ++idx, track->composer, -1, SQLITE_STATIC);
	    } else {
		sqlite3_bind_null(stmt, ++idx);
	    }
	    /* sort_title */
	    if (track->sort_title && *track->sort_title) {
		sqlite3_bind_text(stmt, ++idx, track->sort_title, -1, SQLITE_STATIC);
	    } else {
		if (track->title && *track->title) {
		    sqlite3_bind_text(stmt, ++idx, track->title, -1, SQLITE_STATIC);
		} else {
		    sqlite3_bind_null(stmt, ++idx);
		}
	    }
	    /* sort_artist */
	    if (track->sort_artist && *track->sort_artist) {
		sqlite3_bind_text(stmt, ++idx, track->sort_artist, -1, SQLITE_STATIC);
	    } else {
		if (track->artist && *track->artist) {
		    sqlite3_bind_text(stmt, ++idx, track->artist, -1, SQLITE_STATIC);
		} else {
		    sqlite3_bind_null(stmt, ++idx);
		}
	    }
	    /* sort_album */
	    if (track->sort_album && *track->sort_album) {
		sqlite3_bind_text(stmt, ++idx, track->sort_album, -1, SQLITE_STATIC);
	    } else {
		if (track->album && *track->album) {
		    sqlite3_bind_text(stmt, ++idx, track->album, -1, SQLITE_STATIC);
		} else {
		   sqlite3_bind_null(stmt, ++idx);
		}
	    }
	    /* sort_album_artist */
	    if (track->sort_albumartist && *track->sort_albumartist) {
		sqlite3_bind_text(stmt, ++idx, track->sort_albumartist, -1, SQLITE_STATIC);
	    } else {
		if (track->albumartist && *track->albumartist) {
		    sqlite3_bind_text(stmt, ++idx, track->albumartist, -1, SQLITE_STATIC);
		} else {
		    sqlite3_bind_null(stmt, ++idx);
		}
	    }
	    /* sort_composer */
	    if (track->sort_composer && *track->sort_composer) {
		sqlite3_bind_text(stmt, ++idx, track->sort_composer, -1, SQLITE_STATIC);
	    } else {
		if (track->composer && *track->composer) {
		    sqlite3_bind_text(stmt, ++idx, track->composer, -1, SQLITE_STATIC);
		} else {
		    sqlite3_bind_null(stmt, ++idx);
		}
	    }

	    /* title_order */
	    /* TODO figure out where these values are stored */
	    sqlite3_bind_int(stmt, ++idx, 100);
	    /* artist_order */
	    sqlite3_bind_int(stmt, ++idx, 100);
	    /* album_order */
	    sqlite3_bind_int(stmt, ++idx, 100);
	    /* genre_order */
	    sqlite3_bind_int(stmt, ++idx, 100);
	    /* composer_order */
	    sqlite3_bind_int(stmt, ++idx, 100);
	    /* album_artist_order */
	    sqlite3_bind_int(stmt, ++idx, 100);
	    /* album_by_artist_order */
	    sqlite3_bind_null(stmt, ++idx);
	    /* series_name_order */
	    sqlite3_bind_int(stmt, ++idx, 100);
	    /* comment */
	    sqlite3_bind_text(stmt, ++idx, track->comment, -1, SQLITE_STATIC);
	    /* grouping */
	    if (!track->grouping) {
		sqlite3_bind_null(stmt, ++idx);
	    } else {
		sqlite3_bind_text(stmt, ++idx, track->grouping, -1, SQLITE_STATIC);
	    }
	    /* description */
	    if (!track->description) {
		sqlite3_bind_null(stmt, ++idx);
	    } else {
		sqlite3_bind_text(stmt, ++idx, track->description, -1, SQLITE_STATIC);
	    }
	    /* description_long */
	    /* TODO libgpod doesn't know about it */
	    sqlite3_bind_null(stmt, ++idx);
	    /* title_section_order */
	    sqlite3_bind_null(stmt, ++idx);
	    /* artist_section_order */
	    sqlite3_bind_null(stmt, ++idx);
	    /* album_section_order */
	    sqlite3_bind_null(stmt, ++idx);
	    /* album_artist_section_order */
	    sqlite3_bind_null(stmt, ++idx);
	    /* composer_section_order */
	    sqlite3_bind_null(stmt, ++idx);
	    /* genre_section_order */
	    sqlite3_bind_null(stmt, ++idx);
	    /* series_name_section_order */
	    sqlite3_bind_null(stmt, ++idx);

	    res = sqlite3_step(stmt);
	    if (res != SQLITE_DONE) {
		fprintf(stderr, "[%s] 6 sqlite3_step returned %d\n", __func__, res);
		goto leave;
	    }
	}
	if (stmt) {
	    sqlite3_finalize(stmt);
	    stmt = NULL;
	}

	printf("[%s] -- inserting into \"location_kind_map\" (if required)\n", __func__);
	/* INSERT INTO "location_kind_map" VALUES(1,'MPEG-Audiodatei'); */
	if (SQLITE_OK != sqlite3_prepare_v2(db, "INSERT OR IGNORE INTO \"location_kind_map\" VALUES(?,:kind);", -1, &stmt, &sqltail)) {
	    fprintf(stderr, "[%s] sqlite3_prepare error: %s\n", __func__, sqlite3_errmsg(db));
	    goto leave;
	} else {
	    int res;
	    idx = 0;
	    /* id */
	    sqlite3_bind_int(stmt, ++idx, track->mediatype);
	    /* kind */
	    sqlite3_bind_text(stmt, ++idx, track->filetype, -1, SQLITE_STATIC);
	    
	    res = sqlite3_step(stmt);
	    if (res == SQLITE_DONE) {
		/* expected result */
	    } else {
		fprintf(stderr, "[%s] 5 sqlite3_step returned %d: %s\n", __func__, res, sqlite3_errmsg(db)); 
	    }
	}
	if (stmt) {
	    sqlite3_finalize(stmt);
	    stmt = NULL;
	}

	printf("[%s] -- inserting into \"avformat_info\"\n", __func__);
	/* INSERT INTO "avformat_info" VALUES(-6197982141081478573,0,301,232,44100.0,9425664,1,576,2880,6224207,0,0,0); */
	if (SQLITE_OK != sqlite3_prepare_v2(db, "INSERT INTO \"avformat_info\" VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?);", -1, &stmt, &sqltail)) {
	    fprintf(stderr, "[%s] sqlite3_prepare error: %s\n", __func__, sqlite3_errmsg(db));
	    goto leave;
	} else {
	    int audio_format;
	    int res;
	    idx = 0;
	    /* item_pid */
	    sqlite3_bind_int64(stmt, ++idx, track->dbid);
	    /* sub_id */
	    /* TODO what is this? set to 0 */
	    sqlite3_bind_int64(stmt, ++idx, 0);
	    /* audio_format, TODO what's this? */
	    switch (track->filetype_marker) {
		case 0x4d503320:
		    audio_format = 301;
		    break;
		default:
		    audio_format = 0;
	    }
	    sqlite3_bind_int(stmt, ++idx, audio_format);
	    /* bit_rate */
	    sqlite3_bind_int(stmt, ++idx, track->bitrate);
	    /* sample_rate */
	    sqlite3_bind_double(stmt, ++idx, track->samplerate);
	    /* duration */
	    sqlite3_bind_int(stmt, ++idx, track->tracklen);
	    /* gapless_heuristic_info */
	    sqlite3_bind_int(stmt, ++idx, track->gapless_track_flag);
	    /* gapless_encoding_delay */
	    sqlite3_bind_int(stmt, ++idx, track->pregap);
	    /* gapless_encoding_drain */
	    sqlite3_bind_int(stmt, ++idx, track->postgap);
	    /* gapless_last_frame_resynch */
	    sqlite3_bind_int(stmt, ++idx, track->gapless_data);
	    /* analysis_inhibit_flags */
	    /* TODO don't know where this belongs to */
	    sqlite3_bind_int(stmt, ++idx, 0);
	    /* audio_fingerprint */
	    /* TODO this either */
	    sqlite3_bind_int(stmt, ++idx, 0);
	    /* volume_normalization_energy */
	    /* TODO and where is this value stored?! */
	    sqlite3_bind_int(stmt, ++idx, 0);

	    res = sqlite3_step(stmt);
	    if (res == SQLITE_DONE) {
		/* expected result */
	    } else {
		fprintf(stderr, "[%s] 7 sqlite3_step returned %d\n", __func__, res); 
	    }
	}
	if (stmt) {
	    sqlite3_finalize(stmt);
	    stmt = NULL;
	}

	printf("[%s] -- inserting into \"item_to_container\"\n", __func__);
	/* INSERT INTO "item_to_container" VALUES(-6197982141081478573,959107999841118509,0,NULL); */
	if (SQLITE_OK != sqlite3_prepare_v2(db, "INSERT INTO \"item_to_container\" VALUES(?,?,?,?);", -1, &stmt, &sqltail)) {
	    fprintf(stderr, "[%s] sqlite3_prepare error: %s\n", __func__, sqlite3_errmsg(db));
	    goto leave;
	} else {
	    int res;
	    idx = 0;
	    /* item_pid */
	    sqlite3_bind_int64(stmt, ++idx, track->dbid);
	    /* container_pid */
	    sqlite3_bind_int64(stmt, ++idx, dev_playlist->id);
	    /* physical_order */
	    /* TODO what's this? set to 0 */
	    sqlite3_bind_int(stmt, ++idx, 0);
	    /* shuffle_order */
	    /* TODO what's this? set to NULL */
	    sqlite3_bind_null(stmt, ++idx);

	    res = sqlite3_step(stmt);
	    if (res == SQLITE_DONE) {
		/* expected result */
	    } else {
		fprintf(stderr, "[%s] 8 sqlite3_step returned %d\n", __func__, res); 
	    }
	}
	if (stmt) {
	    sqlite3_finalize(stmt);
	    stmt = NULL;
	}

	/* this is done by a trigger, so we don't need to do this :-D */
	/* INSERT INTO "ext_item_view_membership" VALUES(-6197982141081478573,0,0); */

	if (this_album) {
	printf("[%s] -- inserting into \"album\" (if required)\n", __func__);
	/* INSERT INTO "album" VALUES(-7670716306765259718,2,0,0,-7576753259813584028,0,'Enjoy The Silence 04 CDS',100,0,NULL,0); */
	if (SQLITE_OK != sqlite3_prepare_v2(db, "INSERT OR IGNORE INTO \"album\" VALUES(?,?,?,?,?,?,?,?,?,?,?);", -1, &stmt, &sqltail)) {
	    fprintf(stderr, "[%s] sqlite3_prepare error: %s\n", __func__, sqlite3_errmsg(db));
	    goto leave;
	} else {
	    int res;
	    idx = 0;
	    /* pid */
	    sqlite3_bind_int64(stmt, ++idx, album_pid);
	    /* kind */
	    /* TODO use field from mhia */
	    sqlite3_bind_int(stmt, ++idx, 2);
	    /* artwork_status */
	    /* TODO */
	    sqlite3_bind_int(stmt, ++idx, 0);
	    /* artwork_item_pid */
	    /* TODO */
	    sqlite3_bind_int(stmt, ++idx, 0);
	    /* artist_pid */
	    sqlite3_bind_int64(stmt, ++idx, artist_pid);
	    /* user_rating */
	    sqlite3_bind_int(stmt, ++idx, 0);
	    /* name */
	    sqlite3_bind_text(stmt, ++idx, track->title, -1, SQLITE_STATIC);
	    /* name_order */
	    /* TODO */
	    sqlite3_bind_int(stmt, ++idx, 100);
	    /* all_compilations */
	    /* TODO */
	    sqlite3_bind_int(stmt, ++idx, 0);
	    /* feed_url */
	    /* TODO this is for podcasts? */
	    sqlite3_bind_null(stmt, ++idx);
	    /* season_number */
	    /* TODO this is for tv shows?! */
	    sqlite3_bind_int(stmt, ++idx, 0);

	    res = sqlite3_step(stmt);
	    if (res == SQLITE_DONE) {
		/* expected result */
	    } else {
		fprintf(stderr, "[%s] 8 sqlite3_step returned %d\n", __func__, res); 
	    }
	}
	if (stmt) {
	    sqlite3_finalize(stmt);
	    stmt = NULL;
	}
	}

	if (this_artist) {
	printf("[%s] -- inserting into \"artist\" (if required)\n", __func__);
	/* INSERT INTO "artist" VALUES(-7576753259813584028,2,0,0,'Depeche Mode',100,'Depeche Mode'); */
	if (SQLITE_OK != sqlite3_prepare_v2(db, "INSERT OR IGNORE INTO \"artist\" VALUES(?,?,?,?,?,?,?);", -1, &stmt, &sqltail)) {
	    fprintf(stderr, "[%s] sqlite3_prepare error: %s\n", __func__, sqlite3_errmsg(db));
	    goto leave;
	} else {
	    int res;
	    idx = 0;
	    /* pid */
	    sqlite3_bind_int64(stmt, ++idx, artist_pid);
	    /* kind */
	    /* TODO use field from mhii */
	    sqlite3_bind_int(stmt, ++idx, 2);
	    /* artwork_status */
	    /* TODO */
	    sqlite3_bind_int(stmt, ++idx, 0);
	    /* artwork_album_pid */
	    /* TODO */
	    sqlite3_bind_int(stmt, ++idx, 0);
	    /* name */
	    sqlite3_bind_text(stmt, ++idx, track->artist, -1, SQLITE_STATIC);
	    /* name_order */
	    /* TODO */
	    sqlite3_bind_int(stmt, ++idx, 100);
	    /* sort_name */
	    /* TODO always same as 'name'? */
	    sqlite3_bind_text(stmt, ++idx, track->artist, -1, SQLITE_STATIC);

	    res = sqlite3_step(stmt);
	    if (res == SQLITE_DONE) {
		/* expected result */
	    } else {
		fprintf(stderr, "[%s] 8 sqlite3_step returned %d\n", __func__, res); 
	    }
	}
	if (stmt) {
	    sqlite3_finalize(stmt);
	    stmt = NULL;
	}
	}

    }
    printf("[%s] done.\n", __func__);
leave:
    if (genre_map) {
	g_hash_table_destroy(genre_map);
    }
    if (db) {
	sqlite3_close(db);
    }
    if (dbf) {
	g_free(dbf);
    }
}

static void mk_Locations(Itdb_iTunesDB *itdb, const char *outpath, const char *uuid)
{
    gchar *dbf = NULL;
    sqlite3 *db = NULL;
    sqlite3_stmt *stmt = NULL;
    const char *sqltail = NULL;
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

    /* kill all entries in 'location' as they will be re-inserted */
    if (SQLITE_OK != sqlite3_exec(db, "DELETE FROM location;", NULL, NULL, &errmsg)) {
	fprintf(stderr, "[%s] sqlite3_exec error: %s\n", __func__, sqlite3_errmsg(db));
	if (errmsg) {
	    fprintf(stderr, "[%s] additional error information: %s\n", __func__, errmsg);
	    sqlite3_free(errmsg);
	    errmsg = NULL;
	}
	goto leave;
    }

    if (itdb->tracks) {
	GList *gl = NULL;

	printf("[%s] Processing %d tracks...\n", __func__, g_list_length(itdb->tracks));
	for (gl=itdb->tracks; gl; gl=gl->next) {
	    Itdb_Track *track = gl->data;
	    if (!track->ipod_path) {
		continue;
	    }

	    if (SQLITE_OK != sqlite3_prepare_v2(db, "INSERT INTO \"location\" (item_pid, sub_id, base_location_id, location_type, location, extension, kind_id, date_created, file_size) VALUES(?,?,?,?,?,?,?,?,?);", -1, &stmt, &sqltail)) {
		fprintf(stderr, "[%s] sqlite3_prepare error: %s\n", __func__, sqlite3_errmsg(db));
	    } else {
		char *ipod_path = strdup(track->ipod_path);
		int i = 0;
		int cnt = 0;
		int pos = 0;
		int res;
		idx = 0;
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
	    if (stmt) {
		sqlite3_finalize(stmt);
		stmt = NULL;
	    }
	}
    } else {
	printf("[%s] No tracks available, none written.\n", __func__);
    }

    printf("[%s] done.\n", __func__);
leave:
    if (db) {
	sqlite3_close(db);
    }
    if (dbf) {
	g_free(dbf);
    }
}

static int cbk_calc_sha1_one_block (FILE *f, unsigned char sha1[20])
{
    const guint BLOCK_SIZE = 1024;
    unsigned char block[BLOCK_SIZE];
    size_t read_count;
    GChecksum *checksum;
    gsize sha1_len;

    read_count = fread (block, BLOCK_SIZE, 1, f);
    if ((read_count != 1)) {
	if (feof (f)) {
	    return 1;
	} else {
	    return -1;
	}
    }

    sha1_len = g_checksum_type_get_length (G_CHECKSUM_SHA1);
    g_assert (sha1_len == 20);
    checksum = g_checksum_new (G_CHECKSUM_SHA1);
    g_checksum_update (checksum, block, BLOCK_SIZE);
    g_checksum_get_digest (checksum, sha1, &sha1_len);
    g_checksum_free (checksum);

    return 0;
}

static gboolean cbk_calc_sha1s (const char *filename, GArray *sha1s)
{
    FILE *f;
    int calc_ok;

    f = fopen (filename, "rb");
    if (f == NULL) {
	return FALSE;
    }

    do {
	unsigned char sha1[20];
	calc_ok = cbk_calc_sha1_one_block (f, sha1);
	if (calc_ok != 0) {
	    break;
	}
	g_array_append_vals (sha1s, sha1, sizeof(sha1));
    } while (calc_ok == 0);

    if (calc_ok < 0) {
	goto error;
    }

    fclose (f);
    return TRUE;

error:
    fclose (f);
    return FALSE;
}

static const guint CBK_HEADER_SIZE = 46;

static void cbk_calc_sha1_of_sha1s (GArray *cbk)
{
    GChecksum *checksum;
    unsigned char* final_sha1;
    unsigned char* sha1s;
    gsize final_sha1_len;

    g_assert (cbk->len > CBK_HEADER_SIZE + 20);

    final_sha1 = &g_array_index(cbk, guchar, CBK_HEADER_SIZE);
    sha1s = &g_array_index (cbk, guchar, CBK_HEADER_SIZE + 20);
    final_sha1_len = g_checksum_type_get_length (G_CHECKSUM_SHA1);
    g_assert (final_sha1_len == 20);

    checksum = g_checksum_new (G_CHECKSUM_SHA1);
    g_checksum_update (checksum, sha1s, cbk->len - (CBK_HEADER_SIZE + 20));
    g_checksum_get_digest (checksum, final_sha1, &final_sha1_len);
    g_checksum_free (checksum);
}

static gboolean mk_Locations_cbk (Itdb_iTunesDB *itdb, const char *dirname)
{
    char *locations_filename;
    char *cbk_filename;
    GArray *cbk;
    gboolean success;
    guchar *cbk_hash72;
    guchar *final_sha1;

    cbk = g_array_sized_new (FALSE, TRUE, 1,
			     CBK_HEADER_SIZE + 20);
    g_array_set_size (cbk, CBK_HEADER_SIZE + 20);

    locations_filename = g_build_filename (dirname, "Locations.itdb", NULL);
    success = cbk_calc_sha1s (locations_filename, cbk);
    g_free (locations_filename);
    if (!success) {
	g_array_free (cbk, TRUE);
	return FALSE;
    }
    cbk_calc_sha1_of_sha1s (cbk);
    final_sha1 = &g_array_index (cbk, guchar, CBK_HEADER_SIZE);
    cbk_hash72 = &g_array_index (cbk, guchar, 0);
    success = itdb_hash72_compute_hash_for_sha1 (itdb->device, final_sha1,
						 cbk_hash72);
    if (!success) {
	g_array_free (cbk, TRUE);
	return FALSE;
    }

    cbk_filename = g_build_filename (dirname, "Locations.itdb.cbk", NULL);
    success = g_file_set_contents (cbk_filename, cbk->data, cbk->len, NULL);
    g_free (cbk_filename);
    g_array_free (cbk, TRUE);
    return success;
}

static void build_itdb_files(Itdb_iTunesDB *itdb,
			     GHashTable *album_ids, GHashTable *artist_ids,
			     const char *outpath, const char *uuid)
{
    mk_Dynamic(itdb, outpath);
    mk_Extras(itdb, outpath);
    mk_Genius(itdb, outpath);
    mk_Library(itdb, album_ids, artist_ids, outpath);
    mk_Locations(itdb, outpath, uuid);
    mk_Locations_cbk(itdb, outpath);
}

static int ensure_itlp_dir_exists(const char *itlpdir)
{
    /* check if directory exists */
    if (!g_file_test(itlpdir, G_FILE_TEST_EXISTS)) {
	int success;
	success = g_mkdir(itlpdir, 0755);
	if (!success) {
		fprintf(stderr, "Could not create directory '%s': %s\n", itlpdir, strerror(errno));
		return FALSE;
	}
    } else if (!g_file_test(itlpdir, G_FILE_TEST_IS_DIR)) {
	    fprintf(stderr, "'%s' is not a directory as it should be!\n", itlpdir);
	    return FALSE;
    }

    return TRUE;
}

int itdb_sqlite_generate_itdbs(FExport *fexp)
{
    int res = 0;
    gchar *itlpdir;
    gchar *dirname;

    printf("libitdbprep: %s called with file %s and uuid %s\n", __func__,
	   fexp->itdb->filename, itdb_device_get_uuid(fexp->itdb->device));

    dirname = itdb_get_itunes_dir(itdb_get_mountpoint(fexp->itdb));
    itlpdir = g_build_filename(dirname, "iTunes Library.itlp", NULL);
    g_free(dirname);

    printf("itlp directory='%s'\n", itlpdir);

    if (!ensure_itlp_dir_exists(itlpdir)) {
	res = -1;
	goto leave;
    }

    printf("*.itdb files will be stored in '%s'\n", itlpdir);

    g_assert(fexp->itdb != NULL);
    g_assert(fexp->itdb->playlists != NULL);

    tzoffset = fexp->itdb->tzoffset;

    /* generate itdb files */
    build_itdb_files(fexp->itdb, fexp->albums, fexp->artists, itlpdir,
		     itdb_device_get_uuid(fexp->itdb->device));

leave:
    if (itlpdir) {
	g_free(itlpdir);
    }

    return res;
}

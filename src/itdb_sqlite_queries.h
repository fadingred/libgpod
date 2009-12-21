/*
|  Copyright (C) 2009 Nikias Bassen <nikias@gmx.li>

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
#ifndef __LIBITDBPREP_H
#define __LIBITDBPREP_H

/** creation statement for 'Dynamic.itdb' */
static const char Dynamic_create[] =
	"BEGIN TRANSACTION;" \
	"CREATE TABLE item_stats (item_pid INTEGER NOT NULL, has_been_played INTEGER DEFAULT 0, date_played INTEGER DEFAULT 0, play_count_user INTEGER DEFAULT 0, play_count_recent INTEGER DEFAULT 0, date_skipped INTEGER DEFAULT 0, skip_count_user INTEGER DEFAULT 0, skip_count_recent INTEGER DEFAULT 0, bookmark_time_ms REAL, bookmark_time_ms_common REAL, user_rating INTEGER DEFAULT 0, user_rating_common INTEGER DEFAULT 0, rental_expired INTEGER DEFAULT 0, hidden INTEGER DEFAULT 0, deleted INTEGER DEFAULT 0, has_changes INTEGER DEFAULT 0, PRIMARY KEY (item_pid));" \
	"CREATE TABLE container_ui (container_pid INTEGER NOT NULL, play_order INTEGER DEFAULT 0, is_reversed INTEGER DEFAULT 0, album_field_order INTEGER DEFAULT 0, repeat_mode INTEGER DEFAULT 0, shuffle_items INTEGER DEFAULT 0, has_been_shuffled INTEGER DEFAULT 0, PRIMARY KEY (container_pid));" \
	"CREATE TABLE rental_info (item_pid INTEGER NOT NULL, rental_date_started INTEGER DEFAULT 0, rental_duration INTEGER DEFAULT 0, rental_playback_date_started INTEGER DEFAULT 0, rental_playback_duration INTEGER DEFAULT 0, is_demo INTEGER DEFAULT 0, PRIMARY KEY (item_pid));" \
	"COMMIT;";

/** creation statement for 'Extras.itdb' */
static const char Extras_create[] =
	"BEGIN TRANSACTION;" \
	"CREATE TABLE chapter (item_pid INTEGER NOT NULL, data BLOB, PRIMARY KEY (item_pid));" \
	"CREATE TABLE lyrics (item_pid INTEGER NOT NULL, checksum INTEGER, lyrics TEXT, PRIMARY KEY (item_pid));" \
	"ANALYZE sqlite_master;" \
	"CREATE TABLE db_schema_upgrade_info (user_version INTEGER, device_version INTEGER);" \
	"INSERT INTO \"db_schema_upgrade_info\" VALUES(8,19);" \
	"COMMIT;";

/** creation statement for 'Genius.itdb' */
static const char Genius_create[] =
	"BEGIN TRANSACTION;" \
	"CREATE TABLE genius_metadata (genius_id INTEGER NOT NULL, version INTEGER, data BLOB, PRIMARY KEY (genius_id));" \
	"CREATE TABLE genius_similarities (genius_id INTEGER NOT NULL, version INTEGER, data BLOB, PRIMARY KEY (genius_id));" \
	"CREATE TABLE genius_config (id INTEGER NOT NULL, version INTEGER, default_num_results INTEGER DEFAULT 0, min_num_results INTEGER DEFAULT 0, data BLOB, PRIMARY KEY (id), UNIQUE (version));" \
	"COMMIT;";

/** creation statement for 'Library.itdb' */
static const char Library_create[] =
	"BEGIN TRANSACTION;" \
	"CREATE TABLE version_info (id INTEGER PRIMARY KEY, major INTEGER, minor INTEGER, compatibility INTEGER DEFAULT 0, update_level INTEGER DEFAULT 0, device_update_level INTEGER DEFAULT 0, platform INTEGER DEFAULT 0);" \
	"CREATE TABLE db_info (pid INTEGER NOT NULL, primary_container_pid INTEGER, media_folder_url TEXT, audio_language INTEGER, subtitle_language INTEGER, genius_cuid TEXT, bib BLOB, rib BLOB, PRIMARY KEY (pid));" \
	"CREATE TABLE item (pid INTEGER NOT NULL, revision_level INTEGER, media_kind INTEGER DEFAULT 0, is_song INTEGER DEFAULT 0, is_audio_book INTEGER DEFAULT 0, is_music_video INTEGER DEFAULT 0, is_movie INTEGER DEFAULT 0, is_tv_show INTEGER DEFAULT 0, is_ringtone INTEGER DEFAULT 0, is_voice_memo INTEGER DEFAULT 0, is_rental INTEGER DEFAULT 0, is_itunes_u INTEGER DEFAULT 0, is_podcast INTEGER DEFAULT 0, date_modified INTEGER DEFAULT 0, date_backed_up INTEGER DEFAULT 0, year INTEGER DEFAULT 0, content_rating INTEGER DEFAULT 0, content_rating_level INTEGER DEFAULT 0, is_compilation INTEGER, is_user_disabled INTEGER DEFAULT 0, remember_bookmark INTEGER DEFAULT 0, exclude_from_shuffle INTEGER DEFAULT 0, part_of_gapless_album INTEGER DEFAULT 0, artwork_status INTEGER, artwork_cache_id INTEGER DEFAULT 0, start_time_ms REAL DEFAULT 0, stop_time_ms REAL DEFAULT 0, total_time_ms REAL DEFAULT 0, total_burn_time_ms REAL, track_number INTEGER DEFAULT 0, track_count INTEGER DEFAULT 0, disc_number INTEGER DEFAULT 0, disc_count INTEGER DEFAULT 0, bpm INTEGER DEFAULT 0, relative_volume INTEGER, eq_preset TEXT, radio_stream_status TEXT, genius_id INTEGER DEFAULT 0, genre_id INTEGER DEFAULT 0, category_id INTEGER DEFAULT 0, album_pid INTEGER DEFAULT 0, artist_pid INTEGER DEFAULT 0, composer_pid INTEGER DEFAULT 0, title TEXT, artist TEXT, album TEXT, album_artist TEXT, composer TEXT, sort_title TEXT, sort_artist TEXT, sort_album TEXT, sort_album_artist TEXT, sort_composer TEXT, title_order INTEGER, artist_order INTEGER, album_order INTEGER, genre_order INTEGER, composer_order INTEGER, album_artist_order INTEGER, album_by_artist_order INTEGER, series_name_order INTEGER, comment TEXT, grouping TEXT, description TEXT, description_long TEXT, PRIMARY KEY (pid));" \
	"CREATE TABLE avformat_info (item_pid INTEGER NOT NULL, sub_id INTEGER NOT NULL DEFAULT 0, audio_format INTEGER, bit_rate INTEGER DEFAULT 0, sample_rate REAL DEFAULT 0, duration INTEGER, gapless_heuristic_info INTEGER, gapless_encoding_delay INTEGER, gapless_encoding_drain INTEGER, gapless_last_frame_resynch INTEGER, analysis_inhibit_flags INTEGER, audio_fingerprint INTEGER, volume_normalization_energy INTEGER, PRIMARY KEY (item_pid,sub_id));" \
	"CREATE TABLE video_info (item_pid INTEGER NOT NULL, has_alternate_audio INTEGER, has_subtitles INTEGER, characteristics_valid INTEGER, has_closed_captions INTEGER, is_self_contained INTEGER, is_compressed INTEGER, is_anamorphic INTEGER, season_number INTEGER, audio_language INTEGER, audio_track_index INTEGER, audio_track_id INTEGER, subtitle_language INTEGER, subtitle_track_index INTEGER, subtitle_track_id INTEGER, series_name TEXT, sort_series_name TEXT, episode_id TEXT, episode_sort_id INTEGER, network_name TEXT, extended_content_rating TEXT, movie_info TEXT, PRIMARY KEY (item_pid));" \
	"CREATE TABLE video_characteristics (item_pid INTEGER, sub_id INTEGER DEFAULT 0, track_id INTEGER, height INTEGER, width INTEGER, depth INTEGER, codec INTEGER, frame_rate REAL, percentage_encrypted REAL, bit_rate INTEGER, peak_bit_rate INTEGER, buffer_size INTEGER, profile INTEGER, level INTEGER, complexity_level INTEGER, UNIQUE (item_pid,sub_id,track_id));" \
	"CREATE TABLE store_info (item_pid INTEGER NOT NULL, store_kind INTEGER, date_purchased INTEGER DEFAULT 0, date_released INTEGER DEFAULT 0, account_id INTEGER, key_versions INTEGER, key_platform_id INTEGER, key_id INTEGER, key_id2 INTEGER, store_item_id INTEGER, artist_id INTEGER, composer_id INTEGER, genre_id INTEGER, playlist_id INTEGER, storefront_id INTEGER, store_link_id INTEGER, relevance REAL, popularity REAL, PRIMARY KEY (item_pid));" \
	"CREATE TABLE store_link (id INTEGER NOT NULL, url TEXT, PRIMARY KEY (id));" \
	"CREATE TABLE podcast_info (item_pid INTEGER NOT NULL, date_released INTEGER DEFAULT 0, external_guid TEXT, feed_url TEXT, feed_keywords TEXT, PRIMARY KEY (item_pid));" \
	"CREATE TABLE container (pid INTEGER NOT NULL, distinguished_kind INTEGER, date_created INTEGER, date_modified INTEGER, name TEXT, name_order INTEGER, parent_pid INTEGER, media_kinds INTEGER, workout_template_id INTEGER, is_hidden INTEGER, smart_is_folder INTEGER, smart_is_dynamic INTEGER, smart_is_filtered INTEGER, smart_is_genius INTEGER, smart_enabled_only INTEGER, smart_is_limited INTEGER, smart_limit_kind INTEGER, smart_limit_order INTEGER, smart_limit_value INTEGER, smart_reverse_limit_order INTEGER, smart_criteria BLOB, description TEXT, PRIMARY KEY (pid));" \
	"CREATE TABLE item_to_container (item_pid INTEGER, container_pid INTEGER, physical_order INTEGER, shuffle_order INTEGER);" \
	"CREATE TABLE container_seed (container_pid INTEGER NOT NULL, item_pid INTEGER NOT NULL, seed_order INTEGER DEFAULT 0, UNIQUE (container_pid,item_pid));" \
	"CREATE TABLE album (pid INTEGER NOT NULL, kind INTEGER, artwork_status INTEGER, artwork_item_pid INTEGER, artist_pid INTEGER, user_rating INTEGER, name TEXT, name_order INTEGER, all_compilations INTEGER, feed_url TEXT, season_number INTEGER, PRIMARY KEY (pid));" \
	"CREATE TABLE item_to_album (item_pid INTEGER, album_pid INTEGER);" \
	"CREATE TABLE artist (pid INTEGER NOT NULL, kind INTEGER, artwork_status INTEGER, artwork_album_pid INTEGER, name TEXT, name_order INTEGER, sort_name TEXT, PRIMARY KEY (pid));" \
	"CREATE TABLE item_to_artist (item_pid INTEGER, artist_pid INTEGER);" \
	"CREATE TABLE composer (pid INTEGER NOT NULL, name TEXT, name_order INTEGER, sort_name TEXT, PRIMARY KEY (pid));" \
	"CREATE TABLE item_to_composer (item_pid INTEGER, composer_pid INTEGER);" \
	"CREATE TABLE location_kind_map (id INTEGER NOT NULL, kind TEXT NOT NULL, PRIMARY KEY (id), UNIQUE (kind));" \
	"CREATE TABLE genre_map (id INTEGER NOT NULL, genre TEXT NOT NULL, genre_order INTEGER DEFAULT 0, PRIMARY KEY (id), UNIQUE (genre));" \
	"CREATE TABLE category_map (id INTEGER NOT NULL, category TEXT NOT NULL, PRIMARY KEY (id), UNIQUE (category));" \
	"CREATE TRIGGER insert_item AFTER INSERT ON item BEGIN " \
	"UPDATE item SET is_song=((new.media_kind&1)!=0), is_audio_book=((new.media_kind&8)!=0), is_music_video=((new.media_kind&32)!=0), is_movie=((new.media_kind&2)!=0), is_tv_show=((new.media_kind&64)!=0), is_ringtone=((new.media_kind&16384)!=0), is_podcast=((new.media_kind&4)!=0), is_rental=((new.media_kind&32768)!=0) WHERE pid = new.pid;" \
	"END;" \
	"CREATE TRIGGER update_item_media_kind AFTER UPDATE OF media_kind ON item BEGIN " \
	"UPDATE item SET is_song=((new.media_kind&1)!=0), is_audio_book=((new.media_kind&8)!=0), is_music_video=((new.media_kind&32)!=0), is_movie=((new.media_kind&2)!=0), is_tv_show=((new.media_kind&64)!=0), is_ringtone=((new.media_kind&16384)!=0), is_podcast=((new.media_kind&4)!=0), is_rental=((new.media_kind&32768)!=0) WHERE pid = new.pid;" \
	"END;" \
	"COMMIT;";

/** creation statement for 'Locations.itdb' */
static const char Locations_create[] =
	"BEGIN TRANSACTION;" \
	"CREATE TABLE location (item_pid INTEGER NOT NULL, sub_id INTEGER NOT NULL DEFAULT 0, base_location_id INTEGER DEFAULT 0, location_type INTEGER, location TEXT, extension INTEGER, kind_id INTEGER DEFAULT 0, date_created INTEGER DEFAULT 0, file_size INTEGER DEFAULT 0, file_creator INTEGER, file_type INTEGER, num_dir_levels_file INTEGER, num_dir_levels_lib INTEGER, PRIMARY KEY (item_pid,sub_id));" \
	"CREATE TABLE base_location (id INTEGER NOT NULL, path TEXT, PRIMARY KEY (id));" \
	"ANALYZE sqlite_master;" \
	"INSERT INTO \"sqlite_stat1\" VALUES('location','sqlite_autoindex_location_1','1 1 1');" \
	"COMMIT;";

#endif

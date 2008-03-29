/*
 * Copyright (C) 2006 Christophe Fergeau
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <typeinfo>
#include <unistd.h>

#include <glib/gstdio.h>

#include <id3v2tag.h>
#include <mpegfile.h>

#include "itdb.h"

static char *
itdb_resolve_path (Itdb_iTunesDB *db, const char *filename)
{
	const char *mountpoint;
	char *ipod_path;

	mountpoint = itdb_get_mountpoint (db);
	if (!g_str_has_prefix (filename, mountpoint)) {
		return NULL;
	}
	if (mountpoint[strlen(mountpoint)-1] == G_DIR_SEPARATOR) {
		ipod_path = g_strdup (filename + strlen (mountpoint)-1);
	} else {
		ipod_path = g_strdup (filename + strlen (mountpoint));
	}
	itdb_filename_fs2ipod (ipod_path);
#ifdef VERBOSE
	g_print ("Filename: %s\n", filename);
	g_print ("Mount point: %s (%d)\n", mountpoint, strlen (mountpoint));
	g_print ("Path resolved to %s\n", ipod_path);
#endif
	return ipod_path;
}

static Itdb_Track *
track_from_file (const char *filename)
{
	TagLib::MPEG::File file(filename);
	TagLib::Tag *tag;
	Itdb_Track *track;
	struct stat st;

	tag = file.tag();
	if (tag == NULL) {
		g_print ("Couldn't read tag\n");
		return NULL;
	}
/*	if (typeid (*tag) != typeid (TagLib::ID3v2::Tag)) {
		g_print ("Unknown tag type\n");
		return NULL;
		}*/

	/* FIXME: what happens if we try to open a non-MP3 file? */
#ifdef VERBOSE
	g_print ("%s:\n", filename);
	g_print ("\t%s\n", tag->artist().toCString(true));
	g_print ("\t%s\n", tag->album().toCString(true));
	g_print ("\t%s\n", tag->title().toCString(true));
	g_print ("\t%s\n", tag->genre().toCString(true));
	g_print ("\t%d\n", tag->year());
	g_print ("\t%d\n", tag->track());
	g_print ("\n");
#endif
	track = itdb_track_new ();
	track->title = g_strdup (tag->title().toCString(true));
	track->album = g_strdup (tag->album().toCString(true));
	track->artist = g_strdup (tag->artist().toCString(true));
	track->genre = g_strdup (tag->genre().toCString(true));
	track->comment = g_strdup (tag->comment().toCString(true));
	track->filetype = g_strdup ("MP3-file");
	if (g_stat (filename, &st) == 0) {
		track->size = st.st_size;
	}
	track->tracklen = file.audioProperties()->length() * 1000;
	track->track_nr = tag->track();
	track->bitrate = file.audioProperties()->bitrate();
	track->samplerate = file.audioProperties()->sampleRate();
	track->year = tag->year();
	return track;
}


static void
process_one_file (const char *filename, gpointer data)
{
	Itdb_iTunesDB *db;
	Itdb_Track *track;

	db = (Itdb_iTunesDB *)data;
	track = track_from_file (filename);
	track->ipod_path = itdb_resolve_path (db, filename);
	if (track->ipod_path == NULL) {
		itdb_track_free (track);
		return;
	}

	itdb_track_add (db, track, -1);
	itdb_playlist_add_track (itdb_playlist_mpl(db), track, -1);
}

typedef void (*DirTraversalFunc)(const char *filename, gpointer data);

static void
foreach_file (const char *basedir,
	      DirTraversalFunc func, gpointer data,
	      GError **error)
{
	GError *tmp_error;
	const char *name;
	GDir *dir;

	g_assert (func != NULL);

	tmp_error = NULL;
	dir = g_dir_open (basedir, 0, &tmp_error);
 	if (tmp_error != NULL) {
		g_propagate_error (error, tmp_error);
		return;
	}

	name = g_dir_read_name (dir);
	while (name != NULL) {
		char *absolute_path;

		absolute_path = g_build_filename (basedir, name, NULL);

		if (g_file_test (absolute_path, G_FILE_TEST_IS_DIR)) {
			tmp_error = NULL;
			foreach_file (absolute_path, func, data, &tmp_error);
			if (tmp_error != NULL) {
				g_propagate_error (error, tmp_error);
				g_free (absolute_path);
				g_dir_close (dir);
				return;
			}
		} else if (g_file_test (absolute_path, G_FILE_TEST_IS_REGULAR)){
			func (absolute_path, data);
		}
		g_free (absolute_path);
		name = g_dir_read_name (dir);
	}

	g_dir_close (dir);


}

static void
fill_db (Itdb_iTunesDB *db, GError **error)
{
	GError *err = NULL;
	char *music_dir;

	music_dir = itdb_get_music_dir (itdb_get_mountpoint (db));
	foreach_file (music_dir, process_one_file, db, &err);
	g_free (music_dir);
	if (err != NULL) {
		g_propagate_error (error, err);
		return;
	}
	g_print ("Found %d files\n",
		 g_list_length (itdb_playlist_mpl(db)->members));
}

static Itdb_iTunesDB *
itdb_create (const char *mountpoint)
{
	Itdb_Playlist *mpl;
	Itdb_iTunesDB *db;

	db = itdb_new ();
	if (db == NULL) {
		return NULL;
	}
	itdb_set_mountpoint (db, mountpoint);
	mpl = itdb_playlist_new ("iPod", FALSE);
	itdb_playlist_set_mpl (mpl);
	itdb_playlist_add (db, mpl, -1);

	return db;
}

int main (int argc, char **argv)
{
	Itdb_iTunesDB *db;
	GError *error;

	if (argc != 2) {
		g_print ("Usage:\n");
		g_print ("%s <mountpoint>\n", g_basename (argv[0]));
		exit (1);
	}

	db = itdb_create (argv[1]);

	if (db == NULL) {
		g_print ("Error creating iPod database\n");
		exit (1);
	}

	error = NULL;
	fill_db (db, &error);
	if (error) {
		g_print ("Error reading music files\n");
		if (error->message) {
			g_print("%s\n", error->message);
		}
		g_error_free (error);
		exit (1);
	}

	itdb_write (db, &error);
	if (error) {
		g_print ("Error writing iPod database\n");
		if (error->message) {
			g_print("%s\n", error->message);
		}
		g_error_free (error);
		exit (1);
	}

	itdb_free (db);

	return 0;
}

/*
 * Copyright (C) 2006, 2009 Christophe Fergeau
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


static gboolean
copy_file (Itdb_iTunesDB *db, const char *filename, GError **error)
{
	Itdb_Track *track;
	gboolean copy_success;

	track = track_from_file (filename);
	itdb_track_add (db, track, -1);
	itdb_playlist_add_track (itdb_playlist_mpl(db), track, -1);
	copy_success = itdb_cp_track_to_ipod (track, filename, error);
	if (!copy_success) {
		itdb_track_free (track);
	}
	return copy_success;
}

int main (int argc, char **argv)
{
	Itdb_iTunesDB *db;
	GError *error;

	if (argc != 3) {
		g_print ("Usage:\n");
		g_print ("%s <filename> <mountpoint> \n", g_basename (argv[0]));
		exit (1);
	}

	db = itdb_parse (argv[2], NULL);
	if (db == NULL) {
		return NULL;
	}

	if (db == NULL) {
		g_print ("Error creating iPod database\n");
		exit (1);
	}

	error = NULL;
	if (!copy_file (db, argv[1], &error)) {
		g_print ("Error copying music files\n");
		if (error && error->message) {
			g_print("%s\n", error->message);
			g_error_free (error);
		}
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

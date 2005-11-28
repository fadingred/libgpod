/* Copyright (c) 2005, Christophe Fergeau <teuf@gnome.org>
 * All rights reserved.

 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions are met:
 *
 *    * Redistributions of source code must retain the above copyright notice, 
 *      this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright 
 *      notice, this list of conditions and the following disclaimer in the 
 *      documentation and/or other materials provided with the distribution.
 *    * Neither the name of the <ORGANIZATION> nor the names of its 
 *      contributors may be used to endorse or promote products derived from 
 *      this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE 
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "itdb.h"

#include <locale.h>
#include <string.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

static GList *
get_cover_list (const char *dirname)
{
	GDir *dir;
	const char *filename;
	GList *result;

	dir = g_dir_open (dirname, 0, NULL);
	if (dir == NULL) {
		return NULL;
	}
	result = NULL;
	filename = g_dir_read_name (dir);
	while (filename != NULL) {
		const char *ext;
		ext = strrchr (filename, '.');
		if (ext != NULL) {
			if ((g_ascii_strcasecmp (ext, ".png") == 0) 
			    || (g_ascii_strcasecmp (ext, ".jpg") == 0)
			    || (g_ascii_strcasecmp (ext, ".jpeg") == 0)) {
				char *fullpath;
				fullpath = g_build_filename (dirname, filename,
							     NULL);
				result = g_list_prepend (result, fullpath);
			}
		}
		filename = g_dir_read_name (dir);
	}
	g_dir_close (dir);
	
	return g_list_reverse (result);
}


int
main (int argc, char **argv)
{
	Itdb_iTunesDB *db;
	GList *it;
	GList *covers;
	int nb_covers;

	if (argc < 3) {
		g_print ("Usage: %s mountpoint image-dir\n", argv[0]);
		return 1;
	}
	
	setlocale (LC_ALL, "");
	g_type_init ();
	covers = get_cover_list (argv[2]);
	if (covers == NULL) {
		g_print ("Error, %s should be a directory containing pictures\n", argv[2]);
		return 1;
	}
	nb_covers = g_list_length (covers);
	db = itdb_parse (argv[1], NULL);
	if (db == NULL) {
		g_print ("Error reading iPod database\n");
		return 1;
	}
	for (it = db->tracks; it != NULL; it = it->next) {
		Itdb_Track *song;
		const char *coverpath;

		song = (Itdb_Track*)it->data;
		itdb_artwork_remove_thumbnails (song->artwork);

		coverpath = g_list_nth_data (covers, 
					     g_random_int_range (0, nb_covers));
		itdb_track_set_thumbnails (song, coverpath);
/*		g_print ("%s - %s - %s gets %s\n",  
		song->artist, song->album, song->title, coverpath);*/

	}
/*	if (db->tracks != NULL) {
		Itdb_Track *song;
		const char *coverpath;
		
		song = (Itdb_Track *)db->tracks->data;
		coverpath = g_list_nth_data (covers, 
					     g_random_int_range (0, nb_covers));
		itdb_track_remove_thumbnail (song);
		itdb_track_set_thumbnail (song, coverpath);
		}*/

	itdb_write (db, NULL);
	itdb_free (db);
	g_list_foreach (covers, (GFunc)g_free, NULL);
	g_list_free (covers);

	return 0;
}

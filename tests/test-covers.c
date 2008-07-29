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
#include <gdk-pixbuf/gdk-pixbuf.h>
#include "itdb_thumb.h"

static void
save_itdb_thumb (Itdb_Track *track, GdkPixbuf *pixbuf, guint id)
{
        char *filename;
        gint width;
        gint height;

        g_object_get (G_OBJECT (pixbuf),
                      "width", &width,
                      "height", &height,
                      NULL);
        filename = g_strdup_printf ("%03d_%s-%s-%s-%xx%x-%016"G_GINT64_MODIFIER"x.png",
                                    id,
                                    track->artist, track->album, 
                                    track->title, width, height,
                                    track->dbid);
        if (filename == NULL) {
                return;
        }
        g_print ("  %s\n", filename);
        gdk_pixbuf_save (pixbuf, filename, "png", NULL, NULL);
        gdk_pixbuf_unref (pixbuf);
        /*		g_print ("Saved %s\n", filename); */
        g_free (filename);
}

static void
save_song_thumbnails (Itdb_Track *song)
{
        static guint count = 0;
        GList *it;
        GList *thumbs;
        if (song->artwork->thumbnail == NULL) {
            return;
        }
        thumbs = itdb_thumb_to_pixbufs (song->itdb->device,
                                        song->artwork->thumbnail);
	for (it = thumbs; it != NULL; it = it->next) {
                GdkPixbuf *pixbuf;

		pixbuf = GDK_PIXBUF (it->data);
		g_return_if_fail (pixbuf);

                save_itdb_thumb (song, pixbuf, count);
                count++;
	}
        g_list_free (thumbs);
}


static void
save_thumbnails (Itdb_iTunesDB *db)
{
	GList *it;
	gint count = 0;

	for (it = db->tracks; it != NULL; it = it->next) {
		Itdb_Track *song;
		
		song = (Itdb_Track *)it->data;
		g_print ("Track %d (%016"G_GINT64_MODIFIER"x) %s-%s-%s (%08x)\n",
			 count++, song->dbid,
			 song->artist, song->album,
			 song->title, song->mhii_link);
		save_song_thumbnails (song);
	}
}

int
main (int argc, char **argv)
{
	Itdb_iTunesDB *db;


	if (argc < 2) {
		g_print ("Usage: %s mountpoint\n", argv[0]);
		return 1;
	}
	
	setlocale (LC_ALL, "");
	g_type_init ();
	db = itdb_parse (argv[1], NULL);
	if (db == NULL) {
		g_print ("Error reading iPod database\n");
		return 1;
	}

	g_print ("========== ArtworkDB ==========\n");
	save_thumbnails (db);
	itdb_free (db);

	return 0;
}

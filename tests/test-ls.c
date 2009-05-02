/*
|   Copyright (C) 2002-2003 Jorg Schuler <jcsjcs at users.sourceforge.net>
|   Copyright (C) 2006 Christophe Fergeau  <teuf@gnome.org>
|
|   This program is free software; you can redistribute it and/or modify
|   it under the terms of the GNU General Public License as published by
|   the Free Software Foundation; either version 2 of the License, or
|   (at your option) any later version.
|
|   This program is distributed in the hope that it will be useful,
|   but WITHOUT ANY WARRANTY; without even the implied warranty of
|   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
|   GNU General Public License for more details.
|
|   You should have received a copy of the GNU General Public License
|   along with this program; if not, write to the Free Software
|   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
|
|  iTunes and iPod are trademarks of Apple
|
|  This product is not supported/written/published by Apple!
|
*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <libintl.h>

#include "itdb.h"

#define LOCALDB "/.gtkpod/local_0.itdb"

static void
display_recently_played (Itdb_iTunesDB *db)
{
    Itdb_Playlist *mpl = itdb_playlist_mpl (db);
    GList *it;

    g_print ("Recently played:\n");
    for (it = mpl->members; it != NULL; it = it->next) {
        Itdb_Track *track = (Itdb_Track*)it->data;

        if (track->recent_playcount != 0) {
            char date[30];
            time_t track_time = track->time_played;

            g_print ("%s - %s - %s:\n",
                     track->artist, track->album, track->title);
            strftime (date, sizeof (date), "%F %H:%M:%S",
                      gmtime (&track_time));
            g_print ("\tUTC: %s\n", date);
            strftime (date, sizeof (date), "%F %H:%M:%S",
                      localtime (&track_time));
            g_print ("\tlocal: %s\n", date);
            g_print ("track: %ld ", track_time);
            time (&track_time);
            g_print ("current: %ld\n", track_time);
            g_print ("\n");
        }
    }
}

static void
display_track (Itdb_Track *track, const char *prefix) 
{
    g_print ("%s%s - %s - %s\n", prefix,  
	     track->artist ? track->artist : "(null)",
	     track->album ? track->album : "(null)",
	     track->title ? track->title : "(null)");
    g_print ("%s\t%s\n", prefix, track->ipod_path);
}

static void
display_playlist (Itdb_Playlist *playlist, const char *prefix)
{
    char *track_prefix;
    GList *it;

    if (itdb_playlist_is_mpl (playlist)) {
	g_print ("%s%s (Master Playlist)\n", prefix, playlist->name);
    } else if (itdb_playlist_is_podcasts (playlist)) {
	g_print ("%s%s (Podcasts Playlist)\n", prefix, playlist->name);
    } else {
	g_print ("%s%s\n", prefix, playlist->name);
    }

    printf ("%stracks: %d\n", prefix, g_list_length (playlist->members));
    track_prefix = g_strdup_printf ("%s\t", prefix);
    for (it = playlist->members; it != NULL; it = it->next) {
	Itdb_Track *track;
	
	track = (Itdb_Track *)it->data;
	display_track (track, "\t");
    }
    g_print ("\n\n");
    g_free (track_prefix);
}

int
main (int argc, char *argv[])
{
  GError *error=NULL;
  const gchar *homeenv="HOME";
  Itdb_iTunesDB *itdb;
  gchar *mountpoint = NULL, *playlistname = NULL;

  if (argc >= 2)
      mountpoint = argv[1];

    if (argc >= 3)
      playlistname = argv[2];
 
  if (mountpoint == NULL)
  {
      g_print ("Usage: %s <mountpoint>|-l [<playlistname>]\n\n"
               "-l - List from the local repository (~" LOCALDB ")\n"
	       "<playlistname> - name of the playlist to list (optional)\n",  
                g_basename(argv[0]));
      exit (0);
  }

  if (strcmp(mountpoint, "-l") == 0) {
      mountpoint = g_build_filename(g_getenv(homeenv), LOCALDB, NULL);
      itdb = itdb_parse_file (mountpoint, &error);
  }
  else
      itdb = itdb_parse (mountpoint, &error);
  
  if (error)
  {
      if (error->message) {
	  g_print("%s\n", error->message);
      }
      g_error_free (error);
      error = NULL;
  }

  if (itdb)
  {
      GList *it;

      printf ("playlists: %d\n", g_list_length (itdb->playlists));
      for (it = itdb->playlists; it != NULL; it = it->next) {
	  Itdb_Playlist *playlist;

	  playlist = (Itdb_Playlist *)it->data;
	  
	  if (playlistname == NULL || strcmp(playlist->name, playlistname) == 0) 
	    display_playlist (playlist, "");
      }

      display_recently_played (itdb);
  }

  itdb_free (itdb);

  return 0;
}

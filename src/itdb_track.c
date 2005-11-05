/* Time-stamp: <2005-10-12 01:04:36 jcs>
|
|  Copyright (C) 2002-2005 Jorg Schuler <jcsjcs at users sourceforge net>
|  Part of the gtkpod project.
| 
|  URL: http://www.gtkpod.org/
|  URL: http://gtkpod.sourceforge.net/
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
|  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
|
|  iTunes and iPod are trademarks of Apple
|
|  This product is not supported/written/published by Apple!
|
|  $Id$
*/

#include <config.h>

#include "itdb_private.h"
#include <string.h>
#include <glib/gstdio.h>

static gboolean is_video_ipod (IpodDevice *ipod) 
{
	guint model;

	if (ipod == NULL) {
		return FALSE;
	}
	g_object_get (G_OBJECT (ipod), "device-model", &model, NULL);
	return ((model == MODEL_TYPE_VIDEO_WHITE) 
		|| (model == MODEL_TYPE_VIDEO_BLACK));
}


/* Generate a new Itdb_Track structure */
Itdb_Track *itdb_track_new (void)
{
    Itdb_Track *track = g_new0 (Itdb_Track, 1);

    track->visible = 1;
    return track;
}

/* Attempt to set some of the unknowns to reasonable defaults */
static void itdb_track_set_defaults (Itdb_Track *tr)
{
    auto gboolean haystack (gchar *filetype, gchar **desclist);
    gboolean haystack (gchar *filetype, gchar **desclist)
    {
	gchar **dlp;
	if (!filetype || !desclist) return FALSE;
	for (dlp=desclist; *dlp; ++dlp)
	{
	    if (strstr (filetype, *dlp)) return TRUE;
	}
	return FALSE;
    }

    gchar *mp3_desc[] = {"MPEG", "MP3", "mpeg", "mp3", NULL};
    gchar *mp4_desc[] = {"AAC", "MP4", "aac", "mp4", NULL};
    gchar *audible_subdesc[] = {"Audible", "audible", "Book", "book", NULL};
    gchar *wav_desc[] = {"WAV", "wav", NULL};
    gchar *m4v_desc[] = {"M4V", "MP4", "MP4V", "m4v", "mp4", "mp4v", NULL};

    g_return_if_fail (tr);
    g_return_if_fail (tr->itdb);

    /* The exact meaning of unk126 is unknown, but always seems to be
       0xffff for MP3/AAC songs, 0x0 for uncompressed songs (like WAVE
       format), 0x1 for Audible. */
    if (tr->unk126 == 0)
    {
	if (haystack (tr->filetype, mp3_desc))
	{
	    tr->unk126 = 0xffff;
	}
	else if (haystack (tr->filetype, mp4_desc))
	{
	    if (haystack (tr->filetype, audible_subdesc))
	    {
		tr->unk126 = 0x01;
	    }
		else
	    {
		tr->unk126 = 0xffff;
	    }
	}
	else if (haystack (tr->filetype, wav_desc))
	{
	    tr->unk126 = 0x00;
	}
	else
	{
	    tr->unk126 = 0x00;  /* default value */
	}
    }
    /* The exact meaning of unk144 is unknown, but MP3 songs appear to
       be always 0x0000000c or 0x0100000c (if played one or more times
       in iTunes), AAC songs are always 0x01000033, Audible files are
       0x01000029, WAV files are 0x0. */
    if (tr->unk144 == 0)
    {
	if (haystack (tr->filetype, mp3_desc))
	{
	    tr->unk144 = 0x0000000c;
	}
	else if (haystack (tr->filetype, mp4_desc))
	{
	    if (haystack (tr->filetype, audible_subdesc))
	    {
		tr->unk144 = 0x01000029;
	    }
		else
	    {
		tr->unk144 = 0x01000033;
	    }
	}
	else if (haystack (tr->filetype, wav_desc))
	{
	    tr->unk144 = 0x00;
	}
	else
	{
	    tr->unk144 = 0x00;  /* default value */
	}
    }
    if (is_video_ipod (tr->itdb->device)) {
	/* The unk208 field seems to denote whether the file is a video or not.
	   It seems that it must be set to 0x00000002 for video files. */
	/* Only doing that for iPod videos since it remains to be proven that
	 * setting unk208 to non-0 doesn't upset older ipod models
	 */
	if (haystack (tr->filetype, m4v_desc))
	{
	    /* set type to video (0x00000002) */
	    tr->unk208 = 0x00000002;
	}
	else
	{
	    /* set type to audio */
	    tr->unk208 = 0x00000001;
	}
    }
    /* The sample rate of the song expressed as an IEEE 32 bit
       floating point number.  It's uncertain why this is here.  itdb
       will set this when adding a track */
    tr->samplerate2 = tr->samplerate;

    /* set unique ID when not yet set */
    if (tr->dbid == 0)
    {
	GList *gl;
	guint64 id;
	do
	{
	    id = ((guint64)g_random_int () << 32) |
		((guint64)g_random_int ());
	    /* check if id is really unique */
	    for (gl=tr->itdb->tracks; id && gl; gl=gl->next)
	    {
		Itdb_Track *g_tr = gl->data;
		g_return_if_fail (g_tr);
		if (id == g_tr->dbid)  id = 0;
	    }
	} while (id == 0);
	tr->dbid = id;
    }
    if (tr->dbid2 == 0)  tr->dbid2 = tr->dbid;
}
    


/* Add @track to @itdb->tracks at position @pos (or at the end if pos
   is -1). Application is responsible to also add it to the master
   playlist. */
void itdb_track_add (Itdb_iTunesDB *itdb, Itdb_Track *track, gint32 pos)
{
    g_return_if_fail (itdb);
    g_return_if_fail (track);
    g_return_if_fail (!track->userdata || track->userdata_duplicate);

    track->itdb = itdb;

    itdb_track_set_defaults (track);

    if (pos == -1)  itdb->tracks = g_list_append (itdb->tracks, track);
    else  itdb->tracks = g_list_insert (itdb->tracks, track, pos);
}

void
itdb_track_free_generated_thumbnails (Itdb_Track *track)
{
    GList *it;

    for (it = track->thumbnails; it != NULL; it = it->next) {
	Itdb_Image *image;

	image = (Itdb_Image *)it->data;
	g_free (image->filename);
	g_free (image);
    }
    g_list_free (track->thumbnails);
    track->thumbnails = NULL;
}

/* Free the memory taken by @track */
void itdb_track_free (Itdb_Track *track)
{
    g_return_if_fail (track);

    g_free (track->title);
    g_free (track->artist);
    g_free (track->album);
    g_free (track->genre);
    g_free (track->composer);
    g_free (track->comment);
    g_free (track->filetype);
    g_free (track->grouping);
    g_free (track->category);
    g_free (track->description);
    g_free (track->podcasturl);
    g_free (track->podcastrss);
    g_free (track->subtitle);
    g_free (track->ipod_path);
    g_free (track->chapterdata_raw);
    itdb_track_free_generated_thumbnails (track);
    g_free (track->orig_image_filename);
    if (track->userdata && track->userdata_destroy)
	(*track->userdata_destroy) (track->userdata);
    g_free (track);
}

/* Remove track @track and free memory */
void itdb_track_remove (Itdb_Track *track)
{
    Itdb_iTunesDB *itdb;

    g_return_if_fail (track);
    itdb = track->itdb;
    g_return_if_fail (itdb);

    itdb->tracks = g_list_remove (itdb->tracks, track);
    itdb_track_free (track);
}

/* Remove track @track but do not free memory */
/* track->itdb is set to NULL */
void itdb_track_unlink (Itdb_Track *track)
{
    Itdb_iTunesDB *itdb;

    g_return_if_fail (track);
    itdb = track->itdb;
    g_return_if_fail (itdb);

    itdb->tracks = g_list_remove (itdb->tracks, track);
    track->itdb = NULL;
}

static GList *dup_thumbnails (GList *thumbnails)
{
    GList *it;
    GList *result;
    
    result = NULL;
    for (it = thumbnails; it != NULL; it = it->next)
    {
	Itdb_Image *new_image;
	Itdb_Image *image;

	image = (Itdb_Image *)it->data;
	g_return_val_if_fail (image, NULL);

	new_image = g_new (Itdb_Image, 1);
	memcpy (new_image, image, sizeof (Itdb_Image));
	new_image->filename = g_strdup (image->filename);
	
	result = g_list_prepend (result, new_image);
    }

    return g_list_reverse (result);
}

/* Duplicate an existing track */
Itdb_Track *itdb_track_duplicate (Itdb_Track *tr)
{
    Itdb_Track *tr_dup;

    g_return_val_if_fail (tr, NULL);

    tr_dup = g_new (Itdb_Track, 1);
    memcpy (tr_dup, tr, sizeof (Itdb_Track));

    /* clear itdb pointer */
    tr_dup->itdb = NULL;

    /* copy strings */
    tr_dup->title = g_strdup (tr->title);
    tr_dup->artist = g_strdup (tr->artist);
    tr_dup->album = g_strdup (tr->album);
    tr_dup->genre = g_strdup (tr->genre);
    tr_dup->composer = g_strdup (tr->composer);
    tr_dup->comment = g_strdup (tr->comment);
    tr_dup->filetype = g_strdup (tr->filetype);
    tr_dup->grouping = g_strdup (tr->grouping);
    tr_dup->category = g_strdup (tr->category);
    tr_dup->description = g_strdup (tr->description);
    tr_dup->podcasturl = g_strdup (tr->podcasturl);
    tr_dup->podcastrss = g_strdup (tr->podcastrss);
    tr_dup->subtitle = g_strdup (tr->subtitle);
    tr_dup->ipod_path = g_strdup (tr->ipod_path);

    /* Copy chapterdata */
    if (tr->chapterdata_raw)
    {
	tr_dup->chapterdata_raw = g_new (gchar, tr->chapterdata_raw_length);
	memcpy (tr_dup->chapterdata_raw, tr->chapterdata_raw,
		tr->chapterdata_raw_length);
    }

    /* Copy thumbnail data */
    tr_dup->orig_image_filename = g_strdup (tr->orig_image_filename);
    tr_dup->thumbnails = dup_thumbnails (tr->thumbnails);

    /* Copy userdata */
    if (tr->userdata && tr->userdata_duplicate)
	tr_dup->userdata = tr->userdata_duplicate (tr->userdata);

    return tr_dup;
}



/* Returns the track with the ID @id or NULL if the ID cannot be
 * found. */
/* Looking up tracks by ID is not really a good idea because the IDs
   are created by itdb just before export. The functions are here
   because they are needed during import of the iTunesDB which is
   referencing tracks by IDs */
/* This function is very slow -- if you need to lookup many IDs use
   the functions itdb_track_id_tree_create(),
   itdb_track_id_tree_destroy(), and itdb_track_id_tree_by_id() below. */
Itdb_Track *itdb_track_by_id (Itdb_iTunesDB *itdb, guint32 id)
{
    GList *gl;

    g_return_val_if_fail (itdb, NULL);

    for (gl=itdb->tracks; gl; gl=gl->next)
    {
	Itdb_Track *track = gl->data;
	if (track->id == id)  return track;
    }
    return NULL;
}

static gint track_id_compare (gconstpointer a, gconstpointer b)
{
    if (*(guint32*) a == *(guint32*) b)
	return 0;
    if (*(guint32*) a > *(guint32*) b)
	return 1;
    return -1;
}


/* Creates a balanced-binary tree for quick ID lookup that is used in
   itdb_track_by_id_tree() function below */
GTree *itdb_track_id_tree_create (Itdb_iTunesDB *itdb)
{
    GTree *idtree;
    GList *gl;

    g_return_val_if_fail (itdb, NULL);

    idtree = g_tree_new (track_id_compare);

    for (gl=itdb->tracks; gl; gl=gl->next)
    {
	Itdb_Track *tr = gl->data;
	g_return_val_if_fail (tr, NULL);
	g_tree_insert (idtree, &tr->id, tr);
    }
    return idtree;
}

/* free memory of @idtree */
void itdb_track_id_tree_destroy (GTree *idtree)
{
    g_return_if_fail (idtree);

    g_tree_destroy (idtree);
}

/* lookup track by @id using @idtree for quicker reference */
Itdb_Track *itdb_track_id_tree_by_id (GTree *idtree, guint32 id)
{
    g_return_val_if_fail (idtree, NULL);

    return (Itdb_Track *)g_tree_lookup (idtree, &id);
}

void
itdb_track_remove_thumbnail (Itdb_Track *song)
{
    itdb_track_free_generated_thumbnails (song);
    g_free (song->orig_image_filename);
    song->orig_image_filename = NULL;
    song->image_id = 0;
}


#ifdef HAVE_GDKPIXBUF
/* This operation doesn't make sense when we can't save thumbnail files */
int 
itdb_track_set_thumbnail (Itdb_Track *song, const char *filename)
{
    struct stat statbuf;

    g_return_val_if_fail (song != NULL, -1);

    if (g_stat  (filename, &statbuf) != 0) {
	return -1;
    }
    itdb_track_remove_thumbnail (song);
    song->artwork_size = statbuf.st_size;
    song->orig_image_filename = g_strdup (filename);

    return 0;
}
#else
int 
itdb_track_set_thumbnail (Itdb_Track *song, const char *filename)
{
    return -1;
}
#endif

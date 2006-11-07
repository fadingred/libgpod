/* Time-stamp: <2006-11-07 20:54:45 jcs>
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
#include "itdb_device.h"
#include <string.h>

static gboolean is_video_ipod (Itdb_Device *device) 
{
    const Itdb_IpodInfo *info;

    g_return_val_if_fail (device, FALSE);

    info = itdb_device_get_ipod_info (device);

    if (!info) return FALSE;

    if ((info->ipod_model == ITDB_IPOD_MODEL_VIDEO_WHITE) ||
	(info->ipod_model == ITDB_IPOD_MODEL_VIDEO_BLACK))
	return TRUE;
    else
	return FALSE;
}


/**
 * itdb_track_new:
 * 
 * Creates an empty #Itdb_Track 
 *
 * Return Value: the new #Itdb_Track, free it with itdb_track_free() when no
 * longer needed
 **/
Itdb_Track *itdb_track_new (void)
{
    Itdb_Track *track = g_new0 (Itdb_Track, 1);

    track->artwork = itdb_artwork_new ();

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

    if (tr->mark_unplayed == 0)
    {
	/* don't have the iPod mark this track with a bullet as
	   unplayed. Should be set to 0x02 for podcasts that have not
	   yet been played. */
	tr->mark_unplayed = 0x01;
    }

    /* The exact meaning of unk126 is unknown, but always seems to be
       0xffff for MP3/AAC tracks, 0x0 for uncompressed tracks (like WAVE
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
    /* The exact meaning of unk144 is unknown, but MP3 tracks appear to
       be always 0x0000000c or 0x0100000c (if played one or more times
       in iTunes), AAC tracks are always 0x01000033, Audible files are
       0x01000029, WAV files are 0x0. */
    if (tr->unk144 == 0)
    {
	if (haystack (tr->filetype, mp3_desc))
	{
	    tr->unk144 = 0x000c;
	}
	else if (haystack (tr->filetype, mp4_desc))
	{
	    if (haystack (tr->filetype, audible_subdesc))
	    {
		tr->unk144 = 0x0029;
	    }
		else
	    {
		tr->unk144 = 0x0033;
	    }
	}
	else if (haystack (tr->filetype, wav_desc))
	{
	    tr->unk144 = 0x0000;
	}
	else
	{
	    tr->unk144 = 0x0000;  /* default value */
	}
    }
    if (is_video_ipod (tr->itdb->device))
    {
	/* The unk208 field seems to denote whether the file is a
	   video or not.  It seems that it must be set to 0x00000002
	   for video files. */
	/* Only doing that for iPod videos since it remains to be
	 * proven that setting unk208 to non-0 doesn't upset older
	 * ipod models
	 */
	if (tr->mediatype == 0)
	{
	    if (haystack (tr->filetype, m4v_desc))
	    {
		/* set type to video (0x00000002) */
		tr->mediatype = 0x00000002;
	    }
	    else
	    {
		/* set type to audio */
		tr->mediatype = 0x00000001;
	    }
	}
    }
    /* The sample rate of the track expressed as an IEEE 32 bit
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
	tr->dbid2= id;
    }
    if (tr->dbid2 == 0)  tr->dbid2 = tr->dbid;
}
    


/**
 * itdb_track_add:
 * @itdb: an #Itdb_iTunesDB
 * @track: an #Itdb_Track
 * @pos: position of the track to add
 * 
 * Adds @track to @itdb->tracks at position @pos (or at the end if pos
 * is -1). The application is responsible to also add it to the master
 * playlist. The @itdb gets ownership of the @track and will take care of 
 * freeing the memory it uses when it's no longer necessary.
 **/
void itdb_track_add (Itdb_iTunesDB *itdb, Itdb_Track *track, gint32 pos)
{
    g_return_if_fail (itdb);
    g_return_if_fail (track);
    g_return_if_fail (!track->userdata || track->userdata_duplicate);

    track->itdb = itdb;

    itdb_track_set_defaults (track);

    itdb->tracks = g_list_insert (itdb->tracks, track, pos);
}

/**
 * itdb_track_free:
 * @track: an #Itdb_Track
 *
 * Frees the memory used by @track 
 **/
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
    itdb_artwork_remove_thumbnails (track->artwork);
    g_free (track->artwork);
    if (track->userdata && track->userdata_destroy)
	(*track->userdata_destroy) (track->userdata);
    g_free (track);
}

/**
 * itdb_track_remove:
 * @track: an #Itdb_Track
 *
 * Removes @track from the #Itdb_iTunesDB it's associated with, and frees the
 * memory it uses. It doesn't remove the track from the playlists it may have
 * been added to, in particular it won't be removed from the master playlist.
 **/
void itdb_track_remove (Itdb_Track *track)
{
    Itdb_iTunesDB *itdb;

    g_return_if_fail (track);
    itdb = track->itdb;
    g_return_if_fail (itdb);

    itdb->tracks = g_list_remove (itdb->tracks, track);
    itdb_track_free (track);
}

/**
 * itdb_track_unlink:
 * @track: an #Itdb_Track
 * 
 * Removes @track from the #Itdb_iTunesDB it's associated with, but do not free
 * memory. It doesn't remove the track from the playlists it may have been 
 * added to, in particular it won't be removed from the master playlist.
 * track->itdb is set to NULL.
 **/
void itdb_track_unlink (Itdb_Track *track)
{
    Itdb_iTunesDB *itdb;

    g_return_if_fail (track);
    itdb = track->itdb;
    g_return_if_fail (itdb);

    itdb->tracks = g_list_remove (itdb->tracks, track);
    track->itdb = NULL;
}

/**
 * itdb_track_duplicate:
 * @tr: an #Itdb_Track
 *
 * Duplicates an existing track
 *
 * Return value: a newly allocated #Itdb_Track 
 **/
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
    tr_dup->artwork = itdb_artwork_duplicate (tr->artwork);

    /* Copy userdata */
    if (tr->userdata && tr->userdata_duplicate)
	tr_dup->userdata = tr->userdata_duplicate (tr->userdata);

    return tr_dup;
}


/* called by itdb_track_set_thumbnails() and
   itdb_track_set_thumbnails_from_data() */
static gboolean itdb_track_set_thumbnails_internal (Itdb_Track *track,
						    const gchar *filename,
						    const guchar *image_data,
						    gsize image_data_len)
{					     
    gboolean result = FALSE;

    g_return_val_if_fail (track, FALSE);

    itdb_artwork_remove_thumbnails (track->artwork);

    if (filename)
    {
	result = itdb_artwork_add_thumbnail (track->artwork,
					     ITDB_THUMB_COVER_SMALL,
					     filename);
	if (result == TRUE)
	    result = itdb_artwork_add_thumbnail (track->artwork,
						 ITDB_THUMB_COVER_LARGE,
						 filename);
    }
    if (image_data)
    {
	result = itdb_artwork_add_thumbnail_from_data (track->artwork,
						       ITDB_THUMB_COVER_SMALL,
						       image_data,
						       image_data_len);
	if (result == TRUE)
	    result = itdb_artwork_add_thumbnail_from_data (track->artwork,
							   ITDB_THUMB_COVER_LARGE,
							   image_data,
							   image_data_len);
    }

    if (result == TRUE)
    {
	/* some black magic :-( */
	/* track->artwork_size should actually be the total size of
	   artwork packed into MP3 tags. We don't write mp3 tags... */
	track->artwork_size = track->artwork->artwork_size;
	/* track->artwork_count should actually be the number of
	   images packed into MP3 tags. */
	track->artwork_count = 1;
	/* for some reason artwork->artwork_size is always
	   track->artwork_size + track->artwork_count */
	track->artwork->artwork_size += track->artwork_count;
	/* indicate artwork is present */
	track->has_artwork = 0x01;
    }
    else
    {
	itdb_track_remove_thumbnails (track);
    }

    return result;
}


/**
 * itdb_track_set_thumbnails:
 * @track: an #Itdb_Track
 * @filename: image file to use as a thumbnail
 *
 * Uses the image contained in @filename to generate iPod thumbnails. The image
 * can be in any format supported by gdk-pixbuf. To save memory, the thumbnails
 * will only be generated when necessary, ie when itdb_save() or a similar 
 * function is called.
 *
 * Return value: TRUE if the thumbnail could be added, FALSE otherwise.
 **/
gboolean itdb_track_set_thumbnails (Itdb_Track *track,
				    const gchar *filename)
{
    g_return_val_if_fail (track, FALSE);
    g_return_val_if_fail (filename, FALSE);

    return itdb_track_set_thumbnails_internal (track, filename, NULL, 0);
}




/**
 * itdb_track_set_thumbnails_from_data:
 * @track: an #Itdb_Track
 * @image_data: data used to create the thumbnail (the raw contents of
 *              an image file)
 * @image_data_len: length of above data block
 *
 * Uses @image_data to generate iPod thumbnails. The image can be in
 * any format supported by gdk-pixbuf. To save memory, the thumbnails
 * will only be generated when necessary, ie when itdb_save() or a
 * similar function is called.
 *
 * Return value: TRUE if the thumbnail could be added, FALSE otherwise.
 **/
gboolean itdb_track_set_thumbnails_from_data (Itdb_Track *track,
					      const guchar *image_data,
					      gsize image_data_len)
{
    g_return_val_if_fail (track, FALSE);
    g_return_val_if_fail (image_data, FALSE);

    return itdb_track_set_thumbnails_internal (track, NULL,
					       image_data, image_data_len);
}


/**
 * itdb_track_remove_thumbnails:
 * @track: an #Itdb_Track
 * 
 * Removes the thumbnails associated with @track
 **/
void itdb_track_remove_thumbnails (Itdb_Track *track)
{
    g_return_if_fail (track);
    itdb_artwork_remove_thumbnails (track->artwork);
    track->artwork_size = 0;
    track->artwork_count = 0;
    /* indicate no artwork is present */
    track->has_artwork = 0x02;
}


/**
 * itdb_track_by_id:
 * @itdb: an #Itdb_iTunesDB
 * @id: ID of the track to look for
 *
 * Looks up a track using its ID in @itdb. 
 * Looking up tracks by ID is not really a good idea because the IDs
 * are created by itdb just before export. The functions are here
 * because they are needed during import of the iTunesDB which is
 * referencing tracks by IDs.
 * This function is very slow (linear in the number of tracks contained in the 
 * database). If you need to lookup many IDs use itdb_track_id_tree_create(), 
 * itdb_track_id_tree_destroy(), and itdb_track_id_tree_by_id().
 *
 * Return value: #Itdb_Track with the ID @id or NULL if the ID cannot be
 * found. 
 **/
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


/**
 * itdb_track_id_tree_create:
 * @itdb: an #Itdb_iTunesDB
 *
 * Creates a balanced-binary tree for quick ID lookup that is used in
 * itdb_track_by_id_tree() function below 
 *
 * Return value: a #GTree indexed by track IDs to be freed with 
 * itdb_track_id_tree_destroy() when no longer used
 **/
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

/**
 * itdb_track_id_tree_destroy:
 * @idtree: a #GTree
 *
 * Frees the memory used by @idtree 
 **/
void itdb_track_id_tree_destroy (GTree *idtree)
{
    g_return_if_fail (idtree);

    g_tree_destroy (idtree);
}

/**
 * itdb_track_id_tree_by_id:
 * @idtree: a #GTree created using itdb_track_id_tree_create()
 * @id: the ID of the track to search for
 *
 * Lookup an #Itdb_Track by @id using @idtree for faster lookup (compared to
 * itdb_track_by_id)
 *
 * Return value: the #Itdb_Track whose ID is @id, or NULL if such a track 
 * couldn't be found
 **/
Itdb_Track *itdb_track_id_tree_by_id (GTree *idtree, guint32 id)
{
    g_return_val_if_fail (idtree, NULL);

    return (Itdb_Track *)g_tree_lookup (idtree, &id);
}



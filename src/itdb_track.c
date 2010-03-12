/*
|  Copyright (C) 2002-2007 Jorg Schuler <jcsjcs at users sourceforge net>
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

/**
 * itdb_track_new:
 *
 * Creates an empty #Itdb_Track
 *
 * Returns: the new #Itdb_Track, free it with itdb_track_free() when no
 * longer needed
 */
Itdb_Track *itdb_track_new (void)
{
    Itdb_Track *track = g_new0 (Itdb_Track, 1);

    track->artwork = itdb_artwork_new ();
    track->chapterdata = itdb_chapterdata_new ();
    track->priv = g_new0 (Itdb_Track_Private, 1);

    track->visible = 1;
    return track;
}

static gboolean haystack (gchar *filetype, gchar **desclist)
{
    gchar **dlp;
    if (!filetype || !desclist) return FALSE;
    for (dlp=desclist; *dlp; ++dlp)
    {
	    if (strstr (filetype, *dlp)) return TRUE;
    }
    return FALSE;
}

/* Attempt to set some of the unknowns to reasonable defaults */
static void itdb_track_set_defaults (Itdb_Track *tr)
{
    gchar *mp3_desc[] = {"MPEG", "MP3", "mpeg", "mp3", NULL};
    gchar *mp4_desc[] = {"AAC", "MP4", "aac", "mp4", NULL};
    gchar *audible_subdesc[] = {"Audible", "audible", "Book", "book", NULL};
    gchar *wav_desc[] = {"WAV", "wav", NULL};
    gchar *m4v_desc[] = {"M4V", "MP4", "MP4V", "m4v", "mp4", "mp4v", NULL};
    gchar *mov_desc[] = {"MOV", "mov", NULL};

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
    if (itdb_device_supports_video (tr->itdb->device))
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
	    if (haystack (tr->filetype, m4v_desc)
		 || haystack (tr->filetype, mov_desc))
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
 * @itdb:   an #Itdb_iTunesDB
 * @track:  an #Itdb_Track
 * @pos:    position of the track to add
 *
 * Adds @track to @itdb->tracks at position @pos (or at the end if pos
 * is -1). The application is responsible to also add it to the master
 * playlist. The @itdb gets ownership of the @track and will take care of
 * freeing the memory it uses when it's no longer necessary.
 */
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
 */
void itdb_track_free (Itdb_Track *track)
{
    g_return_if_fail (track);

    g_free (track->title);
    g_free (track->ipod_path);
    g_free (track->album);
    g_free (track->artist);
    g_free (track->genre);
    g_free (track->filetype);
    g_free (track->comment);
    g_free (track->category);
    g_free (track->composer);
    g_free (track->grouping);
    g_free (track->description);
    g_free (track->podcasturl);
    g_free (track->podcastrss);
    g_free (track->subtitle);
    g_free (track->tvshow);
    g_free (track->tvepisode);
    g_free (track->tvnetwork);
    g_free (track->albumartist);
    g_free (track->keywords);
    g_free (track->sort_artist);
    g_free (track->sort_title);
    g_free (track->sort_album);
    g_free (track->sort_albumartist);
    g_free (track->sort_composer);
    g_free (track->sort_tvshow);

    itdb_chapterdata_free (track->chapterdata);

    itdb_artwork_free (track->artwork);

    if (track->userdata && track->userdata_destroy)
	(*track->userdata_destroy) (track->userdata);

    g_free (track->priv);
    g_free (track);
}

/**
 * itdb_track_remove:
 * @track: an #Itdb_Track
 *
 * Removes @track from the #Itdb_iTunesDB it's associated with, and frees the
 * memory it uses. It doesn't remove the track from the playlists it may have
 * been added to, in particular it won't be removed from the master playlist.
 */
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
 * @track->itdb is set to NULL.
 */
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
 * Returns: a newly allocated #Itdb_Track
 */
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
    tr_dup->album = g_strdup (tr->album);
    tr_dup->artist = g_strdup (tr->artist);
    tr_dup->genre = g_strdup (tr->genre);
    tr_dup->filetype = g_strdup (tr->filetype);
    tr_dup->comment = g_strdup (tr->comment);
    tr_dup->category = g_strdup (tr->category);
    tr_dup->composer = g_strdup (tr->composer);
    tr_dup->grouping = g_strdup (tr->grouping);
    tr_dup->description = g_strdup (tr->description);
    tr_dup->podcasturl = g_strdup (tr->podcasturl);
    tr_dup->podcastrss = g_strdup (tr->podcastrss);
    tr_dup->subtitle = g_strdup (tr->subtitle);
    tr_dup->tvshow = g_strdup (tr->tvshow);
    tr_dup->tvepisode = g_strdup (tr->tvepisode);
    tr_dup->tvnetwork = g_strdup (tr->tvnetwork);
    tr_dup->albumartist = g_strdup (tr->albumartist);
    tr_dup->keywords = g_strdup (tr->keywords);
    tr_dup->ipod_path = g_strdup (tr->ipod_path);
    tr_dup->sort_artist = g_strdup (tr->sort_artist);
    tr_dup->sort_title = g_strdup (tr->sort_title);
    tr_dup->sort_album = g_strdup (tr->sort_album);
    tr_dup->sort_albumartist = g_strdup (tr->sort_albumartist);
    tr_dup->sort_composer = g_strdup (tr->sort_composer);
    tr_dup->sort_tvshow = g_strdup (tr->sort_tvshow);

    /* Copy private data too */
    tr_dup->priv = g_memdup (tr->priv, sizeof (Itdb_Track_Private));

    /* Copy chapterdata */
    tr_dup->chapterdata = itdb_chapterdata_duplicate (tr->chapterdata);

    /* Copy thumbnail data */
    if (tr->artwork != NULL) {
        tr_dup->artwork = itdb_artwork_duplicate (tr->artwork);
    }

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
						    gsize image_data_len,
                                                    gpointer *pixbuf,
						    gint rotation,
						    GError **error)
{
    gboolean result = FALSE;

    g_return_val_if_fail (track, FALSE);
    g_return_val_if_fail (filename || image_data || pixbuf, FALSE);

    itdb_artwork_remove_thumbnails (track->artwork);
    /* clear artwork id */
    track->artwork->id = 0;

    if (filename)
    {
        result = itdb_artwork_set_thumbnail (track->artwork, filename,
                                             rotation, error);
    }
    if (image_data)
    {
        result = itdb_artwork_set_thumbnail_from_data (track->artwork,
                                                       image_data,
                                                       image_data_len,
                                                       rotation, error);
    }
    if (pixbuf)
    {
        result = itdb_artwork_set_thumbnail_from_pixbuf (track->artwork,
                                                         pixbuf, rotation,
                                                         error);
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
 * @track:    an #Itdb_Track
 * @filename: image file to use as a thumbnail
 *
 * Uses the image contained in @filename to generate iPod thumbnails. The image
 * can be in any format supported by gdk-pixbuf. To save memory, the thumbnails
 * will only be generated when necessary, i.e. when itdb_save() or a similar
 * function is called.
 *
 * Returns: TRUE if the thumbnail could be added, FALSE otherwise.
 *
 * Since: 0.3.0
 */
gboolean itdb_track_set_thumbnails (Itdb_Track *track,
				    const gchar *filename)
{
    g_return_val_if_fail (track, FALSE);
    g_return_val_if_fail (filename, FALSE);

    return itdb_track_set_thumbnails_internal (track, filename, NULL, 0, NULL,
					       0, NULL);
}

/**
 * itdb_track_set_thumbnails_from_data:
 * @track:          an #Itdb_Track
 * @image_data:     data used to create the thumbnail (the raw contents of
 *                  an image file)
 * @image_data_len: length of above data block
 *
 * Uses @image_data to generate iPod thumbnails. The image can be in
 * any format supported by gdk-pixbuf. To save memory, the thumbnails
 * will only be generated when necessary, i.e. when itdb_save() or a
 * similar function is called.
 *
 * Returns: TRUE if the thumbnail could be added, FALSE otherwise.
 *
 * Since: 0.4.0
 */
gboolean itdb_track_set_thumbnails_from_data (Itdb_Track *track,
					      const guchar *image_data,
					      gsize image_data_len)
{
    g_return_val_if_fail (track, FALSE);
    g_return_val_if_fail (image_data, FALSE);

    return itdb_track_set_thumbnails_internal (track, NULL,
					       image_data, image_data_len,
                                               NULL, 0, NULL);
}

/**
 * itdb_track_set_thumbnails_from_pixbuf:
 * @track:  an #Itdb_Track
 * @pixbuf: a #GdkPixbuf used to generate the thumbnail
 *
 * Uses @pixbuf to generate iPod thumbnails. To save memory, the thumbnails
 * will only be generated when necessary, i.e. when itdb_save() or a
 * similar function is called.
 *
 * Returns: TRUE if the thumbnail could be added, FALSE otherwise.
 *
 * Since: 0.5.0
 */
gboolean itdb_track_set_thumbnails_from_pixbuf (Itdb_Track *track,
                                                gpointer pixbuf)
{
    g_return_val_if_fail (track, FALSE);
    g_return_val_if_fail (pixbuf, FALSE);

    return itdb_track_set_thumbnails_internal (track, NULL, NULL, 0,
                                               pixbuf, 0, NULL);
}

/**
 * itdb_track_remove_thumbnails:
 * @track: an #Itdb_Track
 *
 * Removes the thumbnails associated with @track
 *
 * Since: 0.3.0
 */
void itdb_track_remove_thumbnails (Itdb_Track *track)
{
    g_return_if_fail (track);
    itdb_artwork_remove_thumbnails (track->artwork);
    track->artwork_size = 0;
    track->artwork_count = 0;
    track->mhii_link = 0;
    /* indicate no artwork is present */
    track->has_artwork = 0x02;
}

/**
 * itdb_track_by_id:
 * @itdb: an #Itdb_iTunesDB
 * @id:   ID of the track to look for
 *
 * Looks up a track using its ID in @itdb.
 *
 * Looking up tracks by ID is not really a good idea because the IDs
 * are created by itdb just before export. The functions are here
 * because they are needed during import of the iTunesDB which is
 * referencing tracks by IDs.
 *
 * This function is very slow (linear in the number of tracks
 * contained in the database). If you need to lookup many IDs use
 * itdb_track_id_tree_create(), itdb_track_id_tree_destroy(), and
 * itdb_track_id_tree_by_id().
 *
 * Returns: #Itdb_Track with the ID @id or NULL if the ID cannot be
 * found.
 */
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
 * itdb_track_by_id_tree()
 *
 * Returns: a #GTree indexed by track IDs to be freed with
 * itdb_track_id_tree_destroy() when no longer used
 */
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
 */
void itdb_track_id_tree_destroy (GTree *idtree)
{
    g_return_if_fail (idtree);

    g_tree_destroy (idtree);
}

/**
 * itdb_track_id_tree_by_id:
 * @idtree: a #GTree created using itdb_track_id_tree_create()
 * @id:     the ID of the track to search for
 *
 * Lookup an #Itdb_Track by @id using @idtree for faster lookup
 * (compared to itdb_track_by_id())
 *
 * Returns: the #Itdb_Track whose ID is @id, or NULL if such a track
 * couldn't be found
 */
Itdb_Track *itdb_track_id_tree_by_id (GTree *idtree, guint32 id)
{
    g_return_val_if_fail (idtree, NULL);

    return (Itdb_Track *)g_tree_lookup (idtree, &id);
}

/**
 * itdb_track_has_thumbnails:
 * @track: an #Itdb_Track
 *
 * Determine if a @track has thumbnails
 *
 * Returns: TRUE if @track has artwork available, FALSE otherwise
 *
 * Since: 0.7.0
 */
gboolean itdb_track_has_thumbnails (Itdb_Track *track)
{
    g_return_val_if_fail (track != NULL, FALSE);
    return ((track->artwork) && (track->artwork->thumbnail));
}

/**
 * itdb_track_get_thumbnail:
 * @track:  an #Itdb_Track
 * @width:  width of the pixbuf to retrieve, -1 for the biggest possible size
 *          (with no scaling)
 * @height: height of the pixbuf to retrieve, -1 for the biggest possible size
 *          (with no scaling)
 *
 * Get a thumbnail representing the cover associated with the current track,
 * scaling it if appropriate. If either height or width is -1, then the biggest
 * unscaled thumbnail available will be returned.
 *
 * Returns: a #GdkPixbuf that must be unreffed when no longer used, NULL
 * if no artwork could be found or if libgpod is compiled without GdkPixbuf
 * support
 *
 * Since: 0.7.0
 */
gpointer itdb_track_get_thumbnail (Itdb_Track *track, gint width, gint height)
{
    g_return_val_if_fail (track != NULL, NULL);
    if (!itdb_track_has_thumbnails (track)) {
        return NULL;
    }
    if (track->itdb != NULL) {
        return itdb_thumb_to_pixbuf_at_size (track->itdb->device,
                                             track->artwork->thumbnail,
                                             width, height);
    } else {
        return itdb_thumb_to_pixbuf_at_size (NULL, track->artwork->thumbnail,
                                             width, height);
    }
}


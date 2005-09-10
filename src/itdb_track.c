/* Time-stamp: <2005-06-17 22:12:16 jcs>
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

#include "itdb_private.h"
#include <string.h>

/* Generate a new Itdb_Track structure */
Itdb_Track *itdb_track_new (void)
{
    Itdb_Track *track = g_new0 (Itdb_Track, 1);

    track->unk020 = 1;
    return track;
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

    if (pos == -1)  itdb->tracks = g_list_append (itdb->tracks, track);
    else  itdb->tracks = g_list_insert (itdb->tracks, track, pos);
}

/* Free the memory taken by @track */
void itdb_track_free (Itdb_Track *track)
{
    g_return_if_fail (track);

    g_free (track->album);
    g_free (track->artist);
    g_free (track->title);
    g_free (track->genre);
    g_free (track->comment);
    g_free (track->composer);
    g_free (track->fdesc);
    g_free (track->grouping);
    g_free (track->ipod_path);
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

/* Duplicate an existing track */
Itdb_Track *itdb_track_duplicate (Itdb_Track *tr)
{
    Itdb_Track *tr_dup;

    g_return_val_if_fail (tr, NULL);
    g_return_val_if_fail (!tr->userdata || tr->userdata_duplicate, NULL);

    tr_dup = g_new0 (Itdb_Track, 1);
    memcpy (tr_dup, tr, sizeof (Itdb_Track));

    /* clear itdb pointer */
    tr_dup->itdb = NULL;

    /* copy strings */
    tr_dup->album = g_strdup (tr->album);
    tr_dup->artist = g_strdup (tr->artist);
    tr_dup->title = g_strdup (tr->title);
    tr_dup->genre = g_strdup (tr->genre);
    tr_dup->comment = g_strdup (tr->comment);
    tr_dup->composer = g_strdup (tr->composer);
    tr_dup->fdesc = g_strdup (tr->fdesc);
    tr_dup->grouping = g_strdup (tr->grouping);
    tr_dup->ipod_path = g_strdup (tr->ipod_path);

    /* Copy userdata */
    if (tr->userdata)
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

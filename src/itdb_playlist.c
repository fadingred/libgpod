/* Time-stamp: <2005-11-19 16:24:56 jcs>
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
#include <glib/gi18n-lib.h>
#include <string.h>

/* spl_action_known(), itb_splr_get_field_type(),
 * itb_splr_get_action_type() are adapted from source provided by
 * Samuel "Otto" Wood (sam dot wood at gmail dot com). These part can
 * also be used under a FreeBSD license. You may also contact Samuel
 * for a complete copy of his original C++-classes.
 * */

/**
 * itdb_spl_action_known:
 * action: an #SPLAction
 *
 * Checks if @action is a known (to libgpod) smart playlist action.
 *
 * Return value: TRUE if @action is known. Otherwise a warning is displayed 
 * and FALSE is returned. 
 **/
gboolean itdb_spl_action_known (SPLAction action)
{
    gboolean result = FALSE;

    switch (action)
    {
    case SPLACTION_IS_INT:
    case SPLACTION_IS_GREATER_THAN:
    case SPLACTION_IS_NOT_GREATER_THAN:
    case SPLACTION_IS_LESS_THAN:
    case SPLACTION_IS_NOT_LESS_THAN:
    case SPLACTION_IS_IN_THE_RANGE:
    case SPLACTION_IS_NOT_IN_THE_RANGE:
    case SPLACTION_IS_IN_THE_LAST:
    case SPLACTION_IS_STRING:
    case SPLACTION_CONTAINS:
    case SPLACTION_STARTS_WITH:
    case SPLACTION_DOES_NOT_START_WITH:
    case SPLACTION_ENDS_WITH:
    case SPLACTION_DOES_NOT_END_WITH:
    case SPLACTION_IS_NOT_INT:
    case SPLACTION_IS_NOT_IN_THE_LAST:
    case SPLACTION_IS_NOT:
    case SPLACTION_DOES_NOT_CONTAIN:
	result = TRUE;
    }
    if (result == FALSE)
    {	/* New action! */
	g_warning (_("Unknown action (%d) in smart playlist will be ignored.\n"), action);
    }
    return result;
}

/**
 * itdb_splr_get_field_type:
 * @splr: an #SPLRule
 *
 * Gets the type of the field of the @splr rule
 *
 * Return value: an #SPLFieldType corresponding to @splr field type 
 * (string, int, date, ...)
 **/
SPLFieldType itdb_splr_get_field_type (const SPLRule *splr)
{
    g_return_val_if_fail (splr != NULL, splft_unknown);

    switch(splr->field)
    {
    case SPLFIELD_SONG_NAME:
    case SPLFIELD_ALBUM:
    case SPLFIELD_ARTIST:
    case SPLFIELD_GENRE:
    case SPLFIELD_KIND:
    case SPLFIELD_COMMENT:
    case SPLFIELD_COMPOSER:
    case SPLFIELD_GROUPING:
	return(splft_string);
    case SPLFIELD_BITRATE:
    case SPLFIELD_SAMPLE_RATE:
    case SPLFIELD_YEAR:
    case SPLFIELD_TRACKNUMBER:
    case SPLFIELD_SIZE:
    case SPLFIELD_PLAYCOUNT:
    case SPLFIELD_DISC_NUMBER:
    case SPLFIELD_BPM:
    case SPLFIELD_RATING:
    case SPLFIELD_TIME: /* time is the length of the track in
			   milliseconds */
	return(splft_int);
    case SPLFIELD_COMPILATION:
	return(splft_boolean);
    case SPLFIELD_DATE_MODIFIED:
    case SPLFIELD_DATE_ADDED:
    case SPLFIELD_LAST_PLAYED:
	return(splft_date);
    case SPLFIELD_PLAYLIST:
	return(splft_playlist);
    }
    return(splft_unknown);
}


/**
 * itdb_splr_get_action_type:
 * @splr: an #SPLRule
 * 
 * Gets the type of the action associated with @splr.
 *
 * Return value: type (range, date, string...) of the action field 
 **/
SPLActionType itdb_splr_get_action_type (const SPLRule *splr)
{
    SPLFieldType fieldType;

    g_return_val_if_fail (splr != NULL, splft_unknown);

    fieldType = itdb_splr_get_field_type (splr);

    switch(fieldType)
    {
    case splft_string:
	switch (splr->action)
	{
	case SPLACTION_IS_STRING:
	case SPLACTION_IS_NOT:
	case SPLACTION_CONTAINS:
	case SPLACTION_DOES_NOT_CONTAIN:
	case SPLACTION_STARTS_WITH:
	case SPLACTION_DOES_NOT_START_WITH:
	case SPLACTION_ENDS_WITH:
	case SPLACTION_DOES_NOT_END_WITH:
	    return splat_string;
	case SPLACTION_IS_NOT_IN_THE_RANGE:
	case SPLACTION_IS_INT:
	case SPLACTION_IS_NOT_INT:
	case SPLACTION_IS_GREATER_THAN:
	case SPLACTION_IS_NOT_GREATER_THAN:
	case SPLACTION_IS_LESS_THAN:
	case SPLACTION_IS_NOT_LESS_THAN:
	case SPLACTION_IS_IN_THE_RANGE:
	case SPLACTION_IS_IN_THE_LAST:
	case SPLACTION_IS_NOT_IN_THE_LAST:
	    return splat_invalid;
	default:
	    /* Unknown action type */
	    g_warning ("Unknown action type %d\n\n", splr->action);
	    return splat_unknown;
	}
	break;

    case splft_int:
	switch (splr->action)
	{
	case SPLACTION_IS_INT:
	case SPLACTION_IS_NOT_INT:
	case SPLACTION_IS_GREATER_THAN:
	case SPLACTION_IS_NOT_GREATER_THAN:
	case SPLACTION_IS_LESS_THAN:
	case SPLACTION_IS_NOT_LESS_THAN:
	    return splat_int;
	case SPLACTION_IS_NOT_IN_THE_RANGE:
	case SPLACTION_IS_IN_THE_RANGE:
	    return splat_range_int;
	case SPLACTION_IS_STRING:
	case SPLACTION_CONTAINS:
	case SPLACTION_STARTS_WITH:
	case SPLACTION_DOES_NOT_START_WITH:
	case SPLACTION_ENDS_WITH:
	case SPLACTION_DOES_NOT_END_WITH:
	case SPLACTION_IS_IN_THE_LAST:
	case SPLACTION_IS_NOT_IN_THE_LAST:
	case SPLACTION_IS_NOT:
	case SPLACTION_DOES_NOT_CONTAIN:
	    return splat_invalid;
	default:
	    /* Unknown action type */
	    g_warning ("Unknown action type %d\n\n", splr->action);
	    return splat_unknown;
	}
	break;

    case splft_boolean:
	return splat_none;

    case splft_date:
	switch (splr->action)
	{
	case SPLACTION_IS_INT:
	case SPLACTION_IS_NOT_INT:
	case SPLACTION_IS_GREATER_THAN:
	case SPLACTION_IS_NOT_GREATER_THAN:
	case SPLACTION_IS_LESS_THAN:
	case SPLACTION_IS_NOT_LESS_THAN:
	    return splat_date;
	case SPLACTION_IS_IN_THE_LAST:
	case SPLACTION_IS_NOT_IN_THE_LAST:
	    return splat_inthelast;
	case SPLACTION_IS_IN_THE_RANGE:
	case SPLACTION_IS_NOT_IN_THE_RANGE:
	    return splat_range_date;
	case SPLACTION_IS_STRING:
	case SPLACTION_CONTAINS:
	case SPLACTION_STARTS_WITH:
	case SPLACTION_DOES_NOT_START_WITH:
	case SPLACTION_ENDS_WITH:
	case SPLACTION_DOES_NOT_END_WITH:
	case SPLACTION_IS_NOT:
	case SPLACTION_DOES_NOT_CONTAIN:
	    return splat_invalid;
	default:
	    /* Unknown action type */
	    g_warning ("Unknown action type %d\n\n", splr->action);
	    return splat_unknown;
	}
	break;

    case splft_playlist:
	switch (splr->action)
	{
	case SPLACTION_IS_INT:
	case SPLACTION_IS_NOT_INT:
	    return splat_playlist;
	case SPLACTION_IS_GREATER_THAN:
	case SPLACTION_IS_NOT_GREATER_THAN:
	case SPLACTION_IS_LESS_THAN:
	case SPLACTION_IS_NOT_LESS_THAN:
	case SPLACTION_IS_IN_THE_LAST:
	case SPLACTION_IS_NOT_IN_THE_LAST:
	case SPLACTION_IS_IN_THE_RANGE:
	case SPLACTION_IS_NOT_IN_THE_RANGE:
	case SPLACTION_IS_STRING:
	case SPLACTION_CONTAINS:
	case SPLACTION_STARTS_WITH:
	case SPLACTION_DOES_NOT_START_WITH:
	case SPLACTION_ENDS_WITH:
	case SPLACTION_DOES_NOT_END_WITH:
	case SPLACTION_IS_NOT:
	case SPLACTION_DOES_NOT_CONTAIN:
	    return splat_invalid;
	default:
	    /* Unknown action type */
	    g_warning ("Unknown action type %d\n\n", splr->action);
	    return splat_unknown;
	}

    case splft_unknown:
	    /* Unknown action type */
	    g_warning ("Unknown action type %d\n\n", splr->action);
	    return splat_unknown;
    }
    return splat_unknown;
}

/* -------------------------------------------------------------------
 *
 * smart playlist stuff, adapted from source provided by Samuel "Otto"
 * Wood (sam dot wood at gmail dot com). This part can also be used
 * under a FreeBSD license. You can also contact Samuel for a complete
 * copy of his original C++-classes.
 *
 */


/**
 * itdb_splr_eval:
 * @splr: an #SPLRule
 * @track: an #Itdb_Track
 *
 * Evaluates @splr's truth against @track. track-&gt;itdb must be set.
 *
 * Return value: TRUE if @track matches @splr, FALSE otherwise.
 **/
gboolean itdb_splr_eval (SPLRule *splr, Itdb_Track *track)
{
    SPLFieldType ft;
    SPLActionType at;
    gchar *strcomp = NULL;
    gint64 intcomp = 0;
    gboolean boolcomp = FALSE;
    guint32 datecomp = 0;
    Itdb_Playlist *playcomp = NULL;
    time_t t;
    guint64 mactime;

    g_return_val_if_fail (splr, FALSE);
    g_return_val_if_fail (track, FALSE);
    g_return_val_if_fail (track->itdb, FALSE);

    ft = itdb_splr_get_field_type (splr);
    at = itdb_splr_get_action_type (splr);

    g_return_val_if_fail (at != splat_invalid, FALSE);

    /* find what we need to compare in the track */
    switch (splr->field)
    {
    case SPLFIELD_SONG_NAME:
	strcomp = track->title;
	break;
    case SPLFIELD_ALBUM:
	strcomp = track->album;
	break;
    case SPLFIELD_ARTIST:
	strcomp = track->artist;
	break;
    case SPLFIELD_GENRE:
	strcomp = track->genre;
	break;
    case SPLFIELD_KIND:
	strcomp = track->filetype;
	break;
    case SPLFIELD_COMMENT:
	strcomp = track->comment;
	break;
    case SPLFIELD_COMPOSER:
	strcomp = track->composer;
	break;
    case SPLFIELD_GROUPING:
	strcomp = track->grouping;
	break;
    case SPLFIELD_BITRATE:
	intcomp = track->bitrate;
	break;
    case SPLFIELD_SAMPLE_RATE:
	intcomp = track->samplerate;
	break;
    case SPLFIELD_YEAR:
	intcomp = track->year;
	break;
    case SPLFIELD_TRACKNUMBER:
	intcomp = track->track_nr;
	break;
    case SPLFIELD_SIZE:
	intcomp = track->size;
	break;
    case SPLFIELD_PLAYCOUNT:
	intcomp = track->playcount;
	break;
    case SPLFIELD_DISC_NUMBER:
	intcomp = track->cd_nr;
	break;
    case SPLFIELD_BPM:
	intcomp = track->BPM;
	break;
    case SPLFIELD_RATING:
	intcomp = track->rating;
	break;
    case SPLFIELD_TIME:
	intcomp = track->tracklen/1000;
	break;
    case SPLFIELD_COMPILATION:
	boolcomp = track->compilation;
	break;
    case SPLFIELD_DATE_MODIFIED:
	datecomp = track->time_modified;
	break;
    case SPLFIELD_DATE_ADDED:
	datecomp = track->time_added;
	break;
    case SPLFIELD_LAST_PLAYED:
	datecomp = track->time_played;
	break;
    case SPLFIELD_PLAYLIST:
	playcomp = itdb_playlist_by_id (track->itdb, splr->fromvalue);
	break;
    default: /* unknown field type */
	g_return_val_if_fail (FALSE, FALSE);
    }

    /* actually do the comparison to our rule */
    switch (ft)
    {
    case splft_string:
	if(strcomp && splr->string)
	{
	    gint len1 = strlen (strcomp);
	    gint len2 = strlen (splr->string);
	    switch (splr->action)
	    {
	    case SPLACTION_IS_STRING:
		return (strcmp (strcomp, splr->string) == 0);
	    case SPLACTION_IS_NOT:
		return (strcmp (strcomp, splr->string) != 0);
	    case SPLACTION_CONTAINS:
		return (strstr (strcomp, splr->string) != NULL);
	    case SPLACTION_DOES_NOT_CONTAIN:
		return (strstr (strcomp, splr->string) == NULL);
	    case SPLACTION_STARTS_WITH:
		return (strncmp (strcomp, splr->string, len2) == 0);
	    case SPLACTION_ENDS_WITH:
	    if (len2 > len1)  return FALSE;
	    return (strncmp (strcomp+len1-len2,
			     splr->string, len2) == 0);
	    case SPLACTION_DOES_NOT_START_WITH:
		return (strncmp (strcomp, splr->string,
				 strlen (splr->string)) != 0);
	    case SPLACTION_DOES_NOT_END_WITH:
		if (len2 > len1)  return TRUE;
		return (strncmp (strcomp+len1-len2,
				 splr->string, len2) != 0);
	    };
	}
	return FALSE;
    case splft_int:
	switch(splr->action)
	{
	case SPLACTION_IS_INT:
	    return (intcomp == splr->fromvalue);
	case SPLACTION_IS_NOT_INT:
	    return (intcomp != splr->fromvalue);
	case SPLACTION_IS_GREATER_THAN:
	    return (intcomp > splr->fromvalue);
	case SPLACTION_IS_LESS_THAN:
	    return (intcomp < splr->fromvalue);
	case SPLACTION_IS_IN_THE_RANGE:
	    return ((intcomp <= splr->fromvalue &&
		     intcomp >= splr->tovalue) ||
		    (intcomp >= splr->fromvalue &&
		     intcomp <= splr->tovalue));
	case SPLACTION_IS_NOT_IN_THE_RANGE:
	    return ((intcomp < splr->fromvalue &&
		     intcomp < splr->tovalue) ||
		    (intcomp > splr->fromvalue &&
		     intcomp > splr->tovalue));
	}
	return FALSE;
    case splft_boolean:
	switch (splr->action)
	{
	case SPLACTION_IS_INT:	    /* aka "is set" */
	    return (boolcomp != 0);
	case SPLACTION_IS_NOT_INT:  /* aka "is not set" */
	    return (boolcomp == 0);
	}
	return FALSE;
    case splft_date:
	switch (splr->action)
	{
	case SPLACTION_IS_INT:
	    return (datecomp == splr->fromvalue);
	case SPLACTION_IS_NOT_INT:
	    return (datecomp != splr->fromvalue);
	case SPLACTION_IS_GREATER_THAN:
	    return (datecomp > splr->fromvalue);
	case SPLACTION_IS_LESS_THAN:
	    return (datecomp < splr->fromvalue);
	case SPLACTION_IS_NOT_GREATER_THAN:
	    return (datecomp <= splr->fromvalue);
	case SPLACTION_IS_NOT_LESS_THAN:
	    return (datecomp >= splr->fromvalue);
	case SPLACTION_IS_IN_THE_LAST:
	    time (&t);
	    t += (splr->fromdate * splr->fromunits);
	    mactime = itdb_time_host_to_mac (t);
	    return (datecomp > mactime);
	case SPLACTION_IS_NOT_IN_THE_LAST:
	    time (&t);
	    t += (splr->fromdate * splr->fromunits);
	    mactime = itdb_time_host_to_mac (t);
	    return (datecomp <= mactime);
	case SPLACTION_IS_IN_THE_RANGE:
	    return ((datecomp <= splr->fromvalue &&
		     datecomp >= splr->tovalue) ||
		    (datecomp >= splr->fromvalue &&
		     datecomp <= splr->tovalue));
	case SPLACTION_IS_NOT_IN_THE_RANGE:
	    return ((datecomp < splr->fromvalue &&
		     datecomp < splr->tovalue) ||
		    (datecomp > splr->fromvalue &&
		     datecomp > splr->tovalue));
	}
	return FALSE;
    case splft_playlist:
	/* if we didn't find the playlist, just exit instead of
	   dealing with it */
	if (playcomp == NULL) return FALSE;

	switch(splr->action)
	{
	case SPLACTION_IS_INT:	  /* is this track in this playlist? */
	    return (itdb_playlist_contains_track (playcomp, track));
	case SPLACTION_IS_NOT_INT:/* NOT in this playlist? */
	    return (!itdb_playlist_contains_track (playcomp, track));
	}
	return FALSE;
    case splft_unknown:
	g_return_val_if_fail (ft != splft_unknown, FALSE);
	return FALSE;
    default: /* new type: warning to change this code */
	g_return_val_if_fail (FALSE, FALSE);
	return FALSE;
    }
    /* we should never make it out of the above switches alive */
    g_return_val_if_fail (FALSE, FALSE);
    return FALSE;
}

/* local functions to help with the sorting of the list of tracks so
 * that we can do limits */
static gint compTitle (Itdb_Track *a, Itdb_Track *b)
{
    return strcmp (a->title, b->title);
}
static gint compAlbum (Itdb_Track *a, Itdb_Track *b)
{
    return strcmp (a->album, b->album);
}
static gint compArtist (Itdb_Track *a, Itdb_Track *b)
{
    return strcmp (a->artist, b->artist);
}
static gint compGenre (Itdb_Track *a, Itdb_Track *b)
{
    return strcmp (a->genre, b->genre);
}
static gint compMostRecentlyAdded (Itdb_Track *a, Itdb_Track *b)
{
    return b->time_added - a->time_added;
}
static gint compLeastRecentlyAdded (Itdb_Track *a, Itdb_Track *b)
{
    return a->time_added - b->time_added;
}
static gint compMostOftenPlayed (Itdb_Track *a, Itdb_Track *b)
{
    return b->time_added - a->time_added;
}
static gint compLeastOftenPlayed (Itdb_Track *a, Itdb_Track *b)
{
    return a->time_added - b->time_added;
}
static gint compMostRecentlyPlayed (Itdb_Track *a, Itdb_Track *b)
{
    return b->time_played - a->time_played;
}
static gint compLeastRecentlyPlayed (Itdb_Track *a, Itdb_Track *b)
{
    return a->time_played - b->time_played;
}
static gint compHighestRating (Itdb_Track *a, Itdb_Track *b)
{
    return b->rating - a->rating;
}
static gint compLowestRating (Itdb_Track *a, Itdb_Track *b)
{
    return a->rating - b->rating;
}

/* Randomize the order of the members of the GList @list */
/* Returns a pointer to the new start of the list */
static GList *randomize_glist (GList *list)
{
    gint32 nr = g_list_length (list);

    while (nr > 1)
    {
	/* get random element among the first nr members */
	gint32 rand = g_random_int_range (0, nr);
	GList *gl = g_list_nth (list, rand);
	/* remove it and add it at the end */
	list = g_list_remove_link (list, gl);
	list = g_list_concat (list, gl);
	--nr;
    }
    return list;
}


/**
 * itdb_playlist_randomize:
 * @pl: an #Itdb_Playlist to randomize
 *
 * Randomizes @pl
 **/
void itdb_playlist_randomize (Itdb_Playlist *pl)
{
    g_return_if_fail (pl);

    pl->members = randomize_glist (pl->members);
}


/**
 * itdb_spl_update:
 * @spl: an #Itdb_Playlist
 *
 * Updates the content of the smart playlist @spl (meant to be called if the 
 * tracks stored in the #Itdb_iTunesDB associated with @spl have changed 
 * somehow and you want spl-&gt;members to be accurate with regards to those 
 * changes. Does nothing if @spl isn't a smart playlist.
 **/
void itdb_spl_update (Itdb_Playlist *spl)
{
    GList *gl;
    Itdb_iTunesDB *itdb;
    GList *sel_tracks = NULL;

    g_return_if_fail (spl);
    g_return_if_fail (spl->itdb);

    itdb = spl->itdb;

    /* we only can populate smart playlists */
    if (!spl->is_spl) return;

    /* clear this playlist */
    g_list_free (spl->members);
    spl->members = NULL;
    spl->num = 0;

    for (gl=itdb->tracks; gl ; gl=gl->next)
    {
	Itdb_Track *t = gl->data;
	g_return_if_fail (t);
	/* skip non-checked songs if we have to do so (this takes care
	   of *all* the match_checked functionality) */
	if (spl->splpref.matchcheckedonly && (t->checked == 0))
	    continue;
	/* first, match the rules */
	if (spl->splpref.checkrules)
	{   /* if we are set to check the rules */
	    /* start with true for "match all",
	       start with false for "match any" */
	    gboolean matchrules;
	    GList *gl;

	    if (spl->splrules.match_operator == SPLMATCH_AND)
		 matchrules = TRUE;
	    else matchrules = FALSE;
	    /* assume everything matches with no rules */
	    if (spl->splrules.rules == NULL) matchrules = TRUE;
	    /* match all rules */
	    for (gl=spl->splrules.rules; gl; gl=gl->next)
	    {
		SPLRule* splr = gl->data;
		gboolean ruletruth = itdb_splr_eval (splr, t);
		if (spl->splrules.match_operator == SPLMATCH_AND)
		{
		    if (!ruletruth)
		    {   /* one rule did not match -- we can stop */
			matchrules = FALSE;
			break;
		    }
		}
		else if (spl->splrules.match_operator == SPLMATCH_OR)
		{
		    if (ruletruth)
		    {   /* one rule matched -- we can stop */
			matchrules = TRUE;
			break;
		    }
		}
	    }
	    if (matchrules)
	    {   /* we have a track that matches the ruleset, append to
		 * playlist for now*/
		sel_tracks = g_list_append (sel_tracks, t);
	    }
	}
	else
	{   /* we aren't checking the rules, so just append to
	       playlist */
		sel_tracks = g_list_append (sel_tracks, t);
	}
    }
    /* no reason to go on if nothing matches so far */
    if (g_list_length (sel_tracks) == 0) return;

    /* do the limits */
    if (spl->splpref.checklimits)
    {
	/* use a double because we may need to deal with fractions
	 * here */
	gdouble runningtotal = 0;
	guint32 trackcounter = 0;
	guint32 tracknum = g_list_length (sel_tracks);

/* 	printf("limitsort: %d\n", spl->splpref.limitsort); */

	/* limit to (number) (type) selected by (sort) */
	/* first, we sort the list */
	switch(spl->splpref.limitsort)
	{
	case LIMITSORT_RANDOM:
	    sel_tracks = randomize_glist (sel_tracks);
	    break;
	case LIMITSORT_SONG_NAME:
	    sel_tracks = g_list_sort (sel_tracks, (GCompareFunc)compTitle);
	    break;
	case LIMITSORT_ALBUM:
	    sel_tracks = g_list_sort (sel_tracks, (GCompareFunc)compAlbum);
		break;
	case LIMITSORT_ARTIST:
	    sel_tracks = g_list_sort (sel_tracks, (GCompareFunc)compArtist);
	    break;
	case LIMITSORT_GENRE:
	    sel_tracks = g_list_sort (sel_tracks, (GCompareFunc)compGenre);
	    break;
	case LIMITSORT_MOST_RECENTLY_ADDED:
	    sel_tracks = g_list_sort (sel_tracks,
			 (GCompareFunc)compMostRecentlyAdded);
	    break;
	case LIMITSORT_LEAST_RECENTLY_ADDED:
	    sel_tracks = g_list_sort (sel_tracks,
			 (GCompareFunc)compLeastRecentlyAdded);
	    break;
	case LIMITSORT_MOST_OFTEN_PLAYED:
	    sel_tracks = g_list_sort (sel_tracks,
			 (GCompareFunc)compMostOftenPlayed);
	    break;
	case LIMITSORT_LEAST_OFTEN_PLAYED:
	    sel_tracks = g_list_sort (sel_tracks,
			 (GCompareFunc)compLeastOftenPlayed);
	    break;
	case LIMITSORT_MOST_RECENTLY_PLAYED:
	    sel_tracks = g_list_sort (sel_tracks,
			 (GCompareFunc)compMostRecentlyPlayed);
	    break;
	case LIMITSORT_LEAST_RECENTLY_PLAYED:
	    sel_tracks = g_list_sort (sel_tracks,
			 (GCompareFunc)compLeastRecentlyPlayed);
	    break;
	case LIMITSORT_HIGHEST_RATING:
	    sel_tracks = g_list_sort (sel_tracks,
			 (GCompareFunc)compHighestRating);
	    break;
	case LIMITSORT_LOWEST_RATING:
	    sel_tracks = g_list_sort (sel_tracks,
			 (GCompareFunc)compLowestRating);
	    break;
	default:
	    g_warning ("Programming error: should not reach this point (default of switch (spl->splpref.limitsort)\n");
	    break;
	}
	/* now that the list is sorted in the order we want, we
	   take the top X tracks off the list and insert them into
	   our playlist */

	while ((runningtotal < spl->splpref.limitvalue) &&
	       (trackcounter < tracknum))
	{
	    gdouble currentvalue=0;
	    Itdb_Track *t = g_list_nth_data (sel_tracks, trackcounter);

/* 	    printf ("track: %d runningtotal: %lf, limitvalue: %d\n", */
/* 		    trackcounter, runningtotal, spl->splpref.limitvalue); */

	    /* get the next song's value to add to running total */
	    switch (spl->splpref.limittype)
	    {
	    case LIMITTYPE_MINUTES:
		currentvalue = (double)(t->tracklen)/(60*1000);
		break;
	    case LIMITTYPE_HOURS:
		currentvalue = (double)(t->tracklen)/(60*60*1000);
		break;
	    case LIMITTYPE_MB:
		currentvalue = (double)(t->size)/(1024*1024);
		break;
	    case LIMITTYPE_GB:
		currentvalue = (double)(t->size)/(1024*1024*1024);
		break;
	    case LIMITTYPE_SONGS:
		currentvalue = 1;
		break;
	    default:
		g_warning ("Programming error: should not reach this point (default of switch (spl->splpref.limittype)\n");
		break;
	    }
	    /* check to see that we won't actually exceed the
	     * limitvalue */
	    if (runningtotal + currentvalue <=
		spl->splpref.limitvalue)
	    {
		runningtotal += currentvalue;
		/* Add the playlist entry */
		itdb_playlist_add_track (spl, t, -1);
	    }
	    /* increment the track counter so we can look at the next
	       track */
	    trackcounter++;
/* 	    printf ("  track: %d runningtotal: %lf, limitvalue: %d\n", */
/* 		    trackcounter, runningtotal, spl->splpref.limitvalue); */
	}	/* end while */
	/* no longer needed */
	g_list_free (sel_tracks);
	sel_tracks = NULL;
    } /* end if limits enabled */
    else
    {   /* no limits, so stick everything that matched the rules into
	   the playlist */
	spl->members = sel_tracks;
	spl->num = g_list_length (sel_tracks);
	sel_tracks = NULL;
    }
}


void spl_update (Itdb_Playlist *playlist, gpointer data);
void spl_update (Itdb_Playlist *playlist, gpointer data)
{
    g_return_if_fail (playlist);
    itdb_spl_update (playlist);
}


/**
 * itdb_spl_update_all:
 * @itdb: an #Itdb_iTunesDB
 *
 * Updates all smart playlists contained in @itdb
 **/
void itdb_spl_update_all (Itdb_iTunesDB *itdb)
{
    g_return_if_fail (itdb);

    g_list_foreach (itdb->playlists, (GFunc)spl_update, NULL);
}


void spl_update2 (Itdb_Playlist *playlist, gpointer data);
void spl_update2 (Itdb_Playlist *playlist, gpointer data)
{
    g_return_if_fail (playlist);
    if (playlist->is_spl && playlist->splpref.liveupdate)
        itdb_spl_update (playlist);
}


/** 
 * itdb_spl_update_live:
 * @itdb: an #Itdb_iTunesDB
 *
 * Updates all 'live' smart playlists contained in @itdb, ie those which have 
 * the 'live updating' flag set
 **/
void itdb_spl_update_live (Itdb_iTunesDB *itdb)
{
    g_return_if_fail (itdb);

    g_list_foreach (itdb->playlists, (GFunc)spl_update2, NULL);
}


/* end of code based on Samuel Wood's work */
/* ------------------------------------------------------------------- */


/**
 * itdb_splr_validate:
 * @splr: an #SPLRule
 *
 * Validates a rule 
 **/
void itdb_splr_validate (SPLRule *splr)
{
    SPLActionType at;

    g_return_if_fail (splr != NULL);

    at = itdb_splr_get_action_type (splr);

    g_return_if_fail (at != splat_unknown);

    switch (at)
    {
    case splat_int:
    case splat_playlist:
    case splat_date:
	splr->fromdate = 0;
	splr->fromunits = 1;
	splr->tovalue = splr->fromvalue;
	splr->todate = 0;
	splr->tounits = 1;
	break;
    case splat_range_int:
    case splat_range_date:
	splr->fromdate = 0;
	splr->fromunits = 1;
	splr->todate = 0;
	splr->tounits = 1;
	break;
    case splat_inthelast:
	splr->fromvalue = SPLDATE_IDENTIFIER;
	splr->tovalue = SPLDATE_IDENTIFIER;
	splr->tounits = 1;
	break;
    case splat_none:
    case splat_string:
	splr->fromvalue = 0;
	splr->fromdate = 0;
	splr->fromunits = 0;
	splr->tovalue = 0;
	splr->todate = 0;
	splr->tounits = 0;
	break;
    case splat_invalid:
    case splat_unknown:
	g_return_if_fail (FALSE);
	break;
    }

}


/**
 * itdb_splr_free:
 * @splr: an #SPLRule
 *
 * Frees the memory used by @splr 
 **/
void itdb_splr_free (SPLRule *splr)
{
    if (splr)
    {
	g_free (splr->string);
	g_free (splr);
    }
}

/**
 * itdb_splr_remove:
 * @pl: an #Itdb_Playlist
 * @splr: an SPLRule
 *
 * Removes the smart playlist rule @splr from playlist @pl. The memory used by
 * @splr is freed.
 **/
void itdb_splr_remove (Itdb_Playlist *pl, SPLRule *splr)
{
    g_return_if_fail (pl);
    g_return_if_fail (splr);

    pl->splrules.rules = g_list_remove (pl->splrules.rules, splr);
    itdb_splr_free (splr);
}

/**
 * itdb_splr_add:
 * @pl: an #Itdb_Playlist
 * @splr: an #SPLRule
 * @pos: position of the rule
 *
 * Adds the smart rule @splr to @pl at position @pos. If @pos is -1, @splr gets
 * appended to the end. After this call, @splr memory is managed by @pl, so 
 * you no longer need to call itdb_splr_free()
 **/
void itdb_splr_add (Itdb_Playlist *pl, SPLRule *splr, gint pos)
{
    g_return_if_fail (pl);
    g_return_if_fail (splr);

    pl->splrules.rules = g_list_insert (pl->splrules.rules,
					splr, pos);
}


/**
 * itdb_splr_new:
 * 
 * Creates a new default smart rule 
 *
 * Return value: a new #SPLRule that must be freed with itdb_splr_free() when 
 * no longer needed
 **/
SPLRule *itdb_splr_new (void)
{
    SPLRule *splr = g_new0 (SPLRule, 1);

    splr->field = SPLFIELD_ARTIST;
    splr->action = SPLACTION_CONTAINS;
    splr->fromvalue = 0;
    splr->fromdate = 0;
    splr->fromunits = 0;
    splr->tovalue = 0;
    splr->todate = 0;
    splr->tounits = 0;

    return splr;
}


/**
 * itdb_splr_add_new:
 * @pl: an #Itdb_Playlist
 * @pos: position to insert the rule at
 *
 * Creates a new smart rule and inserts it at position @pos in @pl. If @pos is
 * -1, the new rule gets appended to the end.
 *
 * Return value: pointer to the newly created #SPLRule. Its memory is handled 
 * by @pl though, so you don't need to explicitly call itdb_splr_free() on it
 **/
SPLRule *itdb_splr_add_new (Itdb_Playlist *pl, gint pos)
{
    SPLRule *splr;

    g_return_val_if_fail (pl, NULL);

    splr = itdb_splr_new ();
    itdb_splr_add (pl, splr, pos);
    return splr;
}

/* Duplicate SPLRule @splr */
static SPLRule *splr_duplicate (SPLRule *splr)
{
    SPLRule *dup = NULL;
    if (splr)
    {
	dup = g_malloc (sizeof (SPLRule));
	memcpy (dup, splr, sizeof (SPLRule));

	/* Now copy the strings */
	dup->string = g_strdup (splr->string);
    }
    return dup;
}


/**
 * itdb_playlist_duplicate:
 * @pl: an #Itdb_Playlist
 *
 * Duplicates an existing playlist. pl_dup-&gt;id is set to zero, so that
 * it will be set to a unique value when adding it to an #Itdb_iTunesDB. The
 * returned playlist won't be associated with an #Itdb_iTunesDB.
 *
 * Return value: a newly allocated #Itdb_Playlist that you'll have to free
 * with itdb_playlist_free() when you no longer need it. 
 **/
Itdb_Playlist *itdb_playlist_duplicate (Itdb_Playlist *pl)
{
    Itdb_Playlist *pl_dup;
    GList *gl;

    g_return_val_if_fail (pl, NULL);

    pl_dup = g_new (Itdb_Playlist, 1);
    memcpy (pl_dup, pl, sizeof (Itdb_Playlist));
    /* clear list heads */
    pl_dup->members = NULL;
    pl_dup->splrules.rules = NULL;

    /* clear itdb pointer */
    pl_dup->itdb = NULL;

    /* Now copy strings */
    pl_dup->name = g_strdup (pl->name);

    /* Copy members */
    pl_dup->members = g_list_copy (pl->members);

    /* Copy rules */
    for (gl=pl->splrules.rules; gl; gl=gl->next)
    {
	SPLRule *splr_dup = splr_duplicate (gl->data);
	pl_dup->splrules.rules = g_list_append (
	    pl_dup->splrules.rules, splr_dup);
    }

    /* Set id to 0, so it will be set to a unique value when adding
     * this playlist to a itdb */
    pl_dup->id = 0;

    /* Copy userdata */
    if (pl->userdata && pl->userdata_duplicate)
	pl_dup->userdata = pl->userdata_duplicate (pl->userdata);

    return pl_dup;
}


/**
 * itdb_spl_copy_rules:
 * @dest: destination #Itdb_Playlist
 * @src: source #Itdb_Playlist
 *
 * Copy all relevant information for smart playlist from playlist @src
 * to playlist @dest. If @dest is already a smart playlist, the existing data
 * is overwritten/deleted. 
 **/
void itdb_spl_copy_rules (Itdb_Playlist *dest, Itdb_Playlist *src)
{
    GList *gl;

    g_return_if_fail (dest);
    g_return_if_fail (src);
    g_return_if_fail (dest->is_spl);
    g_return_if_fail (src->is_spl);

    /* remove existing rules */
    g_list_foreach (dest->splrules.rules, (GFunc)(itdb_splr_free), NULL);
    g_list_free (dest->splrules.rules);

    /* copy general spl settings */
    memcpy (&dest->splpref, &src->splpref, sizeof (SPLPref));
    memcpy (&dest->splrules, &src->splrules, sizeof (SPLRules));
    dest->splrules.rules = NULL;

    /* Copy rules */
    for (gl=src->splrules.rules; gl; gl=gl->next)
    {
	SPLRule *splr_dup = splr_duplicate (gl->data);
	dest->splrules.rules = g_list_append (
	    dest->splrules.rules, splr_dup);
    }
}



/**
 * itdb_playlist_new:
 * @title: playlist title
 * @spl: smart playlist flag
 *
 * Creates a new playlist. If @spl is TRUE, a smart
 * playlist is generated. pl->id is set by itdb_playlist_add() when the 
 * playlist is added to an #Itdb_iTunesDB
 * 
 * Return value: a new #Itdb_Playlist which must be freed with 
 * itdb_playlist_free() after use
 **/
Itdb_Playlist *itdb_playlist_new (const gchar *title, gboolean spl)
{
    Itdb_Playlist *pl = g_new0 (Itdb_Playlist, 1);

    pl->type = ITDB_PL_TYPE_NORM;
    pl->name = g_strdup (title);
    pl->sortorder = ITDB_PSO_MANUAL;

    pl->timestamp = itdb_time_get_mac_time ();

    pl->is_spl = spl;
    if (spl)
    {
	pl->splpref.liveupdate = TRUE;
	pl->splpref.checkrules = TRUE;
	pl->splpref.checklimits = FALSE;
	pl->splpref.limittype = LIMITTYPE_HOURS;
	pl->splpref.limitsort = LIMITSORT_RANDOM;
	pl->splpref.limitvalue = 2;
	pl->splpref.matchcheckedonly = FALSE;
	pl->splrules.match_operator = SPLMATCH_AND;
	/* add at least one rule */
	itdb_splr_add_new (pl, 0);
    }
    return pl;
}


/**
 * itdb_playlist_free:
 * @pl: an #Itdb_Playlist
 *
 * Frees the memory used by playlist @pl. 
 **/
void itdb_playlist_free (Itdb_Playlist *pl)
{
    g_return_if_fail (pl);

    g_free (pl->name);
    g_list_free (pl->members);
    g_list_foreach (pl->splrules.rules, (GFunc)(itdb_splr_free), NULL);
    g_list_free (pl->splrules.rules);
    if (pl->userdata && pl->userdata_destroy)
	(*pl->userdata_destroy) (pl->userdata);
    g_free (pl);
}



/**
 * itdb_playlist_add:
 * @itdb: an #Itdb_iTunesDB
 * @pl: an #Itdb_Playlist
 * @pos: position to insert @pl at
 *
 * Adds playlist @pl to the database @itdb at position @pos (-1 for
 * "append to end"). A unique id is created if pl-&gt;id is equal to
 * zero. After calling this function, @itdb manages the memory of @pl, which
 * means you no longer need to explicitly call itdb_playlist_free()
 **/
void itdb_playlist_add (Itdb_iTunesDB *itdb, Itdb_Playlist *pl, gint32 pos)
{
    g_return_if_fail (itdb);
    g_return_if_fail (pl);
    g_return_if_fail (!pl->userdata || pl->userdata_duplicate);

    pl->itdb = itdb;

    /* set unique ID when not yet set */
    if (pl->id == 0)
    {
	GList *gl;
	guint64 id;
	do
	{
	    id = ((guint64)g_random_int () << 32) |
		((guint64)g_random_int ());
	    /* check if id is really unique (with 100 playlists the
	     * chance to create a duplicate is 1 in
	     * 184,467,440,737,095,516.16) */
	    for (gl=itdb->playlists; id && gl; gl=gl->next)
	    {
		Itdb_Playlist *g_pl = gl->data;
		g_return_if_fail (g_pl);
		if (id == g_pl->id)  id = 0;
	    }
	} while (id == 0);
	pl->id = id;
    }
    if (pl->sortorder == 0)  pl->sortorder = ITDB_PSO_MANUAL;
    if (pl->timestamp == 0)  pl->timestamp = itdb_time_get_mac_time ();
    if (pos == -1)  itdb->playlists = g_list_append (itdb->playlists, pl);
    else  itdb->playlists = g_list_insert (itdb->playlists, pl, pos);
}



/**
 * itdb_playlist_move:
 * @pl: an #Itdb_Playlist
 * @pos: new position
 *
 * Moves playlist @pl to position @pos 
 **/
void itdb_playlist_move (Itdb_Playlist *pl, guint32 pos)
{
    Itdb_iTunesDB *itdb;

    g_return_if_fail (pl);
    itdb = pl->itdb;
    g_return_if_fail (itdb);

    itdb->playlists = g_list_remove (itdb->playlists, pl);
    itdb->playlists = g_list_insert (itdb->playlists, pl, pos);
}


/**
 * itdb_playlist_remove:
 * @pl: an #Itdb_Playlist
 * 
 * Removes @pl from the #Itdb_iTunesDB it's associated with 
 * and frees memory 
 **/
void itdb_playlist_remove (Itdb_Playlist *pl)
{
    Itdb_iTunesDB *itdb;

    g_return_if_fail (pl);
    itdb = pl->itdb;
    g_return_if_fail (itdb);

    itdb->playlists = g_list_remove (itdb->playlists, pl);
    itdb_playlist_free (pl);
}


/**
 * itdb_playlist_unlink:
 * @pl: an #Itdb_Playlist
 *
 * Remove @pl from the #Itdb_iTunesDB it's associated with but do not free 
 * memory. pl-&gt;itdb is set to NULL after this function returns
 **/
void itdb_playlist_unlink (Itdb_Playlist *pl)
{
    Itdb_iTunesDB *itdb;

    g_return_if_fail (pl);
    itdb = pl->itdb;
    g_return_if_fail (itdb);

    itdb->playlists = g_list_remove (itdb->playlists, pl);
    pl->itdb = NULL;
}


/**
 * itdb_playlist_exists:
 * @itdb: an #Itdb_iTunesDB
 * @pl: an #Itdb_Playlist
 *
 * Checks if @pl is present in @db
 *
 * Return value: TRUE if @pl exists in @db, FALSE otherwise 
 **/
gboolean itdb_playlist_exists (Itdb_iTunesDB *itdb, Itdb_Playlist *pl)
{
    g_return_val_if_fail (itdb, FALSE);
    g_return_val_if_fail (pl, FALSE);

    if (g_list_find (itdb->playlists, pl))  return TRUE;
    else                                    return FALSE;
}


/**
 * itdb_playlist_add_track:
 * @pl: an #Itdb_Playlist
 * @track: an #Itdb_Track
 * @pos: position to insert @track at
 *
 * Adds @track to @pl at position @pos (-1 for "append to
 * end") 
 **/
void itdb_playlist_add_track (Itdb_Playlist *pl,
			      Itdb_Track *track, gint32 pos)
{
    g_return_if_fail (pl);
    g_return_if_fail (pl->itdb);
    g_return_if_fail (track);

    track->itdb = pl->itdb;

    if (pos == -1)  pl->members = g_list_append (pl->members, track);
    else  pl->members = g_list_insert (pl->members, track, pos);
}



/**
 * itdb_playlist_remove_track:
 * @pl: an #Itdb_Playlist
 * @track: an #Itdb_Track
 *
 * Removes @track from @pl. If @pl is NULL, removes @track from the
 * master playlist. If @track can't be found in @pl, nothing happens. If after
 * removing @track, @pl is empty, it's not removed from the database
 * The memory used by @track isn't freed.
 **/
void itdb_playlist_remove_track (Itdb_Playlist *pl, Itdb_Track *track)
{
    g_return_if_fail (track);

    if (pl == NULL)
	pl = itdb_playlist_mpl (track->itdb);

    g_return_if_fail (pl);

    pl->members = g_list_remove (pl->members, track);
}


/**
 * itdb_playlist_by_id:
 * @itdb: an #Itdb_iTunesDB
 * @id: ID of the playlist to look for
 *
 * Looks up a playlist whose ID is @id 
 *
 * Return value: the #Itdb_Playlist with ID @id or NULL if there is no such
 * playlist.
 **/
Itdb_Playlist *itdb_playlist_by_id (Itdb_iTunesDB *itdb, guint64 id)
{
    GList *gl;

    g_return_val_if_fail (itdb, NULL);

    for (gl=itdb->playlists; gl; gl=gl->next)
    {
	Itdb_Playlist *pl = gl->data;
	if (pl->id == id)  return pl;
    }
    return NULL;
}


/**
 * itdb_playlist_by_nr:
 * @itdb: an #Itdb_iTunesDB
 * @num: the position of the playlist, counting from 0
 *
 * Gets the playlist at the given position in @itdb
 *
 * Return value: the #Itdb_Playlist, or NULL if there is no playlist at @pos
 **/
Itdb_Playlist *itdb_playlist_by_nr (Itdb_iTunesDB *itdb, guint32 num)
{
    Itdb_Playlist *pl;
    g_return_val_if_fail (itdb, NULL);
    pl = g_list_nth_data (itdb->playlists, num);
    g_return_val_if_fail (pl, NULL);
    return pl;
}


/**
 * itdb_playlist_by_name:
 * @itdb: an #Itdb_iTunesDB
 * @name: name of the playlist to look for
 *
 * Searches a playlist whose name is @name in @itdb
 *
 * Return value: the first #Itdb_Playlist with name @name, NULL if there is no
 * such playlist
 **/
Itdb_Playlist *itdb_playlist_by_name (Itdb_iTunesDB *itdb, gchar *name)
{
    GList *gl;
    g_return_val_if_fail (itdb, NULL);
    g_return_val_if_fail (name, NULL);

    for (gl=itdb->playlists; gl; gl=gl->next)
    {
	Itdb_Playlist *pl = gl->data;
	g_return_val_if_fail (pl, NULL);
	if (pl->name && (strcmp (pl->name, name) == 0))
	    return pl;
    }
    return NULL;
}


/**
 * itdb_playlist_is_mpl:
 * @pl: an #Itdb_Playlist
 *
 * Checks if @pl is the master playlist 
 *
 * Return value: TRUE if @pl is the master playlist, FALSE otherwise 
 **/
gboolean itdb_playlist_is_mpl (Itdb_Playlist *pl)
{
    g_return_val_if_fail (pl, FALSE);

    return ((pl->type & 0xff) == ITDB_PL_TYPE_MPL);
}


/**
 * itdb_playlist_is_podcasts:
 * @pl: an #Itdb_Playlist
 *
 * Checks if @pl is the podcasts playlist
 *
 * Return value: TRUE if @pl is the podcasts playlist, FALSE otherwise 
 **/
gboolean itdb_playlist_is_podcasts (Itdb_Playlist *pl)
{
    g_return_val_if_fail (pl, FALSE);

    return (pl->podcastflag == ITDB_PL_FLAG_PODCASTS);
}


/**
 * itdb_playlist_set_mpl:
 * @pl: an #Itdb_Playlist
 *
 * Sets @pl to be a master playlist
 **/
void itdb_playlist_set_mpl (Itdb_Playlist *pl)
{
    g_return_if_fail (pl);

    pl->type = ITDB_PL_TYPE_MPL;
}


/**
 * itdb_playlist_set_podcasts:
 * @pl: an #Itdb_Playlist
 *
 * Set @pl to be a podcasts playlist 
 **/
void itdb_playlist_set_podcasts (Itdb_Playlist *pl)
{
    g_return_if_fail (pl);

    pl->podcastflag = ITDB_PL_FLAG_PODCASTS;
}


/**
 * itdb_playlist_mpl:
 * @itdb: an #Itdb_iTunesDB
 * 
 * Gets the master playlist of @itdb
 *
 * Return value: the master playlist of @itdb 
 **/
Itdb_Playlist *itdb_playlist_mpl (Itdb_iTunesDB *itdb)
{
    Itdb_Playlist *pl;

    g_return_val_if_fail (itdb, NULL);

    pl = g_list_nth_data (itdb->playlists, 0);
    g_return_val_if_fail (pl, NULL);

    /* mpl is guaranteed to be at first position... */
    g_return_val_if_fail (itdb_playlist_is_mpl (pl), NULL);

    return pl;
}


/**
 * itdb_playlist_podcasts:
 * @itdb: an #Itdb_iTunesDB
 *
 * Gets the podcasts playlist of @itdb
 *
 * Return value: the podcasts playlist of @itdb, or NULL if it's there is none
 **/
Itdb_Playlist *itdb_playlist_podcasts (Itdb_iTunesDB *itdb)
{
    GList *gl;

    g_return_val_if_fail (itdb, NULL);

    for (gl=itdb->playlists; gl; gl=gl->next)
    {
	Itdb_Playlist *pl = gl->data;
	g_return_val_if_fail (pl, NULL);

	if (itdb_playlist_is_podcasts (pl))    return pl;
    }

    return NULL;
}



/**
 * itdb_playlist_contains_track:
 * @pl: an #Itdb_Playlist
 * @track: an #Itdb_Track
 *
 * Checks if @track is in @pl.
 * 
 * Return value: TRUE if @track is in @pl, FALSE otherwise
 **/
gboolean itdb_playlist_contains_track (Itdb_Playlist *pl, Itdb_Track *tr)
{
    g_return_val_if_fail (tr, FALSE);

    if (pl == NULL)
	pl = itdb_playlist_mpl (tr->itdb);

    g_return_val_if_fail (pl, FALSE);

    if (g_list_find (pl->members, tr))  return TRUE;
    else                                return FALSE;
}


/**
 * itdb_playlist_contain_track_number:
 * @tr: an #Itdb_Track
 *
 * Counts the number of playlist @track is a member of (not including the 
 * master playlist)
 *
 * Return value: number of playlist containing @track
 **/
guint32 itdb_playlist_contain_track_number (Itdb_Track *tr)
{
    Itdb_iTunesDB *itdb;
    guint32 num = 0;
    GList *gl;

    g_return_val_if_fail (tr, 0);
    itdb = tr->itdb;
    g_return_val_if_fail (itdb, 0);

    /* start with 2nd playlist (skip MPL) */
    gl = g_list_nth (itdb->playlists, 1);
    while (gl)
    {
	g_return_val_if_fail (gl->data, num);
	if (itdb_playlist_contains_track (gl->data, tr)) ++num;
	gl = gl->next;
    }
    return num;
}



/**
 * itdb_playlist_tracks_number:
 * @pl: an #Itdb_Playlist
 *
 * Counts the number of tracks in @pl
 *
 * Return value: track count
 **/
guint32 itdb_playlist_tracks_number (Itdb_Playlist *pl)
{
    g_return_val_if_fail (pl, 0);

    return g_list_length (pl->members);
}

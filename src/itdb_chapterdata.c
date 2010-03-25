/* Time-stamp: <2007-03-21 17:30:57 jcs>
|
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
|  $Id: itdb_chapterdata.c 1612 2007-06-29 16:02:00Z jcsjcs $
*/

#include <config.h>

#include "itdb_device.h"
#include <stdio.h>
#include <string.h>
#include <glib/gi18n-lib.h>

/**
 * itdb_chapterdata_new:
 *
 * Creates a new #Itdb_Chapterdata
 *
 * Returns: a new #Itdb_Chapterdata to be freed with
 *               itdb_chapterdata_free() when no longer needed
 *
 * Since: 0.7.0
 */
Itdb_Chapterdata *itdb_chapterdata_new (void)
{
    Itdb_Chapterdata *chapterdata = g_new0 (Itdb_Chapterdata, 1);
    return chapterdata;
}

/**
 * itdb_chapterdata_free:
 * @chapterdata: an #Itdb_Chapterdata
 *
 * Frees memory used by @chapterdata
 *
 * Since: 0.7.0
 */
void itdb_chapterdata_free (Itdb_Chapterdata *chapterdata)
{
    g_return_if_fail (chapterdata);
    itdb_chapterdata_remove_chapters (chapterdata);
    g_free (chapterdata);
}

static GList *dup_chapters (GList *chapters)
{
    GList *it;
    GList *result;

    g_return_val_if_fail (chapters, NULL);
    result = NULL;
    for (it = chapters; it != NULL; it = it->next)
    {
	Itdb_Chapter *new_chapter;
	Itdb_Chapter *chapter;

	chapter = (Itdb_Chapter *)(it->data);
	g_return_val_if_fail (chapter, NULL);

	new_chapter = itdb_chapter_duplicate (chapter);

	result = g_list_prepend (result, new_chapter);
    }

    return g_list_reverse (result);
}

/**
 * itdb_chapterdata_duplicate:
 * @chapterdata: an #Itdb_Chapterdata
 *
 * Duplicates @chapterdata
 *
 * Returns: a new copy of @chapterdata
 *
 * Since: 0.7.0
 */
Itdb_Chapterdata *itdb_chapterdata_duplicate (Itdb_Chapterdata *chapterdata)
{
    Itdb_Chapterdata *dup;
    int numchapters;
    g_return_val_if_fail (chapterdata, NULL);

    dup = g_new0 (Itdb_Chapterdata, 1);

    memcpy (dup, chapterdata, sizeof (Itdb_Chapterdata));

    numchapters = g_list_length (chapterdata->chapters);
    if (chapterdata->chapters)
	dup->chapters = dup_chapters (chapterdata->chapters);
    else
	dup->chapters = NULL;

    return dup;
}

/**
 * itdb_chapterdata_remove_chapter:
 * @chapterdata: an #Itdb_Chapterdata
 * @chapter:     an #Itdb_Chapter
 *
 * Removes @chapter from @chapterdata. The memory used by @chapter is freed.
 *
 * Since: 0.7.0
 */
void
itdb_chapterdata_remove_chapter (Itdb_Chapterdata *chapterdata, Itdb_Chapter *chapter)
{
    itdb_chapterdata_unlink_chapter(chapterdata, chapter);
    itdb_chapter_free (chapter);
}

void
itdb_chapterdata_unlink_chapter (Itdb_Chapterdata *chapterdata, Itdb_Chapter *chapter)
{
    g_return_if_fail (chapterdata);
    g_return_if_fail (chapter);

    chapterdata->chapters = g_list_remove (chapterdata->chapters, chapter);
}

/**
 * itdb_chapterdata_remove_chapters:
 * @chapterdata: an #Itdb_Chapterdata
 *
 * Removes all chapters from @chapterdata
 *
 * Since: 0.7.0
 */
void
itdb_chapterdata_remove_chapters (Itdb_Chapterdata *chapterdata)
{
    g_return_if_fail (chapterdata);

    while (chapterdata->chapters)
    {
	Itdb_Chapter *chapter = chapterdata->chapters->data;
	g_return_if_fail (chapter);
	itdb_chapterdata_remove_chapter (chapterdata, chapter);
    }
}

/**
 * itdb_chapter_new:
 *
 * Creates a new #Itdb_Chapter
 *
 * Returns: newly allocated #Itdb_Chapter to be freed with itdb_chapter_free()
 * after use
 *
 * Since: 0.7.0
 */
Itdb_Chapter *itdb_chapter_new (void)
{
    Itdb_Chapter *chapter = g_new0 (Itdb_Chapter, 1);
    return chapter;
}

/**
 * itdb_chapter_free:
 * @chapter: an #Itdb_Chapter
 *
 * Frees the memory used by @chapter
 *
 * Since: 0.7.0
 */
void itdb_chapter_free (Itdb_Chapter *chapter)
{
    g_return_if_fail (chapter);

    g_free (chapter->chaptertitle);
    g_free (chapter);
}

/**
 * itdb_chapter_duplicate:
 * @chapter: an #Itdb_Chapter
 *
 * Duplicates the data contained in @chapter
 *
 * Returns: a newly allocated copy of @chapter to be freed with
 * itdb_chapter_free() after use
 *
 * Since: 0.7.0
 */
Itdb_Chapter *itdb_chapter_duplicate (Itdb_Chapter *chapter)
{
    Itdb_Chapter *new_chapter;

    g_return_val_if_fail (chapter, NULL);

    new_chapter = itdb_chapter_new ();
    memcpy (new_chapter, chapter, sizeof (Itdb_Chapter));
    new_chapter->chaptertitle = g_strdup (chapter->chaptertitle);

    return new_chapter;
}

/**
 * itdb_chapterdata_add_chapter
 * @chapterdata:  an #Itdb_Chapterdata
 * @startpos:     chapter start time in milliseconds
 * @chaptertitle: chapter title
 *
 * Appends a chapter to existing chapters in @chapterdata.
 *
 * Returns: TRUE if the chapter could be successfully added, FALSE
 * otherwise.
 *
 * Since: 0.7.0
 */
gboolean
itdb_chapterdata_add_chapter (Itdb_Chapterdata *chapterdata,
			      guint32 startpos,
			      gchar *chaptertitle)
{
    Itdb_Chapter *chapter;

    g_return_val_if_fail (chapterdata, FALSE);
    g_return_val_if_fail (chaptertitle, FALSE);

    chapter = itdb_chapter_new ();
    if (startpos == 0)
	chapter->startpos = 1; /* first chapter should have startpos of 1 */
    else
	chapter->startpos = startpos;
    chapter->chaptertitle = g_strdup (chaptertitle);
    chapterdata->chapters = g_list_append (chapterdata->chapters, chapter);

    return TRUE;
}

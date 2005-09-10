/* Time-stamp: <2005-07-09 15:48:29 jcs>
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

#ifndef __ITDB_PRIVATE_H__
#define __ITDB_PRIVATE_H__

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "itdb.h"

/* keeps the contents of one disk file (read) */
typedef struct
{
    gchar *filename;
    gchar *contents;
    gsize length;
    GError *error;
} FContents;


/* struct used to hold all necessary information when importing a
   Itdb_iTunesDB */
typedef struct
{
    Itdb_iTunesDB *itdb;
    FContents *itunesdb;
    GList *pos_glist;    /* temporary list to store position indicators */
    GList *playcounts;   /* contents of Play Counts file */
    GTree *idtree;       /* temporary tree with track id tree */
    GError *error;       /* where to report errors to */
} FImport;

/* data of playcounts GList above */
struct playcount {
    guint32 playcount;
    guint32 time_played;
    guint32 bookmark_time;
    gint32 rating;
    gint32 unk16;
};

/* value to indicate that playcount was not set in struct playcount
   above */
#define NO_PLAYCOUNT (-1)


/* keeps the contents of the output file (write) */
typedef struct
{
    gchar *filename;
    gchar *contents;     /* pointer to contents */
    gulong pos;          /* current write position ("end of file") */
    gulong total;        /* current total size of *contents array  */
    GError *error;       /* place to report errors to */
} WContents;

/* size of memory by which the total size of above WContents gets
 * increased (1.5 MB) */
#define WCONTENTS_STEPSIZE 1572864

/* struct used to hold all necessary information when exporting a
 * Itdb_iTunesDB */
typedef struct
{
    Itdb_iTunesDB *itdb;
    WContents *itunesdb;
    GError *error;       /* where to report errors to */
} FExport;


gboolean itdb_spl_action_known (SPLAction action);
#endif

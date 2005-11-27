/* File : gpod.i */

/*
 Copyright (C) 2005 Nick Piper <nick-gtkpod at nickpiper co uk>
 Part of the gtkpod project.
 
 URL: http://www.gtkpod.org/
 URL: http://gtkpod.sourceforge.net/

 The code contained in this file is free software; you can redistribute
 it and/or modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either version
 2.1 of the License, or (at your option) any later version.

 This file is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this code; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

 $Id$

Please send any fixes, improvements or suggestions to
<nick-gtkpod at nickpiper co uk>.

*/

%module gpod
%{
#include "../../src/db-artwork-debug.h" 
#include "../../src/db-artwork-parser.h" 
#include "../../src/db-image-parser.h" 
#include "../../src/db-itunes-parser.h" 
#include "../../src/db-parse-context.h" 
#include "../../src/itdb.h" 
#include "../../src/itdb_private.h"

PyObject* sw_get_tracks(Itdb_iTunesDB *itdb) {
  PyObject    *list;
  gint        i;
  GList       *l;
  list = PyList_New(g_list_length(itdb->tracks));
  for (l = itdb->tracks, i = 0; l; l = l->next, ++i) {
    PyList_SET_ITEM(list, i, SWIG_NewPointerObj((void*)(l->data), SWIGTYPE_p_Itdb_Track, 0));
  }
  return list;
 }

PyObject* sw_get_playlist_tracks(Itdb_Playlist *pl) {
  PyObject    *list;
  gint        i;
  GList       *l;
  list = PyList_New(g_list_length(pl->members));
  for (l = pl->members, i = 0; l; l = l->next, ++i) {
    PyList_SET_ITEM(list, i, SWIG_NewPointerObj((void*)(l->data), SWIGTYPE_p_Itdb_Track, 0));
  }
  return list;
 }

PyObject* sw_get_playlists(Itdb_iTunesDB *itdb) {
  PyObject    *list;
  gint        i;
  GList       *l;
  list = PyList_New(g_list_length(itdb->playlists));
  for (l = itdb->playlists, i = 0; l; l = l->next, ++i) {
    PyList_SET_ITEM(list, i, SWIG_NewPointerObj((void*)(l->data), SWIGTYPE_p_Itdb_Playlist, 0));
  }
  return list;
 }
%}

# be nicer to decode these utf8 strings into Unicode objects in the C
# layer. Here we are leaving it to the Python side, and just giving
# them utf8 encoded Strings.
typedef char gchar;

typedef int gboolean;
typedef int gint32;
typedef unsigned int guint32;

#define G_BEGIN_DECLS
#define G_END_DECLS

PyObject* sw_get_tracks(Itdb_iTunesDB *itdb);
PyObject* sw_get_playlists(Itdb_iTunesDB *itdb);
PyObject* sw_get_playlist_tracks(Itdb_Playlist *pl);
%include "../../src/itdb.h"

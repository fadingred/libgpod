/* File : gpod.i */
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

typedef char gchar;
typedef int gboolean;
typedef int gint32;
typedef unsigned int guint32;

PyObject* sw_get_tracks(Itdb_iTunesDB *itdb);
PyObject* sw_get_playlists(Itdb_iTunesDB *itdb);
%include "../../src/itdb.h"

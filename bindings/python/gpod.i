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
    PyList_SET_ITEM(list, i, SWIG_NewPointerObj((void*)(l->data), SWIGTYPE_p__Itdb_Track, 0));
  }
  return list;
 }

PyObject* sw_get_track(GList *list, gint index) {
  GList *position;
  if ( (index >= g_list_length(list)) || index < 0 ) {
   PyErr_SetString(PyExc_IndexError, "Value out of range");
   return NULL;
  }
  position = g_list_nth(list, index);
  return SWIG_NewPointerObj((void*)(position->data), SWIGTYPE_p__Itdb_Track, 0);
 }

PyObject* sw_get_rule(GList *list, gint index) {
  GList *position;
  if ( (index >= g_list_length(list)) || index < 0 ) {
   PyErr_SetString(PyExc_IndexError, "Value out of range");
   return NULL;
  }
  position = g_list_nth(list, index);
  return SWIG_NewPointerObj((void*)(position->data), SWIGTYPE_p__SPLRule, 0);
 }

PyObject* sw_get_list_len(GList *list) {
   return PyInt_FromLong(g_list_length(list));
 }

PyObject* sw_get_playlist_tracks(Itdb_Playlist *pl) {
  PyObject    *list;
  gint        i;
  GList       *l;
  list = PyList_New(g_list_length(pl->members));
  for (l = pl->members, i = 0; l; l = l->next, ++i) {
    PyList_SET_ITEM(list, i, SWIG_NewPointerObj((void*)(l->data), SWIGTYPE_p__Itdb_Track, 0));
  }
  return list;
 }

PyObject* sw_get_playlists(Itdb_iTunesDB *itdb) {
  PyObject    *list;
  gint        i;
  GList       *l;
  list = PyList_New(g_list_length(itdb->playlists));
  for (l = itdb->playlists, i = 0; l; l = l->next, ++i) {
    PyList_SET_ITEM(list, i, SWIG_NewPointerObj((void*)(l->data), SWIGTYPE_p__Itdb_Playlist, 0));
  }
  return list;
 }
%}

# be nicer to decode these utf8 strings into Unicode objects in the C
# layer. Here we are leaving it to the Python side, and just giving
# them utf8 encoded Strings.
typedef char gchar;

%typemap(in) guint8 {
   unsigned long ival;
   ival = PyInt_AsUnsignedLongMask($input);
   if (PyErr_Occurred())
        SWIG_fail;
   if (ival > 255) {
      PyErr_SetString(PyExc_ValueError, "$symname: Value must be between 0 and 255");
      SWIG_fail;
   } else {
      $1 = (guint8) ival;
   }
}

%typemap(in) gint8 {
   long ival;
   ival = PyInt_AsInt($input);
   if (PyErr_Occurred())
        SWIG_fail;
   if ((ival < -128) || (ival > 127)) {
      PyErr_SetString(PyExc_ValueError, "$symname: Value must be between -128 and 127");
      SWIG_fail;
   } else {
      $1 = (gint8) ival;
   }
}

%typemap(in) guint16 {
   unsigned long ival;
   ival = PyInt_AsUnsignedLongMask($input);
   if (PyErr_Occurred())
        SWIG_fail;
   if (ival > 65535) {
      PyErr_SetString(PyExc_ValueError, "$symname: Value must be between 0 and 65535");
      SWIG_fail;
   } else {
      $1 = (guint16) ival;
   }
}

%typemap(in) gint16 {
   long ival;
   ival = PyInt_AsLong($input);
   if (PyErr_Occurred())
        SWIG_fail;
   if ((ival < -32768) || (ival > 32767)) {
      PyErr_SetString(PyExc_ValueError, "$symname: Value must be between -32,768 and 32,767");
      SWIG_fail;
   } else {
      $1 = (gint16) ival;
   }
}

%typemap(in) guint32 {
   unsigned long ival;
   ival = PyInt_AsUnsignedLongMask($input);
   if (PyErr_Occurred())
        SWIG_fail;
   $1 = (guint32) ival;
}

%typemap(in) gint32 {
   long ival;
   ival = PyInt_AsLong($input);
   if (PyErr_Occurred())
        SWIG_fail;
   $1 = (gint32) ival;
}

%typemap(in) guint64 {
   unsigned long ival;
   ival = PyInt_AsUnsignedLongLongMask($input);
   if (PyErr_Occurred())
        SWIG_fail;
   $1 = (guint64) ival;
}

%typemap(in) gint64 {
   long ival;
   ival = PyInt_AsUnsignedLongMask($input);
   if (PyErr_Occurred())
        SWIG_fail;
   $1 = (gint64) ival;
}

%typemap(out) guint64 {
   $result = PyLong_FromUnsignedLongLong($1);
}

%typemap(out) gint64 {
   $result = PyLong_FromLongLong($1);
}

%typemap(out) guint32 {
   $result = PyLong_FromUnsignedLong($1);
}

%typemap(out) gint32 {
   $result = PyLong_FromLong($1);
}

%typemap(out) guint16 {
   $result = PyLong_FromUnsignedLong($1);
}

%typemap(out) guint8 {
   $result = PyInt_FromLong($1);
}

typedef int gboolean;
typedef int gint;

#define G_BEGIN_DECLS
#define G_END_DECLS

PyObject* sw_get_tracks(Itdb_iTunesDB *itdb);
PyObject* sw_get_track(GList *list, gint index);
PyObject* sw_get_rule(GList *list, gint index);
PyObject* sw_get_list_len(GList *list);
PyObject* sw_get_playlists(Itdb_iTunesDB *itdb);
PyObject* sw_get_playlist_tracks(Itdb_Playlist *pl);
%include "../../src/itdb.h"

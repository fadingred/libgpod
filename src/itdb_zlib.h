/*
|  Copyright (C) 2009 Christophe Fergeau <teuf@gnome.org>
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
#ifndef __ITDB_ZLIB_H__
#define __ITDB_ZLIB_H__

G_BEGIN_DECLS
#include <glib.h>
#include <itdb_private.h>

G_GNUC_INTERNAL gboolean itdb_zlib_check_decompress_fimp (FImport *fimp);
G_GNUC_INTERNAL gboolean itdb_zlib_check_compress_fexp (FExport *fexp);

G_END_DECLS

#endif

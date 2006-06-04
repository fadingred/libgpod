/*
 *  Copyright (C) 2005 Christophe Fergeau
 *
 * 
 *  The code contained in this file is free software; you can redistribute
 *  it and/or modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either version
 *  2.1 of the License, or (at your option) any later version.
 *  
 *  This file is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *  
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this code; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307 USA
 * 
 *  iTunes and iPod are trademarks of Apple
 * 
 *  This product is not supported/written/published by Apple!
 *
 *  $Id$
 */

#ifndef GLIB_COMPAT_H
#define GLIB_COMPAT_H

#include <glib.h>

#if ((GLIB_MAJOR_VERSION == 2) && (GLIB_MINOR_VERSION <= 4))
/*** glib <= 2.4 ***/

/* G_GNUC_INTERNAL */
#ifndef G_GNUC_INTERNAL
#define G_GNUC_INTERNAL
#endif

/* g_stat */
#include <stdio.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/stat.h>
#define g_stat stat
#define g_mkdir mkdir
#define g_rename rename
#define g_printf printf

/* G_IS_DIR_SEPARATOR */
#ifdef G_OS_WIN32
#define G_IS_DIR_SEPARATOR(c) ((c) == G_DIR_SEPARATOR || (c) == '/')
#else
#define G_IS_DIR_SEPARATOR(c) ((c) == G_DIR_SEPARATOR)
#endif

#else
/*** glib > 2.4 ***/

/* g_stat */
#include <glib/gstdio.h>
#endif



#endif /* GLIB_COMPAT_H */

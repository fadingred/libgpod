/* Time-stamp: <2005-12-10 00:22:44 jcs>
|
|  Copyright (C) 2005 Jorg Schuler <jcsjcs at users sourceforge net>
|  Part of the gtkpod project.
| 
|  URL: http://www.gtkpod.org/
|  URL: http://gtkpod.sourceforge.net/
|
|
|  The source is taken from libipoddevice, CVS version October 8 2005
|  (http://64.14.94.162/index.php/Libipoddevice).
|
|  I decided not to make libgpod dependent on libipoddevice because
|  the latter depends on libraries not widely available yet (libhal >=
|  0.5.2, glib >= 2.8). It is planned to replace these files with a
|  libipoddevice dependence at some later time.
|
|  The following changes were done:
|
|  - libhal becomes optional (see #ifdef HAVE_LIBHAL sections)
|  - provide some dummy libhal functions to make libhal-independence
|    of ipod-device.c easier.
|
|  Because of these changes only a limited amount of functionality is
|  available. See ipod-device.h for summary.
|
|
|
|
|  $Id$
*/
/* ex: set ts=4: */
/***************************************************************************
*  hal-common.c
*  Copyright (C) 2005 Novell 
*  Written by Aaron Bockover <aaron@aaronbock.net>
****************************************************************************/

/*  
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of version 2.1 of the GNU Lesser General Public
 *  License as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Lesser Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 *  USA
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "itdb_private.h"
#include "hal-common.h"

#ifndef HAVE_LIBHAL
gchar **libhal_manager_find_device_string_match (LibHalContext *hal_ctx,
						 const gchar *type,
						 const gchar *str,
						 gint *vol_count,
						 void *error)
{
/*    gchar **volumes = g_new0 (gchar *, 2);
      volumes[0] = g_strdup (type);*/
    gchar **volumes = NULL;
    *vol_count = 0;
    return volumes;
}
void libhal_free_string_array (gchar **volumes)
{
/*    g_strfreev (volumes);*/
}
gboolean libhal_device_property_exists (LibHalContext *hal_ctx,
					const gchar *vol,
					const gchar *prop,
					void *error)
{
    return FALSE;
}
gboolean libhal_device_get_property_bool (LibHalContext *hal_ctx,
					  const gchar *vol,
					  const gchar *prop,
					  void *error)
{
    return FALSE;
}
void libhal_ctx_shutdown (LibHalContext *hal_ctx, void *error) {}
void libhal_ctx_free (LibHalContext *hal_ctx) {}
#endif

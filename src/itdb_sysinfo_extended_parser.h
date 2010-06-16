/*
|  Copyright (C) 2008 Christophe Fergeau <teuf@gnome.org>
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
#ifndef __ITDB_SYSINFO_EXTENDED_PARSER_H__
#define __ITDB_SYSINFO_EXTENDED_PARSER_H__

#include <glib.h>

G_BEGIN_DECLS

typedef struct _SysInfoIpodProperties SysInfoIpodProperties;

G_GNUC_INTERNAL void
itdb_sysinfo_properties_dump (SysInfoIpodProperties *props);

G_GNUC_INTERNAL
SysInfoIpodProperties *itdb_sysinfo_extended_parse (const char *filename,
                                                    GError **error);

G_GNUC_INTERNAL
SysInfoIpodProperties *itdb_sysinfo_extended_parse_from_xml (const char *xml,
							     GError **error);

G_GNUC_INTERNAL
void itdb_sysinfo_properties_free (SysInfoIpodProperties *props);

G_GNUC_INTERNAL const char *
itdb_sysinfo_properties_get_serial_number (const SysInfoIpodProperties *props);

G_GNUC_INTERNAL const char *
itdb_sysinfo_properties_get_firewire_id (const SysInfoIpodProperties *props);

G_GNUC_INTERNAL const GList *
itdb_sysinfo_properties_get_cover_art_formats (const SysInfoIpodProperties *);
G_GNUC_INTERNAL const GList *
itdb_sysinfo_properties_get_photo_formats (const SysInfoIpodProperties *);
G_GNUC_INTERNAL const GList *
itdb_sysinfo_properties_get_chapter_image_formats (const SysInfoIpodProperties *);
G_GNUC_INTERNAL gboolean
itdb_sysinfo_properties_supports_sparse_artwork (const SysInfoIpodProperties *);
G_GNUC_INTERNAL gboolean
itdb_sysinfo_properties_supports_podcast (const SysInfoIpodProperties *);
G_GNUC_INTERNAL const char *
itdb_sysinfo_properties_get_firmware_version (const SysInfoIpodProperties *);
G_GNUC_INTERNAL gboolean
itdb_sysinfo_properties_supports_sqlite (const SysInfoIpodProperties *props);
G_GNUC_INTERNAL gint
itdb_sysinfo_properties_get_family_id (const SysInfoIpodProperties *props);
G_GNUC_INTERNAL gint
itdb_sysinfo_properties_get_db_version (const SysInfoIpodProperties *props);
G_GNUC_INTERNAL gint
itdb_sysinfo_properties_get_shadow_db_version (const SysInfoIpodProperties *props);

G_END_DECLS

#endif

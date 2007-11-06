/* Copyright (c) 2007, Christophe Fergeau  <teuf@gnome.org>
 *
 * The code contained in this file is free software; you can redistribute
 * it and/or modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either version
 * 2.1 of the License, or (at your option) any later version.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this code; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * iTunes and iPod are trademarks of Apple
 *
 * This product is not supported/written/published by Apple!
 *
 */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>
#include <glib/gi18n.h>

#include "itdb.h"

extern char *read_sysinfo_extended (const char *device);

int
main (int argc, char **argv)
{
    char *xml;
    
    if (argc < 3) {
      g_print (_("usage: %s <device> <mountpoint>\n"), g_basename (argv[0]));
	return 1;
    }

    xml = read_sysinfo_extended (argv[1]);
    if (xml == NULL) {
      g_print (_("Couldn't read xml sysinfo from %s\n"), argv[1]);
      return 1;
    } else {
	const char *mountpoint = argv[2];
	char *device_path;
	char *filename;
	gboolean success;

	device_path = itdb_get_device_dir (mountpoint);
	if (device_path == NULL) {
	    g_free (xml);
	    g_print (_("Couldn't resolve Device directory path on %s"), 
		     mountpoint);
	    return 1;
	}

	filename = g_build_filename (device_path, "SysInfoExtended", NULL);
	g_free (device_path);
	if (filename == NULL) {
	    g_print (_("Couldn't resolve SysInfoExtended path on %s"),
		     mountpoint);
	    g_free (xml);
	    return 1;
	}

	success = g_file_set_contents (filename, xml, -1, NULL);
	g_free (xml);
	g_free (filename);
	if (!success) {
	    g_print (_("Couldn't write SysInfoExtended to %s"), mountpoint);
	    return 1;
	}
    }

    return 0;
}

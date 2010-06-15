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
#include <glib/gstdio.h>
#include <stdlib.h>

#include "itdb.h"
#ifdef HAVE_SGUTILS
extern char *read_sysinfo_extended (const char *device);
#endif
#ifdef HAVE_LIBIMOBILEDEVICE
extern char *read_sysinfo_extended_by_uuid (const char *uuid);
#endif
#ifdef HAVE_LIBUSB
extern char *read_sysinfo_extended_from_usb (guint bus_number, guint device_address);
#endif

static gboolean write_sysinfo_extended (const char *mountpoint,
                                        const char *data)
{
        char *filename;
        char *devdirpath;
        gboolean result;

        devdirpath = itdb_get_device_dir (mountpoint);
        /* Make sure the device dir exists (not necessarily true on
         * Shuffles */
        if (devdirpath == NULL) {
            gchar *itunesdirpath;

            itunesdirpath = itdb_get_itunes_dir (mountpoint);
            if (itunesdirpath == NULL) {
                return FALSE;
            }
            devdirpath = g_build_filename (itunesdirpath, "Device", NULL);
            g_free (itunesdirpath);
            g_mkdir (devdirpath, 0777);
        }
        filename = g_build_filename (devdirpath, "SysInfoExtended", NULL);
        g_free (devdirpath);
        if (filename == NULL) {
                return FALSE;
        }

        result = g_file_set_contents (filename, data, -1, NULL);
        g_free (filename);

        return result;
}

int
main (int argc, char **argv)
{
    char *xml;
    
    if (argc < 3) {
      g_print (_("usage: %s <device|uuid|bus device> <mountpoint>\n"), g_basename (argv[0]));
	return 1;
    }

    xml = NULL;
    if (*argv[1] == '/') {
	/* assume it's a device name */
#ifdef HAVE_SGUTILS
	xml = read_sysinfo_extended (argv[1]);
#else
	g_warning ("Compiled without sgutils support, can't read SysInfoExtended from a device");
#endif
    }
    else if (argc == 4) {
#ifdef HAVE_LIBUSB
        int bus_number;
        int device_number;
        /* 2 arguments in addition to the mountpoint, attempt to parse them
         * as an USB bus number/device number (useful for Nano5G for
         * example)
         */
        bus_number = atoi (argv[1]);
        device_number = atoi (argv[2]);
        xml = read_sysinfo_extended_from_usb (bus_number, device_number);
#else
	g_warning ("Compiled without libusb support, can't read SysInfoExtended through raw USB calls");
#endif
    }
    else {
	/* argument doesn't look like a filename, might be an UUID */
#ifdef HAVE_LIBIMOBILEDEVICE
	xml = read_sysinfo_extended_by_uuid (argv[1]);
#else
	g_warning ("Compiled without libimobiledevice support, can't read SysInfoExtended from an iPhone UUID");
#endif
    }

    if (xml == NULL) {
      g_print (_("Couldn't read xml sysinfo from %s\n"), argv[1]);
      return 1;
    } else {
	const char *mountpoint = argv[argc-1];
        gboolean success;

        success = write_sysinfo_extended (mountpoint, xml);
        g_free (xml);
        if (!success) {
            g_print (_("Couldn't write SysInfoExtended to %s"), mountpoint);
            return 1;
        }
    }

    return 0;
}

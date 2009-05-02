/*
|   Copyright (C) 2007 Christophe Fergeau  <teuf@gnome.org>
|
|   This program is free software; you can redistribute it and/or modify
|   it under the terms of the GNU General Public License as published by
|   the Free Software Foundation; either version 2 of the License, or
|   (at your option) any later version.
|
|   This program is distributed in the hope that it will be useful,
|   but WITHOUT ANY WARRANTY; without even the implied warranty of
|   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
|   GNU General Public License for more details.
|
|   You should have received a copy of the GNU General Public License
|   along with this program; if not, write to the Free Software
|   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
|
|  iTunes and iPod are trademarks of Apple
|
|  This product is not supported/written/published by Apple!
|
*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <libintl.h>

#include <glib-object.h>

#include "itdb.h"

int
main (int argc, char *argv[])
{
    Itdb_Device *device;
    char *fwid;

    if (argc < 2) {
        g_print ("Usage: %s <mountpoint>\n", g_basename (argv[0]));
        return 1;

    }
 
    g_type_init ();

    device = itdb_device_new ();
    if (device == NULL) {
        return 1;
    }

    itdb_device_set_mountpoint (device, argv[1]);

    fwid = itdb_device_get_sysinfo (device, "FirewireGuid");
    if (fwid == NULL) {
        g_print ("Couldn't find firewire ID\n");
        return 1;
    } else {
	g_print ("FireWire ID: %s\n", fwid);
    }

    return 0;
}

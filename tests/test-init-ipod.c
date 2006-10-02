/* Copyright (c) 2006, Jorg Schuler
 * <jcsjcs at users dot sourceforge dot net>
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
 * $Id$
 *
 */

#include "itdb.h"

#include <locale.h>
#include <glib-object.h>
#include <glib/gi18n.h>


static void usage (const char *argv0)
{
    g_print ("Usage: %s mountpoint [modelnumber]\n", argv0);
    g_print ("This test program will create the standard directories on your iPod\n");
    g_print ("including empty iTunesDB and ArtworkDB.\n\n");
    g_print ("Valid model numbers are listed in itdb_device.c, e.g. 'MA350' for a\n");
    g_print ("1 GB iPod nano\n\n");
    g_print ("Often the model number can be found in <mountpoint>/iTunes/Device/SysInfo,\n");
    g_print ("which is written automatically by most iPods when you reset it.\n\n");
    g_print ("If you omit the model number, this program will try to detect the model\n");
    g_print ("number and print it.\n");
}


int
main (int argc, char **argv)
{
    if ((argc < 2) || (argc > 3))
    {
	usage (argv[0]);
        return 1;
    }
    setlocale (LC_ALL, "");
    g_type_init ();

    if (argc == 3)
    {
	GError *error = NULL;

	if (!itdb_init_ipod (argv[1], argv[2], "iPod", &error))
	{
	    if (error)
	    {
		g_print (_("Error initialising iPod: %s\n"),
			 error->message);
		g_error_free (error);
		error = NULL;
	    }
	    else
	    {
		g_print (_("Error initialising iPod, unknown error\n"));
	    }
	}
    }
    if (argc == 2)
    {
	gchar *model_num;
	Itdb_Device *device = itdb_device_new ();
	itdb_device_set_mountpoint (device, argv[1]);

	model_num = itdb_device_get_sysinfo (device, "ModelNumStr");

	if (model_num)
	{
	    g_print ("Your iPod model number seems to be '%s'.\n",
		     model_num);
	}
	else
	{
	    g_print ("Your iPod model number could not be detected.\n");
	    g_print ("Maybe you have an iPod Shuffle? In that case specify  'M9724' (512 MB)\n");
	    g_print ("or 'M9725' (1 GB) to setup your iPod.\n");
	    g_print ("If you have a mobile phone, specify 'Mmobile1'.\n");
	    g_print ("Otherwise look up the model number on the ipod package or in\n");
	    g_print ("itdb_device.c (prepend 'M').\n");
	}
	itdb_device_free (device);
    }
    return 0;
}

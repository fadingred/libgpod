/* Copyright (c) 2007, Christophe Fergeau  <teuf@gnome.org>
 * Part of the libgpod project.
 * 
 * URL: http://www.gtkpod.org/
 * URL: http://gtkpod.sourceforge.net/
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

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <scsi/sg_cmds.h>
#include <sys/stat.h>

#include <glib.h>

#include "itdb.h"


extern char *read_sysinfo_extended (const char *device);
/* do_sg_inquiry and read_sysinfo_extended were heavily
 * inspired from libipoddevice
 */
static unsigned char
do_sg_inquiry (int fd, int page, char *buf, char **start)
{
    const unsigned int IPOD_BUF_LENGTH = 252;

    if (sg_ll_inquiry (fd, 0, 1, page, buf, IPOD_BUF_LENGTH, 1, 0) != 0) {
	*start = NULL;
	return 0;
    } else {
	*start = buf + 4;
	return (unsigned char)buf[3];
    }
}

G_GNUC_INTERNAL char *
read_sysinfo_extended (const char *device)
{
    int fd;
    const unsigned int IPOD_XML_PAGE = 0xc0;
    unsigned char len;
    char buf[512];
    char *start;
    char *offsets;
    GString *xml_sysinfo;
    unsigned int i;

    fd = open (device, O_RDWR);
    if (fd < 0) {
	return NULL;
    }

    len = do_sg_inquiry (fd, IPOD_XML_PAGE, buf, &start);
    if (start == NULL || len == 0) {
	close(fd);
	return NULL;
    }

    offsets = g_new (char, len+1);
    memcpy(offsets, start, len);
    offsets[len] = 0;

    xml_sysinfo = g_string_new_len (NULL, 512);
    if (xml_sysinfo == NULL) {
	g_free (offsets);
	close (fd);
	return NULL;
    }

    for (i = 0; offsets[i]; i++) {
	bzero(buf, 512);
	len = do_sg_inquiry (fd, offsets[i], buf, &start);
	start[len] = 0;
	g_string_append (xml_sysinfo, start);
    }
    
    g_free (offsets);
    close (fd);

    return g_string_free (xml_sysinfo, FALSE);
}



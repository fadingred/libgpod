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
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <scsi/sg_cmds.h>
#include <sys/stat.h>
#include <time.h>

#include <glib.h>

extern void sync_time (const char *device, struct tm *tm);
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

    fd = open (device, O_RDONLY);
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

static void do_sg_write_buffer (const char *device, void *buffer, size_t len)
{
    int fd;
    guint i;

    fd = open (device, O_RDWR);
    if (fd < 0) {
	g_warning ("Couldn't open device %s", device);
	return;
    }

    g_print ("    Data Payload: ");
    for (i = 0; i < len; i++) {
	g_print ("%02x ", *((guchar *)buffer+i));
    }
    g_print ("\n");
    if (sg_ll_write_buffer (fd, 1, 0, 0x0c0000, buffer, len, 1, 1) != 0) {
	g_print ("Error sending data\n");
    }
    close(fd);
}

G_GNUC_INTERNAL void sync_time (const char *device, struct tm *tm)
{
    struct iPodTime {
	guint16 year;
	guint16 days;
	guchar timezone;
	guchar hour;
	guchar minute;
	guchar second;
	guchar dst;
	guchar padding[3];
    } __attribute__((__packed__));
    struct iPodTime ipod_time;

    if (tm == NULL) {
	time_t current_time;
	current_time = time (NULL);
	tm = localtime (&current_time);
    }

    ipod_time.year = GUINT16_TO_BE (1900+tm->tm_year);
    ipod_time.days = GUINT16_TO_BE (tm->tm_yday);
    /* the timezone value is the shift east of UTC in 15 min chunks
     * so eg. UTC+2 is 2*4 = 8
     */
    ipod_time.timezone = tm->tm_gmtoff / 900;
    ipod_time.hour = tm->tm_hour;
    ipod_time.minute = tm->tm_min;
    ipod_time.second = tm->tm_sec;
    if (tm->tm_isdst) {
	ipod_time.dst = 1;
    } else {
	ipod_time.dst = 0;
    }
    memset (ipod_time.padding, 0, sizeof (ipod_time.padding));

    do_sg_write_buffer (device, &ipod_time, sizeof (ipod_time));
}

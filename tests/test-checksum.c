/*
|  Copyright (C) 2007 Christophe Fergeau <teuf@gnome.org>
|
|  The code in this file is heavily based on the proof-of-concept code
|  written by wtbw
|
| Redistribution and use in source and binary forms, with or without
| modification, are permitted provided that the following conditions are met:
|
|   1. Redistributions of source code must retain the above copyright
| notice, this list of conditions and the following disclaimer.
|   2. Redistributions in binary form must reproduce the above copyright
| notice, this list of conditions and the following disclaimer in the
| documentation and/or other materials provided with the distribution.
|   3. The name of the author may not be used to endorse or promote
| products derived from this software without specific prior written
| permission.
|
| THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
| IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
| OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
| IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
| INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
| BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
| OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
| ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
| OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
| OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
| OF SUCH DAMAGE.
|
|
|  iTunes and iPod are trademarks of Apple
|
|  This product is not supported/written/published by Apple!
*/
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <glib.h>

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include "itdb.h"
#include "itdb_sha1.h"

static unsigned char *
calculate_db_checksum (const char *itdb_path, guint64 fwid)
{
    int fd;
    struct stat stat_buf;
    int result;
    unsigned char *itdb_data;
    unsigned char *checksum;

    fd = open (itdb_path, O_RDONLY);
    if (fd < 0) {
	g_warning ("Couldn't open %s", itdb_path);
	return NULL;
    }

    result = fstat (fd, &stat_buf);
    if (result != 0) {
	g_warning ("Couldn't stat %s", itdb_path);
	close (fd);
	return NULL;
    }

    if (stat_buf.st_size < 0x80) {
	g_warning ("%s is too small", itdb_path);
	close (fd);
	return NULL;
    }

    itdb_data = mmap (NULL, stat_buf.st_size, 
		      PROT_READ | PROT_WRITE, 
		      MAP_PRIVATE, fd, 0);
    if (itdb_data == MAP_FAILED) {
	g_warning ("Couldn't mmap %s", itdb_path);
	close (fd);
	return NULL;
    }

    /* Those fields must be zero'ed out for the sha1 calculation */
    memset(itdb_data+0x18, 0, 8);
    memset(itdb_data+0x32, 0, 20);
    memset(itdb_data+0x58, 0, 20);

    checksum = itdb_compute_hash (fwid, itdb_data, stat_buf.st_size, NULL);

    munmap (itdb_data, stat_buf.st_size);
    close (fd);

    return checksum;
}

static gboolean write_checksum_to_file (const char *path, 
					const unsigned char *checksum,
					size_t size)
{
    FILE *f;
    int result;
    size_t count;

    f = fopen (path, "rb+");
    if (f == NULL) {
	return FALSE;
    }
    
    result = fseek (f, 0x58, SEEK_SET);
    if (result != 0) {
	fclose (f);
	return FALSE;
    }
    
    count = fwrite (checksum, size, 1, f);
    fclose (f);
   
    return (count == 1);
}

int
main (int argc, char *argv[])
{
    char *itdb_path;
    guint64 firewire_id;
    unsigned char *checksum;
    int i;

    if (argc < 3) {
	g_print ("Usage: %s <mountpoint> <firewire_id> [write]\n"
		 "       firewire_id in hexadecimal, e.g. 00A745....\n"
		 "       If any third argument is given, the checksum is written back to the iTunesDB.\n",
		 g_basename(argv[0]));
	exit (0);
    }
    
    errno = 0;
    firewire_id = strtoull (argv[2], NULL, 16);
    if (errno != 0) {
	g_warning ("Couldn't parse %s as a firewire ID\n", argv[2]);
	exit (0);	
    }

    itdb_path = itdb_get_itunesdb_path (argv[1]);
    checksum = calculate_db_checksum (itdb_path, firewire_id);

    if (argc >= 4)
    {
	gboolean success;

	success = write_checksum_to_file (itdb_path, checksum, 20);
	if (!success)
	{
	    g_warning ("Couldn't write checksum back to file '%s'\n",
		       argv[1]);
	}
    }

    g_free (itdb_path);
    
    if (checksum == NULL) {
	g_warning ("Couldn't compute checksum");
	exit (0);
    }

    for (i = 0; i < strlen ((char *)checksum); i++) {
	g_print ("%02x ", checksum[i]);
    }
    g_print ("\n");
    g_free (checksum);

    return 0;
}

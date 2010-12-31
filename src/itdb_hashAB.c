/*
|  Copyright (c) 2010 Nikias Bassen <nikias@gmx.li>
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
|  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
|  USA
|
|  iTunes and iPod are trademarks of Apple
|
|  This product is not supported/written/published by Apple!
*/

#if HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "rijndael.h"

#include <glib/gstdio.h>

#include <gmodule.h>

#include "itdb_device.h"
#include "db-itunes-parser.h"
#include "itdb_private.h"

typedef void (*calcHashAB_t)(unsigned char target[57], const unsigned char sha1[20], const unsigned char uuid[20], const unsigned char rnd_bytes[23]);
static calcHashAB_t calc_hashAB = NULL;

static gboolean load_libhashab()
{
	gchar *path;
	GModule *handle;

	if (!g_module_supported()) {
		return FALSE;
	}
	path = g_module_build_path(LIBGPOD_BLOB_DIR, "hashab");
	handle = g_module_open(path, G_MODULE_BIND_LAZY);
	g_free(path);
	if (!handle) {
		return FALSE;
	}

	if (!g_module_symbol(handle, "calcHashAB", (void**)&calc_hashAB)) {
		g_module_close(handle);
		g_warning("symbol 'calcHashAB' not found");
		return FALSE;
	}
	g_module_make_resident(handle);

	printf("***** hashAB support successfully loaded *****\n");

	return TRUE;
}

static void itdb_hashAB_compute_itunesdb_sha1 (unsigned char *itdb_data, 
					       gsize itdb_len,
					       unsigned char sha1[20])
{
    guchar backup18[8];
    guchar backup32[20];
    guchar hashAB[57];
    gsize sha1_len;
    GChecksum *checksum;
    MhbdHeader *header;

    g_assert (itdb_len >= 0x6c);

    header = (MhbdHeader *)itdb_data;
    g_assert (strncmp (header->header_id, "mhbd", strlen ("mhbd")) == 0);
    memcpy (backup18, &header->db_id, sizeof (backup18));
    memcpy (backup32, &header->unk_0x32, sizeof (backup32));
    memcpy (hashAB, &header->hashAB, sizeof (hashAB));

    /* Those fields must be zero'ed out for the sha1 calculation */
    memset(&header->db_id, 0, sizeof (header->db_id));

    memset(&header->hash58, 0, sizeof (header->hash58));
    memset(&header->hash72, 0, sizeof (header->hash72));
    memset(&header->hashAB, 0, sizeof (header->hashAB));

    sha1_len = g_checksum_type_get_length (G_CHECKSUM_SHA1);
    checksum = g_checksum_new (G_CHECKSUM_SHA1);
    g_checksum_update (checksum, itdb_data, itdb_len);
    g_checksum_get_digest (checksum, sha1, &sha1_len);
    g_checksum_free (checksum);

    memcpy (&header->db_id, backup18, sizeof (backup18));
    memcpy (&header->unk_0x32, backup32, sizeof (backup32));
}

gboolean itdb_hashAB_compute_hash_for_sha1 (const Itdb_Device *device,
					    const guchar sha1[20],
					    guchar signature[57],
					    GError **error)
{
    unsigned char uuid[20];
    unsigned char rndpart[23] = "ABCDEFGHIJKLMNOPQRSTUVW";

    if (calc_hashAB == NULL) {
	if (!load_libhashab()) {
	    g_set_error (error, 0, -1, "Unsupported checksum type");
	    return FALSE;
	}
    }

    if (!itdb_device_get_hex_uuid(device, uuid)) return FALSE;

    calc_hashAB(signature, sha1, uuid, rndpart);

    return TRUE;
}

gboolean itdb_hashAB_write_hash (const Itdb_Device *device,
				 unsigned char *itdb_data,
				 gsize itdb_len,
				 GError **error)
{
    guchar sha1[20];
    MhbdHeader *header;

    if (itdb_len < 0xF4) {
	g_set_error (error, 0, -1, "iTunesDB file too small to write checksum");
	return FALSE;
    }

    header = (MhbdHeader *)itdb_data;
    header->hashing_scheme = GUINT16_FROM_LE (ITDB_CHECKSUM_HASHAB);
    itdb_hashAB_compute_itunesdb_sha1 (itdb_data, itdb_len, sha1);
    return itdb_hashAB_compute_hash_for_sha1 (device, sha1, header->hashAB, error);
}

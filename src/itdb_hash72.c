/*
|  Copyright (c) 2009 Chris Lee <clee@mg8.org>
|  Copyright (C) 2009 Christophe Fergeau <cfergeau@mandriva.com>
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

#include "itdb_device.h"
#include "db-itunes-parser.h"
#include "itdb_private.h"

static const uint8_t AES_KEY[16] = { 0x61, 0x8c, 0xa1, 0x0d, 0xc7, 0xf5, 0x7f, 0xd3, 0xb4, 0x72, 0x3e, 0x08, 0x15, 0x74, 0x63, 0xd7 };

/*
| hash_generate and hash_extract are :
| Copyright (c) 2009 Chris Lee <clee@mg8.org>
| licensed under the terms of the WTFPL <http://sam.zoy.org/wtfpl/>
*/
/* Generate a signature for an iTunesDB or a cbk file using the file SHA1
 * and a (IV, random bytes) for this device we want to sign for
 */
static void hash_generate(uint8_t signature[46],
			  const uint8_t sha1[20],
			  const uint8_t iv[16], 
			  const uint8_t random_bytes[12])
{
	uint8_t output[32] = { 0 }, plaintext[32] = { 0 };
	memcpy(plaintext, sha1, 20);
	memcpy(&plaintext[20], random_bytes, 12);

	signature[0] = 0x01;
	signature[1] = 0x00;
	memcpy(&signature[2], random_bytes, 12);

	aes_set_key((uint8_t *)AES_KEY);
	aes_encrypt(iv, plaintext, output, 32);

	memcpy(&signature[14], output, 32);
}

/* Given a valid signature and SHA1 for a file, extracts a valid (IV,
 * random bytes) couple which can be used to sign any iTunesDB or cbk file
 * for the device the signature was made for
 */
static int hash_extract(const uint8_t signature[46],
			const uint8_t sha1[20],
			uint8_t iv[16], 
			uint8_t random_bytes[12])
{
	uint8_t plaintext[32] = { 0 }, output[32] = { 0 };

	if (signature[0] != 0x01 || signature[1] != 0x00) {
		fprintf(stderr, "Invalid signature prefix!\n");
		return -1;
	}

	memcpy(plaintext, sha1, 20);
	memcpy(&plaintext[20], &signature[2], 12);

	memcpy(output, plaintext, 32);

	aes_set_key((uint8_t *)AES_KEY);
	aes_decrypt(plaintext, (uint8_t *)&signature[14], output, 16);

	if (memcmp(&plaintext[16], &output[16], 16)) {
		fprintf(stderr, "uh oh\n");
		return -1;
	}

	memcpy(iv, output, 16);
	memcpy(random_bytes, &signature[2], 12);

	return 0;
}

static int ord_from_hex_char(const char c)
{
  if ('0' <= c && c <= '9')
    return c - '0';
  else if ('a' <= c && c <= 'f')
    return 10 + (c - 'a');
  else if ('A' <= c && c <= 'F')
    return 10 + (c - 'A');
  else
    return -1;
}

static int string_to_hex(unsigned char *dest, const int array_size,
			 const char *s)
{
  /* skip optional '0x' prefix */
  if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))
    s += 2;

  if (strlen(s) > array_size*2)
    return -8;

  do {
    int low, high;
    if((high = ord_from_hex_char(s[0])) == -1 ||
       (low = ord_from_hex_char(s[1])) == -1)
      return -9;
    *dest++ = high<<4 | low;
  } while(*(s+=2));
  return 0;
}

static gboolean get_uuid (const Itdb_Device *device, unsigned char uuid[20])
{
    const char *uuid_str;
    int result;

    uuid_str = itdb_device_get_uuid (device);
    if (uuid_str == NULL) {
	uuid_str = itdb_device_get_firewire_id (device);
    }
    if (uuid_str == NULL) {
	return FALSE;
    }
    memset (uuid, 0, 20);
    result = string_to_hex (uuid, 20, uuid_str);

    return (result == 0);
}

struct Hash78Info {
    unsigned char header[6];
    unsigned char uuid[20];
    unsigned char rndpart[12];
    unsigned char iv[16];
} __attribute__((__packed__));

static char *get_hash_info_path (const Itdb_Device *device)
{
    char *filename;
    char *device_dir;

    device_dir = itdb_get_device_dir (device->mountpoint);
    filename = g_build_filename (device_dir, "HashInfo", NULL);
    g_free (device_dir);

    return filename;
}

static gboolean write_hash_info (const Itdb_Device *device,
				 unsigned char iv[16],
				 unsigned char rndpart[12])
{
    struct Hash78Info hash_info;
    char *filename;
    gboolean success;
    const char header[] = "HASHv0";

    memcpy (hash_info.header, header, sizeof (header));
    success = get_uuid (device, hash_info.uuid);
    if (!success) {
	return FALSE;
    }
    memcpy (hash_info.rndpart, rndpart, sizeof (hash_info.rndpart));
    memcpy (hash_info.iv, iv, sizeof (hash_info.iv));

    filename = get_hash_info_path (device);
    success = g_file_set_contents (filename, (void *)&hash_info,
				   sizeof (hash_info), NULL);
    g_free (filename);
    
    return success;
}

static struct Hash78Info *read_hash_info (const Itdb_Device *device)
{
    char *filename;
    gsize len;
    gboolean read_ok;
    unsigned char uuid[20];
    struct Hash78Info *info;

    if (!get_uuid (device, uuid)) {
	return NULL;
    }

    filename = get_hash_info_path (device);

    read_ok = g_file_get_contents (filename, (gchar**)&info, &len, NULL);
    g_free (filename);
    if (!read_ok) {
	return NULL;
    }
    if (len != sizeof (*info)) {
	g_free (info);
	return NULL;
    }
    if (memcmp (info->uuid, uuid, sizeof (uuid)) != 0) {
	/* the HashInfo file seems invalid for this device, just remove it
	 */
	filename = get_hash_info_path (device);
	g_unlink (filename);
	g_free (filename);
	g_free (info);
	return NULL;
    }

    return info;
}

static void itdb_hash72_compute_itunesdb_sha1 (unsigned char *itdb_data, 
					       gsize itdb_len,
					       unsigned char sha1[20])
{
    guchar backup18[8];
    guchar backup32[20];
    guchar hash72[46];
    gsize sha1_len;
    GChecksum *checksum;
    MhbdHeader *header;

    g_assert (itdb_len >= 0x6c);

    header = (MhbdHeader *)itdb_data;
    g_assert (strncmp (header->header_id, "mhbd", strlen ("mhbd")) == 0);
    memcpy (backup18, &header->db_id, sizeof (backup18));
    memcpy (backup32, &header->unk_0x32, sizeof (backup32));
    memcpy (hash72, &header->hash72, sizeof (hash72));

    /* Those fields must be zero'ed out for the sha1 calculation */
    memset(&header->db_id, 0, sizeof (header->db_id));

    memset(&header->hash58, 0, sizeof (header->hash58));
    memset(&header->hash72, 0, sizeof (header->hash72));

    sha1_len = g_checksum_type_get_length (G_CHECKSUM_SHA1);
    checksum = g_checksum_new (G_CHECKSUM_SHA1);
    g_checksum_update (checksum, itdb_data, itdb_len);
    g_checksum_get_digest (checksum, sha1, &sha1_len);
    g_checksum_free (checksum);

    memcpy (&header->db_id, backup18, sizeof (backup18));
    memcpy (&header->unk_0x32, backup32, sizeof (backup32));
}

gboolean itdb_hash72_extract_hash_info (const Itdb_Device *device, 
					unsigned char *itdb_data, 
					gsize itdb_len)
{
    guchar hash72[46];
    guchar sha1[20];
    guchar iv[16];
    guchar random_bytes[12];
    MhbdHeader *header;
    int iv_extracted;
    struct Hash78Info *hash_info;

    if ((itdb_device_get_checksum_type (device) != ITDB_CHECKSUM_HASH72)
		    || (!itdb_device_supports_sqlite_db (device))) {
	    /* No need to generate a HashInfo file */
	    return FALSE;
    }
    if (itdb_len < 0x6c) {
	return FALSE;
    }

    hash_info = read_hash_info (device);
    g_free (hash_info);
    if (hash_info != NULL) {
	/* We already have what we need to generate signatures for this device,
	 * no need to go on
	 */
	return TRUE;
    }

    header = (MhbdHeader *)itdb_data;
    g_assert (strncmp (header->header_id, "mhbd", strlen ("mhbd")) == 0);
    memcpy (hash72, &header->hash72, sizeof (hash72));

    itdb_hash72_compute_itunesdb_sha1 (itdb_data, itdb_len, sha1);

    iv_extracted = hash_extract (hash72, sha1, iv, random_bytes);
    if (iv_extracted != 0) {
	return FALSE;
    }

    return write_hash_info (device, iv, random_bytes);
}

gboolean itdb_hash72_compute_hash_for_sha1 (const Itdb_Device *device, 
					    const guchar sha1[20],
					    guchar signature[46],
                                            GError **error)
{
    struct Hash78Info *hash_info;

    hash_info = read_hash_info (device);
    if (hash_info == NULL) {
        if (error != NULL) {
            g_set_error (error, ITDB_FILE_ERROR, ITDB_FILE_ERROR_NOTFOUND,
                         "Can't write iPod database because of missing HashInfo file");
        }
	return FALSE;
    }
    hash_generate (signature, sha1, hash_info->iv, hash_info->rndpart);
    g_free (hash_info);

    return TRUE;
}

gboolean itdb_hash72_write_hash (const Itdb_Device *device, 
				 unsigned char *itdb_data, 
				 gsize itdb_len,
				 GError **error)
{
    guchar sha1[20];
    MhbdHeader *header;

    if (itdb_len < 0x6c) {
	g_set_error (error, 0, -1, "iTunesDB file too small to write checksum");
	return FALSE;
    }

    header = (MhbdHeader *)itdb_data;
    header->hashing_scheme = GUINT16_FROM_LE (ITDB_CHECKSUM_HASH72);
    itdb_hash72_compute_itunesdb_sha1 (itdb_data, itdb_len, sha1);
    return itdb_hash72_compute_hash_for_sha1 (device, sha1, header->hash72, error);
}

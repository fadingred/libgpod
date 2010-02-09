/*
 *  Copyright (C) 2005-2007 Christophe Fergeau
 *
 *
 *  The code contained in this file is free software; you can redistribute
 *  it and/or modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either version
 *  2.1 of the License, or (at your option) any later version.
 *
 *  This file is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this code; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  iTunes and iPod are trademarks of Apple
 *
 *  This product is not supported/written/published by Apple!
 *
 */

#include <config.h>
#include "itdb.h"
#include "itdb_device.h"
#include "itdb_private.h"
#include "db-artwork-parser.h"

#if HAVE_GDKPIXBUF
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "db-artwork-debug.h"
#include "db-itunes-parser.h"
#include "db-image-parser.h"
#include "itdb_endianness.h"

#include <glib/gstdio.h>

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <sys/stat.h>
#include <stdio.h>
#include <sys/types.h>

#ifdef WITH_INTERNAL_GCHECKSUM
#include "gchecksum.h"
#endif

#define DEFAULT_GSTRING_SIZE 128*1024

struct iPodSharedDataBuffer {
	GString *data;
	char *filename;
	int ref_count;
};

struct _iPodBuffer {
	struct iPodSharedDataBuffer *shared;
	off_t offset;
	guint byte_order;
	DbType db_type;
};

typedef struct _iPodBuffer iPodBuffer;

static gboolean
ipod_gstring_flush (struct iPodSharedDataBuffer *shared, GError **error)
{
	gboolean success;

	success = g_file_set_contents (shared->filename, 
	 	 		       shared->data->str, shared->data->len,
				       error);
	if (!success) {
                return FALSE;
	}
	g_string_free (shared->data, TRUE);
	g_free (shared->filename);
	g_free (shared);

	return TRUE;
}

static void
ipod_buffer_destroy (iPodBuffer *buffer)
{
	buffer->shared->ref_count--;
	if (buffer->shared->ref_count == 0) {
		ipod_gstring_flush (buffer->shared, NULL);
	}
	g_free (buffer);
}


static void *
ipod_buffer_get_pointer (iPodBuffer *buffer)
{
	if (buffer->shared->data->str == NULL) {
		return NULL;
	}
	g_assert (buffer->offset < buffer->shared->data->len);
	return &((unsigned char *)buffer->shared->data->str)[buffer->offset];
}

static void
ipod_buffer_maybe_grow (iPodBuffer *buffer, off_t size)
{
	g_string_set_size (buffer->shared->data, 
			   buffer->shared->data->len + size);
}

static iPodBuffer *
ipod_buffer_get_sub_buffer (iPodBuffer *buffer, off_t offset)
{
	iPodBuffer *sub_buffer;

	g_assert (buffer->offset + offset <= buffer->shared->data->len);

	sub_buffer = g_new0 (iPodBuffer, 1);
	if (sub_buffer == NULL) {
		return NULL;
	}
	sub_buffer->shared = buffer->shared;
	sub_buffer->offset = buffer->offset + offset;
	sub_buffer->byte_order = buffer->byte_order;
	sub_buffer->db_type = buffer->db_type;

	buffer->shared->ref_count++;

	return sub_buffer;
}

static iPodBuffer *
ipod_buffer_new (const char *filename, guint byte_order, DbType db_type)
{
	struct iPodSharedDataBuffer *shared;
	iPodBuffer *buffer;

	shared = g_new0 (struct iPodSharedDataBuffer, 1);
	if (shared == NULL) {
		return NULL;
	}
	shared->filename = g_strdup (filename);
	shared->data = g_string_sized_new (DEFAULT_GSTRING_SIZE);
	shared->ref_count = 1;

	buffer = g_new0 (iPodBuffer, 1);
	if (buffer == NULL) {
		g_free (shared->filename);
		g_string_free (shared->data, TRUE);
		g_free (shared);
		return NULL;
	}
	buffer->shared = shared;
	buffer->byte_order = byte_order;
	buffer->db_type = db_type;

	return buffer;
}

enum MhsdType {
	MHSD_TYPE_MHLI = 1,
	MHSD_TYPE_MHLA = 2,
	MHSD_TYPE_MHLF = 3
};

#define RETURN_SIZE_FOR(id, size) if (strncmp (id, header_id, 4) == 0) return (size)



/* Returns the "real" size for a header, ie the size iTunes uses for it
 * (padding included)
 */
static int
get_padded_header_size (gchar header_id[4])
{
	RETURN_SIZE_FOR ("mhni", 0x4c);
	RETURN_SIZE_FOR ("mhii", 0x98);
	RETURN_SIZE_FOR ("mhsd", 0x60);
	RETURN_SIZE_FOR ("mhfd", 0x84);
	RETURN_SIZE_FOR ("mhli", 0x5c);
	RETURN_SIZE_FOR ("mhla", 0x5c);
	RETURN_SIZE_FOR ("mhlf", 0x5c);
	RETURN_SIZE_FOR ("mhif", 0x7c);
	RETURN_SIZE_FOR ("mhba", 0x94);

	return 0;
}

static void *
init_header (iPodBuffer *buffer, gchar _header_id[4], guint header_len)
{
	MHeader *mh;
	int padded_size;
	gchar *header_id;

	padded_size = get_padded_header_size (_header_id);
	if (padded_size != 0) {
		header_len = padded_size;
	}
	g_assert (header_len > sizeof (MHeader));
	ipod_buffer_maybe_grow (buffer, header_len);

	mh = (MHeader*)ipod_buffer_get_pointer (buffer);
	if (mh == NULL) {
		return NULL;
	}
	memset (mh, 0, header_len);

	header_id = g_strndup (_header_id, 4);
	if (buffer->byte_order == G_BIG_ENDIAN) {
		g_strreverse (header_id);
	}
	strncpy ((char *)mh->header_id, header_id, 4);
	mh->header_len = get_gint32 (header_len, buffer->byte_order);

	g_free (header_id);
	return mh;
}


static int
write_mhod_type_1 (gchar *string, iPodBuffer *buffer)
{
	ArtworkDB_MhodHeaderString *mhod;
	unsigned int total_bytes;
	int len;
	int padding;

	g_assert (string != NULL);

	total_bytes = sizeof (ArtworkDB_MhodHeaderString);
	mhod = (ArtworkDB_MhodHeaderString *)init_header (buffer, "mhod",
                                                          total_bytes);
	if (mhod == NULL) {
		return -1;
	}
	mhod->total_len = get_gint32 (total_bytes, buffer->byte_order);
	/* Modify header length, since iTunes only puts the length of
	 * MhodHeader in header_len
	 */
	mhod->header_len = get_gint32 (sizeof (ArtworkDB_MhodHeader), buffer->byte_order);
	mhod->encoding = get_gint32 (0x01, buffer->byte_order);
	len = strlen (string);
	mhod->string_len = get_gint32 (len, buffer->byte_order);

	padding = 4 - ( (total_bytes + len) % 4 );
	if (padding == 4)
	    padding = 0;
	mhod->padding_len = padding;
	mhod->type = get_gint16 (0x01, buffer->byte_order);

	/* Make sure we have enough free space to write the string */
	ipod_buffer_maybe_grow (buffer, len + padding);
	mhod = ipod_buffer_get_pointer (buffer);
	if (mhod == NULL) {
		return -1;
	}
	memcpy (mhod->string, string, len);
	total_bytes += len + padding;
	mhod->total_len = get_gint32 (total_bytes, buffer->byte_order);

	dump_mhod_string (mhod);

	return total_bytes;
}

static int
write_mhod_type_3 (gchar *string, iPodBuffer *buffer)
{
	ArtworkDB_MhodHeaderString *mhod;
	unsigned int total_bytes;
	glong len;
	const gint g2l = sizeof (gunichar2);
	gunichar2 *utf16, *strp;
	int i, padding;

	g_assert (string != NULL);

	total_bytes = sizeof (ArtworkDB_MhodHeaderString);
	mhod = (ArtworkDB_MhodHeaderString *) init_header (buffer, "mhod", 
                                                           total_bytes);
	if (mhod == NULL) {
		return -1;
	}
	mhod->total_len = get_gint32 (total_bytes, buffer->byte_order);
	/* Modify header length, since iTunes only puts the length of
	 * MhodHeader in header_len
	 */
	mhod->header_len = get_gint32 (sizeof (ArtworkDB_MhodHeader),
				       buffer->byte_order);
	mhod->type = get_gint16 (3, buffer->byte_order);

	/* FIXME: Tidy this up, combine cases more */
	/* Some magic: endianess-reversed (BE) mobile phones use UTF8
	 * (version 1) with padding, standard iPods (LE) use UTF16
	 * (version 2).*/
	switch (buffer->byte_order)
	{
	case G_LITTLE_ENDIAN:
	    utf16 = g_utf8_to_utf16 (string, -1, NULL, &len, NULL);
	    if (utf16 == NULL) {
		return -1;
	    }

	    mhod->encoding = 2; /* 8 bit field, no need to byteswap */

	    /* number of bytes of the string encoded in UTF-16 */
	    mhod->string_len = get_gint32 (g2l * len, buffer->byte_order);
	    padding = 4 - ( (total_bytes + g2l*len) % 4 );
	    if (padding == 4)
		padding = 0;
	    mhod->padding_len = padding; /* 8 bit field, no need to byteswap */

 	    total_bytes += g2l*len + padding;

	    /* Make sure we have enough free space to write the string */
	    ipod_buffer_maybe_grow (buffer, g2l*len + padding);
	    mhod = ipod_buffer_get_pointer (buffer);
	    if (mhod == NULL) {
		    g_free (utf16);
		    return  -1;
	    }
	    strp = (gunichar2 *)mhod->string;
	    for (i = 0; i < len; i++) {
		strp[i] = get_gint16 (utf16[i], buffer->byte_order);
	    }
	    g_free (utf16);
	    memset (mhod->string + g2l*len, 0, padding);
	    break;
	case G_BIG_ENDIAN:
	    mhod->encoding = 1; /* 8 bit field, no need to byteswap */
            /* FIXME: len isn't initialized */
	    mhod->string_len = get_gint32 (len, buffer->byte_order);
	    /* pad string if necessary */
	    /* e.g. len = 7 bytes, len%4 = 3, 4-3=1 -> requires 1 byte
	       padding */
	    padding = 4 - ( (total_bytes + len) % 4 );
	    if (padding == 4)
		padding = 0;
	    mhod->padding_len = padding; /* 8 bit field, no need to byteswap */

	    /* Make sure we have enough free space to write the string */
	    ipod_buffer_maybe_grow (buffer, len+padding);
	    mhod = ipod_buffer_get_pointer (buffer);
	    if (mhod == NULL) {
		    return -1;
	    }
	    memcpy (mhod->string, string, len);
	    memset (mhod->string + len, 0, padding);
	    total_bytes += (len+padding);
	}
	mhod->total_len = get_gint32 (total_bytes, buffer->byte_order);

	dump_mhod_string (mhod);

	return total_bytes;
}

static int
write_mhni (Itdb_DB *db, Itdb_Thumb_Ipod_Item *item, iPodBuffer *buffer)
{
	MhniHeader *mhni;
	unsigned int total_bytes;
	int bytes_written;
	iPodBuffer *sub_buffer;
        const Itdb_ArtworkFormat *format;

	if (item == NULL) {
		return -1;
	}

	mhni = (MhniHeader *)init_header (buffer, "mhni",
					  sizeof (MhniHeader));
	if (mhni == NULL) {
		return -1;
	}
	total_bytes =          get_gint32 (mhni->header_len,
					   buffer->byte_order);
	mhni->total_len =      get_gint32 (total_bytes,
					   buffer->byte_order);
        format = item->format;
	mhni->format_id = get_gint32 (format->format_id, buffer->byte_order);
	mhni->image_width =    get_gint16 (item->width, buffer->byte_order);
	mhni->image_height =   get_gint16 (item->height, buffer->byte_order);
	mhni->image_size =     get_gint32 (item->size, buffer->byte_order);
	mhni->ithmb_offset =   get_gint32 (item->offset, buffer->byte_order);
	mhni->vertical_padding = get_gint16 (item->vertical_padding,
					     buffer->byte_order);
	mhni->horizontal_padding = get_gint16 (item->horizontal_padding,
					       buffer->byte_order);

	sub_buffer = ipod_buffer_get_sub_buffer (buffer, total_bytes);
	if (sub_buffer == NULL) {
		return  -1;
	}
	bytes_written = write_mhod_type_3 (item->filename, sub_buffer);
	ipod_buffer_destroy (sub_buffer);
	if (bytes_written == -1) {
		return -1;
	}
	total_bytes += bytes_written;
	mhni = ipod_buffer_get_pointer (buffer);
	mhni->total_len = get_gint32 (total_bytes, buffer->byte_order);
	/* Only update number of children when all went well to try to get
	 * something somewhat consistent when there are errors
	 */
	mhni->num_children = get_gint32 (1, buffer->byte_order);

	dump_mhni (mhni);

	return total_bytes;
}

static int
write_mhod (Itdb_DB *db, Itdb_Thumb_Ipod_Item *thumb, iPodBuffer *buffer)
{
	ArtworkDB_MhodHeader *mhod;
	unsigned int total_bytes;
	int bytes_written;
	iPodBuffer *sub_buffer;

	if (thumb == NULL) {
		return -1;
	}

	mhod = (ArtworkDB_MhodHeader *)
	    init_header (buffer, "mhod",
			 sizeof (ArtworkDB_MhodHeader));
	if (mhod == NULL) {
		return -1;
	}
	total_bytes = sizeof (ArtworkDB_MhodHeader);
	mhod->total_len = get_gint32 (total_bytes, buffer->byte_order);
	mhod->type = get_gint16 (MHOD_TYPE_LOCATION, buffer->byte_order);
	sub_buffer = ipod_buffer_get_sub_buffer (buffer, total_bytes);
	if (sub_buffer == NULL) {
		return -1;
	}
	bytes_written = write_mhni (db, thumb, sub_buffer);
	ipod_buffer_destroy (sub_buffer);
	if (bytes_written == -1) {
		return -1;
	}
	total_bytes += bytes_written;
	mhod = ipod_buffer_get_pointer (buffer);
	mhod->total_len = get_gint32 (total_bytes, buffer->byte_order);

	dump_mhod (mhod);

	return total_bytes;
}

static int
write_mhii (Itdb_DB *db, void *data, iPodBuffer *buffer)
{
	MhiiHeader *mhii;
	unsigned int total_bytes;
	int bytes_written;
	int num_children;
	const GList *it = NULL;
	Itdb_Track *song;
	Itdb_Artwork *artwork;
	guint64 mactime;
	Itdb_Device *device = db_get_device (db);

	mhii = (MhiiHeader *)init_header (buffer, "mhii", sizeof (MhiiHeader));
	if (mhii == NULL) {
		return -1;
	}
	total_bytes = get_gint32 (mhii->header_len, buffer->byte_order);

	switch( buffer->db_type) {
	case DB_TYPE_ITUNES:
		song = (Itdb_Track *)data;
		artwork = song->artwork;
		mhii->song_id = get_gint64 (song->dbid, buffer->byte_order);
		break;
	case DB_TYPE_PHOTO:
		artwork = (Itdb_Artwork *)data;
		mhii->song_id = get_gint64 (artwork->id + 2, buffer->byte_order);
		break;
	default:
	        g_return_val_if_reached (-1);
	}
	mhii->image_id = get_guint32 (artwork->id, buffer->byte_order);
	mhii->unknown4 = get_gint32 (artwork->unk028, buffer->byte_order);
	mhii->rating = get_gint32 (artwork->rating, buffer->byte_order);
	mhii->unknown6 = get_gint32 (artwork->unk036, buffer->byte_order);

	mactime = device_time_time_t_to_mac (device, artwork->creation_date);
	mhii->orig_date = get_guint32 (mactime, buffer->byte_order);

	mactime = device_time_time_t_to_mac (device, artwork->digitized_date);
	mhii->digitized_date = get_guint32 (mactime, buffer->byte_order);

	mhii->orig_img_size = get_gint32 (artwork->artwork_size, buffer->byte_order);
	num_children = 0;
        /* Before trying to write the artwork or photo database, the ithmb
         * files have been written, which will have converted all thumbnails 
         * attached to the tracks to ITDB_THUMB_TYPE_IPOD thumbnails.
         */
        g_assert (artwork->thumbnail->data_type == ITDB_THUMB_TYPE_IPOD);
	for (it=itdb_thumb_ipod_get_thumbs ((Itdb_Thumb_Ipod *)artwork->thumbnail); 
             it!=NULL; 
             it=it->next)
	{
		iPodBuffer *sub_buffer;
		Itdb_Thumb_Ipod_Item *thumb;

                thumb = (Itdb_Thumb_Ipod_Item *)it->data;
                if (thumb->format == NULL) {
		    /* skip this thumb */
		    continue;
		}

		mhii->num_children = get_gint32 (num_children,
						 buffer->byte_order);
		mhii->total_len = get_gint32 (total_bytes, buffer->byte_order);
		sub_buffer = ipod_buffer_get_sub_buffer (buffer, total_bytes);
		if (sub_buffer == NULL) {
			return -1;
		}
		bytes_written = write_mhod (db, thumb, sub_buffer);
		ipod_buffer_destroy (sub_buffer);
		if (bytes_written == -1) {
			return -1;
		}
		total_bytes += bytes_written;
		mhii = ipod_buffer_get_pointer (buffer);
		num_children++;
	}

	mhii->num_children = get_gint32 (num_children, buffer->byte_order);
	mhii->total_len = get_gint32 (total_bytes, buffer->byte_order);

	dump_mhii (mhii);

	return total_bytes;
}

static int
write_mhli (Itdb_DB *db, iPodBuffer *buffer )
{
	GList *it = NULL;
	MhliHeader *mhli;
	unsigned int total_bytes;
	int num_thumbs;

	mhli = (MhliHeader *)init_header (buffer, "mhli", sizeof (MhliHeader));
	if (mhli == NULL) {
		return -1;
	}

	num_thumbs = 0;
	total_bytes = get_gint32 (mhli->header_len, buffer->byte_order);
	switch (buffer->db_type) {
	case DB_TYPE_PHOTO:
		it = db_get_photodb(db)->photos;
		break;
	case DB_TYPE_ITUNES:
		it = db_get_itunesdb(db)->tracks;
		break;
	default:
	        g_return_val_if_reached (-1);
	}
	while (it != NULL) {
		Itdb_Track *song;
		int bytes_written;
		iPodBuffer *sub_buffer;
		if (buffer->db_type == DB_TYPE_ITUNES) {
			song = (Itdb_Track*)it->data;
			if (!song->artwork->thumbnail || (song->artwork->dbid == 0)) {
				it = it->next;
				continue;
			}
		}
		sub_buffer = ipod_buffer_get_sub_buffer (buffer, total_bytes);
		if (sub_buffer == NULL) {
			break;
		}
		bytes_written = write_mhii (db, it->data, sub_buffer);
		ipod_buffer_destroy (sub_buffer);
		if (bytes_written != -1) {
			num_thumbs++;
			total_bytes += bytes_written;
		}
		it = it->next;
	}
	mhli = ipod_buffer_get_pointer (buffer);
	mhli->num_children = get_gint32 (num_thumbs, buffer->byte_order);
	dump_mhl ((MhlHeader *)mhli, "mhli");

	return total_bytes;
}

static int
write_mhia (gint image_id, iPodBuffer *buffer)
{
	MhiaHeader *mhia;
	unsigned int total_bytes;


	mhia = (MhiaHeader *)init_header (buffer, "mhia", 40);
	if (mhia == NULL) {
		return -1;
	}

	mhia->total_len = mhia->header_len;
	mhia->image_id = get_gint32 (image_id, buffer->byte_order);
	total_bytes = get_gint32 (mhia->header_len, buffer->byte_order);
	dump_mhia( mhia );
	return total_bytes;
}

static int
write_mhba (Itdb_PhotoAlbum *album, iPodBuffer *buffer)
{
	GList *it;
	MhbaHeader *mhba;
	iPodBuffer *sub_buffer;
	unsigned int total_bytes;
	unsigned int bytes_written;

	mhba = (MhbaHeader *)init_header (buffer, "mhba", sizeof (MhbaHeader));
	if (mhba == NULL) {
		return -1;
	}
	mhba->num_mhods = get_gint32(1, buffer->byte_order);
	mhba->num_mhias = get_gint32(g_list_length (album->members),
				     buffer->byte_order);
	mhba->album_id = get_gint32(album->album_id, buffer->byte_order);
	mhba->unk024 = get_gint32(album->unk024, buffer->byte_order);
	mhba->unk028 = get_gint16(album->unk028, buffer->byte_order);
	mhba->album_type = album->album_type;
	mhba->playmusic = album->playmusic;
	mhba->repeat = album->repeat;
	mhba->random = album->random;
	mhba->show_titles = album->show_titles;
	mhba->transition_direction = album->transition_direction;
	mhba->slide_duration = get_gint32(album->slide_duration,
					  buffer->byte_order);
	mhba->transition_duration = get_gint32(album->transition_duration,
					       buffer->byte_order);
	mhba->unk044 = get_gint32(album->unk044, buffer->byte_order);
	mhba->unk048 = get_gint32(album->unk048, buffer->byte_order);
	mhba->song_id = get_gint64(album->song_id, buffer->byte_order);
	mhba->prev_album_id = get_gint32(album->prev_album_id,
					 buffer->byte_order);

	total_bytes = get_gint32 (mhba->header_len, buffer->byte_order);

	/* FIXME: Write other mhods */
	/* Write album title */
	sub_buffer = ipod_buffer_get_sub_buffer (buffer, total_bytes);
	if (sub_buffer == NULL) {
	    return -1;
	}
	bytes_written = write_mhod_type_1 (album->name, sub_buffer);
	ipod_buffer_destroy (sub_buffer);
	if (bytes_written == -1) {
	    return -1;
	}
	total_bytes += bytes_written;

	for (it = album->members; it != NULL; it = it->next) {
	        Itdb_Artwork *photo = it->data;
		g_return_val_if_fail (photo, -1);

		sub_buffer = ipod_buffer_get_sub_buffer (buffer, total_bytes);
		if (sub_buffer == NULL) {
		    return -1;
		}
		bytes_written = write_mhia (photo->id, sub_buffer);
		ipod_buffer_destroy (sub_buffer);
		if (bytes_written == -1) {
		    return -1;
		}
		total_bytes += bytes_written;
	}
	mhba = ipod_buffer_get_pointer (buffer);
	mhba->total_len = get_gint32( total_bytes, buffer->byte_order );
	dump_mhba ( mhba );
	return total_bytes;
}

static int
write_mhla (Itdb_DB *db, iPodBuffer *buffer)
{
	GList *it;
	MhlaHeader *mhla;
	iPodBuffer *sub_buffer;
	unsigned int total_bytes;

	mhla = (MhlaHeader *)init_header (buffer, "mhla", sizeof (MhlaHeader));
	if (mhla == NULL) {
		return -1;
	}
	total_bytes = get_gint32 (mhla->header_len, buffer->byte_order);
	if (buffer->db_type == DB_TYPE_PHOTO) {
	    unsigned int bytes_written;
            unsigned int num_children = 0;
	    for (it = db_get_photodb(db)->photoalbums; it != NULL; it = it->next) {
		Itdb_PhotoAlbum *album = (Itdb_PhotoAlbum *)it->data;

		sub_buffer = ipod_buffer_get_sub_buffer (buffer, total_bytes);
		if (sub_buffer == NULL) {
		    return -1;
		}
		bytes_written = write_mhba (album, sub_buffer);
		ipod_buffer_destroy (sub_buffer);
		if (bytes_written == -1) {
		    return -1;
		}
		total_bytes += bytes_written;
		mhla = ipod_buffer_get_pointer (buffer);
		num_children++;
		mhla->num_children = get_gint32 (num_children,
                                                 buffer->byte_order);
	    }
	}

	dump_mhl ((MhlHeader *)mhla, "mhla");

	return total_bytes;
}

static int
write_mhif (Itdb_DB *db, iPodBuffer *buffer,
            const Itdb_ArtworkFormat *img_info)
{
	MhifHeader *mhif;

	mhif = (MhifHeader *)init_header (buffer, "mhif", sizeof (MhifHeader));
	if (mhif == NULL) {
		return -1;
	}
	mhif->total_len = mhif->header_len;

	mhif->format_id = get_gint32 (img_info->format_id,
		               	      buffer->byte_order);
	mhif->image_size = get_gint32 (img_info->height * img_info->width * 2,
				       buffer->byte_order);

	dump_mhif (mhif);

	return get_gint32 (mhif->header_len, buffer->byte_order);
}

static int
write_mhlf (Itdb_DB *db, iPodBuffer *buffer)
{
	MhlfHeader *mhlf;
	unsigned int total_bytes;
	int bytes_written;
        GList *formats; 
        GList *it;
        unsigned int num_children;

	mhlf = (MhlfHeader *)init_header (buffer, "mhlf", sizeof (MhlfHeader));
	if (mhlf == NULL) {
		return -1;
	}

	total_bytes = get_gint32 (mhlf->header_len, buffer->byte_order);
        num_children = 0;
        mhlf->num_files = get_gint32 (num_children, buffer->byte_order);

        formats = NULL;
        switch (buffer->db_type) {
        case DB_TYPE_ITUNES:
            formats = itdb_device_get_cover_art_formats(db_get_device(db));
            break;
	case DB_TYPE_PHOTO:
            formats = itdb_device_get_photo_formats(db_get_device(db));
            break;
        }
        if (formats == NULL) {
                return total_bytes;
        }

        for (it = formats; it != NULL; it = it->next) {
                const Itdb_ArtworkFormat *format;
	        iPodBuffer *sub_buffer;
                
                format = (const Itdb_ArtworkFormat *)it->data;
        	sub_buffer = ipod_buffer_get_sub_buffer (buffer, total_bytes);
        	if (sub_buffer == NULL) {
                        g_list_free (formats);
        		return -1;
        	}

        	bytes_written = write_mhif (db, sub_buffer, format);
	        			    
        	ipod_buffer_destroy (sub_buffer);
        	if (bytes_written == -1) {
        		return -1;
        	}
        	total_bytes += bytes_written;
		mhlf = ipod_buffer_get_pointer (buffer);

                num_children++;
        	/* Only update number of children when all went well to try 
                 * to get something somewhat consistent when there are errors
        	 */
        	mhlf->num_files = get_gint32 (num_children, buffer->byte_order);
        }
	dump_mhl ((MhlHeader *)mhlf, "mhlf");
        g_list_free (formats);

	return total_bytes;
}


static int
write_mhsd (Itdb_DB *db, iPodBuffer *buffer, enum MhsdType type)
{
	ArtworkDB_MhsdHeader *mhsd;
	unsigned int total_bytes;
	int bytes_written;
	iPodBuffer *sub_buffer;

	g_assert (type >= MHSD_TYPE_MHLI);
	g_assert (type <= MHSD_TYPE_MHLF);
	mhsd = (ArtworkDB_MhsdHeader *)init_header (buffer, "mhsd", sizeof (ArtworkDB_MhsdHeader));
	if (mhsd == NULL) {
		return -1;
	}
	total_bytes = get_gint32 (mhsd->header_len, buffer->byte_order);
	mhsd->total_len = get_gint32 (total_bytes, buffer->byte_order);
	mhsd->index = get_gint16 (type, buffer->byte_order);
	bytes_written = -1;

	sub_buffer = ipod_buffer_get_sub_buffer (buffer, total_bytes);
	if (sub_buffer == NULL) {
		return -1;
	}
	switch (type) {
	case MHSD_TYPE_MHLI:
		bytes_written = write_mhli (db, sub_buffer);
		break;
	case MHSD_TYPE_MHLA:
		bytes_written = write_mhla (db, sub_buffer);
		break;
	case MHSD_TYPE_MHLF:
		bytes_written = write_mhlf (db, sub_buffer);
		break;
	}
	ipod_buffer_destroy (sub_buffer);
	if (bytes_written == -1) {
		return -1;
	} else {
		total_bytes += bytes_written;
		mhsd = ipod_buffer_get_pointer (buffer);
		mhsd->total_len = get_gint32 (total_bytes, buffer->byte_order);
	}

	dump_mhsd (mhsd);

	return total_bytes;
}

static int
write_mhfd (Itdb_DB *db, iPodBuffer *buffer, int id_max)
{
	MhfdHeader *mhfd;
	unsigned int total_bytes;
	int bytes_written;
	int i;

	
	mhfd = (MhfdHeader *)init_header (buffer, "mhfd", sizeof (MhfdHeader));
	if (mhfd == NULL) {
		return -1;
	}
	total_bytes = get_gint32 (mhfd->header_len, buffer->byte_order);
	mhfd->total_len = get_gint32 (total_bytes, buffer->byte_order);
	switch (buffer->db_type) {
	case DB_TYPE_PHOTO:
		mhfd->unknown2 = get_gint32 (2, buffer->byte_order);
		break;
	case DB_TYPE_ITUNES:
		mhfd->unknown2 = get_gint32 (2, buffer->byte_order);
		break;
	}
	mhfd->next_id = get_gint32 (id_max, buffer->byte_order);
	mhfd->unknown_flag1 = 2;
	for (i = 1 ; i <= 3; i++) {
		iPodBuffer *sub_buffer;

		sub_buffer = ipod_buffer_get_sub_buffer (buffer, total_bytes);
		if (sub_buffer == NULL) {
			continue;
		}
		bytes_written = write_mhsd (db, sub_buffer, i);
		ipod_buffer_destroy (sub_buffer);
		if (bytes_written == -1) {
			return -1;
		}
		total_bytes += bytes_written;
		mhfd = ipod_buffer_get_pointer (buffer);
		mhfd->total_len = get_gint32 (total_bytes, buffer->byte_order);
		mhfd->num_children = get_gint32 (i, buffer->byte_order);
	}

	dump_mhfd (mhfd);

	return total_bytes;
}

/* renumber the artwork IDs for all tracks containing artwork and with
   an ID of != 0 */
/* if the iPod does not support sparse artwork, renumber consecutively
   and all artwork */
/* returns the highest assigned ID or 0 if no IDs were used */
static guint32
ipod_artwork_db_set_ids (Itdb_iTunesDB *db)
{
    GList *gl;
    const guint32 min_id = 0x64;
    guint32 cur_id;

    cur_id = min_id;

    if (itdb_device_supports_sparse_artwork (db->device))
    {
	GHashTable *id_hash;

	id_hash = g_hash_table_new (g_direct_hash, g_direct_equal);

	for (gl = db->tracks; gl != NULL; gl = gl->next)
	{
	    Itdb_Track *song;
	    Itdb_Artwork *artwork;

	    song = gl->data;
	    g_return_val_if_fail (song, -1);

	    artwork = song->artwork;
	    g_return_val_if_fail (artwork, -1);

	    if (itdb_track_has_thumbnails (song) && (artwork->id != 0))
	    {
		gpointer orig_key;
		gpointer orig_val;

		if (g_hash_table_lookup_extended (id_hash, GINT_TO_POINTER (artwork->id),
						  &orig_key, &orig_val))
		{   /* ID was encountered before */
		    artwork->id = GPOINTER_TO_INT (orig_val);
		    artwork->dbid = 0;
		}
		else
		{   /* first time we see this ID */
		    g_hash_table_insert (id_hash, GINT_TO_POINTER (artwork->id),
					 GINT_TO_POINTER (cur_id));
		    artwork->id = cur_id++;
		    artwork->dbid = song->dbid;
		}
		song->mhii_link = artwork->id;
	    }
	    else
	    {
		song->mhii_link = 0;
	    }
	}
	g_hash_table_destroy (id_hash);
    }
    else 
    {   /* iPod does not support sparse artwork -- just renumber */
	for (gl = db->tracks; gl != NULL; gl = gl->next)
	{
	    Itdb_Track *song;

	    song = gl->data;
	    g_return_val_if_fail (song, -1);
	    g_return_val_if_fail (song->artwork, -1);

	    song->mhii_link = 0;

	    if (itdb_track_has_thumbnails (song))
	    {
		song->artwork->id = cur_id++;
		song->artwork->dbid = song->dbid;
	    }
	    song->mhii_link = song->artwork->id;
	}
    }	

    if (cur_id == min_id)
	return 0;
    else
	return cur_id-1;
}



/* for all tracks with new artwork (id == 0) do the following:
   - try to identify if this artwork is identical to previous artwork
     (within the same album)
   - if no: assign new ID
   - if yes: assign same ID

   Returns the highest ID used.
*/
static guint32
ipod_artwork_mark_new_doubles (Itdb_iTunesDB *itdb, guint max_id)
{
    GList *gl;
    GHashTable *hash_file, *hash_memory, *hash_pixbuf;

    hash_file =   g_hash_table_new_full (g_str_hash, g_str_equal,
					 g_free, NULL);
    hash_memory = g_hash_table_new_full (g_str_hash, g_str_equal,
					 g_free, NULL);
    hash_pixbuf = g_hash_table_new_full (g_str_hash, g_str_equal,
					 g_free, NULL);

    for (gl=itdb->tracks; gl; gl=gl->next)
    {
	Itdb_Artwork *artwork;
	Itdb_Track *track;

	track= gl->data;
	g_return_val_if_fail (track, max_id);
	artwork = track->artwork;
	g_return_val_if_fail (artwork, max_id);

	if ((artwork->id == 0) && itdb_track_has_thumbnails (track))
	{
	    const gchar *checkstring;
	    GHashTable *hash=NULL;
	    gpointer orig_val = NULL;
	    Itdb_Thumb *thumb = artwork->thumbnail;
	    GChecksum *checksum = g_checksum_new (G_CHECKSUM_SHA1);

	    /* use the album name as part of the checksum */
	    if (track->album && *track->album)
	    {
		g_checksum_update (checksum,
				   (guchar *)track->album, strlen (track->album));
	    }

	    switch (thumb->data_type)
	    {
	    case ITDB_THUMB_TYPE_MEMORY:
	    {
		Itdb_Thumb_Memory *mthumb = (Itdb_Thumb_Memory *)thumb;
		g_checksum_update (checksum,
				   mthumb->image_data, mthumb->image_data_len);
		hash = hash_memory;
		break;
	    }
	    case ITDB_THUMB_TYPE_PIXBUF:
	    {
		Itdb_Thumb_Pixbuf *pthumb = (Itdb_Thumb_Pixbuf *)thumb;
		g_return_val_if_fail (pthumb->pixbuf, max_id);
		g_checksum_update (checksum,
				   gdk_pixbuf_get_pixels (pthumb->pixbuf),
				   gdk_pixbuf_get_height (pthumb->pixbuf) * gdk_pixbuf_get_rowstride (pthumb->pixbuf));
		hash = hash_pixbuf;
		break;
	    }
	    case ITDB_THUMB_TYPE_FILE:
	    {
		Itdb_Thumb_File *fthumb = (Itdb_Thumb_File *)thumb;
		g_return_val_if_fail (fthumb->filename, max_id);
		g_checksum_update (checksum,
				   (guchar *)fthumb->filename,
				   strlen (fthumb->filename));
		hash = hash_file;
		break;
	    }
	    case ITDB_THUMB_TYPE_INVALID:
		/* programming error */
		g_print ("encountered invalid thumb.\n");
		g_return_val_if_reached (max_id);
		break;
	    case ITDB_THUMB_TYPE_IPOD:
		/* thumbs on the iPod are definitely not expected to
		   have an ID of 0 */
		g_print ("encountered iPod thumb with ID = 0.\n");
		g_return_val_if_reached (max_id);
		break;
	    }

	    checkstring = g_checksum_get_string (checksum);
	    if (g_hash_table_lookup_extended (hash, checkstring, NULL, &orig_val))
	    {   /* same artwork was used before */
		Itdb_Artwork *previous_artwork = orig_val;
		artwork->id = previous_artwork->id;
		artwork->dbid = 0;
	    }
	    else
	    {   /* first occurence of this artwork */
		artwork->id = ++max_id;
		artwork->dbid = track->dbid;
		g_hash_table_insert (hash, g_strdup (checkstring), artwork);
	    }
	    track->mhii_link = artwork->id;
	    g_checksum_free (checksum);
	}
    }

    g_hash_table_destroy (hash_memory);
    g_hash_table_destroy (hash_file);
    g_hash_table_destroy (hash_pixbuf);

    return max_id;
}


/* returns the highest ID used */
static guint32 itdb_prepare_thumbnails (Itdb_iTunesDB *itdb)
{
    gint max_id;

    /* first renumber the old thumbnails to make sure we don't get too
     * high */
    max_id = ipod_artwork_db_set_ids (itdb);

    if (itdb_device_supports_sparse_artwork (itdb->device))
    {
	/* go through all newly added artwork and pass out new IDs. the
	   same ID will be assigned to identical artwork within one album */
	max_id = ipod_artwork_mark_new_doubles (itdb, max_id);

	/* set the IDs again to make sure they are in the right order */
	max_id = ipod_artwork_db_set_ids (itdb);
    }

    return max_id;
}




G_GNUC_INTERNAL int
ipod_write_artwork_db (Itdb_iTunesDB *itdb)
{
	iPodBuffer *buf;
	int bytes_written;
	char *filename;
	int id_max;
	Itdb_DB db;
	int status;

	db.db_type = DB_TYPE_ITUNES;
	db.db.itdb = itdb;

	id_max = itdb_prepare_thumbnails (itdb);

	/* First, let's write the .ithmb files, this will create the
	 * various thumbnails as well */

	status = itdb_write_ithumb_files (&db);
	if (status != 0) {
		return -1;
	}

	filename = ipod_db_get_artwork_db_path (itdb_get_mountpoint (itdb));
	if (filename == NULL) {
		/* FIXME: the iTunesDB will be inconsistent wrt artwork_count
		 * it might be better to 0 out this field in all tracks
		 * when we encounter an error
		 */
		return -1;
	}
	buf = ipod_buffer_new (filename, itdb->device->byte_order, DB_TYPE_ITUNES);
	if (buf == NULL) {
		g_print ("Couldn't create %s\n", filename);
	        g_free (filename);
		return -1;
	}
	bytes_written = write_mhfd (&db, buf, id_max);

	/* Refcount of the shared buffer should drop to 0 and this should
	 * sync buffered data to disk
	 */
	ipod_buffer_destroy (buf);

	if (bytes_written == -1) {
		g_print ("Failed to save %s\n", filename);
		/* FIXME: maybe should unlink the file we may have created */
		g_free (filename);
		return -1;
	}
	g_free (filename);
	return 0;
}

int
ipod_write_photo_db (Itdb_PhotoDB *photodb)
{
	iPodBuffer *buf;
	int bytes_written;
	char *filename;
	int id_max;
	Itdb_DB db;
	int status;

	db.db_type = DB_TYPE_PHOTO;
	db.db.photodb = photodb;

	filename = ipod_db_get_photos_db_path (db_get_mountpoint (&db));

	status = itdb_write_ithumb_files (&db);
	if (status != 0) {
		return -1;
	}

	if (filename == NULL) {
		return -1;
	}
	buf = ipod_buffer_new (filename, photodb->device->byte_order, DB_TYPE_PHOTO);
	if (buf == NULL) {
		g_print ("Couldn't create %s\n", filename);
		g_free (filename);
		return -1;
	}
	id_max = itdb_get_max_photo_id( photodb );
	bytes_written = write_mhfd (&db, buf, id_max+1);

	/* Refcount of the shared buffer should drop to 0 and this should
	 * sync buffered data to disk
	 */
	ipod_buffer_destroy (buf);

	if (bytes_written == -1) {
		g_print ("Failed to save %s\n", filename);
		/* FIXME: maybe should unlink the file we may have created */
	        g_free (filename);
		return -1;
	}
        g_free (filename);

	return 0;
}
#else
G_GNUC_INTERNAL int
ipod_write_artwork_db (Itdb_iTunesDB *itdb)
{
    return -1;
}

int
ipod_write_photo_db (Itdb_PhotoDB *photodb)
{
    return -1;
}
#endif

/*
 *  Copyright (C) 2005 Christophe Fergeau
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
#include "itdb_private.h"
#include "db-artwork-parser.h"

#if HAVE_GDKPIXBUF

#include "db-artwork-debug.h"
#include "db-itunes-parser.h"
#include "db-image-parser.h"
#include "itdb_endianness.h"

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <stdio.h>
#include <sys/types.h>

/* FIXME: Writing aborts if a file exceeds the following size because
   the mremap() further down will fail (at least most of the time on
   most systems). As a workaround the size was increased from 2 MB to
   16 MB. */
#define IPOD_MMAP_SIZE 16 * 1024 * 1024

struct iPodMmapBuffer {
	int fd;
	void *mmap_area;
	size_t size;
	int ref_count;
};

struct _iPodBuffer {
	struct iPodMmapBuffer *mmap;
	off_t offset;
	guint byte_order;
	DbType db_type;
};

typedef struct _iPodBuffer iPodBuffer;

static void
ipod_mmap_buffer_destroy (struct iPodMmapBuffer *buf)
{
	munmap (buf->mmap_area, buf->size);
	close (buf->fd);
	g_free (buf);
}

static void
ipod_buffer_destroy (iPodBuffer *buffer)
{
	buffer->mmap->ref_count--;
	if (buffer->mmap->ref_count == 0) {
		ipod_mmap_buffer_destroy (buffer->mmap);
	}
	g_free (buffer);
}


static void *
ipod_buffer_get_pointer (iPodBuffer *buffer) 
{
	return &((unsigned char *)buffer->mmap->mmap_area)[buffer->offset];
}

static int
ipod_buffer_grow_file (struct iPodMmapBuffer *mmap_buf, off_t new_size)
{
	int result;

	result = lseek (mmap_buf->fd, new_size, SEEK_SET);
	if (result == (off_t)-1) {
		g_print ("Failed to grow file to %lu: %s\n", 
                        (unsigned long)new_size, strerror (errno));
		return -1;
	}
	result = 0;
	result = write (mmap_buf->fd, &result, 1);
	if (result != 1) {
		g_print ("Failed to write a byte at %lu: %s\n", 
                        (unsigned long)new_size, strerror (errno));
		return -1;
	}

	return 0;
}


static void * 
ipod_buffer_grow_mapping (iPodBuffer *buffer, size_t size)
{
	void *new_address;
#ifdef HAVE_MREMAP
	
	new_address = mremap (buffer->mmap->mmap_area, buffer->mmap->size,
			      buffer->mmap->size + size, 0);
#else
	munmap (buffer->mmap->mmap_area, buffer->mmap->size);
	new_address = mmap (buffer->mmap->mmap_area, buffer->mmap->size + size,
			    PROT_READ | PROT_WRITE, MAP_SHARED, 
			    buffer->mmap->fd, 0);
#endif
	/* Don't allow libc to move the current mapping since this would
	 * force us to be very careful wrt pointers in the rest of the code
	 */
	if (new_address != buffer->mmap->mmap_area) {
		return MAP_FAILED;
	}

	return new_address;
}


static int
ipod_buffer_maybe_grow (iPodBuffer *buffer, off_t offset)
{
	void *new_address;

	if (buffer->offset + offset <= buffer->mmap->size) {
		return 0;
	}

	new_address = ipod_buffer_grow_mapping (buffer, IPOD_MMAP_SIZE);
	if (new_address == MAP_FAILED) {
		g_print ("Failed to mremap buffer\n");
		return -1;
	}

	if (ipod_buffer_grow_file (buffer->mmap, 
				   buffer->mmap->size + IPOD_MMAP_SIZE) != 0) {
		return -1;
	}
	buffer->mmap->size += IPOD_MMAP_SIZE;

	return 0;
}

static iPodBuffer *
ipod_buffer_get_sub_buffer (iPodBuffer *buffer, off_t offset)
{
	iPodBuffer *sub_buffer;

	if (ipod_buffer_maybe_grow (buffer, offset) != 0) {
		return NULL;
	}
	sub_buffer = g_new0 (iPodBuffer, 1);
	if (sub_buffer == NULL) {
		return NULL;
	}
	sub_buffer->mmap = buffer->mmap;
	sub_buffer->offset = buffer->offset + offset;
	sub_buffer->byte_order = buffer->byte_order;
	sub_buffer->db_type = buffer->db_type;

	buffer->mmap->ref_count++;

	return sub_buffer;
}

static iPodBuffer *
ipod_buffer_new (const char *filename, guint byte_order, DbType db_type)
{
	int fd;
	struct iPodMmapBuffer *mmap_buf;
	iPodBuffer *buffer;
	void *mmap_area;

	fd = open (filename, O_RDWR | O_CREAT | O_TRUNC, 
		   S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	if (fd == -1) {
		g_print ("Failed to open %s: %s\n", 
			 filename, strerror (errno));
		return NULL;
	}
	
	mmap_area = mmap (0, IPOD_MMAP_SIZE, PROT_READ | PROT_WRITE,
			  MAP_SHARED, fd, 0);
	if (mmap_area == MAP_FAILED) {
		g_print ("Failed to mmap %s in memory: %s\n", filename, 
			 strerror (errno));
		close (fd);
		return NULL;
	}
	mmap_buf = g_new0 (struct iPodMmapBuffer, 1);
	if (mmap_buf == NULL) {
		munmap (mmap_area, IPOD_MMAP_SIZE);
		close (fd);
		return NULL;
	}
	mmap_buf->mmap_area = mmap_area;
	mmap_buf->size = IPOD_MMAP_SIZE;
	mmap_buf->ref_count = 1;
	mmap_buf->fd = fd;

	if (ipod_buffer_grow_file (mmap_buf, IPOD_MMAP_SIZE) != 0) {
		ipod_mmap_buffer_destroy (mmap_buf);
		return  NULL;
	}

	buffer = g_new0 (iPodBuffer, 1);
	if (buffer == NULL) {
		ipod_mmap_buffer_destroy (mmap_buf);
		return NULL;
	}
	buffer->mmap = mmap_buf;
	buffer->byte_order = byte_order;
	buffer->db_type = db_type;

	return buffer;
}

enum MhsdType {
	MHSD_TYPE_MHLI = 1,
	MHSD_TYPE_MHLA = 2,
	MHSD_TYPE_MHLF = 3
};

enum iPodThumbnailType {
	IPOD_THUMBNAIL_FULL_SIZE = 0,
	IPOD_THUMBNAIL_NOW_PLAYING = 1,
	IPOD_PHOTO_LITTLE = 3,
	IPOD_PHOTO_FULL_SIZE = 4
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
	if (ipod_buffer_maybe_grow (buffer, header_len) != 0) {
		return NULL;
	}
	mh = (MHeader*)ipod_buffer_get_pointer (buffer);
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
	MhodHeaderArtworkType1 *mhod;
	unsigned int total_bytes;
	int len;
	int padding;

	g_assert (string != NULL);

	mhod = (MhodHeaderArtworkType1 *)init_header (buffer, "mhod", 
						      sizeof (MhodHeaderArtworkType1));
	if (mhod == NULL) {
		return -1;
	}
	total_bytes = sizeof (MhodHeaderArtworkType1);
	mhod->total_len = get_gint32 (total_bytes, buffer->byte_order);
	/* Modify header length, since iTunes only puts the length of 
	 * MhodHeader in header_len
	 */
	mhod->header_len = get_gint32 (sizeof (ArtworkDB_MhodHeader), buffer->byte_order);
	mhod->unknown3 = get_gint32 (0x01, buffer->byte_order);
	len = strlen (string);
	mhod->string_len = get_gint32 (len, buffer->byte_order);

	padding = 4 - ( (total_bytes + len) % 4 );
	mhod->padding = padding;
	mhod->type = get_gint32 (0x0001, buffer->byte_order);

	/* Make sure we have enough free space to write the string */
	if (ipod_buffer_maybe_grow (buffer, total_bytes + len + padding ) != 0) {
		return  -1;
	}
	memcpy (mhod->string, string, len);
	total_bytes += len + padding;
	mhod->total_len = get_gint32 (total_bytes, buffer->byte_order);	

	dump_mhod_type_1 (mhod);

	return total_bytes;
}

static int 
write_mhod_type_3 (gchar *string, iPodBuffer *buffer)
{
	ArtworkDB_MhodHeaderArtworkType3 *mhod;
	unsigned int total_bytes;
	int len;
	gunichar2 *utf16, *strp;
	int i, padding;

	g_assert (string != NULL);

	mhod = (ArtworkDB_MhodHeaderArtworkType3 *)
	    init_header (buffer, "mhod", 
			 sizeof (ArtworkDB_MhodHeaderArtworkType3));
	if (mhod == NULL) {
		return -1;
	}
	total_bytes = sizeof (ArtworkDB_MhodHeaderArtworkType3);
	mhod->total_len = get_gint32 (total_bytes, buffer->byte_order);
	/* Modify header length, since iTunes only puts the length of 
	 * MhodHeader in header_len
	 */
	mhod->header_len = get_gint32 (sizeof (ArtworkDB_MhodHeader), 
				       buffer->byte_order);
	mhod->type = get_gint16 (3, buffer->byte_order);

	len = strlen (string);

	/* FIXME: Tidy this up, combine cases more */
	/* Some magic: endianess-reversed (BE) mobile phones use UTF8
	 * (version 1) with padding, standard iPods (LE) use UTF16
	 * (version 2).*/
	switch (buffer->byte_order)
	{
	case G_LITTLE_ENDIAN:
	    mhod->mhod_version = 2;
	    /* number of bytes of the string encoded in UTF-16 */
	    mhod->string_len = get_gint32 (2*len, buffer->byte_order);
	    padding = 4 - ( (total_bytes + 2*len) % 4 );
	    mhod->padding = padding; /* high byte is padding length (0-3) */

	    /* Make sure we have enough free space to write the string */
	    if (ipod_buffer_maybe_grow (buffer, total_bytes + 2*len+padding) != 0) {
		return  -1;
	    }

	    utf16 = g_utf8_to_utf16 (string, -1, NULL, NULL, NULL);
	    if (utf16 == NULL) {		
		return -1;
	    }
	    strp = (gunichar2 *)mhod->string;
	    memcpy (strp, utf16, 2*len);
	    g_free (utf16);
	    for (i = 0; i < len; i++) {
		strp[i] = get_gint16 (strp[i], buffer->byte_order);
	    }
	    total_bytes += 2*len+padding;
	    break;
	case G_BIG_ENDIAN:
	    mhod->mhod_version = 1;
	    mhod->string_len = get_gint32 (len, buffer->byte_order);
	    /* pad string if necessary */
	    /* e.g. len = 7 bytes, len%4 = 3, 4-3=1 -> requires 1 byte
	       padding */
	    padding = 4 - ( (total_bytes + len) % 4 );
	    mhod->padding = padding; /* high byte is padding length (0-3) */
	    /* Make sure we have enough free space to write the string */
	    if (ipod_buffer_maybe_grow (buffer, total_bytes + 2*len+padding) != 0) {
		return  -1;
	    }
	    memcpy (mhod->string, string, len);
	    total_bytes += (len+padding);
	}
	mhod->total_len = get_gint32 (total_bytes, buffer->byte_order);

	dump_mhod_type_3 (mhod);

	return total_bytes;
}

static int 
write_mhni (Itdb_DB *db, Itdb_Thumb *thumb, int correlation_id, iPodBuffer *buffer)
{
	MhniHeader *mhni;
	unsigned int total_bytes;
	int bytes_written;
	iPodBuffer *sub_buffer;

	if (thumb == NULL) {
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
	mhni->correlation_id = get_gint32 (correlation_id,
					   buffer->byte_order);
	mhni->image_width =    get_gint16 (thumb->width,
					   buffer->byte_order);
	mhni->image_height =   get_gint16 (thumb->height,
					   buffer->byte_order);
	mhni->image_size =     get_gint32 (thumb->size,
					   buffer->byte_order);
	mhni->ithmb_offset =   get_gint32 (thumb->offset,
					   buffer->byte_order);

	mhni->vertical_padding =
	    get_gint16 (thumb->vertical_padding, buffer->byte_order);
	mhni->horizontal_padding = 
	    get_gint16 (thumb->horizontal_padding, buffer->byte_order);

	sub_buffer = ipod_buffer_get_sub_buffer (buffer, total_bytes);
	if (sub_buffer == NULL) {
		return  -1;
	}
	bytes_written = write_mhod_type_3 (thumb->filename, sub_buffer);
	ipod_buffer_destroy (sub_buffer);
	if (bytes_written == -1) {
		return -1;
	}
	total_bytes += bytes_written;
	mhni->total_len = get_gint32 (total_bytes, buffer->byte_order);
	/* Only update number of children when all went well to try to get
	 * something somewhat consistent when there are errors 
	 */
	mhni->num_children = get_gint32 (1, buffer->byte_order);
	
	dump_mhni (mhni);

	return total_bytes;
}

static int
write_mhod (Itdb_DB *db, Itdb_Thumb *thumb, int correlation_id, iPodBuffer *buffer)
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
	bytes_written = write_mhni (db, thumb, correlation_id, sub_buffer);
	ipod_buffer_destroy (sub_buffer);
	if (bytes_written == -1) {
		return -1;
	}
	total_bytes += bytes_written;
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
	GList *it = NULL;
	Itdb_Track *song;
	Itdb_Artwork *artwork;

	mhii = (MhiiHeader *)init_header (buffer, "mhii", sizeof (MhiiHeader));
	if (mhii == NULL) {
		return -1;
	}
	total_bytes = get_gint32 (mhii->header_len, buffer->byte_order);

	switch( buffer->db_type) {
	case DB_TYPE_ITUNES:
		song = (Itdb_Track *)data;
		mhii->song_id = get_gint64 (song->dbid, buffer->byte_order);
		mhii->image_id = get_guint32 (song->artwork->id, buffer->byte_order);
		mhii->orig_img_size = get_gint32 (song->artwork->artwork_size, 
				buffer->byte_order);
		it = song->artwork->thumbnails;
		break;
	case DB_TYPE_PHOTO:
		artwork = (Itdb_Artwork *)data;
		mhii->song_id = get_gint64 (artwork->id + 2, buffer->byte_order);
		mhii->image_id = get_guint32 (artwork->id, buffer->byte_order);
		mhii->orig_date = get_guint32 (itdb_time_host_to_mac( artwork->creation_date )
				, buffer->byte_order);
		mhii->digitised_date = get_guint32 (itdb_time_host_to_mac( artwork->creation_date )
				, buffer->byte_order);
		it = artwork->thumbnails;
		break;
	}
	num_children = 0;
	while (it != NULL) {
		iPodBuffer *sub_buffer;
		Itdb_Thumb *thumb;
		const Itdb_ArtworkFormat *img_info;

		mhii->num_children = get_gint32 (num_children, 
						 buffer->byte_order);
		mhii->total_len = get_gint32 (total_bytes, buffer->byte_order);
		sub_buffer = ipod_buffer_get_sub_buffer (buffer, total_bytes);
		if (sub_buffer == NULL) {
			return -1;
		}
		thumb = (Itdb_Thumb *)it->data;
		img_info = itdb_get_artwork_info_from_type (
		    db_get_device(db), thumb->type);
		if (img_info == NULL) {
			return -1;
		}
		bytes_written = write_mhod (db, thumb, img_info->correlation_id, 
					    sub_buffer);
		ipod_buffer_destroy (sub_buffer);
		if (bytes_written == -1) {
			return -1;
		}
		total_bytes += bytes_written;
		num_children++;
		it = it->next;
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
			if (song->artwork->id == 0) {
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
write_mhba (Itdb_PhotoAlbum *photo_album, iPodBuffer *buffer)
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
	mhba->playlist_id = get_gint32(photo_album->album_id, buffer->byte_order);
	mhba->master = get_gint32(photo_album->master, buffer->byte_order);
	mhba->prev_playlist_id = get_gint32(photo_album->prev_album_id, buffer->byte_order);
	mhba->num_mhias = get_gint32(photo_album->num_images, buffer->byte_order);
	total_bytes = get_gint32 (mhba->header_len, buffer->byte_order);

	/* FIXME: Write other mhods */
	/* Write album title */
	sub_buffer = ipod_buffer_get_sub_buffer (buffer, total_bytes);
	if (sub_buffer == NULL) {
	    return -1;
	}
	bytes_written = write_mhod_type_1 (photo_album->name, sub_buffer);
	ipod_buffer_destroy (sub_buffer);
	if (bytes_written == -1) {
	    return -1;
	} 
	total_bytes += bytes_written;

	for (it = photo_album->members; it != NULL; it = it->next) {
		gint image_id = GPOINTER_TO_INT(it->data);

		sub_buffer = ipod_buffer_get_sub_buffer (buffer, total_bytes);
		if (sub_buffer == NULL) {
		    return -1;
		}
		bytes_written = write_mhia (image_id, sub_buffer);
		ipod_buffer_destroy (sub_buffer);
		if (bytes_written == -1) {
		    return -1;
		} 
		total_bytes += bytes_written;
	}
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

	    for (it = db_get_photodb(db)->photoalbums; it != NULL; it = it->next) {
		Itdb_PhotoAlbum *photo_album = (Itdb_PhotoAlbum *)it->data;

		sub_buffer = ipod_buffer_get_sub_buffer (buffer, total_bytes);
		if (sub_buffer == NULL) {
		    return -1;
		}
		bytes_written = write_mhba (photo_album, sub_buffer);
		ipod_buffer_destroy (sub_buffer);
		if (bytes_written == -1) {
		    return -1;
		} 
		total_bytes += bytes_written;
		mhla->num_children++;
	    }
	}

	dump_mhl ((MhlHeader *)mhla, "mhla");

	return total_bytes;
}

static int
write_mhif (Itdb_DB *db, iPodBuffer *buffer, enum iPodThumbnailType type)
{
	MhifHeader *mhif;
	const Itdb_ArtworkFormat *img_info;
	
	mhif = (MhifHeader *)init_header (buffer, "mhif", sizeof (MhifHeader));
	if (mhif == NULL) {
		return -1;
	}
	mhif->total_len = mhif->header_len;
	
	img_info = itdb_get_artwork_info_from_type (db_get_device(db), type);
	if (img_info == NULL) {
		return -1;
	}
	mhif->correlation_id = get_gint32 (img_info->correlation_id, 
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
	iPodBuffer *sub_buffer;
	enum iPodThumbnailType thumb_type = IPOD_THUMBNAIL_FULL_SIZE;

	mhlf = (MhlfHeader *)init_header (buffer, "mhlf", sizeof (MhlfHeader));
	if (mhlf == NULL) {
		return -1;
	}

	total_bytes = get_gint32 (mhlf->header_len, buffer->byte_order);

	sub_buffer = ipod_buffer_get_sub_buffer (buffer, total_bytes);
	if (sub_buffer == NULL) {
		return -1;
	}

	switch (buffer->db_type) {
	case DB_TYPE_ITUNES:
		thumb_type = IPOD_THUMBNAIL_FULL_SIZE;
		break;
	case DB_TYPE_PHOTO:
		thumb_type = IPOD_PHOTO_FULL_SIZE;
		break;
	}
	bytes_written = write_mhif (db
				  , sub_buffer
				  , thumb_type);
	ipod_buffer_destroy (sub_buffer);
	if (bytes_written == -1) {
		return -1;
	} 
	total_bytes += bytes_written;
	
	/* Only update number of children when all went well to try to get
	 * something somewhat consistent when there are errors 
	 */
	/* For the ArworkDB file, there are only 2 physical file storing
	 * thumbnails: F1016_1.ithmb for the bigger thumbnails (39200 bytes)
	 * and F1017_1.ithmb for the 'now playing' thumbnails (6272)
	 */
	mhlf->num_files = get_gint32 (1, buffer->byte_order);

	sub_buffer = ipod_buffer_get_sub_buffer (buffer, total_bytes);
	if (sub_buffer == NULL) {
		return -1;
	}

	switch (buffer->db_type) {
	case DB_TYPE_ITUNES:
		thumb_type = IPOD_THUMBNAIL_NOW_PLAYING;
		break;
	case DB_TYPE_PHOTO:
		thumb_type = IPOD_PHOTO_LITTLE;
		break;
	}
	bytes_written = write_mhif (db
				  , sub_buffer
				  , thumb_type);
	ipod_buffer_destroy (sub_buffer);
	if (bytes_written == -1) {
		return -1;
	} 
	total_bytes += bytes_written;

	/* Only update number of children when all went well to try to get
	 * something somewhat consistent when there are errors 
	 */
	/* For the ArworkDB file, there are only 2 physical file storing
	 * thumbnails: F1016_1.ithmb for the bigger thumbnails (39200 bytes)
	 * and F1017_1.ithmb for the 'now playing' thumbnails (6272)
	 */
	mhlf->num_files = get_gint32 (2, buffer->byte_order);

	dump_mhl ((MhlHeader *)mhlf, "mhlf");

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
		mhfd->unknown2 = get_gint32 (1, buffer->byte_order);
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
		mhfd->total_len = get_gint32 (total_bytes, buffer->byte_order);
		mhfd->num_children = get_gint32 (i, buffer->byte_order);
	}

	dump_mhfd (mhfd);

	return total_bytes;
}

static unsigned int
ipod_artwork_db_set_ids (Itdb_iTunesDB *db)
{
	GList *it;
	unsigned int id;
	
	id = 64;
	for (it = db->tracks; it != NULL; it = it->next) {
		Itdb_Track *song;

		song = (Itdb_Track *)it->data;
		if (song->artwork->thumbnails != NULL) {
			song->artwork->id = id;
			id++;
		}
	}
	
	return id;
}

int
ipod_write_artwork_db (Itdb_iTunesDB *itdb)
{
	iPodBuffer *buf;
	int bytes_written;
	int result;
	char *filename;
	int id_max;
	Itdb_DB db;

	db.db_type = DB_TYPE_ITUNES;
	db.db.itdb = itdb;

	/* First, let's write the .ithmb files, this will create the
	 * various thumbnails as well */

	itdb_write_ithumb_files (&db);
	/* Now we can update the ArtworkDB file */
	id_max = ipod_artwork_db_set_ids (itdb);

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

	/* Refcount of the mmap buffer should drop to 0 and this should
	 * sync buffered data to disk
	 * FIXME: it's probably less error-prone to add a ipod_buffer_mmap_sync
	 * call...
	 */
	ipod_buffer_destroy (buf);

	if (bytes_written == -1) {
		g_print ("Failed to save %s\n", filename);
		g_free (filename);
		/* FIXME: maybe should unlink the file we may have created */
		return -1;
	}

	result = truncate (filename, bytes_written);
	if (result != 0) {
		g_print ("Failed to truncate %s: %s\n", 
			 filename, strerror (errno));
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
	int result;
	char *filename;
	int id_max;
	Itdb_DB db;

	db.db_type = DB_TYPE_PHOTO;
	db.db.photodb = photodb;

	filename = ipod_db_get_photos_db_path (db_get_mountpoint (&db));

	itdb_write_ithumb_files (&db);

	if (filename == NULL) {
		return -1;
	}
	buf = ipod_buffer_new (filename, photodb->device->byte_order, DB_TYPE_PHOTO);
	if (buf == NULL) {
		g_print ("Couldn't create %s\n", filename);
		g_free (filename);
		return -1;
	}
	id_max = itdb_get_free_photo_id( photodb );
	bytes_written = write_mhfd (&db, buf, id_max);

	/* Refcount of the mmap buffer should drop to 0 and this should
	 * sync buffered data to disk
	 * FIXME: it's probably less error-prone to add a ipod_buffer_mmap_sync
	 * call...
	 */
	ipod_buffer_destroy (buf);

	if (bytes_written == -1) {
		g_print ("Failed to save %s\n", filename);
		g_free (filename);
		/* FIXME: maybe should unlink the file we may have created */
		return -1;
	}

	result = truncate (filename, bytes_written);
	if (result != 0) {
		g_print ("Failed to truncate %s: %s\n", 
			 filename, strerror (errno));
		g_free (filename);	
		return -1;
	}
	g_free (filename);
	return 0;
}
#else
int
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

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
#include "db-artwork-parser.h"

#if HAVE_GDKPIXBUF

#include "db-artwork-debug.h"
#include "db-itunes-parser.h"
#include "db-image-parser.h"

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

#define IPOD_MMAP_SIZE 2 * 1024 * 1024

struct iPodMmapBuffer {
	int fd;
	void *mmap_area;
	size_t size;
	int ref_count;
};

struct _iPodBuffer {
	struct iPodMmapBuffer *mmap;
	off_t offset;
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
			 new_size, strerror (errno));
		return -1;
	}
	result = 0;
	result = write (mmap_buf->fd, &result, 1);
	if (result != 1) {
		g_print ("Failed to write a byte at %lu: %s\n", 
			 new_size, strerror (errno));
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

	buffer->mmap->ref_count++;

	return sub_buffer;
}

static iPodBuffer *
ipod_buffer_new (const char *filename)
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

	return buffer;
}

enum MhsdType {
	MHSD_TYPE_MHLI = 1,
	MHSD_TYPE_MHLA = 2,
	MHSD_TYPE_MHLF = 3
};

enum iPodThumbnailType {
	IPOD_THUMBNAIL_FULL_SIZE,
	IPOD_THUMBNAIL_NOW_PLAYING
};

#define IPOD_THUMBNAIL_FULL_SIZE_SIZE (140*140*2)
#define IPOD_THUMBNAIL_NOW_PLAYING_SIZE (56*56*2)


#define RETURN_SIZE_FOR(id, size) if (strncmp (id, header_id, 4) == 0) return (size)


static const IpodArtworkFormat *
get_artwork_info (IpodDevice *ipod, int image_type)
{
	const IpodArtworkFormat *formats;
	
	if (ipod == NULL) {
		return NULL;
	}

	g_object_get (G_OBJECT (ipod), "artwork-formats", &formats, NULL);
	if (formats == NULL) {
		return NULL;
	}
	
	while ((formats->type != -1) && (formats->type != image_type)) {
		formats++;
	}

	if (formats->type == -1) {
		return NULL;
	}

	return formats;
}


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

	return 0;
}

static void *
init_header (iPodBuffer *buffer, gchar header_id[4], guint header_len)
{
	MHeader *mh;
	int padded_size;

	padded_size = get_padded_header_size (header_id);
	if (padded_size != 0) {
		header_len = padded_size;
	}
	g_assert (header_len > sizeof (MHeader));
	if (ipod_buffer_maybe_grow (buffer, header_len) != 0) {
		return NULL;
	}
	mh = (MHeader*)ipod_buffer_get_pointer (buffer);
	memset (mh, 0, header_len);
	strncpy ((char *)mh->header_id, header_id, 4);
	mh->header_len = GINT_TO_LE (header_len);
	
	return mh;
}


static int 
write_mhod_type_3 (Itdb_Image *image, iPodBuffer *buffer)
{
	MhodHeaderArtworkType3 *mhod;
	unsigned int total_bytes;
	int len;
	gunichar2 *utf16;
	int i;

	g_assert (image->filename != NULL);

	mhod = (MhodHeaderArtworkType3 *)init_header (buffer, "mhod", 
						      sizeof (MhodHeaderArtworkType3));
	if (mhod == NULL) {
		return -1;
	}
	total_bytes = sizeof (MhodHeaderArtworkType3);
	mhod->total_len = GINT_TO_LE (total_bytes);
	/* Modify header length, since iTunes only puts the length of 
	 * MhodHeader in header_len
	 */
	mhod->header_len = GINT_TO_LE (sizeof (MhodHeader));
	mhod->type = GINT_TO_LE (3);
	mhod->mhod_version = GINT_TO_LE (2);

	len = strlen (image->filename);

	/* number of bytes of the string encoded in UTF-16 */
	mhod->string_len = GINT_TO_LE (2*len);

	/* Make sure we have enough free space to write the string */
	if (ipod_buffer_maybe_grow (buffer, total_bytes + 2*len) != 0) {
		return  -1;
	}
	utf16 = g_utf8_to_utf16 (image->filename, -1, NULL, NULL, NULL);
	if (utf16 == NULL) {		
		return -1;
	}
	memcpy (mhod->string, utf16, 2*len);
	g_free (utf16);
	for (i = 0; i < len; i++) {
		mhod->string[i] = GINT_TO_LE (mhod->string[i]);
	}
	total_bytes += 2*len;
	mhod->total_len = GINT_TO_LE (total_bytes);	

	dump_mhod_type_3 (mhod);

	return total_bytes;
}

static int 
write_mhni (Itdb_Image *image, int correlation_id, iPodBuffer *buffer)
{
	MhniHeader *mhni;
	unsigned int total_bytes;
	int bytes_written;
	iPodBuffer *sub_buffer;

	if (image == NULL) {
		return -1;
	}

	mhni = (MhniHeader *)init_header (buffer, "mhni", 
					  sizeof (MhniHeader));
	if (mhni == NULL) {
		return -1;
	}
	total_bytes = GINT_FROM_LE (mhni->header_len);
	mhni->total_len = GINT_TO_LE (total_bytes);

	mhni->correlation_id = GINT_TO_LE (correlation_id);
	mhni->image_width  = GINT16_TO_LE (image->width);
	mhni->image_height = GINT16_TO_LE (image->height);
	mhni->image_size   = GINT32_TO_LE (image->size);
	mhni->ithmb_offset = GINT32_TO_LE (image->offset);

	sub_buffer = ipod_buffer_get_sub_buffer (buffer, total_bytes);
	if (sub_buffer == NULL) {
		return  -1;
	}
	bytes_written = write_mhod_type_3 (image, sub_buffer);
	ipod_buffer_destroy (sub_buffer);
	if (bytes_written == -1) {
		return -1;
	}
	total_bytes += bytes_written;
	mhni->total_len = GINT_TO_LE (total_bytes);
	/* Only update number of children when all went well to try to get
	 * something somewhat consistent when there are errors 
	 */
	mhni->num_children = GINT_TO_LE (1);
	
	dump_mhni (mhni);

	return total_bytes;
}

static int
write_mhod (Itdb_Image *image, int correlation_id, iPodBuffer *buffer)
{
	MhodHeader *mhod;
	unsigned int total_bytes;
	int bytes_written;
	iPodBuffer *sub_buffer;

	if (image == NULL) {
		return -1;
	}

	mhod = (MhodHeader *)init_header (buffer, "mhod", 
					  sizeof (MhodHeader));
	if (mhod == NULL) {
		return -1;
	}
	total_bytes = sizeof (MhodHeader);
	mhod->total_len = GINT_TO_LE (total_bytes);
	mhod->type = GINT_TO_LE (MHOD_TYPE_LOCATION);
	sub_buffer = ipod_buffer_get_sub_buffer (buffer, total_bytes);
	if (sub_buffer == NULL) {
		return -1;
	}
	bytes_written = write_mhni (image, correlation_id, sub_buffer);
	ipod_buffer_destroy (sub_buffer);
	if (bytes_written == -1) {
		return -1;
	}
	total_bytes += bytes_written;
	mhod->total_len = GINT_TO_LE (total_bytes);

	dump_mhod (mhod);

	return total_bytes;
}

static int
write_mhii (Itdb_Track *song, iPodBuffer *buffer)
{
	MhiiHeader *mhii;
	unsigned int total_bytes;
	int bytes_written;
	int num_children;
	GList *it;

	mhii = (MhiiHeader *)init_header (buffer, "mhii", sizeof (MhiiHeader));
	if (mhii == NULL) {
		return -1;
	}
	total_bytes = GINT_FROM_LE (mhii->header_len);
	mhii->song_id = GINT64_TO_LE (song->dbid);
	mhii->image_id = GUINT_TO_LE (song->image_id);
	/* Adding 1 to artwork_size since this is what iTunes 4.9 does (there
	 * is a 1 difference between the artwork size in iTunesDB and the 
	 * artwork size in ArtworkDB)
	 */
	mhii->orig_img_size = GINT_TO_LE (song->artwork_size)+1;
	num_children = 0;
	for (it = song->thumbnails; it != NULL; it = it->next) {
		iPodBuffer *sub_buffer;
		Itdb_Image *thumb;
		const IpodArtworkFormat *img_info;

		mhii->num_children = GINT_TO_LE (num_children);
		mhii->total_len = GINT_TO_LE (total_bytes);
		sub_buffer = ipod_buffer_get_sub_buffer (buffer, total_bytes);
		if (sub_buffer == NULL) {
			return -1;
		}
		thumb = (Itdb_Image *)it->data;
		img_info = get_artwork_info (song->itdb->device, thumb->type);
		if (img_info == NULL) {
			return -1;
		}
		bytes_written = write_mhod (thumb, img_info->correlation_id, 
					    sub_buffer);
		ipod_buffer_destroy (sub_buffer);
		if (bytes_written == -1) {
			return -1;
		}
		total_bytes += bytes_written;
		num_children++;
	}

	mhii->num_children = GINT_TO_LE (num_children);
	mhii->total_len = GINT_TO_LE (total_bytes);

	dump_mhii (mhii);

	return total_bytes;
}

static int
write_mhli (Itdb_iTunesDB *db, iPodBuffer *buffer)
{
	GList *it;
	MhliHeader *mhli;
	unsigned int total_bytes;
	int num_thumbs;

	mhli = (MhliHeader *)init_header (buffer, "mhli", sizeof (MhliHeader));
	if (mhli == NULL) {
		return -1;
	}

	num_thumbs = 0;
	total_bytes = GINT_FROM_LE (mhli->header_len);
	for (it = db->tracks; it != NULL; it = it->next) {
		Itdb_Track *song;
		int bytes_written;
		iPodBuffer *sub_buffer;

		song = (Itdb_Track*)it->data;
		if (song->image_id == 0) {
			continue;
		}
		sub_buffer = ipod_buffer_get_sub_buffer (buffer, total_bytes);
		if (sub_buffer == NULL) {
			continue;
		}
		bytes_written = write_mhii (song, sub_buffer);
		ipod_buffer_destroy (sub_buffer);
		if (bytes_written != -1) {
			num_thumbs++;
			total_bytes += bytes_written;
		}
	}

	mhli->num_children = GINT_TO_LE (num_thumbs);

	dump_mhl ((MhlHeader *)mhli, "mhli");

	return total_bytes;
}

static int
write_mhla (Itdb_iTunesDB *db, iPodBuffer *buffer)
{
	MhlaHeader *mhla;

	mhla = (MhlaHeader *)init_header (buffer, "mhla", sizeof (MhlaHeader));
	if (mhla == NULL) {
		return -1;
	}
	
	dump_mhl ((MhlHeader *)mhla, "mhla");

	return GINT_FROM_LE (mhla->header_len);
}



static int
write_mhif (Itdb_iTunesDB *db, iPodBuffer *buffer, enum iPodThumbnailType type)
{
	MhifHeader *mhif;
	const IpodArtworkFormat *img_info;
	
	mhif = (MhifHeader *)init_header (buffer, "mhif", sizeof (MhifHeader));
	if (mhif == NULL) {
		return -1;
	}
	mhif->total_len = mhif->header_len;
	
	img_info = get_artwork_info (db->device, type);
	if (img_info == NULL) {
		return -1;
	}

	mhif->correlation_id = GINT_TO_LE (img_info->correlation_id);
	mhif->image_size = GINT_TO_LE (img_info->height * img_info->width * 2);

	dump_mhif (mhif);
	
	return GINT_FROM_LE (mhif->header_len);
}


static int
write_mhlf (Itdb_iTunesDB *db, iPodBuffer *buffer)
{
	MhlfHeader *mhlf;
	unsigned int total_bytes;
	int bytes_written;
	iPodBuffer *sub_buffer;

	mhlf = (MhlfHeader *)init_header (buffer, "mhlf", sizeof (MhlfHeader));
	if (mhlf == NULL) {
		return -1;
	}

	total_bytes = GINT_FROM_LE (mhlf->header_len);

	sub_buffer = ipod_buffer_get_sub_buffer (buffer, total_bytes);
	if (sub_buffer == NULL) {
		return -1;
	}
	bytes_written = write_mhif (db, sub_buffer, IPOD_THUMBNAIL_FULL_SIZE);
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
	mhlf->num_files = GINT_TO_LE (1);

	sub_buffer = ipod_buffer_get_sub_buffer (buffer, total_bytes);
	if (sub_buffer == NULL) {
		return -1;
	}
	bytes_written = write_mhif (db, sub_buffer, IPOD_THUMBNAIL_NOW_PLAYING);
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
	mhlf->num_files = GINT_TO_LE (2);

	dump_mhl ((MhlHeader *)mhlf, "mhlf");

	return total_bytes;
}


static int 
write_mhsd (Itdb_iTunesDB *db, iPodBuffer *buffer, enum MhsdType type)
{
	MhsdHeader *mhsd;
	unsigned int total_bytes;
	int bytes_written;
	iPodBuffer *sub_buffer;

	g_assert (type >= MHSD_TYPE_MHLI);
	g_assert (type <= MHSD_TYPE_MHLF);
	mhsd = (MhsdHeader *)init_header (buffer, "mhsd", sizeof (MhsdHeader));
	if (mhsd == NULL) {
		return -1;
	}
	total_bytes = GINT_FROM_LE (mhsd->header_len);
	mhsd->total_len = GINT_TO_LE (total_bytes);
	mhsd->index = GINT_TO_LE (type);
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
		mhsd->total_len = GINT_TO_LE (total_bytes);
	}

	dump_mhsd (mhsd);

	return total_bytes;
}

static int
write_mhfd (Itdb_iTunesDB *db, iPodBuffer *buffer, int id_max)
{
	MhfdHeader *mhfd;
	unsigned int total_bytes;
	int bytes_written;
	int i;

	mhfd = (MhfdHeader *)init_header (buffer, "mhfd", sizeof (MhfdHeader));
	if (mhfd == NULL) {
		return -1;
	}
	total_bytes = GINT_FROM_LE (mhfd->header_len);
	mhfd->total_len = GINT_TO_LE (total_bytes);
	mhfd->unknown2 = GINT_TO_LE (1);
	mhfd->unknown4 = GINT_TO_LE (id_max);
	mhfd->unknown7 = GINT_TO_LE (2);
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
		mhfd->total_len = GINT_TO_LE (total_bytes);
		mhfd->num_children = GINT_TO_LE (i);
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
		if (song->thumbnails != NULL) {
			song->image_id = id;
			id++;
		}
	}
	
	return id;
}

int
ipod_write_artwork_db (Itdb_iTunesDB *db)
{
	iPodBuffer *buf;
	int bytes_written;
	int result;
	char *filename;
	int id_max;

	/* First, let's write the .ithmb files, this will create the various 
	 * thumbnails as well, and update the Itdb_Track items contained in
	 * the database appropriately (ie set the 'artwork_count' and 
	 * 'artwork_size' fields, as well as the 2 Itdb_Image fields
	 */
	itdb_write_ithumb_files (db, db->mountpoint);
	/* Now we can update the ArtworkDB file */
	id_max = ipod_artwork_db_set_ids (db);

	/* FIXME: need to create the file if it doesn't exist */
	filename = ipod_db_get_artwork_db_path (db->mountpoint);
	if (filename == NULL) {
		/* FIXME: the iTunesDB will be inconsistent wrt artwork_count
		 * it might be better to 0 out this field in all tracks
		 * when we encounter an error
		 */
		return -1;
	}
	buf = ipod_buffer_new (filename);
	if (buf == NULL) {
		g_print ("Couldn't create %s\n", filename);
		g_free (filename);
		return -1;
	}
	bytes_written = write_mhfd (db, buf, id_max);

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
ipod_write_artwork_db (Itdb_iTunesDB *db)
{
    return -1;
}
#endif

/*
|  Copyright (C) 2009 Nikias Bassen <nikias@gmx.li>
|  Part of the gtkpod project.
| 
|  URL: http://www.gtkpod.org/
|  URL: http://gtkpod.sourceforge.net/
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
|  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
|
|  iTunes and iPod are trademarks of Apple
|
|  This product is not supported/written/published by Apple!
|
|  $Id$
*/
#include <config.h>
#include <string.h>
#include <zlib.h>

#include <glib/gi18n-lib.h>

#include "itdb_zlib.h"
#include "itdb_private.h"

#define CHUNK 16384

static int zlib_inflate(gchar *outbuf, gchar *zdata, gsize compressed_size, gsize *uncompressed_size)
{
    int ret;
    guint32 inpos = 0;
    guint32 outpos = 0;
    unsigned have;
    z_stream strm;
    unsigned char out[CHUNK];

    /* allocate inflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;
    ret = inflateInit(&strm);
    if (ret != Z_OK)
        return ret;

    *uncompressed_size = 0;

    /* decompress until deflate stream ends or end of file */
    do {
        strm.avail_in = CHUNK;
	if (inpos+strm.avail_in > compressed_size) {
	    strm.avail_in = compressed_size - inpos;
	}
        strm.next_in = (unsigned char*)zdata+inpos;
	inpos+=strm.avail_in;

        /* run inflate() on input until output buffer not full */
        do {
            strm.avail_out = CHUNK;
            if (outbuf)  {
                strm.next_out = (unsigned char*)(outbuf + outpos);
            } else {
                strm.next_out = out;
            }
            ret = inflate(&strm, Z_NO_FLUSH);
            g_assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
            switch (ret) {
            case Z_NEED_DICT:
                ret = Z_DATA_ERROR;     /* and fall through */
            case Z_DATA_ERROR:
            case Z_MEM_ERROR:
                (void)inflateEnd(&strm);
                return ret;
            }
            have = CHUNK - strm.avail_out;
	    *uncompressed_size += have;
	    if (outbuf) {
		outpos += have;
	    }
        } while (strm.avail_out == 0);

        /* done when inflate() says it's done */
    } while (ret != Z_STREAM_END);

    /* clean up and return */
    (void)inflateEnd(&strm);
    return ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;
}

gboolean itdb_zlib_check_decompress_fimp (FImport *fimp)
{
    FContents *cts;
    guint32 headerSize;
    guint32 cSize;
    size_t uSize;

    g_return_val_if_fail (fimp, FALSE);
    g_return_val_if_fail (fimp->fcontents, FALSE);
    g_return_val_if_fail (fimp->fcontents->filename, FALSE);

    cts = fimp->fcontents;

    cSize = GUINT32_FROM_LE (*(guint32*)(cts->contents+8));
    headerSize = GUINT32_FROM_LE (*(guint32*)(cts->contents+4));
    uSize = 0;

    if (headerSize < 0xA9) {
	g_set_error (&fimp->error,
		     ITDB_FILE_ERROR,
		     ITDB_FILE_ERROR_ITDB_CORRUPT,
		     _("Header is too small for iTunesCDB!\n"));
	return FALSE;
    }

    /* compression flag */
    if (*(guint8*)(cts->contents+0xa8) == 1) {
	*(guint8*)(cts->contents+0xa8) = 0;
    } else {
	g_warning ("Unknown value for 0xa8 in header: should be 1 for uncompressed, is %d.\n", *(guint8*)(cts->contents+0xa8));
    }

    if (zlib_inflate(NULL, cts->contents+headerSize, cSize-headerSize, &uSize) == 0) {
	gchar *new_contents;
	/*g_print("allocating %"G_GSIZE_FORMAT"\n", uSize+headerSize);*/
	new_contents = (gchar*)g_malloc(uSize+headerSize);
	memcpy(new_contents, cts->contents, headerSize);
	/*g_print("decompressing\n");*/
	if (zlib_inflate(new_contents+headerSize, cts->contents+headerSize, cSize-headerSize, &uSize) == 0) {
	    /* update FContents structure */
	    g_free(cts->contents);
	    cts->contents = new_contents;
	    cts->length = uSize+headerSize;
	    /*g_print("uncompressed size: %"G_GSIZE_FORMAT"\n", cts->length);*/
	}
    } else {
	g_set_error (&fimp->error,
		     ITDB_FILE_ERROR,
		     ITDB_FILE_ERROR_CORRUPT,
		     _("iTunesCDB '%s' could not be decompressed"),
		     cts->filename);
	return FALSE;
    }

    return TRUE;
}

gboolean itdb_zlib_check_compress_fexp (FExport *fexp)
{
    WContents *cts;
    guint32 header_len;
    uLongf compressed_len;
    guint32 uncompressed_len;
    gchar *new_contents;
    int status;

    cts = fexp->wcontents;

    /*g_print("target DB needs compression\n");*/

    header_len = GUINT32_FROM_LE (*(guint32*)(cts->contents+4));
    uncompressed_len = GUINT32_FROM_LE(*(guint32*)(cts->contents+8)) - header_len;

    if (header_len < 0xA9) {
	g_set_error (&fexp->error,
		     ITDB_FILE_ERROR,
		     ITDB_FILE_ERROR_ITDB_CORRUPT,
		     _("Header is too small for iTunesCDB!\n"));
	return FALSE;
    }

    /* compression flag */
    if (*(guint8*)(cts->contents+0xa8) == 0) {
	*(guint8*)(cts->contents+0xa8) = 1;
    } else {
	g_warning ("Unknown value for 0xa8 in header: should be 0 for uncompressed, is %d.\n", *(guint8*)(cts->contents+0xa8));
    }

    compressed_len = compressBound (uncompressed_len);

    new_contents = g_malloc (header_len + compressed_len);
    memcpy (new_contents, cts->contents, header_len);
    status = compress2 ((guchar*)new_contents + header_len, &compressed_len,
			(guchar*)cts->contents + header_len, uncompressed_len, 1);
    if (status != Z_OK) {
	g_free (new_contents);
	g_set_error (&fexp->error,
		     ITDB_FILE_ERROR,
		     ITDB_FILE_ERROR_ITDB_CORRUPT,
		     _("Error compressing iTunesCDB file!\n"));
	return FALSE;
    }

    g_free(cts->contents);
    /* update mhbd size */
    *(guint32*)(new_contents+8) = GUINT32_TO_LE (compressed_len + header_len);
    cts->contents = new_contents;
    cts->pos = compressed_len + header_len;
    /*g_print("compressed size: %ld\n", cts->pos);*/

    return TRUE;
}

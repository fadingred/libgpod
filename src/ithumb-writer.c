/*  Time-stamp: <2007-09-09 00:13:13 jcs>
 *
 *  Copyright (C) 2005 Christophe Fergeau
 *
 *  URL: http://www.gtkpod.org/libgpod.html
 *  URL: http://gtkpod.sourceforge.net/
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
 *  $Id$
 */

#include <config.h>
#include "itdb.h"
#include "db-image-parser.h"

#ifdef HAVE_GDKPIXBUF

#include "itdb_private.h"
#include "itdb_endianness.h"
#include "pixmaps.h"

#include <errno.h>
#include <locale.h>
#include <string.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


/* Maximum size for .ithmb files. Reduced from 500 MB to 256 MB after
   reports of slow iPod interface behavior */
#define ITHUMB_MAX_SIZE (256L*1000L*1000L)
/* for testing: */
/*#define ITHUMB_MAX_SIZE (1L*1000L*1000L)*/

struct _iThumbWriter {
	off_t cur_offset;
	FILE *f;
        gchar *mountpoint;
        gchar *filename;
        gint current_file_index;
	const Itdb_ArtworkFormat *img_info;
        DbType db_type;
        guint byte_order;
};
typedef struct _iThumbWriter iThumbWriter;


static guint16 *
pack_RGB_565 (GdkPixbuf *pixbuf, const Itdb_ArtworkFormat *img_info,
	      gint horizontal_padding, gint vertical_padding)
{
	guchar *pixels;
	guint16 *result;
	gint row_stride;
	gint channels;
	gint width;
	gint height;
	gint w;
	gint h;
	gint byte_order;

	g_object_get (G_OBJECT (pixbuf), 
		      "rowstride", &row_stride, "n-channels", &channels,
		      "height", &height, "width", &width,
		      "pixels", &pixels, NULL);
	g_return_val_if_fail ((width <= img_info->width) && (height <= img_info->height), NULL);
	/* dst_width and dst_height come from a width/height database 
	 * hardcoded in libipoddevice code, so dst_width * dst_height * 2 can't
	 * overflow, even on an iPod containing malicious data
	 */
	result = g_malloc0 (img_info->width * img_info->height * 2);

	byte_order = itdb_thumb_get_byteorder (img_info->format);

	for (h = 0; h < height; h++) {
	        gint line = (h+vertical_padding)*img_info->width;
		for (w = 0; w < width; w++) {
			gint r;
			gint g;
			gint b;

			r = pixels[h*row_stride + w*channels];
			g = pixels[h*row_stride + w*channels + 1]; 
			b = pixels[h*row_stride + w*channels + 2]; 

			r >>= (8 - RED_BITS_565);
			g >>= (8 - GREEN_BITS_565);
			b >>= (8 - BLUE_BITS_565);
			r = (r << RED_SHIFT_565) & RED_MASK_565;
			g = (g << GREEN_SHIFT_565) & GREEN_MASK_565;
			b = (b << BLUE_SHIFT_565) & BLUE_MASK_565;
			result[line + w + horizontal_padding] =
			    get_gint16 (r | g | b, byte_order);
		}
	}
	return result;
}

static guint16 *
pack_RGB_555 (GdkPixbuf *pixbuf, const Itdb_ArtworkFormat *img_info,
	      gint horizontal_padding, gint vertical_padding)
{
	guchar *pixels;
	guint16 *result;
	gint row_stride;
	gint channels;
	gint width;
	gint height;
	gint w;
	gint h;
	gint byte_order;

	g_object_get (G_OBJECT (pixbuf), 
		      "rowstride", &row_stride, "n-channels", &channels,
		      "height", &height, "width", &width,
		      "pixels", &pixels, NULL);
	g_return_val_if_fail ((width <= img_info->width) && (height <= img_info->height), NULL);
	/* dst_width and dst_height come from a width/height database 
	 * hardcoded in libipoddevice code, so dst_width * dst_height * 2 can't
	 * overflow, even on an iPod containing malicious data
	 */
	result = g_malloc0 (img_info->width * img_info->height * 2);

	byte_order = itdb_thumb_get_byteorder (img_info->format);

	for (h = 0; h < height; h++) {
	        gint line = (h+vertical_padding)*img_info->width;
		for (w = 0; w < width; w++) {
			gint r;
			gint g;
			gint b;
			gint a;

			r = pixels[h*row_stride + w*channels];
			g = pixels[h*row_stride + w*channels + 1]; 
			b = pixels[h*row_stride + w*channels + 2]; 

			r >>= (8 - RED_BITS_555);
			g >>= (8 - GREEN_BITS_555);
			b >>= (8 - BLUE_BITS_555);
			a = (1 << ALPHA_SHIFT_555) & ALPHA_MASK_555;
			r = (r << RED_SHIFT_555) & RED_MASK_555;
			g = (g << GREEN_SHIFT_555) & GREEN_MASK_555;
			b = (b << BLUE_SHIFT_555) & BLUE_MASK_555;
			result[line + w + horizontal_padding] =
			    get_gint16 (a | r | g | b, byte_order);
			/* I'm not sure if the highest bit really is
			   the alpha channel. For now I'm just setting
			   this bit because that's what I have seen. */
		}
	}
	return result;
}


static guint16 *derange_pixels (guint16 *pixels_s, guint16 *pixels_d,
				gint width, gint height, gint row_stride)
{
    g_return_val_if_fail (width == height, pixels_s);

    if (pixels_s == NULL)
    {
	pixels_s = g_malloc0 (sizeof (guint16)*width*height);
    }

    if (width == 1)
    {
	*pixels_s = *pixels_d;
    }
    else
    {
	derange_pixels (pixels_s + 0,
			pixels_d + 0 + 0,
			width/2, height/2,
			row_stride);
	derange_pixels (pixels_s + (width/2)*(height/2),
			pixels_d + (height/2)*row_stride + 0,
			width/2, height/2,
			row_stride);
	derange_pixels (pixels_s + 2*(width/2)*(height/2),
			pixels_d + width/2,
			width/2, height/2,
			row_stride);
	derange_pixels (pixels_s + 3*(width/2)*(height/2),
			pixels_d + (height/2)*row_stride + width/2,
			width/2, height/2,
			row_stride);
    }
    
    return pixels_s;
}

static guint16 *
pack_rec_RGB_555 (GdkPixbuf *pixbuf, const Itdb_ArtworkFormat *img_info,
		  gint horizontal_padding, gint vertical_padding)
{
    guint16 *pixels;
    guint16 *deranged_pixels = NULL;

    pixels = pack_RGB_555 (pixbuf, img_info,
			   horizontal_padding, vertical_padding);

    if (pixels)
    {
	deranged_pixels = derange_pixels (NULL, pixels,
					  img_info->width, img_info->height,
					  img_info->width);
	g_free (pixels);
    }

    return deranged_pixels;
}


/* pack_UYVY() is adapted from imgconvert.c from the GPixPod project
 * (www.gpixpod.org) */
static guchar *
pack_UYVY (GdkPixbuf *orig_pixbuf, const Itdb_ArtworkFormat *img_info,
	   gint horizontal_padding, gint vertical_padding)
{
    GdkPixbuf *pixbuf;
    guchar *pixels, *yuvdata;
    gint width;
    gint height;
    gint orig_height, orig_width;
    gint x = 0;
    gint z = 0;
    gint z2 = 0;
    gint h = 0;
    gint r0, g0, b0, r1, g1, b1, r2, g2, b2, r3, g3, b3;
    gint rowstride;
    gint yuvsize, halfyuv;
    gint alphabit, rgbpx;
    gint exc;

    g_return_val_if_fail (img_info, NULL);

    width = img_info->width;
    height = img_info->height;

    g_object_get (G_OBJECT (orig_pixbuf), 
		  "height", &orig_height, "width", &orig_width, NULL);

    /* copy into new pixmap with padding applied */
    pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB,
			     gdk_pixbuf_get_has_alpha (orig_pixbuf),
			     8,
			     width, height);
    gdk_pixbuf_copy_area (orig_pixbuf, 0, 0, orig_width, orig_height,
			  pixbuf, horizontal_padding, vertical_padding);

    g_object_get (G_OBJECT (pixbuf), 
		  "rowstride", &rowstride,
		  "pixels", &pixels, NULL);

    yuvsize = width*2*height;
    yuvdata = g_malloc (yuvsize);
    halfyuv = yuvsize/2;
    if (gdk_pixbuf_get_has_alpha(pixbuf))
    {
	alphabit = 1;
	rgbpx = 4;
    }
    else
    {
	alphabit = 0;
	rgbpx = 3;
    }
    exc = rowstride - width*rgbpx;

    while(h < height)
    {
	gint w = 0;
	if((h % 2) == 0)
	{
	    while(w < width)
	    {
		r0 = pixels[x];
		g0 = pixels[x+1];
		b0 = pixels[x+2];
		r1 = pixels[x+3+alphabit];
		g1 = pixels[x+4+alphabit];
		b1 = pixels[x+5+alphabit];
		yuvdata[z] = ((r0*-38 + g0*-74 + b0*112 + 128) >> 8) + 128;/*U0*/
		yuvdata[z+1] = ((r0*66 + g0*129 + b0*25 + 128) >> 8) + 16;/*Y0*/
		yuvdata[z+2] = ((r0*112 + g0*-94 + b0*-18 + 128) >> 8) + 128;/*V0*/
		yuvdata[z+3] = ((r1*66 + g1*129 + b1*25 + 128) >> 8) + 16;/*Y1*/
		w += 2;
		z += 4;
		x += rgbpx*2;
	    }
	}
	else
	{
	    while(w < width)
	    {
		r2 = pixels[x];
		g2 = pixels[x+1];
		b2 = pixels[x+2];
		r3 = pixels[x+3+alphabit];
		g3 = pixels[x+4+alphabit];
		b3 = pixels[x+5+alphabit];
		yuvdata[halfyuv+z2] = ((r2*-38 + g2*-74 + b2*112 + 128) >> 8) + 128;/*U1*/
		yuvdata[halfyuv+z2+1] = ((r2*66 + g2*129 + b2*25 + 128) >> 8) + 16;/*Y2*/
		yuvdata[halfyuv+z2+2] = ((r2*112 + g2*-94 + b2*-18 + 128) >> 8) + 128;/*V1*/
		yuvdata[halfyuv+z2+3] = ((r3*66 + g3*129 + b3*25 + 128) >> 8) + 16;/*Y3*/
		w += 2;
		z2 += 4;
		x += rgbpx*2;
	    }
	}
	x += exc;
	h++;
    }
    g_object_unref (pixbuf);
    return yuvdata;
}



static char *
ipod_image_get_ithmb_filename (const char *mount_point, gint correlation_id, gint index, DbType db_type ) 
{
	gchar *artwork_dir = NULL, *filename, *buf;

	g_return_val_if_fail (mount_point, NULL);
	switch( db_type ) {
	case DB_TYPE_PHOTO:
		artwork_dir = itdb_get_photos_thumb_dir (mount_point);
		if (!artwork_dir)
		{
			/* attempt to create Thumbs dir */
			gchar *photos_dir = itdb_get_photos_dir (mount_point);
			gchar *dir;
			if (!photos_dir)
			{   /* give up */
				return NULL;
			}
			dir = g_build_filename (photos_dir, "Thumbs", NULL);
			mkdir (dir, 0777);
			g_free (dir);
			g_free (photos_dir);

			/* try again */
			artwork_dir = itdb_get_photos_thumb_dir (mount_point);
			if (!artwork_dir)
			{   /* give up */
				return NULL;
			}
		}
		break;
	case DB_TYPE_ITUNES:
	artwork_dir = itdb_get_artwork_dir (mount_point);
	if (!artwork_dir)
	{
		/* attempt to create Artwork dir */
		gchar *control_dir = itdb_get_control_dir (mount_point);
		gchar *dir;
		if (!control_dir)
		{   /* give up */
			return NULL;
		}
		dir = g_build_filename (control_dir, "Artwork", NULL);
		mkdir (dir, 0777);
		g_free (dir);
		g_free (control_dir);

		/* try again */
		artwork_dir = itdb_get_artwork_dir (mount_point);
		if (!artwork_dir)
		{   /* give up */
			return NULL;
		}
	}
	}

	buf = g_strdup_printf ("F%d_%d.ithmb", correlation_id, index);

	filename = itdb_get_path (artwork_dir, buf);

	/* itdb_get_path() only returns existing paths */
	if (!filename)
	{
	    filename = g_build_filename (artwork_dir, buf, NULL);
	}
/*	printf ("%s %s\n", buf, filename);*/

	g_free (buf);
        g_free (artwork_dir);
	return filename;
}



static gboolean
ithumb_writer_write_thumbnail (iThumbWriter *writer, 
			       Itdb_Thumb *thumb)
{
    GdkPixbuf *pixbuf = NULL;
    void *pixels = NULL;
    gint width, height; /* must be gint -- see comment below */

    g_return_val_if_fail (writer, FALSE);
    g_return_val_if_fail (writer->img_info, FALSE);
    g_return_val_if_fail (thumb, FALSE);

    /* Make sure @rotation is valid (0, 90, 180, 270) */
    thumb->rotation = thumb->rotation % 360;
    thumb->rotation /= 90;
    thumb->rotation *= 90;

    /* If we rotate by 90 or 270 degrees interchange the width and
     * height */

    if ((thumb->rotation == 0) || (thumb->rotation == 180))
    {
	width = writer->img_info->width;
	height = writer->img_info->height;
    }
    else
    {
	width = writer->img_info->height;
	height = writer->img_info->width;
    }

    if (thumb->filename)
    {   /* read image from filename */
	pixbuf = gdk_pixbuf_new_from_file_at_size (thumb->filename, 
						   width, height,
						   NULL);

	g_free (thumb->filename);
	thumb->filename = NULL;
    }
    else if (thumb->image_data)
    {   /* image data is stored in image_data and image_data_len */
	GdkPixbufLoader *loader = gdk_pixbuf_loader_new ();
	g_return_val_if_fail (loader, FALSE);
	gdk_pixbuf_loader_set_size (loader,
				    width, height);
	gdk_pixbuf_loader_write (loader,
				 thumb->image_data,
				 thumb->image_data_len,
				 NULL);
	gdk_pixbuf_loader_close (loader, NULL);
	pixbuf = gdk_pixbuf_loader_get_pixbuf (loader);
	if (pixbuf)
	    g_object_ref (pixbuf);
	g_object_unref (loader);

	g_free (thumb->image_data);
	thumb->image_data = NULL;
	thumb->image_data_len = 0;
    }
    else if (thumb->pixbuf)
    {
        pixbuf = gdk_pixbuf_scale_simple (GDK_PIXBUF(thumb->pixbuf),
                                          width, height,
                                          GDK_INTERP_BILINEAR);
        g_object_unref (thumb->pixbuf);
        thumb->pixbuf = NULL;
    }

    if (pixbuf == NULL)
    {
	/* This is quite bad... if we just return FALSE the ArtworkDB
	   gets messed up. */
	pixbuf = gdk_pixbuf_from_pixdata (&questionmark_pixdata, FALSE, NULL);

	if (pixbuf)
	{
	    GdkPixbuf *pixbuf2;
	    pixbuf2 = gdk_pixbuf_scale_simple (pixbuf,
					       writer->img_info->width,
					       writer->img_info->height,
					       GDK_INTERP_BILINEAR);
	    g_object_unref (pixbuf);
	    pixbuf = pixbuf2;
	}
	else
	{
	    /* Somethin went wrong. let's insert a red thumbnail */
	    pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, FALSE, 8,
				     writer->img_info->width,
				     writer->img_info->height);
	    gdk_pixbuf_fill (pixbuf, 0xff000000);
	}
	/* avoid rotation */
	thumb->rotation = 0;
    }

    /* Rotate if necessary */
    if (thumb->rotation != 0)
    {
	GdkPixbuf *new_pixbuf = gdk_pixbuf_rotate_simple (pixbuf, thumb->rotation);
	g_object_unref (pixbuf);
	pixbuf = new_pixbuf;
	/* Clean up */
	thumb->rotation = 0;
    }

    /* !! cannot write directly to &thumb->width/height because
       g_object_get() returns a gint, but thumb->width/height are
       gint16 !! */
    g_object_get (G_OBJECT (pixbuf), 
		  "width", &width,
		  "height", &height,
		  NULL);

    switch (thumb->type)
    {
    case ITDB_THUMB_PHOTO_LARGE:
    case ITDB_THUMB_PHOTO_SMALL:
    case ITDB_THUMB_PHOTO_FULL_SCREEN:
    case ITDB_THUMB_PHOTO_TV_SCREEN:
	thumb->filename = g_strdup_printf (":Thumbs:F%d_%d.ithmb", 
					   writer->img_info->correlation_id,
					   writer->current_file_index);
	break;
    case ITDB_THUMB_COVER_LARGE:
    case ITDB_THUMB_COVER_SMALL:
    case ITDB_THUMB_COVER_XLARGE:
    case ITDB_THUMB_COVER_MEDIUM:
    case ITDB_THUMB_COVER_SMEDIUM:
    case ITDB_THUMB_COVER_XSMALL:
	thumb->filename = g_strdup_printf (":F%d_%d.ithmb", 
					   writer->img_info->correlation_id,
					   writer->current_file_index);
	break;
    }

    switch (writer->db_type)
    {
    case DB_TYPE_PHOTO:
	thumb->horizontal_padding = (writer->img_info->width - width)/2;
	thumb->vertical_padding = (writer->img_info->height - height)/2;
	break;
    case DB_TYPE_ITUNES:
	/* IPOD_COVER_LARGE will be centered automatically using
	   the info in mhni->width/height. Free space around
	   IPOD_COVER_SMALL will be used to display track
	   information -> no padding (tested on iPod
	   Nano). mhni->hor_/ver_padding is working */
	thumb->horizontal_padding = 0;
	thumb->vertical_padding = 0;
	break;
    default:
	g_return_val_if_reached (FALSE);
    }

    /* The thumbnail width/height is inclusive padding */
    thumb->width = thumb->horizontal_padding + width;
    thumb->height = thumb->vertical_padding + height;
    thumb->offset = writer->cur_offset;
    thumb->size = writer->img_info->width * writer->img_info->height * 2;

    switch (writer->img_info->format)
    {
    case THUMB_FORMAT_RGB565_LE_90:
    case THUMB_FORMAT_RGB565_BE_90:
	/* FIXME: actually the previous two might require
	   different treatment (used on iPod Photo for the full
	   screen photo thumbnail) */
    case THUMB_FORMAT_RGB565_LE:
    case THUMB_FORMAT_RGB565_BE:
	pixels = pack_RGB_565 (pixbuf, writer->img_info,
			       thumb->horizontal_padding,
			       thumb->vertical_padding);
	break;
    case THUMB_FORMAT_RGB555_LE_90:
    case THUMB_FORMAT_RGB555_BE_90:
	/* FIXME: actually the previous two might require
	   different treatment (used on iPod Photo for the full
	   screen photo thumbnail) */
    case THUMB_FORMAT_RGB555_LE:
    case THUMB_FORMAT_RGB555_BE:
	pixels = pack_RGB_555 (pixbuf, writer->img_info,
			       thumb->horizontal_padding,
			       thumb->vertical_padding);
	break;
    case THUMB_FORMAT_REC_RGB555_LE_90:
    case THUMB_FORMAT_REC_RGB555_BE_90:
	/* FIXME: actually the previous two might require
	   different treatment (used on iPod Photo for the full
	   screen photo thumbnail) */
    case THUMB_FORMAT_REC_RGB555_LE:
    case THUMB_FORMAT_REC_RGB555_BE:
	pixels = pack_rec_RGB_555 (pixbuf, writer->img_info,
				   thumb->horizontal_padding,
				   thumb->vertical_padding);
	break;
    case THUMB_FORMAT_EXPERIMENTAL_LE:
    case THUMB_FORMAT_EXPERIMENTAL_BE:
	break;
    case THUMB_FORMAT_UYVY_BE:
    case THUMB_FORMAT_UYVY_LE:
	pixels = pack_UYVY (pixbuf, writer->img_info,
			    thumb->horizontal_padding,
			    thumb->vertical_padding);
	break;
    }


    g_object_unref (G_OBJECT (pixbuf));

    if (pixels == NULL)
    {
	return FALSE;
    }
    if (fwrite (pixels, thumb->size, 1, writer->f) != 1) {
	g_free (pixels);
	g_print ("Error writing to file: %s\n", strerror (errno));
	return FALSE;
    }
    g_free (pixels);
    writer->cur_offset += thumb->size;

    if (writer->img_info->padding != 0)
    {
	gint padding = writer->img_info->padding - thumb->size;
	g_return_val_if_fail (padding >= 0, TRUE);
	if (padding != 0)
	{
            /* FIXME: check if a simple fseek() will do the same */
	    gchar *pad_bytes = g_malloc0 (padding);
	    if (fwrite (pad_bytes, padding, 1, writer->f) != 1) {
		g_free (pad_bytes);
		g_print ("Error writing to file: %s\n", strerror (errno));
		return FALSE;
	    }
	    g_free (pad_bytes);
	    writer->cur_offset += padding;
	}
    }
    return TRUE;
}

static gboolean
ithumb_writer_update (iThumbWriter *writer)
{
    while ((writer->f == NULL) || (writer->cur_offset >= ITHUMB_MAX_SIZE))
    {
	if (writer->f)
	{
	    fclose (writer->f);
	    writer->f = NULL;
	}
	g_free (writer->filename);
	writer->filename = NULL;

	/* increment index for filename */
	++writer->current_file_index;

	writer->filename =
	    ipod_image_get_ithmb_filename (writer->mountpoint, 
					   writer->img_info->correlation_id,
					   writer->current_file_index, 
					   writer->db_type);
	if (writer->filename == NULL)
	{
	    return FALSE;
	}
	writer->f = fopen (writer->filename, "ab");
	if (writer->f == NULL)
	{
	    g_print ("Error opening %s: %s\n", writer->filename, strerror (errno));
	    g_free (writer->filename);
	    writer->filename = NULL;
	    return FALSE;
	}
	writer->cur_offset = ftell (writer->f);
    }

    return TRUE;
}



static void
write_thumbnail (gpointer _writer, gpointer _artwork)
{
	iThumbWriter *writer = _writer;
	Itdb_Artwork *artwork = _artwork;
 	Itdb_Thumb *thumb;

	thumb = itdb_artwork_get_thumb_by_type (artwork,
						writer->img_info->type);

	/* size == 0 indicates a thumbnail not yet written to the
	   thumbnail file */
	if (thumb && (thumb->size == 0))
	{
	    /* check if new thumbnail file has to be started */
	    if (ithumb_writer_update (writer))
		ithumb_writer_write_thumbnail (writer, thumb);
	}
}


static void
ithumb_writer_free (iThumbWriter *writer)
{
	g_return_if_fail (writer != NULL);
	if (writer->f)
	{
	    fclose (writer->f);
	    if (writer->filename && (writer->cur_offset == 0))
	    {   /* Remove empty file */
		unlink (writer->filename);
	    }
	}
	g_free (writer->filename);
	g_free (writer->mountpoint);
	g_free (writer);
}


static iThumbWriter *
ithumb_writer_new (const char *mount_point,
		   const Itdb_ArtworkFormat *info,
		   DbType db_type,
		   guint byte_order)
{
	iThumbWriter *writer;

	writer = g_new0 (iThumbWriter, 1);

	writer->img_info = info;

	writer->byte_order = byte_order;
	writer->db_type = db_type;
	writer->mountpoint = g_strdup (mount_point);
	writer->current_file_index = 0;

	if (!ithumb_writer_update (writer))
	{
	    ithumb_writer_free (writer);
	    return NULL;
	}

	return writer;
}

gint offset_sort (gconstpointer a, gconstpointer b);
gint offset_sort (gconstpointer a, gconstpointer b)
{
    return (-(((Itdb_Thumb *)a)->offset - ((Itdb_Thumb *)b)->offset));
}

static gboolean ithumb_rearrange_thumbnail_file (gpointer _key,
						 gpointer _thumbs,
						 gpointer _user_data)
{
    const gchar *filename = _key;
    GList *thumbs = _thumbs;
    gboolean *result = _user_data;
    gint fd = -1;
    guint32 size = 0;
    GList *gl;
    struct stat statbuf;
    guint32 offset;
    void *buf = NULL;

/*     printf ("%s: %d\n", filename, g_list_length (thumbs)); */

    /* check if an error occured */
    if (*result == FALSE)
	goto out;

    if (thumbs == NULL)
    {   /* no thumbnails for this file --> remove altogether */
	if (unlink (filename) == -1)
	{
	    *result = FALSE;
	    goto out;
	}
    }

    /* check if all thumbnails have the same size */
    for (gl=thumbs; gl; gl=gl->next)
    {
	Itdb_Thumb *img = gl->data;

	if (size == 0)
	    size = img->size;
	if (size != img->size)
	{
	    *result = FALSE;
	    goto out;
	}
    }
    if (size == 0)
    {
	*result = FALSE;
	goto out;
    }

    /* OK, all thumbs are the same size @size, let's see how many
     * thumbnails are in the actual file */
    if (g_stat  (filename, &statbuf) != 0)
    {
	*result = FALSE;
	goto out;
    }

    /* check if the file size is a multiple of @size */
    if (((guint32)(statbuf.st_size / size))*size != statbuf.st_size)
    {
	*result = FALSE;
	goto out;
    }

    fd = open (filename, O_RDWR, 0);
    if (fd == -1)
    {
	*result = FALSE;
	goto out;
    }

    /* size is either a value coming from a hardcoded const array from 
     * libipoddevice, or a guint32 read from an iPod file, so no overflow
     * can occur here
     */
    buf = g_malloc (size);

    /* Sort the list of thumbs in reverse order of img->offset */
    thumbs = g_list_sort (thumbs, offset_sort);

    gl = g_list_last (thumbs);

    /* check each thumbnail slot */
    for (offset=0; gl && (offset<statbuf.st_size); offset+=size)
    {
	Itdb_Thumb *thumb = gl->data;
	g_return_val_if_fail (thumb, FALSE);

	/* Try to find a thumbnail that uses this slot */
	while ((gl != NULL) && (thumb->offset < offset))
	{
	    gl = gl->prev;
	    if (gl)
		thumb = gl->data;
	    g_return_val_if_fail (thumb, FALSE);
	}

	if (!gl)
	    break;  /* offset now indicates new length of file */

	if (thumb->offset > offset)
	{
	    /* did not find a thumbnail with matching offset -> copy
	       data from last slot (== first element) */
	    GList *first_gl = g_list_first (thumbs);
	    Itdb_Thumb *first_thumb = first_gl->data;
	    guint32 first_offset;

	    g_return_val_if_fail (first_thumb, FALSE);
	    first_offset = first_thumb->offset;

	    /* actually copy the data */
	    if (lseek (fd, first_offset, SEEK_SET) != first_offset)
	    {
		*result = FALSE;
		goto out;
	    }
	    if (read (fd, buf, size) != size)
	    {
		*result = FALSE;
		goto out;
	    }
	    if (lseek (fd, offset, SEEK_SET) != offset)
	    {
		*result = FALSE;
		goto out;
	    }
	    if (write (fd, buf, size) != size)
	    {
		*result = FALSE;
		goto out;
	    }

	    /* Adjust offset of all thumbnails whose offset is
	       first_offset. Since the list is sorted, they are all at
	       the beginning of the list. */
	    while (first_thumb->offset == first_offset)
	    {
		first_thumb->offset = offset;
		/* There's a possibility that gl is the first
		   element. In that case don't attempt to move it (it
		   wouldn't work as intended because we access
		   gl->next after removing it from the list) */
		if (gl != first_gl)
		{
		    thumbs = g_list_delete_link (thumbs, first_gl);
		    /* Insert /behind/ gl */
		    thumbs = g_list_insert_before (thumbs,
						   gl->next, first_thumb);
		    first_gl = g_list_first (thumbs);
		    first_thumb = first_gl->data;
		    g_return_val_if_fail (first_thumb, FALSE);
		}
	    }
	}
    }
    /* offset corresponds to the new length of the file */
    if (offset > 0)
    {   /* Truncate */
	if (ftruncate (fd, offset) == -1)
	{
	    *result = FALSE;
	    goto out;
	}
    }
    else
    {   /* Remove file altogether */
	close (fd);
	fd = -1;
	if (unlink (filename) == -1)
	{
	    *result = FALSE;
	    goto out;
	}
    }

  out:
    if (fd != -1) close (fd);
    g_free (buf);
    g_list_free (thumbs);
    return TRUE;
}


/* The actual image data of thumbnails is not read into memory. As a
   consequence, writing the thumbnail file is not as straight-forward
   as e.g. writing the iTunesDB where all data is held in memory.

   To avoid the need to read large amounts from the iPod and back, or
   have to large files exist on the iPod (reading from the original
   thumbnail fail and writing to the new thumbnail file), the
   modifications are done in place.

   It is assumed that all thumbnails have the same data size. If not,
   FALSE is returned.

   If a thumbnail has been removed, a slot in the file is opened. This
   slot is filled by copying data from the end of the file and
   adjusting the corresponding Itdb_Image offset pointer. When all
   slots are filled, the file is truncated to the new length.
*/
static gboolean
ithmb_rearrange_existing_thumbnails (Itdb_DB *db,
				     const Itdb_ArtworkFormat *info)
{
    GList *gl;
    GHashTable *filenamehash;
    gboolean result = TRUE;
    GList *thumbs;
    gint i;
    gchar *filename;
    const gchar *mountpoint;

    g_return_val_if_fail (db, FALSE);
    g_return_val_if_fail (info, FALSE);
    g_return_val_if_fail (db_get_device(db), FALSE);

    mountpoint = db_get_mountpoint (db);

    g_return_val_if_fail (mountpoint, FALSE);

    filenamehash = g_hash_table_new_full (g_str_hash, g_str_equal, 
					  g_free, NULL);

    /* Create a hash with all filenames used for thumbnails.
       This will usually be a number of "F%d_%d.ithmb" files. A
       GList is kept with pointers to all images in a given file which
       allows to adjust the offset pointers */
	switch (db->db_type) {
	case DB_TYPE_ITUNES:
		for (gl=db_get_itunesdb(db)->tracks; gl; gl=gl->next)
		{
			Itdb_Thumb *thumb;
			Itdb_Track *track = gl->data;
			g_return_val_if_fail (track, FALSE);

			thumb = itdb_artwork_get_thumb_by_type (track->artwork,
					info->type);
			if (thumb && thumb->filename && (thumb->size != 0))
			{
				filename = itdb_thumb_get_filename (
				        db_get_device(db),
				        thumb);
				if (filename)
				{
					thumbs = g_hash_table_lookup (filenamehash, filename);
					thumbs = g_list_append (thumbs, thumb);
					g_hash_table_insert (filenamehash, filename, thumbs);
				}
			}
		}
		break;
    case DB_TYPE_PHOTO:
	for (gl=db_get_photodb(db)->photos; gl; gl=gl->next)
	{
		Itdb_Thumb *thumb;
		Itdb_Artwork *artwork = gl->data;

		thumb = itdb_artwork_get_thumb_by_type (artwork,
				info->type);
		if (thumb && thumb->filename && (thumb->size != 0))
		{
			filename = itdb_thumb_get_filename (
			        db_get_device (db),
			        thumb);
			if (filename)
			{
				thumbs = g_hash_table_lookup (filenamehash, filename);
				thumbs = g_list_append (thumbs, thumb);
				g_hash_table_insert (filenamehash, filename, thumbs);
			}
		}
	}
	break;
    default:
	g_return_val_if_reached (FALSE);
    }

    /* Check for files present on the iPod but no longer referenced by
       thumbs */

    for (i=0; i<50; ++i)
    {
	filename = ipod_image_get_ithmb_filename (mountpoint,
						  info->correlation_id,
						  i,
						  db->db_type);
	if (g_file_test (filename, G_FILE_TEST_EXISTS))
	{
	    if (g_hash_table_lookup (filenamehash, filename) == NULL)
	    {
		g_hash_table_insert (filenamehash,
				     g_strdup (filename), NULL);
	    }
	}
	g_free (filename);
    }

    /* I'm using the _foreach_remove variant here because the
       thumbnail GList may get changed while calling
       ithumb_rearrange_thumbnail_file but cannot be written back into
       the hash table. Using the _foreach_remove variant here will
       ensure that it will not be possible to access the invalid
       thumbnail GList. The only proper operation after the
       _foreach_remove is a call to g_hash_table_destroy().
       For the same reasons the thumb GList gets free'd in
       ithumb_rearrange_thumbnail_file() */
    g_hash_table_foreach_remove (filenamehash,
				 ithumb_rearrange_thumbnail_file, &result);
    g_hash_table_destroy (filenamehash);

    return result;
}

#endif

G_GNUC_INTERNAL int
itdb_write_ithumb_files (Itdb_DB *db) 
{
#ifdef HAVE_GDKPIXBUF
	GList *writers;
	GList *it;
	Itdb_Device *device;
	const Itdb_ArtworkFormat *format;
	const gchar *mount_point;

	g_return_val_if_fail (db, -1);
	device = db_get_device(db);
	g_return_val_if_fail (device, -1);

	mount_point = db_get_mountpoint (db);
	/* FIXME: support writing to directory rather than writing to
	   iPod */
	if (mount_point == NULL)
	    return -1;
	
	format = itdb_device_get_artwork_formats (device);
	if (format == NULL) {
		return -1;
	}
	writers = NULL;
	while (format->type != -1) {
		iThumbWriter *writer;

		switch (format->type) {
		case ITDB_THUMB_COVER_XLARGE:
		case ITDB_THUMB_COVER_MEDIUM:
		case ITDB_THUMB_COVER_SMEDIUM:
		case ITDB_THUMB_COVER_XSMALL:
		case ITDB_THUMB_COVER_SMALL:
		case ITDB_THUMB_COVER_LARGE:
		case ITDB_THUMB_PHOTO_SMALL:
		case ITDB_THUMB_PHOTO_LARGE:
		case ITDB_THUMB_PHOTO_FULL_SCREEN:
		case ITDB_THUMB_PHOTO_TV_SCREEN:
		        ithmb_rearrange_existing_thumbnails (db, format );
			writer = ithumb_writer_new (mount_point, 
						    format,
						    db->db_type, 
						    device->byte_order);
			if (writer != NULL) {
				writers = g_list_prepend (writers, writer);
			}
			break;
		}
		format++;
	}
	if (writers == NULL) {
		return -1;
	}
	switch (db->db_type) {
	case DB_TYPE_ITUNES:
		for (it = db_get_itunesdb(db)->tracks; it != NULL; it = it->next) {
			Itdb_Track *track;

			track = it->data;
			g_return_val_if_fail (track, -1);

			g_list_foreach (writers, write_thumbnail, track->artwork);
		}
		break;
	case DB_TYPE_PHOTO:
		for (it = db_get_photodb(db)->photos; it != NULL; it = it->next) {
			Itdb_Artwork *photo;

			photo = it->data;
			g_return_val_if_fail (photo, -1);

			g_list_foreach (writers, write_thumbnail, photo);
		}
		break;
	default:
	        g_return_val_if_reached (-1);
	}
	
	g_list_foreach (writers, (GFunc)ithumb_writer_free, NULL);
	g_list_free (writers);

	return 0;
#else
    return -1;
#endif
}

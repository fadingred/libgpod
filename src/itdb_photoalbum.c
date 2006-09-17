#include <config.h>

#include "itdb_private.h"
#include "itdb_device.h"
#include "db-artwork-parser.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <glib/gi18n-lib.h>


/* Set @error with standard error message */
static void error_no_photos_dir (const gchar *mp, GError **error)
{
    gchar *str;

    g_return_if_fail (mp);

    if (error)
    {
	str = g_build_filename (mp, "iPod_Control", "Photos", NULL);
	g_set_error (error,
		     ITDB_FILE_ERROR,
		     ITDB_FILE_ERROR_NOTFOUND,
		     _("Photos directory not found: '%s' (or similar)."),
		     str);
	g_free (str);
    }
}


/**
 * itdb_get_photos_dir:
 * @mountpoint: mountpoint of iPod
 *
 * Retrieve the Photo directory by
 * first calling itdb_get_control_dir() and then adding 'Photos'
 *
 * Return value: path to the Artwork directory or NULL of
 * non-existent. Must g_free() after use.
 */
gchar *itdb_get_photos_dir (const gchar *mountpoint)
{
    gchar *p_ipod[] = {"Photos", NULL};
    /* Use an array with all possibilities, so further extension will
       be easy */
    gchar **paths[] = {p_ipod, NULL};
    gchar ***ptr;
    gchar *result = NULL;

    g_return_val_if_fail (mountpoint, NULL);

    for (ptr=paths; *ptr && !result; ++ptr)
    {
	result = itdb_resolve_path (mountpoint, (const gchar **)*ptr);
    }
    return result;
}

/**
 * itdb_get_photodb_path:
 * @mountpoint: the iPod mountpoint
 *
 * Retrieve a path to the Photo DB
 *
 * Return value: path to the PhotoDB or NULL if non-existent. Must
 * g_free() after use.
 **/
gchar *itdb_get_photodb_path (const gchar *mountpoint)
{
    gchar *photo_dir, *path=NULL;

    g_return_val_if_fail (mountpoint, NULL);

    photo_dir = itdb_get_photos_dir (mountpoint);

    if (photo_dir)
    {
	path = itdb_get_path (photo_dir, "Photo Database");
	g_free (photo_dir);
    }

    return path;
}

/**
 * itdb_get_photos_thumb_dir:
 * @mountpoint: the iPod mountpoint
 *
 * Retrieve the Photo Thumbnail directory by
 * first calling itdb_get_control_dir() and then adding 'Photos/Thumbs'
 *
 * Return value: path to the Artwork directory or NULL of
 * non-existent. Must g_free() after use.
 */
gchar *itdb_get_photos_thumb_dir (const gchar *mountpoint)
{
    gchar *control_dir;
    gchar *result = NULL;
    gchar *dir = "Thumbs";

    g_return_val_if_fail (mountpoint, NULL);
    g_return_val_if_fail (dir, NULL);

    control_dir = itdb_get_photos_dir (mountpoint);
    if (control_dir)
    {
	const gchar *p_dir[] = {NULL, NULL};
	p_dir[0] = dir;
	result = itdb_resolve_path (control_dir, p_dir);
	g_free (control_dir);
    }
    return result;
}


/**
 * itdb_photodb_parse:
 *
 * Parses the photo database of an iPod mounted at @mp.
 *
 * @mp: mountpoint of the iPod
 * @error: will contain the error description when an error occured.
 *
 * Return value: the imported PhotoDB or NULL in case of an error.
 **/
Itdb_PhotoDB *itdb_photodb_parse (const gchar *mp, GError **error)
{
    gchar *photos_dir;
    Itdb_PhotoDB *photodb = NULL;

    photos_dir = itdb_get_photos_dir (mp);

    if (!photos_dir)
    {
	error_no_photos_dir (mp, error);
	return NULL;
    }
    g_free (photos_dir);

    photodb = itdb_photodb_new ();
    itdb_device_set_mountpoint (photodb->device, mp);
    ipod_parse_photo_db (photodb);
    return photodb;
}


/**
 * itdb_photodb_new:
 *
 * Creates a new Itdb_PhotoDB
 *
 * Return value: a newly created Itdb_PhotoDB to be freed with
 * itdb_photodb_free() when it's no longer needed
 **/
Itdb_PhotoDB *itdb_photodb_new (void)
{
    Itdb_PhotoDB *photodb;

    photodb = g_new0 (Itdb_PhotoDB, 1);
    photodb->device = itdb_device_new ();

    return photodb;
}


/** 
 * itdb_photodb_free:
 * @photodb: an #Itdb_PhotoDB
 *
 * Free the memory taken by @photodb. 
 **/
void itdb_photodb_free (Itdb_PhotoDB *photodb)
{
	if (photodb)
	{
		g_list_foreach (photodb->photoalbums,
				(GFunc)(itdb_photodb_photoalbum_free), NULL);
		g_list_free (photodb->photoalbums);
		g_list_foreach (photodb->photos,
				(GFunc)(itdb_artwork_free), NULL);
		g_list_free (photodb->photos);
		itdb_device_free (photodb->device);

		if (photodb->userdata && photodb->userdata_destroy)
		    (*photodb->userdata_destroy) (photodb->userdata);

		g_free (photodb);
	}
}




G_GNUC_INTERNAL gint itdb_get_free_photo_id ( Itdb_PhotoDB *db ) 
{
	gint photo_id = 0;
	GList *it;

	for (it = db->photos; it != NULL; it = it->next) {
		Itdb_Artwork *artwork;

		artwork = (Itdb_Artwork *)it->data;
		if( artwork->id > photo_id )
			photo_id = artwork->id;
	}
	return photo_id + 1;
}

static gint itdb_get_free_photoalbum_id ( Itdb_PhotoDB *db )
{
	gint album_id = 0;
	GList *it;

	for (it = db->photoalbums; it != NULL; it = it->next) {
		Itdb_PhotoAlbum *album;

		album = (Itdb_PhotoAlbum *)it->data;
		if( album->album_id > album_id )
			album_id = album->album_id;
	}
	return album_id + 1;
}

static Itdb_PhotoAlbum *itdb_get_photoalbum ( Itdb_PhotoDB *db, const gchar *albumname )
{
	GList *it;

	if( strcmp( albumname, "master" ) == 0 )
	    return g_list_nth_data (db->photoalbums, 0);

	for (it = db->photoalbums; it != NULL; it = it->next) {
		Itdb_PhotoAlbum *album;

		album = (Itdb_PhotoAlbum *)it->data;
		if( strcmp(album->name, albumname) == 0 )
			return album;
	}
	return (Itdb_PhotoAlbum *)NULL;
}


void itdb_photodb_photoalbum_free (Itdb_PhotoAlbum *pa)
{
    if (pa)
    {
	g_free (pa->name);
	g_list_free (pa->members);

	if (pa->userdata && pa->userdata_destroy)
	    (*pa->userdata_destroy) (pa->userdata);

	g_free (pa);
    }
}

static gboolean itdb_photodb_add_photo_internal (Itdb_PhotoDB *db,
						 const gchar *albumname,
						 const gchar *filename,
						 const guchar *image_data,
						 gsize image_data_len)
{
	gboolean result;
	Itdb_Artwork *artwork;
	Itdb_PhotoAlbum *photoalbum;
	const Itdb_ArtworkFormat *format;
	gint photo_id;

	g_return_val_if_fail (db, FALSE);

	artwork = itdb_artwork_new ();

	photo_id = itdb_get_free_photo_id( db );
	artwork->id = photo_id;

	/* Add a thumbnail for every supported format */
	format = itdb_device_get_artwork_formats(db->device);
	for( result = TRUE; format->type != -1 && result == TRUE; format++)
	{
	    if((format->type == ITDB_THUMB_COVER_SMALL) ||
	       (format->type == ITDB_THUMB_COVER_LARGE))
			continue;
	    if (filename)
	    {
		result = itdb_artwork_add_thumbnail (artwork,
						     format->type,
						     filename);
	    }
	    if (image_data)
	    {
		result = itdb_artwork_add_thumbnail_from_data (artwork,
							       format->type,
							       image_data,
							       image_data_len);
	    }
	}

	if (result != TRUE)
	{
		itdb_artwork_remove_thumbnails (artwork);
		return result;
	}
	db->photos = g_list_append (db->photos, artwork);

	photoalbum = itdb_get_photoalbum( db, albumname );
	if( photoalbum == NULL )
		photoalbum = itdb_photodb_photoalbum_new( db, albumname );
	photoalbum->num_images++;
	photoalbum->members = g_list_append (photoalbum->members, GINT_TO_POINTER(photo_id));

	return result;
}


gboolean itdb_photodb_add_photo (Itdb_PhotoDB *db,
				 const gchar *albumname,
				 const gchar *filename)
{
    g_return_val_if_fail (db, FALSE);
    g_return_val_if_fail (filename, FALSE);

    return itdb_photodb_add_photo_internal (db, albumname, filename,
					    NULL, 0);
}


gboolean itdb_photodb_add_photo_from_data (Itdb_PhotoDB *db,
					   const gchar *albumname,
					   const guchar *image_data,
					   gsize image_data_len)
{
    g_return_val_if_fail (db, FALSE);
    g_return_val_if_fail (image_data, FALSE);

    return itdb_photodb_add_photo_internal (db, albumname, NULL,
					    image_data, image_data_len);
}


gboolean itdb_photodb_remove_photo (Itdb_PhotoDB *db,
		const gint photo_id )
{
	gboolean result = TRUE;
	GList *it;

	g_return_val_if_fail (db, FALSE);

	/* Remove the photo from the image list */
	for (it = db->photos; it != NULL; it = it->next) {
		Itdb_Artwork *artwork;

		artwork = (Itdb_Artwork *)it->data;
		if( artwork->id == photo_id ) {
			db->photos = g_list_remove (db->photos, artwork);
			break;
		}
	}

	/* Remove the photo from any albums containing it */
	for (it = db->photoalbums; it != NULL; it = it->next) {
		Itdb_PhotoAlbum *album;

		album = (Itdb_PhotoAlbum *)it->data;
		album->members = g_list_remove (album->members, GINT_TO_POINTER(photo_id));
		album->num_images = g_list_length( album->members );
	}
	return result;
}

Itdb_PhotoAlbum *itdb_photodb_photoalbum_new (Itdb_PhotoDB *db,
		const gchar *album_name)
{
	Itdb_PhotoAlbum *photoalbum;

	g_return_val_if_fail (db, FALSE);
	g_return_val_if_fail (album_name, FALSE);

	photoalbum = g_new0 (Itdb_PhotoAlbum, 1);
	photoalbum->album_id = itdb_get_free_photoalbum_id( db );
	photoalbum->prev_album_id = photoalbum->album_id - 1;
	photoalbum->name = g_strdup( album_name );
	db->photoalbums = g_list_append (db->photoalbums, photoalbum);

	return photoalbum;
}

/**
 * itdb_photodb_write:
 * @photodb: the #Itdb_PhotoDB to write to disk
 * @error: return location for a #GError or NULL
 *
 * Write out a PhotoDB.
 *
 * FIXME: error is not set yet.
 *
 * Return value: TRUE on success, FALSE on error, in which case @error is
 * set accordingly.
 **/
gboolean itdb_photodb_write (Itdb_PhotoDB *photodb, GError **error)
{
    gboolean result;

    g_return_val_if_fail (photodb, FALSE);
    g_return_val_if_fail (photodb->device, FALSE);

    if (photodb->device->byte_order == 0)
	itdb_device_autodetect_endianess (photodb->device);

    result = ipod_write_photo_db (photodb);

    /* Write SysInfo file if it has changed */
    if (!(*error) && photodb->device->sysinfo_changed)
    {
	itdb_device_write_sysinfo (photodb->device, error);
    }

    if (result == -1)
	return FALSE;
    else
	return TRUE;
}

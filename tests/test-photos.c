/* Copyright (c) 2006, Michael McLellan <mikey@mclellan.org.nz>
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
 * $Id$
 *
 */

#include "itdb.h"

#include <locale.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <glib/gi18n-lib.h>

static void
save_itdb_thumb (Itdb_PhotoDB *itdb, Itdb_Thumb *thumb,
		 const gchar *filename)
{
	GdkPixbuf *pixbuf;
	
	pixbuf = itdb_thumb_get_gdk_pixbuf (itdb->device, thumb);

	if (pixbuf != NULL) {
		gdk_pixbuf_save (pixbuf, filename, "png", NULL, NULL);
		gdk_pixbuf_unref (pixbuf);
	}
}

static void
dump_thumbs (Itdb_PhotoDB *db, Itdb_Artwork *artwork,
	     const gchar *album_name, const gchar *dir)
{
	GList *it;
	gint i = 0;

	for (it = artwork->thumbnails; it != NULL; it = it->next, i++) {
		Itdb_Thumb *thumb;
		gchar *filename, *path;

		thumb = (Itdb_Thumb *)it->data;
		g_return_if_fail (thumb);

		filename = g_strdup_printf ("%s-%d-%d.png",
					    album_name, artwork->id, i );
		path = g_build_filename (dir, filename, NULL);
		g_free (filename);
		save_itdb_thumb (db, thumb, path);
		g_free (path);
	}
}

static void
dump_artwork (Itdb_PhotoDB *db, gint photo_id,
	      const gchar *album_name, const gchar *dir)
{
	GList *it;

	for (it = db->photos; it != NULL; it = it->next) {
		Itdb_Artwork *artwork;

		artwork = (Itdb_Artwork *)it->data;
		g_return_if_fail (artwork);
		if( artwork->id == photo_id ) {
			dump_thumbs (db, artwork, album_name, dir);
			break;
		}
	}
}

static void
dump_albums (Itdb_PhotoDB *db, const gchar *dir)
{
	GList *it;
	
	for (it = db->photoalbums; it != NULL; it = it->next) {
		Itdb_PhotoAlbum *album;
		GList *it2;

		album = (Itdb_PhotoAlbum *)it->data;
		g_return_if_fail (album);

		for (it2 = album->members; it2 != NULL; it2 = it2->next) {
		        gint photo_id = GPOINTER_TO_INT(it2->data);
			dump_artwork (db, photo_id, album->name, dir);
		}
	}
}

int
main (int argc, char **argv)
{
    GError *error = NULL;
    Itdb_PhotoDB *db;
    gint i; 

    if (argc < 4) {
	g_print (_("Usage to add photos:\n"));
        g_print (_("%s <mountpoint> <albumname> <filename(s)>\n"), argv[0]);
        g_print (_("albumname should be set to 'master' to add photos to the master photo album\n"));
	g_print (_("\n"));
	g_print (_("Usage to dump all photos to <output_dir>:\n"));
	g_print (_("%s dump <mountpoint> <output_dir>\n"), argv[0]);
	g_print (_("\n"));
	g_print (_("Usage to delete a photo album:\n"));
	g_print (_("%s delete <mountpoint> <albumname>\n"), argv[0]);
	g_print (_("\n"));
	g_print (_("Usage to rename a photo album:\n"));
	g_print (_("%s rename <mountpoint> <albumname> <new_albumname>\n"), argv[0]);
        return 1;
    }
    setlocale (LC_ALL, "");
    g_type_init ();

    if (strcmp (argv[1], "dump") == 0)
    {
	if (!g_file_test (argv[3], G_FILE_TEST_EXISTS))
	{
	    if (mkdir (argv[3], 0777) == -1)
	    {
		g_print (_("Error creating '%s' (mkdir)\n"), argv[3]);
		return 1;
	    }
	}
	if (!g_file_test (argv[3], G_FILE_TEST_IS_DIR))
	{
	    g_print (_("Error: '%s' is not a directory\n"), argv[3]);
	    return 1;
	}

	db = itdb_photodb_parse (argv[2], &error);
	if (db == NULL)
	{
	    if (error)
	    {
		g_print (_("Error reading iPod photo database (%s).\n"), error->message);
		g_error_free (error);
		error = NULL;
	    }
	    else
	    {
		g_print (_("Error reading iPod photo database.\n"));
	    }
	    return 1;
	}
        dump_albums (db, argv[3]);
	itdb_photodb_free (db);
    }
    else if (strcmp (argv[1], "delete") == 0)
    {
        db = itdb_photodb_parse (argv[2], &error);
        if (db == NULL)
        {
            if (error)
            {
                g_print (_("Error reading iPod photo database.(%s)\n"), error->message);
                g_error_free (error);
                error = NULL;
            }
            else
                g_print (_("Error reading iPod photo database.\n"));

            return 1;
        }
        itdb_photodb_remove_photoalbum( db, argv[3] );
        g_print (_("Writing to the photo database.\n"));
	itdb_photodb_write (db, &error);
        g_print (_("Freeing the photo database.\n"));
        itdb_photodb_free (db);
    }
    else if (strcmp (argv[1], "rename") == 0)
    {
        db = itdb_photodb_parse (argv[2], &error);
        if (db == NULL)
        {
            if (error)
            {
                g_print (_("Error reading iPod photo database.(%s)\n"), error->message);
                g_error_free (error);
                error = NULL;
            }
            else
                g_print (_("Error reading iPod photo database.\n"));

            return 1;
        }
        itdb_photodb_rename_photoalbum( db, argv[3], argv[4] );
        g_print (_("Writing to the photo database.\n"));
	itdb_photodb_write (db, &error);
        g_print (_("Freeing the photo database.\n"));
        itdb_photodb_free (db);
    }
    else
    {
	db = itdb_photodb_parse (argv[1], &error);
	if (db == NULL)
	{
	    if (error)
	    {
		g_print (_("Error reading iPod photo database (%s).\nWill attempt to create a new database.\n"), error->message);
		g_error_free (error);
		error = NULL;
	    }
	    else
	    {
		g_print (_("Error reading iPod photo database, will attempt to create a new database\n"));
	    }
	    db = itdb_photodb_new ();
	    itdb_device_set_mountpoint (db->device, argv[1]);
	}
	for (i=3; i<argc; ++i)
	{
	    itdb_photodb_add_photo (db, argv[2], argv[i]);
	}

	itdb_photodb_write (db, NULL);
	itdb_photodb_free (db);
    }

    return 0;
}


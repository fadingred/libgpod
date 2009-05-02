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

static void usage (int argc, char **argv)
{
/*    gchar *name = argv[0];*/
    gchar *name = "test-photos";

    g_print (_("Usage to add photos:\n  %s add <mountpoint> <albumname> [<filename(s)>]\n  <albumname> should be set to 'NULL' to add photos to the master photo album\n  (Photo Library) only. If you don't specify any filenames an empty album will\n  be created.\n"), name);
    g_print (_("Usage to dump all photos to <output_dir>:\n  %s dump <mountpoint> <output_dir>\n"), name);
    g_print (_("Usage to list all photos IDs to stdout:\n  %s list <mountpoint>\n"), name);
    g_print (_("Usage to remove photo IDs from Photo Library:\n  %s remove <mountpoint> <albumname> [<ID(s)>]\n  <albumname> should be set to 'NULL' to remove photos from the iPod\n  altogether. If you don't specify any IDs, the photoalbum will be removed\n  instead.\n  WARNING: IDs may change when writing the PhotoDB file.\n"), name);
}

/* Retrieve the photo whose ID is @id */
static Itdb_Artwork *get_photo_by_id (Itdb_PhotoDB *db, guint32 id)
{
    GList *gl;

    g_return_val_if_fail (db, NULL);

    for (gl=db->photos; gl; gl=gl->next)
    {
	Itdb_Artwork *photo = gl->data;
	g_return_val_if_fail (photo, NULL);

	if (photo->id == id) return photo;
    }
    return NULL;
}

static void
dump_thumbs (Itdb_PhotoDB *db, Itdb_Artwork *artwork,
	     const gchar *album_name, const gchar *dir)
{
	GList *it;
	gint i = 0;
        GList *thumbnails;

        thumbnails = itdb_thumb_to_pixbufs (db->device, artwork->thumbnail);
	for (it = thumbnails; it != NULL; it = it->next, i++) {
		gchar *filename, *path;
                GdkPixbuf *pixbuf;

                pixbuf = GDK_PIXBUF (it->data);

		g_return_if_fail (pixbuf);

		filename = g_strdup_printf ("%s-%d-%d.png",
					    album_name, artwork->id, i );
		path = g_build_filename (dir, filename, NULL);
		g_free (filename);
		gdk_pixbuf_save (pixbuf, path, "png", NULL, NULL);
		gdk_pixbuf_unref (pixbuf);
		g_free (path);
	}
        g_list_free (thumbnails);
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
		        Itdb_Artwork *photo = it2->data;
			dump_thumbs (db, photo, album->name, dir);
		}
	}
}


static int do_dump (int argc, char **argv)
{
    GError *error = NULL;
    Itdb_PhotoDB *db;

    if (argc != 4)
    {
	g_print (_("Wrong number of command line arguments.\n"));
	usage (argc, argv);
	return 1;
    }

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
    return 0;
}

static int do_list (int argc, char **argv)
{
    GError *error = NULL;
    Itdb_PhotoDB *db;
    GList *gl_album;


    if (argc != 3)
    {
	g_print (_("Insufficient number of command line arguments.\n"));
	usage (argc, argv);
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

    for (gl_album=db->photoalbums; gl_album; gl_album=gl_album->next)
    {
	GList *gl_photo;
	Itdb_PhotoAlbum *album = gl_album->data;
	g_return_val_if_fail (album, 1);

	g_print ("%s: ", album->name?album->name:_("<Unnamed>"));

	for (gl_photo=album->members; gl_photo; gl_photo=gl_photo->next)
	{
	    Itdb_Artwork *photo = gl_photo->data;
	    g_return_val_if_fail (photo, 1);

	    g_print ("%d ", photo->id);
	}
	if (g_list_length (album->members) > 0)
	{
	    g_print ("\n");
	}
	else
	{
	    g_print (_("<No members>\n"));
	}
    }
    itdb_photodb_free (db);
    return 0;
}


static int do_add (int argc, char **argv)
{
    GError *error = NULL;
    Itdb_PhotoAlbum *album = NULL;
    Itdb_PhotoDB *db;
    gint i; 

    if (argc < 4)
    {
	g_print (_("Insufficient number of command line arguments.\n"));
	usage (argc, argv);
	return 1;
    }

    db = itdb_photodb_parse (argv[2], &error);
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
	db = itdb_photodb_create (argv[2]);
    }

    /* Find or create specified photoalbum */
    if (strcmp (argv[3], "NULL") != 0)
    {
	album = itdb_photodb_photoalbum_by_name (db, argv[3]);
	if (!album)
	{
	    album = itdb_photodb_photoalbum_create (db, argv[3], -1);
	}
    }

    for (i=4; i<argc; ++i)
    {
	Itdb_Artwork *photo;
	
	photo = itdb_photodb_add_photo (db, argv[i],
					-1, GDK_PIXBUF_ROTATE_NONE, &error);
	if (photo == NULL)
	{
	    if (error)
	    {
		g_print (_("Error adding photo (%s) to photo database: %s\n"),
			 argv[i], error->message);
		g_error_free (error);
		error = NULL;
	    }
	}
	else
	{
	    if (album)
	    {
		itdb_photodb_photoalbum_add_photo (db, album, photo, -1);
	    }
	}
    }

    itdb_photodb_write (db, NULL);
    itdb_photodb_free (db);
    return 0;
}


static int do_remove (int argc, char **argv)
{
    GError *error = NULL;
    Itdb_PhotoDB *db;
    Itdb_PhotoAlbum *album = NULL;

    if (argc < 4)
    {
	g_print (_("Insufficient number of command line arguments.\n"));
	usage (argc, argv);
	return 1;
    }

    db = itdb_photodb_parse (argv[2], &error);
    if (db == NULL)
    {
	if (error)
	{
	    g_print (_("Error reading iPod photo database (%s).\n"),
		     error->message);
	    g_error_free (error);
	    error = NULL;
	}
	else
	{
	    g_print (_("Error reading iPod photo database.\n"));
	}
	return 1;
    }

    /* Find specified photoalbum */
    if (strcmp (argv[3], "NULL") != 0)
    {
	album = itdb_photodb_photoalbum_by_name (db, argv[3]);
	if (!album)
	{
	    g_print (_("Specified album '%s' not found. Aborting.\n"),
		     argv[3]);
	    itdb_photodb_free (db);
	    return 1;
	}
    }

    if (argc == 4)
    {
	/* Remove photoalbum altogether, but preserve pics */
	if (album == NULL)
	{
	    g_print (_("Cannot remove Photo Library playlist. Aborting.\n"));
	    itdb_photodb_free (db);
	    return 1;
	}
	itdb_photodb_photoalbum_remove (db, album, FALSE);
    }
    else
    {
	/* Remove specified pictures */
	int i;
	for (i=4; i<argc; ++i)
	{
	    Itdb_Artwork *photo;
	    guint32 id;

	    id = g_strtod (argv[i], NULL);

	    photo = get_photo_by_id (db, id);

	    if (photo == NULL)
	    {
		g_print (_("Warning: could not find photo with ID <%d>. Skipping...\n"),
			 id);
	    }
	    else
	    {
		itdb_photodb_remove_photo (db, album, photo);
	    }
	}
    }

    itdb_photodb_write (db, NULL);
    itdb_photodb_free (db);
    return 0;
}



int
main (int argc, char **argv)
{
    if (argc < 2)
    {
	g_print (_("Insufficient number of command line arguments.\n"));
	usage (argc, argv);
	return 1;
    }
    setlocale (LC_ALL, "");
    g_type_init ();

    if (strcmp (argv[1], "dump") == 0)
    {
	return do_dump (argc, argv);
    }
    if (strcmp (argv[1], "add") == 0)
    {
	return do_add (argc, argv);
    }
    if (strcmp (argv[1], "list") == 0)
    {
	return do_list (argc, argv);
    }
    if (strcmp (argv[1], "remove") == 0)
    {
	return do_remove (argc, argv);
    }

    g_print (_("Unknown command '%s'\n"), argv[1]);
    usage (argc, argv);
    return 1;
}


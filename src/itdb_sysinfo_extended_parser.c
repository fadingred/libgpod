/*
|  Copyright (C) 2008 Christophe Fergeau <teuf@gnome.org>
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

/*
 * The code in this file is used to convert the output of the plist parser
 * (a GValue *) and to convert it to data structures usable by libgpod.
 * This is that code which interprets the generic parsed plist data as a
 * SysInfoExtended file. The SysInfoExtended data is used to fill a
 * SysInfoIpodProperties structure and several Itdb_ArtworkFormat structs.
 *
 * I tried to make the filling of the structures quite generic, if some
 * field isn't parsed (which is quite possible since I gathered the various
 * fields names using a few sample files), all is needed to add it is to
 * add a field to the appropriate structure (SysInfoIpodProperties or
 * Itdb_ArtworkFormat) and to add that field to the appropriate
 * _fields_mapping structure. Those _fields_mapping structures are then
 * used to convert from a GValue to the struct, but they are also used by
 * the _dump and _free functions, so there's no need to modify them when
 * you add a new field.
 *
 * If DEBUG_PARSING is defined when building that file, the fields that
 * were found in the SysInfoExtended file but which were not used to build
 * the data structures defined in that file will be dumped to stdout. It's
 * normal to get a few unhandled fields, I left out on purpose a few <dict>
 * because I was too lazy to parse them ;)
 */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <glib-object.h>
#include <stdlib.h>
#include <string.h>

#include "itdb_device.h"
#include "itdb_plist.h"
#include "itdb_sysinfo_extended_parser.h"

struct _SysInfoIpodProperties {
        char *build_id;
        char *connected_bus;
        gint max_transfer_speed;
        gint family_id;
        char *product_type;
        char *firewire_guid;
        char *firewire_version;
        GList *artwork_formats;
        GList *photo_formats;
        GList *chapter_image_formats;
        gboolean podcasts_supported;
        char *min_itunes_version;
        gboolean playlist_folders_supported;
        char *serial_number;
        gint updater_family_id;
        char *visible_build_id;
        gint oem_id;
        gint oem_u;
        gint db_version;
        int shadowdb_version;
        char *min_build_id;
        char *language;
        gboolean voice_memos_supported;
        gint update_method;
        gint max_fw_blocks;
        gint fw_part_size;
        gboolean auto_reboot_after_firmware_update;
        char *volume_format;
        gboolean forced_disk_mode;
        gboolean bang_folder;
        gboolean corrupt_data_partition;
        gboolean corrupt_firmware_partition;
        gboolean can_flash_backlight;
        gboolean can_hibernate;
        gboolean came_with_cd;
        gboolean supports_sparse_artwork;
        gint max_thumb_file_size;
        gint ram;
        gint hotplug_state;
        gint battery_poll_interval;
        gboolean sort_fields_supported;
        gboolean vcard_with_jpeg_supported;
        gint max_file_size_in_gb;
        gint max_tracks;
        gint games_platform_id;
        gint games_platform_version;
        gint rental_clock_bias;
	gboolean sqlite_db;
};

static gint64 get_int64 (GHashTable *dict, const char *key)
{
        GValue *val;

        val = g_hash_table_lookup (dict, key);
        if (val == NULL) {
                return 0;
        }
        if (!G_VALUE_HOLDS_INT64 (val)) {
                return 0;
        }
        return g_value_get_int64 (val);
}

static gdouble get_double (GHashTable *dict, const char *key)
{
        GValue *val;

        val = g_hash_table_lookup (dict, key);
        if (val == NULL) {
                return 0;
        }
        if (!G_VALUE_HOLDS_DOUBLE (val)) {
                return 0;
        }
        return g_value_get_double (val);
}

static gboolean get_boolean (GHashTable *dict, const char *key)
{
        GValue *val;

        val = g_hash_table_lookup (dict, key);
        if (val == NULL) {
                return FALSE;
        }
        if (!G_VALUE_HOLDS_BOOLEAN (val)) {
                return FALSE;
        }
        return g_value_get_boolean (val);
}

static char *get_string (GHashTable *dict, const char *key)
{
        GValue *val;

        val = g_hash_table_lookup (dict, key);
        if (val == NULL) {
                return NULL;
        }
        if (!G_VALUE_HOLDS_STRING (val)) {
                return NULL;
        }
        return g_value_dup_string (val);
}

struct _DictFieldMapping {
    const char* name;
    GType type;
    guint offset;
};

typedef struct _DictFieldMapping DictFieldMapping;
static const DictFieldMapping sysinfo_ipod_properties_fields_mapping[] = {
    { "BuildID",                       G_TYPE_STRING,  G_STRUCT_OFFSET (SysInfoIpodProperties, build_id) },
    { "ConnectedBus",                  G_TYPE_STRING,  G_STRUCT_OFFSET (SysInfoIpodProperties,  connected_bus) },
    { "MaxTransferSpeed",              G_TYPE_INT64,   G_STRUCT_OFFSET (SysInfoIpodProperties, max_transfer_speed) },
    { "FamilyID",                      G_TYPE_INT64,   G_STRUCT_OFFSET (SysInfoIpodProperties, family_id) },
    { "ProductType",                   G_TYPE_STRING,  G_STRUCT_OFFSET (SysInfoIpodProperties, product_type) },
    { "FireWireGUID",                  G_TYPE_STRING,  G_STRUCT_OFFSET (SysInfoIpodProperties, firewire_guid) },
    { "FireWireVersion",               G_TYPE_STRING,  G_STRUCT_OFFSET (SysInfoIpodProperties, firewire_version) },
    { "PodcastsSupported",             G_TYPE_BOOLEAN, G_STRUCT_OFFSET (SysInfoIpodProperties, podcasts_supported) },
    { "MinITunesVersion",              G_TYPE_STRING,  G_STRUCT_OFFSET (SysInfoIpodProperties, min_itunes_version) },
    { "PlaylistFoldersSupported",      G_TYPE_BOOLEAN, G_STRUCT_OFFSET (SysInfoIpodProperties, playlist_folders_supported) },
    { "SerialNumber",                  G_TYPE_STRING,  G_STRUCT_OFFSET (SysInfoIpodProperties, serial_number) },
    { "UpdaterFamilyID",               G_TYPE_INT64,   G_STRUCT_OFFSET (SysInfoIpodProperties, updater_family_id) },
    { "VisibleBuildID",                G_TYPE_STRING,  G_STRUCT_OFFSET (SysInfoIpodProperties, visible_build_id) },
    { "OEMID",                         G_TYPE_INT64,   G_STRUCT_OFFSET (SysInfoIpodProperties, oem_id) },
    { "OEMU",                          G_TYPE_INT64,   G_STRUCT_OFFSET (SysInfoIpodProperties, oem_u) },
    { "DBVersion",                     G_TYPE_INT64,   G_STRUCT_OFFSET (SysInfoIpodProperties, db_version) },
    { "MinBuildID",                    G_TYPE_STRING,  G_STRUCT_OFFSET (SysInfoIpodProperties, min_build_id) },
    { "Language",                      G_TYPE_STRING,  G_STRUCT_OFFSET (SysInfoIpodProperties, language) },
    { "VoiceMemosSupported",           G_TYPE_BOOLEAN, G_STRUCT_OFFSET (SysInfoIpodProperties, voice_memos_supported) },
    { "UpdateMethod",                  G_TYPE_INT64,   G_STRUCT_OFFSET (SysInfoIpodProperties, update_method) },
    { "MaxFWBlocks",                   G_TYPE_INT64,   G_STRUCT_OFFSET (SysInfoIpodProperties, max_fw_blocks) },
    { "FWPartSize",                    G_TYPE_INT64,   G_STRUCT_OFFSET (SysInfoIpodProperties, fw_part_size) },
    { "AutoRebootAfterFirmwareUpdate", G_TYPE_BOOLEAN, G_STRUCT_OFFSET (SysInfoIpodProperties, auto_reboot_after_firmware_update) },
    { "VolumeFormat",                  G_TYPE_STRING,  G_STRUCT_OFFSET (SysInfoIpodProperties, volume_format) },
    { "ForcedDiskMode",                G_TYPE_BOOLEAN, G_STRUCT_OFFSET (SysInfoIpodProperties, forced_disk_mode) },
    { "BangFolder",                    G_TYPE_BOOLEAN, G_STRUCT_OFFSET (SysInfoIpodProperties, bang_folder) },
    { "CorruptDataPartition",          G_TYPE_BOOLEAN, G_STRUCT_OFFSET (SysInfoIpodProperties, corrupt_data_partition) },
    { "CorruptFirmwarePartition",      G_TYPE_BOOLEAN, G_STRUCT_OFFSET (SysInfoIpodProperties, corrupt_firmware_partition) },
    { "CanFlashBacklight",             G_TYPE_BOOLEAN, G_STRUCT_OFFSET (SysInfoIpodProperties, can_flash_backlight) },
    { "CanHibernate",                  G_TYPE_BOOLEAN, G_STRUCT_OFFSET (SysInfoIpodProperties, can_hibernate) },
    { "CameWithCD",                    G_TYPE_BOOLEAN, G_STRUCT_OFFSET (SysInfoIpodProperties, came_with_cd) },
    { "SupportsSparseArtwork",         G_TYPE_BOOLEAN, G_STRUCT_OFFSET (SysInfoIpodProperties, supports_sparse_artwork) },
    { "MaxThumbFileSize",              G_TYPE_INT64,   G_STRUCT_OFFSET (SysInfoIpodProperties, max_thumb_file_size) },
    { "RAM",                           G_TYPE_INT64,   G_STRUCT_OFFSET (SysInfoIpodProperties, ram) },
    { "HotPlugState",                  G_TYPE_INT64,   G_STRUCT_OFFSET (SysInfoIpodProperties, hotplug_state) },
    { "BatteryPollInterval",           G_TYPE_INT64,   G_STRUCT_OFFSET (SysInfoIpodProperties, battery_poll_interval) },
    { "SortFieldsSupported",           G_TYPE_BOOLEAN, G_STRUCT_OFFSET (SysInfoIpodProperties, sort_fields_supported) },
    { "vCardWithJPEGSupported",        G_TYPE_BOOLEAN, G_STRUCT_OFFSET (SysInfoIpodProperties, vcard_with_jpeg_supported) },
    { "MaxFileSizeInGB",               G_TYPE_INT64,   G_STRUCT_OFFSET (SysInfoIpodProperties, max_file_size_in_gb) },
    { "MaxTracks",                     G_TYPE_INT64,   G_STRUCT_OFFSET (SysInfoIpodProperties, max_tracks) },
    { "GamesPlatformID",               G_TYPE_INT64,   G_STRUCT_OFFSET (SysInfoIpodProperties, games_platform_id) },
    { "GamesPlatformVersion",          G_TYPE_INT64,   G_STRUCT_OFFSET (SysInfoIpodProperties, games_platform_version) },
    { "RentalClockBias",               G_TYPE_INT64,   G_STRUCT_OFFSET (SysInfoIpodProperties, rental_clock_bias) },
    { "SQLiteDB",                      G_TYPE_BOOLEAN, G_STRUCT_OFFSET (SysInfoIpodProperties, sqlite_db) },
    { "ShadowDBVersion",               G_TYPE_INT64,   G_STRUCT_OFFSET (SysInfoIpodProperties, shadowdb_version) },
    { NULL,                            G_TYPE_NONE,    0 }
};

static const DictFieldMapping sysinfo_image_format_fields_mapping[] = {
    { "FormatId",         G_TYPE_INT64,   G_STRUCT_OFFSET (Itdb_ArtworkFormat, format_id) },
    { "DisplayWidth",     G_TYPE_INT64,   G_STRUCT_OFFSET (Itdb_ArtworkFormat, display_width) },
    { "RenderWidth",      G_TYPE_INT64,   G_STRUCT_OFFSET (Itdb_ArtworkFormat, width) },
    { "RenderHeight",     G_TYPE_INT64,   G_STRUCT_OFFSET (Itdb_ArtworkFormat, height) },
/*  PixelFormat needs to be converted to ItdbThumbFormat, this is special-cased
 *  in g_value_to_image_format */
/*  { "PixelFormat",      G_TYPE_STRING,  G_STRUCT_OFFSET (Itdb_ArtworkFormat, format) },*/
    { "Interlaced",       G_TYPE_BOOLEAN, G_STRUCT_OFFSET (Itdb_ArtworkFormat, interlaced) },
    { "Crop",             G_TYPE_BOOLEAN, G_STRUCT_OFFSET (Itdb_ArtworkFormat, crop) },
/* AlignRowBytes is an older version of RowBytesAlignment, it's equivalent
 * to a value of 4 for RowBytesAlignemnt */
/*    { "AlignRowBytes",    G_TYPE_BOOLEAN, G_STRUCT_OFFSET (Itdb_ArtworkFormat, align_row_bytes) },*/
    { "Rotation",         G_TYPE_INT64,   G_STRUCT_OFFSET (Itdb_ArtworkFormat, rotation) },
/* BackColor needs to be converted to a gint, this is special-cased
 * in g_value_to_image_format */
/*    { "BackColor",        G_TYPE_INT64   G_STRUCT_OFFSET (Itdb_ArtworkFormat, back_color) }, */
    { "ColorAdjustment",  G_TYPE_INT64,   G_STRUCT_OFFSET (Itdb_ArtworkFormat, color_adjustment) },
    { "GammaAdjustment",  G_TYPE_DOUBLE,  G_STRUCT_OFFSET (Itdb_ArtworkFormat, gamma) },
    { "AssociatedFormat", G_TYPE_INT64,   G_STRUCT_OFFSET (Itdb_ArtworkFormat, associated_format) },
    { "RowBytesAlignment", G_TYPE_INT64,   G_STRUCT_OFFSET (Itdb_ArtworkFormat, row_bytes_alignment) },
    { NULL,               G_TYPE_NONE,    0 }
};

#ifdef DEBUG_PARSING
static void dump_key_name (gpointer key, gpointer val, gpointer data)
{
    g_print ("%s ", (char *)key);
}
#endif

static void dict_to_struct (GHashTable *dict,
                            const DictFieldMapping *mapping,
                            void *struct_ptr)
{
    const DictFieldMapping *it = mapping;
    g_return_if_fail (it != NULL);
    while (it->name != NULL) {
        switch (it->type) {
            case G_TYPE_INT64: {
                gint *field;
                field = G_STRUCT_MEMBER_P (struct_ptr, it->offset);
                *field = get_int64 (dict, it->name);
                break;
            }

            case G_TYPE_BOOLEAN: {
                gboolean *field;
                field = G_STRUCT_MEMBER_P (struct_ptr, it->offset);
                *field = get_boolean (dict, it->name);
                break;
            }

            case G_TYPE_STRING: {
                gchar **field;
                field = G_STRUCT_MEMBER_P (struct_ptr, it->offset);
                *field = get_string (dict, it->name);
                break;
            }

            case G_TYPE_DOUBLE: {
                gdouble *field;
                field = G_STRUCT_MEMBER_P (struct_ptr, it->offset);
                *field = get_double (dict, it->name);
                break;
            }
        }
        g_hash_table_remove (dict, it->name);
        ++it;
    }
#ifdef DEBUG_PARSING
    if (g_hash_table_size (dict) != 0) {
        g_print ("Unused keys:\n");
        g_hash_table_foreach (dict, dump_key_name, NULL);
        g_print ("\n");
    }
#endif
}

static void free_struct (const DictFieldMapping *mapping,
                         void *struct_ptr)
{
    const DictFieldMapping *it = mapping;
    while (it->name != NULL) {
        if (it->type == G_TYPE_STRING) {
                gchar **field;
                field = G_STRUCT_MEMBER_P (struct_ptr, it->offset);
                g_free (*field);
        }
        ++it;
    }
    g_free (struct_ptr);
}

static void free_image_format (Itdb_ArtworkFormat *format)
{
    free_struct (sysinfo_image_format_fields_mapping, format);
}

void itdb_sysinfo_properties_free (SysInfoIpodProperties *props)
{
    g_list_foreach (props->artwork_formats, (GFunc)free_image_format, NULL);
    g_list_free (props->artwork_formats);
    g_list_foreach (props->photo_formats, (GFunc)free_image_format, NULL);
    g_list_free (props->photo_formats);
    g_list_foreach (props->chapter_image_formats, (GFunc)free_image_format, NULL);
    g_list_free (props->chapter_image_formats);
    free_struct (sysinfo_ipod_properties_fields_mapping, props);
}

static void dump_struct (const DictFieldMapping *mapping,
                         void *struct_ptr)
{
    const DictFieldMapping *it = mapping;
    g_return_if_fail (it != NULL);
    while (it->name != NULL) {
        switch (it->type) {
            case G_TYPE_INT64: {
                gint *field;
                field = G_STRUCT_MEMBER_P (struct_ptr, it->offset);
                g_print ("%s: %d\n", it->name, *field);
                break;
            }

            case G_TYPE_BOOLEAN: {
                gboolean *field;
                field = G_STRUCT_MEMBER_P (struct_ptr, it->offset);
                g_print ("%s: %s\n", it->name, (*field)?"true":"false");
                break;
            }

            case G_TYPE_STRING: {
                gchar **field;
                field = G_STRUCT_MEMBER_P (struct_ptr, it->offset);
                g_print ("%s: %s\n", it->name, *field);
                break;
            }

            case G_TYPE_DOUBLE: {
                gdouble *field;
                field = G_STRUCT_MEMBER_P (struct_ptr, it->offset);
                g_print ("%s: %f\n", it->name, *field);
                break;
            }
        }
        ++it;
    }
}

static void dump_image_format (Itdb_ArtworkFormat *format)
{
    dump_struct (sysinfo_image_format_fields_mapping, format);
    g_print ("PixelFormat: %d\n", format->format); 
}

void itdb_sysinfo_properties_dump (SysInfoIpodProperties *props)
{
    dump_struct (sysinfo_ipod_properties_fields_mapping, props);
    g_list_foreach (props->artwork_formats, (GFunc)dump_image_format, NULL);
    g_list_foreach (props->photo_formats, (GFunc)dump_image_format, NULL);
    g_list_foreach (props->chapter_image_formats, (GFunc)dump_image_format, NULL);
}

static gboolean
set_pixel_format (Itdb_ArtworkFormat *img_spec, GHashTable *dict)
{
    char *pixel_format;

    pixel_format = get_string (dict, "PixelFormat");
    if (pixel_format == NULL) {
        return FALSE;
    }

    if (strcmp (pixel_format, "32767579" /* 2vuy */) == 0) {
        img_spec->format = THUMB_FORMAT_UYVY_BE;
    } else if (strcmp (pixel_format, "42353635" /* B565 */) == 0) {
        img_spec->format = THUMB_FORMAT_RGB565_BE;
    } else if (strcmp (pixel_format, "4C353635" /* L565 */) == 0) {
        img_spec->format = THUMB_FORMAT_RGB565_LE;
    } else if (strcmp (pixel_format, "79343230" /* y420 */) == 0) {
        img_spec->format = THUMB_FORMAT_I420_LE;
    } else if (strcmp (pixel_format, "4C353535" /* L555 */) == 0) {
	if (g_hash_table_lookup (dict, "PixelOrder") != NULL) {
	    img_spec->format = THUMB_FORMAT_REC_RGB555_LE;
	} else {
	    img_spec->format = THUMB_FORMAT_RGB555_LE;
	}
    } else {
        g_free (pixel_format);
        return FALSE;
    }
    g_hash_table_remove (dict, "PixelFormat");
    g_hash_table_remove (dict, "PixelOrder");
    g_free (pixel_format);
    return TRUE;
}

static void set_back_color (Itdb_ArtworkFormat *img_spec, GHashTable *dict)
{
    char *back_color_str;
    guint back_color;
    gint i;

    memset (img_spec->back_color, 0, sizeof (img_spec->back_color));;
    back_color_str = get_string (dict, "BackColor");
    if (back_color_str == NULL) {
        return;
    }
    back_color = strtoul (back_color_str, NULL, 16);
    for (i = 3; i >= 0; i--) {
        img_spec->back_color[(guchar)i] = back_color & 0xff;
        back_color = back_color >> 8;
    }
    g_hash_table_remove (dict, "BackColor");
    g_free (back_color_str);
}

static Itdb_ArtworkFormat *g_value_to_image_format (GValue *value)
{
    GHashTable *dict;
    Itdb_ArtworkFormat *img_spec;

    g_return_val_if_fail (G_VALUE_HOLDS (value, G_TYPE_HASH_TABLE), NULL);
    dict = g_value_get_boxed (value);
    g_return_val_if_fail (dict != NULL, NULL);

    img_spec = g_new0 (Itdb_ArtworkFormat, 1);
    if (img_spec == NULL) {
        return NULL;
    }

    if (!set_pixel_format (img_spec, dict)) {
        g_free (img_spec);
        return NULL;
    }
    set_back_color (img_spec, dict);

    dict_to_struct (dict, sysinfo_image_format_fields_mapping, img_spec);

    if (get_boolean (dict, "AlignRowBytes")
            && (img_spec->row_bytes_alignment == 0)) {
        /* at least the nano3g has the AlignRowBytes key with no
         * RowBytesAlignment key.
         */
        img_spec->row_bytes_alignment = 4;
    }

    return img_spec;
}

static GList *parse_one_formats_list (GHashTable *sysinfo_dict, 
                                      const char *key)
{
    GValue *to_parse;
    GList *formats = NULL;
    GValueArray *array;
    gint i;

    to_parse = g_hash_table_lookup (sysinfo_dict, key);
    if (to_parse == NULL) {
        return NULL;
    }
    if (!G_VALUE_HOLDS (to_parse, G_TYPE_VALUE_ARRAY)) {
        return NULL;
    }
    array = (GValueArray*)g_value_get_boxed (to_parse);
    for (i = 0; i < array->n_values; i++) {
        Itdb_ArtworkFormat *format;
	/* SysInfoExtended on the iPhone has <string> fields in the artwork
	 * format array in addition to the hash we parse
	 */
	if (!G_VALUE_HOLDS (g_value_array_get_nth (array, i), G_TYPE_HASH_TABLE)) {
	    continue;
	}
	format = g_value_to_image_format (g_value_array_get_nth (array, i));
	if (format != NULL) {
		formats = g_list_prepend (formats, format);
	}
    } 
    g_hash_table_remove (sysinfo_dict, key);
    return formats;
}

static SysInfoIpodProperties *g_value_to_ipod_properties (GValue *value)
{
    GHashTable *sysinfo_dict;
    SysInfoIpodProperties *props;

    g_return_val_if_fail (G_VALUE_HOLDS (value, G_TYPE_HASH_TABLE), NULL);
    sysinfo_dict = g_value_get_boxed (value);

    props = g_new0 (SysInfoIpodProperties, 1);
    props->artwork_formats = parse_one_formats_list (sysinfo_dict,
                                                     "AlbumArt");
    props->photo_formats = parse_one_formats_list (sysinfo_dict,
                                                   "ImageSpecifications");
    props->chapter_image_formats = parse_one_formats_list (sysinfo_dict,
                                                           "ChapterImageSpecs");
    dict_to_struct (sysinfo_dict,
                    sysinfo_ipod_properties_fields_mapping,
                    props);

    return props;
}

/**
 * itdb_sysinfo_extended_parse:
 * @filename:   name of the SysInfoExtended file to parse
 * @error:      return location for a #GError
 *
 * itdb_sysinfo_extended_parse() parses a SysInfoExtended file into a
 * #SysInfoIpodProperties structure. This structure contains a lot of
 * information about the iPod properties (artwork format supported,
 * podcast capabilities, ...) which can be queried using the
 * appropriate accessors.
 *
 * Returns: a newly allocated #SysInfoIpodProperties which must be
 * freed after use, or NULL if an error occurred during the parsing
 */
SysInfoIpodProperties *itdb_sysinfo_extended_parse (const char *filename,
                                                    GError **error)
{
    GValue *parsed_doc;
    SysInfoIpodProperties *props;

    g_return_val_if_fail (filename != NULL, NULL);

    parsed_doc = itdb_plist_parse_from_file (filename, error);
    if (parsed_doc == NULL) {
        return NULL;
    }
    props = g_value_to_ipod_properties (parsed_doc);
    g_value_unset (parsed_doc);
    g_free (parsed_doc);

    return props;
}

SysInfoIpodProperties *itdb_sysinfo_extended_parse_from_xml (const char *xml,
							     GError **error)
{
    GValue *parsed_doc;
    SysInfoIpodProperties *props;

    g_return_val_if_fail (xml != NULL, NULL);

    parsed_doc = itdb_plist_parse_from_memory (xml, strlen (xml), error);
    if (parsed_doc == NULL) {
        return NULL;
    }
    props = g_value_to_ipod_properties (parsed_doc);
    g_value_unset (parsed_doc);
    g_free (parsed_doc);

    return props;
}

/**
 * itdb_sysinfo_properties_get_serial_number:
 * @props: a #SysInfoIpodProperties structure
 *
 * Gets the iPod serial number from @props if it was found while parsing
 * @props. The serial number uniquely identify an ipod and it can be used
 * to determine when it was produced and its model/color, see
 * http://svn.gnome.org/viewvc/podsleuth/trunk/src/PodSleuth/PodSleuth/SerialNumber.cs?view=markup
 * for more details about what the various parts of the serial number
 * correspond to. Please avoid parsing this serial number by yourself and
 * ask for additionnal API in libgpod if you find yourself needing to parse
 * that serial number :)
 *
 * Returns: the iPod serial number, NULL if the serial number wasn't set in
 * @props. The returned string must not be modified nor freed.
 */
const char *
itdb_sysinfo_properties_get_serial_number (const SysInfoIpodProperties *props)
{
    g_return_val_if_fail (props != NULL, NULL);
    return props->serial_number;
}

/**
 * itdb_sysinfo_properties_get_firewire_id:
 * @props: a #SysInfoIpodProperties structure
 *
 * Gets the iPod firewire ID from @props if it was found while parsing
 * @props. Contrary to what its name implies, the firewire ID is also set
 * on USB iPods and is especially important on iPod Classic and Nano Video
 * since this ID (which is unique on each iPod) is needed to generate the
 * checksum that is required to write a valid iPod database on these
 * models.
 *
 * Returns: the iPod firewire ID, NULL if the serial number wasn't set in
 * @props. The returned string must not be modified nor freed.
 */
const char *
itdb_sysinfo_properties_get_firewire_id (const SysInfoIpodProperties *props)
{
    g_return_val_if_fail (props != NULL, NULL);
    return props->firewire_guid;
}

/**
 * itdb_sysinfo_properties_get_cover_art_formats:
 * @props: a #SysInfoIpodProperties structure
 *
 * Returns: a #GList of #Itdb_ArtworkFormat describing the cover art formats
 * supported by the iPod described in @props. The returned list must not be
 * modified nor freed.
 */
const GList *
itdb_sysinfo_properties_get_cover_art_formats (const SysInfoIpodProperties *props)
{
    g_return_val_if_fail (props != NULL, NULL);
    return props->artwork_formats;
}

/**
 * itdb_sysinfo_properties_get_photo_formats:
 * @props: a #SysInfoIpodProperties structure
 *
 * Returns: a #GList of #Itdb_ArtworkFormat describing the photo formats
 * supported by the iPod described in @props. The returned list must not be
 * modified nor freed.
 */
const GList *
itdb_sysinfo_properties_get_photo_formats (const SysInfoIpodProperties *props)
{
    g_return_val_if_fail (props != NULL, NULL);
    return props->photo_formats;
}

/**
 * itdb_sysinfo_properties_get_chapter_image_formats:
 * @props: a #SysInfoIpodProperties structure
 *
 * Returns: a #GList of #Itdb_ArtworkFormat describing the chapter image
 * formats supported by the iPod described in @props. The returned list must
 * not be modified nor freed.
 */
const GList *
itdb_sysinfo_properties_get_chapter_image_formats (const SysInfoIpodProperties *props)
{
    g_return_val_if_fail (props != NULL, NULL);
    return props->chapter_image_formats;
}

/**
 * itdb_sysinfo_properties_supports_sparse_artwork:
 * @props: a #SysInfoIpodProperties structure
 *
 * Sparse artwork is a way to share artwork between different iPod tracks
 * which make things more efficient space-wise. This function can be used
 * to check if the more space-efficient artwork storage can be used.
 *
 * Returns: TRUE if the iPod supports sparse artwork, FALSE if it does not
 * or if @props doesn't contain any information about sparse artwork
 */
gboolean
itdb_sysinfo_properties_supports_sparse_artwork (const SysInfoIpodProperties *props)
{
    g_return_val_if_fail (props != NULL, FALSE);

    return props->supports_sparse_artwork;
}

gboolean
itdb_sysinfo_properties_supports_podcast (const SysInfoIpodProperties *props)
{
    g_return_val_if_fail (props != NULL, FALSE);

    return props->podcasts_supported;
}

const char *
itdb_sysinfo_properties_get_firmware_version (const SysInfoIpodProperties *props)
{
    g_return_val_if_fail (props != NULL, NULL);

    return props->visible_build_id;
}

gboolean
itdb_sysinfo_properties_supports_sqlite (const SysInfoIpodProperties *props)
{
    g_return_val_if_fail (props != NULL, FALSE);

    return props->sqlite_db;
}

gint
itdb_sysinfo_properties_get_family_id (const SysInfoIpodProperties *props)
{
    g_return_val_if_fail (props != NULL, FALSE);
    return props->family_id;
}

gint
itdb_sysinfo_properties_get_db_version (const SysInfoIpodProperties *props)
{
    g_return_val_if_fail (props != NULL, FALSE);
    return props->db_version;
}

gint
itdb_sysinfo_properties_get_shadow_db_version (const SysInfoIpodProperties *props)
{
   g_return_val_if_fail (props != NULL, 0);
   return props->shadowdb_version;
}

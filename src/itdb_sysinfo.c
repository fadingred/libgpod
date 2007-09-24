/*
|  Copyright (C) 2002-2007 Jorg Schuler <jcsjcs at users sourceforge net>
|  Part of the gtkpod project.
|
|  URL: http://www.gtkpod.org/
|  URL: http://gtkpod.sourceforge.net/
|
|  Part of this code is based on ipod-device.c from the libipoddevice
|  project (http://64.14.94.162/index.php/Libipoddevice).
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
|
|  $Id$
*/

#include <string.h>

#include <glib.h>

#include "itdb_device.h"

enum ParsingState {
    LOOKING_FOR_KEY_TAG,
    PARSING_KEY_TAG,
    LOOKING_FOR_FW_DATA,
    PARSING_FW_DATA,
    DONE
};

struct _SysInfoParseContext {
    enum ParsingState state;
    char *text;
    Itdb_Device *device;
};
typedef struct _SysInfoParseContext SysInfoParseContext;


static void parse_start_element (GMarkupParseContext *context,
                                 const gchar *element_name,
                                 const gchar **attribute_names,
                                 const gchar **attribute_values,
                                 gpointer user_data,
                                 GError **error)
{
    SysInfoParseContext *sysinfo = (SysInfoParseContext *)user_data;

#ifdef DEBUG
    g_print ("start: %s\n", element_name);
#endif
    switch (sysinfo->state) {
    case DONE:
        break;
    case LOOKING_FOR_FW_DATA:
        if (strcmp (element_name, "string") == 0) {
            sysinfo->state = PARSING_FW_DATA;
            break;
        }
        /* Fallback to default case if we didn't find the tag we expected */
    case LOOKING_FOR_KEY_TAG:
    default:
        if (strcmp (element_name, "key") == 0) {
            sysinfo->state = PARSING_KEY_TAG;
        } else {
            sysinfo->state = LOOKING_FOR_KEY_TAG;
        }
        break;
    }
}

static void parse_end_element (GMarkupParseContext *context,
			       const gchar *element_name,
			       gpointer user_data,
			       GError **error)
{
    SysInfoParseContext *sysinfo = (SysInfoParseContext *)user_data;

#ifdef DEBUG
    g_print ("end: %s\n", element_name);
#endif

    switch (sysinfo->state) {
    case PARSING_KEY_TAG:
        if (sysinfo->text == NULL) {
            sysinfo->state = LOOKING_FOR_KEY_TAG;
            break;
        }
        if (strcmp (sysinfo->text, "FireWireGUID") == 0) {
            sysinfo->state = LOOKING_FOR_FW_DATA;
        } else {
            sysinfo->state = LOOKING_FOR_KEY_TAG;
        }
        g_free (sysinfo->text);
        sysinfo->text = NULL;
        break;
    case PARSING_FW_DATA:
        if (sysinfo->text == NULL) {
            sysinfo->state = LOOKING_FOR_KEY_TAG;
        }
	/* Use FirewireGuid instead of FireWireGUID to be coherent with what
	 * the SysInfo file used to contain
	 */
	g_hash_table_insert (sysinfo->device->sysinfo,
			     g_strdup ("FirewireGuid"),
			     sysinfo->text);
	sysinfo->text = NULL;
        sysinfo->state = DONE;
        break;
    case LOOKING_FOR_KEY_TAG:
    case LOOKING_FOR_FW_DATA:
    case DONE:
        break;

    }
}

static void parse_text  (GMarkupParseContext *context,
			 const gchar *text,
			 gsize text_len,
			 gpointer user_data,
			 GError **error)
{
#ifdef DEBUG
    char *str = g_strndup (text, text_len);
    g_print ("text: %s\n", str);
    g_free (str);
#endif

    SysInfoParseContext *sysinfo = (SysInfoParseContext *)user_data;
    switch (sysinfo->state) {
    case DONE:
        break;
    case PARSING_FW_DATA:
    case PARSING_KEY_TAG:
        g_free (sysinfo->text);
        sysinfo->text = g_strndup (text, text_len);
        break;
    default:
        g_free (sysinfo->text);
        sysinfo->text = NULL;
        break;
    }
}

static void parse_error (GMarkupParseContext *context,
			 GError *error,
			 gpointer user_data)
{
    gint line;
    gint row;
    g_markup_parse_context_get_position (context, &line, &row);
    g_warning ("error at line %i row %i", line, row);
}

static char *
get_sysinfo_extended_path (Itdb_Device *device)
{
    const gchar *p_sysinfo[] = {"SysInfoExtended", NULL};
    gchar *dev_path, *sysinfo_path;

    dev_path = itdb_get_device_dir (device->mountpoint);

    if (!dev_path) {
	return NULL;
    }

    sysinfo_path = itdb_resolve_path (dev_path, p_sysinfo);
    g_free (dev_path);
    return sysinfo_path;
}

static void init_gmarkup_parser (GMarkupParser *parser)
{
    parser->start_element = parse_start_element;
    parser->end_element = parse_end_element;
    parser->text = parse_text;
    parser->error = parse_error;
}


static gboolean parse_sysinfo_xml (Itdb_Device *device, 
				   const char *xml, gsize len,
				   GError **error)
{
    GMarkupParseContext *ctx;
    GMarkupParser parser = {0, };
    gboolean success;

    SysInfoParseContext sysinfo = {0, };
    sysinfo.state = LOOKING_FOR_KEY_TAG;
    sysinfo.device = device;

    init_gmarkup_parser (&parser);
    ctx = g_markup_parse_context_new (&parser, 0, &sysinfo, NULL);
    success = g_markup_parse_context_parse (ctx, xml, len, error);
    if (!success) {
        g_markup_parse_context_free (ctx);
        return FALSE;
    }
    success = g_markup_parse_context_end_parse (ctx, error);
    if (!success) {
        g_markup_parse_context_free (ctx);
        return FALSE;
    }
    g_markup_parse_context_free (ctx);

    return TRUE;
}

/**
 * itdb_device_read_sysinfo_xml:
 * @device: an #Itdb_Device 
 * @error: return location for a #GError or NULL
 *
 * Parses a SysInfoExtended file located in iPod_Control/Device on the 
 * passed-in @device. This file contains a detailed description of the iPod 
 * capabilities/characteristics. In particular, it contains the iPod Firewire 
 * ID which is necessary to build the checksum of the iTunesDB.
 * That file is automatically parsed when @device mount point is set 
 * if it can be found.
 *
 * If a SysInfoExtended file could be successfully read, TRUE is returned. 
 * Otherwise it returns FALSE and sets @error.
 *
 * Return value: TRUE on success, FALSE if an error occurred
 *
 **/
gboolean
itdb_device_read_sysinfo_xml (Itdb_Device *device, GError **error)
{
    gboolean success;
    char *sysinfo_path;
    char *xml_data;
    gsize len;

    sysinfo_path = get_sysinfo_extended_path (device);
    if (sysinfo_path == NULL) {
	g_set_error (error, ITDB_FILE_ERROR, ITDB_FILE_ERROR_NOTFOUND,
		     "Couldn't find SysInfoExtended file");
	return FALSE;
    }
    success = g_file_get_contents (sysinfo_path, &xml_data, &len, error);
    g_free (sysinfo_path);

    if (!success) {
        return FALSE;
    }
    
    success = parse_sysinfo_xml (device, xml_data, len, error);
    g_free (xml_data);
    
    return success;
}

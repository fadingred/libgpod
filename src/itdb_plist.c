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

/* This file implements a generic plist parser. A plist file is an
 * Apple-specific XML format which is used to serialize (simple) data
 * structures to disk, see http://en.wikipedia.org/wiki/Property_list
 *
 * This parser should handle most plist files, with those limitations :
 *      - no support for <date> tags
 *
 * The plist file is parsed using libxml, and the parsed result is stored
 * in GValue. The types are mapped in the following way:
 *      - <string> => G_TYPE_STRING (char*)
 *      - <real> => G_TYPE_DOUBLE (double)
 *      - <integer> => G_TYPE_INT64 (gint64)
 *      - <true/>, <false/> => G_TYPE_BOOLEAN (gboolean)
 *      - <data> => G_TYPE_GSTRING (GString *)
 *      - <array> => G_TYPE_VALUE_ARRAY (GValueArray *)
 *      - <dict> => G_TYPE_HASH_TABLE (GHashTable *)
 */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_LIBXML

#include <stdio.h>
#include <string.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <glib.h>
#include <glib-object.h>

#include "itdb_plist.h"

#ifdef DEBUG_SYSINFO_PARSING
#define DEBUG g_print
#else
#define DEBUG(...)
#endif

/* Error domain */
#define ITDB_DEVICE_ERROR itdb_device_error_quark ()
extern GQuark itdb_device_error_quark (void);

static GValue *parse_node (xmlNode *a_node, GError **error);

static void
value_free (GValue *val)
{
    g_value_unset (val);
    g_free (val);
}

static GValue *
parse_integer(xmlNode *a_node, GError **error)
{
    char *str_val;
    char *end_ptr;
    gint64 int_val;
    GValue *value;

    str_val = (char *)xmlNodeGetContent(a_node);
    int_val = strtoll (str_val, &end_ptr, 0);
    if (*end_ptr != '\0') {
        g_set_error (error, ITDB_DEVICE_ERROR, ITDB_DEVICE_ERROR_XML_PARSING,
                     "invalid integer value: %s", str_val);
        xmlFree (str_val);
        return NULL;
    }
    xmlFree (str_val);

    value = g_new0(GValue, 1);
    g_value_init(value, G_TYPE_INT64);
    g_value_set_int64 (value, int_val);

    return value;
}

static GValue *
parse_string(xmlNode *a_node, G_GNUC_UNUSED GError **error)
{
    char *str_val;
    GValue *value;

    str_val = (char *)xmlNodeGetContent(a_node);

    value = g_new0(GValue, 1);
    g_value_init(value, G_TYPE_STRING);
    g_value_set_string (value, str_val);

    xmlFree (str_val);

    return value;
}

static GValue *
parse_real(xmlNode *a_node, GError **error)
{
    char *str_val;
    char *end_ptr;
    gfloat double_val;
    GValue *value;

    str_val = (char *)xmlNodeGetContent(a_node);
    double_val = g_ascii_strtod (str_val, &end_ptr);
    if (*end_ptr != '\0') {
        g_set_error (error, ITDB_DEVICE_ERROR, ITDB_DEVICE_ERROR_XML_PARSING,
                     "invalid real value: %s", str_val);
        xmlFree (str_val);
        return NULL;
    }
    xmlFree (str_val);

    value = g_new0(GValue, 1);
    g_value_init(value, G_TYPE_DOUBLE);
    g_value_set_double (value, double_val);

    return value;
}

static GValue *
parse_boolean (xmlNode *a_node, GError **error)
{
    gboolean bool_val;
    GValue *value;

    if (strcmp ((char *)a_node->name, "true") == 0) {
        bool_val = TRUE;
    } else if (strcmp ((char *)a_node->name, "false") == 0) {
        bool_val = FALSE;
    } else {
        g_set_error (error, ITDB_DEVICE_ERROR, ITDB_DEVICE_ERROR_XML_PARSING,
                     "unexpected boolean value: %s", a_node->name);
        return NULL;
    }

    value = g_new0 (GValue, 1);
    g_value_init (value, G_TYPE_BOOLEAN);
    g_value_set_boolean (value, bool_val);

    return value;
}

static GValue *
parse_data (xmlNode *a_node, G_GNUC_UNUSED GError **error)
{
    char *str_val;
    guchar *raw_data;
    gsize len;
    GString *data_val;
    GValue *value;

    str_val = (char *)xmlNodeGetContent(a_node);
#if GLIB_CHECK_VERSION(2,12,0)
    /* base64 support only after GLib 2.12 */
    raw_data = g_base64_decode (str_val, &len);
#else
#warning GLib > 2.12 required for g_base64_decode(). Working around this problem.
    raw_data = g_strdup ("");
    len = 0;
#endif
    xmlFree (str_val);
    data_val = g_string_new_len ((char *)raw_data, len);
    g_free (raw_data);

    value = g_new0(GValue, 1);
    g_value_init(value, G_TYPE_GSTRING);
    g_value_take_boxed (value, data_val);

    return value;
}

static xmlNode *
parse_one_dict_entry (xmlNode *a_node, GHashTable *dict, GError **error)
{
    xmlNode *cur_node = a_node;
    xmlChar *key_name;
    GValue *value;

    while ((cur_node != NULL) && (xmlStrcmp(cur_node->name, (xmlChar *)"key") != 0)) {
        if (!xmlIsBlankNode (cur_node)) {
            DEBUG ("skipping %s\n", cur_node->name);
        }
        cur_node = cur_node->next;
    }
    if (cur_node == NULL) {
        g_set_error (error, ITDB_DEVICE_ERROR, ITDB_DEVICE_ERROR_XML_PARSING,
                     "Dict entry contains no <key> node");
        return NULL;
    }
    key_name = xmlNodeGetContent (cur_node);
    cur_node = cur_node->next;
    while ((cur_node != NULL) && xmlIsBlankNode (cur_node)) {
        cur_node = cur_node->next;
    }
    if (cur_node == NULL) {
        g_set_error (error, ITDB_DEVICE_ERROR, ITDB_DEVICE_ERROR_XML_PARSING,
                     "<key> %s with no corresponding value node", key_name);
        xmlFree (key_name);
        return NULL;
    }

    value = parse_node (cur_node, error);
    if (value != NULL) {
        g_hash_table_insert (dict, g_strdup ((char *)key_name), value);
    } else {
        g_warning ("Couldn't parse value for %s: %s",
                   key_name, (*error)->message);
        g_clear_error (error);
    }
    xmlFree (key_name);

    return cur_node->next;
}

static GValue *
parse_dict (xmlNode *a_node, GError **error)
{
    xmlNode *cur_node = a_node->children;
    GValue *value;
    GHashTable *dict;

    dict = g_hash_table_new_full (g_str_hash, g_str_equal,
                                  g_free, (GDestroyNotify)value_free);

    while (cur_node != NULL) {
	if (xmlIsBlankNode (cur_node)) {
	    cur_node = cur_node->next;
	} else {
	    cur_node = parse_one_dict_entry (cur_node, dict, error);
	}
    }
    if ((error != NULL) && (*error != NULL)) {
        g_hash_table_destroy (dict);
        return NULL;
    }
    value = g_new0 (GValue, 1);
    value = g_value_init (value, G_TYPE_HASH_TABLE);
    g_value_take_boxed (value, dict);

    return value;
}
	
typedef GValue *(*ParseCallback) (xmlNode *, GError **);
static ParseCallback get_parser_for_type (const xmlChar *type);

static GValue *
parse_array (xmlNode *a_node, GError **error)
{
    xmlNode *cur_node = a_node->children;
    GValue *value;
    GValueArray *array;

    array = g_value_array_new (4);

    while (cur_node != NULL) {
	if (get_parser_for_type (cur_node->name) != NULL) {
   	    GValue *cur_value;
	    cur_value = parse_node (cur_node, error);
	    if (cur_value != NULL) {
	        array = g_value_array_append (array, cur_value);
		g_value_unset (cur_value);
		g_free (cur_value);
	    }
	}
	/* When an array contains an element enclosed in "unknown" tags (ie
	 * non-type ones), we silently skip them since early
	 * SysInfoExtended files used to have <key> values enclosed within
	 * <array> tags.
	 */
	cur_node = cur_node->next;
    }

    if ((error != NULL) && (*error != NULL)) {
	g_value_array_free (array);
        return NULL;
    }
    value = g_new0 (GValue, 1);
    value = g_value_init (value, G_TYPE_VALUE_ARRAY);
    g_value_take_boxed (value, array);

    return value;
}

struct Parser {
    const char * const type_name;
    ParseCallback parser;
};

static const struct Parser parsers[] = { {"integer", parse_integer},
					 {"real",    parse_real},
					 {"string",  parse_string},
					 {"true",    parse_boolean},
					 {"false",   parse_boolean},
					 {"data",    parse_data},
					 {"dict",    parse_dict},
					 {"array",   parse_array},
					 {NULL,      NULL} };

static ParseCallback get_parser_for_type (const xmlChar *type)
{
    guint i = 0;

    while (parsers[i].type_name != NULL) {
        if (xmlStrcmp (type, (xmlChar *)parsers[i].type_name) == 0) {
            if (parsers[i].parser != NULL) {
                return parsers[i].parser;
            }
        }
        i++;
    }
    return NULL;
}

static GValue *parse_node (xmlNode *a_node, GError **error)
{
    ParseCallback parser;

    g_return_val_if_fail (a_node != NULL, NULL);
    parser = get_parser_for_type (a_node->name);
    if (parser != NULL) {
	return parser (a_node, error);
    } else {
	DEBUG ("no parser for <%s>\n", a_node->name);
	return NULL;
    }
}

static GValue *
itdb_plist_parse (xmlNode * a_node, GError **error)
{
    xmlNode *cur_node;
    if (a_node == NULL) {
        g_set_error (error, ITDB_DEVICE_ERROR, ITDB_DEVICE_ERROR_XML_PARSING,
                     "Empty XML document");
        return NULL;
    }
    if (xmlStrcmp (a_node->name, (xmlChar *)"plist") != 0) {
        g_set_error (error, ITDB_DEVICE_ERROR, ITDB_DEVICE_ERROR_XML_PARSING,
                     "XML document does not seem to be a plist document");
        return NULL;
    }
    cur_node = a_node->xmlChildrenNode;
    while ((cur_node != NULL) && (xmlIsBlankNode (cur_node))) {
        cur_node = cur_node->next;
    }
    if (cur_node != NULL) {
        return parse_node (cur_node, error);
    }
    g_set_error (error, ITDB_DEVICE_ERROR, ITDB_DEVICE_ERROR_XML_PARSING,
                 "Empty XML document");
    return NULL;
}

/**
 * itdb_plist_parse:
 * @filename:   name of the XML plist file to parse
 * @error:      return location for a #GError
 *
 * Parses the XML plist file stored in @filename. If an error occurs
 * during the parsing, itdb_plist_parse will return NULL and @error
 * will be set
 *
 * Returns: NULL on error (@error will be set), a newly allocated
 * #GValue containing a #GHashTable otherwise.
 */
GValue *
itdb_plist_parse_from_file (const char *filename, GError **error)
{
    xmlDoc *doc = NULL;
    xmlNode *root_element = NULL;
    GValue *parsed_doc;

    doc = xmlReadFile(filename, NULL, 0);

    if (doc == NULL) {
        g_set_error (error, ITDB_DEVICE_ERROR, ITDB_DEVICE_ERROR_XML_PARSING,
                     "Error during XML parsing of file %s", filename);
        return NULL;
    }

    root_element = xmlDocGetRootElement(doc);

    parsed_doc = itdb_plist_parse (root_element, error);

    xmlFreeDoc(doc);

    return parsed_doc;
}

/**
 * itdb_plist_parse_from_memory:
 * @data:   memory location containing XML plist data to parse
 * @len:    length in bytes of the string to parse
 * @error:  return location for a #GError
 *
 * Parses the XML plist file stored in @data which length is @len
 * bytes. If an error occurs during the parsing,
 * itdb_plist_parse_from_memory() will return NULL and @error will be
 * set.
 *
 * Returns: NULL on error (@error will be set), a newly allocated
 * #GValue containing a #GHashTable otherwise.
 */
GValue *
itdb_plist_parse_from_memory (const char *data, gsize len, GError **error)
{
    xmlDoc *doc = NULL;
    xmlNode *root_element = NULL;
    GValue *parsed_doc;

    doc = xmlReadMemory(data, len, "noname.xml", NULL, 0);

    if (doc == NULL) {
        g_set_error (error, ITDB_DEVICE_ERROR, ITDB_DEVICE_ERROR_XML_PARSING,
                     "Error during XML parsing of in-memory data");
        return NULL;
    }

    root_element = xmlDocGetRootElement(doc);

    parsed_doc = itdb_plist_parse (root_element, error);

    xmlFreeDoc(doc);

    return parsed_doc;
}
#else
#include <glib-object.h>
#include "itdb_plist.h"

GValue *itdb_plist_parse_from_file (G_GNUC_UNUSED const char *filename,
                                    G_GNUC_UNUSED GError **error)
{
    return NULL;
}

GValue *itdb_plist_parse_from_memory (G_GNUC_UNUSED const char *data,
                                      G_GNUC_UNUSED gsize len,
                                      G_GNUC_UNUSED GError **error)
{
    return NULL;
}
#endif

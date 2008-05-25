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
 *      - no support for <array> containing "unamed" elements (ie with no
 *      <key> tag)
 *
 * The plist file is parsed using libxml, and the parsed result is stored 
 * in GValue. The types are mapped in the following way:
 *      - <string> => G_TYPE_STRING (char*)
 *      - <real> => G_TYPE_DOUBLE (double)
 *      - <integer> => G_TYPE_INT (gint)
 *      - <true/>, <false/> => G_TYPE_BOOLEAN (gboolean)
 *      - <data> => G_TYPE_GSTRING (GString *)
 *      - <array> => G_TYPE_HASH_TABLE (GHashTable *)
 *      - <dict> => G_TYPE_HASH_TABLE (GHashTable *)
 */
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

static GValue *parse_node (xmlNode *a_node);

static void 
value_free (GValue *val)
{
    g_value_unset (val);
    g_free (val);
}

static GValue * 
parse_integer(xmlNode *a_node)
{
    char *str_val;
    gint int_val;
    GValue *value;

    str_val = (char *)xmlNodeGetContent(a_node);
    int_val = strtol (str_val, NULL, 0);
    xmlFree (str_val);

    value = g_new0(GValue, 1);
    g_value_init(value, G_TYPE_INT);
    g_value_set_int (value, int_val);

    return value;
}

static GValue * 
parse_string(xmlNode *a_node)
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
parse_real(xmlNode *a_node)
{
    char *str_val;
    gfloat double_val;
    GValue *value;

    str_val = (char *)xmlNodeGetContent(a_node);
    double_val = g_ascii_strtod (str_val, NULL);
    xmlFree (str_val);

    value = g_new0(GValue, 1);
    g_value_init(value, G_TYPE_DOUBLE);
    g_value_set_double (value, double_val);

    return value;
}

static GValue *
parse_boolean (xmlNode *a_node)
{
    gboolean bool_val;
    GValue *value;

    if (strcmp ((char *)a_node->name, "true") == 0) {
        bool_val = TRUE;
    } else if (strcmp ((char *)a_node->name, "false") == 0) {
        bool_val = FALSE;
    } else {
        DEBUG ("unexpected boolean value\n");
        return NULL;
    }

    value = g_new0 (GValue, 1);
    g_value_init (value, G_TYPE_BOOLEAN);
    g_value_set_boolean (value, bool_val);

    return value;
}

static GValue *
parse_data (xmlNode *a_node)
{
    char *str_val;
    guchar *raw_data;
    gsize len;
    GString *data_val;
    GValue *value;

    str_val = (char *)xmlNodeGetContent(a_node);
    raw_data = g_base64_decode (str_val, &len);
    xmlFree (str_val);
    data_val = g_string_new_len ((char *)raw_data, len);
    g_free (raw_data);

    value = g_new0(GValue, 1);
    g_value_init(value, G_TYPE_GSTRING);
    g_value_take_boxed (value, data_val);

    return value;
}

static xmlNode *
parse_one_dict_entry (xmlNode *a_node, GHashTable *dict)
{
    xmlNode *cur_node = a_node;
    xmlChar *key_name;
    GValue *value;

    while ((cur_node != NULL) && (xmlStrcmp(cur_node->name, (xmlChar *)"key") != 0)) {
        if (!xmlNodeIsText (cur_node)) {
            DEBUG ("skipping %s\n", cur_node->name);
        }
        cur_node = cur_node->next;
    }
    if (cur_node == NULL) {
        return NULL;
    }
    key_name = xmlNodeGetContent(cur_node);
    cur_node = cur_node->next;
    while ((cur_node != NULL) && xmlNodeIsText(cur_node)) {
        cur_node = cur_node->next;
    }
    if (cur_node == NULL) {
        DEBUG ("<key> %s with no corresponding value node", key_name);
        xmlFree (key_name);
        return NULL;
    }

    value = parse_node (cur_node);
    if (value != NULL) {
        g_hash_table_insert (dict, g_strdup ((char *)key_name), value);
    }
    xmlFree (key_name);

    return cur_node->next;
}

static GValue * 
parse_dict (xmlNode *a_node)
{
    xmlNode *cur_node = a_node->children;
    GValue *value;
    GHashTable *dict;

    dict = g_hash_table_new_full (g_str_hash, g_str_equal, 
                                  g_free, (GDestroyNotify)value_free);

    while (cur_node != NULL) {
        cur_node = parse_one_dict_entry (cur_node, dict);
    }

    value = g_new0 (GValue, 1);
    value = g_value_init (value, G_TYPE_HASH_TABLE);
    g_value_take_boxed (value, dict);

    return value;
}

typedef GValue *(*ParseCallback) (xmlNode *);
struct Parser {
    const char * const type_name;
    ParseCallback parser;
};

static struct Parser parsers[] = { {"integer", parse_integer},
                                   {"real",    parse_real},
                                   {"string",  parse_string},
                                   {"true",    parse_boolean},
                                   {"false",   parse_boolean},
                                   {"data",    parse_data},
                                   {"dict",    parse_dict}, 
                                   {"array",   parse_dict},
                                   {NULL,      NULL} };

static GValue *parse_node (xmlNode *a_node)
{
    guint i = 0;
    g_return_val_if_fail (a_node != NULL, FALSE);
    while (parsers[i].type_name != NULL) {        
        if (xmlStrcmp (a_node->name, (xmlChar *)parsers[i].type_name) == 0) {
            if (parsers[i].parser != NULL) {
                return parsers[i].parser (a_node);
            }
        }
        i++;
    }
    DEBUG ("no parser for <%s>\n", a_node->name);
    return NULL;
}

static GValue *
itdb_plist_parse (xmlNode * a_node)
{
    xmlNode *cur_node;
    if (a_node == NULL) {
        DEBUG ("empty file\n");
        return NULL;
    }
    if (xmlStrcmp (a_node->name, (xmlChar *)"plist") != 0) {
        DEBUG ("not a plist file\n");
        return NULL;
    }
    cur_node = a_node->xmlChildrenNode;
    while ((cur_node != NULL) && (xmlNodeIsText (cur_node))) {
        cur_node = cur_node->next;
    }
    if (cur_node != NULL) {
        return parse_node (cur_node);
    }
    return NULL;
}

GValue *
itdb_plist_parse_from_file (const char *filename)
{
    xmlDoc *doc = NULL;
    xmlNode *root_element = NULL;
    GValue *parsed_doc;

    doc = xmlReadFile(filename, NULL, 0);

    if (doc == NULL) {
        printf("error: could not parse file %s\n", filename);
        return NULL;
    }

    root_element = xmlDocGetRootElement(doc);

    parsed_doc = itdb_plist_parse (root_element);

    xmlFreeDoc(doc);
    xmlCleanupParser();

    return parsed_doc;
}

GValue *
itdb_plist_parse_from_memory (const char *data, gsize len)
{
    xmlDoc *doc = NULL;
    xmlNode *root_element = NULL;
    GValue *parsed_doc;

    doc = xmlReadMemory(data, len, "noname.xml", NULL, 0);

    if (doc == NULL) {
        printf("error: could not parse data from memory\n");
        return NULL;
    }

    root_element = xmlDocGetRootElement(doc);

    parsed_doc = itdb_plist_parse (root_element);

    xmlFreeDoc(doc);
    xmlCleanupParser();

    return parsed_doc;
}

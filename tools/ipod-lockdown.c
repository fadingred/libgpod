/* Copyright (c) 2009, Martin S. <opensuse@sukimashita.com>
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
 */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <libxml/xmlmemory.h>

#include <libiphone/afc.h>
#include <libiphone/libiphone.h>
#include <libiphone/lockdown.h>

#ifndef HAVE_LIBPLIST_1_0
/* Old API */
#define plist_dict_get_item plist_get_dict_el_from_key
#define plist_new_string plist_add_sub_key_el
#define plist_dict_insert_item plist_add_sub_string_el
#endif

extern char *read_sysinfo_extended_by_uuid (const char *uuid);
extern gboolean iphone_write_sysinfo_extended (const char *uuid, const char *xml);

G_GNUC_INTERNAL char *
read_sysinfo_extended_by_uuid (const char *uuid)
{
	lockdownd_client_t client = NULL;
	iphone_device_t device = NULL;
	iphone_error_t ret = IPHONE_E_UNKNOWN_ERROR;
	char *xml = NULL; char *str = NULL;
	char *gxml;
	uint32_t xml_length = 0;
	plist_t value = NULL;
	plist_t global = NULL;
	plist_t ptr = NULL;
#ifdef HAVE_LIBIPHONE_1_0
	ret = iphone_device_new(&device, uuid);
#else
	ret = iphone_get_device_by_uuid(&device, uuid);
#endif
	if (ret != IPHONE_E_SUCCESS) {
		printf("No device found with uuid %s, is it plugged in?\n", uuid);
		return NULL;
	}

	if (LOCKDOWN_E_SUCCESS != lockdownd_client_new(device, &client)) {
		iphone_device_free(device);
		return NULL;
	}

	/* run query and get format plist */
	lockdownd_get_value(client, NULL, NULL, &global);
	lockdownd_get_value(client, "com.apple.mobile.iTunes", NULL, &value);
	
	/* add some required values manually to emulate old plist format */
	ptr = plist_dict_get_item(global, "SerialNumber");
	plist_get_string_val(ptr, &str);
	if (str != NULL) {
	    ptr = plist_new_string(str);
	    plist_dict_insert_item(value, "SerialNumber", ptr);
	    free(str);
	}

	ptr = plist_dict_get_item(global, "BuildVersion");
	plist_get_string_val(ptr, &str);
	if (str != NULL) {
	    ptr = plist_new_string(str);
	    plist_dict_insert_item(value, "BuildID", ptr);
	    free(str);
	}

	ptr = plist_new_string(uuid);
	plist_dict_insert_item(value, "FireWireGUID", ptr);

	ptr = plist_new_string(uuid);
	plist_dict_insert_item(value, "UniqueDeviceID", ptr);

	plist_to_xml(value, &xml, &xml_length);

	ptr = NULL;
	if (value)
		plist_free(value);
	value = NULL;
	if (global)
		plist_free(global);
	global = NULL;

	lockdownd_client_free(client);
	iphone_device_free(device);

	/* Jump through hoops since libxml will say to free mem it allocated
	 * with xmlFree while memory freed with g_free has to be allocated
	 * by glib.
	 */
	if (xml != NULL) {
		gxml = g_strdup(xml);
		xmlFree(xml);
	} else {
		gxml = NULL;
	}
	return gxml;
}

G_GNUC_INTERNAL gboolean
iphone_write_sysinfo_extended (const char *uuid, const char *xml)
{
	lockdownd_client_t client = NULL;
	iphone_device_t device = NULL;
	afc_client_t afc = NULL;
	iphone_error_t ret = IPHONE_E_UNKNOWN_ERROR;
	afc_error_t afc_ret;
	int afcport = 0;
	char *dest_filename;
	char *dest_directory;
	uint64_t handle;
	uint32_t bytes_written;

#ifdef HAVE_LIBIPHONE_1_0
	ret = iphone_device_new(&device, uuid);
#else
	ret = iphone_get_device_by_uuid(&device, uuid);
#endif
	if (IPHONE_E_SUCCESS != ret) {
		printf("No device found with uuid %s, is it plugged in?\n", uuid);
		return FALSE;
	}

	if (LOCKDOWN_E_SUCCESS != lockdownd_client_new(device, &client)) {
		iphone_device_free(device);
		return FALSE;
	}

	if (LOCKDOWN_E_SUCCESS != lockdownd_start_service(client, "com.apple.afc", &afcport)) {
		lockdownd_client_free(client);
		iphone_device_free(device);
		return FALSE;
	}
	g_assert (afcport != 0);
	if (AFC_E_SUCCESS != afc_client_new(device, afcport, &afc)) {
		lockdownd_client_free(client);
		iphone_device_free(device);
		return FALSE;
	}
	dest_directory = g_build_filename("iTunes_Control", "Device", NULL);
	afc_ret = afc_make_directory(afc, dest_directory);
	if ((AFC_E_SUCCESS != ret) && (AFC_E_OBJECT_EXISTS != ret)) {
		g_free (dest_directory);
		afc_client_free(afc);
		lockdownd_client_free(client);
		iphone_device_free(device);
		return FALSE;
	}
	dest_filename = g_build_filename(dest_directory, "SysInfoExtended", NULL);
	g_free (dest_directory);
	if (AFC_E_SUCCESS != afc_file_open(afc, dest_filename, AFC_FOPEN_WRONLY, &handle)) {
		g_free (dest_filename);
		afc_client_free(afc);
		lockdownd_client_free(client);
		iphone_device_free(device);
		return FALSE;
	}
	g_free (dest_filename);

	if (AFC_E_SUCCESS != afc_file_write(afc, handle, xml, strlen(xml), &bytes_written)) {
		afc_file_close(afc, handle);
		afc_client_free(afc);
		lockdownd_client_free(client);
		iphone_device_free(device);
		return FALSE;
	}

	afc_file_close(afc, handle);
	afc_client_free(afc);
	lockdownd_client_free(client);
	iphone_device_free(device);

	return TRUE;
}

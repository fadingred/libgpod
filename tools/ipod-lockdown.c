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

#include <libimobiledevice/afc.h>
#include <libimobiledevice/libimobiledevice.h>
#include <libimobiledevice/lockdown.h>

extern char *read_sysinfo_extended_by_uuid (const char *uuid);
extern gboolean iphone_write_sysinfo_extended (const char *uuid, const char *xml);

G_GNUC_INTERNAL char *
read_sysinfo_extended_by_uuid (const char *uuid)
{
	lockdownd_client_t client = NULL;
	idevice_t device = NULL;
	idevice_error_t ret = IDEVICE_E_UNKNOWN_ERROR;
	char *xml = NULL; char *str = NULL;
	char *gxml;
	uint32_t xml_length = 0;
	plist_t value = NULL;
	plist_t global = NULL;
	plist_t ptr = NULL;
	int cnt = 0;

	/* usbmuxd needs some time to start up so we try several times
	 * to open the device before finally returning with an error */ 
	while (cnt++ < 20) {
		ret = idevice_new(&device, uuid);
		if (ret == IDEVICE_E_SUCCESS) {
			break;
		}
		if (ret != IDEVICE_E_NO_DEVICE) {
			break;
		}
		g_usleep (G_USEC_PER_SEC / 2);
	}
	if (ret != IDEVICE_E_SUCCESS) {
		printf("No device found with uuid %s, is it plugged in?\n", uuid);
		return NULL;
	}

	if (LOCKDOWN_E_SUCCESS != lockdownd_client_new_with_handshake(device, &client, "libgpod")) {
		idevice_free(device);
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

	ptr = plist_dict_get_item(global, "ProductVersion");
	plist_get_string_val(ptr, &str);
	if (str != NULL) {
	    ptr = plist_new_string(str);
	    plist_dict_insert_item(value, "VisibleBuildID", ptr);
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
	idevice_free(device);

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
	idevice_t device = NULL;
	afc_client_t afc = NULL;
	idevice_error_t ret = IDEVICE_E_UNKNOWN_ERROR;
	afc_error_t afc_ret;
	uint16_t afcport = 0;
	uint64_t handle;
	uint32_t bytes_written;
	const char device_dir[] = "/iTunes_Control/Device";
	const char sysinfoextended_path[] = "/iTunes_Control/Device/SysInfoExtended";

	ret = idevice_new(&device, uuid);
	if (IDEVICE_E_SUCCESS != ret) {
		printf("No device found with uuid %s, is it plugged in?\n", uuid);
		return FALSE;
	}

	if (LOCKDOWN_E_SUCCESS != lockdownd_client_new_with_handshake(device, &client, "libgpod")) {
		idevice_free(device);
		return FALSE;
	}

	if (LOCKDOWN_E_SUCCESS != lockdownd_start_service(client, "com.apple.afc", &afcport)) {
		lockdownd_client_free(client);
		idevice_free(device);
		return FALSE;
	}
	g_assert (afcport != 0);
	if (AFC_E_SUCCESS != afc_client_new(device, afcport, &afc)) {
		lockdownd_client_free(client);
		idevice_free(device);
		return FALSE;
	}
	afc_ret = afc_make_directory(afc, device_dir);
	if ((AFC_E_SUCCESS != afc_ret) && (AFC_E_OBJECT_EXISTS != afc_ret)) {
		afc_client_free(afc);
		lockdownd_client_free(client);
		idevice_free(device);
		return FALSE;
	}
	if (AFC_E_SUCCESS != afc_file_open(afc, sysinfoextended_path,
					   AFC_FOPEN_WRONLY, &handle)) {
		afc_client_free(afc);
		lockdownd_client_free(client);
		idevice_free(device);
		return FALSE;
	}

	if (AFC_E_SUCCESS != afc_file_write(afc, handle, xml, strlen(xml), &bytes_written)) {
		afc_file_close(afc, handle);
		afc_client_free(afc);
		lockdownd_client_free(client);
		idevice_free(device);
		return FALSE;
	}

	afc_file_close(afc, handle);
	afc_client_free(afc);
	lockdownd_client_free(client);
	idevice_free(device);

	return TRUE;
}

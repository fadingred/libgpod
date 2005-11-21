/* Time-stamp: <2005-10-10 01:23:29 jcs>
|
|  Copyright (C) 2005 Jorg Schuler <jcsjcs at users sourceforge net>
|  Part of the gtkpod project.
| 
|  URL: http://www.gtkpod.org/
|  URL: http://gtkpod.sourceforge.net/
|
|
|  The source is taken from libipoddevice, CVS version October 8 2005
|  (http://64.14.94.162/index.php/Libipoddevice).
|
|  I decided not to make libgpod dependent on libipoddevice because
|  the latter depends on libraries not widely available yet (libhal >=
|  0.5.2, glib >= 2.8). It is planned to replace these files with a
|  libipoddevice dependence at some later time.
|
|  The following changes were done:
|
|  - libhal becomes optional (see #ifdef HAVE_LIBHAL sections)
|  - g_mkdir_with_parents() is provided if not available (glib < 2.8)
|  - publicly available functions were renamed from ipod_device_...()
|    to itdb_device_...()
|
|  Because of these changes only a limited amount of functionality is
|  available. See ipod-device.h for summary.
|
|
|
|
|  $Id$
*/
/* ex: set ts=4: */
/***************************************************************************
*  ipod-device.c
*  Copyright (C) 2005 Novell 
*  Written by Aaron Bockover <aaron@aaronbock.net>
****************************************************************************/

/*  
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of version 2.1 of the GNU Lesser General Public
 *  License as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Lesser Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 *  USA
 */


/* JCS: Change from ipod_device... to itdb_device for public functions */
#define ipod_device_get_type itdb_device_get_type
#define ipod_device_new itdb_device_new
#define ipod_device_rescan_disk itdb_device_rescan_disk
#define ipod_device_eject itdb_device_eject
#define ipod_device_reboot itdb_device_reboot
#define ipod_device_debug itdb_device_debug
#define ipod_device_save itdb_device_save
#define ipod_device_list_devices itdb_device_list_devices
#define ipod_device_list_device_udis itdb_device_list_device_udis

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <glib/gstdio.h>

#include "hal-common.h"
#include "ipod-device.h"

#define GB 1024

typedef struct _IpodModel {
	const gchar *model_number;
	const guint64 capacity;
	guint model_type;
	guint generation;
} IpodModel;


static const IpodModel ipod_model_table [] = {
	/* Handle idiots who hose their iPod file system, or 
	   lucky people with iPods we don't yet know about*/
	{"Invalid", 0,    MODEL_TYPE_INVALID,    UNKNOWN_GENERATION},
	{"Unknown", 0,    MODEL_TYPE_UNKNOWN,    UNKNOWN_GENERATION},
	
	/* First Generation */
	{"8513", 5  * GB, MODEL_TYPE_REGULAR,    FIRST_GENERATION},
	{"8541", 5  * GB, MODEL_TYPE_REGULAR,    FIRST_GENERATION},
	{"8697", 5  * GB, MODEL_TYPE_REGULAR,    FIRST_GENERATION},
	{"8709", 10 * GB, MODEL_TYPE_REGULAR,    FIRST_GENERATION},
	
	/* Second Generation */
	{"8737", 10 * GB, MODEL_TYPE_REGULAR,    SECOND_GENERATION},
	{"8740", 10 * GB, MODEL_TYPE_REGULAR,    SECOND_GENERATION},
	{"8738", 20 * GB, MODEL_TYPE_REGULAR,    SECOND_GENERATION},
	{"8741", 20 * GB, MODEL_TYPE_REGULAR,    SECOND_GENERATION},
	
	/* Third Generation */
	{"8976", 10 * GB, MODEL_TYPE_REGULAR,    THIRD_GENERATION},
	{"8946", 15 * GB, MODEL_TYPE_REGULAR,    THIRD_GENERATION},
	{"9460", 15 * GB, MODEL_TYPE_REGULAR,    THIRD_GENERATION},
	{"9244", 20 * GB, MODEL_TYPE_REGULAR,    THIRD_GENERATION},
	{"8948", 30 * GB, MODEL_TYPE_REGULAR,    THIRD_GENERATION},
	{"9245", 40 * GB, MODEL_TYPE_REGULAR,    THIRD_GENERATION},
	
	
	/* Fourth Generation */
	{"9282", 20 * GB, MODEL_TYPE_REGULAR,    FOURTH_GENERATION},
	{"9787", 25 * GB, MODEL_TYPE_REGULAR_U2, FOURTH_GENERATION},
	{"9268", 40 * GB, MODEL_TYPE_REGULAR,    FOURTH_GENERATION},
	{"A079", 20 * GB, MODEL_TYPE_COLOR,      FOURTH_GENERATION},
	{"A127", 20 * GB, MODEL_TYPE_COLOR_U2,   FOURTH_GENERATION},
	{"9830", 60 * GB, MODEL_TYPE_COLOR,      FOURTH_GENERATION},
	
	/* First Generation Mini */
	{"9160", 4 * GB, MODEL_TYPE_MINI,        FIRST_GENERATION},
	{"9436", 4 * GB, MODEL_TYPE_MINI_BLUE,   FIRST_GENERATION},
	{"9435", 4 * GB, MODEL_TYPE_MINI_PINK,   FIRST_GENERATION},
	{"9434", 4 * GB, MODEL_TYPE_MINI_GREEN,  FIRST_GENERATION},
	{"9437", 4 * GB, MODEL_TYPE_MINI_GOLD,   FIRST_GENERATION},	

	/* Second Generation Mini */
	{"9800", 4 * GB, MODEL_TYPE_MINI,        SECOND_GENERATION},
	{"9802", 4 * GB, MODEL_TYPE_MINI_BLUE,   SECOND_GENERATION},
	{"9804", 4 * GB, MODEL_TYPE_MINI_PINK,   SECOND_GENERATION},
	{"9806", 4 * GB, MODEL_TYPE_MINI_GREEN,  SECOND_GENERATION},
	{"9801", 6 * GB, MODEL_TYPE_MINI,        SECOND_GENERATION},
	{"9803", 6 * GB, MODEL_TYPE_MINI_BLUE,   SECOND_GENERATION},
	{"9805", 6 * GB, MODEL_TYPE_MINI_PINK,   SECOND_GENERATION},
	{"9807", 6 * GB, MODEL_TYPE_MINI_GREEN,  SECOND_GENERATION},	

	/* Photo / Fourth Generation */
	{"9829", 30 * GB, MODEL_TYPE_COLOR,      FOURTH_GENERATION},
	{"9585", 40 * GB, MODEL_TYPE_COLOR,      FOURTH_GENERATION},
	{"9586", 60 * GB, MODEL_TYPE_COLOR,      FOURTH_GENERATION},
	{"9830", 60 * GB, MODEL_TYPE_COLOR,      FOURTH_GENERATION},
	
	/* Shuffle / Fourth Generation */
	{"9724", GB / 2, MODEL_TYPE_SHUFFLE,     FOURTH_GENERATION},
	{"9725", GB,     MODEL_TYPE_SHUFFLE,     FOURTH_GENERATION},
	
	/* Nano / Fifth Generation */
	{"A004", GB * 2, MODEL_TYPE_NANO_WHITE,  FIFTH_GENERATION},
	{"A099", GB * 2, MODEL_TYPE_NANO_BLACK,  FIFTH_GENERATION},
	{"A005", GB * 4, MODEL_TYPE_NANO_WHITE,  FIFTH_GENERATION},
	{"A107", GB * 4, MODEL_TYPE_NANO_BLACK,  FIFTH_GENERATION},

	/* Video / Fifth Generation */
	{"A002", GB * 30, MODEL_TYPE_VIDEO_WHITE, FIFTH_GENERATION},
	{"A146", GB * 30, MODEL_TYPE_VIDEO_BLACK, FIFTH_GENERATION},
	{"A003", GB * 60, MODEL_TYPE_VIDEO_WHITE, FIFTH_GENERATION},
	{"A147", GB * 60, MODEL_TYPE_VIDEO_BLACK, FIFTH_GENERATION},
	
	/* HP iPods, need contributions for this table */
	{"E436", 40 * GB, MODEL_TYPE_REGULAR, FOURTH_GENERATION},
	
	{NULL, 0, 0, 0}
};

static const gchar *ipod_model_name_table [] = {
	"Invalid",
	"Unknown",
	"Color",
	"Color U2",
	"Grayscale",
	"Grayscale U2",
	"Mini (Silver)",
	"Mini (Blue)",
	"Mini (Pink)",
	"Mini (Green)",
	"Mini (Gold)",
	"Shuffle",
	"Nano (White)",
	"Nano (Black)",
	"Video (White)",
	"Video (Black)",
	NULL
};

static const IpodArtworkFormat ipod_color_artwork_info[] = {
	{IPOD_COVER_SMALL,        56,  56, 1017},
	{IPOD_COVER_LARGE,       140, 140, 1016},
	{IPOD_PHOTO_SMALL,        42,  30, 1009},
	{IPOD_PHOTO_LARGE,       130,  88, 1015},
	{IPOD_PHOTO_FULL_SCREEN, 220, 176, 1013},
	{IPOD_PHOTO_TV_SCREEN,   720, 480, 1019},
	{-1,                      -1,  -1,   -1}
};

static const IpodArtworkFormat ipod_nano_artwork_info[] = {
	{IPOD_COVER_SMALL,        42,  42, 1031},
	{IPOD_COVER_LARGE,       100, 100, 1027},
	{IPOD_PHOTO_LARGE,        42,  37, 1032},
	{IPOD_PHOTO_FULL_SCREEN, 176, 132, 1023},
	{-1,                      -1,  -1,   -1}
};

static const IpodArtworkFormat ipod_video_artwork_info[] = {
	{IPOD_COVER_SMALL,       100, 100, 1028},
	{IPOD_COVER_LARGE,       200, 200, 1029},
	{IPOD_PHOTO_SMALL,        50,  41, 1036},
	{IPOD_PHOTO_LARGE,       130,  88, 1015},
	{IPOD_PHOTO_FULL_SCREEN, 320, 240, 1024},
	{IPOD_PHOTO_TV_SCREEN,   720, 480, 1019},
	{-1,                      -1,  -1,   -1}
};

/* This will be indexed using a value from the MODEL_TYPE enum */
static const IpodArtworkFormat *ipod_artwork_info_table[] = {
        NULL,                    /* Invalid       */
	NULL,                    /* Unknown       */
	ipod_color_artwork_info, /* Color         */
	ipod_color_artwork_info, /* Color U2      */
	NULL,                    /* Grayscale     */ 
	NULL,                    /* Grayscale U2  */
	NULL,                    /* Mini (Silver) */
	NULL,                    /* Mini (Blue)   */
	NULL,                    /* Mini (Pink)   */
	NULL,                    /* Mini (Green)  */
	NULL,                    /* Mini (Gold)   */
	NULL,                    /* Shuffle       */
	ipod_nano_artwork_info,  /* Nano (White)  */
	ipod_nano_artwork_info,  /* Nano (Black)  */
	ipod_video_artwork_info, /* Video (White) */
	ipod_video_artwork_info  /* Video (Black) */
};


#define g_free_if_not_null(o) \
    if(o != NULL) {           \
        g_free(o);            \
        o = NULL;             \
    }
	
static const gchar *sysinfo_field_names [] = {
	"pszSerialNumber",
	"ModelNumStr",
	"visibleBuildID",
	NULL
};
	
static gchar *sysinfo_arr_get_dup(gchar *arr[], const gchar *key)
{
	gint i = 0;
	
	for(i = 0; sysinfo_field_names[i] != NULL; i++) {
		if(g_strcasecmp(sysinfo_field_names[i], key) == 0)
			return g_strdup(arr[i]);
	}

	return NULL;	
}


#if ((GTK_MAJOR_VERSION <= 2) && (GTK_MINOR_VERSION < 8))
/**
 * g_mkdir_with_parents:
 * @pathname: a pathname in the GLib file name encoding
 * @mode: permissions to use for newly created directories
 *
 * Create a directory if it doesn't already exist. Create intermediate
 * parent directories as needed, too.
 *
 * Returns: 0 if the directory already exists, or was successfully
 * created. Returns -1 if an error occurred, with errno set.
 *
 * Since: 2.8  (copied from GLIB version 2.8 - JCS)
 */
int
g_mkdir_with_parents (const gchar *pathname,
		      int          mode);
int
g_mkdir_with_parents (const gchar *pathname,
		      int          mode)
{
  gchar *fn, *p;

  if (pathname == NULL || *pathname == '\0')
    {
      errno = EINVAL;
      return -1;
    }

  fn = g_strdup (pathname);

  if (g_path_is_absolute (fn))
    p = (gchar *) g_path_skip_root (fn);
  else
    p = fn;

  do
    {
      while (*p && !G_IS_DIR_SEPARATOR (*p))
	p++;
      
      if (!*p)
	p = NULL;
      else
	*p = '\0';
      
      if (!g_file_test (fn, G_FILE_TEST_EXISTS))
	{
	  if (g_mkdir (fn, mode) == -1)
	    {
	      int errno_save = errno;
	      g_free (fn);
	      errno = errno_save;
	      return -1;
	    }
	}
      else if (!g_file_test (fn, G_FILE_TEST_IS_DIR))
	{
	  g_free (fn);
	  errno = ENOTDIR;
	  return -1;
	}
      if (p)
	{
	  *p++ = G_DIR_SEPARATOR;
	  while (*p && G_IS_DIR_SEPARATOR (*p))
	    p++;
	}
    }
  while (p);

  g_free (fn);

  return 0;
}
#endif


static void ipod_device_class_init(IpodDeviceClass *klass);
static void ipod_device_init(IpodDevice *sp);
static void ipod_device_finalize(GObject *object);

gchar *ipod_device_read_device_info_string(FILE *fd);
void ipod_device_write_device_info_string(gchar *str, FILE *fd);
	
gboolean ipod_device_reload(IpodDevice *device);
void ipod_device_construct_paths(IpodDevice *device);
gboolean ipod_device_info_load(IpodDevice *device);
guint ipod_device_detect_model(IpodDevice *device);
gboolean ipod_device_detect_volume_info(IpodDevice *device);
LibHalContext *ipod_device_hal_initialize(void);
void ipod_device_detect_volume_used(IpodDevice *device);
guint64 ipod_device_dir_size(const gchar *path);
gboolean ipod_device_has_open_fd(IpodDevice *device);
gboolean ipod_device_read_sysinfo(IpodDevice *device);
gboolean ipod_device_detect_writeable(IpodDevice *device);
void ipod_device_restore_reboot_preferences(IpodDevice *device);
	
struct IpodDevicePrivate {
	LibHalContext *hal_context;
	
	/* Paths */
	gchar *device_path;
	gchar *mount_point;
	gchar *control_path;
	gchar *hal_volume_id;
	
	gchar *adv_capacity;
	guint model_index;

	/* DeviceInfo Fields (All Devices) */ 
	gchar *device_name;
	gchar *user_name;
	gchar *host_name;
	
	/* Volume Size/Usage */
	guint64 volume_size;
	guint64 volume_available;
	guint64 volume_used;
	
	/* System Info */
	gchar *serial_number;
	gchar *model_number;
	gchar *firmware_version;
	
	gchar *volume_uuid;
	gchar *volume_label;
	
	/* Fresh from the factory/restore? */
	gboolean is_new;
	
	/* Safety */
	gboolean is_ipod;
	gboolean can_write;
};

static GObjectClass *parent_class = NULL;

/* GObject Class Specific Methods */

GType
ipod_device_get_type()
{
	static GType type = 0;

	if(type == 0) {
		static const GTypeInfo our_info = {
			sizeof (IpodDeviceClass),
			NULL,
			NULL,
			(GClassInitFunc)ipod_device_class_init,
			NULL,
			NULL,
			sizeof (IpodDevice),
			0,
			(GInstanceInitFunc)ipod_device_init,
		};

		type = g_type_register_static(G_TYPE_OBJECT, 
			"IpodDevice", &our_info, 0);
	}

	return type;
}

static void 
ipod_device_get_property(GObject *object, guint prop_id, 
	GValue *value, GParamSpec *pspec)
{
	IpodDevice *device = IPOD_DEVICE(object);
	
	switch(prop_id) {
		case PROP_HAL_CONTEXT:
			g_value_set_pointer(value, device->priv->hal_context);
			break;
		case PROP_HAL_VOLUME_ID:
			g_value_set_string(value, device->priv->hal_volume_id);
			break;
		case PROP_MOUNT_POINT:
			g_value_set_string(value, device->priv->mount_point);
			break;
		case PROP_DEVICE_PATH:
			g_value_set_string(value, device->priv->device_path);
			break;
		case PROP_CONTROL_PATH:
			g_value_set_string(value, device->priv->control_path);
			break;
		case PROP_DEVICE_MODEL:
			g_value_set_uint(value, 
				ipod_model_table[device->priv->model_index].model_type);
			break;
		case PROP_DEVICE_MODEL_STRING:
			g_value_set_string(value,
				ipod_model_name_table[ipod_model_table[
					device->priv->model_index].model_type]);
			break;
		case PROP_DEVICE_GENERATION:
			g_value_set_uint(value, 
				ipod_model_table[device->priv->model_index].generation);
			break;
		case PROP_ADVERTISED_CAPACITY:
			g_value_set_string(value, device->priv->adv_capacity);
			break;
		case PROP_DEVICE_NAME:
			g_value_set_string(value, device->priv->device_name);
			break;
		case PROP_USER_NAME:
			g_value_set_string(value, device->priv->user_name);
			break;
		case PROP_HOST_NAME:
			g_value_set_string(value, device->priv->host_name);
			break;
		case PROP_VOLUME_SIZE:
			g_value_set_uint64(value, device->priv->volume_size);
			break;
		case PROP_VOLUME_AVAILABLE:
			g_value_set_uint64(value, device->priv->volume_available);
			break;
		case PROP_VOLUME_USED:
			g_value_set_uint64(value, device->priv->volume_used);
			break;
		case PROP_IS_IPOD:
			g_value_set_boolean(value, device->priv->is_ipod);
			break;
		case PROP_IS_NEW:
			g_value_set_boolean(value, device->priv->is_new);
			break;
		case PROP_SERIAL_NUMBER:
			g_value_set_string(value, device->priv->serial_number);
			break;
		case PROP_MODEL_NUMBER:
			g_value_set_string(value, device->priv->model_number);
			break;
		case PROP_FIRMWARE_VERSION:
			g_value_set_string(value, device->priv->firmware_version);
			break;
		case PROP_VOLUME_UUID:
			g_value_set_string(value, device->priv->volume_uuid);
			break;
		case PROP_VOLUME_LABEL:
			g_value_set_string(value, device->priv->volume_label);
			break;
		case PROP_CAN_WRITE:
			g_value_set_boolean(value, device->priv->can_write);
			break;
     	        case PROP_ARTWORK_FORMAT:
			g_value_set_pointer(value, 
				(gpointer)ipod_artwork_info_table[ipod_model_table[
					device->priv->model_index].model_type]);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
			break;
	}
}

static void
ipod_device_set_property(GObject *object, guint prop_id,
	const GValue *value, GParamSpec *pspec)
{
	IpodDevice *device = IPOD_DEVICE(object);
	const gchar *str;
	gchar **volumes;
	gint volume_count;

	switch(prop_id) {
		case PROP_MOUNT_POINT:
		case PROP_DEVICE_PATH:
		case PROP_HAL_VOLUME_ID:
			str = g_value_get_string(value);
			volumes = libhal_manager_find_device_string_match(
				device->priv->hal_context, "block.device", str, 
				&volume_count, NULL);
		
			if(volume_count == 0) {
				libhal_free_string_array(volumes);
				volumes = libhal_manager_find_device_string_match(
					device->priv->hal_context, "volume.mount_point", 
					str, &volume_count, NULL);
				
				if(volume_count >= 1)
					str = volumes[0];
			} else {
				str = volumes[0];
			}
#ifdef HAVE_LIBHAL
			g_free_if_not_null(device->priv->hal_volume_id);
			device->priv->hal_volume_id = g_strdup(str);
#else
/* JCS for libgpod */
			g_free (device->priv->mount_point);
			device->priv->mount_point = g_strdup (str);
/* end JCS for libgpod */
#endif
			device->priv->is_ipod = ipod_device_reload(device);
			libhal_free_string_array(volumes);	
			break;
		case PROP_DEVICE_NAME:
			str = g_value_get_string(value);
			g_free_if_not_null(device->priv->device_name);
			device->priv->device_name = g_strdup(str);
			break;
		case PROP_USER_NAME:
			str = g_value_get_string(value);
			g_free_if_not_null(device->priv->user_name);
			device->priv->user_name = g_strdup(str);
			break;
		case PROP_HOST_NAME:
			str = g_value_get_string(value);
			g_free_if_not_null(device->priv->host_name);
			device->priv->host_name = g_strdup(str);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
			break;
	}
}

static void
ipod_device_class_init(IpodDeviceClass *klass)
{
	GParamSpec *hal_context_param;
	GParamSpec *hal_volume_id_param;
	GParamSpec *mount_point_param;
	GParamSpec *device_path_param;
	GParamSpec *control_path_param;
	GParamSpec *device_model_param;
	GParamSpec *device_model_string_param;
	GParamSpec *device_name_param;
	GParamSpec *user_name_param;
	GParamSpec *host_name_param;
	GParamSpec *volume_size_param;
	GParamSpec *volume_available_param;
	GParamSpec *volume_used_param;
	GParamSpec *is_ipod_param;
	GParamSpec *is_new_param;
	GParamSpec *serial_number_param;
	GParamSpec *model_number_param;
	GParamSpec *firmware_version_param;
	GParamSpec *volume_uuid_param;
	GParamSpec *volume_label_param;
	GParamSpec *can_write_param;
	GParamSpec *device_generation_param;
	GParamSpec *advertised_capacity_param;
	GParamSpec *artwork_format_param;

	GObjectClass *class = G_OBJECT_CLASS(klass);

	parent_class = g_type_class_peek_parent(klass);
	class->finalize = ipod_device_finalize;
	
	hal_context_param = g_param_spec_pointer("hal-context", "HAL Context",
		"LibHalContext handle", G_PARAM_READABLE);
	
	hal_volume_id_param = g_param_spec_string("hal-volume-id", "HAL Volume ID", 
		"Volume ID of a device in HAL",
		NULL, G_PARAM_READWRITE);
	
	mount_point_param = g_param_spec_string("mount-point", "Mount Point", 
		"Where iPod is mounted (parent of an iPod_Control directory)",
		NULL, G_PARAM_READWRITE);
	
	device_path_param = g_param_spec_string("device-path", "Device Path",
		"Path to raw iPod Device (/dev/sda2, for example)",
		NULL, G_PARAM_READWRITE);
		
	control_path_param = g_param_spec_string("control-path", 
		"iPod_Control Path","Full path to iPod_Control", 
		NULL, G_PARAM_READABLE);
		 
	device_model_param = g_param_spec_uint("device-model", "Device Model",
		"Type of iPod (Regular, Photo, Shuffle)", 0, 2, 0, G_PARAM_READABLE);
	
	device_model_string_param = g_param_spec_string("device-model-string",
		"Device Model String", "String type of iPod (Regular, Shuffle)",
		NULL, G_PARAM_READABLE);
	
	device_name_param = g_param_spec_string("device-name", "Device Name",
		"The user-assigned name of their iPod", NULL, G_PARAM_READWRITE);
		 
	user_name_param = g_param_spec_string("user-name", "User Name",
		"On Windows, Maybe Mac, the user account owning the iPod",
		NULL, G_PARAM_READWRITE);
		
	host_name_param = g_param_spec_string("host-name", "Host Name",
		"On Windows, Maybe Mac, the host/computer name owning the iPod",
		NULL, G_PARAM_READWRITE);
		
	volume_size_param = g_param_spec_uint64("volume-size", "Volume Size",
		"Total size of the iPod's hard drive", 0, G_MAXLONG, 0, 
		G_PARAM_READABLE);
		
	volume_available_param = g_param_spec_uint64("volume-available", 
		"Volume Available", "Available space on the iPod",
		0, G_MAXLONG, 0, G_PARAM_READABLE);
		
	volume_used_param = g_param_spec_uint64("volume-used", "Volume Used",
		"How much space has been used", 0, G_MAXLONG, 0, G_PARAM_READABLE);
	
	is_ipod_param = g_param_spec_boolean("is-ipod", "Is iPod",
		"If all device checks are okay, then this is true",
		FALSE, G_PARAM_READABLE);
		
	is_new_param = g_param_spec_boolean("is-new", "Is iPod",
		"If device is fresh from factory/restore, this is true",
		FALSE, G_PARAM_READABLE);
	
	serial_number_param = g_param_spec_string("serial-number", "Serial Number",
		"Serial Number of the iPod",
		NULL, G_PARAM_READABLE);
		
	model_number_param = g_param_spec_string("model-number", "Model Number",
		"Model Number of the iPod",
		NULL, G_PARAM_READABLE);
		
	firmware_version_param = g_param_spec_string("firmware-version",
		"Firmware Version", "iPod Firmware Version",
		NULL, G_PARAM_READABLE);
			
	volume_uuid_param = g_param_spec_string("volume-uuid", "Volume UUID",
		"Volume UUID of the iPod",
		NULL, G_PARAM_READABLE);
		
	volume_label_param = g_param_spec_string("volume-label", "Volume Label",
		"Volume Label of the iPod",
		NULL, G_PARAM_READABLE);
		
	can_write_param = g_param_spec_boolean("can-write", "Can Write",
		"True if device can be written to (mounted read/write)",
		FALSE, G_PARAM_READABLE);

	advertised_capacity_param = g_param_spec_string("advertised-capacity", 
		"Advertised Capacity", "Apple Marketed/Advertised Capacity String",
		NULL, G_PARAM_READABLE);

	device_generation_param = g_param_spec_uint("device-generation", 
		"Generation", "Generation of the iPod",
		0, G_MAXUINT, 0, G_PARAM_READABLE);

	artwork_format_param = g_param_spec_pointer("artwork-formats",
		"Artwork Format", "Support Artwork Formats", G_PARAM_READABLE);

	class->set_property = ipod_device_set_property;
	class->get_property = ipod_device_get_property;	
	g_object_class_install_property(class, PROP_HAL_CONTEXT,
		hal_context_param);
	
	g_object_class_install_property(class, PROP_HAL_VOLUME_ID,
		hal_volume_id_param);
	
	g_object_class_install_property(class, PROP_MOUNT_POINT, 
		mount_point_param);
	
	g_object_class_install_property(class, PROP_DEVICE_PATH, 
		device_path_param);
	
	g_object_class_install_property(class, PROP_CONTROL_PATH, 
		control_path_param);
	
	g_object_class_install_property(class, PROP_DEVICE_MODEL, 
		device_model_param);
		
	g_object_class_install_property(class, PROP_DEVICE_MODEL_STRING,
		device_model_string_param);
		
	g_object_class_install_property(class, PROP_DEVICE_NAME, 
		device_name_param);
		
	g_object_class_install_property(class, PROP_USER_NAME, 
		user_name_param);
		
	g_object_class_install_property(class, PROP_HOST_NAME, 
		host_name_param);
		
	g_object_class_install_property(class, PROP_VOLUME_SIZE, 
		volume_size_param);
		
	g_object_class_install_property(class, PROP_VOLUME_AVAILABLE, 
		volume_available_param);
		
	g_object_class_install_property(class, PROP_VOLUME_USED, 
		volume_used_param);
		
	g_object_class_install_property(class, PROP_IS_IPOD, is_ipod_param);
	
	g_object_class_install_property(class, PROP_IS_NEW, is_new_param);
	
	g_object_class_install_property(class, PROP_SERIAL_NUMBER, 
		serial_number_param);
	
	g_object_class_install_property(class, PROP_MODEL_NUMBER, 
		model_number_param);
	
	g_object_class_install_property(class, PROP_FIRMWARE_VERSION, 
		firmware_version_param);
	
	g_object_class_install_property(class, PROP_VOLUME_UUID, 
		volume_uuid_param);
	
	g_object_class_install_property(class, PROP_VOLUME_LABEL, 
		volume_label_param);

	g_object_class_install_property(class, PROP_CAN_WRITE,
		can_write_param);
		
	g_object_class_install_property(class, PROP_DEVICE_GENERATION, 
		device_generation_param);

	g_object_class_install_property(class, PROP_ADVERTISED_CAPACITY,
		advertised_capacity_param);
	
	g_object_class_install_property(class, PROP_ARTWORK_FORMAT,
		artwork_format_param);
		
}

static void
ipod_device_init(IpodDevice *device)
{
	device->priv = g_new0(IpodDevicePrivate, 1);
	
	device->priv->hal_context = ipod_device_hal_initialize();
	
	device->priv->hal_volume_id = NULL;
	device->priv->mount_point = NULL;
	device->priv->device_path = NULL;
	device->priv->control_path = NULL;
	device->priv->device_name = NULL;
	device->priv->user_name = NULL;
	device->priv->host_name = NULL;
	device->priv->adv_capacity = NULL;
	device->priv->serial_number = NULL;
	device->priv->model_number = NULL;
	device->priv->firmware_version = NULL;
	device->priv->volume_uuid = NULL;
	device->priv->volume_label = NULL;
	
	device->priv->volume_size = 0;
	device->priv->volume_available = 0;
	device->priv->volume_used = 0;
	
	device->priv->is_new = FALSE;
	device->priv->can_write = FALSE;
}

static void
ipod_device_finalize(GObject *object)
{
	IpodDevice *device = IPOD_DEVICE(object);
	
	/* Free private members, etc. */
	g_free_if_not_null(device->priv->hal_volume_id);
	g_free_if_not_null(device->priv->device_path);
	g_free_if_not_null(device->priv->mount_point);
	g_free_if_not_null(device->priv->control_path);
	g_free_if_not_null(device->priv->device_name);
	g_free_if_not_null(device->priv->user_name);
	g_free_if_not_null(device->priv->host_name);
	g_free_if_not_null(device->priv->adv_capacity);
	g_free_if_not_null(device->priv->serial_number);
	g_free_if_not_null(device->priv->model_number);
	g_free_if_not_null(device->priv->firmware_version);
	g_free_if_not_null(device->priv->volume_uuid);
	g_free_if_not_null(device->priv->volume_label);

	if(device->priv->hal_context != NULL) {
		libhal_ctx_shutdown(device->priv->hal_context, NULL);
		libhal_ctx_free(device->priv->hal_context);
	}
	g_free(device->priv);
	G_OBJECT_CLASS(parent_class)->finalize(object);
}

/* PRIVATE METHODS */

LibHalContext *
ipod_device_hal_initialize()
{
#ifdef HAVE_LIBHAL
	LibHalContext *hal_context;
	DBusError error;
	DBusConnection *dbus_connection;
	char **devices;
	gint device_count;

	hal_context = libhal_ctx_new();
	if(hal_context == NULL) 
		return NULL;
	
	dbus_error_init(&error);
	dbus_connection = dbus_bus_get(DBUS_BUS_SYSTEM, &error);
	if(dbus_error_is_set(&error)) {
		dbus_error_free(&error);
		libhal_ctx_free(hal_context);
		return NULL;
	}
	
	libhal_ctx_set_dbus_connection(hal_context, dbus_connection);
	
	if(!libhal_ctx_init(hal_context, &error)) {
		libhal_ctx_free(hal_context);
		return NULL;
	}

	devices = libhal_get_all_devices(hal_context, &device_count, NULL);
	if(devices == NULL) {
		libhal_ctx_shutdown(hal_context, NULL);
		libhal_ctx_free(hal_context);
		hal_context = NULL;
		return NULL;
	}
	
	libhal_free_string_array(devices);
	
	return hal_context;
#else
	return NULL;
#endif
}

gboolean
ipod_device_reload(IpodDevice *device)
{
	g_return_val_if_fail(IS_IPOD_DEVICE(device), FALSE);
	
	device->priv->model_index = 0;

	if(!ipod_device_detect_volume_info(device))
		return FALSE;
	
	ipod_device_construct_paths(device);
	
	device->priv->is_new = !ipod_device_info_load(device);
	
	ipod_device_detect_volume_used(device);
	ipod_device_detect_writeable(device);
	ipod_device_read_sysinfo(device);
	ipod_device_detect_model(device);
	ipod_device_restore_reboot_preferences(device);
	
	return device->priv->model_index != 0;
}

void 
ipod_device_construct_paths(IpodDevice *device)
{
	int len;
	
	g_return_if_fail(IS_IPOD_DEVICE(device));
	g_return_if_fail(device->priv->mount_point != NULL);
	
	len = strlen(device->priv->mount_point);
	if(device->priv->mount_point[len - 1] == '/')
		device->priv->mount_point[len - 1] = '\0';

	if(strlen(device->priv->mount_point) == 0)
		return;
	
	device->priv->control_path = g_strdup_printf("%s/%s",
		device->priv->mount_point, "iPod_Control/");
}

gchar *
ipod_device_read_device_info_string(FILE *fd)
{
	gshort length;
	gunichar2 *utf16;
	gchar *utf8;
	
	fread(&length, 1, sizeof(gshort), fd);
	
	if(length <= 0)
		return NULL;
	
	utf16 = (gunichar2 *)g_malloc(length * sizeof(gunichar2));
	fread(utf16, sizeof(gunichar2), length, fd);
	
	if(utf16 == NULL)
		return NULL;
	
	utf8 = g_utf16_to_utf8(utf16, length, NULL, NULL, NULL);
	
	g_free(utf16);
	utf16 = NULL;
	
	return utf8;
}

void
ipod_device_write_device_info_string(gchar *str, FILE *fd)
{
	gunichar2 *unistr;
	gshort length;

	if(str == NULL)
		return;
	
	length = strlen(str);
	unistr = g_utf8_to_utf16(str, length, NULL, NULL, NULL);
	
	length = length > 0x198 ? 0x198 : length;

	fwrite(&length, 2, 1, fd);
	fwrite(unistr, 2, length, fd);
	
	g_free(unistr);
}

gboolean
ipod_device_read_sysinfo(IpodDevice *device)
{
	gchar *field_values[sizeof(sysinfo_field_names) + 1];
	gchar *tmp, *path, buf[512];
	gint i, name_len;
	FILE *fd;
	
	g_return_val_if_fail(IS_IPOD_DEVICE(device), FALSE);
	
	path = g_strdup_printf("%sDevice/SysInfo", 
		device->priv->control_path);
	
	fd = fopen(path, "r");
	if(fd == NULL) {
		g_free(path);
		return FALSE;
	}
	
	while(fgets(buf, sizeof(buf), fd)) {
		buf[strlen(buf) - 1] = '\0';
		
		for(i = 0; sysinfo_field_names[i] != NULL; i++) {
			name_len = strlen(sysinfo_field_names[i]);
			if(strncasecmp(buf, sysinfo_field_names[i], name_len) == 0) {
				field_values[i] = strdup(buf + name_len + 2);
				if(strncasecmp(field_values[i], "0x", 2) == 0) {
					if((tmp = strstr(field_values[i], "(")) != NULL) {
						field_values[i] = tmp + 1;
						field_values[i][strlen(field_values[i]) - 1] = '\0';
					}
				}
				
				field_values[i] = g_strdup(field_values[i]);
			}
		}
	}
	
	fclose(fd);
	
	device->priv->serial_number = sysinfo_arr_get_dup(field_values, 
		"pszSerialNumber");
	device->priv->model_number = sysinfo_arr_get_dup(field_values, 
		"ModelNumStr");
	device->priv->firmware_version = sysinfo_arr_get_dup(field_values, 
		"visibleBuildID");
		
	g_free(path);
	
	return TRUE;
}

gboolean
ipod_device_info_load(IpodDevice *device)
{
	gchar *path;
	FILE *fd;
	
	g_return_val_if_fail(IS_IPOD_DEVICE(device), FALSE);
	
	path = g_strdup_printf("%siTunes/DeviceInfo", 
		device->priv->control_path);
	
	fd = fopen(path, "r");
	if(fd == NULL) {
		g_free(path);
		return FALSE;
	}
	
	device->priv->device_name = ipod_device_read_device_info_string(fd);
	if(device->priv->device_name == NULL)
		device->priv->device_name = g_strdup("iPod");
	
	fseek(fd, 0x200, SEEK_SET);
	device->priv->user_name = ipod_device_read_device_info_string(fd);
	
	fseek(fd, 0x400, SEEK_SET);
	device->priv->host_name = ipod_device_read_device_info_string(fd);
	
	fclose(fd);
	g_free(path);

	return TRUE;
}

gboolean
ipod_device_detect_writeable(IpodDevice *device)
{
	FILE *fp;
	gchar *itunes_dir, *music_dir, *itunesdb_path;
	struct stat finfo;
		
	g_return_val_if_fail(IS_IPOD_DEVICE(device), FALSE);
	
	device->priv->can_write = FALSE;

	itunes_dir = g_strdup_printf("%siTunes", device->priv->control_path);
	
	if(!g_file_test(itunes_dir, G_FILE_TEST_IS_DIR)) {
		if(g_mkdir_with_parents(itunes_dir, 0755) != 0) {
			g_free(itunes_dir);
			return FALSE;
		}
	}
	
	itunesdb_path = g_strdup_printf("%s/iTunesDB", itunes_dir);
	
	if((fp = fopen(itunesdb_path, "a+")) != NULL) {
		device->priv->can_write = TRUE;
		fclose(fp);
		
		memset(&finfo, 0, sizeof(finfo));
		if(stat(itunesdb_path, &finfo) == 0) {
			if(finfo.st_size == 0) {
				unlink(itunesdb_path);
			}
		}
	} else {
		g_free(itunes_dir);
		g_free(itunesdb_path);
		return FALSE;
	}

	music_dir = g_strdup_printf("%sMusic", device->priv->control_path);
	
	if(!g_file_test(music_dir, G_FILE_TEST_IS_DIR)) {
		device->priv->can_write = g_mkdir_with_parents(music_dir, 0755) == 0;
	}
	
	g_free(itunes_dir);
	g_free(itunesdb_path);
	g_free(music_dir);
	
	return device->priv->can_write;
}

gint
ipod_device_get_model_index_from_table(const gchar *_model_num);
gint
ipod_device_get_model_index_from_table(const gchar *_model_num)
{
	gint i;
	gchar *model_num = g_strdup(_model_num);
	gchar *p = model_num;
	
	if(isalpha(model_num[0])) 
		*p++;
	
	for(i = 2; ipod_model_table[i].model_number != NULL; i++) {
		if(g_strncasecmp(p, ipod_model_table[i].model_number, 4) == 0) {
			g_free(model_num);
			return i;
		}
	}	
	
	g_free(model_num);
	return 1;
}

guint
ipod_device_detect_model(IpodDevice *device)
{
	gint i;
	guint64 adv, act;
	gint cap;
	
	g_return_val_if_fail(IS_IPOD_DEVICE(device), 0);

	device->priv->model_index = 0;

	/* Shuffle! */
	if(device->priv->model_number == NULL) {
		for(i = 2; ipod_model_table[i].model_number != NULL; i++) {
			if(ipod_model_table[i].model_type != MODEL_TYPE_SHUFFLE)
				continue;
			
			cap = ipod_model_table[i].capacity;	
			adv = cap * 1048576;
			act = device->priv->volume_size;

			if((adv - act) / 1048576 < 50) {
				device->priv->model_index = i;
				device->priv->model_number = g_strdup_printf("M%s",
					ipod_model_table[i].model_number);
					
				device->priv->adv_capacity = g_strdup_printf("%d %s", 
					cap < 1024 ? cap : cap / 1024,
					cap < 1024 ? "MB" : "GB");
				break;
			}
		}	
	} else {
		/* Anything Else */
		device->priv->model_index = 
			ipod_device_get_model_index_from_table(device->priv->model_number);
		
		cap = ipod_model_table[device->priv->model_index].capacity;
	
		device->priv->adv_capacity = g_strdup_printf("%d %s", 
			cap < 1024 ? cap : cap / 1024,
			cap < 1024 ? "MB" : "GB");
	}
		
	return device->priv->model_index;
}

void
ipod_device_restore_reboot_preferences(IpodDevice *device)
{
	gchar *backup_prefs, *prefs;
	
	backup_prefs = g_strdup_printf("%s/.Reboot_Preferences", 
		device->priv->control_path);
	prefs = g_strdup_printf("%s/Device/Preferences",
		device->priv->control_path);
		
	g_return_if_fail(IS_IPOD_DEVICE(device));
	
	if(g_file_test(backup_prefs, G_FILE_TEST_EXISTS)) {
		unlink(prefs);
		g_rename(backup_prefs, prefs);
	}
}

gboolean
ipod_device_detect_volume_info(IpodDevice *device)
{
#ifdef HAVE_LIBHAL
	LibHalContext *hal_context;
	gchar **volumes;
	gchar *hd_mount_point, *hd_device_path;
	gchar *hd_hal_id = NULL, *maybe_hd_hal_id = NULL;
	gint volume_count, i;
	
	g_return_val_if_fail(IS_IPOD_DEVICE(device), FALSE);
	hal_context = device->priv->hal_context;
	
	g_free_if_not_null(device->priv->device_path);
	g_free_if_not_null(device->priv->mount_point);
	device->priv->volume_size = 0;
	
	if(!libhal_device_exists(hal_context, device->priv->hal_volume_id, NULL)) {
		/* For testing/debugging... we don't have a real device, but
		   the location may be a directory containing an iPod image */
		if(g_strncasecmp(device->priv->hal_volume_id, "/dev/", 5) == 0 || 
			device->priv->hal_volume_id[0] != '/')
			return FALSE;

		g_free_if_not_null(device->priv->mount_point);
		device->priv->mount_point = g_strdup(device->priv->hal_volume_id);
		
		g_free_if_not_null(device->priv->hal_volume_id);
		g_free_if_not_null(device->priv->device_path);
		
		/* Let's find out about the disk drive containing our image */
	
		volumes = libhal_manager_find_device_string_match(hal_context, 
			"info.category", "volume", &volume_count, NULL);
		
		for(i = 0; i < volume_count; i++) {
			if(!libhal_device_property_exists(hal_context, 
				volumes[i], "volume.is_mounted", NULL) || 
				!libhal_device_get_property_bool(hal_context, 
				volumes[i], "volume.is_mounted", NULL)) {
				continue;
			}
			
			hd_mount_point = libhal_device_get_property_string(hal_context,
				volumes[i], "volume.mount_point", NULL);
			
			if(g_strncasecmp(hd_mount_point, device->priv->mount_point, 
				strlen(hd_mount_point)) != 0)
				continue;
			
			if(g_strcasecmp(hd_mount_point, "/") == 0)
				maybe_hd_hal_id = volumes[i];
			else 
				hd_hal_id = volumes[i];
		}
		
		if(hd_hal_id == NULL && maybe_hd_hal_id != NULL)
			hd_hal_id = maybe_hd_hal_id;
		
		if(hd_hal_id == NULL) {
			libhal_free_string_array(volumes);
			return FALSE;
		}
		
		if(!libhal_device_exists(hal_context, hd_hal_id, NULL)) {
			libhal_free_string_array(volumes);
			return FALSE;
		}
		
		hd_device_path = libhal_device_get_property_string(hal_context,
			hd_hal_id, "block.device", NULL);
		
		device->priv->hal_volume_id = g_strdup(hd_hal_id);
		device->priv->device_path = g_strdup(hd_device_path);
		device->priv->volume_size = libhal_device_get_property_uint64(
			hal_context, hd_hal_id, "volume.size", NULL);
		
		libhal_free_string_array(volumes);
		
		return TRUE;
	}
	
	if(!libhal_device_property_exists(hal_context, 
		device->priv->hal_volume_id, "volume.is_mounted", NULL) 
		|| !libhal_device_get_property_bool(hal_context, 
		device->priv->hal_volume_id, "volume.is_mounted", NULL)) {
		return FALSE;
	}
			
	if(libhal_device_property_exists(hal_context, device->priv->hal_volume_id, 
		"block.device", NULL))
		device->priv->device_path = libhal_device_get_property_string(
			hal_context, device->priv->hal_volume_id, "block.device", NULL);
	
	if(libhal_device_property_exists(hal_context, device->priv->hal_volume_id,
		"volume.mount_point", NULL))
		device->priv->mount_point = libhal_device_get_property_string(
			hal_context, device->priv->hal_volume_id, 
			"volume.mount_point", NULL);
	
	if(libhal_device_property_exists(hal_context, device->priv->hal_volume_id,
		"volume.size", NULL))
		device->priv->volume_size = libhal_device_get_property_uint64(
			hal_context, device->priv->hal_volume_id, "volume.size", NULL);
	
	if(libhal_device_property_exists(hal_context, device->priv->hal_volume_id,
		"volume.uuid", NULL)) {
		device->priv->volume_uuid = libhal_device_get_property_string(
			hal_context, device->priv->hal_volume_id,
			"volume.uuid", NULL);
		
		if(strlen(device->priv->volume_uuid) == 0) {
			g_free(device->priv->volume_uuid);
			device->priv->volume_uuid = NULL;
		}
	}

	if(libhal_device_property_exists(hal_context, device->priv->hal_volume_id,
		"volume.label", NULL))
		device->priv->volume_label = libhal_device_get_property_string(
			hal_context, device->priv->hal_volume_id,
			"volume.label", NULL);
#endif	
	return TRUE;
}

void 
ipod_device_detect_volume_used(IpodDevice *device)
{
	device->priv->volume_used = 
		ipod_device_dir_size(device->priv->mount_point);
	device->priv->volume_available = device->priv->volume_size - 
		device->priv->volume_used;
}

void 
_ipod_device_dir_size(const gchar *path, guint64 *total_size);
void 
_ipod_device_dir_size(const gchar *path, guint64 *total_size)
{
	GDir *dir;
	const gchar *next_path;
	gchar *fullpath;
	struct stat finfo;
	
	if((dir = g_dir_open(path, 0, NULL)) == NULL)
		return;

	while((next_path = g_dir_read_name(dir)) != NULL) {
		fullpath = g_strdup_printf("%s/%s", path, next_path);

		if(g_file_test(fullpath, G_FILE_TEST_IS_DIR))
			_ipod_device_dir_size(fullpath, total_size);
		else 
			*total_size += stat(fullpath, &finfo) == 0 ? finfo.st_size : 0;
		
		g_free(fullpath);
		fullpath = NULL;
	}
	
	g_dir_close(dir);
}

guint64
ipod_device_dir_size(const gchar *path)
{
	guint64 retsize, *size = g_new(guint64, 1);
	*size = 0;
	_ipod_device_dir_size(path, size);
	retsize = *size;
	g_free(size);
	return retsize;
}

gboolean 
ipod_device_has_open_fd(IpodDevice *device)
{
	GDir *dir, *fddir;
	gchar *fdpath, *fdidpath, *realpath;
	const gchar *procpath, *fdid;

	g_return_val_if_fail(IS_IPOD_DEVICE(device), FALSE);
	
	if((dir = g_dir_open("/proc", 0, NULL)) == NULL)
		return FALSE;

	while((procpath = g_dir_read_name(dir)) != NULL) {
		if(atoi(procpath) <= 0)
			continue;
		
		fdpath = g_strdup_printf("/proc/%s/fd", procpath);
		
		if(!g_file_test(fdpath, G_FILE_TEST_IS_DIR)) {
			g_free(fdpath);
			continue;
		}
		
		if((fddir = g_dir_open(fdpath, 0, NULL)) == NULL) {
			g_free(fdpath);
			continue;
		}
		
		while((fdid = g_dir_read_name(fddir)) != NULL) {
			fdidpath = g_strdup_printf("%s/%s", fdpath, fdid);
			realpath = g_file_read_link(fdidpath, NULL);
			
			if(realpath == NULL) {
				g_free(fdidpath);
				continue;
			}
			
			if(g_strncasecmp(realpath, device->priv->mount_point, 
				strlen(device->priv->mount_point)) == 0) {
				g_dir_close(fddir);
				g_dir_close(dir);
				g_free(realpath);
				g_free(fdidpath);
				return TRUE;
			}
			
			g_free(realpath);
			g_free(fdidpath);
		}
		
		g_dir_close(fddir);
	}
	
	g_dir_close(dir);
	
	return FALSE;
}

#ifdef HAVE_LIBHAL
/* modded from g-v-m */
static int
ipod_device_run_command(IpodDevice *device, const char *command, 
	GError **error_out)
{
	char *path;
	const char *inptr, *start;
	GError *error = NULL;
	GString *exec;
	char *argv[4];
	int status = 0;
	
	exec = g_string_new(NULL);
	
	/* perform s/%d/device/, s/%m/mount_point/ and s/%h/udi/ */
	start = inptr = command;
	while((inptr = strchr (inptr, '%')) != NULL) {
		g_string_append_len(exec, start, inptr - start);
		inptr++;
		switch (*inptr) {
		case 'd':
			g_string_append(exec, device->priv->device_path ? 
				device->priv->device_path : "");
			break;
		case 'm':
			if(device->priv->mount_point) {
				path = g_shell_quote(device->priv->mount_point);
				g_string_append(exec, path);
				g_free(path);
			} else {
				g_string_append(exec, "\"\"");
			}
			break;
		case 'h':
			g_string_append(exec, device->priv->hal_volume_id);
			break;
		case '%':
			g_string_append_c(exec, '%');
			break;
		default:
			g_string_append_c(exec, '%');
			if(*inptr)
				g_string_append_c(exec, *inptr);
			break;
		}
		
		if(*inptr)
			inptr++;
		start = inptr;
	}
	
	g_string_append(exec, start);
	
	argv[0] = "/bin/sh";
	argv[1] = "-c";
	argv[2] = exec->str;
	argv[3] = NULL;

	g_spawn_sync(g_get_home_dir(), argv, NULL, 0, NULL, 
		NULL, NULL, NULL, &status, &error);
	
	if(error != NULL)
		g_propagate_error(error_out, error);
	
	g_string_free(exec, TRUE);
	
	return status;
}
#endif

/* PUBLIC METHODS */

IpodDevice *
ipod_device_new(const gchar *hal_volume_id)
{
	IpodDevice *device = g_object_new(TYPE_IPOD_DEVICE, 
		"hal-volume-id", hal_volume_id, NULL);

	if (device == NULL) {
		/* This can happen if one forgot to call g_type_init before
		 * calling ipod_device_new
		 */
		return NULL;
	}
	
	if(!device->priv->is_ipod) {
		g_object_unref(device);
		return NULL;
	}
	
	return device;
}

gboolean 
ipod_device_rescan_disk(IpodDevice *device)
{
	g_return_val_if_fail(IS_IPOD_DEVICE(device), FALSE);
	ipod_device_detect_volume_used(device);
	return TRUE;
}

guint
ipod_device_eject(IpodDevice *device, GError **error_out)
{
#ifdef HAVE_LIBHAL	
	gint exit_status;
	GError *error = NULL;

	g_return_val_if_fail(IS_IPOD_DEVICE(device), EJECT_ERROR);
	g_return_val_if_fail(device->priv->is_ipod, EJECT_ERROR);

	if(ipod_device_has_open_fd(device))
		return EJECT_BUSY;

#ifdef ASSUME_SUBMOUNT
	sync();
	
	exit_status = ipod_device_run_command(device, EJECT_COMMAND, &error);
	if(error) {
		g_propagate_error(error_out, error);
		return EJECT_ERROR;
	}
		
	return exit_status == 0 ? EJECT_OK : EJECT_ERROR;
#endif
	
	exit_status = ipod_device_run_command(device, UNMOUNT_COMMAND, &error);
	
	if(!error && exit_status == 0) {
		exit_status = ipod_device_run_command(device, EJECT_COMMAND, &error);
		if(error) {
			g_propagate_error(error_out, error);
			return EJECT_ERROR;
		}
		
		return exit_status == 0 ? EJECT_OK : EJECT_ERROR;
	} 
	
	if(error)
		g_propagate_error(error_out, error);
	
	return EJECT_ERROR;
}

guint
ipod_device_reboot(IpodDevice *device, GError **error_out)
{
	gchar *sysinfo, *prefs, *backup_prefs, *devpath;
	gboolean can_eject = TRUE;

	g_return_val_if_fail(IS_IPOD_DEVICE(device), EJECT_ERROR);
	g_return_val_if_fail(device->priv->is_ipod, EJECT_ERROR);
	
	devpath = g_strdup_printf("%s/Device/", device->priv->control_path);
	sysinfo = g_strdup_printf("%sSysInfo", devpath);
	prefs = g_strdup_printf("%sPreferences", devpath);
	backup_prefs = g_strdup_printf("%s/.Reboot_Preferences", 
		device->priv->control_path);
		
	if(g_file_test(sysinfo, G_FILE_TEST_EXISTS))
		can_eject = unlink(sysinfo) == 0;
		
	if(g_file_test(prefs, G_FILE_TEST_EXISTS) && can_eject) {
		if(g_file_test(backup_prefs, G_FILE_TEST_EXISTS))
			unlink(backup_prefs);
			
		g_rename(prefs, backup_prefs);
		unlink(prefs);
		
		can_eject = TRUE;
	}
		
	if(can_eject) 
		can_eject = rmdir(devpath) == 0;
		
	g_free(devpath);
	g_free(sysinfo);
	g_free(prefs);
	g_free(backup_prefs);
		
	if(can_eject)
		return ipod_device_eject(device, error_out);
	
	g_propagate_error(error_out, g_error_new(g_quark_from_string("UNLINK"),
		errno, "Could not remove file to initiate iPod reboot"));
#endif		
	return EJECT_ERROR;
}

GList *
_ipod_device_list_devices(gboolean create_device);
GList *
_ipod_device_list_devices(gboolean create_device)
{
	LibHalContext *hal_context;
	GList *finalDevices = NULL;
	gchar **ipods, **volumes;
	gint ipod_count, volume_count, i, j;
	IpodDevice *ipod;
	gboolean validIpod = FALSE;
	
	hal_context = ipod_device_hal_initialize();
	if(hal_context == NULL)
		return NULL;
	
	ipods = libhal_manager_find_device_string_match(hal_context, 
		"info.product", "iPod", &ipod_count, NULL);
	
	for(i = 0; i < ipod_count; i++) {
		volumes = libhal_manager_find_device_string_match(hal_context,
			"info.parent", ipods[i], &volume_count, NULL);
		
		for(j = 0; j < volume_count; j++) {
			if(!libhal_device_property_exists(hal_context, 
				volumes[j], "volume.is_mounted", NULL) 
				|| !libhal_device_get_property_bool(hal_context, 
				volumes[j], "volume.is_mounted", NULL))
				continue;
					
			if(!create_device) {
				finalDevices = g_list_append(finalDevices, 
					g_strdup(volumes[j]));
				continue;
			}
				
			if((ipod = ipod_device_new(volumes[j])) == NULL)
				continue;
			
			g_object_get(ipod, "is-ipod", &validIpod, NULL);
			if(validIpod)
				finalDevices = g_list_append(finalDevices, ipod);
		}
	}
	
	libhal_ctx_shutdown(hal_context, NULL);
	libhal_ctx_free(hal_context);
	
	return finalDevices;
}

GList *
ipod_device_list_devices()
{
	return _ipod_device_list_devices(TRUE);
}

GList *
ipod_device_list_device_udis()
{
	return _ipod_device_list_devices(FALSE);
}
	
gboolean
ipod_device_save(IpodDevice *device, GError **error_out)
{
	FILE *fd;
	gchar *path, *itunes_dir;
	gchar bs = 0;
	GError *error = NULL;
	
	g_return_val_if_fail(IS_IPOD_DEVICE(device), FALSE);
	
	itunes_dir = g_strdup_printf("%siTunes", device->priv->control_path);
	path = g_strdup_printf("%s/DeviceInfo", itunes_dir);
	
	if(!g_file_test(itunes_dir, G_FILE_TEST_IS_DIR)) {
		if(g_mkdir_with_parents(itunes_dir, 0744) != 0) {
			if(error_out != NULL) {
				error = g_error_new(g_quark_from_static_string("IPOD_DEVICE"),
					ERROR_SAVE, "Could not create iTunes Directory: %s", 
					itunes_dir);
				g_propagate_error(error_out, error);
			}
			
			g_free(path);
			g_free(itunes_dir);
			
			return FALSE;
		}
	}
	
	fd = fopen(path, "w+");
	if(fd == NULL) {
		if(error_out != NULL) {
			error = g_error_new(g_quark_from_static_string("IPOD_DEVICE"),
				ERROR_SAVE, "Could not save DeviceInfo file: %s", path);
			g_propagate_error(error_out, error);
		}
		
		g_free(path);
		g_free(itunes_dir);
		
		return FALSE;
	}
	
	ipod_device_write_device_info_string(device->priv->device_name, fd);
	
	fseek(fd, 0x200, SEEK_SET);
	ipod_device_write_device_info_string(device->priv->user_name, fd);
	
	fseek(fd, 0x400, SEEK_SET);
	ipod_device_write_device_info_string(device->priv->host_name, fd);
	
	fseek(fd, 0X5FF, SEEK_SET);
	fwrite(&bs, 1, 1, fd);
	
	fclose(fd);
	
	g_free(path);
	g_free(itunes_dir);
	
	return TRUE;
}

void
ipod_device_debug(IpodDevice *device)
{	
	static const gchar *generation_names [] = {
		"Unknown",
		"First",
		"Second",
		"Third",
		"Fourth"	
	};
	
	gchar *device_path, *mount_point, *control_path, *hal_id;
	gchar *model_number, *adv_capacity, *model_string;
	guint model, generation;
	gboolean is_new, can_write;
	gchar *serial_number, *firmware_version;
	guint64 volume_size, volume_used, volume_available;
	gchar *volume_uuid, *volume_label;
	gchar *device_name, *user_name, *host_name;
	
	g_return_if_fail(IS_IPOD_DEVICE(device));
	
	g_object_get(device, 
		"device-path", &device_path,
		"mount-point", &mount_point,
		"control-path", &control_path,
		"hal-volume-id", &hal_id,
		"model-number", &model_number,
		"device-model", &model,
		"device-model-string", &model_string,
		"device-generation", &generation,
		"advertised-capacity", &adv_capacity,
		"is-new", &is_new,
		"can-write", &can_write,
		"serial-number", &serial_number,
		"firmware-version", &firmware_version,
		"volume-size", &volume_size,
		"volume-used", &volume_used,
		"volume-available", &volume_available,
		"volume_uuid", &volume_uuid,
		"volume-label", &volume_label,
		"device-name", &device_name,
		"user-name", &user_name,
		"host-name", &host_name,
		NULL);
		
	g_printf("Path Info\n");
	g_printf("   Device Path:      %s\n", device_path);
	g_printf("   Mount Point:      %s\n", mount_point);
	g_printf("   Control Path:     %s\n", control_path);
	g_printf("   HAL ID:           %s\n", hal_id);
	
	g_printf("Device Info\n");
	g_printf("   Model Number:     %s\n", model_number);
	g_printf("   Device Model:     %s\n", model_string);
	g_printf("   iPod Generation:  %s\n", generation_names[generation]);
	g_printf("   Adv. Capacity:    %s\n", adv_capacity);
	g_printf("   Is New:           %s\n", is_new ? "YES" : "NO");
	g_printf("   Writeable:        %s\n", can_write ? "YES" : "NO");
	g_printf("   Serial Number:    %s\n", serial_number);
	g_printf("   Firmware Version: %s\n", firmware_version);
	g_printf("Volume Info\n");
	g_printf("   Volume Size:      %lld\n", (long long int)volume_size);
	g_printf("   Volume Used:      %lld\n", (long long int)volume_used);
	g_printf("   Available         %lld\n", (long long int)volume_available);
	g_printf("   UUID:             %s\n", volume_uuid);
	g_printf("   Label             %s\n", volume_label);
	g_printf("User-Provided Info\n");
	g_printf("   Device Name:      %s\n", device_name);
	g_printf("   User Name:        %s\n", user_name);
	g_printf("   Host Name:        %s\n", host_name);
	
	g_printf("\n");
	fflush(stdout);
}

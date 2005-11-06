/* Time-stamp: <2005-10-10 01:23:28 jcs>
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
|  - libhal becomes optional (see #if HAVE_LIBHAL sections)
|  - publicly available functions were renamed from ipod_device_...()
|    to itdb_device_...()
|
|  Because of these changes only a limited amount of functionality is
|  available if libhal is not present. In particular the following
|  functions are non-functional:
|
|    GList *ipod_device_list_devices(void);
|    GList *ipod_device_list_device_udis(void);
|    guint ipod_device_eject(IpodDevice *device, GError **error);
|    guint ipod_device_reboot(IpodDevice *device, GError **error_out);
|
|  Only the following properties are available:
|
|
|    device-model
|    device-model-string
|    device-generation
|    advertised-capacity
|    is-new
|    can-write
|    serial-number
|    firmware-version
|    device-name
|    user-name
|    host-name
|
|
|  $Id$
*/
/* ex: set ts=4: */
/***************************************************************************
*  ipod-device.h
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

#ifndef IPOD_DEVICE_H
#define IPOD_DEVICE_H

#include <glib.h>
#include <glib-object.h>

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#if HAVE_LIBHAL
#  include <libhal.h>
#  include <dbus/dbus.h>
#  include <dbus/dbus-glib.h>
#endif

G_BEGIN_DECLS

#define TYPE_IPOD_DEVICE         (ipod_device_get_type ())
#define IPOD_DEVICE(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), TYPE_IPOD_DEVICE, IpodDevice))
#define IPOD_DEVICE_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), TYPE_IPOD_DEVICE, IpodDeviceClass))
#define IS_IPOD_DEVICE(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), TYPE_IPOD_DEVICE))
#define IS_IPOD_DEVICE_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), TYPE_IPOD_DEVICE))
#define IPOD_DEVICE_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), TYPE_IPOD_DEVICE, IpodDeviceClass))

typedef struct IpodDevicePrivate IpodDevicePrivate;

typedef struct {
	GObject parent;
	IpodDevicePrivate *priv;
} IpodDevice;

typedef struct {
	GObjectClass parent_class;
} IpodDeviceClass;

enum {
	UNKNOWN_GENERATION,
	FIRST_GENERATION,
	SECOND_GENERATION,
	THIRD_GENERATION,
	FOURTH_GENERATION,
	FIFTH_GENERATION
};

enum {
	MODEL_TYPE_INVALID,
	MODEL_TYPE_UNKNOWN,
	MODEL_TYPE_COLOR,
	MODEL_TYPE_COLOR_U2,
	MODEL_TYPE_REGULAR,
	MODEL_TYPE_REGULAR_U2,
	MODEL_TYPE_MINI,
	MODEL_TYPE_MINI_BLUE,
	MODEL_TYPE_MINI_PINK,
	MODEL_TYPE_MINI_GREEN,
	MODEL_TYPE_MINI_GOLD,
	MODEL_TYPE_SHUFFLE,
	MODEL_TYPE_NANO_WHITE,
	MODEL_TYPE_NANO_BLACK,
	MODEL_TYPE_VIDEO_WHITE,
	MODEL_TYPE_VIDEO_BLACK
};

enum {
	EJECT_OK,
	EJECT_ERROR,
	EJECT_BUSY
};

enum {
	PROP_IPOD_0,
	PROP_HAL_VOLUME_ID,
	PROP_HAL_CONTEXT,
	PROP_MOUNT_POINT,
	PROP_DEVICE_PATH,
	PROP_CONTROL_PATH,
	PROP_DEVICE_MODEL,
	PROP_DEVICE_MODEL_STRING,
	PROP_DEVICE_GENERATION,
	PROP_ADVERTISED_CAPACITY,
	PROP_DEVICE_NAME,
	PROP_USER_NAME,
	PROP_HOST_NAME,
	PROP_VOLUME_SIZE,
	PROP_VOLUME_AVAILABLE,
	PROP_VOLUME_USED,
	PROP_IS_IPOD,
	PROP_IS_NEW,
	PROP_SERIAL_NUMBER,
	PROP_MODEL_NUMBER,
	PROP_FIRMWARE_VERSION,
	PROP_VOLUME_UUID,
	PROP_VOLUME_LABEL,
	PROP_CAN_WRITE,
	PROP_ARTWORK_FORMAT
};

enum {
	ERROR_SAVE
};

enum { 
	IPOD_COVER_SMALL,
	IPOD_COVER_LARGE,
	IPOD_PHOTO_SMALL,
	IPOD_PHOTO_LARGE,
	IPOD_PHOTO_FULL_SCREEN,
	IPOD_PHOTO_TV_SCREEN
};


typedef struct {
	gint  type;
	gint16 width;
	gint16 height;
	gint16 correlation_id;
} IpodArtworkFormat;


GType itdb_device_get_type(void);
IpodDevice *itdb_device_new(const gchar *mount_point);
gboolean itdb_device_rescan_disk(IpodDevice *device);
guint itdb_device_eject(IpodDevice *device, GError **error);
guint itdb_device_reboot(IpodDevice *device, GError **error_out);
void itdb_device_debug(IpodDevice *device);
gboolean itdb_device_save(IpodDevice *device, GError **error);
GList *itdb_device_list_devices(void);
GList *itdb_device_list_device_udis(void);

G_END_DECLS

#endif /* IPOD_DEVICE_H */

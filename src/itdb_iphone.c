/*
|  Copyright (C) 2009 Nikias Bassen <nikias@gmx.li>
|  Part of the gtkpod project.
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
*/
#include <itdb.h>
#include <itdb_private.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#ifdef HAVE_LIBIMOBILEDEVICE
#include <libimobiledevice/libimobiledevice.h>
#include <libimobiledevice/lockdown.h>
#include <libimobiledevice/afc.h>
#include <libimobiledevice/notification_proxy.h>

struct itdbprep_int {
	idevice_t device;
	afc_client_t afc;
	uint64_t lockfile;
};
typedef struct itdbprep_int *itdbprep_t;

#define LOCK_ATTEMPTS	50
#define LOCK_WAIT	200000


static int itdb_iphone_post_notification(idevice_t device,
					 lockdownd_client_t client,
					 const char *notification)
{
    np_client_t np = NULL;
    uint16_t nport = 0;

    lockdownd_start_service(client, "com.apple.mobile.notification_proxy", &nport);
    if (!nport) {
	fprintf(stderr, "notification_proxy could not be started!\n");
	return -1;
    }

    np_client_new(device, nport, &np);
    if(!np) {
	fprintf(stderr, "connection to notification_proxy failed!\n");
	return -1;
    }

    if(np_post_notification(np, notification)) {
	fprintf(stderr, "failed to post notification!\n");
	np_client_free(np);
	return -1;
    }

    np_client_free(np);
    return 0;
}

int itdb_iphone_start_sync(Itdb_Device *device, void **prepdata)
{
    int res = 0;
    int sync_starting = 0;
    itdbprep_t pdata_loc = NULL;
    const char *uuid;
    lockdownd_client_t client = NULL;
    uint16_t afcport = 0;
    int i;

    uuid = itdb_device_get_uuid (device);

    if (!uuid) {
	fprintf(stderr, "Couldn't find get device UUID itdbprep processing won't work!");
	return -ENODEV;
    }

    printf("libitdbprep: %s called with uuid=%s\n", __func__, uuid);

    *prepdata = NULL;

    pdata_loc = g_new0 (struct itdbprep_int, 1);
    res = idevice_new(&pdata_loc->device, uuid);
    if (IDEVICE_E_SUCCESS != res) {
	fprintf(stderr, "No iPhone found, is it plugged in?\n");
	res = -ENODEV;
	goto leave_with_err;
    }
    if (LOCKDOWN_E_SUCCESS != lockdownd_client_new_with_handshake(pdata_loc->device, &client, "libgpod")) {
	fprintf(stderr, "Error: Could not establish lockdownd connection!\n");
	res = -1;
	goto leave_with_err;
    }

    lockdownd_start_service(client, "com.apple.afc", &afcport);
    if (!afcport) {
	fprintf(stderr, "Error: Could not start AFC service!\n");
	res = -1;
	goto leave_with_err;
    }
    afc_client_new(pdata_loc->device, afcport, &pdata_loc->afc);
    if (!pdata_loc->afc) {
	fprintf(stderr, "Error: Could not start AFC client!\n");
	res = -1;
	goto leave_with_err;
    }

    if (itdb_iphone_post_notification(pdata_loc->device, client, NP_SYNC_WILL_START)) {
	fprintf(stderr, "could not post syncWillStart notification!\n");
	res = -1;
	goto leave_with_err;
    }
    printf("%s: posted syncWillStart\n", __func__);
    sync_starting = 1;

    /* OPEN AND LOCK /com.apple.itunes.lock_sync */
    afc_file_open(pdata_loc->afc, "/com.apple.itunes.lock_sync", AFC_FOPEN_RW, &pdata_loc->lockfile);

    if (!pdata_loc->lockfile) {
	fprintf(stderr, "could not open lockfile\n");
	res = -1;
	goto leave_with_err;
    }

    if (itdb_iphone_post_notification(pdata_loc->device, client, "com.apple.itunes-mobdev.syncLockRequest")) {
	fprintf(stderr, "could not post syncLockRequest\n");
	res = -1;
	goto leave_with_err;
    }
    printf("%s: posted syncLockRequest\n", __func__);

    for (i=0; i<LOCK_ATTEMPTS; i++) {
	fprintf(stderr, "Locking for sync, attempt %d...\n", i);
	res = afc_file_lock(pdata_loc->afc, pdata_loc->lockfile, AFC_LOCK_EX);
	if (res == AFC_E_SUCCESS) {
	    break;
	} else if (res == AFC_E_OP_WOULD_BLOCK) {
	    usleep(LOCK_WAIT);
	    continue;
	} else {
	    fprintf(stderr, "ERROR: could not lock file! error code: %d\n", res);
	    res = -1;
	    goto leave_with_err;
	}
    }
    if (i == LOCK_ATTEMPTS) {
	fprintf(stderr, "ERROR: timeout while locking for sync\n");
	res = -1;
	goto leave_with_err;
    }

    if (itdb_iphone_post_notification(pdata_loc->device, client, NP_SYNC_DID_START)) {
	fprintf(stderr, "could not post syncDidStart\n");
	res = -1;
	goto leave_with_err;
    }
    printf("%s: posted syncDidStart\n", __func__);

    lockdownd_client_free(client);
    client = NULL;

    *prepdata = pdata_loc;

    return 0;

leave_with_err:
    if (client && sync_starting) {
	itdb_iphone_post_notification(pdata_loc->device, client, "com.apple.itunes-mobdev.syncFailedToStart");
	printf("%s: posted syncFailedToStart\n", __func__);
    }

    if (pdata_loc) {
	if (pdata_loc->afc) {
	    if (pdata_loc->lockfile) {
		afc_file_lock(pdata_loc->afc, pdata_loc->lockfile, AFC_LOCK_UN);
		afc_file_close(pdata_loc->afc, pdata_loc->lockfile);
		pdata_loc->lockfile = 0;
	    }
	    afc_client_free(pdata_loc->afc);
	    pdata_loc->afc = NULL;
	}
	if (pdata_loc->device) {
	    idevice_free(pdata_loc->device);
	    pdata_loc->device = NULL;
	}
	g_free(pdata_loc);
	pdata_loc = NULL;
    }

    if (client) {
	lockdownd_client_free(client);
	client = NULL;
    }

    *prepdata = NULL;

    return res;
}

int itdb_iphone_stop_sync(void *sync_ctx)
{
    lockdownd_client_t client = NULL;
    itdbprep_t prepdata = sync_ctx;

    printf("libitdbprep: %s called\n", __func__);

    if (!prepdata) {
	printf("%s called but prepdata is NULL!\n", __func__);
	return -1;
    }

    if (!prepdata->afc) {
	printf("%s called but prepdata->afc is NULL!\n", __func__);
    } else {
	/* remove .status-com.apple.itdbprep.command.runPostProcess */
	if (afc_remove_path(prepdata->afc, "/iTunes_Control/iTunes/iTunes Library.itlp/DBTemp/.status-com.apple.itdprep.command.runPostProcess") != IDEVICE_E_SUCCESS) {
	    fprintf(stderr, "Could not delete '.status-com.apple.itdprep.command.runPostProcess'\n");
	}
	/* remove ddd.itdbm */
	if (afc_remove_path(prepdata->afc, "/iTunes_Control/iTunes/iTunes Library.itlp/DBTemp/ddd.itdbm") != IDEVICE_E_SUCCESS) {
	    fprintf(stderr, "Could not delete 'ddd.itdbm'\n");
	}
	if (prepdata->lockfile) {
	    afc_file_lock(prepdata->afc, prepdata->lockfile, AFC_LOCK_UN);
	    afc_file_close(prepdata->afc, prepdata->lockfile);
	    prepdata->lockfile = 0;
	} else {
	    printf("%s called but lockfile is 0\n", __func__);
	}
	afc_client_free(prepdata->afc);
	prepdata->afc = NULL;
    }

    if (LOCKDOWN_E_SUCCESS != lockdownd_client_new_with_handshake(prepdata->device, &client, "libgpod")) {
	fprintf(stderr, "Error: Could not establish lockdownd connection!\n");
	goto leave;
    }

    if(itdb_iphone_post_notification(prepdata->device, client, NP_SYNC_DID_FINISH)) {
	fprintf(stderr, "failed to post syncDidFinish\n");
    }
    printf("%s: posted syncDidFinish\n", __func__);

    lockdownd_client_free(client);

leave:
    idevice_free(prepdata->device);

    g_free(prepdata);

    return 0;

}
#endif

#include <itdb.h>
#include <itdb_private.h>
#include <errno.h>
#include <stdio.h>
#ifdef HAVE_LIBIPHONE
#include <libiphone/libiphone.h>
#include <libiphone/lockdown.h>
#include <libiphone/afc.h>
#include <libiphone/notification_proxy.h>

struct itdbprep_int {
	iphone_device_t device;
	afc_client_t afc;
	uint64_t lockfile;
};
typedef struct itdbprep_int *itdbprep_t;

int itdb_iphone_start_sync(Itdb_Device *device, void **prepdata)
{
    int res = 0;
    int sync_started = 0;
    itdbprep_t pdata_loc = NULL;
    const char *uuid;
    lockdownd_client_t client = NULL;
    int afcport = 0;
    np_client_t np = NULL;
    int nport = 0;


    uuid = itdb_device_get_uuid (device);

    if (!uuid) {
	fprintf(stderr, "Couldn't find get device UUID itdbprep processing won't work!");
	return -ENODEV;
    }

    printf("libitdbprep: %s called with uuid=%s\n", __func__, uuid);

    *prepdata = NULL;

    pdata_loc = g_new0 (struct itdbprep_int, 1);

    if (IPHONE_E_SUCCESS != iphone_get_device_by_uuid(&pdata_loc->device, uuid)) {
	fprintf(stderr, "No iPhone found, is it plugged in?\n");
	res = -ENODEV;
	goto leave_with_err;
    }
    if (LOCKDOWN_E_SUCCESS != lockdownd_client_new(pdata_loc->device, &client)) {
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

    /* before we try to launch the 'sync in progress...' screen, we'll try
     * if the lockfile is lockable. If not, abort.
     */
    afc_file_open(pdata_loc->afc, "/com.apple.itunes.lock_sync", AFC_FOPEN_RW, &pdata_loc->lockfile);

    if (!pdata_loc->lockfile) {
	fprintf(stderr, "could not open lockfile in first place\n");
	res = -1;
	goto leave_with_err;
    }

    if (afc_file_lock(pdata_loc->afc, pdata_loc->lockfile, AFC_LOCK_EX) != IPHONE_E_SUCCESS) {
	fprintf(stderr, "ERROR: could not lock file in first place!\n");
	afc_file_close(pdata_loc->afc, pdata_loc->lockfile);
	pdata_loc->lockfile = 0;
	goto leave_with_err;
    }

    if (afc_file_lock(pdata_loc->afc, pdata_loc->lockfile, AFC_LOCK_UN) != IPHONE_E_SUCCESS) {
	fprintf(stderr, "ERROR: could not unlock file in first place!\n");
    }
    afc_file_close(pdata_loc->afc, pdata_loc->lockfile);
    pdata_loc->lockfile = 0;

    lockdownd_start_service(client, "com.apple.mobile.notification_proxy", &nport);
    if (nport) {
	np_client_new(pdata_loc->device, nport, &np);
	if (np) {
	    np_post_notification(np, NP_SYNC_WILL_START);
	    sync_started = 1;
	}
    }
    if (!nport || !np) {
	fprintf(stderr, "notification_proxy could not be started!\n");
	res = -1;
	goto leave_with_err;
    }
    np_client_free(np);

    nport = 0;
    np = NULL;

    /* OPEN AND LOCK /com.apple.itunes.lock_sync */ 
    afc_file_open(pdata_loc->afc, "/com.apple.itunes.lock_sync", AFC_FOPEN_RW, &pdata_loc->lockfile);

    if (!pdata_loc->lockfile) {
	fprintf(stderr, "could not open lockfile\n");
	res = -1;
	goto leave_with_err;
    }

    if (afc_file_lock(pdata_loc->afc, pdata_loc->lockfile, AFC_LOCK_EX) != IPHONE_E_SUCCESS) {
	fprintf(stderr, "ERROR: could not lock file!\n");
	afc_file_close(pdata_loc->afc, pdata_loc->lockfile);
	pdata_loc->lockfile = 0;
	goto leave_with_err;
    }

    lockdownd_start_service(client, "com.apple.mobile.notification_proxy", &nport);
    if (nport) {
	np_client_new(pdata_loc->device, nport, &np);
	if (np) {
	    np_post_notification(np, NP_SYNC_DID_START);
	}
    }
    if (!nport || !np) {
	fprintf(stderr, "notification_proxy could not be started!\n");
	res = -1;
	afc_file_lock(pdata_loc->afc, pdata_loc->lockfile, AFC_LOCK_UN);
	afc_file_close(pdata_loc->afc, pdata_loc->lockfile);
	pdata_loc->lockfile = 0;
	goto leave_with_err;
    }
    np_client_free(np);

    lockdownd_client_free(client);
    client = NULL;

    *prepdata = pdata_loc;

    return 0;

leave_with_err:
    if (client && sync_started) {
	nport = 0;
	np = NULL;
	lockdownd_start_service(client, "com.apple.mobile.notification_proxy", &nport);
	if (nport) {
	    np_client_new(pdata_loc->device, nport, &np);
	    if (np) {
		np_post_notification(np, NP_SYNC_DID_FINISH);
	    }
	}
	if (!nport || !np) {
	    fprintf(stderr, "notification_proxy could not be started!\n");
	    res = -1;
	}
	np_client_free(np);

	lockdownd_client_free(client);
    }
    if (pdata_loc) {
	if (pdata_loc->afc) {
	    afc_client_free(pdata_loc->afc);
	}
	if (pdata_loc->device) {
	    iphone_device_free(pdata_loc->device);
	}
	g_free(pdata_loc);
    }

    return res;
}

int itdb_iphone_stop_sync(void *sync_ctx)
{

    itdbprep_t prepdata = sync_ctx;
    printf("libitdbprep: %s called\n", __func__);

    if (!prepdata) {
	printf("%s called but prepdata is NULL!\n", __func__);
	return -1;
    }

    if (!prepdata->afc) {
	printf("%s called but prepdata->afc is NULL!\n", __func__);
    }

    /* remove .status-com.apple.itdbprep.command.runPostProcess */
    if (afc_remove_path(prepdata->afc, "/iTunes_Control/iTunes/iTunes Library.itlp/DBTemp/.status-com.apple.itdprep.command.runPostProcess") != IPHONE_E_SUCCESS) {
	fprintf(stderr, "Could not delete '.status-com.apple.itdprep.command.runPostProcess'\n");
    }
    /* remove ddd.itdbm */
    if (afc_remove_path(prepdata->afc, "/iTunes_Control/iTunes/iTunes Library.itlp/DBTemp/ddd.itdbm") != IPHONE_E_SUCCESS) {
	fprintf(stderr, "Could not delete 'ddd.itdbm'\n");
    }

    if (prepdata->lockfile) {
	afc_file_lock(prepdata->afc, prepdata->lockfile, AFC_LOCK_UN);
	afc_file_close(prepdata->afc, prepdata->lockfile);
	prepdata->lockfile = 0;
    } else {
	printf("%s called but lockfile is 0\n", __func__);
    }
    if (prepdata->afc) {
	afc_client_free(prepdata->afc);
	prepdata->afc = NULL;
    }

    iphone_device_free(prepdata->device);

    g_free(prepdata);

    return 0;
}
#endif

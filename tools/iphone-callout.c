#define _BSD_SOURCE 1 /* for daemon() */
#include <glib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

extern char *read_sysinfo_extended_by_uuid (const char *uuid);
extern gboolean iphone_write_sysinfo_extended (const char *uuid, const char *xml);

static gboolean write_sysinfo_extended (const char *uuid)
{
    gboolean write_ok;
    char *sysinfo_extended;

    sysinfo_extended = read_sysinfo_extended_by_uuid (uuid);
    if (sysinfo_extended == NULL) {
        g_print ("couldn't read SysInfoExtended from device %s\n", uuid);
        return FALSE;
    }

    write_ok = iphone_write_sysinfo_extended (uuid, sysinfo_extended);
    g_free (sysinfo_extended);

    if (!write_ok) {
        g_print ("couldn't write SysInfoExtended to device %s\n", uuid);
        return FALSE;
    }

    return TRUE;
}

int main (int argc, char **argv)
{
    int daemonize_failed;
    const char *uuid;

    if (argc != 1) {
        g_print ("%s should only be ran by HAL or udev\n", argv[0]);
        return 1;
    }

#ifdef USE_UDEV
    uuid = g_getenv ("ID_SERIAL_SHORT");
#else
    uuid = g_getenv ("HAL_PROP_USB_SERIAL");
#endif
    if (uuid == NULL) {
        g_print ("%s should only be ran by HAL or udev\n", argv[0]);
        return 1;
    }

    /* we don't want to block the calling process since it would delay udev
     * event handling, and usbmuxd might not be ready yet so we could be
     * blocking for some time. Just daemonize to run in the background, 
     * SysInfoExtended generation can be async anyway
     */
    daemonize_failed = daemon (0, 0);
    if (daemonize_failed) {
        g_print ("%s: daemon() failed: %s\n", argv[0], strerror (errno));
        return 1;
    }

    return write_sysinfo_extended (uuid);
}


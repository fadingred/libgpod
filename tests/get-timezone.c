/*
 * Compile with:
 * gcc $(pkg-config --cflags --libs libgpod-1.0) -o tz ./get-timezone.c
 *
 * then run:
 * ./tz <ipod-mountpoint>
 *
 * This should output something like:
 * Timezone: UTC+1 DST
 *
 * which means I'm living in an UTC+1 timezone with DST which adds a 1 hour
 * shift, ie my local time is UTC+2. DST won't be shown if not active.
 *
 */

#include <errno.h>
#include <stdio.h>
#include <itdb.h>

int main (int argc, char **argv)
{
    char *mountpoint;
    char *device_dir;
    char *prefs_filename;
    FILE *f;
    int result;
    gint32 timezone;
    const int GMT_OFFSET = 0x19;

    if (argc >= 2) {
        mountpoint = argv[1];
    } else {
        g_print ("Usage: %s <mountpoint>\n\n", g_basename(argv[0]));
        return -1;
    }

    device_dir = itdb_get_device_dir (mountpoint);
    if (device_dir == NULL) {
        g_print ("No iPod mounted at %s\n", mountpoint);
        return -1;
    }
    prefs_filename = itdb_get_path (device_dir, "Preferences");
    g_free (device_dir);

    f = fopen (prefs_filename, "r");
    if (f == NULL) {
        g_print ("Couldn't open %s: %s\n", prefs_filename, g_strerror (errno));
        g_free (prefs_filename);
        return -1;
    }

    result = fseek (f, 0xB10, SEEK_SET);
    if (result != 0) {
        g_print ("Couldn't seek in %s: %s\n", prefs_filename,
                 g_strerror (errno));
        fclose (f);
        g_free (prefs_filename);
        return -1;
    }

    result = fread (&timezone, sizeof (timezone), 1, f);
    if (result != 1) {
        g_print ("Couldn't read from %s: %s\n", prefs_filename,
                 g_strerror (errno));
        fclose (f);
        g_free (prefs_filename);
    }

    fclose (f);
    g_free (prefs_filename);

    timezone = GINT32_FROM_LE (timezone);
    timezone -= GMT_OFFSET;

    g_print ("Timezone: UTC%+d", timezone >> 1);
    if (timezone & 1) {
        g_print (" DST");
    }
    g_print ("\n");

    return 0;
}

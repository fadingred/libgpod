#include <glib.h>

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

    if (!write_ok) {
        g_print ("couldn't write SysInfoExtended to device %s\n", uuid);
        return FALSE;
    }

    return TRUE;
}

int main (int argc, char **argv)
{
    if (argc != 2) {
        g_print ("Usage: %s <uuid>\n", argv[0]);
        return 1;
    }

    return write_sysinfo_extended (argv[1]);
}


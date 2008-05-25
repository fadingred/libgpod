#include "itdb_sysinfo_extended_parser.h"

#include <glib.h>
#include <glib-object.h>

int main (int argc, char **argv)
{
    SysInfoIpodProperties *props;

    if (argc != 2)
        return(1);

    g_type_init ();
    props = itdb_sysinfo_extended_parse (argv[1]);
    if (props == NULL) {
        g_print ("Couldn't parse %s\n", argv[1]);
    }
    itdb_sysinfo_properties_dump (props);
    itdb_sysinfo_properties_free (props);
    props = NULL;

    return 0;
}


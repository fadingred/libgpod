#include "itdb_sysinfo_extended_parser.h"

#include <glib.h>
#include <glib-object.h>

int main (int argc, char **argv)
{
    SysInfoIpodProperties *props;
    GError *error = NULL;
    if (argc != 2)
        return(1);

    g_type_init ();
    props = itdb_sysinfo_extended_parse (argv[1], &error);
    if (props == NULL) {
        g_print ("Couldn't parse %s: %s\n", argv[1], error->message);
        return(2);
    }
    itdb_sysinfo_properties_dump (props);
    itdb_sysinfo_properties_free (props);
    props = NULL;

    return 0;
}


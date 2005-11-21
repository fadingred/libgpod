#ifndef GLIB_COMPAT_H
#define GLIB_COMPAT_H

#include <glib.h>

#if ((GLIB_MAJOR_VERSION == 2) && (GLIB_MINOR_VERSION <= 4))
/*** glib <= 2.4 ***/

/* G_GNUC_INTERNAL */
#ifndef G_GNUC_INTERNAL
#define G_GNUC_INTERNAL
#endif

/* g_stat */
#include<sys/types.h>
#include<sys/stat.h>
#include<unistd.h>
#define g_stat stat
#define g_mkdir mkdir
#define g_rename rename
#define g_printf printf

/* G_IS_DIR_SEPARATOR */
#ifdef G_OS_WIN32
#define G_IS_DIR_SEPARATOR(c) ((c) == G_DIR_SEPARATOR || (c) == '/')
#else
#define G_IS_DIR_SEPARATOR(c) ((c) == G_DIR_SEPARATOR)
#endif

#else
/*** glib > 2.4 ***/

/* g_stat */
#include <glib/gstdio.h>
#endif



#endif /* GLIB_COMPAT_H */

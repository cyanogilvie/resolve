#ifndef _RESOLVE_H
#define _RESOLVE_H

#include <tcl.h>

/* Tcl_Size compat for Tcl 8 */
#ifndef TCL_SIZE_MAX
typedef int Tcl_Size;
#  define TCL_SIZE_MODIFIER ""
#endif

#ifdef BUILD_resolve
#undef TCL_STORAGE_CLASS
#define TCL_STORAGE_CLASS DLLEXPORT
#endif

#endif

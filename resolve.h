#ifndef _RESOLVE_H
#define _RESOLVE_H

#include <tcl.h>

#ifdef BUILD_resolve
#undef TCL_STORAGE_CLASS
#define TCL_STORAGE_CLASS DLLEXPORT
#endif

#endif

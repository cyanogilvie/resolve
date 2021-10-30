#include "resolveInt.h"

#ifdef __cplusplus
extern "C" {
#endif

DLLEXPORT int Resolve_Init(Tcl_Interp* interp) //<<<
{
	Tcl_Namespace*		ns = NULL;

#ifdef USE_TCL_STUBS
	if (Tcl_InitStubs(interp, "8.6", 0) == NULL)
		return TCL_ERROR;
#endif

#define NS	"::resolve"

	ns = Tcl_CreateNamespace(interp, NS, NULL, NULL);
	TEST_OK(Tcl_Export(interp, ns, "*", 0));

#ifdef USE_RESOLVE_STUBS
	//TEST_OK(Tcl_PkgProvideEx(interp, PACKAGE_NAME, PACKAGE_VERSION, resolveStubs));
#else
	TEST_OK(Tcl_PkgProvide(interp, PACKAGE_NAME, PACKAGE_VERSION));
#endif

	return TCL_OK;
}

//>>>
DLLEXPORT int Resolve_SafeInit(Tcl_Interp* interp) //<<<
{
	// No unsafe features
	return Resolve_Init(interp);
}

//>>>
DLLEXPORT int Resolve_Unload(Tcl_Interp* interp, int flags) //<<<
{
	Tcl_Namespace*		ns = NULL;

	switch (flags) {
		case TCL_UNLOAD_DETACH_FROM_INTERPRETER:
		case TCL_UNLOAD_DETACH_FROM_PROCESS:
			ns = Tcl_FindNamespace(interp, "::resolve", NULL, TCL_GLOBAL_ONLY);
			if (ns) {
				Tcl_DeleteNamespace(ns);
				ns = NULL;
			}
			break;
		default:
			THROW_ERROR("Unhandled flags");
	}

	return TCL_OK;
}

//>>>
DLLEXPORT int Resolve_SafeUnload(Tcl_Interp* interp, int flags) //<<<
{
	// No unsafe features
	return Resolve_Unload(interp, flags);
}

//>>>

#ifdef __cplusplus
}
#endif

// vim: foldmethod=marker foldmarker=<<<,>>> ts=4 shiftwidth=4

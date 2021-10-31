#include "resolveInt.h"

static int getaddrinfo_cmd(ClientData cdata, Tcl_Interp* interp, int objc, Tcl_Obj* const objv[]) //<<<
{
	CHECK_ARGS(2, "node service");

	return TCL_OK;
}

//>>>
void free_interp_cx(ClientData cdata, Tcl_Interp* interp) //<<<
{
	struct interp_cx*	l = cdata;

	if (l) {
		/*
		close(l->pipe[PIPE_R]);
		close(l->pipe[PIPE_W]);
		*/
		Tcl_UnregisterChannel(interp, l->pipechan[PIPE_W]); l->pipechan[PIPE_W] = NULL;
		Tcl_UnregisterChannel(interp, l->pipechan[PIPE_R]); l->pipechan[PIPE_R] = NULL;
		ckfree(l); l = NULL;
	}
}

//>>>

#ifdef __cplusplus
extern "C" {
#endif

DLLEXPORT int Resolve_Init(Tcl_Interp* interp) //<<<
{
	Tcl_Namespace*		ns = NULL;
	struct interp_cx*	l = NULL;
	int					rc;

#ifdef USE_TCL_STUBS
	if (Tcl_InitStubs(interp, "8.6", 0) == NULL)
		return TCL_ERROR;
#endif

	l = (struct interp_cx*)ckalloc(sizeof *l);

	// IPC pipe <<<
#	if HAVE_PIPE2
	rc = pipe2(l->pipe, O_CLOEXEC | O_DIRECT);
#	else
	rc = pipe(l->pipe);
	fcntl(l->pipe[PIPE_R], F_SETFD, FD_CLOEXEC);
	fcntl(l->pipe[PIPE_W], F_SETFD, FD_CLOEXEC);
#	endif
	if (rc) {
		Tcl_SetObjResult(interp, Tcl_ObjPrintf("Error creating pipe: %s", Tcl_ErrnoMsg(Tcl_GetErrno())));
		return TCL_ERROR;
	}
	l->pipechan[PIPE_R] = Tcl_MakeFileChannel(INT2PTR(l->pipe[PIPE_R]), TCL_READABLE);
	l->pipechan[PIPE_W] = Tcl_MakeFileChannel(INT2PTR(l->pipe[PIPE_W]), TCL_WRITABLE);
	Tcl_RegisterChannel(interp, l->pipechan[PIPE_R]);
	Tcl_RegisterChannel(interp, l->pipechan[PIPE_W]);
	// IPC pipe >>>

	Tcl_SetAssocData(interp, "resolve", free_interp_cx, l);

#define NS	"::resolve"

	ns = Tcl_CreateNamespace(interp, NS, NULL, NULL);
	TEST_OK(Tcl_Export(interp, ns, "*", 0));

	TEST_OK(Tcl_CreateObjCommand(interp, NS "::getaddrinfo", getaddrinfo_cmd, NULL, NULL));

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

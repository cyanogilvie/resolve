#ifndef _RESOLVE_H
#define _RESOLVE_H

#include "resolve.h"
#include "tclstuff.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>

// Taken from tclInt.h:
#if !defined(INT2PTR) && !defined(PTR2INT)
#	if defined(HAVE_INTPTR_T) || defined(intptr_t)
#		define INT2PTR(p) ((void *)(intptr_t)(p))
#		define PTR2INT(p) ((intptr_t)(p))
#	else
#		define INT2PTR(p) ((void *)(p))
#		define PTR2INT(p) ((long)(p))
#	endif
#endif
#if !defined(UINT2PTR) && !defined(PTR2UINT)
#	if defined(HAVE_UINTPTR_T) || defined(uintptr_t)
#		define UINT2PTR(p) ((void *)(uintptr_t)(p))
#		define PTR2UINT(p) ((uintptr_t)(p))
#	else
#		define UINT2PTR(p) ((void *)(p))
#		define PTR2UINT(p) ((unsigned long)(p))
#	endif
#endif

#define PIPE_R	0
#define PIPE_W	1

struct interp_cx {
	int			pipe[2];		/* Results ready signal pipe from getaddrinfo_a */
	Tcl_Channel	pipechan[2];
	Tcl_Obj*	empty;
	Tcl_Obj*	t;
	Tcl_Obj*	f;
};

struct gai_cx {
	char*				cb;
	int					pipe_w;
	struct gaicb**		reqs;
	struct gaicb*		req;
	int					req_n;
	int*				reported;
	int					outstanding;
};

/* The fixed-length pipe message sent back to the thread and interp that is waiting for an async response */
#pragma pack(push,1)
struct pipe_msg {
	size_t			len;
	char*			msg;	/* Points to len bytes of string allocated with malloc,
							   receiver should retrieve and free */
};
#pragma pack(pop)

#endif

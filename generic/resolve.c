#include "resolveInt.h"

static void append_outcome(Tcl_DString* ds, const char* name, const char* service, const int rc, const struct addrinfo* addrs, const int serv_requested) //<<<
{
	const struct addrinfo*	ai = NULL;
	int						e;
	const char*				errnoid = NULL;
	const char*				gai_err_str = NULL;

	if (rc == EAI_SYSTEM) {
		e = Tcl_GetErrno();			// Save these now, before later calls could overwrite errno
		errnoid = Tcl_ErrnoId();
	}

	Tcl_DStringAppendElement(ds, name ? name : "");
	Tcl_DStringAppendElement(ds, service ? service : "");

	switch (rc) {
		case 0:					gai_err_str = "ok";					break;
#ifdef EAI_CANCELLED
		case EAI_CANCELED:		gai_err_str = "EAI_CANCELED";		break;
#endif
#ifdef EAI_ADDRFAMILY
		case EAI_ADDRFAMILY:	gai_err_str = "EAI_ADDRFAMILY";		break;
#endif
		case EAI_AGAIN:			gai_err_str = "EAI_AGAIN";			break;
		case EAI_BADFLAGS:		gai_err_str = "EAI_BADFLAGS";		break;
		case EAI_FAIL:			gai_err_str = "EAI_FAIL";			break;
		case EAI_FAMILY:		gai_err_str = "EAI_FAMILY";			break;
		case EAI_MEMORY:		gai_err_str = "EAI_MEMORY";			break;
#ifdef EAI_NODATA
		case EAI_NODATA:		gai_err_str = "EAI_NODATA";			break;
#endif
		case EAI_NONAME:		gai_err_str = "EAI_NONAME";			break;
		case EAI_SERVICE:		gai_err_str = "EAI_SERVICE";		break;
		case EAI_SOCKTYPE:		gai_err_str = "EAI_SOCKTYPE";		break;
		case EAI_SYSTEM:		gai_err_str = "EAI_SYSTEM";			break;
		default:				gai_err_str = "unknown";			break;
	}

	Tcl_DStringAppendElement(ds, gai_err_str);

	if (rc == EAI_SYSTEM) {
		Tcl_DStringAppendElement(ds, errnoid);
		Tcl_DStringAppendElement(ds, Tcl_ErrnoMsg(e));
	}

	if (rc) return;

	for (ai=addrs; ai; ai=ai->ai_next) {
		Tcl_DStringStartSublist(ds);

		Tcl_DStringAppendElement(ds, "ai_family");
		switch (ai->ai_family) {
			case AF_INET:	Tcl_DStringAppendElement(ds, "AF_INET");	break;
			case AF_INET6:	Tcl_DStringAppendElement(ds, "AF_INET6");	break;
			// TODO: somehow include all the others from socket.h here?
			default:		Tcl_DStringAppendElement(ds, "unhandled");	break;
		}

		Tcl_DStringAppendElement(ds, "ai_socktype");
		switch (ai->ai_socktype) {
			case SOCK_STREAM:		Tcl_DStringAppendElement(ds, "SOCK_STREAM");	break;
			case SOCK_DGRAM:		Tcl_DStringAppendElement(ds, "SOCK_DGRAM");	break;
#ifdef SOCK_RAW
			case SOCK_RAW:			Tcl_DStringAppendElement(ds, "SOCK_RAW");		break;
#endif
#ifdef SOCK_RDM
			case SOCK_RDM:			Tcl_DStringAppendElement(ds, "SOCK_RDM");		break;
#endif
#ifdef SOCK_SEQPACKET
			case SOCK_SEQPACKET:	Tcl_DStringAppendElement(ds, "SOCK_SEQPACKET");break;
#endif
#ifdef SOCK_DCCP
			case SOCK_DCCP:			Tcl_DStringAppendElement(ds, "SOCK_DCCP");		break;
#endif
#ifdef SOCK_PACKET
			case SOCK_PACKET:		Tcl_DStringAppendElement(ds, "SOCK_PACKET");	break;
#endif
			default:				Tcl_DStringAppendElement(ds, "unhandled");		break;
		}

		Tcl_DStringAppendElement(ds, "ai_protocol");
		switch (ai->ai_protocol) {
			case IPPROTO_IP:		Tcl_DStringAppendElement(ds, "IPPROTO_IP");	break;
			case IPPROTO_TCP:		Tcl_DStringAppendElement(ds, "IPPROTO_TCP");	break;
			case IPPROTO_UDP:		Tcl_DStringAppendElement(ds, "IPPROTO_UDP");	break;
#ifdef IPPROTO_SCTP
			case IPPROTO_SCTP:		Tcl_DStringAppendElement(ds, "IPPROTO_SCTP");	break;
#endif
			// TODO: somehow include all the others from netinet/in.h?
			default:				Tcl_DStringAppendElement(ds, "unhandled");		break;
		}

		{
#ifdef NI_MAXHOST
			char		host[NI_MAXHOST];
#else
			char		host[1025];
#endif
#ifdef NI_MAXSERV
			char		serv[NI_MAXSERV];
#else
			char		serv[32];
#endif
			const int	rc = getnameinfo(ai->ai_addr, ai->ai_addrlen, 
					host, sizeof(host), serv, sizeof(serv), NI_NUMERICHOST | NI_NUMERICSERV);

			if (rc) {
				Tcl_DStringAppendElement(ds, "error");
				Tcl_DStringAppendElement(ds, gai_strerror(rc));
			} else {
				if (host[0]) {
					Tcl_DStringAppendElement(ds, "addr");
					Tcl_DStringAppendElement(ds, host);
				}
				if (serv_requested && serv[0]) {	// Only include service part if it was requested
					Tcl_DStringAppendElement(ds, "serv");
					Tcl_DStringAppendElement(ds, serv);
				}
			}
		}

		if (ai->ai_canonname) {
			Tcl_DStringAppendElement(ds, "ai_canonname");
			Tcl_DStringAppendElement(ds, ai->ai_canonname);
		}

		Tcl_DStringEndSublist(ds);
	}
}

//>>>
static int getaddrinfo_threadworker_cmd(ClientData cdata, Tcl_Interp* interp, int objc, Tcl_Obj* const objv[]) //<<<
{
	int						retcode = TCL_OK;
	const char*				name = NULL;
	const char*				service = NULL;
	const char*				cb_str = NULL;
	const struct addrinfo*	hints = NULL;
	int						hints_len, rc;
	struct addrinfo*		res = NULL;
	Tcl_DString				cb;
	struct pipe_msg			msg;
	int						cb_len;
	int						pipe_w;

	CHECK_ARGS(5, "name service pipe_w cb compiled_hints");

	Tcl_DStringInit(&cb);

	name = Tcl_GetString(objv[1]);
	service = Tcl_GetString(objv[2]);
	TEST_OK_LABEL(done, retcode, Tcl_GetIntFromObj(interp, objv[3], &pipe_w));
	cb_str = Tcl_GetStringFromObj(objv[4], &cb_len);
	hints = (const struct addrinfo*)Tcl_GetByteArrayFromObj(objv[5], &hints_len);

	if (name[0] == 0)    name = NULL;
	if (service[0] == 0) service = NULL;

	if (!(name || service))
		THROW_ERROR_LABEL(done, retcode, "At least one of name or service must not be blank");
	
	if (hints_len != sizeof(*hints))
		THROW_ERROR_LABEL(done, retcode, "Compiled hints is the wrong size");

	Tcl_DStringAppend(&cb, cb_str, cb_len);

	Tcl_DStringStartSublist(&cb);
	rc = getaddrinfo(name, service, hints, &res);
	append_outcome(&cb, name, service, rc, res, service != NULL);
	Tcl_DStringEndSublist(&cb);

	/* Send only the len and a pointer to a copy of the string - that way the message is fixed-length,
	 * and short enough to be guaranteed not interleved with other writes by POSIX pipe semantics */
	msg.len = Tcl_DStringLength(&cb);
	msg.msg = strdup(Tcl_DStringValue(&cb));

	size_t					remain = sizeof(msg);
	const unsigned char*	bytes = (const unsigned char*)&msg;
	ssize_t					got;

again:
	got = write(pipe_w, bytes, remain);
	if (got == -1) {
		if (errno == EINTR) goto again;
		THROW_POSIX_LABEL(done, retcode, "Error writing getaddrinfo to pipe_w");
	} else if (got < remain) {
		// I think pipe semantics guarantees we can't land here
		remain -= got;
		bytes += got;
		goto again;
	}

done:
	Tcl_DStringFree(&cb);

	if (res) {
		freeaddrinfo(res);
		res = NULL;
	}
	return retcode;
}

//>>>
#if HAVE_GETADDRINFO_A
static void free_gai_cx(struct gai_cx* cx) //<<<
{
	int		i;

	if (cx) {
		if (cx->req) {
			for (i=0; i<cx->req_n; i++) {
				struct gaicb*	req = &cx->req[i];

				if (req->ar_name) {
					free((void*)req->ar_name);
					req->ar_name = NULL;
				}

				if (req->ar_service) {
					free((void*)req->ar_service);
					req->ar_service = NULL;
				}

				if (req->ar_request) {
					free((void*)req->ar_request);
					req->ar_request = NULL;
				}

				freeaddrinfo(req->ar_result);
				req = NULL;
			}

			free(cx->reqs);
			cx->reqs = NULL;

			free(cx->req);
			cx->req = NULL;
		}

		if (cx->reported) {
			free(cx->reported);
			cx->reported = NULL;
		}

		if (cx->cb) {
			free(cx->cb);
			cx->cb = NULL;
		}

		free(cx);
		cx = NULL;
	}
}

//>>>
static void notify_thread(sigval_t arg) //<<<
{
	struct gai_cx*	cx = (struct gai_cx*)arg.sival_ptr;
	int				i;
	Tcl_DString		cb;
	struct pipe_msg	msg;

	for (i=0; i<cx->req_n; i++) {
		struct gaicb*	req = &cx->req[i];
		const int		gai_err = gai_error(req);
		size_t			written;

		if (gai_err == EAI_INPROGRESS) continue;
		if (cx->reported[i]) continue;	/* We've flagged this request as already notified */

		Tcl_DStringInit(&cb);
		Tcl_DStringAppend(&cb, cx->cb, strlen(cx->cb));

		Tcl_DStringStartSublist(&cb);
		append_outcome(&cb, req->ar_name, req->ar_service, gai_err, req->ar_result, req->ar_service != NULL);
		Tcl_DStringEndSublist(&cb);

		/* Send only the len and a pointer to a copy of the string - that way the message is fixed-length,
		 * and short enough to be guaranteed not interleved with other writes by POSIX pipe semantics */
		msg.len = Tcl_DStringLength(&cb);
		msg.msg = strdup(Tcl_DStringValue(&cb));
		written = write(cx->pipe_w, &msg, sizeof(msg));
		Tcl_DStringFree(&cb);

		if (written != sizeof(msg)) {
			// TODO: what?
			Tcl_Panic("Short write on resolve notify chan - expected %ld, but wrote: %ld", sizeof(msg), written);
		}

		/* Flag this request as notified by NULLing name and service (a combination that isn't valid to request) */
		cx->reported[i] = 1;

		cx->outstanding--;
	}

	if (cx->outstanding <= 0) {
		free_gai_cx(cx);
		cx = NULL;
	}
}

//>>>
static int handle_response_cmd(ClientData cdata, Tcl_Interp* interp, int objc, Tcl_Obj* const objv[]) //<<<
{
	struct interp_cx*	l = Tcl_GetAssocData(interp, "resolve", NULL);
	struct pipe_msg		msg;
	int					got = read(l->pipe[PIPE_R], &msg, sizeof(msg));

	if (got == -1) {
		int		e = Tcl_GetErrno();

		// TODO: handle EINTR?
		Tcl_SetErrorCode(interp, "RESOLVE", "RESPONSE", Tcl_ErrnoId());
		Tcl_SetObjResult(interp, Tcl_ObjPrintf("Error reading from response pipe: %s", Tcl_ErrnoMsg(e)));
		return TCL_ERROR;
	}

	if (msg.msg) {
		int		retcode = TCL_OK;

		//Tcl_Obj*	cb = NULL;
		//replace_tclobj(&cb, Tcl_NewStringObj(msg.msg, msg.len));
		retcode = Tcl_EvalEx(interp, msg.msg, msg.len, TCL_EVAL_GLOBAL);
		free(msg.msg);
		msg.msg = NULL;
		return retcode;
	} else {
		/* msg.msg could be NULL if the strdup failed to allocate memory for the copy.  Things are far gone
		 * if that is the case, but at least don't make things worse here by segfaulting.
		 * Of course, in that case this error reporting will probably fail anyway, but maybe zippy still has
		 * some memory available where malloc didn't */
		Tcl_SetObjResult(interp, Tcl_ObjPrintf("%s", "Error retrieving response, probably due to memory exhaustion"));
		return TCL_ERROR;
	}
}

//>>>
static int getaddrinfo_a_cmd(ClientData cdata, Tcl_Interp* interp, int objc, Tcl_Obj* const objv[]) //<<<
{
	struct interp_cx*	l = Tcl_GetAssocData(interp, "resolve", NULL);
	int					retcode = TCL_OK;
	struct sigevent		sigev;
	int					rc;
	struct gai_cx*		cx = NULL;
	int					i;
	Tcl_Obj**			reqv;
	int					reqc;

	CHECK_ARGS(2, "reqs cb");

	TEST_OK_LABEL(err, retcode, Tcl_ListObjGetElements(interp, objv[1], &reqc, &reqv));

	cx = (struct gai_cx*)malloc(sizeof *cx);
	memset(cx, 0, sizeof *cx);
	cx->cb = strdup(Tcl_GetString(objv[2]));
	if (cx->cb == NULL)
		// Since the only way we should be able to get here is ENOMEM, this probably won't work:
		THROW_ERROR_LABEL(err, retcode, "Error copying callback string: ", Tcl_ErrnoMsg(Tcl_GetErrno()));

	cx->pipe_w = l->pipe[PIPE_W];
	cx->req_n = reqc;
	cx->outstanding = cx->req_n;
	cx->reported = (int*)malloc(sizeof(int) * cx->req_n);
	memset(cx->reported, 0, sizeof(int) * cx->req_n);

	cx->reqs = (struct gaicb**)malloc(sizeof(struct gaicb*) * cx->req_n);
	memset(cx->reqs, 0, sizeof(struct gaicb*)*cx->req_n);
	cx->req = (struct gaicb*)malloc(sizeof(*cx->req) * cx->req_n);
	memset(cx->req, 0, sizeof(*cx->req)*cx->req_n);

	for (i=0; i<cx->req_n; i++) {
		Tcl_Obj**				rfv;
		int						rfc;
		const char*				str;
		int						str_len;
		const unsigned char*	compiled_req = NULL;
		int						compiled_req_len;
		struct addrinfo*		req = NULL;

		TEST_OK_LABEL(err, retcode, Tcl_ListObjGetElements(interp, reqv[i], &rfc, &rfv));
		if (rfc != 3)
			THROW_ERROR_LABEL(err, retcode, "Each req must be a list of length 3");

		str = Tcl_GetStringFromObj(rfv[0], &str_len);
		if (str_len) {
			cx->req[i].ar_name = strdup(str);
			if (cx->req[i].ar_name == NULL) {
				// Since the only way we should be able to get here is ENOMEM, this probably won't work:
				Tcl_SetObjResult(interp, Tcl_ObjPrintf("Error copying name string: %s", Tcl_ErrnoMsg(Tcl_GetErrno())));
				retcode = TCL_ERROR;
				goto err;
			}
		}

		str = Tcl_GetStringFromObj(rfv[1], &str_len);
		if (str_len) {
			cx->req[i].ar_service = strdup(str);
			if (cx->req[i].ar_service == NULL)
				// Since the only way we should be able to get here is ENOMEM, this probably won't work:
				THROW_ERROR_LABEL(err, retcode, "Error copying service string: ", Tcl_ErrnoMsg(Tcl_GetErrno()));
		}

		compiled_req = Tcl_GetByteArrayFromObj(rfv[2], &compiled_req_len);
		if (compiled_req_len != sizeof(struct addrinfo))
			THROW_ERROR_LABEL(err, retcode, "Compiled request is the wrong size");

		req = malloc(compiled_req_len);
		memcpy(req, compiled_req, compiled_req_len);
		cx->req[i].ar_request = req;
		cx->reqs[i] = &cx->req[i];
	}

	sigev.sigev_notify = SIGEV_THREAD;
	sigev.sigev_value.sival_ptr = cx;
	sigev.sigev_notify_function = notify_thread;
	sigev.sigev_notify_attributes = NULL;

	rc = getaddrinfo_a(GAI_NOWAIT, cx->reqs, cx->req_n, &sigev);
	if (rc) {
		const char*		gai_err = NULL;

		Tcl_SetObjResult(interp, Tcl_ObjPrintf("Error dispatching getaddrinfo_a: %s", gai_strerror(rc)));
		switch (rc) {
			case EAI_AGAIN:		gai_err = "EAI_AGAIN";	break;
			case EAI_MEMORY:	gai_err = "EAI_MEMORY";	break;
			case EAI_SYSTEM:	gai_err = "EAI_SYSTEM";	break;
			default:			gai_err = "unknown";	break;
		}
		Tcl_SetErrorCode(interp, "RESOLVE", "GAI", gai_err, NULL);

		retcode = TCL_ERROR;
		goto err;
	}

	return retcode;

err:
	if (cx) {
		free_gai_cx(cx);
		cx = NULL;
	}

	return retcode;
}

//>>>
#endif
static int _getnameinfo_flags(Tcl_Interp* interp, Tcl_Obj* obj, int* flags) //<<<
{
	int					retcode = TCL_OK;
	int					res = 0;
	Tcl_Obj**			ov = NULL;
	int					oc;
	int					i;
	static const char* flag_str[] = {
		"NI_NAMEREQD",
		"NI_DGRAM",
		"NI_NOFQDN",
		"NI_NUMERICHOST",
		"NI_NUMERICSERV",
#ifdef NI_IDN
		"NI_IDN",
#endif
/* These are deprecated
#ifdef NI_IDN_ALLOW_UNASSIGNED
		"NI_IDN_ALLOW_UNASSIGNED",
#endif
#ifdef NI_IDN_USE_STD3_ASCII_RULES
		"NI_IDN_USE_STD3_ASCII_RULES",
#endif
*/
		NULL
	};
	int flag_map[] = {
		NI_NAMEREQD,
		NI_DGRAM,
		NI_NOFQDN,
		NI_NUMERICHOST,
		NI_NUMERICSERV,
#ifdef NI_IDN
		NI_IDN,
#endif
/* These are deprecated
#ifdef NI_IDN_ALLOW_UNASSIGNED
		NI_IDN_ALLOW_UNASSIGNED,
#endif
#ifdef NI_IDN_USE_STD3_ASCII_RULES
		NI_IDN_USE_STD3_ASCII_RULES,
#endif
*/
		0
	};

	TEST_OK_LABEL(done, retcode, Tcl_ListObjGetElements(interp, obj, &oc, &ov));

	for (i=0; i<oc; i++) {
		int	index;

		TEST_OK_LABEL(done, retcode, Tcl_GetIndexFromObj(interp, ov[i], flag_str, "flag", TCL_EXACT, &index));
		res |= flag_map[index];
	}

	*flags = res;

done:
	return retcode;
}

//>>>
static int getnameinfo_ipv4_cmd(ClientData cdata, Tcl_Interp* interp, int objc, Tcl_Obj* const objv[]) //<<<
{
	struct interp_cx*	l = Tcl_GetAssocData(interp, "resolve", NULL);
	int					retcode = TCL_OK;
	char				host[NI_MAXHOST];
	char				serv[NI_MAXSERV];
	char*				hostptr = host;
	char*				servptr = serv;
	socklen_t			hostptr_len = NI_MAXHOST;
	socklen_t			servptr_len = NI_MAXSERV;
	const char*			addr_str = NULL;
	const char*			serv_str = NULL;
	int					addr_str_len, serv_str_len;
	struct sockaddr_in	addr = {0};
	socklen_t			addrlen = sizeof(addr);
	int					rc, flags;
	Tcl_Obj*			res[2] = {NULL, NULL};

	CHECK_ARGS(3, "addr port flags");

	addr_str = Tcl_GetStringFromObj(objv[1], &addr_str_len);

	addr.sin_family = AF_INET;
	if (addr_str_len > 0) {
		if (inet_pton(AF_INET, addr_str, &addr.sin_addr.s_addr) != 1)
			THROW_ERROR_LABEL(done, retcode, "Invalid IPv4 address: \"", addr_str, "\"");
	} else {
		hostptr = NULL;
		hostptr_len = 0;
	}

	serv_str = Tcl_GetStringFromObj(objv[2], &serv_str_len);

	if (serv_str_len > 0) {
		int port;
		TEST_OK_LABEL(done, retcode, Tcl_GetIntFromObj(interp, objv[2], &port));
		if (port < 0 || port >= (1<<16))
			THROW_ERROR_LABEL(done, retcode, "Port is outside the valid range [0..2**16): ", serv_str);
		addr.sin_port = htons(port);
	} else {
		servptr = NULL;
		servptr_len = 0;
	}

	TEST_OK_LABEL(done, retcode, _getnameinfo_flags(interp, objv[3], &flags));

	if (hostptr_len || servptr_len) {
		rc = getnameinfo((struct sockaddr*)&addr, addrlen, hostptr, hostptr_len, servptr, servptr_len, flags);
		if (rc) {
			if (rc == EAI_SYSTEM) {
				Tcl_SetErrno(errno);
				Tcl_SetObjResult(interp, Tcl_NewStringObj(Tcl_PosixError(interp), -1));
				retcode = TCL_ERROR;
				goto done;
			} else {
				const char*		gai_err = NULL;

				Tcl_SetObjResult(interp, Tcl_ObjPrintf("getnameinfo error: %s", gai_strerror(rc)));
				switch (rc) {
					case EAI_AGAIN:		gai_err = "EAI_AGAIN";		break;
					case EAI_BADFLAGS:	gai_err = "EAI_BADFLAGS";	break;
					case EAI_FAIL:		gai_err = "EAI_FAIL";		break;
					case EAI_FAMILY:	gai_err = "EAI_FAMILY";		break;
					case EAI_MEMORY:	gai_err = "EAI_MEMORY";		break;
					case EAI_NONAME:	gai_err = "EAI_NONAME";		break;
					case EAI_OVERFLOW:	gai_err = "EAI_OVERFLOW";	break;
					default:			gai_err = "unknown";		break;
				}
				Tcl_SetErrorCode(interp, "RESOLVE", "GAI", gai_err, NULL);
				retcode = TCL_ERROR;
				goto done;
			}
		}
	}

	replace_tclobj(&res[0], hostptr_len ? Tcl_NewStringObj(host, -1) : l->empty);	// TODO: use dedup?
	replace_tclobj(&res[1], servptr_len ? Tcl_NewStringObj(serv, -1) : l->empty);

	Tcl_SetObjResult(interp, Tcl_NewListObj(2, res));

done:
	replace_tclobj(&res[0], NULL);
	replace_tclobj(&res[1], NULL);

	return retcode;
}

//>>>
static int getnameinfo_ipv6_cmd(ClientData cdata, Tcl_Interp* interp, int objc, Tcl_Obj* const objv[]) //<<<
{
	struct interp_cx*	l = Tcl_GetAssocData(interp, "resolve", NULL);
	int					retcode = TCL_OK;
	char				host[NI_MAXHOST];
	char				serv[NI_MAXSERV];
	char*				hostptr = host;
	char*				servptr = serv;
	socklen_t			hostptr_len = NI_MAXHOST;
	socklen_t			servptr_len = NI_MAXSERV;
	const char*			addr_str = NULL;
	const char*			serv_str = NULL;
	int					addr_str_len, serv_str_len;
	struct sockaddr_in6	addr = {0};
	socklen_t			addrlen = sizeof(addr);
	int					rc, flags;
	Tcl_Obj*			res[2] = {NULL, NULL};

	CHECK_ARGS(3, "addr port flags");

	addr_str = Tcl_GetStringFromObj(objv[1], &addr_str_len);

	addr.sin6_family = AF_INET6;
	if (addr_str_len > 0) {
		if (inet_pton(AF_INET6, addr_str, &addr.sin6_addr.s6_addr) != 1)
			THROW_ERROR_LABEL(done, retcode, "Invalid IPv6 address: \"", addr_str, "\"");
	} else {
		hostptr = NULL;
		hostptr_len = 0;
	}

	serv_str = Tcl_GetStringFromObj(objv[2], &serv_str_len);

	if (serv_str_len > 0) {
		int port;
		TEST_OK_LABEL(done, retcode, Tcl_GetIntFromObj(interp, objv[2], &port));
		if (port < 0 || port >= (1<<16))
			THROW_ERROR_LABEL(done, retcode, "Port is outside the valid range [0..2**16): ", serv_str);
		addr.sin6_port = htons(port);
	} else {
		servptr = NULL;
		servptr_len = 0;
	}

	TEST_OK_LABEL(done, retcode, _getnameinfo_flags(interp, objv[3], &flags));

	if (hostptr_len || servptr_len) {
		rc = getnameinfo((struct sockaddr*)&addr, addrlen, hostptr, hostptr_len, servptr, servptr_len, flags);
		if (rc) {
			if (rc == EAI_SYSTEM) {
				Tcl_SetErrno(errno);
				Tcl_SetObjResult(interp, Tcl_NewStringObj(Tcl_PosixError(interp), -1));
				retcode = TCL_ERROR;
				goto done;
			} else {
				const char*		gai_err = NULL;

				Tcl_SetObjResult(interp, Tcl_ObjPrintf("getnameinfo error: %s", gai_strerror(rc)));
				switch (rc) {
					case EAI_AGAIN:		gai_err = "EAI_AGAIN";		break;
					case EAI_BADFLAGS:	gai_err = "EAI_BADFLAGS";	break;
					case EAI_FAIL:		gai_err = "EAI_FAIL";		break;
					case EAI_FAMILY:	gai_err = "EAI_FAMILY";		break;
					case EAI_MEMORY:	gai_err = "EAI_MEMORY";		break;
					case EAI_NONAME:	gai_err = "EAI_NONAME";		break;
					case EAI_OVERFLOW:	gai_err = "EAI_OVERFLOW";	break;
					default:			gai_err = "unknown";		break;
				}
				Tcl_SetErrorCode(interp, "RESOLVE", "GAI", gai_err, NULL);
				retcode = TCL_ERROR;
				goto done;
			}
		}
	}

	replace_tclobj(&res[0], hostptr_len ? Tcl_NewStringObj(host, -1) : l->empty);	// TODO: use dedup?
	replace_tclobj(&res[1], servptr_len ? Tcl_NewStringObj(serv, -1) : l->empty);

	Tcl_SetObjResult(interp, Tcl_NewListObj(2, res));

done:
	replace_tclobj(&res[0], NULL);
	replace_tclobj(&res[1], NULL);

	return retcode;
}

//>>>
static int have_idn_cmd(ClientData cdata, Tcl_Interp* interp, int objc, Tcl_Obj* const objv[]) //<<<
{
	struct interp_cx*	l = Tcl_GetAssocData(interp, "resolve", NULL);
#ifdef NI_IDN
	const int			have_idn = 1;
#else
	const int			have_idn = 0;
#endif
	CHECK_ARGS(0, "");
	Tcl_SetObjResult(interp, have_idn ? l->t : l->f);
	return TCL_OK;
}

//>>>
static int compile_hints_cmd(ClientData cdata, Tcl_Interp* interp, int objc, Tcl_Obj* const objv[]) //<<<
{
	struct addrinfo	hints;
	static const char* family_str[] = {
		"AF_UNSPEC",
		"AF_INET",
		"AF_INET6",
		NULL
	};
	int family_map[] = {
		AF_UNSPEC,
		AF_INET,
		AF_INET6
	};
	int family_idx;
	static const char* protocol_str[] = {
		"",
		"IPPROTO_TCP",
		"IPPROTO_UDP",
		NULL,
	};
	int protocol_map[] = {
		0,
		IPPROTO_TCP,
		IPPROTO_UDP
	};
	int protocol_idx;
	static const char* socktype_str[] = {
		"",
		"SOCK_STREAM",
		"SOCK_DGRAM",
		NULL
	};
	int socktype_map[] = {
		0,
		SOCK_STREAM,
		SOCK_DGRAM
	};
	int socktype_idx;
	static const char* flags_str[] = {
		"AI_PASSIVE",
		"AI_CANONNAME",
		"AI_NUMERICHOST",
		"AI_V4MAPPED",
		"AI_ALL",
		"AI_ADDRCONFIG",
		"AI_NUMERICSERV",
		NULL
	};
	int flags_map[] = {
		AI_PASSIVE,
		AI_CANONNAME,
		AI_NUMERICHOST,
		AI_V4MAPPED,
		AI_ALL,
		AI_ADDRCONFIG,
		AI_NUMERICSERV
	};
	int flags_idx;
	Tcl_Obj**		flagv;
	int				flagc;
	int				i;

	CHECK_ARGS(4, "family protocol socktype flags");

	TEST_OK(Tcl_GetIndexFromObj(interp, objv[1], family_str,   "family",   TCL_EXACT, &family_idx));
	TEST_OK(Tcl_GetIndexFromObj(interp, objv[2], protocol_str, "protocol", TCL_EXACT, &protocol_idx));
	TEST_OK(Tcl_GetIndexFromObj(interp, objv[3], socktype_str, "socktype", TCL_EXACT, &socktype_idx));
	TEST_OK(Tcl_ListObjGetElements(interp, objv[4], &flagc, &flagv));

	memset(&hints, 0, sizeof(hints));

	hints.ai_family = family_map[family_idx];
	hints.ai_protocol = protocol_map[protocol_idx];
	hints.ai_socktype = socktype_map[socktype_idx];
	for (i=0; i<flagc; i++) {
		TEST_OK(Tcl_GetIndexFromObj(interp, flagv[i], flags_str, "flag", TCL_EXACT, &flags_idx));
		hints.ai_flags |= flags_map[flags_idx];
	}

	Tcl_SetObjResult(interp, Tcl_NewByteArrayObj((const unsigned char*)&hints, sizeof(hints)));

	return TCL_OK;
}

//>>>
void free_interp_cx(ClientData cdata, Tcl_Interp* interp) //<<<
{
	struct interp_cx*	l = cdata;

	if (l) {
		if (!Tcl_InterpDeleted(interp)) {
			if (TCL_OK != Tcl_EvalEx(interp, "::resolve::_unload", -1, TCL_EVAL_GLOBAL)) {
				fprintf(stderr, "Error running ::resolve::_unload: %s\n", Tcl_GetString(Tcl_GetObjResult(interp)));
			}
		}
		Tcl_UnregisterChannel(interp, l->pipechan[PIPE_R]);  l->pipechan[PIPE_R] = NULL;
		if (-1 == close(l->pipe[PIPE_W]))
			perror("close pipe_w");

		l->pipe[PIPE_W] = -1;
		l->pipe[PIPE_R] = -1;

		replace_tclobj(&l->empty, NULL);
		replace_tclobj(&l->t, NULL);
		replace_tclobj(&l->f, NULL);

		ckfree(l);  l = NULL;
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

#define NS	"::resolve"
	ns = Tcl_CreateNamespace(interp, NS, NULL, NULL);
	TEST_OK(Tcl_Export(interp, ns, "*", 0));

	l = (struct interp_cx*)ckalloc(sizeof *l);
	memset(l, 0, sizeof *l);

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

	replace_tclobj(&l->empty, Tcl_NewObj());
	replace_tclobj(&l->t, Tcl_NewBooleanObj(1));
	replace_tclobj(&l->f, Tcl_NewBooleanObj(0));

	Tcl_SetChannelOption(interp, l->pipechan[PIPE_R], "-translation", "binary");
	Tcl_SetChannelOption(interp, l->pipechan[PIPE_R], "-buffering",   "none");
	Tcl_RegisterChannel(interp, l->pipechan[PIPE_R]);
	Tcl_Obj*	pipe_r_varname = NULL;
	replace_tclobj(&pipe_r_varname, Tcl_NewStringObj(NS "::_result_pipe", -1));
	if (NULL == Tcl_ObjSetVar2(interp, pipe_r_varname, NULL, Tcl_NewStringObj(Tcl_GetChannelName(l->pipechan[PIPE_R]), -1), TCL_LEAVE_ERR_MSG)) {
		replace_tclobj(&pipe_r_varname, NULL);
		free_interp_cx(l, interp);
		return TCL_ERROR;
	}
	replace_tclobj(&pipe_r_varname, NULL);
	Tcl_Obj*	pipe_w_varname = NULL;
	replace_tclobj(&pipe_w_varname, Tcl_NewStringObj(NS "::_result_pipe_w", -1));
	if (NULL == Tcl_ObjSetVar2(interp, pipe_w_varname, NULL, Tcl_NewIntObj(l->pipe[PIPE_W]), TCL_LEAVE_ERR_MSG)) {
		replace_tclobj(&pipe_w_varname, NULL);
		free_interp_cx(l, interp);
		return TCL_ERROR;
	}
	replace_tclobj(&pipe_w_varname, NULL);
	// IPC pipe >>>

	Tcl_SetAssocData(interp, "resolve", free_interp_cx, l);

	Tcl_CreateObjCommand(interp, NS "::_getaddrinfo_threadworker", getaddrinfo_threadworker_cmd, NULL, NULL);
	Tcl_CreateObjCommand(interp, NS "::_compile_hints", compile_hints_cmd, NULL, NULL);
#if HAVE_GETADDRINFO_A
	Tcl_CreateObjCommand(interp, NS "::_getaddrinfo_a", getaddrinfo_a_cmd, NULL, NULL);
	Tcl_CreateObjCommand(interp, NS "::_handle_response", handle_response_cmd, NULL, NULL);
#endif
	Tcl_CreateObjCommand(interp, NS "::_getnameinfo_ipv4", getnameinfo_ipv4_cmd, NULL, NULL);
	Tcl_CreateObjCommand(interp, NS "::_getnameinfo_ipv6", getnameinfo_ipv6_cmd, NULL, NULL);
	Tcl_CreateObjCommand(interp, NS "::_have_idn", have_idn_cmd, NULL, NULL);

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
	int					retcode = TCL_OK;
	Tcl_Namespace*		ns = NULL;

	if (!Tcl_InterpDeleted(interp)) {
		Tcl_DeleteAssocData(interp, "resolve");
	}

	ns = Tcl_FindNamespace(interp, "::resolve", NULL, TCL_GLOBAL_ONLY);
	if (ns) {
		Tcl_DeleteNamespace(ns);
		ns = NULL;
	}

	return retcode;
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

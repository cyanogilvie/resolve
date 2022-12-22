package require gc_class
package require parse_args
package require sop
try {
	package require thread
} on error {} {
	package require Thread
}

namespace eval ::resolve {
	namespace path [list {*}{
		::parse_args
	} {*}[namespace path]]

	if {[llength [info commands ::log]] == 0} {
		proc log {lvl msg} {puts stderr $msg}
	} else {
		interp alias {} ::resolve::log {} ::log
	}

	# Internal machinery <<<
	# _handle_response defined in c, $_result_pipe set from c during init
	proc _result_pipe_readable {} { #<<<
		#::resolve::log notice "_result_pipe_readable"
		try {
			::resolve::_handle_response
		} on error {errmsg options} {
			::resolve::log error "Error handling response ([dict get $options -errorcode]): [dict get $options -errorinfo]"
		}
	}

	#>>>
	chan event $_result_pipe readable ::resolve::_result_pipe_readable

	proc _resolver_response {obj outcome} { # Chain getaddrinfo_a responses to the resolver instance that requested them, while handling the case of the instance having already died <<<
		if {![info object isa object $obj]} return	;# resolver instance has gone away
		try {
			$obj response $outcome
		} on error {errmsg options} {
			::resolve::log error "Error handling ${obj}::response ([dict get $options -errorcode]): [dict get $options -errorinfo]"
		}
	}

	#>>>

	proc _pool {} { #<<<
		tsv::lock _resolve {
			if {![tsv::exists _resolve pool]} {
				package require Thread
				set pool	[tpool::create -minworkers 1 -initcmd {load {} Resolve}]
				tsv::set _resolve pool $pool
			}
			if {![info exists ::resolve::_pool_reffed]} {
				tpool::preserve [tsv::get _resolve pool]
				set ::resolve::_pool_reffed 1
			}
		}

		tsv::get _resolve pool
	}

	#>>>
	proc _unload {} { # Internal handler for things to be cleaned up when the package is unloaded <<<
		tsv::lock _resolve {
			if {[tsv::exists _resolve pool]} {
				if {[info exists ::resolve::_pool_reffed]} {
					tpool::release [tsv::get _resolve pool]
					unset ::resolve::_pool_reffed
				}
			}
		}
	}

	#>>>
	# Internal machinery >>>

	gc_class create resolver { #<<<
		superclass ::sop::signalsource

		variable {*}{
			signals
			reqs
			results
			start
			pool
			implementation
		}

		constructor args { #<<<
			if {"::parse_args" ni [namespace path]} {
				namespace path [list {*}{
					::parse_args
				} {*}[namespace path]]
			}

			parse_args $args {
				-implementation	{-enum {getaddrinfo_a getaddrinfo} -# {Force an implementation backend, for testing}}
			}

			if {![info exists implementation]} {
				if {[llength [info commands ::resolve::_getaddrinfo_a]] > 0} {
					set implementation	getaddrinfo_a
				} else {
					set implementation	getaddrinfo
				}
			}

			set reqs			{}
			set results			{}
			array set signals	{}

			sop::gate new signals(ready) -mode and -default 0 -name "ready"	;# All requests resolved
			sop::signal new signals(sent) -name "sent"						;# Request has been dispatched

			if {[self next] ne ""} next
		}

		#>>>
		method add {name args} { # add a host and/or service to the lookup <<<
			# name: The hostname and/or service to resolve, as hostname(:service)?

			if {[$signals(sent) state]} {
				error "Already in flight, can't add more requests"
			}

			parse_args $args {
				-family		{-enum {AF_UNSPEC AF_INET AF_INET6} -default AF_UNSPEC}
				-protocol	{-enum {{} IPPROTO_TCP IPPROTO_UDP} -default IPPROTO_TCP}
				-socktype	{-enum {{} SOCK_STREAM SOCK_DGRAM}}
				-flags		{-default {AI_V4MAPPED AI_ADDRCONFIG}}
			}

			if {![info exists socktype]} {
				# MacOS overrides protocol with SOCK_STREAM if they conflict, so we have
				# to set a matching socktype default based on the protocol
				switch -exact -- $protocol {
					{}			{set socktype	{}}
					IPPROTO_TCP	{set socktype	SOCK_STREAM}
					IPPROTO_UDP	{set socktype	SOCK_DGRAM}
					default		{set socktype	{}}
				}
			}

			# TODO: handle IPv6 format like [::1]:https
			lassign [split $name :] host service
			if {[info exists signals(req_$name)]} {
				error "Already have a pending request for \"$name\""
			}

			lappend reqs	[list $host $service [::resolve::_compile_hints $family $protocol $socktype $flags]]

			sop::signal new signals(req_$name) -name "req_$name"
			$signals(ready) attach_input $signals(req_$name)
		}

		#>>>
		method go {} { #<<<
			if {[$signals(sent) state]} {
				error "Already in flight"
			}
			set start	[clock microseconds]
			$signals(sent) set_state 1
			switch -exact -- $implementation {
				getaddrinfo_a {
					::resolve::_getaddrinfo_a $reqs [list ::resolve::_resolver_response [self]]
				}

				getaddrinfo {
					package require Thread
					foreach req $reqs {
						lassign $req host service hints
						set thread_script	[subst {
							try {
								[list ::resolve::_getaddrinfo_threadworker $host $service $::resolve::_result_pipe_w [list ::resolve::_resolver_response [self]]] \[binary decode hex [binary encode hex $hints]\]
							} on error {errmsg options} {
								puts stderr "resolve pool thread \[thread::id\] got error: \$errmsg"
							}
						}]
						tpool::post -detached -nowait [::resolve::_pool] $thread_script
					}
				}

				default {
					error "Invalid implementation \"$implementation\""
				}
			}
		}

		#>>>
		method response outcome { #<<<
			set rest	[lassign $outcome host service status]

			set name	$host
			if {$service ne ""} {
				append name	:$service
			}

			switch -exact -- $status {
				ok {
					set addrs	$rest
					# each addr is a dict with keys: ai_family, ai_socktype, ai_protocol, addr (if a name was requested), serv (if a service was requested), ai_canonname (if one was returned)

					dict set results $name status ok
					dict set results $name addrs $addrs
				}
				EAI_SYSTEM {
					lassign $rest errno errmsg
					dict set results $name status error
					dict set results $name errorcode [list $status $errno]
					dict set results $name errmsg $errmsg
				}
				tclerror {
					lassign $rest o r
					dict set results $name status error
					dict set results $name errorcode	[list tclerror {*}[dict get $o -errorcode]]
					dict set results $name errmsg		$r
				}
				default {
					dict set results $name status error
					dict set results $name errorcode $status $rest
				}
			}

			$signals(req_$name) set_state 1
		}

		#>>>
		method get {name args} { #<<<
			parse_args $args {
				-timeout		{-default 3.0 -# {Maximum number of seconds to wait for the result before timing out}}
			}

			if {![$signals(sent) state]} {
				my go
			}
			if {![info exists signals(req_$name)]} {
				error "No request for \"$name\" was made"
			}
			if {![$signals(req_$name) state]} {
				$signals(req_$name) waitfor true [expr {int($timeout * 1e3)}]
			}

			switch -exact -- [dict get $results $name status] {
				ok {
					lmap addr [dict get $results $name addrs] {
						set res	{}
						if {[dict exist $addr addr]} {
							append res	[dict get $addr addr]
						}
						if {[dict exists $addr serv]} {
							if {[dict get $addr ai_family] eq "AF_INET6"} {
								set res	"\[$res\]"
							}
							append res	:[dict get $addr serv]
						}
						set res
					}
				}
				error {
					if {[dict exists $results $name errmsg]} {
						set errmsg	": [dict get $results $name errmsg]"
					} else {
						set errmsg	""
					}
					throw [list RESOLVE {*}[dict get $results $name errorcode]] "Failed to resolve $name$errmsg"
				}
				default {
					throw [list RESOLVE BAD_STATUS] "Invalid result status \"[dict get $results $name status]\""
				}
			}
		}

		#>>>
	}

	#>>>
	proc getnameinfo args { #<<<
		parse_args $args {
			-namerequired	{-boolean}
			-noidn			{-boolean}
			-dgram			{-boolean -# {Resolve serv as a datagram service}}
			-nofqdn			{-boolean}
			-numerichost	{-boolean}
			-numericserv	{-boolean}
			-ip				{-default {}}
			-port			{-default {}}
		}

		set flags	{}
		if {$namerequired}			{lappend flags NI_NAMEREQD}
		if {[_have_idn] && !$noidn}	{lappend flags NI_IDN}		;# IDN is the default, except on systems that don't support it (musl)
		if {$dgram}					{lappend flags NI_DGRAM}
		if {$nofqdn}				{lappend flags NI_NOFQDN}
		if {$numerichost}			{lappend flags NI_NUMERICHOST}
		if {$numericserv}			{lappend flags NI_NUMERICSERV}

		lassign [if {[string match *:* $ip]} {
			_getnameinfo_ipv6 $ip $port $flags
		} else {
			_getnameinfo_ipv4 $ip $port $flags
		}] host serv

		set res	{}
		if {$host ne ""} {
			dict set res host $host
		}
		if {$serv ne ""} {
			dict set res serv $serv
		}
		set res
	}

	#>>>
}

# vim: foldmethod=marker foldmarker=<<<,>>> ts=4 shiftwidth=4

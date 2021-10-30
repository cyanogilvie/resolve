namespace eval ::resolve {
	namespace ensemble create -prefixes no

	proc _async_tcl {name cb} { # Tcl fallback implementation <<<
		variable pool
		if {![info exists pool]} {
			package require Thread
			set pool	[tpool::create -minworkers 1 -initcmd {
				proc resolve {name tid cb} {
					# TODO: getaddrinfo command
					set addrs	[lmap {- addr} [regexp -all -inline -line { has address (.*)$} [exec host $name]] {
						set addr
					}]
					thread::send -async $tid [list {*}$cb $addrs]
				}
			}]
			tpool::preserve $pool
		}

		tpool::post -detached -nowait $pool [list resolve $name [thread::id] $cb]
	}

	#>>>

	if {[llength [info commands ::resolve::async]] == 0} {
		interp alias {} ::resolve::async {} ::resolve::_async_tcl
	}
}

# vim: foldmethod=marker foldmarker=<<<,>>> ts=4 shiftwidth=4

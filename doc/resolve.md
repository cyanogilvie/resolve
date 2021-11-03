% resolve(3) 0.1 | Advanced Name Resolution for Tcl Scripts
% Cyan Ogilvie
% 0.1


# NAME

resolve - Advanced name resolution for Tcl Scripts


# SYNOPSIS

**package require resolve** ?0.1?

**resolve::resolver instvar** *instancevar* ?**-implementation** **getaddrinfo_a**|**getaddrinfo**?

**resolve::resolver create** *instance* ?**-implementation** **getaddrinfo_a**|**getaddrinfo**?

**resolve::resolver new** ?**-implementation** **getaddrinfo_a**|**getaddrinfo**?

*instance* **add** *name* ?-*option* *value* ...?

*instance* **add** *name:service* ?*-option* *value* ...?

*instance* **add** *:service* ?-*option* *value* ...?

*instance* **go**

*instance* **get** *name* ?**-timeout** *seconds*?

*instance* **destroy**

# DESCRIPTION

Provides additional name resolution capabilities to Tcl scripts, like asynchronous
resolution, multiple addresses, filtering by address family (ipv4 / ipv6), etc.  Uses the
standard c library's name resolution functions, so the system config is respected for the
order and precedence of sources like the hosts file and DNS.

This package is designed as a thin layer around the POSIX standard function getaddrinfo
(and the GNU async extension getaddrinfo_a), so a lot of their dynamics show through in
the names of flags and protocols and such.  A good companion to this document is the
man page for getaddrinfo(3).


# COMMANDS

**resolve::resolver instvar** *instancevar* ?**-implementation** **getaddrinfo_a**|**getaddrinfo**?
:	Create a resolver instance and store the instance command in *instancevar*.  When
	*instancevar* is unset or its value is overwritten, the instance will be automatically
	destroyed.  *-implementation* is only intended for use in the test suite to allow
	it to exercise all available implementations.  By default, if getaddrinfo_a is available
	on the current platform it will be used, otherwise a fallback using a threadpool and
	getaddrinfo will be used.  The API is unchanged between the backend implementations.


**resolve::resolver create** *instance* ?**-implementation** **getaddrinfo_a**|**getaddrinfo**?
:	As for **instvar**, but without the lifecycle management tied to a variable.

**resolve::resolver new** ?**-implementation** **getaddrinfo_a**|**getaddrinfo**?
:	As for **instvar**, but without the lifecycle management tied to a variable.

*instance* **add** *name* ?-*option* *value* ...?
:	Add the host *name* to the set to be resolved.  See **ADD OPTIONS** for the available options.

*instance* **add** *name:service* ?*-option* *value* ...?
:	Add the pair of host *name* and service *service* to the set to be resolved.  See **ADD OPTIONS**
	for the available options.

*instance* **add** *:service* ?-*option* *value* ...?
:	Add the service *service* to the set to be resolved.  See **ADD OPTIONS** for the available options.

*instance* **go**
:	Dispatch the requests that were queued with **add**.  After this point no new requests can be
	added.

*instance* **get** *name* ?**-timeout** *seconds*?
:	Retrieve the *name* result (or *name:service* / *:service* variants).  If the result isn't
	ready yet, wait up to the fractional number of *seconds* for the result to arrive, or throw
	a timeout exception with the error code prefix: **SOP TIMEOUT**.  If the request is queued but
	hasn't been dispatched yet, do that implicitly and then wait for the result.  If this method
	is called in a coroutine context, then implement the blocking by yielding the active coroutine,
	otherwise block using vwait.  The coroutine case is preferred since it avoids the problems with
	nested vwaits.  Returns a list of addresses (there could be more than one address returned by
	DNS for instance).  The list is ordered as described in the RFC and the intent is that the
	addresses are tried in the order they are returned.

*instance* **destroy**
:	Destroys the instance and cleans up related resources, in the usual TclOO fashion.  Since
	the **resolver** class uses gc_class, it is also possible to just let the *instancevar* variable
	go out of scope and the instance will be automatically destroyed.


# ADD OPTIONS

Some options are available when adding a request to control the type of matches returned:

**-family** *AF_UNSPEC*|*AF_INET*|*AF_INET6*
:	Filter results by address family.  If *AF_UNSPEC* (the default) then return results from any
	address family (possibly modified by the **-flags** option).  *AF_INET* returns only IPv4,
	*AF_INET6* returns only IPv6.

**-protocol** *{}*|*IPPROTO_TCP*|*IPPROTO_UDP*
:	Filter results by protocol (only applies to service name lookups).  If an empty string
	then return results from any protocol.  *IPPROTO_TCP* (the default) returns services registered
	for TCP, and *IPPROTO_UDP* for udp.

**-socktype** *{}*|*SOCK_STREAM*|*SOCK_DGRAM*
:	Filter results by socket type (only applies to service name lookups).  An empty string
	returns any socket type, *SOCK_STREAM* (the default) returns only registrations for
	stream protocols, *SOCK_DGRAM* only datagram protocols.

**-flags** *list of flags*
:	Modify the request with the set of flags specified.  See **FLAGS** for the available choices.


# FLAGS

**AI_PASSIVE**
:	Return results suitable for binding to a listening socket, so :http -> 0.0.0.0:80.  Without
	this option the address portion will default to the localhost address, like: 127.0.0.1:80.

**AI_CANONNAME**
:	Return the canonical name for the host (if supplied by the resolution source).  Not properly
	exposed in the result yet.

**AI_NUMERICHOST**
:	Prevent a lookup on the supplied host.

**AI_V4MAPPED**
:	Return IPv4 addresses mapped into IPv6, if no native IPv6 addresses were found and the family
	was AF_INET6.

**AI_ALL**
:	Return IPv4 addresses mapped to IPv6 in addition to any native IPv6 matches found, if the family
	was AF_INET6 and AI_V4MAPPED was also set.  Ignored if AI_V4MAPPED is absent.

**AI_ADDRCONFIG**
:	Only return results for address families that the current host has addresses bound for, other
	than on the localhost virtual interface.

**AI_NUMERICSERV**
:	Prevent a lookup of the supplied service.


# SIGNALS EXPORTED

The **resolve::resolver** is a subclass of **::sop::signalsource** (of the **sop** package), and
exposes relevant state via that mechanism.  The exported signals are:

**ready**
:	True when all added requests have been resolved (whether successfully or not).

**req_$name**
:	True when the individual request for *$name* has been resolved.


# EXAMPLES

Lookup a few hostnames and services:

~~~tcl
resolve::resolver instvar lookup

# Simple lookup, will return IPv4 and/aor IPv6 addresses based on
# what the current host's interfaces are configured with
$lookup add google.com

# Resolve the hostname www.rubylane.com and the service name https,
# restricting the result to IPv4 addresses only
$lookup add www.rubylane.com:https -family AF_INET

# Resolve the numeric port number registered for the ssh service on tcp
$lookup add :ssh -protocol IPPROTO_TCP

# Dispatch the queued requests (optional: "get" will do this if we don't,
# but this lets us start the request and do other tasks in this thread
# while the lookup is underway)
$lookup go

# Do other things...

# Retrieve the results
puts "result for :ssh: [$lookup get :ssh]"
puts "result for www.rubylane.com:https: [$lookup get www.rubylane.com:https]"
puts "result for google.com: [$lookup get google.com]"
~~~

Produces output like:

~~~
result for :ssh: [::1]:22 127.0.0.1:22
result for www.rubylane.com:https: 35.169.104.128:443 34.195.117.50:443
result for google.com: 172.217.170.46 2c0f:fb50:4002:804::200e
~~~

# SEE ALSO

getaddrinfo(3)


# LICENSE

This package is Copyright 2021 Cyan Ogilvie, and is made available under the
same license terms as the Tcl Core


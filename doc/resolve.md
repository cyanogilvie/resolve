% resolve(3) 0.1 | Advanced Name Resolution for Tcl Scripts
% Cyan Ogilvie
% 0.1

# NAME

resolve - Advanced name resolution for Tcl Scripts

# SYNOPSIS

**package require resolve** ?0.1?

**resolve async** *name* *cb*

# DESCRIPTION

Provides additional name resolution capabilities to Tcl scripts, like asynchronous
resolution, multiple addresses, etc.

# COMMANDS

**resolve async** *name* *cb*
:	Resolve *name* asynchronously, and call *cb* with the result when ready.

# EXAMPLES

~~~tcl
resolve async google.com [list apply {addrs {
    puts "Resolved google.com to: $addrs"
}}]
~~~

# LICENSE

This package is Copyright 2021 Cyan Ogilvie, and is made available under the
same license terms as the Tcl Core


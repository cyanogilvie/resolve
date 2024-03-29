#!/usr/local/bin/tclsh

package require platform
puts "dbuild.tcl platform: [platform::generic]"
switch -glob -- [platform::generic] {
	*-x86_64 {
		set cflags	{-O3 -march=haswell}
	}

	*-aarch64 {
		set cflags	{-O3 -moutline-atomics -march=armv8.2-a}
	}

	default {
		set cflags	{-O3}
	}
}

file mkdir /tmp/build
cd /tmp/build
foreach file {
	aclocal.m4
	configure.ac
	tclconfig
	generic
	library
	tests
	pkgIndex.tcl.in
	Makefile.in
} {
	exec cp -a [file join /src/resolvelocal $file] .
}
exec -ignorestderr autoconf >@ stdout
exec -ignorestderr ./configure --enable-symbols --with-tcl=/usr/local/lib >@ stdout
exec -ignorestderr make clean install-binaries install-libraries DESTDIR=/tmp/install >@ stdout
set target	[file join /install [platform::generic]]
file mkdir $target
exec -ignorestderr sh -c "cp -a /tmp/install/usr/local/lib/* $target" >@ stdout
exec -ignorestderr chown -R [lindex $argv 0]:[lindex $argv 1] $target >@ stdout

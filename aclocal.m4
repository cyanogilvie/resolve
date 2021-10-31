#
# Include the TEA standard macro set
#

builtin(include,tclconfig/tcl.m4)

#
# Add here whatever m4 macros you want to define for your package
#

# All the best stuff seems to be Linux / glibc specific :(
AC_DEFUN([CHECK_GLIBC], [
	AC_MSG_CHECKING([for GNU libc])
	AC_TRY_COMPILE([#include <features.h>], [
#if ! (defined __GLIBC__ || defined __GNU_LIBRARY__)
#	error "Not glibc"
#endif
], glibc=yes, glibc=no)

	if test "$glibc" = yes
	then
		AC_DEFINE(_GNU_SOURCE, 1, [Always define _GNU_SOURCE when using glibc])
		AC_MSG_RESULT([yes])
	else
		AC_MSG_RESULT([no])
	fi
	])

AC_DEFUN([CHECK_GETADDRINFO_A], [
	AC_MSG_CHECKING([for getaddrinfo_a])
	AC_TRY_COMPILE([
		#include <netdb.h>
	], [
		void* f = &getaddrinfo_a;
	],
		have_getaddrinfo_a=yes,
		have_getaddrinfo_a=no)

	if test "$have_getaddrinfo_a" = yes
	then
		AC_DEFINE(HAVE_GETADDRINFO_A, 1, [Have getaddrinfo_a?])
		AC_MSG_RESULT([yes])
	else
		AC_DEFINE(HAVE_GETADDRINFO_A, 0, [Have getaddrinfo_a?])
		AC_MSG_RESULT([no])
	fi])

AC_DEFUN([CHECK_PIPE2], [
	AC_MSG_CHECKING([for pipe2])
	AC_TRY_COMPILE([
		#include <fcntl.h>
		#include <unistd.h>
	], [
		void* f = &pipe2;
	],
		have_pipe2=yes,
		have_pipe2=no)

	if test "$have_pipe2" = yes
	then
		AC_DEFINE(HAVE_PIPE2, 1, [Have pipe2?])
		AC_MSG_RESULT([yes])
	else
		AC_DEFINE(HAVE_PIPE2, 0, [Have pipe2?])
		AC_MSG_RESULT([no])
	fi])

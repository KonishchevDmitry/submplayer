AC_INIT([submplayer], [0.2.2], [konishchev@gmail.com])


AC_CONFIG_SRCDIR([src/main.cpp])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([config.h])


dnl foreign option tells Automake that this package is not follow the GNU
dnl Standards. GNU packages should always distribute additional files such as
dnl NEWS, README, etc.
dnl
dnl dist-bzip2 tells Automake also to create tar.bz2 archives during 'make dist'
dnl and 'make distcheck'.
AM_INIT_AUTOMAKE([-Wall -Werror foreign dist-bzip2])


AC_PROG_AWK
AC_PROG_SED
AC_PROG_CXX
AC_PROG_RANLIB
AC_PROG_INSTALL
AC_LANG([C++])

PKG_PROG_PKG_CONFIG
AM_GNU_GETTEXT([external])


dnl Configuring command line options -->
	AC_ARG_ENABLE(debug_mode,
		[AS_HELP_STRING([--enable-debug-mode],
			[enable debug mode for application])],
		[debug_mode=$enableval],
		[debug_mode=no]
	)

	AC_ARG_ENABLE(develop_mode,
		[AS_HELP_STRING([--enable-develop-mode],
			[enable develop mode for application (this mode is only for developer usage)])],
		[develop_mode=$enableval],
		[develop_mode=no]
	)

	if test "X$develop_mode" = "Xyes"
	then
		dnl Debug mode always enabled in develop mode
		debug_mode=yes
	fi

	AC_MSG_NOTICE([debug mode: $debug_mode])
	AC_MSG_NOTICE([develop mode: $develop_mode])
dnl Configuring command line options <--


dnl config.h -->
	AH_TOP([
#ifndef HEADER_CONFIG
#define HEADER_CONFIG
	])

	AH_TEMPLATE([APP_NAME], [Application name])
	AC_DEFINE_UNQUOTED([APP_NAME], ["$PACKAGE_NAME"])

	AH_TEMPLATE([APP_UNIX_NAME], [Application UNIX name])
	AC_DEFINE_UNQUOTED([APP_UNIX_NAME], ["$PACKAGE"])

	AH_TEMPLATE([APP_MAJOR_VERSION], [Application major version number])
	APP_MAJOR_VERSION=`echo "$PACKAGE_VERSION" | awk -F. '{ print $1; }'`
	AC_DEFINE_UNQUOTED([APP_MAJOR_VERSION], [$APP_MAJOR_VERSION])

	AH_TEMPLATE([APP_MINOR_VERSION], [Application minor version number])
	APP_MINOR_VERSION=`echo "$PACKAGE_VERSION" | awk -F. '{ print $2; }'`
	AC_DEFINE_UNQUOTED([APP_MINOR_VERSION], [$APP_MINOR_VERSION])

	AH_TEMPLATE([APP_SUB_MINOR_VERSION], [Application sub-minor version number])
	APP_SUB_MINOR_VERSION=`echo "$PACKAGE_VERSION" | awk -F. '{ if($3 == "") print 0; else print $3; }'`
	AC_DEFINE_UNQUOTED([APP_SUB_MINOR_VERSION], [$APP_SUB_MINOR_VERSION])

	AH_TEMPLATE([APP_VERSION_STRING], [Application version string])
	AC_DEFINE_UNQUOTED([APP_VERSION_STRING], ["$PACKAGE_VERSION"])


	AH_TEMPLATE([MLIB_DEBUG_MODE], [Is mlib compiling in debug mode])
	AH_TEMPLATE([DEBUG_MODE], [Is application compiling in debug mode])
	if test "X$debug_mode" = "Xyes"
	then
		AC_DEFINE([MLIB_DEBUG_MODE])
		AC_DEFINE([DEBUG_MODE])
	fi

	AH_TEMPLATE([MLIB_DEVELOP_MODE], [Is mlib compiling in develop mode])
	AH_TEMPLATE([DEVELOP_MODE], [Is application compiling in develop mode])
	if test "X$develop_mode" = "Xyes"
	then
		AC_DEFINE([DEVELOP_MODE])
		AC_DEFINE([MLIB_DEVELOP_MODE])
	fi

	AH_TEMPLATE([_LARGEFILE_SOURCE], [Large files support])
	AC_DEFINE([_LARGEFILE_SOURCE])

	AH_TEMPLATE([_FILE_OFFSET_BITS], [Large files support])
	AC_DEFINE([_FILE_OFFSET_BITS], [64])

	AH_TEMPLATE([MLIB_ENABLE_ALIASES], [Enable short functions and classes names from mlib])
	AC_DEFINE([MLIB_ENABLE_ALIASES])

	AH_TEMPLATE([MLIB_ENABLE_GTK], [Enable GTK support for mlib])
	AC_DEFINE([MLIB_ENABLE_GTK])

	AH_BOTTOM([
#endif
	])
dnl config.h <--


AM_CONDITIONAL([DEVELOP_MODE], [test "X$develop_mode" = "Xyes"])


dnl compiler flags -->
	if test "X$develop_mode" = "Xyes"
	then
		CXXFLAGS="$CXXFLAGS -O0 -g -Wall"
	fi

	COMMON_CPPFLAGS=""
	APP_DEPENDENCIES=""
	APP_LDADD=""

	dnl Including config.h anywhere
	COMMON_CPPFLAGS="-include \$(top_builddir)/config.h -I \$(top_srcdir)/src"

	dnl libraries -->
		dnl boost -->
			AX_BOOST_BASE([1.34])
			if test -z "$BOOST_LDFLAGS"
			then
				AC_MSG_ERROR([unable to find Boost library.])
			fi
			COMMON_CPPFLAGS="$COMMON_CPPFLAGS $BOOST_CPPFLAGS"
			APP_LDADD="$APP_LDADD $BOOST_LDFLAGS"

			AX_BOOST_FILESYSTEM
			if test -z "$BOOST_FILESYSTEM_LIB"
			then
				AC_MSG_ERROR([unable to find Boost.Filesystem library.])
			fi
			APP_LDADD="$APP_LDADD $BOOST_FILESYSTEM_LIB"

			AX_BOOST_THREAD
			if test -z "$BOOST_THREAD_LIB"
			then
				AC_MSG_ERROR([unable to find Boost.Thread library.])
			fi
			APP_LDADD="$APP_LDADD $BOOST_THREAD_LIB"

			AX_BOOST_SYSTEM
			if test -z "$BOOST_SYSTEM_LIB"
			then
				AC_MSG_ERROR([unable to find Boost.System library.])
			fi
			APP_LDADD="$APP_LDADD $BOOST_SYSTEM_LIB"
		dnl boost <--

		dnl GTK -->
			PKG_CHECK_MODULES([gthread], [gthread-2.0])
			COMMON_CPPFLAGS="$COMMON_CPPFLAGS $gthread_CFLAGS"
			APP_LDADD="$APP_LDADD $gthread_LIBS"

			PKG_CHECK_MODULES([gtkmm], [gtkmm-2.4])
			COMMON_CPPFLAGS="$COMMON_CPPFLAGS $gtkmm_CFLAGS"
			APP_LDADD="$APP_LDADD $gtkmm_LIBS"
		dnl GTK <--
	dnl libraries <--

	dnl Gettext libraries
	APP_LDADD="$APP_LDADD $LIBINTL"

	MLIB_CPPFLAGS="$COMMON_CPPFLAGS"
	MLIB_LIBRARIES="\$(top_builddir)/src/mlib/libmlib.a \$(top_builddir)/src/mlib/gtk/libmlib_gtk.a"
	APP_CPPFLAGS="$COMMON_CPPFLAGS -I \$(top_srcdir)/src/mlib/src -include \$(top_srcdir)/src/common.hpp"
	APP_DEPENDENCIES="$APP_DEPENDENCIES $MLIB_LIBRARIES"
	APP_LDADD="$APP_LDADD $MLIB_LIBRARIES"

	AC_SUBST([MLIB_CPPFLAGS])
	AC_SUBST([APP_DEPENDENCIES])
	AC_SUBST([APP_CPPFLAGS])
	AC_SUBST([APP_LDADD])
dnl compiler flags <--


AC_CONFIG_FILES([
	po/Makefile.in
	src/Makefile
	src/mlib/Makefile
	src/mlib/gtk/Makefile
	Makefile
])

AC_OUTPUT


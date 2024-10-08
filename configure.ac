dnl Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)

AC_INIT([engrampa], [1.28.2], [https://github.com/mate-desktop/engrampa/issues], [engrampa], [https://mate-desktop.org])
AM_INIT_AUTOMAKE([1.9 foreign dist-xz no-dist-gzip check-news])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_HEADER([config.h])
AC_CONFIG_MACRO_DIR([m4])

MATE_COMMON_INIT
MATE_DEBUG_CHECK([no])
MATE_COMPILE_WARNINGS

AC_PROG_CC
AM_DISABLE_STATIC
AC_PROG_LIBTOOL
PKG_PROG_PKG_CONFIG
GLIB_GSETTINGS

AC_PATH_PROG(GLIB_GENMARSHAL, glib-genmarshal)
AC_PATH_PROG(GLIB_MKENUMS, glib-mkenums)
AC_PATH_PROG(GLIB_COMPILE_RESOURCES, glib-compile-resources)

dnl ==========================================================================
dnl
dnl If you add a version number here, you *must* add an AC_SUBST line for
dnl it too, or it will never make it into the spec file!
dnl
dnl ==========================================================================

GLIB_REQUIRED=2.50.0
GIO_REQUIRED=2.50.0
GTK_REQUIRED=3.22.0
CAJA_REQUIRED=1.17.1
JSON_GLIB_REQUIRED=0.14.0
SM_REQUIRED=1.0.0
ICE_REQUIRED=1.0.0

AC_SUBST(GLIB_REQUIRED)
AC_SUBST(GIO_REQUIRED)
AC_SUBST(GTK_REQUIRED)
AC_SUBST(CAJA_REQUIRED)

dnl ===========================================================================

PKG_CHECK_MODULES(LIBEGG, [glib-2.0 >= $GLIB_REQUIRED
                           gtk+-3.0 >= $GTK_REQUIRED
                           sm >= $SM_REQUIRED
                           ice >= $ICE_REQUIRED])
AC_SUBST([LIBEGG_CFLAGS])
AC_SUBST([LIBEGG_LIBS])

dnl ===========================================================================

AC_ARG_ENABLE(run_in_place,
              AS_HELP_STRING([--enable-run-in-place],
                             [load ui data and extensions from the source tree]),,
              [enable_run_in_place=no])
AM_CONDITIONAL(RUN_IN_PLACE, test "x$enable_run_in_place" != xno)

dnl ===========================================================================

PKG_CHECK_MODULES(FR,					\
	glib-2.0 >= $GLIB_REQUIRED			\
	gthread-2.0					\
	gio-unix-2.0 >= $GIO_REQUIRED			\
	gtk+-3.0 >= $GTK_REQUIRED)
AC_SUBST(FR_CFLAGS)
AC_SUBST(FR_LIBS)

dnl ******************************

AC_ARG_ENABLE([caja_actions],
	[AS_HELP_STRING([--disable-caja-actions],
			[do not build the caja context menu actions])],,
	[enable_caja_actions=yes])

if test x"$enable_caja_actions" != x"no" ; then
	PKG_CHECK_MODULES(CAJA, \
			  libcaja-extension >= $CAJA_REQUIRED \
			  glib-2.0 \
			  gio-2.0)
	AC_SUBST(CAJA_CFLAGS)
	AC_SUBST(CAJA_LIBS)

fi
AM_CONDITIONAL(ENABLE_CAJA_ACTIONS, test "x$enable_caja_actions" = xyes)

AC_ARG_WITH(cajadir,
        AS_HELP_STRING([--with-cajadir=DIR],[Installation path for Caja extension @<:@auto@:>@]),
        [ac_with_cajadir=$withval],
        [ac_with_cajadir=""])
if test "${ac_with_cajadir}" = ""; then
        ac_with_cajadir=`$PKG_CONFIG --variable=extensiondir libcaja-extension`
fi

AC_MSG_NOTICE([installing caja plugin in ${ac_with_cajadir}])
AC_SUBST([CAJA_EXTENSION_DIR],[${ac_with_cajadir}])

dnl ******************************

PKG_CHECK_MODULES(JSON_GLIB,
		  [json-glib-1.0 >= $JSON_GLIB_REQUIRED
		   glib-2.0],
		  [enable_json_glib=yes],
		  [enable_json_glib=no])

if test x$enable_json_glib = xyes ; then
	AC_DEFINE(HAVE_JSON_GLIB, 1, [Define to 1 if json-glib support is included])
fi

AC_SUBST(JSON_GLIB_CFLAGS)
AC_SUBST(JSON_GLIB_LIBS)

AM_CONDITIONAL(ENABLE_JSON_GLIB, test x"$enable_json_glib" != x"no")

dnl ******************************

AC_CHECK_LIB(m, floor)

dnl ==========================================================================

AC_ARG_ENABLE(packagekit,
	      [AS_HELP_STRING([--disable-packagekit],[build without PackageKit support])],,
	      [enable_packagekit=yes])
if test "x$enable_packagekit" != "xno"; then
	AC_DEFINE(ENABLE_PACKAGEKIT, 1, [define to enable PackageKit installer])
fi
AM_CONDITIONAL(ENABLE_PACKAGEKIT, test x"$enable_packagekit" != x"no")

dnl ******************************

GETTEXT_PACKAGE=AC_PACKAGE_NAME
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE", [GetText Package])

AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION([0.19.8])
AM_GNU_GETTEXT_REQUIRE_VERSION([0.19.8])
AM_CONDITIONAL([USE_NLS], [test "x${USE_NLS}" = "xyes"])

dnl ******************************

AC_ARG_ENABLE([magic],
              AS_HELP_STRING([--enable-magic], [use libmagic to detect file type]),,
              [enable_magic=no])

if test x"$enable_magic" = x"yes" ; then
	save_LIBS="$LIBS"
	LIBS="$LIBS -lmagic"
	AC_MSG_CHECKING([whether libmagic works])
	AC_LINK_IFELSE([AC_LANG_SOURCE([
#include <magic.h>
int main () { magic_t m = magic_open(MAGIC_NONE); }
])],
		       [AC_MSG_RESULT([yes])],
		       [AC_MSG_RESULT([no])
		        AC_MSG_ERROR([libmagic is needed for magic])])

	LIBS="$save_LIBS"

	MAGIC_CFLAGS=
	MAGIC_LIBS=-lmagic

	AC_SUBST(MAGIC_CFLAGS)
	AC_SUBST(MAGIC_LIBS)

	AC_DEFINE(ENABLE_MAGIC, 1, [define to enable magic])
fi

AM_CONDITIONAL(ENABLE_MAGIC, test x"$enable_magic" != x"no")

AC_ARG_VAR([CPIO], [Path to the `cpio` command])
AC_PATH_PROGS(CPIO, gcpio cpio, cpio)
AC_DEFINE_UNQUOTED(CPIO_PATH, "${CPIO}", [Path to cpio])

dnl ******************************

YELP_HELP_INIT

dnl ******************************

AC_CONFIG_FILES([Makefile
		 data/Makefile
		 data/engrampa.appdata.xml.in
		 data/engrampa.desktop.in
		 data/org.mate.engrampa.gschema.xml
		 data/icons/Makefile
		 mate-submodules/Makefile
		 mate-submodules/libegg/Makefile
		 src/Makefile
		 src/sh/Makefile
		 src/ui/Makefile
		 caja/Makefile
		 caja/libcaja-engrampa.caja-extension.desktop.in
		 help/Makefile
		 po/Makefile.in])
AC_OUTPUT

echo "
Configure summary:

	${PACKAGE_STRING}
	`echo $PACKAGE_STRING | sed "s/./=/g"`

	Source code location:      ${srcdir}
	Compiler:                  ${CC}
	Compiler flags:            ${CFLAGS}
	Warning flags:             ${WARN_CFLAGS}
	Linker flags:              ${LDFLAGS}
	Caja support:              ${enable_caja_actions}
	PackageKit support:        ${enable_packagekit}
	Run in place               ${enable_run_in_place}
	Use libmagic:              ${enable_magic}
	JSON support:              ${enable_json_glib}
	Native Language support:   ${USE_NLS}
"

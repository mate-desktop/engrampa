dnl Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)

AC_INIT(file-roller, 2.32.0, [http://bugzilla.mate.org/enter_bug.cgi?product=file-roller])
AM_INIT_AUTOMAKE([1.9 foreign])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_HEADER([config.h])

MATE_COMMON_INIT

AC_PROG_CC
AM_DISABLE_STATIC
AC_PROG_LIBTOOL

AC_PATH_PROG(GLIB_GENMARSHAL, glib-genmarshal)
AC_PATH_PROG(GLIB_MKENUMS, glib-mkenums)

dnl ==========================================================================
dnl
dnl If you add a version number here, you *must* add an AC_SUBST line for
dnl it too, or it will never make it into the spec file!
dnl
dnl ==========================================================================

GLIB_REQUIRED=2.25.5
GIO_REQUIRED=2.25.5
MATECONF_REQUIRED=2.6.0
CAJA_REQUIRED=2.22.2

AC_MSG_CHECKING([which gtk+ version to compile against])
AC_ARG_WITH([gtk],
  [AS_HELP_STRING([--with-gtk=2.0|3.0],[which gtk+ version to compile against (default: 2.0)])],
  [case "$with_gtk" in
     2.0|3.0) ;;
     *) AC_MSG_ERROR([invalid gtk version specified]) ;;
   esac],
  [with_gtk=2.0])
AC_MSG_RESULT([$with_gtk])

case "$with_gtk" in
  2.0) GTK_API_VERSION=2.0
       GTK_REQUIRED=2.21.4
       ;;
  3.0) GTK_API_VERSION=3.0
       GTK_REQUIRED=2.90.0
       ;;
esac

AC_SUBST(GLIB_REQUIRED)
AC_SUBST(GIO_REQUIRED)
AC_SUBST(GTK_REQUIRED)
AC_SUBST(CAJA_REQUIRED)

dnl ===========================================================================

PKG_CHECK_MODULES(GTK, [gtk+-$GTK_API_VERSION >= $GTK_REQUIRED])
AC_SUBST([GTK_CFLAGS])
AC_SUBST([GTK_LIBS])

dnl ===========================================================================

AC_ARG_ENABLE(run_in_place,
              AS_HELP_STRING([--enable-run-in-place],[load ui data and extensions from the source tree]),
              [case "${enableval}" in
              yes) enable_run_in_place=yes ;;
              no) enable_run_in_place=no ;;
              *) AC_MSG_ERROR([bad value ${enableval} for --enable-run-in-place]) ;;
              esac],
              [enable_run_in_place=no])
AM_CONDITIONAL(RUN_IN_PLACE, test "x$enable_run_in_place" != xno)

dnl ===========================================================================

PKG_CHECK_MODULES(FR,					\
	glib-2.0 >= $GLIB_REQUIRED			\
	gthread-2.0					\
	gio-unix-2.0 >= $GIO_REQUIRED			\
	gtk+-$GTK_API_VERSION >= $GTK_REQUIRED		\
	mateconf-2.0 >= $MATECONF_REQUIRED)
AC_SUBST(FR_CFLAGS)
AC_SUBST(FR_LIBS)


CAJA_CFLAGS=""
CAJA_LIBS=""
build_caja_actions=no
AC_ARG_ENABLE(caja_actions,AS_HELP_STRING([--disable-caja-actions],[don't build the caja context menu actions]),)

if test x"$enable_caja_actions" != xno; then
	if pkg-config --atleast-version=$CAJA_REQUIRED libcaja-extension; then
		CAJA_CFLAGS=`pkg-config --cflags libcaja-extension glib-2.0 gio-2.0`
		CAJA_LIBS=`pkg-config --libs libcaja-extension glib-2.0 gio-2.0`
		build_caja_actions=yes
	fi
fi
AM_CONDITIONAL(ENABLE_CAJA_ACTIONS, test "x$build_caja_actions" = xyes)
AC_SUBST(CAJA_CFLAGS)
AC_SUBST(CAJA_LIBS)


dnl Checks for mkdtemp function

mkdtemp_missing=false
AC_CHECK_FUNC(mkdtemp,
    [AC_DEFINE([HAVE_MKDTEMP], 1, [Have GlibC function to make temp dirs])],
    mkdtemp_missing=true)
AM_CONDITIONAL(MKDTEMP_MISSING, test x$mkdtemp_missing = xtrue)

AC_CHECK_LIB(m, floor)

dnl ==========================================================================

AC_ARG_ENABLE(packagekit,
	      [AC_HELP_STRING([--disable-packagekit],[build without PackageKit support])],,
	      [enable_packagekit=yes])
if test "x$enable_packagekit" != "xno"; then
	AC_DEFINE(ENABLE_PACKAGEKIT, 1, [define to enable PackageKit installer])
fi

dnl ******************************

GETTEXT_PACKAGE=file-roller
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE", [GetText Package])

AM_GLIB_GNU_GETTEXT
IT_PROG_INTLTOOL([0.35.0])

if test "x${prefix}" = "xNONE"; then
  AC_DEFINE_UNQUOTED(LOCALEDIR, "${ac_default_prefix}/${DATADIRNAME}/locale", [Locale dir])
else
  AC_DEFINE_UNQUOTED(LOCALEDIR, "${prefix}/${DATADIRNAME}/locale", [Locale dir])
fi

dnl ******************************

PKG_PROG_PKG_CONFIG
MATE_DOC_INIT

dnl ******************************

AC_ARG_ENABLE(deprecations,AS_HELP_STRING([--enable-deprecations],[warn about deprecated usages]),)
if test "x$enable_deprecations" = "xyes"; then
        DISABLE_DEPRECATED="-DGSEAL_ENABLE -DG_DISABLE_DEPRECATED -DGDK_DISABLE_DEPRECATED -DGDK_PIXBUF_DISABLE_DEPRECATED -DGTK_DISABLE_DEPRECATED"
fi
AC_SUBST(DISABLE_DEPRECATED)

dnl ******************************

AC_PATH_PROG(MATECONFTOOL, mateconftool-2, no)
if test x"$MATECONFTOOL" = xno; then
	AC_MSG_ERROR([mateconftool-2 executable not found in your path - should be installed with MateConf])
fi
AM_MATECONF_SOURCE_2

dnl ******************************

CAJA_EXTENSION_DIR=`$PKG_CONFIG --variable=extensiondir libcaja-extension`
AC_SUBST(CAJA_EXTENSION_DIR)

dnl ******************************

AC_CONFIG_FILES([Makefile
		 file-roller.spec
		 data/Makefile
		 data/file-roller.desktop.in
		 data/ui/Makefile
		 data/icons/Makefile
		 data/icons/16x16/Makefile
		 data/icons/16x16/actions/Makefile
		 data/icons/16x16/apps/Makefile
		 data/icons/22x22/Makefile
		 data/icons/22x22/apps/Makefile
		 data/icons/24x24/Makefile
		 data/icons/24x24/actions/Makefile
		 data/icons/24x24/apps/Makefile
		 data/icons/32x32/Makefile
		 data/icons/32x32/apps/Makefile
		 data/icons/scalable/Makefile
		 data/icons/scalable/apps/Makefile
		 copy-n-paste/Makefile
		 src/Makefile
		 src/commands/Makefile
		 src/sh/Makefile
		 caja/Makefile
		 help/Makefile
		 po/Makefile.in])
AC_OUTPUT

echo "
Configuration:

	Source code location:   ${srcdir}
	Compiler:               ${CC}
	Internal mkdtemp:       ${mkdtemp_missing}
	Caja support:       ${build_caja_actions}
	PackageKit support:     ${enable_packagekit}
	Run in place            ${enable_run_in_place}
"

AC_INIT([c-smie], [0])
AC_PREREQ([2.62])

AM_INIT_AUTOMAKE

AC_PROG_CC
AC_PROG_YACC

LT_INIT

PKG_CHECK_MODULES([DEPS], [glib-2.0 >= 2.44])

AC_ARG_ENABLE([gtksourceview],
  [AS_HELP_STRING([--disable-gtksourceview],
     [disable gtksourceview])],
  [enable_gtksourceview="$enableval"],
  [enable_gtksourceview=auto])

if test "$enable_gtksourceview" != no; then
  PKG_CHECK_MODULES([GTKSOURCEVIEW], [gtksourceview-3.0 gtk+-3.0 >= 3.10],
    have_gtksourceview=yes, have_gtksourceview=no)
  AC_PATH_PROG(GLIB_COMPILE_RESOURCES, glib-compile-resources)
fi
AM_CONDITIONAL([ENABLE_GTKSOURCEVIEW], [test "$have_gtksourceview" = yes])

AC_CONFIG_MACRO_DIRS([m4])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile])
AC_OUTPUT

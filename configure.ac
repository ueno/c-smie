AC_INIT([c-smie], [0])
AC_PREREQ([2.62])

AM_INIT_AUTOMAKE([subdir-objects])

AC_PROG_CC
AC_PROG_YACC

LT_INIT

PKG_CHECK_MODULES([DEPS], [glib-2.0 >= 2.32])

AC_ARG_ENABLE([gtksourceview],
  [AS_HELP_STRING([--disable-gtksourceview],
     [disable gtksourceview])],
  [enable_gtksourceview="$enableval"],
  [enable_gtksourceview=auto])

dnl "editor" example uses G_DECLARE_FINAL_TYPE, which was added in GLib 2.44
if test "$enable_gtksourceview" != no; then
  PKG_CHECK_MODULES([GTKSOURCEVIEW],
    [gtksourceview-3.0 gtk+-3.0 >= 3.10 gobject-2.0 >= 2.44],
    have_gtksourceview=yes, have_gtksourceview=no)
  AC_PATH_PROG(GLIB_COMPILE_RESOURCES, glib-compile-resources)
fi
AM_CONDITIONAL([ENABLE_GTKSOURCEVIEW], [test "$have_gtksourceview" = yes])

dnl check for gtk-doc
m4_ifdef([GTK_DOC_CHECK], [
GTK_DOC_CHECK([1.14],[--flavour no-tmpl])
],[
AM_CONDITIONAL([ENABLE_GTK_DOC], false)
])

AC_CONFIG_MACRO_DIRS([m4])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile
docs/Makefile
docs/references/Makefile
docs/references/libsmie/Makefile])
AC_OUTPUT

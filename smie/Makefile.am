ACLOCAL_AMFLAGS = -I m4
AM_YFLAGS = -d

BUILT_SOURCES = smie/smie-gram-gen.h

nobase_include_HEADERS =			\
	smie/smie-grammar.h			\
	smie/smie-indenter.h			\
	smie/smie.h

lib_LTLIBRARIES = libsmie.la
libsmie_la_SOURCES =				\
	smie/smie-private.h			\
	smie/smie-grammar.c			\
	smie/smie-gram-gen.y			\
	smie/smie-indenter.c

libsmie_la_CFLAGS = $(DEPS_CFLAGS)
libsmie_la_LIBADD = $(DEPS_LIBS)
libsmie_la_LDFLAGS = -no-undefined -export-symbols-regex '^smie_'

if ENABLE_GTKSOURCEVIEW
lib_LTLIBRARIES += libsmiegtksourceview.la
libsmiegtksourceview_la_SOURCES =		\
	smie/smie-gtksourceview.h		\
	smie/smie-gtksourceview.c
libsmiegtksourceview_la_CFLAGS = $(DEPS_CFLAGS) $(GTKSOURCEVIEW_CFLAGS)
libsmiegtksourceview_la_LIBADD = libsmie.la $(DEPS_LIBS) $(GTKSOURCEVIEW_LIBS)
libsmiegtksourceview_la_LDFLAGS = -no-undefined -export-symbols-regex '^smie_'
endif

SUBDIRS = docs

ACLOCAL_AMFLAGS = -I m4
AM_YFLAGS = -d

BUILT_SOURCES = smie-gram-gen.h

include_HEADERS =				\
	smie-grammar.h				\
	smie-indenter.h				\
	smie.h

lib_LTLIBRARIES = libsmie.la
libsmie_la_SOURCES =				\
	smie-private.h				\
	smie-grammar.c				\
	smie-gram-gen.y				\
	smie-indenter.c

libsmie_la_CFLAGS = $(DEPS_CFLAGS)
libsmie_la_LIBADD = $(DEPS_LIBS)
libsmie_la_LDFLAGS = -no-undefined -export-symbols-regex '^smie_'

EXTRA_DIST =

if ENABLE_GTKSOURCEVIEW
lib_LTLIBRARIES += libsmiegtksourceview.la
libsmiegtksourceview_la_SOURCES = smie-gtksourceview.h smie-gtksourceview.c
libsmiegtksourceview_la_CFLAGS = $(DEPS_CFLAGS) $(GTKSOURCEVIEW_CFLAGS)
libsmiegtksourceview_la_LIBADD = libsmie.la $(DEPS_LIBS) $(GTKSOURCEVIEW_LIBS)
libsmiegtksourceview_la_LDFLAGS = -no-undefined -export-symbols-regex '^smie_'

noinst_PROGRAMS = editor
editor_SOURCES = editor.c editor-resources.c
editor_CFLAGS = $(DEPS_CFLAGS) $(GTKSOURCEVIEW_CFLAGS)
editor_LDADD = libsmiegtksourceview.la libsmie.la $(DEPS_LIBS) \
	$(GTKSOURCEVIEW_LIBS)

editor-resources.c: editor.gresource.xml editor.ui
	$(AM_V_GEN) $(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir) --generate-source $(srcdir)/editor.gresource.xml
endif

TESTS = test-grammar test-indenter

check_LTLIBRARIES = libtest.la
libtest_la_SOURCES = smie-test.h smie-test.c
libtest_la_CFLAGS = $(DEPS_CFLAGS)
libtest_la_LIBADD = $(DEPS_LIBS)

check_PROGRAMS = test-grammar test-indenter
test_grammar_SOURCES = test-grammar.c
test_grammar_CFLAGS = $(DEPS_CFLAGS)
test_grammar_LDADD = libtest.la libsmie.la $(DEPS_LIBS)

test_indenter_SOURCES = test-indenter.c
test_indenter_CFLAGS = \
	$(DEPS_CFLAGS) \
	-DGRAMMAR_FILE=\"$(srcdir)/test.grammar\" \
	-DINPUT_FILE=\"$(srcdir)/test.input\"
test_indenter_LDADD = libtest.la libsmie.la $(DEPS_LIBS)

EXTRA_DIST += test.grammar test.input

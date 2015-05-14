TESTS = test-grammar test-indenter

check_LTLIBRARIES = libtest.la
libtest_la_SOURCES = tests/test-common.h tests/test-common.c
libtest_la_CFLAGS = $(DEPS_CFLAGS)
libtest_la_LIBADD = $(DEPS_LIBS)

check_PROGRAMS = test-grammar test-indenter
test_grammar_SOURCES = tests/test-grammar.c
test_grammar_CFLAGS = $(DEPS_CFLAGS)
test_grammar_LDADD = libtest.la libsmie.la $(DEPS_LIBS)

test_indenter_SOURCES = tests/test-indenter.c
test_indenter_CFLAGS = \
	$(DEPS_CFLAGS) \
	-DGRAMMAR_FILE=\"$(top_srcdir)/tests/test.grammar\" \
	-DINPUT_FILE=\"$(top_srcdir)/tests/test.input\"
test_indenter_LDADD = libtest.la libsmie.la $(DEPS_LIBS)

EXTRA_DIST = tests/test.grammar tests/test.input

if ENABLE_GTKSOURCEVIEW
noinst_PROGRAMS = editor
editor_SOURCES = tests/editor.c editor-resources.c
editor_CFLAGS = $(DEPS_CFLAGS) $(GTKSOURCEVIEW_CFLAGS)
editor_LDADD = libsmiegtksourceview.la libsmie.la $(DEPS_LIBS) \
	$(GTKSOURCEVIEW_LIBS)

editor-resources.c: tests/editor.gresource.xml tests/editor.ui
	$(AM_V_GEN) $(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(top_srcdir)/tests --generate-source $(top_srcdir)/tests/editor.gresource.xml

EXTRA_DIST += tests/editor.gresource.xml tests/editor.ui
endif

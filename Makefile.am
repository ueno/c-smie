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

TESTS = test-grammar

check_LTLIBRARIES = libtest.la
libtest_la_SOURCES = smie-test.h smie-test.c
libtest_la_CFLAGS = $(DEPS_CFLAGS)
libtest_la_LIBADD = $(DEPS_LIBS)

check_PROGRAMS = test-grammar
test_grammar_SOURCES = test-grammar.c
test_grammar_CFLAGS = $(DEPS_CFLAGS)
test_grammar_LDADD = libtest.la libsmie.la $(DEPS_LIBS)

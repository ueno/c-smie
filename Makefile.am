ACLOCAL_AMFLAGS = -I m4
AM_YFLAGS = -d

BUILT_SOURCES = smie-gram-gen.h

lib_LTLIBRARIES = libsmie.la
libsmie_la_SOURCES =				\
	smie-grammar.h				\
	smie-grammar.c				\
	smie-gram-gen.y

libsmie_la_CFLAGS = $(DEPS_CFLAGS)
libsmie_la_LIBADD = $(DEPS_LIBS)
libsmie_la_LDFLAGS = -no-undefined -export-symbols-regex '^smie_'

check_PROGRAMS = test-grammar
test_grammar_SOURCES = test-grammar.c
test_grammar_CFLAGS = $(DEPS_CFLAGS)
test_grammar_LDADD = libsmie.la $(DEPS_LIBS)

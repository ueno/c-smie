SUBDIRS = . docs

EXTRA_DIST = smie.pc.in smie-gtksourceview.pc.in

include $(top_srcdir)/smie/Makefile.am
include $(top_srcdir)/tests/Makefile.am

GITIGNOREFILES =				\
	INSTALL					\
	aclocal.m4				\
	compile					\
	config.guess				\
	config.h.in				\
	config.sub				\
	depcomp					\
	docs/Makefile				\
	docs/Makefile.in			\
	docs/references/Makefile		\
	docs/references/Makefile.in		\
	docs/references/smie/smie-overrides.txt	\
	docs/references/smie/smie-sections.txt	\
	docs/references/smie/smie.types		\
	editor-resources.c			\
	gtk-doc.m4				\
	gtk-doc.make				\
	install-sh				\
	ltmain.sh				\
	missing					\
	smie/smie-gram-gen.c			\
	test-driver				\
	ylwrap

-include $(top_srcdir)/git.mk

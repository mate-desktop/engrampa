## Process this file with automake to produce Makefile.in.
if ENABLE_CAJA_ACTIONS
CAJA = caja
else
CAJA =
endif

SUBDIRS =		\
	copy-n-paste	\
	data		\
	src		\
	help		\
	$(CAJA)	\
	po

distcleancheck_listfiles = find . -type f -print | grep -v 'omf\.out'

distuninstallcheck_listfiles = find . -type f -print | grep -v '^\./var/scrollkeeper' | grep -v 'omf' | grep -v 'figures'


EXTRA_DIST = 			\
	AUTHORS			\
	HACKING			\
	MAINTAINERS		\
	NEWS			\
	README			\
	file-roller.spec	\
	file-roller.spec.in	\
	intltool-merge.in	\
	intltool-update.in	\
	intltool-extract.in	\
	mate-doc-utils.make

DISTCLEANFILES =                        \
	mate-doc-utils.make		\
	po/.intltool-merge-cache        \
	intltool-extract                \
	intltool-merge                  \
	intltool-update

DISTCHECK_CONFIGURE_FLAGS = --disable-scrollkeeper

MAINTAINERCLEANFILES = \
	$(srcdir)/INSTALL \
	$(srcdir)/aclocal.m4 \
	$(srcdir)/autoscan.log \
	$(srcdir)/compile \
	$(srcdir)/config.guess \
	$(srcdir)/config.h.in \
	$(srcdir)/config.sub \
	$(srcdir)/configure.scan \
	$(srcdir)/depcomp \
 	$(srcdir)/install-sh \
	$(srcdir)/ltmain.sh \
	$(srcdir)/m4 \
	$(srcdir)/missing \
	$(srcdir)/mkinstalldirs \
	$(srcdir)/omf.make \
	$(srcdir)/xmldocs.make \
	$(srcdir)/gtk-doc.make \
	$(srcdir)/ChangeLog \
	`find "$(srcdir)" -type f -name Makefile.in -print`

GITIGNOREFILES = build .cdt* .cproject .project .settings/

CLEANFILES = ChangeLog

# Build ChangeLog from GIT history
ChangeLog:
	@echo Creating $@
	@if test -d $(top_srcdir)/.git; then \
		(GIT_DIR=$(top_srcdir)/.git $(top_srcdir)/missing --run git log --stat -M -C --name-status  --date=short --no-color) | fmt --split-only > $@.tmp \
		 && mv -f $@.tmp $@ \
		 || ($(RM) $@.tmp; \
		     echo Failed to generate ChangeLog, your ChangeLog may be outdated >&2; \
		     (test -f $@ || echo git-log is required to generate this file >> $@)); \
	else \
		test -f $@ || \
		(echo A git checkout and git-log is required to generate ChangeLog >&2 && \
		echo A git checkout and git-log is required to generate this file >> $@); \
	fi

dist: ChangeLog

.PHONY: ChangeLog

-include $(top_srcdir)/git.mk

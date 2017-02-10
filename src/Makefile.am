SUBDIRS = commands sh ui

bin_PROGRAMS = engrampa
libexec_PROGRAMS = engrampa-server
noinst_PROGRAMS = test-server

if MKDTEMP_MISSING
MKDTEMP_FILES=mkdtemp.c mkdtemp.h
else
MKDTEMP_FILES=
endif

if RUN_IN_PLACE
privdatadir = $(top_srcdir)/data/
uidir = $(top_srcdir)/data/ui
privexecdir = $(abs_top_builddir)/src/commands/
shdir = $(top_srcdir)/src/sh/
else
privdatadir = $(datadir)/engrampa/
uidir = $(datadir)/engrampa/ui
privexecdir = $(libexecdir)/engrampa/
shdir = $(libexecdir)/engrampa/
endif

AM_CPPFLAGS =						\
	-I$(top_srcdir)/copy-n-paste/			\
	-I$(top_srcdir)					\
	-I$(top_builddir)				\
	-DFR_PREFIX=\"$(prefix)\"           		\
        -DFR_SYSCONFDIR=\"$(sysconfdir)\"   		\
        -DFR_DATADIR=\"$(datadir)\"     	    	\
        -DPRIVDATADIR=\"$(privdatadir)\"		\
        -DUI_DIR=\"$(uidir)\"				\
        -DFR_LIBDIR=\"$(libdir)\"  	         	\
        -DPKG_DATA_DIR=\"$(pkgdatadir)\"                \
	-DPIXMAPSDIR=\""$(datadir)/pixmaps"\"		\
	-DGLADEDIR=\""$(gladedir)"\" 			\
	-DPRIVEXECDIR=\"$(privexecdir)\"		\
	-DSHDIR=\"$(shdir)\"				\
	$(FR_CFLAGS)					\
	$(JSON_GLIB_CFLAGS)

BUILT_SOURCES =			\
	fr-marshal.c		\
	fr-marshal.h		\
	fr-enum-types.h		\
	fr-enum-types.c		\
	fr-resources.c		\
	fr-resources.h		\
	$(NULL)

COMMON_SOURCES = 			\
	actions.h			\
	actions.c			\
	dlg-add-files.c			\
	dlg-add-files.h			\
	dlg-add-folder.c		\
	dlg-add-folder.h		\
	dlg-ask-password.c		\
	dlg-ask-password.h		\
	dlg-batch-add.c			\
	dlg-batch-add.h			\
	dlg-delete.c			\
	dlg-delete.h			\
	dlg-extract.c			\
	dlg-extract.h			\
	dlg-new.c			\
	dlg-new.h			\
	dlg-open-with.c			\
	dlg-open-with.h			\
	dlg-package-installer.c		\
	dlg-package-installer.h		\
	dlg-password.c			\
	dlg-password.h			\
	dlg-prop.c			\
	dlg-prop.h			\
	dlg-update.c			\
	dlg-update.h			\
	eggfileformatchooser.c		\
	eggfileformatchooser.h		\
	egg-macros.h			\
	eggtreemultidnd.c		\
	eggtreemultidnd.h		\
	file-data.c			\
	file-data.h			\
	file-utils.c			\
	file-utils.h			\
	fr-archive.c			\
	fr-archive.h			\
	fr-command.c			\
	fr-command.h			\
	fr-command-ace.c		\
	fr-command-ace.h		\
	fr-command-alz.c		\
	fr-command-alz.h		\
	fr-command-ar.c			\
	fr-command-ar.h			\
	fr-command-arj.c		\
	fr-command-arj.h		\
	fr-command-cfile.c		\
	fr-command-cfile.h		\
	fr-command-cpio.c		\
	fr-command-cpio.h		\
	fr-command-dpkg.c		\
	fr-command-dpkg.h		\
	fr-command-iso.c		\
	fr-command-iso.h		\
	fr-command-jar.h		\
	fr-command-jar.c		\
	fr-command-lha.c		\
	fr-command-lha.h		\
	fr-command-rar.c		\
	fr-command-rar.h		\
	fr-command-rpm.c		\
	fr-command-rpm.h		\
	fr-command-tar.c		\
	fr-command-tar.h		\
	fr-command-unstuff.c		\
	fr-command-unstuff.h		\
	fr-command-zip.c		\
	fr-command-zip.h		\
	fr-command-lrzip.c		\
	fr-command-lrzip.h		\
	fr-command-zoo.c		\
	fr-command-zoo.h		\
	fr-command-7z.c			\
	fr-command-7z.h			\
	fr-error.c			\
	fr-error.h			\
	fr-init.c			\
	fr-init.h			\
	fr-list-model.c			\
	fr-list-model.h			\
	fr-stock.c			\
	fr-stock.h			\
	fr-proc-error.c			\
	fr-proc-error.h			\
	fr-process.c			\
	fr-process.h			\
	fr-window.c			\
	fr-window.h			\
	gio-utils.c			\
	gio-utils.h			\
	glib-utils.c			\
	glib-utils.h			\
	gtk-utils.c			\
	gtk-utils.h			\
	java-utils.c			\
	java-utils.h			\
	open-file.c			\
	open-file.h			\
	preferences.c			\
	preferences.h			\
	rar-utils.c			\
	rar-utils.h			\
	typedefs.h			\
	ui.h				\
	$(MKDTEMP_FILES)		\
	$(BUILT_SOURCES)

if ENABLE_JSON_GLIB
COMMON_SOURCES +=			\
	fr-command-unarchiver.h		\
	fr-command-unarchiver.c
endif

fr-marshal.h: fr-marshal.list $(GLIB_GENMARSHAL)
	$(AM_V_GEN)( $(GLIB_GENMARSHAL) $< --header --prefix=fr_marshal > $@ )

fr-marshal.c: fr-marshal.list $(GLIB_GENMARSHAL)
	$(AM_V_GEN)( echo "#include \"fr-marshal.h\"" > $@ && \
	$(GLIB_GENMARSHAL) $< --body --prefix=fr_marshal >> $@ )

fr-enum-types.h: typedefs.h $(GLIB_MKENUMS)
	$(AM_V_GEN)( $(GLIB_MKENUMS) \
		--fhead "#ifndef FR_ENUM_TYPES_H\n#define FR_ENUM_TYPES_H\n\n#include <glib-object.h>\n\nG_BEGIN_DECLS\n" \
		--fprod "/* enumerations from \"@filename@\" */\n" \
		--vhead "GType @enum_name@_get_type (void);\n#define FR_TYPE_@ENUMSHORT@ (@enum_name@_get_type())\n" \
		--ftail "G_END_DECLS\n\n#endif /* FR_ENUM_TYPES_H */" \
		$^> xgen-$(@F) \
	&& (cmp -s xgen-$(@F) fr-enum-types.h || cp xgen-$(@F) fr-enum-types.h ) \
	&& rm -f xgen-$(@F) )

fr-enum-types.c: typedefs.h fr-enum-types.h
	$(AM_V_GEN)( $(GLIB_MKENUMS) \
		--fhead "#include <glib-object.h>\n" \
		--fprod "\n/* enumerations from \"@filename@\" */\n#include \"@filename@\"" \
		--vhead "GType\n@enum_name@_get_type (void)\n{\n  static GType etype = 0;\n  if (etype == 0) {\n    static const G@Type@Value values[] = {" \
		--vprod "      { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
		--vtail "      { 0, NULL, NULL }\n    };\n    etype = g_@type@_register_static (\"@EnumName@\", values);\n  }\n  return etype;\n}\n" \
		$^> xgen-$(@F) \
	&& (cmp -s xgen-$(@F) fr-enum-types.c || cp xgen-$(@F) fr-enum-types.c ) \
	&& rm -f xgen-$(@F) )

fr-resources.c: engrampa.gresource.xml $(shell $(GLIB_COMPILE_RESOURCES) --generate-dependencies engrampa.gresource.xml)
	$(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir) --generate --c-name fr $(srcdir)/engrampa.gresource.xml

fr-resources.h: engrampa.gresource.xml $(shell $(GLIB_COMPILE_RESOURCES) --generate-dependencies engrampa.gresource.xml)
	$(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir) --generate --c-name fr $(srcdir)/engrampa.gresource.xml

engrampa_SOURCES = 			\
	$(COMMON_SOURCES)		\
	main.c

engrampa_LDADD =					\
	$(top_builddir)/copy-n-paste/libeggsmclient.la	\
	$(FR_LIBS)					\
	${JSON_GLIB_LIBS}

engrampa_server_SOURCES =		\
	$(COMMON_SOURCES)		\
	server.c

engrampa_server_LDADD =		\
	$(engrampa_LDADD)

test_server_SOURCES = test-server.c
test_server_FLAGS = $(FR_CFLAGS)
test_server_LDADD = $(FR_LIBS)

if ENABLE_MAGIC
engrampa_LDADD += $(MAGIC_LIBS)
endif

EXTRA_DIST = 				\
	engrampa.gresource.xml		\
	fr-marshal.list

CLEANFILES = $(BUILT_SOURCES)

dist-hook:
	cd $(distdir); rm -f $(CLEANFILES)

-include $(top_srcdir)/git.mk

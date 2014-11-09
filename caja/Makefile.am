AM_CPPFLAGS =						\
	-DFR_DATADIR=\"$(datadir)\"			\
	-DMATELOCALEDIR=\""$(datadir)/locale"\" 	\
	-I$(top_srcdir)					\
	-I$(top_builddir)				\
	$(DISABLE_DEPRECATED)				\
	$(CAJA_CFLAGS)				\
	$(FR_CFLAGS)					\
	$(JSON_GLIB_CFLAGS)

caja_extensiondir=$(CAJA_EXTENSION_DIR)

caja_extension_LTLIBRARIES=libcaja-engrampa.la

libcaja_engrampa_la_SOURCES =     \
	caja-engrampa.c           \
	caja-engrampa.h           \
	engrampa-module.c

libcaja_engrampa_la_LDFLAGS = -module -avoid-version -no-undefined
libcaja_engrampa_la_LIBADD  = $(CAJA_LIBS)

extensiondir = $(datadir)/caja/extensions
extension_in_files = libcaja-engrampa.caja-extension.in
extension_DATA = $(extension_in_files:.caja-extension.in=.caja-extension)
%.caja-extension: %.caja-extension.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*.po) ; $(AM_V_GEN) LC_ALL=C $(INTLTOOL_MERGE) -d -u -c $(top_builddir)/po/.intltool-merge-cache $(top_srcdir)/po $< $@

CLEANFILES = libcaja-engrampa.caja-extension

EXTRA_DIST = $(extension_in_files)
-include $(top_srcdir)/git.mk

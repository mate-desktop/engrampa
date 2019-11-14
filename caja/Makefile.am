AM_CPPFLAGS =						\
	$(WARN_CFLAGS)					\
	-DFR_DATADIR=\"$(datadir)\"			\
	-DMATELOCALEDIR=\""$(datadir)/locale"\" 	\
	-I$(top_srcdir)					\
	-I$(top_builddir)				\
	$(CAJA_CFLAGS)

caja_extensiondir=$(CAJA_EXTENSION_DIR)

caja_extension_LTLIBRARIES=libcaja-engrampa.la

libcaja_engrampa_la_SOURCES =     \
	caja-engrampa.c           \
	caja-engrampa.h           \
	engrampa-module.c

libcaja_engrampa_la_LDFLAGS = -module -avoid-version -no-undefined
libcaja_engrampa_la_LIBADD  = $(CAJA_LIBS)

extensiondir = $(datadir)/caja/extensions
extension_in_files = libcaja-engrampa.caja-extension.desktop.in
extension_DATA = $(extension_in_files:.caja-extension.desktop.in=.caja-extension)
$(extension_DATA): $(extension_in_files)
	$(AM_V_GEN) $(MSGFMT) --desktop --keyword=Name --keyword=Description --template $< -d $(top_srcdir)/po -o $@

CLEANFILES = $(extension_DATA)

EXTRA_DIST = $(extension_in_files)
-include $(top_srcdir)/git.mk

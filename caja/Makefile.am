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
extension_in_in_files = libcaja-engrampa.caja-extension.desktop.in.in
extension_in_files = $(extension_in_in_files:.caja-extension.desktop.in.in=.caja-extension.desktop.in)
extension_DATA = $(extension_in_files:.caja-extension.desktop.in=.caja-extension)
$(extension_DATA): $(extension_in_files)
if USE_NLS
	$(AM_V_GEN) $(MSGFMT) --desktop --keyword= --keyword=Copyright --keyword=Name --keyword=Description --template $< -d $(top_srcdir)/po -o $@
else
	$(AM_V_GEN) cp -f $< $@
endif

DISTCLEANFILES = $(extension_in_files)
CLEANFILES = $(extension_DATA)

EXTRA_DIST = $(extension_in_in_files)

-include $(top_srcdir)/git.mk

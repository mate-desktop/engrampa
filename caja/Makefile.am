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

-include $(top_srcdir)/git.mk

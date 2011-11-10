INCLUDES =						\
	-DFR_DATADIR=\"$(datadir)\"			\
	-DMATELOCALEDIR=\""$(datadir)/locale"\" 	\
	-I$(top_srcdir)					\
	-I$(top_builddir)				\
	$(DISABLE_DEPRECATED)				\
	$(CAJA_CFLAGS)				\
	$(FR_CFLAGS)

caja_extensiondir=$(CAJA_EXTENSION_DIR)

caja_extension_LTLIBRARIES=libcaja-fileroller.la

libcaja_fileroller_la_SOURCES =     \
	caja-fileroller.c           \
	caja-fileroller.h           \
	fileroller-module.c

libcaja_fileroller_la_LDFLAGS = -module -avoid-version -no-undefined
libcaja_fileroller_la_LIBADD  = $(CAJA_LIBS)

-include $(top_srcdir)/git.mk

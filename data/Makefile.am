@INTLTOOL_DESKTOP_RULE@

SUBDIRS = ui icons

desktop_in_in_files = file-roller.desktop.in.in
desktop_in_files = $(desktop_in_in_files:.desktop.in.in=.desktop.in) 

desktopdir = $(datadir)/applications
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop) 

matchdir = $(datadir)/file-roller
match_DATA = packages.match

schemadir = @MATECONF_SCHEMA_FILE_DIR@
schema_DATA = file-roller.schemas

gtk_update_icon_cache = gtk-update-icon-cache -f -t $(datadir)/icons/hicolor

install-data-local:
if MATECONF_SCHEMAS_INSTALL
	MATECONF_CONFIG_SOURCE=$(MATECONF_SCHEMA_CONFIG_SOURCE) $(MATECONFTOOL) --makefile-install-rule $(srcdir)/$(schema_DATA)
endif

install-data-hook: update-icon-cache
uninstall-hook: update-icon-cache
update-icon-cache:
	if test -z "$(DESTDIR)"; then \
		echo "Updating Gtk icon cache."; \
		$(gtk_update_icon_cache); \
	else \
		echo "*** Icon cache not updated.  After (un)install, run this:"; \
		echo "***   $(gtk_update_icon_cache)"; \
	fi


EXTRA_DIST = 			\
	$(desktop_in_in_files)	\
	$(desktop_in_files)	\
	$(desktop_DATA)		\
	packages.match.in	\
	packages.match		\
	$(schema_DATA)


DISTCLEANFILES =                \
	$(desktop_in_files)	\
	$(desktop_DATA)	

-include $(top_srcdir)/git.mk

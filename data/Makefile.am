SUBDIRS = ui icons

desktopdir = $(datadir)/applications
desktop_in_in_files = engrampa.desktop.in.in
desktop_in_files = $(desktop_in_in_files:.desktop.in.in=.desktop.in) 
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

@INTLTOOL_DESKTOP_RULE@

matchdir = $(datadir)/engrampa
match_DATA = packages.match

gsettingsschema_in_files = org.mate.engrampa.gschema.xml.in
gsettings_SCHEMAS = $(gsettingsschema_in_files:.xml.in=.xml)
.PRECIOUS: $(gsettings_SCHEMAS)

@INTLTOOL_XML_NOMERGE_RULE@

@GSETTINGS_RULES@

convertdir = $(datadir)/MateConf/gsettings
convert_DATA = engrampa.convert

gtk_update_icon_cache = gtk-update-icon-cache -f -t $(datadir)/icons/hicolor

install-data-hook: update-cache
uninstall-hook: update-cache
update-cache:
	if test -z "$(DESTDIR)"; then \
		echo "Updating Gtk icon cache."; \
		$(gtk_update_icon_cache); \
	else \
		echo "*** Icon cache not updated.  After (un)install, run this:"; \
		echo "***   $(gtk_update_icon_cache)"; \
	fi

EXTRA_DIST = \
       engrampa.convert \
       packages.match.in \
       packages.match \
       $(desktop_in_in_files) \
       $(desktop_in_files) \
       $(desktop_DATA) \
       $(gsettingsschema_in_files) \
       $(NULL)

DISTCLEANFILES = \
       $(desktop_in_files) \
       $(desktop_DATA) \
       $(NULL)

CLEANFILES = \
       $(gsettings_SCHEMAS) \
       $(NULL)

dist-hook: 
	cd $(distdir); rm -f $(CLEANFILES)

-include $(top_srcdir)/git.mk

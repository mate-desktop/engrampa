SUBDIRS = icons

desktopdir = $(datadir)/applications
desktop_in_in_files = engrampa.desktop.in.in
desktop_in_files = $(desktop_in_in_files:.desktop.in.in=.desktop.in)
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
$(desktop_DATA): $(desktop_in_files)
if USE_NLS
	$(AM_V_GEN) $(MSGFMT) --desktop --template $< -d $(top_srcdir)/po -o $@
else
	$(AM_V_GEN) sed '/^# Translators/d' < $< > $@
endif

if ENABLE_PACKAGEKIT
matchdir = $(datadir)/engrampa
match_DATA = packages.match
endif

servicedir = $(datadir)/dbus-1/services
service_in_files = org.mate.Engrampa.service.in
service_DATA = $(service_in_files:.service.in=.service)
$(service_DATA): $(service_in_files) Makefile
	$(AM_V_GEN) $(SED) -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

appdatadir = $(datadir)/metainfo
appdata_in_in_files = engrampa.appdata.xml.in.in
appdata_in_files = $(appdata_in_in_files:.xml.in.in=.xml.in)
appdata_DATA = $(appdata_in_files:.xml.in=.xml)
$(appdata_DATA): $(appdata_in_files)
if USE_NLS
	$(AM_V_GEN) $(MSGFMT) --xml --template $< -d $(top_srcdir)/po -o $@
else
	$(AM_V_GEN) cp -f $< $@
endif

gsettingsschema_in_files = org.mate.engrampa.gschema.xml.in
gsettings_SCHEMAS = $(gsettingsschema_in_files:.xml.in=.xml)
.PRECIOUS: $(gsettings_SCHEMAS)

@GSETTINGS_RULES@

man_MANS = engrampa.1

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
	packages.match.in \
	packages.match \
	$(appdata_in_in_files) \
	$(desktop_in_in_files) \
	$(gsettingsschema_in_files) \
	$(man_MANS) \
	$(service_in_files) \
	$(NULL)

DISTCLEANFILES = \
	$(appdata_in_files) \
	$(desktop_in_files) \
	$(service_DATA) \
	$(NULL)

CLEANFILES = \
	$(appdata_DATA) \
	$(desktop_DATA) \
	$(gsettings_SCHEMAS) \
	$(NULL)

dist-hook:
	cd $(distdir); rm -f $(CLEANFILES)

-include $(top_srcdir)/git.mk

iconsdir = $(datadir)/icons/hicolor

nobase_dist_icons_DATA = \
	16x16/actions/add-files-to-archive.png \
	16x16/actions/add-folder-to-archive.png \
	16x16/actions/extract-archive.png \
	16x16/apps/engrampa.png \
	22x22/apps/engrampa.png \
	24x24/actions/add-files-to-archive.png \
	24x24/actions/add-folder-to-archive.png \
	24x24/actions/extract-archive.png \
	24x24/apps/engrampa.png \
	32x32/apps/engrampa.png \
	scalable/apps/engrampa.svg

gtk_update_icon_cache = gtk-update-icon-cache -f -t $(iconsdir)

install-data-hook: update-icon-cache
uninstall-hook: update-icon-cache
update-icon-cache:
	@-if test -z "$(DESTDIR)"; then \
		echo "Updating Gtk icon cache."; \
		$(gtk_update_icon_cache); \
	else \
		echo "*** Icon cache not updated.  After install, run this:"; \
		echo "***   $(gtk_update_icon_cache)"; \
	fi

-include $(top_srcdir)/git.mk

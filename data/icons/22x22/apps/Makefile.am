themedir = $(datadir)/icons/hicolor
size = 22x22
context = apps

iconsdir = $(themedir)/$(size)/$(context)

icons_DATA = \
	file-roller.png

noinst_DATA =

EXTRA_DIST =		\
	$(icons_DATA)	\
	$(noinst_DATA)

gtk_update_icon_cache = gtk-update-icon-cache -f -t $(datadir)/icons/hicolor

install-data-hook:
	@-if test -z "$(DESTDIR)"; then \
		echo "Updating Gtk icon cache."; \
		$(gtk_update_icon_cache); \
	else \
		echo "*** Icon cache not updated.  After install, run this:"; \
		echo "***   $(gtk_update_icon_cache)"; \
	fi
-include $(top_srcdir)/git.mk

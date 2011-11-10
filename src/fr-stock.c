/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  File Roller
 *
 *  Copyright (C) 2003 Free Software Foundation, Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307, USA.
 */

#include <config.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include "fr-stock.h"


static gboolean stock_initialized = FALSE;

static const  struct {
        char *stock_id;
        char *icon;
} stock_icons [] = {
	{ FR_STOCK_CREATE_ARCHIVE, "add-files-to-archive" },
        { FR_STOCK_ADD_FILES, "add-files-to-archive" },
        { FR_STOCK_ADD_FOLDER, "add-folder-to-archive" },
        { FR_STOCK_EXTRACT, "extract-archive" }
};

static const GtkStockItem stock_items [] = {
	{ FR_STOCK_CREATE_ARCHIVE, N_("C_reate"), 0, 0, GETTEXT_PACKAGE },
	{ FR_STOCK_ADD_FILES, N_("_Add"), 0, 0, GETTEXT_PACKAGE },
	{ FR_STOCK_ADD_FOLDER, N_("_Add"), 0, 0, GETTEXT_PACKAGE },	
	{ FR_STOCK_EXTRACT, N_("_Extract"), 0, 0, GETTEXT_PACKAGE }
};

void
fr_stock_init (void)
{        
	GtkIconFactory *factory;
        GtkIconSource  *source;
        int             i;
	
	if (stock_initialized)
		return;
	stock_initialized = TRUE;

	gtk_stock_add_static (stock_items, G_N_ELEMENTS (stock_items));

        factory = gtk_icon_factory_new ();
        gtk_icon_factory_add_default (factory);

        source = gtk_icon_source_new ();

        for (i = 0; i < G_N_ELEMENTS (stock_icons); i++) {
                GtkIconSet *set;

                gtk_icon_source_set_icon_name (source, stock_icons [i].icon);

                set = gtk_icon_set_new ();
                gtk_icon_set_add_source (set, source);

                gtk_icon_factory_add (factory, stock_icons [i].stock_id, set);
                gtk_icon_set_unref (set);
        }

        gtk_icon_source_free (source);

        g_object_unref (factory);
}

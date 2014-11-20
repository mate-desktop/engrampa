/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  Engrampa
 *
 *  Copyright (C) 2001, 2003 Free Software Foundation, Inc.
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
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02110-1301, USA.
 */

#include <string.h>
#include "typedefs.h"
#include "preferences.h"
#include "fr-init.h"
#include "file-utils.h"
#include "fr-window.h"


void
pref_util_save_window_geometry (GtkWindow  *window,
				const char *dialog_id)
{
	char *schema;
	GSettings *settings;
	int width;
	int height;

	schema = g_strconcat (ENGRAMPA_SCHEMA_DIALOGS, ".", dialog_id, NULL);
	settings = g_settings_new (schema);

	gtk_window_get_size (window, &width, &height);
	g_settings_set_int (settings, "width", width);
	g_settings_set_int (settings, "height", height);
	g_object_unref (settings);
	g_free (schema);
}


void
pref_util_restore_window_geometry (GtkWindow  *window,
				   const char *dialog_id)
{
	char *schema;
	GSettings *settings;
	int width;
	int height;

	schema = g_strconcat (ENGRAMPA_SCHEMA_DIALOGS, ".", dialog_id, NULL);
	settings = g_settings_new (schema);

	width = g_settings_get_int (settings, "width");
	height = g_settings_get_int (settings, "height");
	if ((width != -1) && (height != 1))
		gtk_window_set_default_size (window, width, height);

	gtk_window_present (window);

	g_object_unref (settings);
	g_free (schema);
}

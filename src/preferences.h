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

#ifndef PREFERENCES_H
#define PREFERENCES_H

#include <gtk/gtk.h>
#include "typedefs.h"
#include "fr-window.h"

#define PREF_LIST_SORT_METHOD      "/apps/engrampa/listing/sort_method"
#define PREF_LIST_SORT_TYPE        "/apps/engrampa/listing/sort_type"
#define PREF_LIST_MODE             "/apps/engrampa/listing/list_mode"
#define PREF_LIST_SHOW_TYPE        "/apps/engrampa/listing/show_type"
#define PREF_LIST_SHOW_SIZE        "/apps/engrampa/listing/show_size"
#define PREF_LIST_SHOW_TIME        "/apps/engrampa/listing/show_time"
#define PREF_LIST_SHOW_PATH        "/apps/engrampa/listing/show_path"
#define PREF_LIST_USE_MIME_ICONS   "/apps/engrampa/listing/use_mime_icons"
#define PREF_NAME_COLUMN_WIDTH     "/apps/engrampa/listing/name_column_width"

#define PREF_UI_WINDOW_WIDTH       "/apps/engrampa/ui/window_width"
#define PREF_UI_WINDOW_HEIGHT      "/apps/engrampa/ui/window_height"
#define PREF_UI_SIDEBAR_WIDTH      "/apps/engrampa/ui/sidebar_width"
#define PREF_UI_HISTORY_LEN        "/apps/engrampa/ui/history_len"
#define PREF_UI_TOOLBAR            "/apps/engrampa/ui/view_toolbar"
#define PREF_UI_STATUSBAR          "/apps/engrampa/ui/view_statusbar"
#define PREF_UI_FOLDERS            "/apps/engrampa/ui/view_folders"

#define PREF_EDIT_EDITORS          "/apps/engrampa/general/editors"
#define PREF_ADD_COMPRESSION_LEVEL "/apps/engrampa/general/compression_level"
#define PREF_ENCRYPT_HEADER        "/apps/engrampa/general/encrypt_header"
#define PREF_MIGRATE_DIRECTORIES   "/apps/engrampa/general/migrate_directories"

#define PREF_EXTRACT_OVERWRITE        "/apps/engrampa/dialogs/extract/overwrite"
#define PREF_EXTRACT_SKIP_NEWER       "/apps/engrampa/dialogs/extract/skip_newer"
#define PREF_EXTRACT_RECREATE_FOLDERS "/apps/engrampa/dialogs/extract/recreate_folders"

#define PREF_ADD_CURRENT_FOLDER       "/apps/engrampa/dialogs/add/current_folder"
#define PREF_ADD_FILENAME             "/apps/engrampa/dialogs/add/filename"
#define PREF_ADD_INCLUDE_FILES        "/apps/engrampa/dialogs/add/include_files"
#define PREF_ADD_EXCLUDE_FILES        "/apps/engrampa/dialogs/add/exclude_files"
#define PREF_ADD_EXCLUDE_FOLDERS      "/apps/engrampa/dialogs/add/exclude_folders"
#define PREF_ADD_UPDATE               "/apps/engrampa/dialogs/add/update"
#define PREF_ADD_RECURSIVE            "/apps/engrampa/dialogs/add/recursive"
#define PREF_ADD_NO_SYMLINKS          "/apps/engrampa/dialogs/add/no_symlinks"

#define PREF_BATCH_ADD_DEFAULT_EXTENSION "/apps/engrampa/dialogs/batch-add/default_extension"
#define PREF_BATCH_OTHER_OPTIONS         "/apps/engrampa/dialogs/batch-add/other_options"
#define PREF_BATCH_VOLUME_SIZE           "/apps/engrampa/dialogs/batch-add/volume_size"

#define PREF_DESKTOP_ICON_THEME         "/desktop/mate/file_views/icon_theme"
#define PREF_DESKTOP_MENUS_HAVE_TEAROFF "/desktop/mate/interface/menus_have_tearoff"
#define PREF_DESKTOP_MENUBAR_DETACHABLE "/desktop/mate/interface/menubar_detachable"
#define PREF_DESKTOP_TOOLBAR_DETACHABLE "/desktop/mate/interface/toolbar_detachable"
#define PREF_CAJA_CLICK_POLICY "/apps/caja/preferences/click_policy"

FrWindowSortMethod  preferences_get_sort_method       (void);
void                preferences_set_sort_method       (FrWindowSortMethod  i_value);
GtkSortType         preferences_get_sort_type         (void);
void                preferences_set_sort_type         (GtkSortType         i_value);
FrWindowListMode    preferences_get_list_mode         (void);
void                preferences_set_list_mode         (FrWindowListMode    i_value);
FrCompression       preferences_get_compression_level (void);
void                preferences_set_compression_level (FrCompression       i_value);
void                pref_util_save_window_geometry    (GtkWindow          *window,
						       const char         *dialog);
void                pref_util_restore_window_geometry (GtkWindow          *window,
						       const char         *dialog);

#endif /* PREFERENCES_H */

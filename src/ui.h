/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  Engrampa
 *
 *  Copyright (C) 2004 Free Software Foundation, Inc.
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef UI_H
#define UI_H

#include "actions.h"

static GActionEntry action_entries[] = {
	/* FileMenu */
	{ "New", activate_action_new, NULL, NULL, NULL, { 0 } },
	{ "Open", activate_action_open, NULL, NULL, NULL, { 0 } },
	{ "SaveAs", activate_action_save_as, NULL, NULL, NULL, { 0 } },
	{ "Extract", activate_action_extract, NULL, NULL, NULL, { 0 } },
	{ "TestArchive", activate_action_test_archive, NULL, NULL, NULL, { 0 } },
	{ "Properties",  activate_action_properties, NULL, NULL, NULL, { 0 } },
	{ "Close", activate_action_close, NULL, NULL, NULL, { 0 } },
	/* EditMenu */
	{ "Copy", activate_action_copy, NULL, NULL, NULL, { 0 } },
	{ "Cut", activate_action_cut, NULL, NULL, NULL, { 0 } },
	{ "Paste", activate_action_paste, NULL, NULL, NULL, { 0 } },
	{ "Rename", activate_action_rename, NULL, NULL, NULL, { 0 } },
	{ "Delete", activate_action_delete, NULL, NULL, NULL, { 0 } },
	{ "SelectAll", activate_action_select_all, NULL, NULL, NULL, { 0 } },
	{ "DeselectAll", activate_action_deselect_all, NULL, NULL, NULL, { 0 } },
	{ "Find", activate_action_find, NULL, NULL, NULL, { 0 } },
	{ "AddFiles", activate_action_add_files, NULL, NULL, NULL, { 0 } },
	{ "AddFolder", activate_action_add_folder, NULL, NULL, NULL, { 0 } },
	{ "Password", activate_action_password, NULL, NULL, NULL, { 0 } },
	/* ViewMenu */
	{ "ViewToolbar", activate_toggle, NULL, "true", activate_action_view_toolbar, { 0 } },
	{ "ViewStatusbar", activate_toggle, NULL, "true", activate_action_view_statusbar, { 0 } },
	{ "ViewFolders",  activate_toggle, NULL, "false", activate_action_view_folders, { 0 } },
	{ "LastOutput", activate_action_last_output, NULL, NULL, NULL, { 0 } },
	{ "Stop", activate_action_stop, NULL, NULL, NULL, { 0 } },
	{ "Reload", activate_action_reload, NULL, NULL, NULL, { 0 } },
	/* HelpMenu */
	{ "About", activate_action_about, NULL, NULL, NULL, { 0 } },
	{ "Contents", activate_action_manual, NULL, NULL, NULL, { 0 } },
	/* Toolbar */
	{ "OpenRecent_Toolbar", activate_action_open, NULL, NULL, NULL, { 0 } },
	{ "ExtractToolbar", activate_action_extract, NULL, NULL, NULL, { 0 } },
	{ "AddFilesToolbar", activate_action_add_files, NULL, NULL, NULL, { 0 } },
	{ "AddFolderToolbar", activate_action_add_folder, NULL, NULL, NULL, { 0 } },
	/* LocationBar*/
	{ "GoBack", activate_action_go_back, NULL, NULL, NULL, { 0 } },
	{ "GoForward", activate_action_go_forward, NULL, NULL, NULL, { 0 } },
	{ "GoUp", activate_action_go_up, NULL, NULL, NULL, { 0 } },
	{ "GoHome", activate_action_go_home, NULL, NULL, NULL, { 0 } },
};

static const struct {
  guint keyval;
  GdkModifierType modifier;
  const gchar *widget_id;
} menu_keybindings [] = {
  { GDK_KEY_N,      GDK_CONTROL_MASK, "new_item"},
  { GDK_KEY_O,      GDK_CONTROL_MASK, "open_item"},
  { GDK_KEY_O,      GDK_CONTROL_MASK, "open_recent_item"},
  { GDK_KEY_E,      GDK_CONTROL_MASK, "extract_item"},
  { GDK_KEY_Return, GDK_MOD1_MASK,    "properties_item"},
  { GDK_KEY_W,      GDK_CONTROL_MASK, "close_item"},
  { GDK_KEY_X,      GDK_CONTROL_MASK, "cut_item"},
  { GDK_KEY_C,      GDK_CONTROL_MASK, "copy_item"},
  { GDK_KEY_V,      GDK_CONTROL_MASK, "paste_item"},
  { GDK_KEY_F2,     0,                "rename_item"},
  { GDK_KEY_Delete, 0,                "delete_item"},
  { GDK_KEY_A,      GDK_CONTROL_MASK, "select_all_item"},
  { GDK_KEY_A,      GDK_SHIFT_MASK | GDK_CONTROL_MASK, "deselect_all_item"},
  { GDK_KEY_F,      GDK_CONTROL_MASK, "find_item"},
  { GDK_KEY_F9,     0,                "view_folder_item"},
  { GDK_KEY_1,      GDK_CONTROL_MASK, "view_all_files_item"},
  { GDK_KEY_2,      GDK_CONTROL_MASK, "view_as_folder_item"},
  { GDK_KEY_Escape, 0,                "stop_item"},
  { GDK_KEY_R,      GDK_CONTROL_MASK, "reload_item"},
  { GDK_KEY_F1,     0,                "help_item"},
  { GDK_KEY_O,      GDK_CONTROL_MASK, "popup_open_item2"},
  { GDK_KEY_X,      GDK_CONTROL_MASK, "popup_cut_item2"},
  { GDK_KEY_C,      GDK_CONTROL_MASK, "popup_copy_item2"},
  { GDK_KEY_V,      GDK_CONTROL_MASK, "popup_paste_item2"},
  { GDK_KEY_F2,     0,                "popup_rename_item2"},
  { GDK_KEY_Delete, 0,                "popup_delete_item2"},
};

#endif /* UI_H */

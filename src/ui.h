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
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02110-1301, USA.
 */

#ifndef UI_H
#define UI_H


#include "actions.h"
#include "fr-stock.h"


static GtkActionEntry action_entries[] = {
	{ "FileMenu", NULL, N_("_Archive") },
	{ "EditMenu", NULL, N_("_Edit") },
	{ "ViewMenu", NULL, N_("_View") },
	{ "HelpMenu", NULL, N_("_Help") },
	{ "ArrangeFilesMenu", NULL, N_("_Arrange Files") },

	{ "About", "help-about",
	  N_("_About"), NULL,
	  N_("Information about the program"),
	  G_CALLBACK (activate_action_about) },
	{ "AddFiles", FR_STOCK_ADD_FILES,
	  N_("_Add Files…"), NULL,
	  N_("Add files to the archive"),
	  G_CALLBACK (activate_action_add_files) },
	{ "AddFiles_Toolbar", FR_STOCK_ADD_FILES,
	  N_("Add Files"), NULL,
	  N_("Add files to the archive"),
	  G_CALLBACK (activate_action_add_files) },
	{ "AddFolder", FR_STOCK_ADD_FOLDER,
	  N_("Add a _Folder…"), NULL,
	  N_("Add a folder to the archive"),
	  G_CALLBACK (activate_action_add_folder) },
	{ "AddFolder_Toolbar", FR_STOCK_ADD_FOLDER,
	  N_("Add Folder"), NULL,
	  N_("Add a folder to the archive"),
	  G_CALLBACK (activate_action_add_folder) },
	{ "Close", "window-close",
	  N_("_Close"), "<control>W",
	  N_("Close the current archive"),
	  G_CALLBACK (activate_action_close) },
	{ "Contents", "help-browser",
	  N_("Contents"), "F1",
	  N_("Display the Engrampa Manual"),
	  G_CALLBACK (activate_action_manual) },

	{ "Copy", "edit-copy",
	  N_("_Copy"), "<control>C",
	  N_("Copy the selection"),
	  G_CALLBACK (activate_action_copy) },
	{ "Cut", "edit-cut",
	  N_("Cu_t"), "<control>X",
	  N_("Cut the selection"),
	  G_CALLBACK (activate_action_cut) },
	{ "Paste", "edit-paste",
	  N_("_Paste"), "<control>V",
	  N_("Paste the clipboard"),
	  G_CALLBACK (activate_action_paste) },
	{ "Rename", NULL,
	  N_("_Rename…"), "F2",
	  N_("Rename the selection"),
	  G_CALLBACK (activate_action_rename) },
	{ "Delete", "edit-delete",
	  N_("_Delete"), "Delete",
	  N_("Delete the selection from the archive"),
	  G_CALLBACK (activate_action_delete) },

	{ "CopyFolderFromSidebar", "edit-copy",
	  N_("_Copy"), "<control>C",
	  N_("Copy the selection"),
	  G_CALLBACK (activate_action_copy_folder_from_sidebar) },
	{ "CutFolderFromSidebar", "edit-cut",
	  N_("Cu_t"), "<control>X",
	  N_("Cut the selection"),
	  G_CALLBACK (activate_action_cut_folder_from_sidebar) },
	{ "PasteFolderFromSidebar", "edit-paste",
	  N_("_Paste"), "<control>V",
	  N_("Paste the clipboard"),
	  G_CALLBACK (activate_action_paste_folder_from_sidebar) },
	{ "RenameFolderFromSidebar", NULL,
	  N_("_Rename…"), "F2",
	  N_("Rename the selection"),
	  G_CALLBACK (activate_action_rename_folder_from_sidebar) },
	{ "DeleteFolderFromSidebar", "edit-delete",
	  N_("_Delete"), "Delete",
	  N_("Delete the selection from the archive"),
	  G_CALLBACK (activate_action_delete_folder_from_sidebar) },

	{ "DeselectAll", NULL,
	  N_("Dese_lect All"), "<shift><control>A",
	  N_("Deselect all files"),
	  G_CALLBACK (activate_action_deselect_all) },
	{ "Extract", FR_STOCK_EXTRACT,
	  N_("_Extract…"), "<control>E",
	  N_("Extract files from the archive"),
	  G_CALLBACK (activate_action_extract) },
	{ "ExtractFolderFromSidebar", FR_STOCK_EXTRACT,
	  N_("_Extract…"), NULL,
	  N_("Extract files from the archive"),
	  G_CALLBACK (activate_action_extract_folder_from_sidebar) },
	{ "Extract_Toolbar", FR_STOCK_EXTRACT,
	  N_("Extract"), NULL,
	  N_("Extract files from the archive"),
	  G_CALLBACK (activate_action_extract) },
	{ "Find", "edit-find",
	  N_("Find…"), "<control>F",
	  NULL,
	  G_CALLBACK (activate_action_find) },

	{ "LastOutput", NULL,
	  N_("_Last Output"), NULL,
	  N_("View the output produced by the last executed command"),
	  G_CALLBACK (activate_action_last_output) },
	{ "New", "document-new",
	  N_("New…"), "<control>N",
	  N_("Create a new archive"),
	  G_CALLBACK (activate_action_new) },
	{ "Open", "document-open",
	  N_("Open…"), "<control>O",
	  N_("Open archive"),
	  G_CALLBACK (activate_action_open) },
	{ "Open_Toolbar", "document-open",
	  N_("_Open"), "<control>O",
	  N_("Open archive"),
	  G_CALLBACK (activate_action_open) },
	{ "OpenSelection", NULL,
	  N_("_Open With…"), NULL,
	  N_("Open selected files with an application"),
	  G_CALLBACK (activate_action_open_with) },
	{ "Password", NULL,
	  N_("Pass_word…"), NULL,
	  N_("Specify a password for this archive"),
	  G_CALLBACK (activate_action_password) },
	{ "Properties", "document-properties",
	  N_("_Properties"), "<alt>Return",
	  N_("Show archive properties"),
	  G_CALLBACK (activate_action_properties) },
	{ "Reload", "view-refresh",
	  N_("_Refresh"), "<control>R",
	  N_("Reload current archive"),
	  G_CALLBACK (activate_action_reload) },
	{ "SaveAs", "document-save-as",
	  N_("Save As…"), NULL,
	  N_("Save the current archive with a different name"),
	  G_CALLBACK (activate_action_save_as) },
	{ "SelectAll", "edit-select-all",
	  N_("Select _All"), "<control>A",
	  N_("Select all files"),
	  G_CALLBACK (activate_action_select_all) },
	{ "Stop", "process-stop",
	  N_("_Stop"), "Escape",
	  N_("Stop current operation"),
	  G_CALLBACK (activate_action_stop) },
	{ "TestArchive", NULL,
	  N_("_Test Integrity"), NULL,
	  N_("Test whether the archive contains errors"),
	  G_CALLBACK (activate_action_test_archive) },
	{ "ViewSelection", "document-open",
	  N_("_Open"), "<control>O",
	  N_("Open the selected file"),
	  G_CALLBACK (activate_action_view_or_open) },
	{ "ViewSelection_Toolbar", "document-open",
	  N_("_Open"), "<control>O",
	  N_("Open the selected file"),
	  G_CALLBACK (activate_action_view_or_open) },
	{ "OpenFolder", "document-open",
	  N_("_Open"), "<control>O",
	  N_("Open the selected folder"),
	  G_CALLBACK (activate_action_open_folder) },
	{ "OpenFolderFromSidebar", "document-open",
	  N_("_Open"), "<control>O",
	  N_("Open the selected folder"),
	  G_CALLBACK (activate_action_open_folder_from_sidebar) },

	{ "GoBack", "go-previous",
	  "Back", NULL,
	  N_("Go to the previous visited location"),
	  G_CALLBACK (activate_action_go_back) },
	{ "GoForward", "go-next",
	  NULL, NULL,
	  N_("Go to the next visited location"),
	  G_CALLBACK (activate_action_go_forward) },
	{ "GoUp", "go-up",
	  NULL, NULL,
	  N_("Go up one level"),
	  G_CALLBACK (activate_action_go_up) },
	{ "GoHome", "go-home",
	  NULL, NULL,
	  /* Translators: the home location is the home folder. */
	  N_("Go to the home location"),
	  G_CALLBACK (activate_action_go_home) },
};
static guint n_action_entries = G_N_ELEMENTS (action_entries);


static GtkToggleActionEntry action_toggle_entries[] = {
	{ "ViewToolbar", NULL,
	  N_("_Toolbar"), NULL,
	  N_("View the main toolbar"),
	  G_CALLBACK (activate_action_view_toolbar),
	  TRUE },
	{ "ViewStatusbar", NULL,
	  N_("Stat_usbar"), NULL,
	  N_("View the statusbar"),
	  G_CALLBACK (activate_action_view_statusbar),
	  TRUE },
	{ "SortReverseOrder", NULL,
	  N_("_Reversed Order"), NULL,
	  N_("Reverse the list order"),
	  G_CALLBACK (activate_action_sort_reverse_order),
	  FALSE },
	{ "ViewFolders", NULL,
	  N_("_Folders"), "F9",
	  N_("View the folders pane"),
	  G_CALLBACK (activate_action_view_folders),
	  FALSE },
};
static guint n_action_toggle_entries = G_N_ELEMENTS (action_toggle_entries);


static GtkRadioActionEntry view_as_entries[] = {
	{ "ViewAllFiles", NULL,
	  N_("View All _Files"), "<control>1",
	  " ", FR_WINDOW_LIST_MODE_FLAT },
	{ "ViewAsFolder", NULL,
	  N_("View as a F_older"), "<control>2",
	  " ", FR_WINDOW_LIST_MODE_AS_DIR },
};
static guint n_view_as_entries = G_N_ELEMENTS (view_as_entries);


static GtkRadioActionEntry sort_by_entries[] = {
	{ "SortByName", NULL,
	  N_("by _Name"), NULL,
	  N_("Sort file list by name"), FR_WINDOW_SORT_BY_NAME },
	{ "SortBySize", NULL,
	  N_("by _Size"), NULL,
	  N_("Sort file list by file size"), FR_WINDOW_SORT_BY_SIZE },
	{ "SortByType", NULL,
	  N_("by T_ype"), NULL,
	  N_("Sort file list by type"), FR_WINDOW_SORT_BY_TYPE },
	{ "SortByDate", NULL,
	  N_("by _Date Modified"), NULL,
	  N_("Sort file list by modification time"), FR_WINDOW_SORT_BY_TIME },
	{ "SortByLocation", NULL,
	  /* Translators: this is the "sort by file location" menu item */
	  N_("by _Location"), NULL,
	  /* Translators: location is the file location */
	  N_("Sort file list by location"), FR_WINDOW_SORT_BY_PATH },
};
static guint n_sort_by_entries = G_N_ELEMENTS (sort_by_entries);


#endif /* UI_H */

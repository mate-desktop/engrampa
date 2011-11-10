/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  File-Roller
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
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307, USA.
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
	/* Translators: this is the label for the "open recent file" sub-menu. */
	{ "OpenRecentMenu", NULL, N_("Open _Recent") },

	{ "About", GTK_STOCK_ABOUT,
	  NULL, NULL,
	  N_("Information about the program"),
	  G_CALLBACK (activate_action_about) },
	{ "AddFiles", FR_STOCK_ADD_FILES,
	  N_("_Add Files..."), NULL,
	  N_("Add files to the archive"),
	  G_CALLBACK (activate_action_add_files) },
	{ "AddFiles_Toolbar", FR_STOCK_ADD_FILES,
	  N_("Add Files"), NULL,
	  N_("Add files to the archive"),
	  G_CALLBACK (activate_action_add_files) },
	{ "AddFolder", FR_STOCK_ADD_FOLDER,
	  N_("Add a _Folder..."), NULL,
	  N_("Add a folder to the archive"),
	  G_CALLBACK (activate_action_add_folder) },
	{ "AddFolder_Toolbar", FR_STOCK_ADD_FOLDER,
	  N_("Add Folder"), NULL,
	  N_("Add a folder to the archive"),
	  G_CALLBACK (activate_action_add_folder) },
	{ "Close", GTK_STOCK_CLOSE,
	  NULL, NULL,
	  N_("Close the current archive"),
	  G_CALLBACK (activate_action_close) },
	{ "Contents", GTK_STOCK_HELP,
	  N_("Contents"), "F1",
	  N_("Display the File Roller Manual"),
	  G_CALLBACK (activate_action_manual) },

	{ "Copy", GTK_STOCK_COPY,
	  NULL, NULL,
	  N_("Copy the selection"),
	  G_CALLBACK (activate_action_copy) },
	{ "Cut", GTK_STOCK_CUT,
	  NULL, NULL,
	  N_("Cut the selection"),
	  G_CALLBACK (activate_action_cut) },
	{ "Paste", GTK_STOCK_PASTE,
	  NULL, NULL,
	  N_("Paste the clipboard"),
	  G_CALLBACK (activate_action_paste) },
	{ "Rename", NULL,
	  N_("_Rename..."), "F2",
	  N_("Rename the selection"),
	  G_CALLBACK (activate_action_rename) },
	{ "Delete", GTK_STOCK_DELETE,
	  NULL, "Delete",
	  N_("Delete the selection from the archive"),
	  G_CALLBACK (activate_action_delete) },

	{ "CopyFolderFromSidebar", GTK_STOCK_COPY,
	  NULL, NULL,
	  N_("Copy the selection"),
	  G_CALLBACK (activate_action_copy_folder_from_sidebar) },
	{ "CutFolderFromSidebar", GTK_STOCK_CUT,
	  NULL, NULL,
	  N_("Cut the selection"),
	  G_CALLBACK (activate_action_cut_folder_from_sidebar) },
	{ "PasteFolderFromSidebar", GTK_STOCK_PASTE,
	  NULL, NULL,
	  N_("Paste the clipboard"),
	  G_CALLBACK (activate_action_paste_folder_from_sidebar) },
	{ "RenameFolderFromSidebar", NULL,
	  N_("_Rename..."), "F2",
	  N_("Rename the selection"),
	  G_CALLBACK (activate_action_rename_folder_from_sidebar) },
	{ "DeleteFolderFromSidebar", GTK_STOCK_DELETE,
	  NULL, NULL,
	  N_("Delete the selection from the archive"),
	  G_CALLBACK (activate_action_delete_folder_from_sidebar) },

	{ "DeselectAll", NULL,
	  N_("Dese_lect All"), "<shift><control>A",
	  N_("Deselect all files"),
	  G_CALLBACK (activate_action_deselect_all) },
	{ "Extract", FR_STOCK_EXTRACT,
	  N_("_Extract..."), "<control>E",
	  N_("Extract files from the archive"),
	  G_CALLBACK (activate_action_extract) },
	{ "ExtractFolderFromSidebar", FR_STOCK_EXTRACT,
	  N_("_Extract..."), NULL,
	  N_("Extract files from the archive"),
	  G_CALLBACK (activate_action_extract_folder_from_sidebar) },
	{ "Extract_Toolbar", FR_STOCK_EXTRACT,
	  N_("Extract"), NULL,
	  N_("Extract files from the archive"),
	  G_CALLBACK (activate_action_extract) },
	{ "Find", GTK_STOCK_FIND,
	  N_("Find..."), NULL,
	  NULL,
	  G_CALLBACK (activate_action_find) },

	{ "LastOutput", NULL,
	  N_("_Last Output"), NULL,
	  N_("View the output produced by the last executed command"),
	  G_CALLBACK (activate_action_last_output) },
	{ "New", GTK_STOCK_NEW,
	  NC_("File", "New..."), NULL,
	  N_("Create a new archive"),
	  G_CALLBACK (activate_action_new) },
	{ "Open", GTK_STOCK_OPEN,
	  NC_("File", "Open..."), NULL,
	  N_("Open archive"),
	  G_CALLBACK (activate_action_open) },
	{ "Open_Toolbar", GTK_STOCK_OPEN,
	  NULL, NULL,
	  N_("Open archive"),
	  G_CALLBACK (activate_action_open) },
	{ "OpenSelection", NULL,
	  N_("_Open With..."), NULL,
	  N_("Open selected files with an application"),
	  G_CALLBACK (activate_action_open_with) },
	{ "Password", NULL,
	  N_("Pass_word..."), NULL,
	  N_("Specify a password for this archive"),
	  G_CALLBACK (activate_action_password) },
	{ "Properties", GTK_STOCK_PROPERTIES,
	  NULL, "<alt>Return",
	  N_("Show archive properties"),
	  G_CALLBACK (activate_action_properties) },
	{ "Reload", GTK_STOCK_REFRESH,
	  NULL, "<control>R",
	  N_("Reload current archive"),
	  G_CALLBACK (activate_action_reload) },
	{ "SaveAs", GTK_STOCK_SAVE_AS,
	  NC_("File", "Save As..."), NULL,
	  N_("Save the current archive with a different name"),
	  G_CALLBACK (activate_action_save_as) },
	{ "SelectAll", GTK_STOCK_SELECT_ALL,
	  NULL, "<control>A",
	  N_("Select all files"),
	  G_CALLBACK (activate_action_select_all) },
	{ "Stop", GTK_STOCK_STOP,
	  NULL, "Escape",
	  N_("Stop current operation"),
	  G_CALLBACK (activate_action_stop) },
	{ "TestArchive", NULL,
	  N_("_Test Integrity"), NULL,
	  N_("Test whether the archive contains errors"),
	  G_CALLBACK (activate_action_test_archive) },
	{ "ViewSelection", GTK_STOCK_OPEN,
	  NULL, NULL,
	  N_("Open the selected file"),
	  G_CALLBACK (activate_action_view_or_open) },
	{ "ViewSelection_Toolbar", GTK_STOCK_OPEN,
	  NULL, NULL,
	  N_("Open the selected file"),
	  G_CALLBACK (activate_action_view_or_open) },
	{ "OpenFolder", GTK_STOCK_OPEN,
	  NULL, NULL,
	  N_("Open the selected folder"),
	  G_CALLBACK (activate_action_open_folder) },
	{ "OpenFolderFromSidebar", GTK_STOCK_OPEN,
	  NULL, NULL,
	  N_("Open the selected folder"),
	  G_CALLBACK (activate_action_open_folder_from_sidebar) },

	{ "GoBack", GTK_STOCK_GO_BACK,
	  NULL, NULL,
	  N_("Go to the previous visited location"),
	  G_CALLBACK (activate_action_go_back) },
	{ "GoForward", GTK_STOCK_GO_FORWARD,
	  NULL, NULL,
	  N_("Go to the next visited location"),
	  G_CALLBACK (activate_action_go_forward) },
	{ "GoUp", GTK_STOCK_GO_UP,
	  NULL, NULL,
	  N_("Go up one level"),
	  G_CALLBACK (activate_action_go_up) },
	{ "GoHome", GTK_STOCK_HOME,
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


static const gchar *ui_info =
"<ui>"
"  <menubar name='MenuBar'>"
"    <menu name='Archive' action='FileMenu'>"
"      <menuitem action='New'/>"
"      <menuitem action='Open'/>"
"      <menu name='OpenRecentMenu' action='OpenRecentMenu'>"
"        <menuitem action='Open'/>"
"      </menu>"
"      <menuitem action='SaveAs'/>"
"      <separator/>"
"      <menuitem action='Extract'/>"
"      <menuitem action='TestArchive'/>"
"      <separator/>"
"      <menuitem action='Properties'/>"
"      <separator/>"
"      <menuitem action='Close'/>"
"    </menu>"
"    <menu action='EditMenu'>"
"      <menuitem action='Cut'/>"
"      <menuitem action='Copy'/>"
"      <menuitem action='Paste'/>"
"      <menuitem action='Rename'/>"
"      <menuitem action='Delete'/>"
"      <separator/>"
"      <menuitem action='SelectAll'/>"
"      <menuitem action='DeselectAll'/>"
"      <separator/>"
"      <menuitem action='Find'/>"
"      <separator/>"
"      <menuitem action='AddFiles'/>"
"      <menuitem action='AddFolder'/>"
"      <separator/>"
"      <menuitem action='Password'/>"
"    </menu>"
"    <menu action='ViewMenu'>"
"      <menuitem action='ViewToolbar'/>"
"      <menuitem action='ViewStatusbar'/>"
"      <menuitem action='ViewFolders'/>"
"      <separator/>"
"      <menuitem action='ViewAllFiles'/>"
"      <menuitem action='ViewAsFolder'/>"
/*"      <separator/>"
"      <menu action='ArrangeFilesMenu'>"
"        <menuitem action='SortByName'/>"
"        <menuitem action='SortBySize'/>"
"        <menuitem action='SortByType'/>"
"        <menuitem action='SortByDate'/>"
"        <menuitem action='SortByLocation'/>"
"        <separator/>"
"        <menuitem action='SortReverseOrder'/>"
"      </menu>"*/
"      <separator/>"
"      <menuitem action='LastOutput'/>"
"      <separator/>"
"      <menuitem action='Stop'/>"
"      <menuitem action='Reload'/>"
"    </menu>"
"    <menu action='HelpMenu'>"
"      <menuitem action='Contents'/>"
"      <menuitem action='About'/>"
"    </menu>"
"  </menubar>"
"  <toolbar name='ToolBar'>"
"    <toolitem action='New'/>"
"    <separator/>"
"    <toolitem action='Extract_Toolbar'/>"
"    <separator/>"
"    <toolitem action='AddFiles_Toolbar'/>"
"    <toolitem action='AddFolder_Toolbar'/>"
"    <separator/>"
"    <toolitem action='Stop'/>"
"  </toolbar>"
"  <toolbar name='LocationBar'>"
"    <toolitem action='GoBack'/>"
"    <toolitem action='GoForward'/>"
"    <toolitem action='GoUp'/>"
"    <toolitem action='GoHome'/>"
"  </toolbar>"
"  <popup name='FilePopupMenu'>"
"    <menuitem action='ViewSelection'/>"
"    <menuitem action='OpenSelection'/>"
"    <separator/>"
"    <menuitem action='Extract'/>"
"    <separator/>"
"    <menuitem action='Cut'/>"
"    <menuitem action='Copy'/>"
"    <menuitem action='Paste'/>"
"    <menuitem action='Rename'/>"
"    <menuitem action='Delete'/>"
"  </popup>"
"  <popup name='FolderPopupMenu'>"
"    <menuitem action='OpenFolder'/>"
"    <separator/>"
"    <menuitem action='Extract'/>"
"    <separator/>"
"    <menuitem action='Cut'/>"
"    <menuitem action='Copy'/>"
"    <menuitem action='Paste'/>"
"    <menuitem action='Rename'/>"
"    <menuitem action='Delete'/>"
"  </popup>"
"  <popup name='AddMenu'>"
"    <menuitem action='AddFiles'/>"
"    <menuitem action='AddFolder'/>"
"  </popup>"
"  <popup name='SidebarFolderPopupMenu'>"
"    <menuitem action='OpenFolderFromSidebar'/>"
"    <separator/>"
"    <menuitem action='ExtractFolderFromSidebar'/>"
"    <separator/>"
"    <menuitem action='CutFolderFromSidebar'/>"
"    <menuitem action='CopyFolderFromSidebar'/>"
"    <menuitem action='PasteFolderFromSidebar'/>"
"    <menuitem action='RenameFolderFromSidebar'/>"
"    <menuitem action='DeleteFolderFromSidebar'/>"
"  </popup>"
"</ui>";


#endif /* UI_H */

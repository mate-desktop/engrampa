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

#ifndef ACTIONS_H
#define ACTIONS_H

#include <gtk/gtk.h>
#include "fr-window.h"

void show_new_archive_dialog (FrWindow *window, const char *archive_name);

void activate_action_new (GtkAction *action, gpointer data);
void activate_action_open (GtkAction *action, gpointer data);
void activate_action_save_as (GtkAction *action, gpointer data);
void activate_action_test_archive (GtkAction *action, gpointer data);
void activate_action_properties (GtkAction *action, gpointer data);
void activate_action_close (GtkAction *action, gpointer data);
void activate_action_quit (GtkAction *action, gpointer data);

void activate_action_add_files (GtkAction *action, gpointer data);
void activate_action_add_folder (GtkAction *action, gpointer data);
void activate_action_extract (GtkAction *action, gpointer data);
void activate_action_extract_folder_from_sidebar (GtkAction *action, gpointer data);

void activate_action_copy (GtkAction *action, gpointer data);
void activate_action_cut (GtkAction *action, gpointer data);
void activate_action_paste (GtkAction *action, gpointer data);
void activate_action_rename (GtkAction *action, gpointer data);
void activate_action_delete (GtkAction *action, gpointer data);

void activate_action_copy_folder_from_sidebar (GtkAction *action, gpointer data);
void activate_action_cut_folder_from_sidebar (GtkAction *action, gpointer data);
void activate_action_paste_folder_from_sidebar (GtkAction *action, gpointer data);
void activate_action_rename_folder_from_sidebar (GtkAction *action, gpointer data);
void activate_action_delete_folder_from_sidebar (GtkAction *action, gpointer data);

void activate_action_find (GtkAction *action, gpointer data);
void activate_action_select_all (GtkAction *action, gpointer data);
void activate_action_deselect_all (GtkAction *action, gpointer data);
void activate_action_open_with (GtkAction *action, gpointer data);
void activate_action_view_or_open (GtkAction *action, gpointer data);
void activate_action_open_folder (GtkAction *action, gpointer data);
void activate_action_open_folder_from_sidebar (GtkAction *action, gpointer data);
void activate_action_password (GtkAction *action, gpointer data);

void activate_action_view_toolbar (GtkAction *action, gpointer data);
void activate_action_view_statusbar (GtkAction *action, gpointer data);
void activate_action_view_folders (GtkAction *action, gpointer data);
void activate_action_stop (GtkAction *action, gpointer data);
void activate_action_reload (GtkAction *action, gpointer data);
void activate_action_sort_reverse_order (GtkAction *action, gpointer data);
void activate_action_last_output (GtkAction *action, gpointer data);

void activate_action_go_back (GtkAction *action, gpointer data);
void activate_action_go_forward (GtkAction *action, gpointer data);
void activate_action_go_up (GtkAction *action, gpointer data);
void activate_action_go_home (GtkAction *action, gpointer data);

void activate_action_manual (GtkAction *action, gpointer data);
void activate_action_about (GtkAction *action, gpointer data);


#endif /* ACTIONS_H */

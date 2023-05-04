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

#ifndef ACTIONS_H
#define ACTIONS_H

#include <gtk/gtk.h>
#include "fr-window.h"

void show_new_archive_dialog                    (FrWindow      *window,
                                                 const char    *archive_name);

void activate_action_new                        (GSimpleAction *action,
                                                 GVariant      *parameter,
                                                 gpointer       data);

void activate_action_open                       (GSimpleAction *action,
                                                 GVariant      *parameter,
                                                 gpointer       data);

void activate_action_save_as                    (GSimpleAction *action,
                                                 GVariant      *parameter,
                                                 gpointer       data);

void activate_action_test_archive               (GSimpleAction *action,
                                                 GVariant      *parameter,
                                                 gpointer       data);

void activate_action_properties                 (GSimpleAction *action,
                                                 GVariant      *parameter,
                                                 gpointer       data);

void activate_action_close                      (GSimpleAction *action,
                                                 GVariant      *parameter,
                                                 gpointer       data);

void activate_action_quit                       (GSimpleAction *action,
                                                 GVariant      *parameter,
                                                 gpointer       data);

void activate_action_add_files                  (GSimpleAction *action,
                                                 GVariant      *parameter,
                                                 gpointer       data);

void activate_action_add_folder                 (GSimpleAction *action,
                                                 GVariant      *parameter,
                                                 gpointer       data);

void activate_action_extract                    (GSimpleAction *action,
                                                 GVariant      *parameter,
                                                 gpointer       data);

void activate_action_copy                       (GSimpleAction *action,
                                                 GVariant      *parameter,
                                                 gpointer       data);

void activate_action_cut                        (GSimpleAction *action,
                                                 GVariant      *parameter,
                                                 gpointer       data);

void activate_action_paste                      (GSimpleAction *action,
                                                 GVariant      *parameter,
                                                 gpointer       data);

void activate_action_rename                     (GSimpleAction *action,
                                                 GVariant      *parameter,
                                                 gpointer       data);

void activate_action_delete                     (GSimpleAction *action,
                                                 GVariant      *parameter,
                                                 gpointer       data);

void activate_action_find                       (GSimpleAction *action,
                                                 GVariant      *parameter,
                                                 gpointer       data);

void activate_action_select_all                 (GSimpleAction *action,
                                                 GVariant      *parameter,
                                                 gpointer       data);

void activate_action_deselect_all               (GSimpleAction *action,
                                                 GVariant      *parameter,
                                                 gpointer       data);

void activate_action_open_with                  (GSimpleAction *action,
                                                 GVariant      *parameter,
                                                 gpointer       data);

void activate_action_password                   (GSimpleAction *action,
                                                 GVariant      *parameter,
                                                 gpointer       data);

void activate_action_view_toolbar               (GSimpleAction *action,
                                                 GVariant      *parameter,
                                                 gpointer       data);

void activate_action_view_statusbar             (GSimpleAction *action,
                                                 GVariant      *parameter,
                                                 gpointer       data);

void activate_action_view_folders               (GSimpleAction *action,
                                                 GVariant      *parameter,
                                                 gpointer       data);

void activate_action_stop                       (GSimpleAction *action,
                                                 GVariant      *parameter,
                                                 gpointer       data);

void activate_action_reload                     (GSimpleAction *action,
                                                 GVariant      *parameter,
                                                 gpointer       data);

void activate_action_last_output                (GSimpleAction *action,
                                                 GVariant      *parameter,
                                                 gpointer       data);

void activate_action_go_back                    (GSimpleAction *action,
                                                 GVariant      *parameter,
                                                 gpointer       data);

void activate_action_go_forward                 (GSimpleAction *action,
                                                 GVariant      *parameter,
                                                 gpointer       data);

void activate_action_go_up                      (GSimpleAction *action,
                                                 GVariant      *parameter,
                                                 gpointer       data);

void activate_action_go_home                    (GSimpleAction *action,
                                                 GVariant      *parameter,
                                                 gpointer       data);

void activate_action_manual                     (GSimpleAction *action,
                                                 GVariant      *parameter,
                                                 gpointer       data);

void activate_action_about                      (GSimpleAction *action,
                                                 GVariant      *parameter,
                                                 gpointer       data);

void activate_toggle                            (GSimpleAction *action,
                                                 GVariant      *parameter,
                                                 gpointer       user_data);

void activate_popup_open                        (GtkMenuItem   *menuitem,
                                                 gpointer       data);

void activate_popup_open_select                 (GtkMenuItem   *menuitem,
                                                 gpointer       data);

void activate_popup_extract                     (GtkMenuItem   *menuitem,
                                                 gpointer       data);

void activate_popup_cut                         (GtkMenuItem   *menuitem,
                                                 gpointer       data);

void activate_popup_paste                       (GtkMenuItem   *menuitem,
                                                 gpointer       data);

void activate_popup_rename                      (GtkMenuItem   *menuitem,
                                                 gpointer       data);

void activate_popup_delete                      (GtkMenuItem   *menuitem,
                                                 gpointer       data);

void init_engramp_menu_popup                    (FrWindow      *window,
                                                 GtkBuilder    *ui);

void activate_popup_open_folder                 (GtkMenuItem   *menuitem,
                                                 gpointer       data);

void activate_popup_copy                        (GtkMenuItem   *menuitem,
                                                 gpointer       data);

void activate_popup_open_folder_from_sidebar    (GtkMenuItem   *menuitem,
                                                 gpointer       data);

void activate_popup_extract_folder_from_sidebar (GtkMenuItem   *menuitem,
                                                 gpointer       data);

void activate_popup_copy_folder_from_sidebar    (GtkMenuItem   *menuitem,
                                                 gpointer       data);

void activate_popup_cut_folder_from_sidebar     (GtkMenuItem   *menuitem,
                                                 gpointer       data);

void activate_popup_paste_folder_from_sidebar   (GtkMenuItem   *menuitem,
                                                 gpointer       data);

void activate_popup_rename_folder_from_sidebar  (GtkMenuItem   *menuitem,
                                                 gpointer       data);

void activate_popup_delete_folder_from_sidebar  (GtkMenuItem   *menuitem,
                                                 gpointer       data);
#endif /* ACTIONS_H */

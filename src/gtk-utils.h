/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  File-Roller
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
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307, USA.
 */

#ifndef GTK_UTILS_H
#define GTK_UTILS_H

#include <glib/gi18n.h>
#include <gio/gio.h>
#include <gtk/gtk.h>

int         _gtk_count_selected             (GtkTreeSelection *selection);
GtkWidget*  _gtk_message_dialog_new         (GtkWindow        *parent,
					     GtkDialogFlags    flags,
					     const char       *stock_id,
					     const char       *message,
					     const char       *secondary_message,
					     const char       *first_button_text,
					     ...);
gchar*      _gtk_request_dialog_run         (GtkWindow        *parent,
					     GtkDialogFlags    flags,
					     const char       *title,
					     const char       *message,
					     const char       *default_value,
					     int               max_length,
					     const char       *no_button_text,
					     const char       *yes_button_text);
GtkWidget*  _gtk_yesno_dialog_new           (GtkWindow        *parent,
					     GtkDialogFlags    flags,
					     const char       *message,
					     const char       *no_button_text,
					     const char       *yes_button_text);
GtkWidget*  _gtk_error_dialog_new           (GtkWindow        *parent,
					     GtkDialogFlags    flags,
					     GList            *row_output,
					     const char       *primary_text,
					     const char       *secondary_text,
					     ...) G_GNUC_PRINTF (5, 6);
void        _gtk_error_dialog_run           (GtkWindow        *parent,
					     const gchar      *main_message,
					     const gchar      *format,
					     ...);
void        _gtk_entry_set_locale_text      (GtkEntry   *entry,
					     const char *text);
char *      _gtk_entry_get_locale_text      (GtkEntry   *entry);
void        _gtk_label_set_locale_text      (GtkLabel   *label,
					     const char *text);
char *      _gtk_label_get_locale_text      (GtkLabel   *label);
void        _gtk_entry_set_filename_text    (GtkEntry   *entry,
					     const char *text);
char *      _gtk_entry_get_filename_text    (GtkEntry   *entry);
void        _gtk_label_set_filename_text    (GtkLabel   *label,
					     const char *text);
char *      _gtk_label_get_filename_text    (GtkLabel   *label);
GdkPixbuf * get_icon_pixbuf                 (GIcon        *icon,
		 			     int           size,
		 			     GtkIconTheme *icon_theme);
GdkPixbuf * get_mime_type_pixbuf            (const char   *mime_type,
		                             int           icon_size,
		                             GtkIconTheme *icon_theme);
int         get_folder_pixbuf_size_for_list (GtkWidget    *widget);
gboolean    show_uri                        (GdkScreen   *screen,
					     const char  *uri,
				             guint32      timestamp,
				             GError     **error);
void        show_help_dialog                (GtkWindow    *parent,
					     const char   *section);
GtkBuilder *  
            _gtk_builder_new_from_file      (const char   *filename);
GtkWidget *
	    _gtk_builder_get_widget         (GtkBuilder   *builder,
			 		     const char   *name);
			 		  
#endif

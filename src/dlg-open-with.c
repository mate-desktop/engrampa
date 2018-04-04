/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  Engrampa
 *
 *  Copyright (C) 2001, 2003, 2005 Free Software Foundation, Inc.
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

#include <config.h>
#include <string.h>

#include <gtk/gtk.h>
#include "file-utils.h"
#include "glib-utils.h"
#include "gtk-utils.h"
#include "fr-init.h"
#include "fr-window.h"
#include "dlg-open-with.h"


typedef struct {
	FrWindow *window;
	GList    *file_list;
} OpenData;


static void
app_chooser_response_cb (GtkDialog *dialog,
			 int        response_id,
			 gpointer   user_data)
{
	OpenData *o_data = user_data;
	GAppInfo *app_info;

	switch (response_id) {
	case GTK_RESPONSE_OK:
		app_info = gtk_app_chooser_get_app_info (GTK_APP_CHOOSER (dialog));
		if (app_info != NULL) {
			fr_window_open_files_with_application (o_data->window, o_data->file_list, app_info);
			g_object_unref (app_info);
		}
		g_free (o_data);
		gtk_widget_destroy (GTK_WIDGET (dialog));
		break;

	case GTK_RESPONSE_CANCEL:
	case GTK_RESPONSE_DELETE_EVENT:
		g_free (o_data);
		gtk_widget_destroy (GTK_WIDGET (dialog));
		break;

	default:
		break;
	}
}


void
dlg_open_with (FrWindow *window,
	       GList    *file_list)
{
	OpenData  *o_data;
	GtkWidget *app_chooser;
	GFile     *first_file;

	o_data = g_new0 (OpenData, 1);
	o_data->window = window;
	o_data->file_list = file_list;

	first_file = g_file_new_for_path (file_list->data);
	app_chooser = gtk_app_chooser_dialog_new (GTK_WINDOW (window),
						  GTK_DIALOG_MODAL,
						  first_file);
	g_signal_connect (app_chooser,
			  "response",
			  G_CALLBACK (app_chooser_response_cb),
			  o_data);
	gtk_widget_show (app_chooser);
}


void
open_with_cb (GtkWidget *widget,
	      void      *callback_data)
{
	FrWindow *window = callback_data;
	GList    *file_list;

	file_list = fr_window_get_file_list_selection (window, FALSE, NULL);
	if (file_list == NULL)
		return;

	fr_window_open_files (window, file_list, TRUE);
	path_list_free (file_list);
}

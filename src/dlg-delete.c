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

#include <config.h>
#include <gtk/gtk.h>
#include "fr-window.h"
#include "gtk-utils.h"
#include "file-utils.h"


typedef struct {
	FrWindow  *window;
	GList     *selected_files;
	GtkBuilder *builder;

	GtkWidget *dialog;
	GtkWidget *d_all_files_radio;
	GtkWidget *d_selected_files_radio;
	GtkWidget *d_files_radio;
	GtkWidget *d_files_entry;
} DialogData;


/* called when the main dialog is closed. */
static void
destroy_cb (GtkWidget  *widget,
	    DialogData *data)
{
	path_list_free (data->selected_files);
	g_object_unref (G_OBJECT (data->builder));
	g_free (data);
}


/* called when the "ok" button is pressed. */
static void
ok_clicked_cb (GtkWidget  *widget,
	       DialogData *data)
{
	gboolean  selected_files;
	gboolean  pattern_files;
	FrWindow *window = data->window;
	GList    *file_list = NULL;
	gboolean  do_not_remove_if_null = FALSE;

	selected_files = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->d_selected_files_radio));
	pattern_files = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->d_files_radio));

	/* create the file list. */

	if (selected_files) {
		file_list = data->selected_files;
		data->selected_files = NULL;       /* do not the list when destroying the dialog. */
	}
	else if (pattern_files) {
		const char *pattern;

		pattern = gtk_entry_get_text (GTK_ENTRY (data->d_files_entry));
		file_list = fr_window_get_file_list_pattern (window, pattern);
		if (file_list == NULL)
			do_not_remove_if_null = TRUE;
	}

	/* close the dialog. */

	gtk_widget_destroy (data->dialog);

	/* remove ! */

	if (! do_not_remove_if_null || (file_list != NULL))
		fr_window_archive_remove (window, file_list);

	path_list_free (file_list);
}


static void
entry_changed_cb (GtkWidget  *widget,
		  DialogData *data)
{
	if (! gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->d_files_radio)))
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->d_files_radio), TRUE);
}


static void
dlg_delete__common (FrWindow *window,
	            GList    *selected_files)
{
	DialogData *data;
	GtkWidget  *cancel_button;
	GtkWidget  *ok_button;

	data = g_new (DialogData, 1);
	data->window = window;
	data->selected_files = selected_files;

	data->builder = _gtk_builder_new_from_file ("delete.ui");
	if (data->builder == NULL) {
		g_free (data);
		return;
	}

	/* Get the widgets. */

	data->dialog = _gtk_builder_get_widget (data->builder, "delete_dialog");
	data->d_all_files_radio = _gtk_builder_get_widget (data->builder, "d_all_files_radio");
	data->d_selected_files_radio = _gtk_builder_get_widget (data->builder, "d_selected_files_radio");
	data->d_files_radio = _gtk_builder_get_widget (data->builder, "d_files_radio");
	data->d_files_entry = _gtk_builder_get_widget (data->builder, "d_files_entry");

	ok_button = _gtk_builder_get_widget (data->builder, "d_ok_button");
	cancel_button = _gtk_builder_get_widget (data->builder, "d_cancel_button");

	/* Set widgets data. */

	if (data->selected_files != NULL)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->d_selected_files_radio), TRUE);
	else {
		gtk_widget_set_sensitive (data->d_selected_files_radio, FALSE);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->d_all_files_radio), TRUE);
	}

	/* Set the signals handlers. */

	g_signal_connect (G_OBJECT (data->dialog),
			  "destroy",
			  G_CALLBACK (destroy_cb),
			  data);
	g_signal_connect_swapped (G_OBJECT (cancel_button),
				  "clicked",
				  G_CALLBACK (gtk_widget_destroy),
				  G_OBJECT (data->dialog));
	g_signal_connect (G_OBJECT (ok_button),
			  "clicked",
			  G_CALLBACK (ok_clicked_cb),
			  data);
	g_signal_connect (G_OBJECT (data->d_files_entry),
			  "changed",
			  G_CALLBACK (entry_changed_cb),
			  data);

	/* Run dialog. */

	gtk_window_set_transient_for (GTK_WINDOW (data->dialog),
				      GTK_WINDOW (window));
	gtk_window_set_modal         (GTK_WINDOW (data->dialog), TRUE);

	gtk_widget_show (data->dialog);
}


void
dlg_delete (GtkWidget *widget,
	    gpointer   callback_data)
{
	FrWindow *window = callback_data;
	dlg_delete__common (window,
			    fr_window_get_file_list_selection (window, TRUE, NULL));
}


void
dlg_delete_from_sidebar (GtkWidget *widget,
			 gpointer   callback_data)
{
	FrWindow *window = callback_data;
	dlg_delete__common (window,
			    fr_window_get_folder_tree_selection (window, TRUE, NULL));
}

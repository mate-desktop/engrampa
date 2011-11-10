/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  File-Roller
 *
 *  Copyright (C) 2001 The Free Software Foundation, Inc.
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
#include <string.h>
#include <gtk/gtk.h>
#include "fr-window.h"
#include "mateconf-utils.h"
#include "gtk-utils.h"
#include "preferences.h"


typedef struct {
	GtkBuilder *builder;
	FrWindow  *window;
	GtkWidget *dialog;
	GtkWidget *pw_password_entry;
	GtkWidget *pw_encrypt_header_checkbutton;
} DialogData;


/* called when the main dialog is closed. */
static void
destroy_cb (GtkWidget  *widget,
	    DialogData *data)
{
	g_object_unref (data->builder);
	g_free (data);
}


static void
response_cb (GtkWidget  *dialog,
	     int         response_id,
	     DialogData *data)
{
	char     *password;
	gboolean  encrypt_header;

	switch (response_id) {
	case GTK_RESPONSE_OK:
		password = _gtk_entry_get_locale_text (GTK_ENTRY (data->pw_password_entry));
		fr_window_set_password (data->window, password);
		g_free (password);

		encrypt_header = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->pw_encrypt_header_checkbutton));
		eel_mateconf_set_boolean (PREF_ENCRYPT_HEADER, encrypt_header);
		fr_window_set_encrypt_header (data->window, encrypt_header);
		break;
	default:
		break;
	}

	gtk_widget_destroy (data->dialog);
}


void
dlg_password (GtkWidget *widget,
	      gpointer   callback_data)
{
	FrWindow   *window = callback_data;
	DialogData *data;

	data = g_new0 (DialogData, 1);

	data->builder = _gtk_builder_new_from_file ("password.ui");
	if (data->builder == NULL) {
		g_free (data);
		return;
	}

	data->window = window;

	/* Get the widgets. */

	data->dialog = _gtk_builder_get_widget (data->builder, "password_dialog");
	data->pw_password_entry = _gtk_builder_get_widget (data->builder, "pw_password_entry");
	data->pw_encrypt_header_checkbutton = _gtk_builder_get_widget (data->builder, "pw_encrypt_header_checkbutton");

	/* Set widgets data. */

	_gtk_entry_set_locale_text (GTK_ENTRY (data->pw_password_entry), fr_window_get_password (window));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->pw_encrypt_header_checkbutton), fr_window_get_encrypt_header (window));

	/* Set the signals handlers. */

	g_signal_connect (G_OBJECT (data->dialog),
			  "destroy",
			  G_CALLBACK (destroy_cb),
			  data);

	g_signal_connect (G_OBJECT (data->dialog),
			  "response",
			  G_CALLBACK (response_cb),
			  data);

	/* Run dialog. */

	gtk_widget_grab_focus (data->pw_password_entry);
	if (gtk_widget_get_realized (GTK_WIDGET (window)))
		gtk_window_set_transient_for (GTK_WINDOW (data->dialog),
					      GTK_WINDOW (window));
	gtk_window_set_modal (GTK_WINDOW (data->dialog), TRUE);

	gtk_widget_show (data->dialog);
}

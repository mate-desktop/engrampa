/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  Engrampa
 *
 *  Copyright (C) 2001, 2004 Free Software Foundation, Inc.
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

#include <config.h>
#include <string.h>
#include <gtk/gtk.h>
#include "glib-utils.h"
#include "file-utils.h"
#include "gtk-utils.h"
#include "fr-window.h"
#include "dlg-prop.h"


typedef struct {
	GtkBuilder *builder;
	GtkWidget *dialog;
} DialogData;


/* called when the main dialog is closed. */
static void
destroy_cb (GtkWidget  *widget,
	    DialogData *data)
{
	g_object_unref (G_OBJECT (data->builder));
	g_free (data);
}


static int
help_cb (GtkWidget   *w,
	 DialogData  *data)
{
	show_help_dialog (GTK_WINDOW (data->dialog), "engrampa-view-archive-properties");
	return TRUE;
}


void
dlg_prop (FrWindow *window)
{
	DialogData       *data;
	GtkWidget        *ok_button;
	GtkWidget        *help_button;
	GtkWidget        *label;
	GFile            *parent;
	char             *uri;
	char             *markup;
	char             *s;
	goffset           size, uncompressed_size;
	char             *utf8_name;
	char             *title_txt;
	double            ratio;

	data = g_new (DialogData, 1);

	data->builder = _gtk_builder_new_from_resource ("properties.ui");
	if (data->builder == NULL) {
		g_free (data);
		return;
	}

	/* Get the widgets. */

	data->dialog = _gtk_builder_get_widget (data->builder, "prop_dialog");
	ok_button = _gtk_builder_get_widget (data->builder, "p_ok_button");
	help_button = _gtk_builder_get_widget (data->builder, "p_help_button");

	/* Set widgets data. */

	label = _gtk_builder_get_widget (data->builder, "p_path_label");
	uri = remove_level_from_path (fr_window_get_archive_uri (window));
	parent = g_file_new_for_uri (uri);
	utf8_name = g_file_get_parse_name (parent);
	markup = g_strdup_printf ("<a href=\"%s\">%s</a>", uri, utf8_name);
	gtk_label_set_markup (GTK_LABEL (label), markup);

	g_free (markup);
	g_free (utf8_name);
	g_free (uri);
	g_object_unref (parent);

	/**/

	label = _gtk_builder_get_widget (data->builder, "p_name_label");
	utf8_name = g_uri_display_basename (fr_window_get_archive_uri (window));
	gtk_label_set_text (GTK_LABEL (label), utf8_name);

	title_txt = g_strdup_printf (_("%s Properties"), utf8_name);
	gtk_window_set_title (GTK_WINDOW (data->dialog), title_txt);
	g_free (title_txt);

	g_free (utf8_name);

	/**/

	label = _gtk_builder_get_widget (data->builder, "p_date_label");
	GDateTime *date_time;
	date_time = g_date_time_new_from_unix_local (get_file_mtime (fr_window_get_archive_uri (window)));
	s = g_date_time_format (date_time, _("%d %B %Y, %H:%M"));
	g_date_time_unref (date_time);
	gtk_label_set_text (GTK_LABEL (label), s);
	g_free (s);

	/**/

	label = _gtk_builder_get_widget (data->builder, "p_size_label");
	size = get_file_size (fr_window_get_archive_uri (window));
	s = g_format_size_full (size, G_FORMAT_SIZE_LONG_FORMAT);
	gtk_label_set_text (GTK_LABEL (label), s);
	g_free (s);

	/**/

	uncompressed_size = 0;
	if (fr_window_archive_is_present (window)) {
		guint i;

		for (i = 0; i < window->archive->command->files->len; i++) {
			FileData *fd = g_ptr_array_index (window->archive->command->files, i);
			uncompressed_size += fd->size;
		}
	}

	label = _gtk_builder_get_widget (data->builder, "p_uncomp_size_label");
	s = g_format_size_full (uncompressed_size, G_FORMAT_SIZE_LONG_FORMAT);
	gtk_label_set_text (GTK_LABEL (label), s);
	g_free (s);

	/**/

	label = _gtk_builder_get_widget (data->builder, "p_cratio_label");

	if (uncompressed_size != 0)
		ratio = (double) uncompressed_size / size;
	else
		ratio = 0.0;
	s = g_strdup_printf ("%0.2f", ratio);
	gtk_label_set_text (GTK_LABEL (label), s);
	g_free (s);

	/**/

	label = _gtk_builder_get_widget (data->builder, "p_files_label");
	s = g_strdup_printf ("%d", window->archive->command->n_regular_files);
	gtk_label_set_text (GTK_LABEL (label), s);
	g_free (s);

	/**/

	label = _gtk_builder_get_widget (data->builder, "p_mime_type_label");
	gtk_label_set_text (GTK_LABEL (label), window->archive->command->mime_type);

	/* Set the signals handlers. */

	g_signal_connect (G_OBJECT (data->dialog),
			  "destroy",
			  G_CALLBACK (destroy_cb),
			  data);
	g_signal_connect_swapped (G_OBJECT (ok_button),
				  "clicked",
				  G_CALLBACK (gtk_widget_destroy),
				  G_OBJECT (data->dialog));
	g_signal_connect (G_OBJECT (help_button),
			  "clicked",
			  G_CALLBACK (help_cb),
			  data);

	/* Run dialog. */

	gtk_window_set_transient_for (GTK_WINDOW (data->dialog),
				      GTK_WINDOW (window));
	gtk_window_set_modal (GTK_WINDOW (data->dialog), TRUE);

	gtk_widget_show (data->dialog);
}

/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  File-Roller
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
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307, USA.
 */

#include <config.h>
#include <string.h>
#include <gtk/gtk.h>
#include "glib-utils.h"
#include "file-utils.h"
#include "gtk-utils.h"
#include "fr-window.h"


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


static void
set_label_type (GtkWidget *label, const char *text, const char *type)
{
	char *t;

	t = g_strdup_printf ("<%s>%s</%s>", type, text, type);
	gtk_label_set_markup (GTK_LABEL (label), t);
	g_free (t);
}


static void
set_label (GtkWidget *label, const char *text)
{
	set_label_type (label, text, "b");
}


static int
help_cb (GtkWidget   *w,
	 DialogData  *data)
{
	show_help_dialog (GTK_WINDOW (data->dialog), "file-roller-view-archive-properties");
	return TRUE;
}


void
dlg_prop (FrWindow *window)
{
	DialogData       *data;
	GtkWidget        *ok_button;
	GtkWidget        *help_button;
	GtkWidget        *label_label;
	GtkWidget        *label;
	char             *s;
	goffset           size, uncompressed_size;
	char             *utf8_name;
	char             *title_txt;
	double            ratio;

	data = g_new (DialogData, 1);

	data->builder = _gtk_builder_new_from_file ("properties.ui");
	if (data->builder == NULL) {
		g_free (data);
		return;
	}

	/* Get the widgets. */

	data->dialog = _gtk_builder_get_widget (data->builder, "prop_dialog");
	ok_button = _gtk_builder_get_widget (data->builder, "p_ok_button");
	help_button = _gtk_builder_get_widget (data->builder, "p_help_button");

	/* Set widgets data. */

	label_label = _gtk_builder_get_widget (data->builder, "p_path_label_label");
	/* Translators: after the colon there is a folder name. */
	set_label (label_label, _("Location:"));

	label = _gtk_builder_get_widget (data->builder, "p_path_label");
	s = remove_level_from_path (fr_window_get_archive_uri (window));
	utf8_name = g_filename_display_name (s);
	gtk_label_set_text (GTK_LABEL (label), utf8_name);
	g_free (utf8_name);
	g_free (s);

	/**/

	label_label = _gtk_builder_get_widget (data->builder, "p_name_label_label");
	set_label (label_label, C_("File", "Name:"));

	label = _gtk_builder_get_widget (data->builder, "p_name_label");
	utf8_name = g_uri_display_basename (fr_window_get_archive_uri (window));
	gtk_label_set_text (GTK_LABEL (label), utf8_name);

	title_txt = g_strdup_printf (_("%s Properties"), utf8_name);
	gtk_window_set_title (GTK_WINDOW (data->dialog), title_txt);
	g_free (title_txt);

	g_free (utf8_name);

	/**/

	label_label = _gtk_builder_get_widget (data->builder, "p_date_label_label");
	set_label (label_label, _("Modified on:"));

	label = _gtk_builder_get_widget (data->builder, "p_date_label");
	s = get_time_string (get_file_mtime (fr_window_get_archive_uri (window)));
	gtk_label_set_text (GTK_LABEL (label), s);
	g_free (s);

	/**/

	label_label = _gtk_builder_get_widget (data->builder, "p_size_label_label");
	set_label (label_label, _("Archive size:"));

	label = _gtk_builder_get_widget (data->builder, "p_size_label");
	size = get_file_size (fr_window_get_archive_uri (window));
	s = g_format_size_for_display (size);
	gtk_label_set_text (GTK_LABEL (label), s);
	g_free (s);

	/**/

	label_label = _gtk_builder_get_widget (data->builder, "p_uncomp_size_label_label");
	set_label (label_label, _("Content size:"));

	uncompressed_size = 0;
	if (fr_window_archive_is_present (window)) {
		int i;

		for (i = 0; i < window->archive->command->files->len; i++) {
			FileData *fd = g_ptr_array_index (window->archive->command->files, i);
			uncompressed_size += fd->size;
		}
	}

	label = _gtk_builder_get_widget (data->builder, "p_uncomp_size_label");
	s = g_format_size_for_display (uncompressed_size);
	gtk_label_set_text (GTK_LABEL (label), s);
	g_free (s);

	/**/

	label_label = _gtk_builder_get_widget (data->builder, "p_cratio_label_label");
	set_label (label_label, _("Compression ratio:"));

	label = _gtk_builder_get_widget (data->builder, "p_cratio_label");

	if (uncompressed_size != 0)
		ratio = (double) uncompressed_size / size;
	else
		ratio = 0.0;
	s = g_strdup_printf ("%0.2f", ratio);
	gtk_label_set_text (GTK_LABEL (label), s);
	g_free (s);

	/**/

	label_label = _gtk_builder_get_widget (data->builder, "p_files_label_label");
	set_label (label_label, _("Number of files:"));

	label = _gtk_builder_get_widget (data->builder, "p_files_label");
	s = g_strdup_printf ("%d", window->archive->command->n_regular_files);
	gtk_label_set_text (GTK_LABEL (label), s);
	g_free (s);

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

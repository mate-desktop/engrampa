/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  File-Roller
 *
 *  Copyright (C) 2008 Free Software Foundation, Inc.
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
#include <math.h>
#include <unistd.h>
#include <gio/gio.h>
#include "dlg-new.h"
#include "file-utils.h"
#include "fr-stock.h"
#include "mateconf-utils.h"
#include "gtk-utils.h"
#include "main.h"
#include "preferences.h"


#define GET_WIDGET(x) (_gtk_builder_get_widget (data->builder, (x)))
#define DEFAULT_EXTENSION ".tar.gz"
#define BAD_CHARS "/\\*"
#define MEGABYTE (1024 * 1024)


/* called when the main dialog is closed. */
static void
destroy_cb (GtkWidget  *widget,
	    DlgNewData *data)
{
	g_object_unref (data->builder);
	g_free (data);
}


static void
update_sensitivity (DlgNewData *data)
{
	gtk_toggle_button_set_inconsistent (GTK_TOGGLE_BUTTON (data->n_encrypt_header_checkbutton), ! data->can_encrypt_header);
	gtk_widget_set_sensitive (data->n_encrypt_header_checkbutton, data->can_encrypt_header);
	gtk_widget_set_sensitive (data->n_volume_spinbutton, ! data->can_create_volumes || gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->n_volume_checkbutton)));
}


static void
update_sensitivity_for_ext (DlgNewData *data,
			    const char *ext)
{
	const char *mime_type;
	int         i;

	data->can_encrypt = FALSE;
	data->can_encrypt_header = FALSE;
	data->can_create_volumes = FALSE;

	mime_type = get_mime_type_from_extension (ext);

	if (mime_type == NULL) {
		gtk_widget_set_sensitive (data->n_password_entry, FALSE);
		gtk_widget_set_sensitive (data->n_password_label, FALSE);
		gtk_widget_set_sensitive (data->n_encrypt_header_checkbutton, FALSE);
		gtk_widget_set_sensitive (data->n_volume_box, FALSE);
		return;
	}

	for (i = 0; mime_type_desc[i].mime_type != NULL; i++) {
		if (strcmp (mime_type_desc[i].mime_type, mime_type) == 0) {
			data->can_encrypt = mime_type_desc[i].capabilities & FR_COMMAND_CAN_ENCRYPT;
			gtk_widget_set_sensitive (data->n_password_entry, data->can_encrypt);
			gtk_widget_set_sensitive (data->n_password_label, data->can_encrypt);

			data->can_encrypt_header = mime_type_desc[i].capabilities & FR_COMMAND_CAN_ENCRYPT_HEADER;
			gtk_widget_set_sensitive (data->n_encrypt_header_checkbutton, data->can_encrypt_header);

			data->can_create_volumes = mime_type_desc[i].capabilities & FR_COMMAND_CAN_CREATE_VOLUMES;
			gtk_widget_set_sensitive (data->n_volume_box, data->can_create_volumes);

			break;
		}
	}

	update_sensitivity (data);
}


static int
get_archive_type (DlgNewData *data)
{
	const char *uri;
	const char *ext;

	uri = gtk_file_chooser_get_uri (GTK_FILE_CHOOSER (data->dialog));
	if (uri == NULL)
		return -1;

	ext = get_archive_filename_extension (uri);
	if (ext == NULL) {
		int idx;

		idx = egg_file_format_chooser_get_format (EGG_FILE_FORMAT_CHOOSER (data->format_chooser), uri);
		/*idx = gtk_combo_box_get_active (GTK_COMBO_BOX (data->n_archive_type_combo_box)) - 1;*/
		if (idx >= 0)
			return data->supported_types[idx];

		ext = DEFAULT_EXTENSION;
	}

	return get_mime_type_index (get_mime_type_from_extension (ext));
}


/* FIXME
static void
archive_type_combo_box_changed_cb (GtkComboBox *combo_box,
				   DlgNewData  *data)
{
	const char *uri;
	const char *ext;
	int         idx;

	uri = gtk_file_chooser_get_uri (GTK_FILE_CHOOSER (data->dialog));

	ext = get_archive_filename_extension (uri);
	idx = gtk_combo_box_get_active (GTK_COMBO_BOX (data->n_archive_type_combo_box)) - 1;
	if ((ext == NULL) && (idx >= 0))
		ext = mime_type_desc[data->supported_types[idx]].default_ext;

	update_sensitivity_for_ext (data, ext);

	if ((idx >= 0) && (uri != NULL)) {
		const char *new_ext;
		const char *basename;
		char       *basename_noext;
		char       *new_basename;
		char       *new_basename_uft8;

		new_ext = mime_type_desc[data->supported_types[idx]].default_ext;
		basename = file_name_from_path (uri);
		if (g_str_has_suffix (basename, ext))
			basename_noext = g_strndup (basename, strlen (basename) - strlen (ext));
		else
			basename_noext = g_strdup (basename);
		new_basename = g_strconcat (basename_noext, new_ext, NULL);
		new_basename_uft8 = g_uri_unescape_string (new_basename, NULL);

		gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (data->dialog), new_basename_uft8);
		update_sensitivity_for_ext (data, new_ext);

		g_free (new_basename_uft8);
		g_free (new_basename);
		g_free (basename_noext);
	}
}
*/


static void
password_entry_changed_cb (GtkEditable *editable,
			   gpointer     user_data)
{
	update_sensitivity ((DlgNewData *) user_data);
}


static void
volume_toggled_cb (GtkToggleButton *toggle_button,
		   gpointer         user_data)
{
	update_sensitivity ((DlgNewData *) user_data);
}


static void
format_chooser_selection_changed_cb (EggFileFormatChooser *format_chooser,
				     DlgNewData           *data)
{
	const char *uri;
	const char *ext;
	int         n_format;

	uri = gtk_file_chooser_get_uri (GTK_FILE_CHOOSER (data->dialog));
	if (uri == NULL)
		return;

	ext = get_archive_filename_extension (uri);
	n_format = egg_file_format_chooser_get_format (EGG_FILE_FORMAT_CHOOSER (data->format_chooser), uri);
	if (ext == NULL)
		ext = mime_type_desc[data->supported_types[n_format - 1]].default_ext;

	update_sensitivity_for_ext (data, ext);

	if (uri != NULL) {
		const char *new_ext;
		const char *basename;
		char       *basename_noext;
		char       *new_basename;
		char       *new_basename_uft8;

		new_ext = mime_type_desc[data->supported_types[n_format - 1]].default_ext;
		basename = file_name_from_path (uri);
		if (g_str_has_suffix (basename, ext))
			basename_noext = g_strndup (basename, strlen (basename) - strlen (ext));
		else
			basename_noext = g_strdup (basename);
		new_basename = g_strconcat (basename_noext, new_ext, NULL);
		new_basename_uft8 = g_uri_unescape_string (new_basename, NULL);

		gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (data->dialog), new_basename_uft8);
		update_sensitivity_for_ext (data, new_ext);

		g_free (new_basename_uft8);
		g_free (new_basename);
		g_free (basename_noext);
	}
}


static char *
get_icon_name_for_type (const char *mime_type)
{
	char *name = NULL;

	if (mime_type != NULL) {
		char *s;

		name = g_strconcat ("mate-mime-", mime_type, NULL);
		for (s = name; *s; ++s)
			if (! g_ascii_isalpha (*s))
				*s = '-';
	}

	if ((name == NULL) || ! gtk_icon_theme_has_icon (gtk_icon_theme_get_default (), name)) {
		g_free (name);
		name = g_strdup ("package-x-generic");
	}

	return name;
}


static void
options_expander_unmap_cb (GtkWidget *widget,
			   gpointer   user_data)
{
	egg_file_format_chooser_emit_size_changed ((EggFileFormatChooser *) user_data);
}


static DlgNewData *
dlg_new_archive (FrWindow  *window,
		int        *supported_types,
		const char *default_name)
{
	DlgNewData    *data;
	GtkWidget     *n_new_button;
	GtkFileFilter *filter;
	/*char          *default_ext;*/
	int            i;

	data = g_new0 (DlgNewData, 1);

	data->builder = _gtk_builder_new_from_file ("new.ui");
	if (data->builder == NULL) {
		g_free (data);
		return NULL;
	}

	data->window = window;
	data->supported_types = supported_types;
	sort_mime_types_by_description (data->supported_types);

	/* Get the widgets. */

	data->dialog = _gtk_builder_get_widget (data->builder, "filechooserdialog");

	data->n_password_entry = _gtk_builder_get_widget (data->builder, "n_password_entry");
	data->n_password_label = _gtk_builder_get_widget (data->builder, "n_password_label");
	data->n_other_options_expander = _gtk_builder_get_widget (data->builder, "n_other_options_expander");
	data->n_encrypt_header_checkbutton = _gtk_builder_get_widget (data->builder, "n_encrypt_header_checkbutton");

	data->n_volume_checkbutton = _gtk_builder_get_widget (data->builder, "n_volume_checkbutton");
	data->n_volume_spinbutton = _gtk_builder_get_widget (data->builder, "n_volume_spinbutton");
	data->n_volume_box = _gtk_builder_get_widget (data->builder, "n_volume_box");

	n_new_button = _gtk_builder_get_widget (data->builder, "n_new_button");

	/* Set widgets data. */

	gtk_dialog_set_default_response (GTK_DIALOG (data->dialog), GTK_RESPONSE_OK);
	gtk_file_chooser_set_current_folder_uri (GTK_FILE_CHOOSER (data->dialog), fr_window_get_open_default_dir (window));

	if (default_name != NULL) {
		gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (data->dialog), default_name);
		/*char *ext, *name_ext;

		ext = eel_mateconf_get_string (PREF_BATCH_ADD_DEFAULT_EXTENSION, ".tgz");
		name_ext = g_strconcat (default_name, ext, NULL);
		gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (data->dialog), name_ext);
		g_free (name_ext);
		g_free (ext);*/
	}

	filter = gtk_file_filter_new ();
	gtk_file_filter_set_name (filter, _("All archives"));
	for (i = 0; data->supported_types[i] != -1; i++)
		gtk_file_filter_add_mime_type (filter, mime_type_desc[data->supported_types[i]].mime_type);
	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (data->dialog), filter);
	gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (data->dialog), filter);

	filter = gtk_file_filter_new ();
	gtk_file_filter_set_name (filter, _("All files"));
	gtk_file_filter_add_pattern (filter, "*");
	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (data->dialog), filter);

	/**/

	gtk_button_set_use_stock (GTK_BUTTON (n_new_button), TRUE);
	gtk_button_set_label (GTK_BUTTON (n_new_button), FR_STOCK_CREATE_ARCHIVE);
	gtk_expander_set_expanded (GTK_EXPANDER (data->n_other_options_expander), FALSE);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->n_encrypt_header_checkbutton), eel_mateconf_get_boolean (PREF_ENCRYPT_HEADER, FALSE));
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (data->n_volume_spinbutton), (double) eel_mateconf_get_integer (PREF_BATCH_VOLUME_SIZE, 0) / MEGABYTE);

	/* format chooser */

	data->format_chooser = (EggFileFormatChooser *) egg_file_format_chooser_new ();
	for (i = 0; data->supported_types[i] != -1; i++) {
		int   idx = data->supported_types[i];
		char *exts[4];
		int   e;
		int   n_exts;
		char *icon_name;

		n_exts = 0;
		for (e = 0; (n_exts < 4) && file_ext_type[e].ext != NULL; e++) {
			if (strcmp (file_ext_type[e].ext, mime_type_desc[idx].default_ext) == 0)
				continue;
			if (strcmp (file_ext_type[e].mime_type, mime_type_desc[idx].mime_type) == 0)
				exts[n_exts++] = file_ext_type[e].ext;
		}
		while (n_exts < 4)
			exts[n_exts++] = NULL;

		/* g_print ("%s => %s, %s, %s, %s\n", mime_type_desc[idx].mime_type, exts[0], exts[1], exts[2], exts[3]); */

		icon_name = get_icon_name_for_type (mime_type_desc[idx].mime_type);
		egg_file_format_chooser_add_format (data->format_chooser,
						    0,
						    _(mime_type_desc[idx].name),
						    icon_name,
						    mime_type_desc[idx].default_ext,
						    exts[0],
						    exts[1],
						    exts[2],
						    exts[3],
						    NULL);

		g_free (icon_name);
	}
	egg_file_format_chooser_set_format (data->format_chooser, 0);
	gtk_widget_show (GTK_WIDGET (data->format_chooser));
	gtk_box_pack_start (GTK_BOX (GET_WIDGET ("format_chooser_box")), GTK_WIDGET (data->format_chooser), TRUE, TRUE, 0);

	/* Set the signals handlers. */

	/*g_signal_connect (G_OBJECT (data->dialog),
			  "response",
			  G_CALLBACK (new_file_response_cb),
			  data);*/

	g_signal_connect (G_OBJECT (data->dialog),
			  "destroy",
			  G_CALLBACK (destroy_cb),
			  data);

	/*
	g_signal_connect_swapped (G_OBJECT (cancel_button),
				  "clicked",
				  G_CALLBACK (gtk_widget_destroy),
				  G_OBJECT (data->dialog));
	g_signal_connect (G_OBJECT (add_button),
			  "clicked",
			  G_CALLBACK (add_clicked_cb),
			  data);*/

	/* FIXME g_signal_connect (G_OBJECT (data->n_archive_type_combo_box),
			  "changed",
			  G_CALLBACK (archive_type_combo_box_changed_cb),
			  data); */
	g_signal_connect (G_OBJECT (data->n_password_entry),
			  "changed",
			  G_CALLBACK (password_entry_changed_cb),
			  data);
	g_signal_connect (G_OBJECT (data->n_volume_checkbutton),
			  "toggled",
			  G_CALLBACK (volume_toggled_cb),
			  data);
	g_signal_connect (G_OBJECT (data->format_chooser),
			  "selection-changed",
			  G_CALLBACK (format_chooser_selection_changed_cb),
			  data);
	g_signal_connect_after (GET_WIDGET ("other_oprtions_alignment"),
				"unmap",
				G_CALLBACK (options_expander_unmap_cb),
				data->format_chooser);

	/* Run dialog. */

/*	default_ext = eel_mateconf_get_string (PREF_BATCH_ADD_DEFAULT_EXTENSION, DEFAULT_EXTENSION);
	update_archive_type_combo_box_from_ext (data, default_ext);
	g_free (default_ext);*/

	update_sensitivity (data);

	gtk_window_set_modal (GTK_WINDOW (data->dialog), TRUE);
	gtk_window_set_transient_for (GTK_WINDOW (data->dialog), GTK_WINDOW (data->window));
	/*gtk_window_present (GTK_WINDOW (data->dialog));*/

	return data;
}


DlgNewData *
dlg_new (FrWindow *window)
{
	DlgNewData *data;

	data = dlg_new_archive (window, create_type, NULL);
	gtk_window_set_title (GTK_WINDOW (data->dialog), C_("File", "New"));

	return data;
}


DlgNewData *
dlg_save_as (FrWindow   *window,
	     const char *default_name)
{
	DlgNewData *data;

	data = dlg_new_archive (window, save_type, default_name);
	gtk_window_set_title (GTK_WINDOW (data->dialog), C_("File", "Save"));

	return data;
}


const char *
dlg_new_data_get_password (DlgNewData *data)
{
	const char *password = NULL;
	int         idx;

	idx = get_archive_type (data);
	if (idx < 0)
		return NULL;

	if (mime_type_desc[idx].capabilities & FR_COMMAND_CAN_ENCRYPT)
		password = (char*) gtk_entry_get_text (GTK_ENTRY (data->n_password_entry));

	return password;
}


gboolean
dlg_new_data_get_encrypt_header (DlgNewData *data)
{
	gboolean encrypt_header = FALSE;
	int      idx;

	idx = get_archive_type (data);
	if (idx < 0)
		return FALSE;

	if (mime_type_desc[idx].capabilities & FR_COMMAND_CAN_ENCRYPT) {
		const char *password = gtk_entry_get_text (GTK_ENTRY (data->n_password_entry));
		if (password != NULL) {
			if (strcmp (password, "") != 0) {
				if (mime_type_desc[idx].capabilities & FR_COMMAND_CAN_ENCRYPT_HEADER)
					encrypt_header = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->n_encrypt_header_checkbutton));
			}
		}
	}

	return encrypt_header;
}


int
dlg_new_data_get_volume_size (DlgNewData *data)
{
	guint volume_size = 0;
	int   idx;

	idx = get_archive_type (data);
	if (idx < 0)
		return 0;

	if ((mime_type_desc[idx].capabilities & FR_COMMAND_CAN_CREATE_VOLUMES)
	    && gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->n_volume_checkbutton)))
	{
		double value;

		value = gtk_spin_button_get_value (GTK_SPIN_BUTTON (data->n_volume_spinbutton));
		volume_size = floor (value * MEGABYTE);

	}

	return volume_size;
}

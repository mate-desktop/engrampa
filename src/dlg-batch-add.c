/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  Engrampa
 *
 *  Copyright (C) 2001, 2003, 2004 Free Software Foundation, Inc.
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
#include <math.h>
#include <unistd.h>
#include <gio/gio.h>
#include <gtk/gtk.h>
#include "dlg-batch-add.h"
#include "file-utils.h"
#include "fr-init.h"
#include "fr-stock.h"
#include "fr-window.h"
#include "gtk-utils.h"
#include "glib-utils.h"
#include "preferences.h"
#include "typedefs.h"


#define ARCHIVE_ICON_SIZE (48)
#define BAD_CHARS "/\\*"
#define GET_WIDGET(x) (_gtk_builder_get_widget (data->builder, (x)))

static gboolean has_password = FALSE;
static gboolean can_encrypt_header = FALSE;


typedef struct {
	FrWindow   *window;
	GSettings *settings;
	GSettings *settings_general;
	GtkBuilder *builder;
	int        *supported_types;
	GtkWidget  *archive_type_combo_box;
	GList      *file_list;
	gboolean    add_clicked;
	const char *last_mime_type;
	gboolean    single_file;
} DialogData;


static const char *
get_ext (DialogData *data)
{
	int idx;

	idx = gtk_combo_box_get_active (GTK_COMBO_BOX (data->archive_type_combo_box));

	return mime_type_desc[data->supported_types[idx]].default_ext;
}


/* called when the main dialog is closed. */
static void
destroy_cb (GtkWidget  *widget,
	    DialogData *data)
{
	g_settings_set_string (data->settings, PREF_BATCH_ADD_DEFAULT_EXTENSION, get_ext (data));
	/*g_settings_set_boolean (data->settings, PREF_BATCH_ADD_OTHER_OPTIONS, data->add_clicked ? FALSE : gtk_expander_get_expanded (GTK_EXPANDER (GET_WIDGET ("a_other_options_expander"))));*/
	g_settings_set_boolean (data->settings_general, PREF_GENERAL_ENCRYPT_HEADER, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("a_encrypt_header_checkbutton"))));


	if (! data->add_clicked) {
		fr_window_pop_message (data->window);
		fr_window_stop_batch (data->window);
	}

	g_object_unref (data->builder);
	g_object_unref (data->settings_general);
	g_object_unref (data->settings);
	g_free (data);
}


static void
set_archive_options (DialogData *data)
{
	int idx;

	idx = gtk_combo_box_get_active (GTK_COMBO_BOX (data->archive_type_combo_box));
	if (mime_type_desc[data->supported_types[idx]].capabilities & FR_COMMAND_CAN_ENCRYPT) {
		const char *pwd;

		pwd = gtk_entry_get_text (GTK_ENTRY (GET_WIDGET ("a_password_entry")));
		if (pwd != NULL) {
			if (strcmp (pwd, "") != 0) {
				fr_window_set_password (data->window, pwd);
				if (mime_type_desc[data->supported_types[idx]].capabilities & FR_COMMAND_CAN_ENCRYPT_HEADER)
					fr_window_set_encrypt_header (data->window, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("a_encrypt_header_checkbutton"))));
			}
		}
	}

	if ((mime_type_desc[data->supported_types[idx]].capabilities & FR_COMMAND_CAN_CREATE_VOLUMES)
	    && gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("a_volume_checkbutton"))))
	{
		double value;
		int    size;

		value = gtk_spin_button_get_value (GTK_SPIN_BUTTON (GET_WIDGET ("a_volume_spinbutton")));
		size = floor (value * MEGABYTE);
		g_settings_set_int (data->settings, PREF_BATCH_ADD_VOLUME_SIZE, size);
		fr_window_set_volume_size (data->window, (guint) size);
	}
}


static void
help_clicked_cb (GtkWidget  *widget,
		 DialogData *data)
{
	show_help_dialog (GTK_WINDOW (GET_WIDGET ("dialog")), "engrampa-fmgr-add");
}


static void
add_clicked_cb (GtkWidget  *widget,
		DialogData *data)
{
	FrWindow   *window = data->window;
	char       *archive_name;
	char       *archive_dir;
	char       *archive_file;
	char       *tmp;
	const char *archive_ext;
	gboolean    do_not_add = FALSE;
	GError     *error = NULL;

	data->add_clicked = TRUE;

	/* Collect data */

	archive_name = g_uri_escape_string (gtk_entry_get_text (GTK_ENTRY (GET_WIDGET ("a_add_to_entry"))), NULL, FALSE);

	/* Check whether the user entered a valid archive name. */

	if ((archive_name == NULL) || (*archive_name == '\0')) {
		GtkWidget *d;

		d = _gtk_error_dialog_new (GTK_WINDOW (window),
					   GTK_DIALOG_DESTROY_WITH_PARENT,
					   NULL,
					   _("Could not create the archive"),
					   "%s",
					   _("You have to specify an archive name."));
		gtk_dialog_run (GTK_DIALOG (d));
		gtk_widget_destroy (GTK_WIDGET (d));
		g_free (archive_name);

		return;
	}
	else if (strchrs (archive_name, BAD_CHARS)) {
		GtkWidget *d;
		char      *utf8_name = g_filename_display_name (archive_name);

		d = _gtk_error_dialog_new (GTK_WINDOW (window),
					   GTK_DIALOG_DESTROY_WITH_PARENT,
					   NULL,
					   _("Could not create the archive"),
					   _("The name \"%s\" is not valid because it cannot contain the characters: %s\n\n%s"),
					   utf8_name,
					   BAD_CHARS,
					   _("Please use a different name."));
		gtk_dialog_run (GTK_DIALOG (d));
		gtk_widget_destroy (GTK_WIDGET (d));

		g_free (utf8_name);
		g_free (archive_name);

		return;
	}

	/* Check directory existence. */

	archive_dir = gtk_file_chooser_get_uri (GTK_FILE_CHOOSER (GET_WIDGET ("a_location_filechooserbutton")));
	if (archive_dir == NULL) {
		g_free (archive_dir);
		g_free (archive_name);
		return;
	}

	if (! check_permissions (archive_dir, R_OK|W_OK|X_OK)) {
		GtkWidget  *d;

		d = _gtk_error_dialog_new (GTK_WINDOW (window),
					   GTK_DIALOG_DESTROY_WITH_PARENT,
					   NULL,
					   _("Could not create the archive"),
					   "%s",
					   _("You don't have the right permissions to create an archive in the destination folder."));
		gtk_dialog_run (GTK_DIALOG (d));
		gtk_widget_destroy (GTK_WIDGET (d));

		g_free (archive_dir);
		g_free (archive_name);
		return;
	}

	if (! uri_is_dir (archive_dir)) {
		GtkWidget *d;
		int        r;
		char      *folder_name;
		char      *msg;

		folder_name = g_filename_display_name (archive_dir);
		msg = g_strdup_printf (_("Destination folder \"%s\" does not exist.\n\nDo you want to create it?"), folder_name);
		g_free (folder_name);

		d = _gtk_message_dialog_new (GTK_WINDOW (GET_WIDGET ("dialog")),
					     GTK_DIALOG_MODAL,
					     "dialog-question",
					     msg,
					     NULL,
					     "gtk-cancel", GTK_RESPONSE_CANCEL,
					     _("Create _Folder"), GTK_RESPONSE_YES,
					     NULL);

		gtk_dialog_set_default_response (GTK_DIALOG (d), GTK_RESPONSE_YES);
		r = gtk_dialog_run (GTK_DIALOG (d));
		gtk_widget_destroy (GTK_WIDGET (d));

		g_free (msg);

		do_not_add = (r != GTK_RESPONSE_YES);
	}

	if (! do_not_add && ! ensure_dir_exists (archive_dir, 0755, &error)) {
		GtkWidget  *d;

		d = _gtk_error_dialog_new (GTK_WINDOW (window),
					   GTK_DIALOG_DESTROY_WITH_PARENT,
					   NULL,
					   _("Could not create the archive"),
					   _("Could not create the destination folder: %s."),
					   error->message);
		gtk_dialog_run (GTK_DIALOG (d));
		gtk_widget_destroy (GTK_WIDGET (d));

		g_error_free (error);
		g_free (archive_dir);
		g_free (archive_name);
		return;
	}

	if (do_not_add) {
		GtkWidget *d;

		d = _gtk_message_dialog_new (GTK_WINDOW (window),
					     GTK_DIALOG_DESTROY_WITH_PARENT,
					     "dialog-warning",
					     _("Archive not created"),
					     NULL,
					     "gtk-ok", GTK_RESPONSE_OK,
					     NULL);
		gtk_dialog_set_default_response (GTK_DIALOG (d), GTK_RESPONSE_OK);
		gtk_dialog_run (GTK_DIALOG (d));
		gtk_widget_destroy (GTK_WIDGET (d));

		g_free (archive_dir);
		g_free (archive_name);

		return;
	}

	/**/

	archive_ext = get_ext (data);
	tmp = archive_name;
	archive_name = g_strconcat (tmp, archive_ext, NULL);
	g_free (tmp);
	archive_file = g_strconcat (archive_dir, "/", archive_name, NULL);

	if (uri_is_dir (archive_file)) {
		GtkWidget  *d;

		d = _gtk_error_dialog_new (GTK_WINDOW (window),
					   GTK_DIALOG_DESTROY_WITH_PARENT,
					   NULL,
					   _("Could not create the archive"),
					   "%s",
					   _("You have to specify an archive name."));
		gtk_dialog_run (GTK_DIALOG (d));
		gtk_widget_destroy (GTK_WIDGET (d));

		g_free (archive_name);
		g_free (archive_dir);
		g_free (archive_file);

		return;
	}

	if (uri_exists (archive_file)) {
		GtkWidget *d;
		int        r;

		d = _gtk_message_dialog_new (GTK_WINDOW (GET_WIDGET ("dialog")),
					     GTK_DIALOG_MODAL,
					     "dialog-question",
					     _("The archive is already present.  Do you want to overwrite it?"),
					     NULL,
					     "gtk-no", GTK_RESPONSE_NO,
					     _("_Overwrite"), GTK_RESPONSE_YES,
					     NULL);

		gtk_dialog_set_default_response (GTK_DIALOG (d), GTK_RESPONSE_YES);
		r = gtk_dialog_run (GTK_DIALOG (d));
		gtk_widget_destroy (GTK_WIDGET (d));

		if (r == GTK_RESPONSE_YES) {
			GFile  *file;
			GError *err = NULL;

			file = g_file_new_for_uri (archive_file);
			g_file_delete (file, NULL, &err);
			if (err != NULL) {
				g_warning ("Failed to delete file %s: %s",
					   archive_file,
					   err->message);
				g_clear_error (&err);
			}
			g_object_unref (file);
		}
		else {
			g_free (archive_name);
			g_free (archive_dir);
			g_free (archive_file);
			return;
		}
	}
	set_archive_options (data);
	gtk_widget_destroy (GET_WIDGET ("dialog"));

	fr_window_archive_new (window, archive_file);

	g_free (archive_name);
	g_free (archive_dir);
	g_free (archive_file);
}


static void
update_sensitivity_for_mime_type (DialogData *data,
				  const char *mime_type)
{
	int i;

	if (mime_type == NULL) {
		gtk_widget_set_sensitive (GET_WIDGET ("a_password_entry"), FALSE);
		gtk_widget_set_sensitive (GET_WIDGET ("a_password_label"), FALSE);
		gtk_widget_set_sensitive (GET_WIDGET ("a_encrypt_header_checkbutton"), FALSE);
		gtk_toggle_button_set_inconsistent (GTK_TOGGLE_BUTTON (GET_WIDGET ("a_encrypt_header_checkbutton")), TRUE);
		gtk_widget_set_sensitive (GET_WIDGET ("a_volume_box"), FALSE);
		return;
	}

	for (i = 0; mime_type_desc[i].mime_type != NULL; i++) {
		if (strcmp (mime_type_desc[i].mime_type, mime_type) == 0) {
			gboolean sensitive;

			sensitive = mime_type_desc[i].capabilities & FR_COMMAND_CAN_ENCRYPT;
			gtk_widget_set_sensitive (GET_WIDGET ("a_password_entry"), sensitive);
			gtk_widget_set_sensitive (GET_WIDGET ("a_password_label"), sensitive);

			sensitive = mime_type_desc[i].capabilities & FR_COMMAND_CAN_ENCRYPT_HEADER;
			can_encrypt_header = sensitive;
			gtk_widget_set_sensitive (GET_WIDGET ("a_encrypt_header_checkbutton"), sensitive ? has_password : FALSE);
			gtk_toggle_button_set_inconsistent (GTK_TOGGLE_BUTTON (GET_WIDGET ("a_encrypt_header_checkbutton")), sensitive ? (!has_password) : TRUE);

			sensitive = mime_type_desc[i].capabilities & FR_COMMAND_CAN_CREATE_VOLUMES;
			gtk_widget_set_sensitive (GET_WIDGET ("a_volume_box"), sensitive);

			break;
		}
	}
}


static void
archive_type_combo_box_changed_cb (GtkComboBox *combo_box,
				   DialogData  *data)
{
	const char *mime_type;
	int         idx = gtk_combo_box_get_active (combo_box);
	GdkPixbuf  *icon;

	mime_type = mime_type_desc[data->supported_types[idx]].mime_type;

	icon = get_mime_type_pixbuf (mime_type, ARCHIVE_ICON_SIZE, NULL);
	if (icon != NULL) {
		gtk_image_set_from_pixbuf (GTK_IMAGE (GET_WIDGET ("archive_icon_image")), icon);
		g_object_unref (icon);
	}

	update_sensitivity_for_mime_type (data, mime_type);
}


static void
update_archive_type_combo_box_from_ext (DialogData  *data,
					const char  *ext)
{
	int idx = 0;
	int i;

	if (ext == NULL) {
		gtk_combo_box_set_active (GTK_COMBO_BOX (data->archive_type_combo_box), 0);
		return;
	}

	for (i = 0; data->supported_types[i] != -1; i++)
		if (strcmp (ext, mime_type_desc[data->supported_types[i]].default_ext) == 0) {
			idx = i;
			break;
		}

	gtk_combo_box_set_active (GTK_COMBO_BOX (data->archive_type_combo_box), idx);
}


static void
update_sensitivity (DialogData *data)
{
	const char *password;

	gtk_widget_set_sensitive (GET_WIDGET ("a_volume_spinbutton"),
				  gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("a_volume_checkbutton"))));

	password = gtk_entry_get_text (GTK_ENTRY (GET_WIDGET ("a_password_entry")));
	has_password = (password != NULL) && (*password != '\0');
	gtk_toggle_button_set_inconsistent (GTK_TOGGLE_BUTTON (GET_WIDGET ("a_encrypt_header_checkbutton")), can_encrypt_header ? (!has_password) : TRUE);
	gtk_widget_set_sensitive (GET_WIDGET ("a_encrypt_header_checkbutton"), can_encrypt_header ? has_password : FALSE);
}


static void
password_entry_notify_text_cb (GObject    *object,
			       GParamSpec *spec,
			       gpointer    user_data)
{
	update_sensitivity ((DialogData *) user_data);
}


static void
volume_toggled_cb (GtkToggleButton *toggle_button,
		   gpointer         user_data)
{
	update_sensitivity ((DialogData *) user_data);
}


void
dlg_batch_add_files (FrWindow *window,
		     GList    *file_list)
{
	DialogData   *data;
	GtkSizeGroup *size_group;
	char         *automatic_name = NULL;
	char         *default_ext;
	const char   *first_filename;
	char         *parent;
	int           i;

	if (file_list == NULL)
		return;

	data = g_new0 (DialogData, 1);
	data->settings = g_settings_new (ENGRAMPA_SCHEMA_BATCH_ADD);
	data->settings_general = g_settings_new (ENGRAMPA_SCHEMA_GENERAL);

	data->builder = _gtk_builder_new_from_resource ("batch-add-files.ui");
	if (data->builder == NULL) {
		g_free (data);
		return;
	}

	data->window = window;
	data->file_list = file_list;
	data->single_file = ((file_list->next == NULL) && uri_is_file ((char*) file_list->data));
	data->add_clicked = FALSE;

	/* Set widgets data. */

	size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
	gtk_size_group_add_widget (size_group, GET_WIDGET ("a_archive_label"));
	gtk_size_group_add_widget (size_group, GET_WIDGET ("a_location_label"));
	gtk_size_group_add_widget (size_group, GET_WIDGET ("a_password_label"));

	gtk_button_set_use_stock (GTK_BUTTON (GET_WIDGET ("a_add_button")), TRUE);
	gtk_button_set_label (GTK_BUTTON (GET_WIDGET ("a_add_button")), FR_STOCK_CREATE_ARCHIVE);
	gtk_expander_set_expanded (GTK_EXPANDER (GET_WIDGET ("a_other_options_expander")), FALSE /*g_settings_get_boolean (data->settings, PREF_BATCH_ADD_OTHER_OPTIONS)*/);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("a_encrypt_header_checkbutton")), g_settings_get_boolean (data->settings_general, PREF_GENERAL_ENCRYPT_HEADER));
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (GET_WIDGET ("a_volume_spinbutton")), (double) g_settings_get_int (data->settings, PREF_BATCH_ADD_VOLUME_SIZE) / MEGABYTE);


	first_filename = (char*) file_list->data;
	parent = remove_level_from_path (first_filename);

	if (file_list->next == NULL)
		automatic_name = g_uri_unescape_string (file_name_from_path ((char*) file_list->data), NULL);
	else {
		automatic_name = g_uri_unescape_string (file_name_from_path (parent), NULL);
		if ((automatic_name == NULL) || (automatic_name[0] == '\0')) {
			g_free (automatic_name);
			automatic_name = g_uri_unescape_string (file_name_from_path (first_filename), NULL);
		}
	}

	_gtk_entry_set_filename_text (GTK_ENTRY (GET_WIDGET ("a_add_to_entry")), automatic_name);
	g_free (automatic_name);

	if (check_permissions (parent, R_OK | W_OK))
		gtk_file_chooser_set_current_folder_uri (GTK_FILE_CHOOSER (GET_WIDGET ("a_location_filechooserbutton")), parent);
	else
		gtk_file_chooser_set_current_folder_uri (GTK_FILE_CHOOSER (GET_WIDGET ("a_location_filechooserbutton")), get_home_uri ());
	g_free (parent);

	/* archive type combobox */

	data->archive_type_combo_box = gtk_combo_box_text_new ();
	if (data->single_file)
		data->supported_types = single_file_save_type;
	else
		data->supported_types = save_type;
	sort_mime_types_by_extension (data->supported_types);

	for (i = 0; data->supported_types[i] != -1; i++)
		gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (data->archive_type_combo_box),
						mime_type_desc[data->supported_types[i]].default_ext);

	gtk_box_pack_start (GTK_BOX (GET_WIDGET ("a_archive_type_box")), data->archive_type_combo_box, TRUE, TRUE, 0);
	gtk_widget_show_all (GET_WIDGET ("a_archive_type_box"));

	/* Set the signals handlers. */

	g_signal_connect (G_OBJECT (GET_WIDGET ("dialog")),
			  "destroy",
			  G_CALLBACK (destroy_cb),
			  data);
	g_signal_connect_swapped (GET_WIDGET ("a_cancel_button"),
				  "clicked",
				  G_CALLBACK (gtk_widget_destroy),
				  G_OBJECT (GET_WIDGET ("dialog")));
	g_signal_connect (G_OBJECT (GET_WIDGET ("a_add_button")),
			  "clicked",
			  G_CALLBACK (add_clicked_cb),
			  data);
	g_signal_connect (G_OBJECT (GET_WIDGET ("a_help_button")),
			  "clicked",
			  G_CALLBACK (help_clicked_cb),
			  data);
	g_signal_connect (G_OBJECT (data->archive_type_combo_box),
			  "changed",
			  G_CALLBACK (archive_type_combo_box_changed_cb),
			  data);
	g_signal_connect (GET_WIDGET ("a_password_entry"),
			  "notify::text",
			  G_CALLBACK (password_entry_notify_text_cb),
			  data);
	g_signal_connect (GET_WIDGET ("a_volume_checkbutton"),
			  "toggled",
			  G_CALLBACK (volume_toggled_cb),
			  data);

	/* Run dialog. */

	default_ext = g_settings_get_string (data->settings, PREF_BATCH_ADD_DEFAULT_EXTENSION);
	update_archive_type_combo_box_from_ext (data, default_ext);
	g_free (default_ext);

	gtk_widget_grab_focus (GET_WIDGET ("a_add_to_entry"));
	gtk_editable_select_region (GTK_EDITABLE (GET_WIDGET ("a_add_to_entry")),
				    0, -1);

	update_sensitivity (data);

	gtk_window_set_modal (GTK_WINDOW (GET_WIDGET ("dialog")), FALSE);
	gtk_window_present (GTK_WINDOW (GET_WIDGET ("dialog")));
}

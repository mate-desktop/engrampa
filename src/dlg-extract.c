/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  File-Roller
 *
 *  Copyright (C) 2001, 2003, 2004, 2005 Free Software Foundation, Inc.
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
#include <unistd.h>
#include "file-utils.h"
#include "fr-stock.h"
#include "main.h"
#include "gtk-utils.h"
#include "fr-window.h"
#include "typedefs.h"
#include "mateconf-utils.h"


typedef struct {
	FrWindow     *window;
	GList        *selected_files;
	char         *base_dir_for_selection;

	GtkWidget    *dialog;

	GtkWidget    *e_main_vbox;
	GtkWidget    *e_all_radiobutton;
	GtkWidget    *e_selected_radiobutton;
	GtkWidget    *e_files_radiobutton;
	GtkWidget    *e_files_entry;
	GtkWidget    *e_recreate_dir_checkbutton;
	GtkWidget    *e_overwrite_checkbutton;
	GtkWidget    *e_not_newer_checkbutton;

	gboolean      extract_clicked;
} DialogData;


/* called when the main dialog is closed. */
static void
destroy_cb (GtkWidget  *widget,
	    DialogData *data)
{
	if (! data->extract_clicked) {
		fr_window_pop_message (data->window);
		fr_window_stop_batch (data->window);
	}
	path_list_free (data->selected_files);
	g_free (data->base_dir_for_selection);
	g_free (data);
}


static int
extract_cb (GtkWidget   *w,
	    DialogData  *data)
{
	FrWindow   *window = data->window;
	gboolean    do_not_extract = FALSE;
	char       *extract_to_dir;
	gboolean    overwrite;
	gboolean    skip_newer;
	gboolean    selected_files;
	gboolean    pattern_files;
	gboolean    junk_paths;
	GList      *file_list;
	char       *base_dir = NULL;
	GError     *error = NULL;

	data->extract_clicked = TRUE;

	/* collect extraction options. */

	extract_to_dir = gtk_file_chooser_get_uri (GTK_FILE_CHOOSER (data->dialog));

	/* check directory existence. */

	if (! uri_is_dir (extract_to_dir)) {
		if (! ForceDirectoryCreation) {
			GtkWidget *d;
			int        r;
			char      *folder_name;
			char      *msg;

			folder_name = g_filename_display_name (extract_to_dir);
			msg = g_strdup_printf (_("Destination folder \"%s\" does not exist.\n\nDo you want to create it?"), folder_name);
			g_free (folder_name);

			d = _gtk_message_dialog_new (GTK_WINDOW (data->dialog),
						     GTK_DIALOG_MODAL,
						     GTK_STOCK_DIALOG_QUESTION,
						     msg,
						     NULL,
						     GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
						     _("Create _Folder"), GTK_RESPONSE_YES,
						     NULL);

			gtk_dialog_set_default_response (GTK_DIALOG (d), GTK_RESPONSE_YES);
			r = gtk_dialog_run (GTK_DIALOG (d));
			gtk_widget_destroy (GTK_WIDGET (d));

			g_free (msg);

			if (r != GTK_RESPONSE_YES)
				do_not_extract = TRUE;
		}

		if (! do_not_extract && ! ensure_dir_exists (extract_to_dir, 0755, &error)) {
			GtkWidget  *d;

			d = _gtk_error_dialog_new (GTK_WINDOW (window),
						   GTK_DIALOG_DESTROY_WITH_PARENT,
						   NULL,
						   _("Extraction not performed"),
						   _("Could not create the destination folder: %s."),
						   error->message);
			gtk_dialog_run (GTK_DIALOG (d));
			gtk_widget_destroy (GTK_WIDGET (d));

			g_error_free (error);

			return FALSE;
		}
	}

	if (do_not_extract) {
		GtkWidget *d;

		d = _gtk_message_dialog_new (GTK_WINDOW (window),
					     GTK_DIALOG_DESTROY_WITH_PARENT,
					     GTK_STOCK_DIALOG_WARNING,
					     _("Extraction not performed"),
					     NULL,
					     GTK_STOCK_OK, GTK_RESPONSE_OK,
					     NULL);
		gtk_dialog_set_default_response (GTK_DIALOG (d), GTK_RESPONSE_OK);
		gtk_dialog_run (GTK_DIALOG (d));
		gtk_widget_destroy (GTK_WIDGET (d));

		if (fr_window_is_batch_mode (data->window))
			gtk_widget_destroy (data->dialog);

		return FALSE;
	}

	/* check extraction directory permissions. */

	if (uri_is_dir (extract_to_dir)
	    && ! check_permissions (extract_to_dir, R_OK | W_OK)) 
	{
		GtkWidget *d;
		char      *utf8_path;

		utf8_path = g_filename_display_name (extract_to_dir);

		d = _gtk_error_dialog_new (GTK_WINDOW (window),
					   GTK_DIALOG_DESTROY_WITH_PARENT,
					   NULL,
					   _("Extraction not performed"),
					   _("You don't have the right permissions to extract archives in the folder \"%s\""),
					   utf8_path);
		gtk_dialog_run (GTK_DIALOG (d));
		gtk_widget_destroy (GTK_WIDGET (d));

		g_free (utf8_path);
		g_free (extract_to_dir);

		return FALSE;
	}

	fr_window_set_extract_default_dir (window, extract_to_dir, TRUE);

	overwrite = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->e_overwrite_checkbutton));
	skip_newer = ! gtk_toggle_button_get_inconsistent (GTK_TOGGLE_BUTTON (data->e_not_newer_checkbutton)) && gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->e_not_newer_checkbutton));
	junk_paths = ! gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->e_recreate_dir_checkbutton));

	eel_mateconf_set_boolean (PREF_EXTRACT_OVERWRITE, overwrite);
	if (!gtk_toggle_button_get_inconsistent (GTK_TOGGLE_BUTTON (data->e_not_newer_checkbutton)))
		eel_mateconf_set_boolean (PREF_EXTRACT_SKIP_NEWER, skip_newer);
	eel_mateconf_set_boolean (PREF_EXTRACT_RECREATE_FOLDERS, !junk_paths);

	selected_files = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->e_selected_radiobutton));
	pattern_files = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->e_files_radiobutton));

	/* create the file list. */

	file_list = NULL;

	if (selected_files) {
		file_list = data->selected_files;
		data->selected_files = NULL;       /* do not the list when destroying the dialog. */
	}
	else if (pattern_files) {
		const char *pattern;

		pattern = gtk_entry_get_text (GTK_ENTRY (data->e_files_entry));
		file_list = fr_window_get_file_list_pattern (window, pattern);
		if (file_list == NULL) {
			gtk_widget_destroy (data->dialog);
			g_free (extract_to_dir);
			return FALSE;
		}
	}

	if (selected_files) {
		base_dir = data->base_dir_for_selection;
		data->base_dir_for_selection = NULL;
	}
	else
		base_dir = NULL;

	/* close the dialog. */

	gtk_widget_destroy (data->dialog);

	/* extract ! */

	fr_window_archive_extract (window,
				   file_list,
				   extract_to_dir,
				   base_dir,
				   skip_newer,
				   overwrite,
				   junk_paths,
				   TRUE);

	path_list_free (file_list);
	g_free (extract_to_dir);
	g_free (base_dir);

	return TRUE;
}


static int
file_sel_response_cb (GtkWidget    *widget,
		      int           response,
		      DialogData   *data)
{
	if ((response == GTK_RESPONSE_CANCEL) || (response == GTK_RESPONSE_DELETE_EVENT)) {
		gtk_widget_destroy (data->dialog);
		return TRUE;
	}

	if (response == GTK_RESPONSE_HELP) {
		show_help_dialog (GTK_WINDOW (data->dialog), "file-roller-extract-options");
		return TRUE;
	}

	if (response == GTK_RESPONSE_OK)
		return extract_cb (widget, data);

	return FALSE;
}


static void
files_entry_changed_cb (GtkWidget  *widget,
			DialogData *data)
{
	if (! gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->e_files_radiobutton)))
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->e_files_radiobutton), TRUE);
}


static void
overwrite_toggled_cb (GtkToggleButton *button,
		      DialogData      *data)
{
	gboolean active = gtk_toggle_button_get_active (button);
	gtk_toggle_button_set_inconsistent (GTK_TOGGLE_BUTTON (data->e_not_newer_checkbutton), !active);
	gtk_widget_set_sensitive (data->e_not_newer_checkbutton, active);
}


static void
set_bold_label (GtkWidget  *label,
		const char *label_txt)
{
	char *bold_label;

	bold_label = g_strconcat ("<b>", label_txt, "</b>", NULL);
	gtk_label_set_markup (GTK_LABEL (label), bold_label);
	g_free (bold_label);
}


static GtkWidget *
create_extra_widget (DialogData *data)
{
	GtkWidget *vbox1;
	GtkWidget *hbox28;
	GtkWidget *vbox19;
	GtkWidget *e_files_label;
	GtkWidget *hbox29;
	GtkWidget *label47;
	GtkWidget *table1;
	GSList    *e_files_radiobutton_group = NULL;
	GtkWidget *vbox20;
	GtkWidget *e_actions_label;
	GtkWidget *hbox30;
	GtkWidget *label48;
	GtkWidget *vbox15;

	vbox1 = gtk_vbox_new (FALSE, 6);
	gtk_container_set_border_width (GTK_CONTAINER (vbox1), 0);

	hbox28 = gtk_hbox_new (FALSE, 12);
	gtk_box_pack_start (GTK_BOX (vbox1), hbox28, TRUE, TRUE, 0);

	vbox19 = gtk_vbox_new (FALSE, 6);
	gtk_box_pack_start (GTK_BOX (hbox28), vbox19, TRUE, TRUE, 0);

	e_files_label = gtk_label_new ("");
	set_bold_label (e_files_label, _("Extract"));
	gtk_box_pack_start (GTK_BOX (vbox19), e_files_label, FALSE, FALSE, 0);
	gtk_label_set_justify (GTK_LABEL (e_files_label), GTK_JUSTIFY_LEFT);
	gtk_misc_set_alignment (GTK_MISC (e_files_label), 0, 0.5);

	hbox29 = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox19), hbox29, TRUE, TRUE, 0);

	label47 = gtk_label_new ("    ");
	gtk_box_pack_start (GTK_BOX (hbox29), label47, FALSE, FALSE, 0);
	gtk_label_set_justify (GTK_LABEL (label47), GTK_JUSTIFY_LEFT);

	table1 = gtk_table_new (3, 2, FALSE);
	gtk_box_pack_start (GTK_BOX (hbox29), table1, TRUE, TRUE, 0);
	gtk_table_set_row_spacings (GTK_TABLE (table1), 6);
	gtk_table_set_col_spacings (GTK_TABLE (table1), 6);

	data->e_files_radiobutton = gtk_radio_button_new_with_mnemonic (NULL, _("_Files:"));
	gtk_table_attach (GTK_TABLE (table1), data->e_files_radiobutton, 0, 1, 2, 3,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (0), 0, 0);
	gtk_radio_button_set_group (GTK_RADIO_BUTTON (data->e_files_radiobutton), e_files_radiobutton_group);
	e_files_radiobutton_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (data->e_files_radiobutton));

	data->e_files_entry = gtk_entry_new ();
	gtk_table_attach (GTK_TABLE (table1), data->e_files_entry, 1, 2, 2, 3,
			  (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
			  (GtkAttachOptions) (0), 0, 0);
	gtk_widget_set_tooltip_text (data->e_files_entry, _("example: *.txt; *.doc"));
	gtk_entry_set_activates_default (GTK_ENTRY (data->e_files_entry), TRUE);

	data->e_all_radiobutton = gtk_radio_button_new_with_mnemonic (NULL, _("_All files"));
	gtk_table_attach (GTK_TABLE (table1), data->e_all_radiobutton, 0, 2, 0, 1,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (0), 0, 0);
	gtk_radio_button_set_group (GTK_RADIO_BUTTON (data->e_all_radiobutton), e_files_radiobutton_group);
	e_files_radiobutton_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (data->e_all_radiobutton));

	data->e_selected_radiobutton = gtk_radio_button_new_with_mnemonic (NULL, _("_Selected files"));
	gtk_table_attach (GTK_TABLE (table1), data->e_selected_radiobutton, 0, 2, 1, 2,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (0), 0, 0);
	gtk_radio_button_set_group (GTK_RADIO_BUTTON (data->e_selected_radiobutton), e_files_radiobutton_group);
	e_files_radiobutton_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (data->e_selected_radiobutton));

	vbox20 = gtk_vbox_new (FALSE, 6);
	gtk_box_pack_start (GTK_BOX (hbox28), vbox20, TRUE, TRUE, 0);

	e_actions_label = gtk_label_new ("");
	set_bold_label (e_actions_label, _("Actions"));
	gtk_box_pack_start (GTK_BOX (vbox20), e_actions_label, FALSE, FALSE, 0);
	gtk_label_set_use_markup (GTK_LABEL (e_actions_label), TRUE);
	gtk_label_set_justify (GTK_LABEL (e_actions_label), GTK_JUSTIFY_LEFT);
	gtk_misc_set_alignment (GTK_MISC (e_actions_label), 0, 0.5);

	hbox30 = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox20), hbox30, TRUE, TRUE, 0);

	label48 = gtk_label_new ("    ");
	gtk_box_pack_start (GTK_BOX (hbox30), label48, FALSE, FALSE, 0);
	gtk_label_set_justify (GTK_LABEL (label48), GTK_JUSTIFY_LEFT);

	vbox15 = gtk_vbox_new (FALSE, 6);
	gtk_box_pack_start (GTK_BOX (hbox30), vbox15, TRUE, TRUE, 0);

	data->e_recreate_dir_checkbutton = gtk_check_button_new_with_mnemonic (_("Re-crea_te folders"));
	gtk_box_pack_start (GTK_BOX (vbox15), data->e_recreate_dir_checkbutton, FALSE, FALSE, 0);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->e_recreate_dir_checkbutton), TRUE);

	data->e_overwrite_checkbutton = gtk_check_button_new_with_mnemonic (_("Over_write existing files"));
	gtk_box_pack_start (GTK_BOX (vbox15), data->e_overwrite_checkbutton, FALSE, FALSE, 0);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->e_overwrite_checkbutton), TRUE);

	data->e_not_newer_checkbutton = gtk_check_button_new_with_mnemonic (_("Do not e_xtract older files"));
	gtk_box_pack_start (GTK_BOX (vbox15), data->e_not_newer_checkbutton, FALSE, FALSE, 0);

	gtk_widget_show_all (vbox1);

	return vbox1;
}


static void
dlg_extract__common (FrWindow *window,
	             GList    *selected_files,
	             char     *base_dir_for_selection)
{
	DialogData *data;
	GtkWidget  *file_sel;

	data = g_new0 (DialogData, 1);

	data->window = window;
	data->selected_files = selected_files;
	data->base_dir_for_selection = base_dir_for_selection;
	data->extract_clicked = FALSE;

	data->dialog = file_sel =
		gtk_file_chooser_dialog_new (_("Extract"),
					     GTK_WINDOW (data->window),
					     GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
					     GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					     FR_STOCK_EXTRACT, GTK_RESPONSE_OK,
					     GTK_STOCK_HELP, GTK_RESPONSE_HELP,
					     NULL);

	gtk_window_set_default_size (GTK_WINDOW (file_sel), 530, 510);

	gtk_file_chooser_set_select_multiple (GTK_FILE_CHOOSER (file_sel), FALSE);
	gtk_file_chooser_set_local_only (GTK_FILE_CHOOSER (file_sel), FALSE);
	gtk_dialog_set_default_response (GTK_DIALOG (file_sel), GTK_RESPONSE_OK);

	gtk_file_chooser_set_extra_widget (GTK_FILE_CHOOSER (file_sel), create_extra_widget (data));

	/* Set widgets data. */

	gtk_file_chooser_set_current_folder_uri (GTK_FILE_CHOOSER (file_sel), fr_window_get_extract_default_dir (window));

	if (data->selected_files != NULL)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->e_selected_radiobutton), TRUE);
	else {
		gtk_widget_set_sensitive (data->e_selected_radiobutton, FALSE);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->e_all_radiobutton), TRUE);
	}

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->e_overwrite_checkbutton), eel_mateconf_get_boolean (PREF_EXTRACT_OVERWRITE, FALSE));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->e_not_newer_checkbutton), eel_mateconf_get_boolean (PREF_EXTRACT_SKIP_NEWER, FALSE));
	if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->e_overwrite_checkbutton))) {
		gtk_toggle_button_set_inconsistent (GTK_TOGGLE_BUTTON (data->e_not_newer_checkbutton), TRUE);
		gtk_widget_set_sensitive (data->e_not_newer_checkbutton, FALSE);
	}

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->e_recreate_dir_checkbutton), eel_mateconf_get_boolean (PREF_EXTRACT_RECREATE_FOLDERS, TRUE));

	/* Set the signals handlers. */

	g_signal_connect (G_OBJECT (data->dialog),
			  "destroy",
			  G_CALLBACK (destroy_cb),
			  data);

	g_signal_connect (G_OBJECT (file_sel),
			  "response",
			  G_CALLBACK (file_sel_response_cb),
			  data);

	g_signal_connect (G_OBJECT (data->e_overwrite_checkbutton),
			  "toggled",
			  G_CALLBACK (overwrite_toggled_cb),
			  data);
	g_signal_connect (G_OBJECT (data->e_files_entry),
			  "changed",
			  G_CALLBACK (files_entry_changed_cb),
			  data);

	/* Run dialog. */

	gtk_window_set_modal (GTK_WINDOW (file_sel),TRUE);
	gtk_widget_show (file_sel);
}


void
dlg_extract (GtkWidget *widget,
	     gpointer   callback_data)
{
	FrWindow *window = callback_data;
	GList    *files;
	char     *base_dir;

	files = fr_window_get_selection (window, FALSE, &base_dir);
	dlg_extract__common (window, files, base_dir);
}


void
dlg_extract_folder_from_sidebar (GtkWidget *widget,
	     			 gpointer   callback_data)
{
	FrWindow *window = callback_data;
	GList    *files;
	char     *base_dir;

	files = fr_window_get_selection (window, TRUE, &base_dir);
	dlg_extract__common (window, files, base_dir);
}

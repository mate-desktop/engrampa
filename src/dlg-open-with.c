/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  File-Roller
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
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307, USA.
 */

#include <config.h>
#include <string.h>

#include <gtk/gtk.h>
#include "file-utils.h"
#include "mateconf-utils.h"
#include "glib-utils.h"
#include "gtk-utils.h"
#include "main.h"
#include "fr-window.h"


#define TEMP_DOCS  "temp_docs"

enum { ICON_COLUMN, TEXT_COLUMN, DATA_COLUMN, N_COLUMNS };

typedef struct {
	FrWindow     *window;
	GtkBuilder *builder;

	GtkWidget    *dialog;
	GtkWidget    *o_app_tree_view;
	GtkWidget    *o_recent_tree_view;
	GtkWidget    *o_app_entry;
	GtkWidget    *o_del_button;
	GtkWidget    *ok_button;

	GList        *app_list;
	GList        *file_list;

	GtkTreeModel *app_model;
	GtkTreeModel *recent_model;
	
	GtkWidget    *last_clicked_list;
} DialogData;


/* called when the main dialog is closed. */
static void
open_with__destroy_cb (GtkWidget  *widget,
		       DialogData *data)
{
	g_object_unref (G_OBJECT (data->builder));

	if (data->app_list != NULL) 
		g_list_free (data->app_list);

	if (data->file_list != NULL)
		path_list_free (data->file_list);

	g_free (data);
}


static void
open_cb (GtkWidget *widget,
	 gpointer   callback_data)
{
	DialogData  *data = callback_data;
	const char  *application;
	gboolean     present = FALSE;
	char        *command = NULL;
	GList       *scan;
	GSList      *sscan, *editors;

	application = gtk_entry_get_text (GTK_ENTRY (data->o_app_entry));

	for (scan = data->app_list; scan; scan = scan->next) {
		GAppInfo *app = scan->data;
		if (strcmp (g_app_info_get_executable (app), application) == 0) {
			fr_window_open_files_with_application (data->window, data->file_list, app);
			gtk_widget_destroy (data->dialog);
			return;
		}
	}

	/* add the command to the editors list if not already present. */

	editors = eel_mateconf_get_string_list (PREF_EDIT_EDITORS);
	for (sscan = editors; sscan && ! present; sscan = sscan->next) {
		char *recent_command = sscan->data;
		if (strcmp (recent_command, application) == 0) {
			command = g_strdup (recent_command);
			present = TRUE;
		}
	}

	if (! present) {
		editors = g_slist_prepend (editors, g_strdup (application));
		command = g_strdup (application);
		eel_mateconf_set_string_list (PREF_EDIT_EDITORS, editors);
	}

	g_slist_foreach (editors, (GFunc) g_free, NULL);
	g_slist_free (editors);

	/* exec the application */

	if (command != NULL) {
		fr_window_open_files_with_command (data->window, data->file_list, command);
		g_free (command);
	}

	gtk_widget_destroy (data->dialog);
}


static void
app_list_selection_changed_cb (GtkTreeSelection *selection,
			       gpointer          p)
{
	DialogData  *data = p;
	GtkTreeIter  iter;
	GAppInfo    *app;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (data->o_app_tree_view));
	if (selection == NULL)
		return;

	if (! gtk_tree_selection_get_selected (selection, NULL, &iter))
		return;

	gtk_tree_model_get (data->app_model, &iter,
			    DATA_COLUMN, &app,
			    -1);
	_gtk_entry_set_locale_text (GTK_ENTRY (data->o_app_entry), g_app_info_get_executable (app));
	data->last_clicked_list = data->o_app_tree_view;
}


static void
app_activated_cb (GtkTreeView       *tree_view,
		  GtkTreePath       *path,
		  GtkTreeViewColumn *column,
		  gpointer           callback_data)
{
	DialogData   *data = callback_data;
	GtkTreeIter   iter;
	GAppInfo     *app;

	if (! gtk_tree_model_get_iter (data->app_model, &iter, path))
		return;

	gtk_tree_model_get (data->app_model, &iter,
			    DATA_COLUMN, &app,
			    -1);

	_gtk_entry_set_locale_text (GTK_ENTRY (data->o_app_entry), g_app_info_get_executable (app));

	open_cb (NULL, data);
}


static void
recent_list_selection_changed_cb (GtkTreeSelection *selection,
				  gpointer          p)
{
	DialogData   *data = p;
	GtkTreeIter   iter;
	char         *editor;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (data->o_recent_tree_view));
	if (selection == NULL)
		return;

	if (! gtk_tree_selection_get_selected (selection, NULL, &iter))
		return;

	gtk_tree_model_get (data->recent_model, &iter,
			    0, &editor,
			    -1);
	_gtk_entry_set_locale_text (GTK_ENTRY (data->o_app_entry), editor);
	g_free (editor);
	data->last_clicked_list = data->o_recent_tree_view;
}


static void
recent_activated_cb (GtkTreeView       *tree_view,
		     GtkTreePath       *path,
		     GtkTreeViewColumn *column,
		     gpointer           callback_data)
{
	DialogData   *data = callback_data;
	GtkTreeIter   iter;
	char         *editor;

	if (! gtk_tree_model_get_iter (data->recent_model, &iter, path))
		return;

	gtk_tree_model_get (data->recent_model, &iter,
			    0, &editor,
			    -1);
	_gtk_entry_set_locale_text (GTK_ENTRY (data->o_app_entry), editor);
	g_free (editor);

	open_cb (NULL, data);
}


static void
app_entry__changed_cb (GtkEditable *editable,
		       gpointer     user_data)
{
	DialogData *data = user_data;
	const char *text;

	text = eat_void_chars (gtk_entry_get_text (GTK_ENTRY (data->o_app_entry)));
	gtk_widget_set_sensitive (data->ok_button, strlen (text) > 0);
}


static void
delete_recent_cb (GtkWidget *widget,
		  gpointer   callback_data)
{
	DialogData       *data = callback_data;
	GtkTreeSelection *selection;
	GtkTreeIter       iter;
	

	if (data->last_clicked_list == data->o_recent_tree_view) {
		char   *editor;
		GSList *editors, *link;
	
		selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (data->o_recent_tree_view));
		if (! gtk_tree_selection_get_selected (selection, NULL, &iter))
			return;

		gtk_tree_model_get (data->recent_model, &iter,
				    0, &editor,
				    -1);
		gtk_list_store_remove (GTK_LIST_STORE (data->recent_model), &iter);

		/**/

		editors = eel_mateconf_get_string_list (PREF_EDIT_EDITORS);
		link = g_slist_find_custom (editors, editor, (GCompareFunc) strcmp);
		if (link != NULL) {
			editors = g_slist_remove_link (editors, link);
			eel_mateconf_set_string_list (PREF_EDIT_EDITORS, editors);
			g_free (link->data);
			g_slist_free (link);
		}
		g_slist_foreach (editors, (GFunc) g_free, NULL);
		g_slist_free (editors);

		g_free (editor);
	}
	else if (data->last_clicked_list == data->o_app_tree_view) {
		GAppInfo *app;
		
		selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (data->o_app_tree_view));
		if (! gtk_tree_selection_get_selected (selection, NULL, &iter))
			return;

		gtk_tree_model_get (data->app_model, &iter,
			    	    DATA_COLUMN, &app,
			    	    -1);
		gtk_list_store_remove (GTK_LIST_STORE (data->app_model), &iter);
		
		if (g_app_info_can_remove_supports_type (app)) {
			const char *mime_type;
			
			mime_type = get_file_mime_type_for_path ((char*) data->file_list->data, FALSE);
			g_app_info_remove_supports_type (app, mime_type, NULL);
		}
	}
}


/* create the "open with" dialog. */
void
dlg_open_with (FrWindow *window,
	       GList    *file_list)
{
	DialogData        *data;
	GAppInfo          *app;
	GList             *scan, *app_names = NULL;
	GSList            *sscan, *editors;
	GtkWidget         *cancel_button;
	GtkTreeIter        iter;
	GtkCellRenderer   *renderer;
	GtkTreeViewColumn *column;
	GtkIconTheme      *theme;
	int                icon_size;

	if (file_list == NULL)
		return;

	data = g_new0 (DialogData, 1);

	data->builder = _gtk_builder_new_from_file ("open-with.ui");
	if (data->builder == NULL) {
		g_free (data);
		return;
	}

	data->file_list = path_list_dup (file_list);
	data->window = window;

	/* Get the widgets. */

	data->dialog = _gtk_builder_get_widget (data->builder, "open_with_dialog");
	data->o_app_tree_view = _gtk_builder_get_widget (data->builder, "o_app_list_tree_view");
	data->o_recent_tree_view = _gtk_builder_get_widget (data->builder, "o_recent_tree_view");
	data->o_app_entry = _gtk_builder_get_widget (data->builder, "o_app_entry");
	data->o_del_button = _gtk_builder_get_widget (data->builder, "o_del_button");
	data->ok_button = _gtk_builder_get_widget (data->builder, "o_ok_button");
	cancel_button = _gtk_builder_get_widget (data->builder, "o_cancel_button");

	gtk_widget_set_sensitive (data->ok_button, FALSE);

	/* Set the signals handlers. */

	g_signal_connect (G_OBJECT (data->dialog),
			  "destroy",
			  G_CALLBACK (open_with__destroy_cb),
			  data);

	g_signal_connect (G_OBJECT (data->o_app_entry),
			  "changed",
			  G_CALLBACK (app_entry__changed_cb),
			  data);

	g_signal_connect (G_OBJECT (gtk_tree_view_get_selection (GTK_TREE_VIEW (data->o_app_tree_view))),
			  "changed",
			  G_CALLBACK (app_list_selection_changed_cb),
			  data);
	g_signal_connect (G_OBJECT (data->o_app_tree_view),
			  "row_activated",
			  G_CALLBACK (app_activated_cb),
			  data);

	g_signal_connect (G_OBJECT (gtk_tree_view_get_selection (GTK_TREE_VIEW (data->o_recent_tree_view))),
			  "changed",
			  G_CALLBACK (recent_list_selection_changed_cb),
			  data);
	g_signal_connect (G_OBJECT (data->o_recent_tree_view),
			  "row_activated",
			  G_CALLBACK (recent_activated_cb),
			  data);

	g_signal_connect (G_OBJECT (data->ok_button),
			  "clicked",
			  G_CALLBACK (open_cb),
			  data);
	g_signal_connect_swapped (G_OBJECT (cancel_button),
				  "clicked",
				  G_CALLBACK (gtk_widget_destroy),
				  G_OBJECT (data->dialog));
	g_signal_connect (G_OBJECT (data->o_del_button),
			  "clicked",
			  G_CALLBACK (delete_recent_cb),
			  data);

	/* Set data. */

	/* * registered applications list. */

	data->app_list = NULL;
	for (scan = data->file_list; scan; scan = scan->next) {
		const char *mime_type;
		const char *name = scan->data;

		mime_type = get_file_mime_type_for_path (name, FALSE);
		if ((mime_type != NULL) && ! g_content_type_is_unknown (mime_type))
			data->app_list = g_list_concat (data->app_list, g_app_info_get_all_for_type (mime_type));
	}

	data->app_model = GTK_TREE_MODEL (gtk_list_store_new (N_COLUMNS,
							      GDK_TYPE_PIXBUF,
							      G_TYPE_STRING,
							      G_TYPE_POINTER));

	gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (data->app_model),
					      TEXT_COLUMN,
					      GTK_SORT_ASCENDING);

	gtk_tree_view_set_model (GTK_TREE_VIEW (data->o_app_tree_view),
				 data->app_model);
	g_object_unref (G_OBJECT (data->app_model));

	theme = gtk_icon_theme_get_default ();
	icon_size = get_folder_pixbuf_size_for_list (GTK_WIDGET (data->dialog));

	for (scan = data->app_list; scan; scan = scan->next) {
		gboolean   found;
		char      *utf8_name;
		GdkPixbuf *icon_image = NULL;

		app = scan->data;

		found = FALSE;
		if (app_names != NULL) {
			GList *p;
			for (p = app_names; p && !found; p = p->next)
				if (strcmp ((char*)p->data, g_app_info_get_executable (app)) == 0)
					found = TRUE;
		}

		if (found)
			continue;

		app_names = g_list_prepend (app_names, (char*) g_app_info_get_executable (app));

		utf8_name = g_locale_to_utf8 (g_app_info_get_name (app), -1, NULL, NULL, NULL);	
		icon_image = get_icon_pixbuf (g_app_info_get_icon (app), icon_size, theme);
		
		gtk_list_store_append (GTK_LIST_STORE (data->app_model),
				       &iter);
		gtk_list_store_set (GTK_LIST_STORE (data->app_model),
				    &iter,
				    ICON_COLUMN, icon_image,
				    TEXT_COLUMN, utf8_name,
				    DATA_COLUMN, app,
				    -1);

		g_free (utf8_name);
	}

	column = gtk_tree_view_column_new ();

	renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_tree_view_column_pack_start (column, renderer, FALSE);
	gtk_tree_view_column_set_attributes (column, renderer,
					     "pixbuf", ICON_COLUMN,
					     NULL);

	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (column,
					 renderer,
					 TRUE);
	gtk_tree_view_column_set_attributes (column, renderer,
					     "text", TEXT_COLUMN,
					     NULL);

	gtk_tree_view_append_column (GTK_TREE_VIEW (data->o_app_tree_view),
				     column);

	if (app_names)
		g_list_free (app_names);

	/* * recent editors list. */

	data->recent_model = GTK_TREE_MODEL (gtk_list_store_new (1, G_TYPE_STRING));
	gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (data->recent_model), 0, GTK_SORT_ASCENDING);

	gtk_tree_view_set_model (GTK_TREE_VIEW (data->o_recent_tree_view),
				 data->recent_model);
	g_object_unref (G_OBJECT (data->recent_model));

	editors = eel_mateconf_get_string_list (PREF_EDIT_EDITORS);
	for (sscan = editors; sscan; sscan = sscan->next) {
		char *editor = sscan->data;

		gtk_list_store_append (GTK_LIST_STORE (data->recent_model), &iter);
		gtk_list_store_set (GTK_LIST_STORE (data->recent_model), &iter,
				    0, editor,
				    -1);
	}
	g_slist_foreach (editors, (GFunc) g_free, NULL);
	g_slist_free (editors);

	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (NULL,
							   renderer,
							   "text", 0,
							   NULL);
	gtk_tree_view_column_set_sort_column_id (column, 0);
	gtk_tree_view_append_column (GTK_TREE_VIEW (data->o_recent_tree_view),
				     column);

	/* Run dialog. */
	gtk_window_set_transient_for (GTK_WINDOW (data->dialog),
				      GTK_WINDOW (window));
	gtk_window_set_modal (GTK_WINDOW (data->dialog), TRUE);
	gtk_widget_show_all (data->dialog);
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

/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  File-Roller
 *
 *  Copyright (C) 2005 Free Software Foundation, Inc.
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
#include "eggtreemultidnd.h"
#include "fr-list-model.h"
#include "fr-window.h"


static GtkListStoreClass *parent_class;


static gboolean
fr_list_model_multi_row_draggable (EggTreeMultiDragSource *drag_source,
				   GList                  *path_list)
{
	FrWindow     *window;
	GtkTreeModel *model;
	GList        *scan;

	window = g_object_get_data (G_OBJECT (drag_source), "FrWindow");
	g_return_val_if_fail (window != NULL, FALSE);

	model = fr_window_get_list_store (window);

	for (scan = path_list; scan; scan = scan->next) {
		GtkTreeRowReference *reference = scan->data;
		GtkTreePath         *path;
		GtkTreeIter          iter;
		FileData            *fdata;

		path = gtk_tree_row_reference_get_path (reference);
		if (path == NULL)
			continue;

		if (! gtk_tree_model_get_iter (model, &iter, path))
			continue;

		gtk_tree_model_get (model, &iter,
				    COLUMN_FILE_DATA, &fdata,
				    -1);

		if (fdata != NULL)
			return TRUE;
	}

	return FALSE;
}


static gboolean
fr_list_model_multi_drag_data_get (EggTreeMultiDragSource *drag_source,
				   GdkDragContext         *context,
				   GtkSelectionData       *selection_data,
				   GList                  *path_list)
{
	FrWindow *window;

	window = g_object_get_data (G_OBJECT (drag_source), "FrWindow");
	g_return_val_if_fail (window != NULL, FALSE);

	return fr_window_file_list_drag_data_get (window,
						  context,
						  selection_data,
						  path_list);
}


static gboolean
fr_list_model_multi_drag_data_delete (EggTreeMultiDragSource *drag_source,
				      GList                  *path_list)
{
	return TRUE;
}


static void
fr_list_model_finalize (GObject *object)
{
	if (G_OBJECT_CLASS (parent_class)->finalize)
		G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
fr_list_model_init (FRListModel *model)
{
}


static void
fr_list_model_class_init (FRListModelClass *klass)
{
	GObjectClass *object_class;

	object_class = (GObjectClass *)klass;
	parent_class = g_type_class_peek_parent (klass);

	object_class->finalize = fr_list_model_finalize;
}



static void
fr_list_model_multi_drag_source_init (EggTreeMultiDragSourceIface *iface)
{
	iface->row_draggable = fr_list_model_multi_row_draggable;
	iface->drag_data_get = fr_list_model_multi_drag_data_get;
	iface->drag_data_delete = fr_list_model_multi_drag_data_delete;
}


GType
fr_list_model_get_type (void)
{
	static GType object_type = 0;

	if (object_type == 0) {
		static const GTypeInfo object_info = {
			sizeof (FRListModelClass),
			NULL,		/* base_init */
			NULL,		/* base_finalize */
			(GClassInitFunc) fr_list_model_class_init,
			NULL,		/* class_finalize */
			NULL,		/* class_data */
			sizeof (FRListModel),
			0,
			(GInstanceInitFunc) fr_list_model_init,
		};

		static const GInterfaceInfo multi_drag_source_info = {
			(GInterfaceInitFunc) fr_list_model_multi_drag_source_init,
			NULL,
			NULL
		};

		object_type = g_type_register_static (GTK_TYPE_LIST_STORE, "FRListModel", &object_info, 0);
		g_type_add_interface_static (object_type,
					     EGG_TYPE_TREE_MULTI_DRAG_SOURCE,
					     &multi_drag_source_info);
	}

	return object_type;
}


GtkListStore *
fr_list_model_new (int n_columns, ...)
{
	GtkListStore *retval;
	GType        *types;
	va_list       args;
	int           i;

	g_return_val_if_fail (n_columns > 0, NULL);

	retval = g_object_new (FR_TYPE_LIST_MODEL, NULL);

	va_start (args, n_columns);
	types = g_new0 (GType, n_columns);
	for (i = 0; i < n_columns; i++)
		types[i] = va_arg (args, GType);
	va_end (args);

	gtk_list_store_set_column_types (retval, n_columns, types);
	g_free (types);

	return retval;
}

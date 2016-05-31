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

#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <json-glib/json-glib.h>

#include "file-data.h"
#include "file-utils.h"
#include "glib-utils.h"
#include "fr-command.h"
#include "fr-command-json.h"

#define ACTIVITY_DELAY 20

static void fr_command_json_class_init  (FrCommandJsonClass *class);
static void fr_command_json_init        (FrCommand          *afile);
static void fr_command_json_finalize    (GObject            *object);

/* Parent Class */

static FrCommandClass *parent_class = NULL;


/* -- list -- */

typedef struct _json_recurse_data json_recurse_data;

struct _json_recurse_data
{
	FrCommandJson *json_comm;
	gchar         *path;
};

static void
process_line (char     *line,
              gpointer  data)
{
	FrCommandJson *json_comm = FR_COMMAND_JSON (data);
	g_memory_input_stream_add_data (G_MEMORY_INPUT_STREAM (json_comm->stream), line, -1, NULL);
}


static void json_recurse (const char    *full_name, 
                          JsonNode      *node,
                          FrCommandJson *json_comm);


static gchar*
json_get_string (JsonNode *node)
{
	if (!JSON_NODE_HOLDS_VALUE (node) && !JSON_NODE_HOLDS_NULL (node)) {
		return NULL;
	}

	if (json_node_get_node_type (node) == JSON_NODE_NULL) {
		return g_strdup ("null");
	}
	else
	{
		switch (json_node_get_value_type (node)) {
			case G_TYPE_INT64: {
					gchar *result_string;

					result_string = g_malloc (G_ASCII_DTOSTR_BUF_SIZE);
					g_ascii_dtostr (result_string, G_ASCII_DTOSTR_BUF_SIZE, (gdouble) json_node_get_int (node));

					return result_string;
				}
			case G_TYPE_DOUBLE: {
					gchar *result_string;

					result_string = g_malloc (G_ASCII_DTOSTR_BUF_SIZE);
					g_ascii_dtostr (result_string, G_ASCII_DTOSTR_BUF_SIZE, json_node_get_double (node));

					return result_string;
				}
			case G_TYPE_BOOLEAN:
				switch (json_node_get_boolean (node)) {
					case TRUE:
						return g_strdup ("true");
					case FALSE:
						return g_strdup ("false");
				}
			case G_TYPE_STRING:
				return g_strdup (json_node_get_string (node));
		}
	}

	return NULL;
}


static void
json_recurse_object (JsonObject *parent,
                     const char *node_name,
                     JsonNode   *node,
                     gpointer    data)
{
	gchar *sanitised_node_name;
	gchar *full_path;

	sanitised_node_name = g_strdelimit (g_strdup (node_name), "/", '_');
	full_path = g_strconcat (((json_recurse_data*) data)->path, sanitised_node_name, NULL);
	g_free (sanitised_node_name);
	json_recurse (full_path, node, ((json_recurse_data*) data)->json_comm);
	g_free (full_path);
}


static void
json_recurse_array (JsonArray *parent,
                    guint      index,
                    JsonNode  *node,
                    gpointer   data)
{
	gchar *node_name;
	gchar *full_path;

	node_name = g_malloc (G_ASCII_DTOSTR_BUF_SIZE);
	g_ascii_dtostr (node_name, G_ASCII_DTOSTR_BUF_SIZE, (gdouble) index);
	full_path = g_strconcat (((json_recurse_data*) data)->path, node_name, NULL);
	g_free (node_name);
	json_recurse (full_path, node, ((json_recurse_data*) data)->json_comm);
	g_free (full_path);
}


static void
json_recurse (const char    *full_path,
              JsonNode      *node,
              FrCommandJson *json_comm)
{
	switch (json_node_get_node_type (node)) {
		case JSON_NODE_OBJECT: {
				json_recurse_data *new_data;

				new_data = malloc (sizeof (json_recurse_data));
				new_data->json_comm = json_comm;
				new_data->path = g_strconcat (full_path, "/", NULL);
				json_object_foreach_member (json_node_get_object (node), json_recurse_object, (gpointer) new_data);
				g_free (new_data->path);
				free (new_data);
			}
			break;
		case JSON_NODE_ARRAY: {
				json_recurse_data *new_data;

				new_data = malloc (sizeof (json_recurse_data));
				new_data->json_comm = json_comm;
				new_data->path = g_strconcat (full_path, "/", NULL);
				json_array_foreach_element (json_node_get_array (node), json_recurse_array, (gpointer) new_data);
				g_free (new_data->path);
				free (new_data);
			}
			break;
		case JSON_NODE_VALUE:
		case JSON_NODE_NULL: {
				FileData *fdata;
				gchar    *node_text;

				node_text = json_get_string (node);

				fdata = file_data_new ();
				fdata->size = strlen (node_text);
				fdata->modified = 0;
				fdata->full_path = g_strdup (full_path);
				fdata->original_path = fdata->full_path;
				fdata->link = NULL;
				fdata->name = g_strdup (file_name_from_path (fdata->full_path));
				fdata->path = remove_level_from_path (fdata->full_path);

				fr_command_add_file (FR_COMMAND (json_comm), fdata);

				g_free (node_text);
			}
			break;
	}
}


static void
list_command_completed (gpointer data)
{
	FrCommandJson *json_comm = FR_COMMAND_JSON (data);
	JsonParser    *parser;
	GError        *error = NULL;

	parser = json_parser_new ();
	if (json_parser_load_from_stream (parser, json_comm->stream, NULL, &error)) {
		json_recurse ("", json_parser_get_root (parser), (gpointer) json_comm);
	}

	g_object_unref (parser);
}


static void extract_command_completed (gpointer data)
{
	FrCommandJson *json_comm = FR_COMMAND_JSON (data);
	JsonParser    *parser;
	GList         *file_list_scan;
	GError        *error = NULL;

	parser = json_parser_new ();
	if (json_parser_load_from_stream (parser, json_comm->stream, NULL, &error)) {
		for (file_list_scan = json_comm->file_list; file_list_scan; file_list_scan = file_list_scan->next) {
			gchar    **path_components;
			JsonNode *node;
			int      index;
			gchar    *dest_file;
			gchar    *out_string;

			path_components = g_strsplit (file_list_scan->data, "/", 0);
			node = json_parser_get_root (parser);
			for (index = 1; index < g_strv_length (path_components); index++) {
				switch (json_node_get_node_type (node)) {
					case JSON_NODE_OBJECT:
						node = json_object_get_member (json_node_get_object (node), path_components[index]);
						break;
					case JSON_NODE_ARRAY:
						node = json_array_get_element (json_node_get_array (node), g_ascii_strtoull (path_components[index], NULL, 0));
						break;
					default:
						node = NULL;
						break;
				}
				if (node == NULL) {
					break;
				}
			}
			g_strfreev (path_components);
			if (node == NULL) {
				continue;
			}

			out_string = json_get_string (node);
			if (out_string != NULL) {
				dest_file = g_strconcat (json_comm->dest_dir, file_list_scan->data, NULL);
				if (g_mkdir_with_parents (remove_level_from_path (dest_file), 511) == 0) {
					error = NULL;
					g_file_set_contents (dest_file, out_string, -1, &error);
				}
				g_free (dest_file);
			}
			g_free (out_string);
		}
	}

	g_object_unref (parser);
	g_list_free_full (json_comm->file_list, g_free);
	g_free (json_comm->dest_dir);
}


static void
fr_command_json_list (FrCommand *comm)
{
	FrCommandJson *json_comm = FR_COMMAND_JSON (comm);

	g_object_unref (json_comm->stream);
	json_comm->stream = g_memory_input_stream_new ();

	fr_process_set_out_line_func (comm->process, process_line, comm);

	fr_process_begin_command (comm->process, "cat");
	fr_process_set_end_func (comm->process, list_command_completed, comm);
	fr_process_add_arg (comm->process, comm->filename);
	fr_process_end_command (comm->process);

	fr_process_start (comm->process);
}


static void
fr_command_json_extract (FrCommand  *comm,
                         const char *from_file,
                         GList      *file_list,
                         const char *dest_dir,
                         gboolean    overwrite,
                         gboolean    skip_older,
                         gboolean    junk_paths)
{
	FrCommandJson *json_comm = FR_COMMAND_JSON (comm);

	json_comm->dest_dir = g_strdup (dest_dir);
	json_comm->file_list = g_list_copy_deep (file_list, (GCopyFunc) g_strdup, NULL);

	g_object_unref (json_comm->stream);
	json_comm->stream = g_memory_input_stream_new ();

	fr_process_set_out_line_func (comm->process, process_line, comm);

	fr_process_begin_command (comm->process, "cat");
	fr_process_set_end_func (comm->process, extract_command_completed, comm);
	fr_process_add_arg (comm->process, comm->filename);
	fr_process_end_command (comm->process);

	fr_process_start (comm->process);
}


static void
fr_command_json_handle_error (FrCommand   *comm,
                              FrProcError *error)
{
	if (error->type != FR_PROC_ERROR_NONE) {
		if (error->status <= 1)
			error->type = FR_PROC_ERROR_NONE;
	}
}


const char *json_mime_types[] = { "application/json", NULL };


static const char **
fr_command_json_get_mime_types (FrCommand *comm)
{
	return json_mime_types;
}


static FrCommandCap
fr_command_json_get_capabilities (FrCommand  *comm,
                                  const char *mime_type,
                                  gboolean   check_command)
{
	FrCommandCap capabilities;

	capabilities = FR_COMMAND_CAN_DO_NOTHING;
	capabilities |= FR_COMMAND_CAN_READ;

	return capabilities;
}


static const char *
fr_command_json_get_packages (FrCommand  *comm,
                              const char *mime_type)
{
	return NULL;
}


static void
fr_command_json_class_init (FrCommandJsonClass *class)
{
    GObjectClass   *gobject_class = G_OBJECT_CLASS (class);
    FrCommandClass *afc;

    parent_class = g_type_class_peek_parent (class);
	afc = (FrCommandClass*) class;

	gobject_class->finalize = fr_command_json_finalize;

    afc->list             = fr_command_json_list;
	afc->extract          = fr_command_json_extract;
	afc->handle_error     = fr_command_json_handle_error;
	afc->get_mime_types   = fr_command_json_get_mime_types;
	afc->get_capabilities = fr_command_json_get_capabilities;
	afc->get_packages     = fr_command_json_get_packages;
}


static void
fr_command_json_init (FrCommand *comm)
{
	FrCommandJson *comm_json = (FrCommandJson*) comm;

	comm->propExtractCanAvoidOverwrite  = FALSE;
	comm->propExtractCanSkipOlder       = FALSE;
	comm->propExtractCanJunkPaths       = FALSE;
	comm->propPassword                  = FALSE;
	comm->propTest                      = FALSE;
	comm->propCanDeleteNonEmptyFolders  = FALSE;
	comm->propCanExtractNonEmptyFolders = FALSE;
	comm->propListFromFile              = TRUE;

	comm_json->stream = NULL;
}


static void
fr_command_json_finalize (GObject *object)
{
	FrCommandJson *comm_json;

    g_return_if_fail (object != NULL);
    g_return_if_fail (FR_IS_COMMAND_JSON (object));

	comm_json = FR_COMMAND_JSON (object);

	g_object_unref (comm_json->stream);

	/* Chain up */
        if (G_OBJECT_CLASS (parent_class)->finalize)
		G_OBJECT_CLASS (parent_class)->finalize (object);
}


GType
fr_command_json_get_type ()
{
        static GType type = 0;

        if (! type) {
                GTypeInfo type_info = {
			sizeof (FrCommandJsonClass),
			NULL,
			NULL,
			(GClassInitFunc) fr_command_json_class_init,
			NULL,
			NULL,
			sizeof (FrCommandJson),
			0,
			(GInstanceInitFunc) fr_command_json_init
		};

		type = g_type_register_static (FR_TYPE_COMMAND,
					       "FRCommandJson",
					       &type_info,
					       0);
        }

        return type;
}

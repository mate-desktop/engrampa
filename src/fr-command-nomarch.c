/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  Engrampa
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#define _XOPEN_SOURCE /* strptime */

#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <glib.h>

#include "file-data.h"
#include "file-utils.h"
#include "glib-utils.h"
#include "fr-command.h"
#include "fr-command-nomarch.h"

#define LSNOMARCH_DATE_FORMAT "%Y-%m-%d %H:%M"

static void fr_command_nomarch_class_init  (FrCommandNomarchClass *class);
static void fr_command_nomarch_init        (FrCommand             *afile);
static void fr_command_nomarch_finalize    (GObject               *object);

/* Parent Class */

static FrCommandClass *parent_class = NULL;

/* -- list -- */

static time_t
mktime_from_string (const char *time_s)
{
	struct tm tm = {0, };
	tm.tm_isdst = -1;
	strptime (time_s, LSNOMARCH_DATE_FORMAT, &tm);
	return mktime (&tm);
}

static void
list__process_line (char     *line,
		    gpointer  data)
{
	FileData    *fdata;
	FrCommand   *comm = FR_COMMAND (data);
	char       **fields;
	int          date_idx;
	char        *field_date, *field_name;

	g_return_if_fail (line != NULL);

	date_idx = file_list__get_index_from_pattern (line, "%n%n%n%n-%n%n-%n%n %n%n:%n%n");
	if (date_idx < 0)
		return;

	fdata = file_data_new ();
	fields = split_line (line, 8);

	field_name = fields[0];
	fdata->full_path = g_strconcat ("/", field_name, NULL);
	fdata->original_path = fdata->full_path + 1;
	fdata->name = g_strdup (file_name_from_path (fdata->full_path));
	fdata->path = remove_level_from_path (fdata->full_path);

	fdata->size = g_ascii_strtoull (fields[4], NULL, 10);

	field_date = g_strndup (line + date_idx, 16);
	fdata->modified = mktime_from_string (field_date);
	g_free (field_date);

	g_strfreev (fields);

	if (*fdata->name == 0)
		file_data_free (fdata);
	else
		fr_command_add_file (comm, fdata);
}

static void
fr_command_nomarch_list (FrCommand *comm)
{
	fr_process_set_out_line_func (comm->process, list__process_line, comm);

	fr_process_begin_command (comm->process, "nomarch");
	fr_process_add_arg (comm->process, "-lvU");
	fr_process_add_arg (comm->process, comm->filename);
	fr_process_end_command (comm->process);
	fr_process_start (comm->process);
}

static void
fr_command_nomarch_add (FrCommand     *comm,
                        const char    *from_file,
                        GList         *file_list,
                        const char    *base_dir,
                        gboolean       update,
                        gboolean       recursive)
{
	GList *scan;

	fr_process_begin_command (comm->process, "arc");

	if (base_dir != NULL)
		fr_process_set_working_dir (comm->process, base_dir);

	if (update)
		fr_process_add_arg (comm->process, "u");
	else
		fr_process_add_arg (comm->process, "a");

	fr_process_add_arg (comm->process, comm->filename);

	for (scan = file_list; scan; scan = scan->next)
		fr_process_add_arg (comm->process, (gchar*) scan->data);

	fr_process_end_command (comm->process);
}

static void
fr_command_nomarch_delete (FrCommand  *comm,
                           const char *from_file,
                           GList      *file_list)
{
	GList *scan;

	fr_process_begin_command (comm->process, "arc");
	fr_process_add_arg (comm->process, "d");

	fr_process_add_arg (comm->process, comm->filename);

	for (scan = file_list; scan; scan = scan->next)
		fr_process_add_arg (comm->process, scan->data);
	fr_process_end_command (comm->process);
}

static void
fr_command_nomarch_extract (FrCommand  *comm,
                            const char *from_file,
                            GList      *file_list,
                            const char *dest_dir,
                            gboolean    overwrite,
                            gboolean    skip_older,
                            gboolean    junk_paths)
{
	GList *scan;

	fr_process_begin_command (comm->process, "nomarch");

	if (dest_dir != NULL)
		fr_process_set_working_dir (comm->process, dest_dir);

	fr_process_add_arg (comm->process, "-U");
	fr_process_add_arg (comm->process, comm->filename);
	for (scan = file_list; scan; scan = scan->next)
		fr_process_add_arg (comm->process, scan->data);

	fr_process_end_command (comm->process);
}

static void
fr_command_nomarch_test (FrCommand   *comm)
{
	fr_process_begin_command (comm->process, "nomarch");
	fr_process_add_arg (comm->process, "-tU");
	fr_process_add_arg (comm->process, comm->filename);
	fr_process_end_command (comm->process);
}

static void
fr_command_nomarch_handle_error (FrCommand   *comm,
                                 FrProcError *error)
{
	if (error->type != FR_PROC_ERROR_NONE) {
 		if (error->status <= 1)
 			error->type = FR_PROC_ERROR_NONE;
		else if (error->status == 3)
 			error->type = FR_PROC_ERROR_ASK_PASSWORD;
 	}
}

const char *nomarch_mime_type[] = { "application/x-arc", NULL };

static const char **
fr_command_nomarch_get_mime_types (FrCommand *comm)
{
	return nomarch_mime_type;
}

static FrCommandCaps
fr_command_nomarch_get_capabilities (FrCommand  *comm,
                                     const char *mime_type,
                                     gboolean    check_command)
{
	FrCommandCaps capabilities;

	capabilities = FR_COMMAND_CAN_ARCHIVE_MANY_FILES;
	if (is_program_available ("nomarch", check_command)) {
		capabilities |= FR_COMMAND_CAN_READ;
		if (is_program_available ("arc", check_command))
			capabilities |= FR_COMMAND_CAN_WRITE;
	}
	return capabilities;
}

static const char *
fr_command_nomarch_get_packages (FrCommand  *comm,
                                 const char *mime_type)
{
	return PACKAGES ("nomarch,arc");
}

static void
fr_command_nomarch_class_init (FrCommandNomarchClass *class)
{
	GObjectClass   *gobject_class = G_OBJECT_CLASS (class);
	FrCommandClass *afc;

	parent_class = g_type_class_peek_parent (class);
	afc = (FrCommandClass*) class;

	gobject_class->finalize = fr_command_nomarch_finalize;

	afc->list             = fr_command_nomarch_list;
	afc->add              = fr_command_nomarch_add;
	afc->delete           = fr_command_nomarch_delete;
	afc->extract          = fr_command_nomarch_extract;
	afc->test             = fr_command_nomarch_test;
	afc->handle_error     = fr_command_nomarch_handle_error;
	afc->get_mime_types   = fr_command_nomarch_get_mime_types;
	afc->get_capabilities = fr_command_nomarch_get_capabilities;
	afc->get_packages     = fr_command_nomarch_get_packages;
}

static void
fr_command_nomarch_init (FrCommand *comm)
{
	FrCommandNomarch *nomarch_comm;

	comm->propAddCanUpdate             = TRUE;
	comm->propAddCanReplace            = TRUE;
	comm->propAddCanStoreFolders       = FALSE;
	comm->propExtractCanAvoidOverwrite = TRUE;
	comm->propExtractCanSkipOlder      = TRUE;
	comm->propExtractCanJunkPaths      = TRUE;
	comm->propPassword                 = TRUE;
	comm->propTest                     = TRUE;

	nomarch_comm = FR_COMMAND_NOMARCH (comm);
	nomarch_comm->fdata = FALSE;
}

static void
fr_command_nomarch_finalize (GObject *object)
{
	g_return_if_fail (object != NULL);
	g_return_if_fail (FR_IS_COMMAND_NOMARCH (object));

	/* Chain up */
	if (G_OBJECT_CLASS (parent_class)->finalize)
		G_OBJECT_CLASS (parent_class)->finalize (object);
}

GType
fr_command_nomarch_get_type ()
{
	static GType type = 0;

	if (! type) {
		GTypeInfo type_info = {
			sizeof (FrCommandNomarchClass),
			NULL,
			NULL,
			(GClassInitFunc) fr_command_nomarch_class_init,
			NULL,
			NULL,
			sizeof (FrCommandNomarch),
			0,
			(GInstanceInitFunc) fr_command_nomarch_init,
			NULL
		};

		type = g_type_register_static (FR_TYPE_COMMAND,
					       "FRCommandNomarch",
					       &type_info,
					       0);
	}

	return type;
}

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <glib.h>
#include <glib/gi18n.h>

#include "file-data.h"
#include "file-utils.h"
#include "gio-utils.h"
#include "glib-utils.h"
#include "fr-command.h"
#include "fr-command-rar.h"
#include "fr-error.h"

static void fr_command_rar_class_init  (FrCommandRarClass *class);
static void fr_command_rar_init        (FrCommand         *afile);
static void fr_command_rar_finalize    (GObject           *object);

/* Parent Class */

static FrCommandClass *parent_class = NULL;


static gboolean
have_rar (void)
{
	return is_program_in_path ("rar");
}


/* -- list -- */


static time_t
mktime_from_string (char *date_s,
		    char *time_s)
{
	struct tm   tm = {0, };
	char      **fields;

	tm.tm_isdst = -1;

	/* date */

	fields = g_strsplit (date_s, "-", 3);
	if (fields[0] != NULL) {
		tm.tm_mday = atoi (fields[0]);
		if (fields[1] != NULL) {
			tm.tm_mon = atoi (fields[1]) - 1;
			if (fields[2] != NULL)
				tm.tm_year = 100 + atoi (fields[2]);
		}
	}
	g_strfreev (fields);

	/* time */

	fields = g_strsplit (time_s, ":", 2);
	if (fields[0] != NULL) {
		tm.tm_hour = atoi (fields[0]);
		if (fields[1] != NULL)
			tm.tm_min = atoi (fields[1]);
	}
	g_strfreev (fields);

	return mktime (&tm);
}


static void
process_line (char     *line,
	      gpointer  data)
{
	FrCommand     *comm = FR_COMMAND (data);
	FrCommandRar  *rar_comm = FR_COMMAND_RAR (comm);
	char         **fields;
	const char    *name_field;

	g_return_if_fail (line != NULL);

	if (! rar_comm->list_started) {
		if (strncmp (line, "--------", 8) == 0) {
			rar_comm->list_started = TRUE;
			rar_comm->odd_line = TRUE;
		}
		else if (strncmp (line, "Volume ", 7) == 0)
			comm->multi_volume = TRUE;
		return;
	}

	if (strncmp (line, "--------", 8) == 0) {
		rar_comm->list_started = FALSE;
		return;
	}

	if (! rar_comm->odd_line) {
		FileData *fdata;

		fdata = rar_comm->fdata;

		/* read file info. */

		fields = split_line (line, 6);
		if (g_strv_length (fields) < 6) {
			/* wrong line format, treat this line as a filename line */
			g_strfreev (fields);
			file_data_free (rar_comm->fdata);
			rar_comm->fdata = NULL;
			rar_comm->odd_line = TRUE;
		}
		else {
			if ((strcmp (fields[2], "<->") == 0)
			    || (strcmp (fields[2], "<--") == 0))
			{
				/* ignore files that span more volumes */

				file_data_free (rar_comm->fdata);
				rar_comm->fdata = NULL;
			}
			else {
				fdata->size = g_ascii_strtoull (fields[0], NULL, 10);
				fdata->modified = mktime_from_string (fields[3], fields[4]);

				if ((fields[5][1] == 'D') || (fields[5][0] == 'd')) {
					char *tmp;

					tmp = fdata->full_path;
					fdata->full_path = g_strconcat (fdata->full_path, "/", NULL);

					fdata->original_path = g_strdup (fdata->original_path);
					fdata->free_original_path = TRUE;

					g_free (tmp);

					fdata->name = dir_name_from_path (fdata->full_path);
					fdata->dir = TRUE;
				}
				else
					fdata->name = g_strdup (file_name_from_path (fdata->full_path));

				fr_command_add_file (comm, fdata);
				rar_comm->fdata = NULL;
			}

			g_strfreev (fields);
		}
	}

	if (rar_comm->odd_line) {
		FileData *fdata;

		rar_comm->fdata = fdata = file_data_new ();

		/* read file name. */

		fdata->encrypted = (line[0] == '*') ? TRUE : FALSE;

		name_field = line + 1;

		if (*name_field == '/') {
			fdata->full_path = g_strdup (name_field);
			fdata->original_path = fdata->full_path;
		}
		else {
			fdata->full_path = g_strconcat ("/", name_field, NULL);
			fdata->original_path = fdata->full_path + 1;
		}

		fdata->link = NULL;
		fdata->path = remove_level_from_path (fdata->full_path);
	}
	else {

	}

	rar_comm->odd_line = ! rar_comm->odd_line;
}


static void
add_password_arg (FrCommand  *comm,
		  const char *password,
		  gboolean    disable_query)
{
	if ((password != NULL) && (password[0] != '\0')) {
		if (comm->encrypt_header)
			fr_process_add_arg_concat (comm->process, "-hp", password, NULL);
		else
			fr_process_add_arg_concat (comm->process, "-p", password, NULL);
	}
	else if (disable_query)
		fr_process_add_arg (comm->process, "-p-");
}


typedef enum {
	FIRST_VOLUME_IS_000,
	FIRST_VOLUME_IS_001,
	FIRST_VOLUME_IS_RAR
} FirstVolumeExtension;


static char *
get_first_volume_name (const char           *name,
		       const char           *pattern,
		       FirstVolumeExtension  extension_type)
{
	char   *volume_name = NULL;
	GRegex *re;

	re = g_regex_new (pattern, G_REGEX_CASELESS, 0, NULL);
	if (g_regex_match (re, name, 0, NULL)) {
		char **parts;
		int    l, i;

		parts = g_regex_split (re, name, 0);
		l = strlen (parts[2]);
		switch (extension_type) {
		case FIRST_VOLUME_IS_000:
			for (i = 0; i < l; i++)
				parts[2][i] = '0';
			break;

		case FIRST_VOLUME_IS_001:
			for (i = 0; i < l; i++)
				parts[2][i] = (i < l - 1) ? '0' : '1';
			break;

		case FIRST_VOLUME_IS_RAR:
			if (g_str_has_suffix (parts[1], "r")) {
				parts[2][0] = 'a';
				parts[2][1] = 'r';
			}
			else {
				parts[2][0] = 'A';
				parts[2][1] = 'R';
			}
			break;
		}

		volume_name = g_strjoinv ("", parts);

		g_strfreev (parts);
	}
	g_regex_unref (re);

	if (volume_name != NULL) {
		char *tmp;

		tmp = volume_name;
		volume_name = g_filename_from_utf8 (tmp, -1, NULL, NULL, NULL);
		g_free (tmp);
	}

	return volume_name;
}


static void
check_multi_vomule (FrCommand *comm)
{
	GFile *file;
	char   buffer[11];

	file = g_file_new_for_path (comm->filename);
	if (! g_load_file_in_buffer (file, buffer, 11, NULL)) {
		g_object_unref (file);
		return;
	}

	if ((buffer[10] & 0x01) == 0x01) {
		char   *volume_name = NULL;
		char   *name;

		name = g_filename_to_utf8 (file_name_from_path (comm->filename), -1, NULL, NULL, NULL);

		volume_name = get_first_volume_name (name, "^(.*\\.part)([0-9]+)(\\.rar)$", FIRST_VOLUME_IS_001);
		if (volume_name == NULL)
			volume_name = get_first_volume_name (name, "^(.*\\.r)([0-9]+)$", FIRST_VOLUME_IS_RAR);
		if (volume_name == NULL)
			volume_name = get_first_volume_name (name, "^(.*\\.)([0-9]+)$", FIRST_VOLUME_IS_001);

		if (volume_name != NULL) {
			GFile *parent;
			GFile *child;
			char  *volume_filename;

			parent = g_file_get_parent (file);
			child = g_file_get_child (parent, volume_name);
			volume_filename = g_file_get_path (child);
			fr_command_set_multi_volume (comm, volume_filename);

			g_free (volume_filename);
			g_object_unref (child);
			g_object_unref (parent);
		}

		g_free (name);
	}

	g_object_unref (file);
}


static void
list__begin (gpointer data)
{
	FrCommandRar *comm = data;

	comm->list_started = FALSE;
}


static void
fr_command_rar_list (FrCommand  *comm)
{
	check_multi_vomule (comm);

	fr_process_set_out_line_func (comm->process, process_line, comm);

	if (have_rar ())
		fr_process_begin_command (comm->process, "rar");
	else
		fr_process_begin_command (comm->process, "unrar");
	fr_process_set_begin_func (comm->process, list__begin, comm);
	fr_process_add_arg (comm->process, "v");
	fr_process_add_arg (comm->process, "-c-");
	fr_process_add_arg (comm->process, "-v");

	add_password_arg (comm, comm->password, TRUE);

	/* stop switches scanning */
	fr_process_add_arg (comm->process, "--");

	fr_process_add_arg (comm->process, comm->filename);
	fr_process_end_command (comm->process);

	fr_process_start (comm->process);
}


static char Progress_Message[4196];
static char Progress_Filename[4096];


static void
parse_progress_line (FrCommand  *comm,
		     const char *prefix,
		     const char *message_prefix,
		     const char *line)
{
	int prefix_len;

	prefix_len = strlen (prefix);
	if (strncmp (line, prefix, prefix_len) == 0) {
		double  fraction;
		int     len;
		char   *b_idx;

		strcpy (Progress_Filename, line + prefix_len);

		/* when a new volume is created a sequence of backspaces is
		 * issued, remove the backspaces from the filename */
		b_idx = strchr (Progress_Filename, '\x08');
		if (b_idx != NULL)
			*b_idx = 0;

		/* remove the OK at the end of the filename */
		len = strlen (Progress_Filename);
		if ((len > 5) && (strncmp (Progress_Filename + len - 5, "  OK ", 5) == 0))
			Progress_Filename[len - 5] = 0;

		sprintf (Progress_Message, "%s%s", message_prefix, file_name_from_path (Progress_Filename));
		fr_command_message (comm, Progress_Message);

		fraction = (double) ++comm->n_file / (comm->n_files + 1);
		fr_command_progress (comm, fraction);
	}
}


static void
process_line__add (char     *line,
		   gpointer  data)
{
	FrCommand *comm = FR_COMMAND (data);

	if (strncmp (line, "Creating archive ", 17) == 0) {
		const char *archive_filename = line + 17;
		char *uri;

		uri = g_filename_to_uri (archive_filename, NULL, NULL);
		if ((comm->volume_size > 0)
		    && g_regex_match_simple ("^.*\\.part(0)*2\\.rar$", uri, G_REGEX_CASELESS, 0))
		{
			char *volume_filename;

			volume_filename = g_strdup (archive_filename);
			volume_filename[strlen (volume_filename) - 5] = '1';
			fr_command_set_multi_volume (comm, volume_filename);
			g_free (volume_filename);
		}
		fr_command_working_archive (comm, uri);

		g_free (uri);
		return;
	}

	if (comm->n_files != 0)
		parse_progress_line (comm, "Adding    ", _("Adding file: "), line);
}


static void
fr_command_rar_add (FrCommand     *comm,
		    const char    *from_file,
		    GList         *file_list,
		    const char    *base_dir,
		    gboolean       update,
		    gboolean       recursive)
{
	GList *scan;

	fr_process_use_standard_locale (comm->process, TRUE);
	fr_process_set_out_line_func (comm->process,
				      process_line__add,
				      comm);

	fr_process_begin_command (comm->process, "rar");

	if (base_dir != NULL)
		fr_process_set_working_dir (comm->process, base_dir);

	if (update)
		fr_process_add_arg (comm->process, "u");
	else
		fr_process_add_arg (comm->process, "a");

	switch (comm->compression) {
	case FR_COMPRESSION_VERY_FAST:
		fr_process_add_arg (comm->process, "-m1"); break;
	case FR_COMPRESSION_FAST:
		fr_process_add_arg (comm->process, "-m2"); break;
	case FR_COMPRESSION_NORMAL:
		fr_process_add_arg (comm->process, "-m3"); break;
	case FR_COMPRESSION_MAXIMUM:
		fr_process_add_arg (comm->process, "-m5"); break;
	}

	add_password_arg (comm, comm->password, FALSE);

	if (comm->volume_size > 0)
		fr_process_add_arg_printf (comm->process, "-v%ub", comm->volume_size);

	/* disable percentage indicator */
	fr_process_add_arg (comm->process, "-Idp");

	fr_process_add_arg (comm->process, "--");
	fr_process_add_arg (comm->process, comm->filename);

	if (from_file == NULL)
		for (scan = file_list; scan; scan = scan->next)
			fr_process_add_arg (comm->process, scan->data);
	else
		fr_process_add_arg_concat (comm->process, "@", from_file, NULL);

	fr_process_end_command (comm->process);
}


static void
process_line__delete (char     *line,
		      gpointer  data)
{
	FrCommand *comm = FR_COMMAND (data);

	if (strncmp (line, "Deleting from ", 14) == 0) {
		char *uri;

		uri = g_filename_to_uri (line + 14, NULL, NULL);
		fr_command_working_archive (comm, uri);
		g_free (uri);

		return;
	}

	if (comm->n_files != 0)
		parse_progress_line (comm, "Deleting ", _("Removing file: "), line);
}


static void
fr_command_rar_delete (FrCommand  *comm,
		       const char *from_file,
		       GList      *file_list)
{
	GList *scan;

	fr_process_use_standard_locale (comm->process, TRUE);
	fr_process_set_out_line_func (comm->process,
				      process_line__delete,
				      comm);

	fr_process_begin_command (comm->process, "rar");
	fr_process_add_arg (comm->process, "d");

	fr_process_add_arg (comm->process, "--");
	fr_process_add_arg (comm->process, comm->filename);

	if (from_file == NULL)
		for (scan = file_list; scan; scan = scan->next)
			fr_process_add_arg (comm->process, scan->data);
	else
		fr_process_add_arg_concat (comm->process, "@", from_file, NULL);

	fr_process_end_command (comm->process);
}


static void
process_line__extract (char     *line,
		       gpointer  data)
{
	FrCommand *comm = FR_COMMAND (data);

	if (strncmp (line, "Extracting from ", 16) == 0) {
		char *uri;

		uri = g_filename_to_uri (line + 16, NULL, NULL);
		fr_command_working_archive (comm, uri);
		g_free (uri);

		return;
	}

	if (comm->n_files != 0)
		parse_progress_line (comm, "Extracting  ", _("Extracting file: "), line);
}


static void
fr_command_rar_extract (FrCommand  *comm,
			const char *from_file,
			GList      *file_list,
			const char *dest_dir,
			gboolean    overwrite,
			gboolean    skip_older,
			gboolean    junk_paths)
{
	GList *scan;

	fr_process_use_standard_locale (comm->process, TRUE);
	fr_process_set_out_line_func (comm->process,
				      process_line__extract,
				      comm);

	if (have_rar ())
		fr_process_begin_command (comm->process, "rar");
	else
		fr_process_begin_command (comm->process, "unrar");

	fr_process_add_arg (comm->process, "x");

	/* keep broken extracted files */
	fr_process_add_arg (comm->process, "-kb");

	if (overwrite)
		fr_process_add_arg (comm->process, "-o+");
	else
		fr_process_add_arg (comm->process, "-o-");

	if (skip_older)
		fr_process_add_arg (comm->process, "-u");

	if (junk_paths)
		fr_process_add_arg (comm->process, "-ep");

	add_password_arg (comm, comm->password, TRUE);

	/* disable percentage indicator */
	fr_process_add_arg (comm->process, "-Idp");

	fr_process_add_arg (comm->process, "--");
	fr_process_add_arg (comm->process, comm->filename);

	if (from_file == NULL)
		for (scan = file_list; scan; scan = scan->next)
			fr_process_add_arg (comm->process, scan->data);
	else
		fr_process_add_arg_concat (comm->process, "@", from_file, NULL);

	if (dest_dir != NULL)
		fr_process_add_arg (comm->process, dest_dir);

	fr_process_end_command (comm->process);
}


static void
fr_command_rar_test (FrCommand   *comm)
{
	if (have_rar ())
		fr_process_begin_command (comm->process, "rar");
	else
		fr_process_begin_command (comm->process, "unrar");

	fr_process_add_arg (comm->process, "t");

	add_password_arg (comm, comm->password, TRUE);

	/* disable percentage indicator */
	fr_process_add_arg (comm->process, "-Idp");

	/* stop switches scanning */
	fr_process_add_arg (comm->process, "--");

	fr_process_add_arg (comm->process, comm->filename);
	fr_process_end_command (comm->process);
}


static void
fr_command_rar_handle_error (FrCommand   *comm,
			     FrProcError *error)
{
	GList *scan;

#if 0
	{
		GList *scan;

		for (scan = g_list_last (comm->process->err.raw); scan; scan = scan->prev)
			g_print ("%s\n", (char*)scan->data);
	}
#endif

	if (error->type == FR_PROC_ERROR_NONE)
		return;

	/*if (error->status == 3)
		error->type = FR_PROC_ERROR_ASK_PASSWORD;
	else */
	if (error->status <= 1)
		error->type = FR_PROC_ERROR_NONE;

	for (scan = g_list_last (comm->process->err.raw); scan; scan = scan->prev) {
		char *line = scan->data;

		if (strstr (line, "password incorrect") != NULL) {
			error->type = FR_PROC_ERROR_ASK_PASSWORD;
			break;
		}

		if (strncmp (line, "Unexpected end of archive", 25) == 0) {
			/* FIXME: handle this type of errors at a higher level when the freeze is over. */
		}

		if (strncmp (line, "Cannot find volume", 18) == 0) {
			char *volume_filename;

			g_clear_error (&error->gerror);

			error->type = FR_PROC_ERROR_MISSING_VOLUME;
			volume_filename = g_path_get_basename (line + strlen ("Cannot find volume "));
			error->gerror = g_error_new (FR_ERROR, error->status, _("Could not find the volume: %s"), volume_filename);
			g_free (volume_filename);
			break;
		}
	}
}


const char *rar_mime_type[] = { "application/x-cbr",
				"application/x-rar",
				NULL };


const char **
fr_command_rar_get_mime_types (FrCommand *comm)
{
	return rar_mime_type;
}


FrCommandCap
fr_command_rar_get_capabilities (FrCommand  *comm,
			         const char *mime_type,
				 gboolean    check_command)
{
	FrCommandCap capabilities;

	capabilities = FR_COMMAND_CAN_ARCHIVE_MANY_FILES | FR_COMMAND_CAN_ENCRYPT | FR_COMMAND_CAN_ENCRYPT_HEADER;
	if (is_program_available ("rar", check_command))
		capabilities |= FR_COMMAND_CAN_READ_WRITE | FR_COMMAND_CAN_CREATE_VOLUMES;
	else if (is_program_available ("unrar", check_command))
		capabilities |= FR_COMMAND_CAN_READ;

	/* multi-volumes are read-only */
	if ((comm->files->len > 0) && comm->multi_volume && (capabilities & FR_COMMAND_CAN_WRITE))
		capabilities ^= FR_COMMAND_CAN_WRITE;

	return capabilities;
}


static const char *
fr_command_rar_get_packages (FrCommand  *comm,
			     const char *mime_type)
{
	return PACKAGES ("rar,unrar");
}


static void
fr_command_rar_class_init (FrCommandRarClass *class)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (class);
	FrCommandClass *afc;

	parent_class = g_type_class_peek_parent (class);
	afc = (FrCommandClass*) class;

	gobject_class->finalize = fr_command_rar_finalize;

	afc->list             = fr_command_rar_list;
	afc->add              = fr_command_rar_add;
	afc->delete           = fr_command_rar_delete;
	afc->extract          = fr_command_rar_extract;
	afc->test             = fr_command_rar_test;
	afc->handle_error     = fr_command_rar_handle_error;
	afc->get_mime_types   = fr_command_rar_get_mime_types;
	afc->get_capabilities = fr_command_rar_get_capabilities;
	afc->get_packages     = fr_command_rar_get_packages;
}


static void
fr_command_rar_init (FrCommand *comm)
{
	comm->propAddCanUpdate             = TRUE;
	comm->propAddCanReplace            = TRUE;
	comm->propAddCanStoreFolders       = TRUE;
	comm->propExtractCanAvoidOverwrite = TRUE;
	comm->propExtractCanSkipOlder      = TRUE;
	comm->propExtractCanJunkPaths      = TRUE;
	comm->propPassword                 = TRUE;
	comm->propTest                     = TRUE;
	comm->propListFromFile             = TRUE;
}


static void
fr_command_rar_finalize (GObject *object)
{
	g_return_if_fail (object != NULL);
	g_return_if_fail (FR_IS_COMMAND_RAR (object));

	/* Chain up */
	if (G_OBJECT_CLASS (parent_class)->finalize)
		G_OBJECT_CLASS (parent_class)->finalize (object);
}


GType
fr_command_rar_get_type ()
{
	static GType type = 0;

	if (! type) {
		GTypeInfo type_info = {
			sizeof (FrCommandRarClass),
			NULL,
			NULL,
			(GClassInitFunc) fr_command_rar_class_init,
			NULL,
			NULL,
			sizeof (FrCommandRar),
			0,
			(GInstanceInitFunc) fr_command_rar_init
		};

		type = g_type_register_static (FR_TYPE_COMMAND,
					       "FRCommandRar",
					       &type_info,
					       0);
	}

	return type;
}

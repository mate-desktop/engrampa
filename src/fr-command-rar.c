/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  Engrampa
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#define _XOPEN_SOURCE 700

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
#include "rar-utils.h"


#define IS_OUTPUT_TYPE(comm, type) ((comm)->output_type & (type))


static void fr_command_rar_class_init  (FrCommandRarClass *class);
static void fr_command_rar_init        (FrCommand         *afile);
static void fr_command_rar_finalize    (GObject           *object);

/* Parent Class */

static FrCommandClass *parent_class = NULL;

/* rar 5.30 and later uses YYYY-MM-DD instead DD-MM-YY in the listing output */

static gboolean date_newstyle = FALSE;

static const struct {
	const gchar *command;
	FrCommandCaps capabilities;
} candidate_commands[] = {
	{ "rar", FR_COMMAND_CAN_READ_WRITE | FR_COMMAND_CAN_CREATE_VOLUMES },
	{ "unrar", FR_COMMAND_CAN_READ },
	{ "unrar-free", FR_COMMAND_CAN_READ }
};

static const gchar *
get_rar_command (void)
{
	for (guint i = 0; i < G_N_ELEMENTS(candidate_commands); i++) {
		if (is_program_in_path (candidate_commands[i].command))
			return candidate_commands[i].command;
	}

	/* this should never happen as we shouldn't get called if none of the
	 * candidate exist, but better safe than sorry */
	return "unrar";
}

/* -- list -- */

/*

// SAMPLE RAR VERSION 5.30 AND NEWER LISTING OUTPUT:

RAR 5.30   Copyright (c) 1993-2015 Alexander Roshal   18 Nov 2015
Trial version             Type RAR -? for help

Archive: /test.rar
Details: RAR 4

 Attributes      Size    Packed Ratio    Date    Time   Checksum  Name
----------- ---------  -------- ----- ---------- -----  --------  ----
 -rw-rw-r--      3165      1310  41%  2017-03-07 21:34  888D50B3  loremipsum.txt
 -rw-rw-r--         0         8   0%  2017-03-07 21:36  00000000  file2.empty
----------- ---------  -------- ----- ---------- -----  --------  ----
                 3165      1318  41%                              2

// SAMPLE RAR VERSION 5.00 TO 5.21 LISTING OUTPUT:

RAR 5.21   Copyright (c) 1993-2015 Alexander Roshal   15 Feb 2015
Trial version             Type RAR -? for help

Archive: /test.rar
Details: RAR 4

 Attributes      Size    Packed Ratio   Date   Time   Checksum  Name
----------- ---------  -------- ----- -------- -----  --------  ----
 -rw-rw-r--      3165      1310  41%  07-03-17 21:34  888D50B3  loremipsum.txt
 -rw-rw-r--         0         8   0%  07-03-17 21:36  00000000  file2.empty
----------- ---------  -------- ----- -------- -----  --------  ----
                 3165      1318  41%                            2

// SAMPLE RAR VERSION 4.20 AND OLDER LISTING OUTPUT:

RAR 4.20   Copyright (c) 1993-2012 Alexander Roshal   9 Jun 2012
Trial version             Type RAR -? for help

Archive /test.rar

Pathname/Comment
                  Size   Packed Ratio  Date   Time     Attr      CRC   Meth Ver
-------------------------------------------------------------------------------
 loremipsum.txt
                  3165     1310  41% 07-03-17 21:34 -rw-rw-r-- 888D50B3 m3b 2.9
 file2.empty
                     0        8   0% 07-03-17 21:36 -rw-rw-r-- 00000000 m3b 2.9
-------------------------------------------------------------------------------
    2             3165     1318  41%

*/

static time_t
mktime_from_string (const char *date_s,
		    const char *time_s)
{
	struct tm tm = {0, };
	char *date_time_s;

	tm.tm_isdst = -1;
	date_time_s = g_strjoin (" ", date_s, time_s, NULL);
	strptime (date_time_s, date_newstyle ? "%Y-%m-%d %H:%M" : "%d-%m-%y %H:%M", &tm);
	g_free (date_time_s);

	return mktime (&tm);
}

static gboolean
attribute_field_with_space (const char *line)
{
	/* sometimes when the archive is encrypted the attributes field is
	* like this: "* ..A...."
	* */
	return ((line[0] != ' ') && (line[1] == ' '));
}

static gboolean
attr_field_is_dir (const char   *attr_field,
                   FrCommandRar *rar_comm)
{
        if ((attr_field[0] == 'd') ||
            (IS_OUTPUT_TYPE (rar_comm, FR_COMMAND_RAR_TYPE_RAR5) && attr_field[3] == 'D') ||
            (! IS_OUTPUT_TYPE (rar_comm, FR_COMMAND_RAR_TYPE_RAR5) && attr_field[1] == 'D'))
                return TRUE;

        return FALSE;
}

static void
parse_name_field (char         *line,
		  FrCommandRar *rar_comm)
{
	char     *name_field;
	FileData *fdata;

	rar_comm->fdata = fdata = file_data_new ();

	/* read file name. */

	fdata->encrypted = (line[0] == '*') ? TRUE : FALSE;

	if (IS_OUTPUT_TYPE (rar_comm, FR_COMMAND_RAR_TYPE_RAR5)) {
		int field_num = attribute_field_with_space (line) ? 9 : 8;
		const char *field;

		/* in RAR7 format, directories have a blank CRC32, not the
		 * "????????" of the RAR5 format */
		if (IS_OUTPUT_TYPE (rar_comm, FR_COMMAND_RAR_TYPE_RAR7)) {
			field = get_last_field (line, field_num - 8);
			if (attr_field_is_dir (field, rar_comm))
				field_num--;
		}

		field = get_last_field (line, field_num);
		/* rar-5 output adds trailing spaces to short file names :( */
		name_field = field ? g_strchomp (g_strdup (field)) : NULL;
	} else
		name_field = g_strdup (line + 1);

	if (name_field == NULL)
		return;

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

	g_free (name_field);
}

static void
process_line (char     *line,
	      gpointer  data)
{
	FrCommand     *comm = FR_COMMAND (data);
	FrCommandRar  *rar_comm = FR_COMMAND_RAR (comm);
	char         **fields;

	g_return_if_fail (line != NULL);

	if (! rar_comm->list_started) {
		int version = 0;

		if (sscanf (line, "RAR %d.", &version) == 1 || sscanf (line, "UNRAR %d.", &version) == 1) {
			if (version >= 7)
				rar_comm->output_type = FR_COMMAND_RAR_TYPE_RAR7;
			else if (version >= 5)
				rar_comm->output_type = FR_COMMAND_RAR_TYPE_RAR5;
			else
				rar_comm->output_type = FR_COMMAND_RAR_TYPE_RAR4;

			if (version > 5)
				date_newstyle = TRUE;
			else if (version == 5 && (sscanf (line, "RAR 5.%d ", &version) == 1 ||
			                          sscanf (line, "UNRAR 5.%d ", &version) == 1))
			{
				if (version >= 30)
					date_newstyle = TRUE;
			}
		}
		else if (g_str_has_prefix (line, "unrar-free ")) {
			rar_comm->output_type = FR_COMMAND_RAR_TYPE_UNRAR_FREE;
			date_newstyle = FALSE;
		}
		else if (strncmp (line, "--------", 8) == 0) {
			rar_comm->list_started = TRUE;
			if (! IS_OUTPUT_TYPE (rar_comm, FR_COMMAND_RAR_TYPE_RAR5))
			    rar_comm->rar4_odd_line = TRUE;
		}
		else if (strncmp (line, "Volume ", 7) == 0)
			comm->multi_volume = TRUE;
		return;
	}

	if (strncmp (line, "--------", 8) == 0) {
		rar_comm->list_started = FALSE;
		return;
	}

	if (rar_comm->rar4_odd_line || IS_OUTPUT_TYPE (rar_comm, FR_COMMAND_RAR_TYPE_RAR5))
		parse_name_field (line, rar_comm);

	if (! rar_comm->rar4_odd_line && rar_comm->fdata && rar_comm->fdata->full_path) {
		FileData   *fdata;
		const char *size_field, *ratio_field, *date_field, *time_field, *attr_field;
		int n_fields;

		if (IS_OUTPUT_TYPE (rar_comm, FR_COMMAND_RAR_TYPE_RAR5))
			n_fields = 6 + (attribute_field_with_space (line) != 0);
		else if (IS_OUTPUT_TYPE (rar_comm, FR_COMMAND_RAR_TYPE_UNRAR_FREE))
			n_fields = 4;
		else
			n_fields = 6;

		fdata = rar_comm->fdata;

		/* read file info. */

		fields = split_line (line, n_fields);
		if (g_strv_length (fields) < (guint) n_fields) {
			/* wrong line format, treat this line as a filename line */
			g_strfreev (fields);
			file_data_free (rar_comm->fdata);
			rar_comm->fdata = NULL;
			rar_comm->rar4_odd_line = TRUE;
			parse_name_field (line, rar_comm);
		}
		else {
			if (IS_OUTPUT_TYPE (rar_comm, FR_COMMAND_RAR_TYPE_RAR5)) {
				int offset = attribute_field_with_space (line) ? 1 : 0;

				size_field = fields[1+offset];
				ratio_field = fields[3+offset];
				date_field = fields[4+offset];
				time_field = fields[5+offset];
				attr_field = fields[0+offset];
			}
			else if (IS_OUTPUT_TYPE (rar_comm, FR_COMMAND_RAR_TYPE_UNRAR_FREE)) {
				size_field = fields[0];
				date_field = fields[1];
				time_field = fields[2];
				attr_field = fields[3];
				ratio_field = "";
			}
			else {
				size_field = fields[0];
				ratio_field = fields[2];
				date_field = fields[3];
				time_field = fields[4];
				attr_field = fields[5];
			}

			if ((strcmp (ratio_field, "<->") == 0)
			    || (strcmp (ratio_field, "<--") == 0))
			{
				/* ignore files that span more volumes */

				file_data_free (rar_comm->fdata);
				rar_comm->fdata = NULL;
			}
			else {
				fdata->size = g_ascii_strtoull (size_field, NULL, 10);
				fdata->modified = mktime_from_string (date_field, time_field);

				if (attr_field_is_dir (attr_field, rar_comm)) {
					char *tmp;

					tmp = fdata->full_path;
					fdata->full_path = g_strconcat (fdata->full_path, "/", NULL);

					fdata->original_path = g_strdup (fdata->original_path);
					fdata->free_original_path = TRUE;

					g_free (tmp);

					fdata->name = dir_name_from_path (fdata->full_path);
					fdata->dir = TRUE;
				}
				else {
					fdata->name = g_strdup (file_name_from_path (fdata->full_path));
					if (attr_field[0] == 'l')
						fdata->link = g_strdup (file_name_from_path (fdata->full_path));
				}

				fr_command_add_file (comm, fdata);
				rar_comm->fdata = NULL;
			}

			g_strfreev (fields);
		}
	}

	if (! IS_OUTPUT_TYPE (rar_comm, FR_COMMAND_RAR_TYPE_RAR5))
		rar_comm->rar4_odd_line = ! rar_comm->rar4_odd_line;
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

static void
list__begin (gpointer data)
{
	FrCommandRar *comm = data;

	comm->list_started = FALSE;
}

static void
fr_command_rar_list (FrCommand  *comm)
{
	rar_check_multi_volume (comm);

	fr_process_set_out_line_func (comm->process, process_line, comm);

	fr_process_begin_command (comm->process, get_rar_command ());
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

static void
parse_progress_line (FrCommand  *comm,
		     const char *prefix,
		     const char *message_prefix,
		     const char *line)
{
	if (strncmp (line, prefix, strlen (prefix)) == 0)
		fr_command_progress (comm, (double) ++comm->n_file / (comm->n_files + 1));
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
			char  *volume_filename;
			GFile *volume_file;

			volume_filename = g_strdup (archive_filename);
			volume_filename[strlen (volume_filename) - 5] = '1';
			volume_file = g_file_new_for_path (volume_filename);
			fr_command_set_multi_volume (comm, volume_file);

			g_object_unref (volume_file);
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

	fr_process_begin_command (comm->process, get_rar_command ());

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
	fr_process_begin_command (comm->process, get_rar_command ());

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

		if ((strstr (line, "Incorrect password") != NULL) ||
		    (strstr (line, "password incorrect") != NULL) ||
		    (strstr (line, "password is incorrect") != NULL) ||
		    (strstr (line, "wrong password") != NULL)) {
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

			/* RAR7 complains about missing volume for incorrect passwords
			 * as well, so don't make this a definite error in case we also
			 * had an incorrect password one earlier */
			if (! IS_OUTPUT_TYPE (FR_COMMAND_RAR (comm), FR_COMMAND_RAR_TYPE_RAR7))
				break;
		}
	}
}

const char *rar_mime_type[] = { "application/x-cbr",
				"application/x-rar",
				NULL };

static const char **
fr_command_rar_get_mime_types (FrCommand *comm)
{
	return rar_mime_type;
}

static FrCommandCaps
fr_command_rar_get_capabilities (FrCommand  *comm,
			         const char *mime_type,
				 gboolean    check_command)
{
	FrCommandCaps capabilities;

	capabilities = FR_COMMAND_CAN_ARCHIVE_MANY_FILES | FR_COMMAND_CAN_ENCRYPT | FR_COMMAND_CAN_ENCRYPT_HEADER;
	/* add command-specific capabilities depending on availability */
	for (guint i = 0; i < G_N_ELEMENTS (candidate_commands); i++) {
		if (is_program_available (candidate_commands[i].command, check_command)) {
			capabilities |= candidate_commands[i].capabilities;
			break;
		}
	}

	/* multi-volumes are read-only */
	if ((comm->files->len > 0) && comm->multi_volume && (capabilities & FR_COMMAND_CAN_WRITE))
		capabilities ^= FR_COMMAND_CAN_WRITE;

	return capabilities;
}

static const char *
fr_command_rar_get_packages (FrCommand  *comm,
			     const char *mime_type)
{
	return PACKAGES ("rar,unrar,unrar-free");
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
			(GInstanceInitFunc) fr_command_rar_init,
			NULL
		};

		type = g_type_register_static (FR_TYPE_COMMAND,
					       "FRCommandRar",
					       &type_info,
					       0);
	}

	return type;
}

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
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307, USA.
 */

#include <config.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <glib.h>

#include "file-data.h"
#include "file-utils.h"
#include "glib-utils.h"
#include "fr-command.h"
#include "fr-command-rpm.h"

static void fr_command_rpm_class_init  (FrCommandRpmClass *class);
static void fr_command_rpm_init        (FrCommand         *afile);
static void fr_command_rpm_finalize    (GObject           *object);

/* Parent Class */

static FrCommandClass *parent_class = NULL;


/* -- list -- */

static time_t
mktime_from_string (char *month,
		    char *mday,
		    char *year)
{
	static char  *months[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
				   "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
	struct tm     tm = {0, };

	tm.tm_isdst = -1;

	if (month != NULL) {
		int i;
		for (i = 0; i < 12; i++)
			if (strcmp (months[i], month) == 0) {
				tm.tm_mon = i;
				break;
			}
	}
	tm.tm_mday = atoi (mday);
	if (strchr (year, ':') != NULL) {
		char **fields = g_strsplit (year, ":", 2);
        	if (n_fields (fields) == 2) {
	        	time_t      now;
        		struct tm  *now_tm;

	  		tm.tm_hour = atoi (fields[0]);
	  		tm.tm_min = atoi (fields[1]);

	  		now = time(NULL);
	  		now_tm = localtime (&now);
	  		tm.tm_year = now_tm->tm_year;
        	}
	} else
		tm.tm_year = atoi (year) - 1900;

	return mktime (&tm);
}


static void
list__process_line (char     *line,
		    gpointer  data)
{
	FileData    *fdata;
	FrCommand   *comm = FR_COMMAND (data);
	char       **fields;
	const char  *name_field;
	char        *name;
	int          ofs = 0;

	g_return_if_fail (line != NULL);

	fdata = file_data_new ();

#ifdef __sun
	fields = split_line (line, 9);
	fdata->size = g_ascii_strtoull (fields[4], NULL, 10);
	fdata->modified = mktime_from_string (fields[5], fields[6], fields[8]);
	g_strfreev (fields);

	name_field = get_last_field (line, 10);
#else /* !__sun */
	/* Handle char and block device files */
	if ((line[0] == 'c') || (line[0] == 'b')) {
		fields = split_line (line, 9);
		ofs = 1;
		fdata->size = 0;
		/* TODO We should also specify the content type */
	}
	else {
		fields = split_line (line, 8);
		fdata->size = g_ascii_strtoull (fields[4], NULL, 10);
	}
	fdata->modified = mktime_from_string (fields[5+ofs], fields[6+ofs], fields[7+ofs]);
	g_strfreev (fields);

	name_field = get_last_field (line, 9+ofs);
#endif /* !__sun */

	fields = g_strsplit (name_field, " -> ", 2);

	if (fields[1] == NULL) {
		g_strfreev (fields);
		fields = g_strsplit (name_field, " link to ", 2);
	}

	fdata->dir = line[0] == 'd';

	name = g_strcompress (fields[0]);
	if (*(fields[0]) == '/') {
		fdata->full_path = g_strdup (name);
		fdata->original_path = fdata->full_path;
	}
	else {
		fdata->full_path = g_strconcat ("/", name, NULL);
		fdata->original_path = fdata->full_path + 1;
	}
	if (fdata->dir && (name[strlen (name) - 1] != '/')) {
		char *old_full_path = fdata->full_path;
		fdata->full_path = g_strconcat (old_full_path, "/", NULL);
		g_free (old_full_path);
		fdata->original_path = g_strdup (name);
		fdata->free_original_path = TRUE;
	}
	g_free (name);

	if (fields[1] != NULL)
		fdata->link = g_strcompress (fields[1]);
	g_strfreev (fields);

	if (fdata->dir)
		fdata->name = dir_name_from_path (fdata->full_path);
	else
		fdata->name = g_strdup (file_name_from_path (fdata->full_path));
	fdata->path = remove_level_from_path (fdata->full_path);

	if (*fdata->name == 0)
		file_data_free (fdata);
	else
		fr_command_add_file (comm, fdata);
}


static void
fr_command_rpm_list (FrCommand *comm)
{
	fr_process_set_out_line_func (comm->process, list__process_line, comm);

	fr_process_begin_command (comm->process, "sh");
	fr_process_add_arg (comm->process, "-c");
	fr_process_add_arg_concat (comm->process, PRIVEXECDIR "rpm2cpio ", comm->e_filename, " -itv", NULL);
	fr_process_end_command (comm->process);
	fr_process_start (comm->process);
}


static void
fr_command_rpm_extract (FrCommand  *comm,
		        const char  *from_file,
			GList      *file_list,
			const char *dest_dir,
			gboolean    overwrite,
			gboolean    skip_older,
			gboolean    junk_paths)
{
	GList   *scan;
	GString *cmd;

	fr_process_begin_command (comm->process, "sh");
	if (dest_dir != NULL)
                fr_process_set_working_dir (comm->process, dest_dir);
	fr_process_add_arg (comm->process, "-c");

	cmd = g_string_new (PRIVEXECDIR "rpm2cpio ");
	g_string_append (cmd, comm->e_filename);
	g_string_append (cmd, " -idu ");
	for (scan = file_list; scan; scan = scan->next) {
		char *filename = g_shell_quote (scan->data);
		g_string_append (cmd, filename);
		g_free (filename);
		g_string_append (cmd, " ");
	}
	fr_process_add_arg (comm->process, cmd->str);
	g_string_free (cmd, TRUE);

	fr_process_end_command (comm->process);
	fr_process_start (comm->process);
}


const char *rpm_mime_type[] = { "application/x-rpm", NULL };


const char **
fr_command_rpm_get_mime_types (FrCommand *comm)
{
	return rpm_mime_type;
}


FrCommandCap
fr_command_rpm_get_capabilities (FrCommand  *comm,
			         const char *mime_type,
				 gboolean    check_command)
{
	FrCommandCap capabilities;

	capabilities = FR_COMMAND_CAN_ARCHIVE_MANY_FILES;
	if (is_program_available ("cpio", check_command))
		capabilities |= FR_COMMAND_CAN_READ;

	return capabilities;
}


static const char *
fr_command_rpm_get_packages (FrCommand  *comm,
			     const char *mime_type)
{
	return PACKAGES ("cpio,rpm");
}


static void
fr_command_rpm_class_init (FrCommandRpmClass *class)
{
        GObjectClass   *gobject_class = G_OBJECT_CLASS (class);
        FrCommandClass *afc;

        parent_class = g_type_class_peek_parent (class);
	afc = (FrCommandClass*) class;

	gobject_class->finalize = fr_command_rpm_finalize;

        afc->list             = fr_command_rpm_list;
	afc->extract          = fr_command_rpm_extract;
	afc->get_mime_types   = fr_command_rpm_get_mime_types;
	afc->get_capabilities = fr_command_rpm_get_capabilities;
	afc->get_packages     = fr_command_rpm_get_packages;
}


static void
fr_command_rpm_init (FrCommand *comm)
{
	comm->propAddCanUpdate             = FALSE;
	comm->propAddCanReplace            = FALSE;
	comm->propExtractCanAvoidOverwrite = FALSE;
	comm->propExtractCanSkipOlder      = FALSE;
	comm->propExtractCanJunkPaths      = FALSE;
	comm->propPassword                 = FALSE;
	comm->propTest                     = FALSE;
}


static void
fr_command_rpm_finalize (GObject *object)
{
        g_return_if_fail (object != NULL);
        g_return_if_fail (FR_IS_COMMAND_RPM (object));

	/* Chain up */
        if (G_OBJECT_CLASS (parent_class)->finalize)
		G_OBJECT_CLASS (parent_class)->finalize (object);
}


GType
fr_command_rpm_get_type ()
{
        static GType type = 0;

        if (! type) {
                GTypeInfo type_info = {
			sizeof (FrCommandRpmClass),
			NULL,
			NULL,
			(GClassInitFunc) fr_command_rpm_class_init,
			NULL,
			NULL,
			sizeof (FrCommandRpm),
			0,
			(GInstanceInitFunc) fr_command_rpm_init
		};

		type = g_type_register_static (FR_TYPE_COMMAND,
					       "FRCommandRpm",
					       &type_info,
					       0);
        }

        return type;
}

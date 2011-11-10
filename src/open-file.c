/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  File-Roller
 *
 *  Copyright (C) 2001-2008 The Free Software Foundation, Inc.
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
#include "open-file.h"
#include "file-utils.h"


OpenFile*
open_file_new (const char *path,
	       const char *extracted_path,
	       const char *temp_dir)
{
	OpenFile *ofile;
	
	ofile = g_new0 (OpenFile, 1);
	ofile->path = g_strdup (path);
	ofile->extracted_uri = g_filename_to_uri (extracted_path, NULL, NULL);
	if (! uri_exists (ofile->extracted_uri)) {
		open_file_free (ofile);
		return NULL;
	} 
	ofile->temp_dir = g_strdup (temp_dir);
	ofile->last_modified = get_file_mtime (ofile->extracted_uri);
	
	return ofile;
}


void
open_file_free (OpenFile *ofile)
{
	if (ofile == NULL)
		return;
	if (ofile->monitor != NULL)
		g_object_unref (ofile->monitor);
	g_free (ofile->path);
	g_free (ofile->extracted_uri);
	g_free (ofile->temp_dir);
	g_free (ofile);
}


OpenFile *
open_file_copy (OpenFile *src)
{
	OpenFile *ofile;

	ofile = g_new0 (OpenFile, 1);
	ofile->path = g_strdup (src->path);
	ofile->extracted_uri = g_strdup (src->extracted_uri);
	ofile->temp_dir = g_strdup (src->temp_dir);
	ofile->last_modified = src->last_modified;

	return ofile;
}


GType
open_file_get_type (void)
{
	static GType type = 0;
  
	if (type == 0)
		type = g_boxed_type_register_static ("FROpenFile", (GBoxedCopyFunc) open_file_copy, (GBoxedFreeFunc) open_file_free);
  
	return type;
}

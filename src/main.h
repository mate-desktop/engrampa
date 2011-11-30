/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  Engrampa
 *
 *  Copyright (C) 2001, 2008 The Free Software Foundation, Inc.
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

#ifndef MAIN_H
#define MAIN_H


#include <glib.h>
#include <glib/gi18n.h>
#include <gio/gio.h>
#include "preferences.h"
#include "fr-process.h"
#include "fr-window.h"


typedef struct {
	FrWindow  *window;
	FrProcess *process;
	char      *filename;
	char      *e_filename;
	char      *temp_dir;
} ViewerData;

typedef struct {
	FrWindow  *window;
	FrProcess *process;
	char      *command;
	GAppInfo  *app;
	GList     *file_list;
	char      *temp_dir;
} CommandData;


void  viewer_done      (ViewerData  *vdata);
void  command_done     (CommandData *cdata);


extern GList       *WindowList;
extern GList       *CommandList;
extern gint         ForceDirectoryCreation;
extern GHashTable  *ProgramsCache;
extern GPtrArray   *Registered_Commands;

extern FrMimeTypeDescription mime_type_desc[];
extern FrExtensionType file_ext_type[];
extern int single_file_save_type[]; /* File types that can be saved when
				     * a single file is selected.
				     * Includes single file compressors
				     * such as gzip, compress, etc. */
extern int save_type[];             /* File types that can be saved. */
extern int open_type[];             /* File types that can be opened. */
extern int create_type[];           /* File types that can be created. */

GType        get_command_type_from_mime_type         (const char    *mime_type,
						      FrCommandCaps  requested_capabilities);
GType        get_preferred_command_for_mime_type     (const char    *mime_type,
						      FrCommandCaps  requested_capabilities);
void         update_registered_commands_capabilities (void);
const char * get_mime_type_from_extension            (const char    *ext);
const char * get_archive_filename_extension          (const char    *uri);
int          get_mime_type_index                     (const char    *mime_type);
void         sort_mime_types_by_extension            (int           *a);
void         sort_mime_types_by_description          (int           *a);
		                              
#endif /* MAIN_H */

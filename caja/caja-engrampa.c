/*
 *  Engrampa
 *
 *  Copyright (C) 2004 Free Software Foundation, Inc.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *  Author: Paolo Bacchilega <paobac@cvs.mate.org>
 *
 */

#include <config.h>
#include <string.h>
#include <glib/gi18n-lib.h>
#include <gio/gio.h>
#include <libcaja-extension/caja-extension-types.h>
#include <libcaja-extension/caja-file-info.h>
#include <libcaja-extension/caja-menu-provider.h>
#include "caja-engrampa.h"


static GObjectClass *parent_class;


static void
extract_to_callback (CajaMenuItem *item,
		     gpointer          user_data)
{
	GList            *files, *scan;
	CajaFileInfo *file;
	char             *default_dir;
	char             *quoted_default_dir;
	GString          *cmd;

	files = g_object_get_data (G_OBJECT (item), "files");
	file = files->data;

	default_dir = caja_file_info_get_parent_uri (file);

	quoted_default_dir = g_shell_quote (default_dir);

	cmd = g_string_new ("engrampa");
	g_string_append_printf(cmd," --default-dir=%s --extract", quoted_default_dir);

	for (scan = files; scan; scan = scan->next) {
		CajaFileInfo *file = scan->data;
		char             *uri, *quoted_uri;

		uri = caja_file_info_get_uri (file);
		quoted_uri = g_shell_quote (uri);
		g_string_append_printf (cmd, " %s", quoted_uri);
		g_free (uri);
		g_free (quoted_uri);
	}

#ifdef DEBUG
	g_print ("EXEC: %s\n", cmd->str);
#endif

	g_spawn_command_line_async (cmd->str, NULL);

	g_string_free (cmd, TRUE);
	g_free (default_dir);
	g_free (quoted_default_dir);
}


static void
extract_here_callback (CajaMenuItem *item,
		       gpointer          user_data)
{
	GList            *files, *scan;
	GString          *cmd;

	files = g_object_get_data (G_OBJECT (item), "files");

	cmd = g_string_new ("engrampa --extract-here");

	for (scan = files; scan; scan = scan->next) {
		CajaFileInfo *file = scan->data;
		char             *uri, *quoted_uri;

		uri = caja_file_info_get_uri (file);
		quoted_uri = g_shell_quote (uri);
		g_string_append_printf (cmd, " %s", quoted_uri);
		g_free (uri);
		g_free (quoted_uri);
	}

	g_spawn_command_line_async (cmd->str, NULL);

#ifdef DEBUG
	g_print ("EXEC: %s\n", cmd->str);
#endif

	g_string_free (cmd, TRUE);
}


static void
add_callback (CajaMenuItem *item,
	      gpointer          user_data)
{
	GList            *files, *scan;
	CajaFileInfo *file;
	char             *uri, *dir;
	char             *quoted_uri, *quoted_dir;
	GString          *cmd;

	files = g_object_get_data (G_OBJECT (item), "files");
	file = files->data;

	uri = caja_file_info_get_uri (file);
	dir = g_path_get_dirname (uri);
	quoted_dir = g_shell_quote (dir);

	cmd = g_string_new ("engrampa");
	g_string_append_printf (cmd," --default-dir=%s --add", quoted_dir);

	g_free (uri);
	g_free (dir);
	g_free (quoted_dir);

	for (scan = files; scan; scan = scan->next) {
		CajaFileInfo *file = scan->data;

		uri = caja_file_info_get_uri (file);
		quoted_uri = g_shell_quote (uri);
		g_string_append_printf (cmd, " %s", quoted_uri);
		g_free (uri);
		g_free (quoted_uri);
	}

	g_spawn_command_line_async (cmd->str, NULL);

	g_string_free (cmd, TRUE);
}


static struct {
	char     *mime_type;
	gboolean  is_compressed;
} archive_mime_types[] = {
		{ "application/vnd.ms-cab-compressed", TRUE },
		{ "application/x-7z-compressed", TRUE },
		{ "application/x-7z-compressed-tar", TRUE },
		{ "application/x-ace", TRUE },
		{ "application/x-alz", TRUE },
		{ "application/x-ar", TRUE },
		{ "application/x-arj", TRUE },
		{ "application/x-brotli", TRUE },
		{ "application/x-brotli-compressed-tar", TRUE },
		{ "application/x-bzip", TRUE },
		{ "application/x-bzip-compressed-tar", TRUE },
		{ "application/x-bzip1", TRUE },
		{ "application/x-bzip1-compressed-tar", TRUE },
		{ "application/x-cbr", TRUE },
		{ "application/x-cbz", TRUE },
		{ "application/x-cd-image", FALSE },
		{ "application/x-compress", TRUE },
		{ "application/x-compressed-tar", TRUE },
		{ "application/x-cpio", TRUE },
		{ "application/x-deb", TRUE },
		{ "application/x-ear", TRUE },
		{ "application/x-gtar", FALSE },
		{ "application/x-gzip", TRUE },
		{ "application/x-gzpostscript", TRUE },
		{ "application/x-java-archive", TRUE },
		{ "application/x-lha", TRUE },
		{ "application/x-lhz", TRUE },
		{ "application/x-lzip", TRUE },
		{ "application/x-lzip-compressed-tar", TRUE },
		{ "application/x-lzma", TRUE },
		{ "application/x-lzma-compressed-tar", TRUE },
		{ "application/x-lzop", TRUE },
		{ "application/x-lzop-compressed-tar", TRUE },
		{ "application/x-ms-dos-executable", FALSE },
		{ "application/x-ms-wim", TRUE },
		{ "application/x-rar", TRUE },
		{ "application/x-rar-compressed", TRUE },
		{ "application/x-rpm", TRUE },
		{ "application/x-rzip", TRUE },
		{ "application/x-stuffit", TRUE },
		{ "application/x-tar", FALSE },
		{ "application/x-tarz", TRUE },
		{ "application/x-war", TRUE },
		{ "application/x-xz", TRUE },
		{ "application/x-xz-compressed-tar", TRUE },
		{ "application/x-zip", TRUE },
		{ "application/x-zip-compressed", TRUE },
		{ "application/x-zoo", TRUE },
		{ "application/zip", TRUE },
		{ "multipart/x-zip", TRUE },
		{ NULL, FALSE }
};


typedef struct {
      gboolean is_archive;
      gboolean is_derived_archive;
      gboolean is_compressed_archive;
} FileMimeInfo;


static FileMimeInfo
get_file_mime_info (CajaFileInfo *file)
{
	FileMimeInfo file_mime_info;
	int          i;

	file_mime_info.is_archive = FALSE;
	file_mime_info.is_derived_archive = FALSE;
	file_mime_info.is_compressed_archive = FALSE;

	for (i = 0; archive_mime_types[i].mime_type != NULL; i++)
		if (caja_file_info_is_mime_type (file, archive_mime_types[i].mime_type)) {
			char *mime_type;
			char *content_type_mime_file;
			char *content_type_mime_compare;

			mime_type = caja_file_info_get_mime_type (file);

			content_type_mime_file = g_content_type_from_mime_type (mime_type);
			content_type_mime_compare = g_content_type_from_mime_type (archive_mime_types[i].mime_type);

			file_mime_info.is_archive = TRUE;
			file_mime_info.is_compressed_archive = archive_mime_types[i].is_compressed;
			if ((content_type_mime_file != NULL) && (content_type_mime_compare != NULL))
				file_mime_info.is_derived_archive = ! g_content_type_equals (content_type_mime_file, content_type_mime_compare);

			g_free (mime_type);
			g_free (content_type_mime_file);
			g_free (content_type_mime_compare);

			return file_mime_info;
		}

	return file_mime_info;
}


static gboolean
unsupported_scheme (CajaFileInfo *file)
{
	gboolean  result = FALSE;
	GFile    *location;
	char     *scheme;

	location = caja_file_info_get_location (file);
	scheme = g_file_get_uri_scheme (location);

	if (scheme != NULL) {
		const char *unsupported[] = { "trash", "computer", NULL };
		int         i;

		for (i = 0; unsupported[i] != NULL; i++)
			if (strcmp (scheme, unsupported[i]) == 0)
				result = TRUE;
	}

	g_free (scheme);
	g_object_unref (location);

	return result;
}


static GList *
caja_fr_get_file_items (CajaMenuProvider *provider,
			    GtkWidget            *window,
			    GList                *files)
{
	GList    *items = NULL;
	GList    *scan;
	gboolean  can_write = TRUE;
	gboolean  one_item;
	gboolean  one_archive = FALSE;
	gboolean  one_derived_archive = FALSE;
	gboolean  one_compressed_archive = FALSE;
	gboolean  all_archives = TRUE;
	gboolean  all_archives_derived = TRUE;
	gboolean  all_archives_compressed = TRUE;

	if (files == NULL)
		return NULL;

	if (unsupported_scheme ((CajaFileInfo *) files->data))
		return NULL;

	for (scan = files; scan; scan = scan->next) {
		CajaFileInfo *file = scan->data;
		FileMimeInfo      file_mime_info;

		file_mime_info = get_file_mime_info (file);

		if (all_archives && ! file_mime_info.is_archive)
			all_archives = FALSE;

		if (all_archives_compressed && file_mime_info.is_archive && ! file_mime_info.is_compressed_archive)
			all_archives_compressed = FALSE;

		if (all_archives_derived && file_mime_info.is_archive && ! file_mime_info.is_derived_archive)
			all_archives_derived = FALSE;

		if (can_write) {
			CajaFileInfo *parent;

			parent = caja_file_info_get_parent_info (file);
 			can_write = caja_file_info_can_write (parent);
			g_object_unref (parent);
		}
	}

	/**/

	one_item = (files != NULL) && (files->next == NULL);
	one_archive = one_item && all_archives;
	one_derived_archive = one_archive && all_archives_derived;
	one_compressed_archive = one_archive && all_archives_compressed;

	if (all_archives && can_write) {
		CajaMenuItem *item;

		item = caja_menu_item_new ("CajaFr::extract_here",
					       _("Extract Here"),
					       /* Translators: the current position is the current folder */
					       _("Extract the selected archive to the current position"),
					       "drive-harddisk");
		g_signal_connect (item,
				  "activate",
				  G_CALLBACK (extract_here_callback),
				  provider);
		g_object_set_data_full (G_OBJECT (item),
					"files",
					caja_file_info_list_copy (files),
					(GDestroyNotify) caja_file_info_list_free);

		items = g_list_append (items, item);
	}
	if (all_archives) {
		CajaMenuItem *item;

		item = caja_menu_item_new ("CajaFr::extract_to",
					       _("Extract To..."),
					       _("Extract the selected archive"),
					       "drive-harddisk");
		g_signal_connect (item,
				  "activate",
				  G_CALLBACK (extract_to_callback),
				  provider);
		g_object_set_data_full (G_OBJECT (item),
					"files",
					caja_file_info_list_copy (files),
					(GDestroyNotify) caja_file_info_list_free);

		items = g_list_append (items, item);

	}

	if (! one_compressed_archive || one_derived_archive) {
		CajaMenuItem *item;

		item = caja_menu_item_new ("CajaFr::add",
					       _("Compress..."),
					       _("Create a compressed archive with the selected objects"),
					       "mate-mime-application-x-archive");
		g_signal_connect (item,
				  "activate",
				  G_CALLBACK (add_callback),
				  provider);
		g_object_set_data_full (G_OBJECT (item),
					"files",
					caja_file_info_list_copy (files),
					(GDestroyNotify) caja_file_info_list_free);

		items = g_list_append (items, item);
	}

	return items;
}


static void
caja_fr_menu_provider_iface_init (CajaMenuProviderIface *iface)
{
	iface->get_file_items = caja_fr_get_file_items;
}


static void
caja_fr_instance_init (CajaFr *fr)
{
}


static void
caja_fr_class_init (CajaFrClass *class)
{
	parent_class = g_type_class_peek_parent (class);
}


static GType fr_type = 0;


GType
caja_fr_get_type (void)
{
	return fr_type;
}


void
caja_fr_register_type (GTypeModule *module)
{
	static const GTypeInfo info = {
		sizeof (CajaFrClass),
		(GBaseInitFunc) NULL,
		(GBaseFinalizeFunc) NULL,
		(GClassInitFunc) caja_fr_class_init,
		NULL,
		NULL,
		sizeof (CajaFr),
		0,
		(GInstanceInitFunc) caja_fr_instance_init,
	};

	static const GInterfaceInfo menu_provider_iface_info = {
		(GInterfaceInitFunc) caja_fr_menu_provider_iface_init,
		NULL,
		NULL
	};

	fr_type = g_type_module_register_type (module,
					       G_TYPE_OBJECT,
					       "CajaEngrampa",
					       &info, 0);

	g_type_module_add_interface (module,
				     fr_type,
				     CAJA_TYPE_MENU_PROVIDER,
				     &menu_provider_iface_info);
}

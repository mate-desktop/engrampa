/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  Engrampa
 *
 *  Copyright (C) 2010 Free Software Foundation, Inc.
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
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <config.h>
#include <stdlib.h>
#include <glib/gi18n.h>
#include <glib.h>
#include <gio/gio.h>
#include "file-utils.h"
#include "fr-init.h"


#define FR_SERVICE_NAME "org.mate.Engrampa"
#define FR_SERVICE_PATH "/org/mate/Engrampa"


static const char introspection_xml[] =
	"<node>"
	"  <interface name='org.mate.ArchiveManager'>"

	/**
	 * GetSupportedTypes:
	 *
	 * Returns the supported archive types for a specific action.
	 *
	 * Input arguments:
	 *
	 * @action:
	 *   Can be one of the following values:
	 *   *) create: create an archive that can contain many files.
	 *   *) create_single_file: create an archive that can contain a single file.
	 *   *) extract: extract the content of an archive.
	 *
	 * Output arguments:
	 *
	 * @types:
	 *   The supported archive types described as an array of hash tables,
	 *   where each hash table has the following keys:
	 *   *) mime-type: the mime type relative to the archive type.
	 *   *) default-extension: the extension to use for newly created archives.
	 *   *) description: a human readable description of the archive type.
	 */

	"    <method name='GetSupportedTypes'>"
	"      <arg name='action' type='s' direction='in'/>"
	"      <arg name='types' type='aa{ss}' direction='out'/>"
	"    </method>"

	/**
	 * AddToArchive:
	 *
	 * Adds the specified files to an archive.  If the archive already
	 * exists the archive is updated.
	 *
	 * Input arguments:
	 *
	 * @archive:
	 *   The archive URI.
	 * @files:
	 *   The files to add to the archive, as an array of URIs.
	 * @use_progress_dialog:
	 *   Whether to show the progress dialog.
	 */

	"    <method name='AddToArchive'>"
	"      <arg name='archive' type='s' direction='in'/>"
	"      <arg name='files' type='as' direction='in'/>"
	"      <arg name='use_progress_dialog' type='b' direction='in'/>"
	"    </method>"

	/**
	 * Compress:
	 *
	 * Compresses a series of files in an archive. The user is asked to
	 * enter an archive name, archive type and other options.  In this case
	 * it's used the same dialog used by the "Compress..." command from the
	 * Nautilus context menu.
	 * If the user chooses an existing archive, the archive is updated.
	 *
	 * Input arguments:
	 *
	 * @files:
	 *   The files to add to the archive, as an array of URIs.
	 * @destination:
	 *   An optional destination, if not specified the folder of the first
	 *   file in @files is used.
	 * @use_progress_dialog:
	 *   Whether to show the progress dialog.
	 */

	"    <method name='Compress'>"
	"      <arg name='files' type='as' direction='in'/>"
	"      <arg name='destination' type='s' direction='in'/>"
	"      <arg name='use_progress_dialog' type='b' direction='in'/>"
	"    </method>"

	/**
	 * Extract:
	 *
	 * Extract an archive in a specified location.
	 *
	 * Input arguments:
	 *
	 * @archive:
	 *   The archive to extract.
	 * @destination:
	 *   The location where to extract the archive.
	 * @use_progress_dialog:
	 *   Whether to show the progress dialog.
	 */

	"    <method name='Extract'>"
	"      <arg name='archive' type='s' direction='in'/>"
	"      <arg name='destination' type='s' direction='in'/>"
	"      <arg name='use_progress_dialog' type='b' direction='in'/>"
	"    </method>"

	/**
	 * ExtractHere:
	 *
	 * Extract an archive in a specified location.
	 *
	 * Input arguments:
	 *
	 * @archive:
	 *   The archive to extract.
	 * @use_progress_dialog:
	 *   Whether to show the progress dialog.
	 */

	"    <method name='ExtractHere'>"
	"      <arg name='archive' type='s' direction='in'/>"
	"      <arg name='use_progress_dialog' type='b' direction='in'/>"
	"    </method>"

	/**
	 * Progress (signal)
	 *
	 * Arguments:
	 *
	 * @fraction:
	 *   number from 0.0 to 100.0 that indicates the percentage of
	 *   completion of the operation.
	 * @details:
	 *   text message that describes the current operation.
	 */

	"    <signal name='Progress'>"
	"      <arg name='fraction' type='d'/>"
	"      <arg name='details' type='s'/>"
	"    </signal>"

	"  </interface>"
	"</node>";

static GDBusNodeInfo *introspection_data = NULL;


static void
window_ready_cb (GtkWidget *widget,
		 GError    *error,
		 gpointer   user_data)
{
	if (error == NULL)
		g_dbus_method_invocation_return_value ((GDBusMethodInvocation *) user_data, NULL);
	else
		g_dbus_method_invocation_return_error ((GDBusMethodInvocation *) user_data,
						       error->domain,
						       error->code,
						       "%s",
						       error->message);
}


static gboolean
window_progress_cb (FrWindow *window,
		    double    fraction,
		    char     *details,
		    gpointer  user_data)
{
	GDBusConnection *connection = user_data;

	g_dbus_connection_emit_signal (connection,
				       NULL,
				       FR_SERVICE_PATH,
				       "org.mate.ArchiveManager",
				       "Progress",
				       g_variant_new ("(ds)",
						      fraction,
						      details),
				       NULL);

	return TRUE;
}


static void
handle_method_call (GDBusConnection       *connection,
		    const char            *sender,
		    const char            *object_path,
		    const char            *interface_name,
		    const char            *method_name,
		    GVariant              *parameters,
		    GDBusMethodInvocation *invocation,
		    gpointer               user_data)
{
	update_registered_commands_capabilities ();

	if (g_strcmp0 (method_name, "GetSupportedTypes") == 0) {
		char *action;
		int  *supported_types = NULL;

		g_variant_get (parameters, "(s)", &action);

		if (g_strcmp0 (action, "create") == 0) {
			supported_types = save_type;
		}
		else if (g_strcmp0 (action, "create_single_file") == 0) {
			supported_types = single_file_save_type;
		}
		else if (g_strcmp0 (action, "extract") == 0) {
			supported_types = open_type;
		}

		if (supported_types == NULL) {
			g_dbus_method_invocation_return_error (invocation,
							       G_IO_ERROR,
							       G_IO_ERROR_INVALID_ARGUMENT,
							       "Action not valid %s, valid values are: create, create_single_file, extract",
							       action);
		}
		else {
			GVariantBuilder builder;
			int             i;

			g_variant_builder_init (&builder, G_VARIANT_TYPE ("(aa{ss})"));
			g_variant_builder_open (&builder, G_VARIANT_TYPE ("aa{ss}"));
			for (i = 0; supported_types[i] != -1; i++) {
				g_variant_builder_open (&builder, G_VARIANT_TYPE ("a{ss}"));
				g_variant_builder_add (&builder, "{ss}",
						       "mime-type",
						       mime_type_desc[supported_types[i]].mime_type);
				g_variant_builder_add (&builder, "{ss}",
						       "default-extension",
						       mime_type_desc[supported_types[i]].default_ext);
				g_variant_builder_add (&builder, "{ss}",
						       "description",
						       _(mime_type_desc[supported_types[i]].name));
				g_variant_builder_close (&builder);
			}
			g_variant_builder_close (&builder);

			g_dbus_method_invocation_return_value (invocation, g_variant_builder_end (&builder));
		}

		g_free (action);
	}
	else if (g_strcmp0 (method_name, "AddToArchive") == 0) {
		char       *archive;
		char      **files;
		gboolean    use_progress_dialog;
		int         i;
		GList      *file_list = NULL;
		GtkWidget  *window;

		g_variant_get (parameters, "(s^asb)", &archive, &files, &use_progress_dialog);

		for (i = 0; files[i] != NULL; i++)
			file_list = g_list_prepend (file_list, files[i]);
		file_list = g_list_reverse (file_list);

		window = fr_window_new ();
		fr_window_use_progress_dialog (FR_WINDOW (window), use_progress_dialog);

		g_signal_connect (window, "progress", G_CALLBACK (window_progress_cb), connection);
		g_signal_connect (window, "ready", G_CALLBACK (window_ready_cb), invocation);

		fr_window_new_batch (FR_WINDOW (window), _("Compress"));
		fr_window_set_batch__add (FR_WINDOW (window), archive, file_list);
		fr_window_append_batch_action (FR_WINDOW (window), FR_BATCH_ACTION_QUIT, NULL, NULL);
		fr_window_start_batch (FR_WINDOW (window));

		g_free (archive);

		gtk_main ();
	}
	else if (g_strcmp0 (method_name, "Compress") == 0) {
		char      **files;
		char       *destination;
		gboolean    use_progress_dialog;
		int         i;
		GList      *file_list = NULL;
		GtkWidget  *window;

		g_variant_get (parameters, "(^assb)", &files, &destination, &use_progress_dialog);

		for (i = 0; files[i] != NULL; i++)
			file_list = g_list_prepend (file_list, files[i]);
		file_list = g_list_reverse (file_list);

		if ((destination == NULL) || (strcmp (destination, "") == 0))
			destination = remove_level_from_path (file_list->data);

		window = fr_window_new ();
		fr_window_use_progress_dialog (FR_WINDOW (window), use_progress_dialog);
		fr_window_set_default_dir (FR_WINDOW (window), destination, TRUE);

		g_signal_connect (window, "progress", G_CALLBACK (window_progress_cb), connection);
		g_signal_connect (window, "ready", G_CALLBACK (window_ready_cb), invocation);

		fr_window_new_batch (FR_WINDOW (window), _("Extract archive"));
		fr_window_set_batch__add (FR_WINDOW (window), NULL, file_list);
		fr_window_append_batch_action (FR_WINDOW (window), FR_BATCH_ACTION_QUIT, NULL, NULL);
		fr_window_start_batch (FR_WINDOW (window));

		g_free (destination);

		gtk_main ();
	}
	else if (g_strcmp0 (method_name, "Extract") == 0) {
		char      *archive;
		char      *destination;
		gboolean   use_progress_dialog;
		GtkWidget *window;

		g_variant_get (parameters, "(ssb)", &archive, &destination, &use_progress_dialog);

		window = fr_window_new ();
		fr_window_use_progress_dialog (FR_WINDOW (window), use_progress_dialog);
		if ((destination != NULL) && (strcmp (destination, "") != 0))
			fr_window_set_default_dir (FR_WINDOW (window), destination, TRUE);

		g_signal_connect (window, "progress", G_CALLBACK (window_progress_cb), connection);
		g_signal_connect (window, "ready", G_CALLBACK (window_ready_cb), invocation);

		fr_window_new_batch (FR_WINDOW (window), _("Extract archive"));
		fr_window_set_batch__extract (FR_WINDOW (window), archive, destination);
		fr_window_append_batch_action (FR_WINDOW (window), FR_BATCH_ACTION_QUIT, NULL, NULL);
		fr_window_start_batch (FR_WINDOW (window));

		g_free (destination);
		g_free (archive);

		gtk_main ();
	}
	else if (g_strcmp0 (method_name, "ExtractHere") == 0) {
		char      *archive;
		gboolean   use_progress_dialog;
		GtkWidget *window;

		g_variant_get (parameters, "(sb)", &archive, &use_progress_dialog);

		window = fr_window_new ();
		fr_window_use_progress_dialog (FR_WINDOW (window), use_progress_dialog);

		g_signal_connect (window, "progress", G_CALLBACK (window_progress_cb), connection);
		g_signal_connect (window, "ready", G_CALLBACK (window_ready_cb), invocation);

		fr_window_new_batch (FR_WINDOW (window), _("Extract archive"));
		fr_window_set_batch__extract_here (FR_WINDOW (window), archive);
		fr_window_append_batch_action (FR_WINDOW (window), FR_BATCH_ACTION_QUIT, NULL, NULL);
		fr_window_start_batch (FR_WINDOW (window));

		g_free (archive);

		gtk_main ();
	}
}


static const GDBusInterfaceVTable interface_vtable = {
	handle_method_call,
	NULL, 			/* handle_get_property */
	NULL 			/* handle_set_property */
};


static void
on_bus_acquired (GDBusConnection *connection,
		 const char      *name,
		 gpointer         user_data)
{
	guint registration_id;

	registration_id = g_dbus_connection_register_object (connection,
							     FR_SERVICE_PATH,
							     introspection_data->interfaces[0],
							     &interface_vtable,
							     NULL,
							     NULL,  /* user_data_free_func */
							     NULL); /* GError** */
	g_assert (registration_id > 0);

	initialize_data ();
}


static void
on_name_acquired (GDBusConnection *connection,
		  const char      *name,
		  gpointer         user_data)
{
}


static void
on_name_lost (GDBusConnection *connection,
	      const char      *name,
	      gpointer         user_data)
{
	gtk_main_quit ();
}


int
main (int argc, char *argv[])
{
	GOptionContext *context = NULL;
	GError         *error = NULL;
	guint           owner_id;

	bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);

	context = g_option_context_new (N_("- Create and modify an archive"));
	g_option_context_set_translation_domain (context, GETTEXT_PACKAGE);
	g_option_context_add_group (context, gtk_get_option_group (TRUE));

	if (! g_option_context_parse (context, &argc, &argv, &error)) {
		g_critical ("Failed to parse arguments: %s", error->message);
		g_error_free (error);
		g_option_context_free (context);
		return EXIT_FAILURE;
	}

	g_option_context_free (context);

	g_set_application_name (_("Engrampa"));
	gtk_window_set_default_icon_name ("engrampa");

	gtk_icon_theme_append_search_path (gtk_icon_theme_get_default (),
					   PKG_DATA_DIR G_DIR_SEPARATOR_S "icons");

	introspection_data = g_dbus_node_info_new_for_xml (introspection_xml, NULL);
	g_assert (introspection_data != NULL);

	owner_id = g_bus_own_name (G_BUS_TYPE_SESSION,
				   FR_SERVICE_NAME,
				   G_BUS_NAME_OWNER_FLAGS_NONE,
				   on_bus_acquired,
				   on_name_acquired,
				   on_name_lost,
				   NULL,
				   NULL);

	gtk_main ();

	g_bus_unown_name (owner_id);
	g_dbus_node_info_unref (introspection_data);

	return 0;
}

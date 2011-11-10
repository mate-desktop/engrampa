/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  File-Roller
 *
 *  Copyright (C) 2001-2009 The Free Software Foundation, Inc.
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
#include <string.h>
#include <glib/gi18n.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include "dlg-package-installer.h"
#include "gtk-utils.h"
#include "main.h"


typedef struct {
	FrWindow   *window;
	FrArchive  *archive;
	FrAction    action;
	const char *packages;
} InstallerData;


static void
installer_data_free (InstallerData *idata)
{
	g_object_unref (idata->archive);
	g_object_unref (idata->window);
	g_free (idata);
}


static void
package_installer_terminated (InstallerData *idata,
			      const char    *error)
{
	GdkWindow *window;

	window = gtk_widget_get_window (GTK_WIDGET (idata->window));
	if (window != NULL)
		gdk_window_set_cursor (window, NULL);

	if (error != NULL) {
		fr_archive_action_completed (idata->archive,
					     FR_ACTION_CREATING_NEW_ARCHIVE,
					     FR_PROC_ERROR_GENERIC,
					     error);
	}
	else {
		update_registered_commands_capabilities ();
		if (fr_window_is_batch_mode (idata->window))
			fr_window_resume_batch (idata->window);
		else
			fr_window_restart_current_batch_action (idata->window);
	}

	installer_data_free (idata);
}


#ifdef ENABLE_PACKAGEKIT


static void
packagekit_install_package_names_ready_cb (GObject      *source_object,
					   GAsyncResult *res,
					   gpointer      user_data)
{
	InstallerData *idata = user_data;
	GDBusProxy    *proxy;
	GVariant      *values;
	GError        *error = NULL;
	char          *message = NULL;

	proxy = G_DBUS_PROXY (source_object);
	values = g_dbus_proxy_call_finish (proxy, res, &error);
	if (values == NULL) {
		message = g_strdup_printf ("%s\n%s",
					   _("There was an internal error trying to search for applications:"),
					   error->message);
		g_clear_error (&error);
	}

	package_installer_terminated (idata, message);

	g_free (message);
	if (values != NULL)
		g_variant_unref (values);
	g_object_unref (proxy);
}


static char **
get_packages_real_names (char **names)
{
	char     **real_names;
	GKeyFile  *key_file;
	char      *filename;
	int        i;

	real_names = g_new0 (char *, g_strv_length (names));
	key_file = g_key_file_new ();
	filename = g_build_filename (PRIVDATADIR, "packages.match", NULL);
	g_key_file_load_from_file (key_file, filename, G_KEY_FILE_NONE, NULL);

	for (i = 0; names[i] != NULL; i++) {
		char *real_name;

		real_name = g_key_file_get_string (key_file, "Package Matches", names[i], NULL);
		if (real_name != NULL)
			real_name = g_strstrip (real_name);
		if ((real_name == NULL) || (strncmp (real_name, "", 1) == 0))
			real_names[i] = g_strdup (real_name);

		g_free (real_name);
	}

	g_free (filename);
	g_key_file_free (key_file);

	return real_names;
}


static void
install_packages (InstallerData *idata)
{
	GDBusConnection *connection;
	GError          *error = NULL;

	connection = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, &error);
	if (connection != NULL) {
		GdkWindow  *window;
		GDBusProxy *proxy;

		window = gtk_widget_get_window (GTK_WIDGET (idata->window));
		if (window != NULL) {
			GdkCursor *cursor;

			cursor = gdk_cursor_new (GDK_WATCH);
			gdk_window_set_cursor (window, cursor);
			gdk_cursor_unref (cursor);
		}

		proxy = g_dbus_proxy_new_sync (connection,
					       G_DBUS_PROXY_FLAGS_NONE,
					       NULL,
					       "org.freedesktop.PackageKit",
					       "/org/freedesktop/PackageKit",
					       "org.freedesktop.PackageKit.Modify",
					       NULL,
					       &error);

		if (proxy != NULL) {
			guint   xid;
			char  **names;
			char  **real_names;

			if (window != NULL)
				xid = GDK_WINDOW_XID (window);
			else
				xid = 0;

			names = g_strsplit (idata->packages, ",", -1);
			real_names = get_packages_real_names (names);

			g_dbus_proxy_call (proxy,
					   "InstallPackageNames",
					   g_variant_new ("(u^ass)",
							  xid,
							  names,
							  "hide-confirm-search,hide-finished,hide-warning"),
					   G_DBUS_CALL_FLAGS_NONE,
					   G_MAXINT,
					   NULL,
					   packagekit_install_package_names_ready_cb,
					   idata);

			g_strfreev (real_names);
			g_strfreev (names);
		}
	}

	if (error != NULL) {
		char *message;

		message = g_strdup_printf ("%s\n%s",
					   _("There was an internal error trying to search for applications:"),
					   error->message);
		package_installer_terminated (idata, message);

		g_clear_error (&error);
	}
}


static void
confirm_search_dialog_response_cb (GtkDialog *dialog,
				   int        response_id,
				   gpointer   user_data)
{
	InstallerData *idata = user_data;

	gtk_widget_destroy (GTK_WIDGET (dialog));

	if (response_id == GTK_RESPONSE_YES) {
		install_packages (idata);
	}
	else {
		fr_window_stop_batch (idata->window);
		installer_data_free (idata);
	}
}


#endif /* ENABLE_PACKAGEKIT */


void
dlg_package_installer (FrWindow  *window,
		       FrArchive *archive,
		       FrAction   action)
{
	InstallerData   *idata;
	GType            command_type;
	FrCommand       *command;

	idata = g_new0 (InstallerData, 1);
	idata->window = g_object_ref (window);
	idata->archive = g_object_ref (archive);
	idata->action = action;

	command_type = get_preferred_command_for_mime_type (idata->archive->content_type, FR_COMMAND_CAN_READ_WRITE);
	if (command_type == 0)
		command_type = get_preferred_command_for_mime_type (idata->archive->content_type, FR_COMMAND_CAN_READ);
	if (command_type == 0) {
		package_installer_terminated (idata, _("Archive type not supported."));
		return;
	}

	command = g_object_new (command_type, 0);
	idata->packages = fr_command_get_packages (command, idata->archive->content_type);
	g_object_unref (command);

	if (idata->packages == NULL) {
		package_installer_terminated (idata, _("Archive type not supported."));
		return;
	}

#ifdef ENABLE_PACKAGEKIT

	{
		char      *secondary_text;
		GtkWidget *dialog;

		secondary_text = g_strdup_printf (_("There is no command installed for %s files.\nDo you want to search for a command to open this file?"),
						  g_content_type_get_description (idata->archive->content_type));
		dialog = _gtk_message_dialog_new (GTK_WINDOW (idata->window),
						  GTK_DIALOG_MODAL,
						  GTK_STOCK_DIALOG_ERROR,
						  _("Could not open this file type"),
						  secondary_text,
						  GTK_STOCK_CANCEL, GTK_RESPONSE_NO,
						  _("_Search Command"), GTK_RESPONSE_YES,
						  NULL);
		g_signal_connect (dialog, "response", G_CALLBACK (confirm_search_dialog_response_cb), idata);
		gtk_widget_show (dialog);

		g_free (secondary_text);
	}

#else /* ! ENABLE_PACKAGEKIT */

	package_installer_terminated (idata, _("Archive type not supported."));

#endif /* ENABLE_PACKAGEKIT */
}

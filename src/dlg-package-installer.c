/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  Engrampa
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <config.h>
#include <string.h>
#include <glib/gi18n.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include "dlg-package-installer.h"
#include "gtk-utils.h"
#include "fr-init.h"


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
package_installer_terminated (InstallerData   *idata,
			      FrProcErrorType  error_type,
			      const char      *error_message)
{
	GdkWindow *window;

	window = gtk_widget_get_window (GTK_WIDGET (idata->window));
	if (window != NULL)
		gdk_window_set_cursor (window, NULL);

	if (error_type != FR_PROC_ERROR_NONE) {
		fr_archive_action_completed (idata->archive,
					     idata->action,
					     error_type,
					     error_message);
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
	InstallerData   *idata = user_data;
	GDBusProxy      *proxy;
	GVariant        *values;
	GError          *error = NULL;
	FrProcErrorType  error_type = FR_PROC_ERROR_NONE;
	char            *error_message = NULL;

	proxy = G_DBUS_PROXY (source_object);
	values = g_dbus_proxy_call_finish (proxy, res, &error);
	if (values == NULL) {
		if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED)
		    || (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_DBUS_ERROR)
			&& (error->message != NULL)
			&& (strstr (error->message, "org.freedesktop.Packagekit.Modify.Cancelled") != NULL)))
		{
			error_type = FR_PROC_ERROR_STOPPED;
			error_message = NULL;
		}
		else {
			error_type = FR_PROC_ERROR_GENERIC;
			error_message = g_strdup_printf ("%s\n%s",
							 _("There was an internal error trying to search for applications:"),
							 error->message);
		}
		g_clear_error (&error);
	}

	package_installer_terminated (idata, error_type, error_message);

	g_free (error_message);
	if (values != NULL)
		g_variant_unref (values);
	g_object_unref (proxy);
}


static char **
get_packages_real_names (char **names)
{
	char     **real_names;
	GKeyFile  *key_file;
	int        i;

	real_names = g_new0 (char *, g_strv_length (names));
	key_file = g_key_file_new ();
	g_key_file_load_from_file (key_file,
	                           PRIVDATADIR "/packages.match",
	                           G_KEY_FILE_NONE, NULL);

	for (i = 0; names[i] != NULL; i++) {
		char *real_name;

		real_name = g_key_file_get_string (key_file, "Package Matches", names[i], NULL);
		if (real_name != NULL)
			real_name = g_strstrip (real_name);
		if ((real_name == NULL) || (strncmp (real_name, "", 1) == 0)) {
			g_free (real_name);
			real_name = g_strdup (names[i]);
		}
		real_names[i] = real_name;
		real_name = NULL;
	}

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
			GdkDisplay *display;

			display = gtk_widget_get_display (GTK_WIDGET (idata));
			cursor = gdk_cursor_new_for_display (display, GDK_WATCH);
			gdk_window_set_cursor (window, cursor);
			g_object_unref (cursor);
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
							  real_names,
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
		package_installer_terminated (idata, FR_PROC_ERROR_GENERIC, message);

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

static GtkWidget *
create_button (const char *icon_name,
	       const char *text)
{
	GtkIconTheme *icon_theme;
	GtkWidget    *button;

	button = gtk_button_new_with_mnemonic (text);
	icon_theme = gtk_icon_theme_get_default ();
	if (gtk_icon_theme_has_icon (icon_theme, icon_name)) {
		GtkWidget *image;
		image = gtk_image_new_from_icon_name (icon_name, GTK_ICON_SIZE_BUTTON);
		gtk_button_set_image (GTK_BUTTON (button), image);
	}
	gtk_widget_set_can_default (button, TRUE);

	gtk_widget_show (button);

	return button;
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
		package_installer_terminated (idata, FR_PROC_ERROR_GENERIC, _("Archive type not supported."));
		return;
	}

	command = g_object_new (command_type, 0);
	idata->packages = fr_command_get_packages (command, idata->archive->content_type);
	g_object_unref (command);

	if (idata->packages == NULL) {
		package_installer_terminated (idata, FR_PROC_ERROR_GENERIC, _("Archive type not supported."));
		return;
	}

#ifdef ENABLE_PACKAGEKIT

	{
		char      *secondary_text;
		GtkWidget *dialog;
		char      *description;

		description = g_content_type_get_description (idata->archive->content_type);
		secondary_text = g_strdup_printf (_("There is no command installed for %s files.\nDo you want to search for a command to open this file?"),
		                                  description);
		dialog = _gtk_message_dialog_new (GTK_WINDOW (idata->window),
						  GTK_DIALOG_MODAL,
						  "dialog-error",
						  _("Could not open this file type"),
						  secondary_text,
						  NULL);

		gtk_dialog_add_action_widget (GTK_DIALOG (dialog),
		                              create_button ("process-stop", _("_Cancel")),
		                              GTK_RESPONSE_NO);

		gtk_dialog_add_action_widget (GTK_DIALOG (dialog),
		                              create_button ("edit-find", _("_Search Command")),
		                              GTK_RESPONSE_YES);

		gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_YES);

		g_signal_connect (dialog, "response", G_CALLBACK (confirm_search_dialog_response_cb), idata);
		gtk_widget_show (dialog);

		g_free (description);
		g_free (secondary_text);
	}

#else /* ! ENABLE_PACKAGEKIT */

	package_installer_terminated (idata, FR_PROC_ERROR_GENERIC, _("Archive type not supported."));

#endif /* ENABLE_PACKAGEKIT */
}

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
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02110-1301, USA.
 */

#include <config.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <stdlib.h>
#include <glib/gi18n.h>
#include <glib.h>
#include <gio/gio.h>

#include "eggsmclient.h"

#include "fr-init.h"

gint          ForceDirectoryCreation;

static char **remaining_args;
static char  *add_to = NULL;
static int    add;
static char  *extract_to = NULL;
static int    extract;
static int    extract_here;
static char  *default_url = NULL;

/* argv[0] from main(); used as the command to restart the program */
static const char *program_argv0 = NULL;

static const GOptionEntry options[] = {
	{ "add-to", 'a', 0, G_OPTION_ARG_STRING, &add_to,
	  N_("Add files to the specified archive and quit the program"),
	  N_("ARCHIVE") },

	{ "add", 'd', 0, G_OPTION_ARG_NONE, &add,
	  N_("Add files asking the name of the archive and quit the program"),
	  NULL },

	{ "extract-to", 'e', 0, G_OPTION_ARG_STRING, &extract_to,
	  N_("Extract archives to the specified folder and quit the program"),
	  N_("FOLDER") },

	{ "extract", 'f', 0, G_OPTION_ARG_NONE, &extract,
	  N_("Extract archives asking the destination folder and quit the program"),
	  NULL },

	{ "extract-here", 'h', 0, G_OPTION_ARG_NONE, &extract_here,
	  N_("Extract the contents of the archives in the archive folder and quit the program"),
	  NULL },

	{ "default-dir", '\0', 0, G_OPTION_ARG_STRING, &default_url,
	  N_("Default folder to use for the '--add' and '--extract' commands"),
	  N_("FOLDER") },

	{ "force", '\0', 0, G_OPTION_ARG_NONE, &ForceDirectoryCreation,
	  N_("Create destination folder without asking confirmation"),
	  NULL },

	{ G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_STRING_ARRAY, &remaining_args,
	  NULL,
	  NULL },

	{ NULL }
};

static void
fr_restore_session (EggSMClient *client)
{
	GKeyFile *state = NULL;
	guint i;

	state = egg_sm_client_get_state_file (client);

	i = g_key_file_get_integer (state, "Session", "archives", NULL);

	for (; i > 0; i--) {
		GtkWidget *window;
		gchar *key, *archive;

		key = g_strdup_printf ("archive%d", i);
		archive = g_key_file_get_string (state, "Session", key, NULL);
		g_free (key);

		window = fr_window_new ();
		gtk_widget_show (window);
		if (strlen (archive))
			fr_window_archive_open (FR_WINDOW (window), archive, GTK_WINDOW (window));

		g_free (archive);
	}
}

static char *
get_uri_from_command_line (const char *path)
{
	GFile *file;
	char  *uri;

	file = g_file_new_for_commandline_arg (path);
	uri = g_file_get_uri (file);
	g_object_unref (file);

	return uri;
}

static void
prepare_app (void)
{
	char        *extract_to_uri = NULL;
	char        *add_to_uri = NULL;
	EggSMClient *client = NULL;

	client = egg_sm_client_get ();
	if (egg_sm_client_is_resumed (client)) {
		fr_restore_session (client);
		return;
	}

	/**/

	if (remaining_args == NULL) { /* No archive specified. */
		fr_window_new ();
		return;
	}

	if (extract_to != NULL)
		extract_to_uri = get_uri_from_command_line (extract_to);

	if (add_to != NULL)
		add_to_uri = get_uri_from_command_line (add_to);

	if ((add_to != NULL) || (add == 1)) { /* Add files to an archive */
		GtkWidget   *window;
		GList       *file_list = NULL;
		const char  *filename;
		int          i = 0;

		window = fr_window_new ();
		if (default_url != NULL)
			fr_window_set_default_dir (FR_WINDOW (window), default_url, TRUE);

		while ((filename = remaining_args[i++]) != NULL)
			file_list = g_list_prepend (file_list, get_uri_from_command_line (filename));
		file_list = g_list_reverse (file_list);

		fr_window_new_batch (FR_WINDOW (window), _("Compress"));
		fr_window_set_batch__add (FR_WINDOW (window), add_to_uri, file_list);
		fr_window_append_batch_action (FR_WINDOW (window),
					       FR_BATCH_ACTION_QUIT,
					       NULL,
					       NULL);
		fr_window_start_batch (FR_WINDOW (window));
	}
	else if ((extract_to != NULL) || (extract == 1) || (extract_here == 1)) {

		/* Extract all archives. */

		GtkWidget  *window;
		const char *archive;
		int         i = 0;

		window = fr_window_new ();
		if (default_url != NULL)
			fr_window_set_default_dir (FR_WINDOW (window), default_url, TRUE);

		fr_window_new_batch (FR_WINDOW (window), _("Extract archive"));
		while ((archive = remaining_args[i++]) != NULL) {
			char *archive_uri;

			archive_uri = get_uri_from_command_line (archive);
			if (extract_here == 1)
				fr_window_set_batch__extract_here (FR_WINDOW (window),
								   archive_uri);
			else
				fr_window_set_batch__extract (FR_WINDOW (window),
							      archive_uri,
							      extract_to_uri);
			g_free (archive_uri);
		}
		fr_window_append_batch_action (FR_WINDOW (window),
					       FR_BATCH_ACTION_QUIT,
					       NULL,
					       NULL);

		fr_window_start_batch (FR_WINDOW (window));
	}
	else { /* Open each archive in a window */
		const char *filename = NULL;

		int i = 0;
		while ((filename = remaining_args[i++]) != NULL) {
			GtkWidget *window;
			GFile     *file;
			char      *uri;

			window = fr_window_new ();

			file = g_file_new_for_commandline_arg (filename);
			uri = g_file_get_uri (file);
			fr_window_archive_open (FR_WINDOW (window), uri, GTK_WINDOW (window));
			g_free (uri);
			g_object_unref (file);
		}
	}

	g_free (add_to_uri);
	g_free (extract_to_uri);
}


static void
startup_cb (GApplication *application)
{
	initialize_data ();
	prepare_app ();
}


static void
activate_cb (GApplication *application)
{
	GList *link;

	for (link = gtk_application_get_windows (GTK_APPLICATION (application));
	     link != NULL;
	     link = link->next)
	{
		if (! fr_window_is_batch_mode (FR_WINDOW (link->data)))
			gtk_widget_show (GTK_WIDGET (link->data));
	}
}


static void
fr_save_state (EggSMClient *client, GKeyFile *state, gpointer user_data)
{
	/* discard command is automatically set by EggSMClient */

	const char   *argv[2] = { NULL };
	GApplication *application;
	guint         i = 0;

	/* restart command */
	argv[0] = program_argv0;
	argv[1] = NULL;

	egg_sm_client_set_restart_command (client, 1, argv);

	/* state */
	application = g_application_get_default ();
	if (application != NULL) {
		GList *window;

		for (window = gtk_application_get_windows (GTK_APPLICATION (application)), i = 0;
		     window != NULL;
		     window = window->next, i++)
		{
			FrWindow *session = window->data;
			gchar *key;

			key = g_strdup_printf ("archive%d", i);
			if ((session->archive == NULL) || (session->archive->file == NULL)) {
				g_key_file_set_string (state, "Session", key, "");
			}
			else {
				gchar *uri;

				uri = g_file_get_uri (session->archive->file);
				g_key_file_set_string (state, "Session", key, uri);
				g_free (uri);
			}
			g_free (key);
		}
	}

	g_key_file_set_integer (state, "Session", "archives", i);
}


int
main (int argc, char **argv)
{
	GOptionContext *context = NULL;
	GError         *error = NULL;
	GtkApplication *app = NULL;
	EggSMClient    *client = NULL;
	int             status;

	program_argv0 = argv[0];

	bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);

	context = g_option_context_new (N_("- Create and modify an archive"));
	g_option_context_set_translation_domain (context, GETTEXT_PACKAGE);
	g_option_context_add_main_entries (context, options, GETTEXT_PACKAGE);

	g_option_context_add_group (context, gtk_get_option_group (TRUE));
	g_option_context_add_group (context, egg_sm_client_get_option_group ());

	if (! g_option_context_parse (context, &argc, &argv, &error)) {
		g_critical ("Failed to parse arguments: %s", error->message);
		g_error_free (error);
		g_option_context_free (context);
		return EXIT_FAILURE;
	}

	g_option_context_free (context);

	g_set_application_name (_("Engrampa"));
	gtk_window_set_default_icon_name ("engrampa");

	client = egg_sm_client_get ();
	g_signal_connect (client, "save-state", G_CALLBACK (fr_save_state), NULL);

	gtk_icon_theme_append_search_path (gtk_icon_theme_get_default (),
					   PKG_DATA_DIR G_DIR_SEPARATOR_S "icons");

	app = gtk_application_new (NULL, G_APPLICATION_FLAGS_NONE);
	g_signal_connect (app, "startup", G_CALLBACK (startup_cb), NULL);
	g_signal_connect (app, "activate", G_CALLBACK (activate_cb), NULL);

	status = g_application_run (G_APPLICATION (app), argc, argv);

	release_data ();

	return status;
}

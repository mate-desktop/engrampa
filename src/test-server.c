/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  Engrampa
 *
 *  Copyright (C) 2010 The Free Software Foundation, Inc.
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
#include <gio/gio.h>


GMainLoop *loop;


static void
engrampa_getsupportedtypes_ready_cb (GObject      *source_object,
				    GAsyncResult *res,
				    gpointer      user_data)
{
	GDBusProxy *proxy;
	GVariant   *values;
	GError     *error = NULL;

	proxy = G_DBUS_PROXY (source_object);
	values = g_dbus_proxy_call_finish (proxy, res, &error);
	if (values == NULL) {
		g_error ("%s\n", error->message);
		g_clear_error (&error);
	}
	else {
		GVariantIter  argument_iter;
		GVariant     *array_of_types;
		GVariantIter  type_iter;
		GVariant     *supported_type;
		int           n = 0;

		g_variant_iter_init (&argument_iter, values);
		array_of_types = g_variant_iter_next_value (&argument_iter);

		g_variant_iter_init (&type_iter, array_of_types);
		while ((supported_type = g_variant_iter_next_value (&type_iter))) {
			char         *mime_type = NULL;
			char         *default_ext = NULL;
			char         *description = NULL;
			char         *key;
			char         *value;
			GVariantIter  value_iter;

			g_variant_iter_init (&value_iter, supported_type);
			while (g_variant_iter_next (&value_iter, "{ss}", &key, &value)) {
				if (g_strcmp0 (key, "mime-type") == 0)
					mime_type = g_strdup (value);
				else if (g_strcmp0 (key, "default-extension") == 0)
					default_ext = g_strdup (value);
				else if (g_strcmp0 (key, "description") == 0)
					description = g_strdup (value);

				g_free (key);
				g_free (value);
			}

			n++;
			g_print ("%d)\tmime-type: %s\n\tdefault-extension: %s\n\tdescription: %s\n", n, mime_type, default_ext, description);

			g_free (description);
			g_free (default_ext);
			g_free (mime_type);
			g_variant_unref (supported_type);
		}

		g_variant_unref (array_of_types);
	}

	g_object_unref (proxy);
	g_main_loop_quit (loop);
}


static void
engrampa_addtoarchive_ready_cb (GObject      *source_object,
				  GAsyncResult *res,
				  gpointer      user_data)
{
	GDBusProxy *proxy;
	GVariant   *values;
	GError     *error = NULL;

	proxy = G_DBUS_PROXY (source_object);
	values = g_dbus_proxy_call_finish (proxy, res, &error);
	if (values == NULL) {
		g_error ("%s\n", error->message);
		g_clear_error (&error);
	}

	if (values != NULL)
		g_variant_unref (values);
	g_object_unref (proxy);

	g_main_loop_quit (loop);
}


static void
on_signal (GDBusProxy *proxy,
	   char       *sender_name,
	   char       *signal_name,
	   GVariant   *parameters,
	   gpointer    user_data)
{
	if (g_strcmp0 (signal_name, "Progress") == 0) {
		double  fraction;
		char   *details;

		g_variant_get (parameters, "(ds)", &fraction, &details);
		g_print ("Progress: %f (%s)\n", fraction, details);

		g_free (details);
	}
}


int
main (int argc, char *argv[])
{
	GDBusConnection *connection;
	GError          *error = NULL;

	connection = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, &error);
	if (connection != NULL) {
		GDBusProxy *proxy;

		proxy = g_dbus_proxy_new_sync (connection,
					       G_DBUS_PROXY_FLAGS_NONE,
					       NULL,
					       "org.mate.Engrampa",
					       "/org/mate/Engrampa",
					       "org.mate.ArchiveManager",
					       NULL,
					       &error);

		if (proxy != NULL) {

			g_signal_connect (proxy,
			                  "g-signal",
			                  G_CALLBACK (on_signal),
			                  NULL);

#if 0
			/* -- GetSupportedTypes -- */

			g_dbus_proxy_call (proxy,
					   "GetSupportedTypes",
					   g_variant_new ("(s)", "create"),
					   G_DBUS_CALL_FLAGS_NONE,
					   G_MAXINT,
					   NULL,
					   engrampa_getsupportedtypes_ready_cb,
					   NULL);
#endif

			/* -- AddToArchive -- */

			char  *archive;
			char **files;

			archive = g_strdup ("file:///home/paolo/Scrivania/firefox-4.0b8pre.tar.gz");
			files = g_new0 (char *, 2);
			files[0] = g_strdup ("file:///home/paolo/Scrivania/firefox-4.0b8pre");
			files[1] = NULL;

			g_dbus_proxy_call (proxy,
					   "AddToArchive",
					   g_variant_new ("(s^asb)",
							  archive,
							  files,
							  FALSE),
					   G_DBUS_CALL_FLAGS_NONE,
					   G_MAXINT,
					   NULL,
					   engrampa_addtoarchive_ready_cb,
					   NULL);

			g_free (archive);
			g_strfreev (files);

#if 0

			/* -- Compress -- */

			char **files;
			char  *destination;

			files = g_new0 (char *, 2);
			files[0] = g_strdup ("file:///home/paolo/Scrivania/firefox-4.0b8pre");
			files[1] = NULL;
			destination = g_strdup ("file:///home/paolo/Scrivania");

			g_dbus_proxy_call (proxy,
					   "Compress",
					   g_variant_new ("(^assb)",
							  files,
							  destination,
							  TRUE),
					   G_DBUS_CALL_FLAGS_NONE,
					   G_MAXINT,
					   NULL,
					   engrampa_addtoarchive_ready_cb,
					   NULL);

			g_strfreev (files);
			g_free (destination);

			/* -- Extract -- */

			g_dbus_proxy_call (proxy,
					   "Extract",
					   g_variant_new ("(ssb)",
							  "file:///home/paolo/Scrivania/test.tar.gz",
							  "file:///home/paolo/Scrivania",
							  TRUE),
					   G_DBUS_CALL_FLAGS_NONE,
					   G_MAXINT,
					   NULL,
					   engrampa_addtoarchive_ready_cb,
					   NULL);

			/* -- ExtractHere -- */

			g_dbus_proxy_call (proxy,
					   "ExtractHere",
					   g_variant_new ("(sb)",
					                  "file:///home/paolo/Scrivania/test.tar.gz",
					                  TRUE),
					   G_DBUS_CALL_FLAGS_NONE,
					   G_MAXINT,
					   NULL,
					   engrampa_addtoarchive_ready_cb,
					   NULL);

#endif

		}
	}

	loop = g_main_loop_new (NULL, FALSE);
	g_main_loop_run (loop);

	return 0;
}

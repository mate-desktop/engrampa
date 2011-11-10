/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  File-Roller
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
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307, USA.
 */

#include <config.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <stdlib.h>
#include <gio/gio.h>
#include "file-utils.h"
#include "glib-utils.h"
#include "fr-command.h"
#include "fr-command-ace.h"
#include "fr-command-alz.h"
#include "fr-command-ar.h"
#include "fr-command-arj.h"
#include "fr-command-cfile.h"
#include "fr-command-cpio.h"
#include "fr-command-dpkg.h"
#include "fr-command-iso.h"
#include "fr-command-jar.h"
#include "fr-command-lha.h"
#include "fr-command-rar.h"
#include "fr-command-rpm.h"
#include "fr-command-tar.h"
#include "fr-command-unstuff.h"
#include "fr-command-zip.h"
#include "fr-command-zoo.h"
#include "fr-command-7z.h"
#include "fr-command-lrzip.h"
#include "fr-process.h"
#include "fr-stock.h"
#include "mateconf-utils.h"
#include "fr-window.h"
#include "typedefs.h"
#include "preferences.h"
#include "file-data.h"
#include "main.h"

#include "eggsmclient.h"

static void     prepare_app         (void);
static void     initialize_data     (void);
static void     release_data        (void);

GList        *WindowList = NULL;
GList        *CommandList = NULL;
gint          ForceDirectoryCreation;
GHashTable   *ProgramsCache = NULL;
GPtrArray    *Registered_Commands = NULL;

static char **remaining_args;

static char  *add_to = NULL;
static int    add;
static char  *extract_to = NULL;
static int    extract;
static int    extract_here;
static char  *default_url = NULL;

/* The capabilities are computed automatically in
 * compute_supported_archive_types() so it's correct to initialize to 0 here. */
FrMimeTypeDescription mime_type_desc[] = {
	{ "application/x-7z-compressed",        ".7z",       N_("7-Zip (.7z)"), 0 },
	{ "application/x-7z-compressed-tar",    ".tar.7z",   N_("Tar compressed with 7z (.tar.7z)"), 0 },
	{ "application/x-ace",                  ".ace",      N_("Ace (.ace)"), 0 },
	{ "application/x-alz",                  ".alz",      NULL, 0 },
	{ "application/x-ar",                   ".ar",       N_("Ar (.ar)"), 0 },
	{ "application/x-arj",                  ".arj",      N_("Arj (.arj)"), 0 },
	{ "application/x-bzip",                 ".bz2",      NULL, 0 },
	{ "application/x-bzip-compressed-tar",  ".tar.bz2",  N_("Tar compressed with bzip2 (.tar.bz2)"), 0 },
	{ "application/x-bzip1",                ".bz",       NULL, 0 },
	{ "application/x-bzip1-compressed-tar", ".tar.bz",   N_("Tar compressed with bzip (.tar.bz)"), 0 },
	{ "application/vnd.ms-cab-compressed",  ".cab",      N_("Cabinet (.cab)"), 0 },
	{ "application/x-cbr",                  ".cbr",      N_("Rar Archived Comic Book (.cbr)"), 0 },
	{ "application/x-cbz",                  ".cbz",      N_("Zip Archived Comic Book (.cbz)"), 0 },
	{ "application/x-cd-image",             ".iso",      NULL, 0 },
	{ "application/x-compress",             ".Z",        NULL, 0 },
	{ "application/x-compressed-tar",       ".tar.gz",   N_("Tar compressed with gzip (.tar.gz)"), 0 },
	{ "application/x-cpio",                 ".cpio",     NULL, 0 },
	{ "application/x-deb",                  ".deb",      NULL, 0 },
	{ "application/x-ear",                  ".ear",      N_("Ear (.ear)"), 0 },
	{ "application/x-ms-dos-executable",    ".exe",      N_("Self-extracting zip (.exe)"), 0 },
	{ "application/x-gzip",                 ".gz",       NULL, 0 },
	{ "application/x-java-archive",         ".jar",      N_("Jar (.jar)"), 0 },
	{ "application/x-lha",                  ".lzh",      N_("Lha (.lzh)"), 0 },
	{ "application/x-lrzip",                ".lrz",      N_("Lrzip (.lrz)"), 0},
	{ "application/x-lrzip-compressed-tar", ".tar.lrz",  N_("Tar compressed with lrzip (.tar.lrz)"), 0 },
	{ "application/x-lzip",                 ".lz",       NULL, 0 },
	{ "application/x-lzip-compressed-tar",  ".tar.lz",   N_("Tar compressed with lzip (.tar.lz)"), 0 },
	{ "application/x-lzma",                 ".lzma",     NULL, 0 },
	{ "application/x-lzma-compressed-tar",  ".tar.lzma", N_("Tar compressed with lzma (.tar.lzma)"), 0 },
	{ "application/x-lzop",                 ".lzo",      NULL, 0 },
	{ "application/x-lzop-compressed-tar",  ".tar.lzo",  N_("Tar compressed with lzop (.tar.lzo)"), 0 },
	{ "application/x-rar",                  ".rar",      N_("Rar (.rar)"), 0 },
	{ "application/x-rpm",                  ".rpm",      NULL, 0 },
	{ "application/x-rzip",                 ".rz",       NULL, 0 },
	{ "application/x-tar",                  ".tar",      N_("Tar uncompressed (.tar)"), 0 },
	{ "application/x-tarz",                 ".tar.Z",    N_("Tar compressed with compress (.tar.Z)"), 0 },
	{ "application/x-stuffit",              ".sit",      NULL, 0 },
	{ "application/x-war",                  ".war",      N_("War (.war)"), 0 },
	{ "application/x-xz",                   ".xz",       N_("Xz (.xz)"), 0 },
	{ "application/x-xz-compressed-tar",    ".tar.xz",   N_("Tar compressed with xz (.tar.xz)"), 0 },
	{ "application/x-zoo",                  ".zoo",      N_("Zoo (.zoo)"), 0 },
	{ "application/zip",                    ".zip",      N_("Zip (.zip)"), 0 },
	{ NULL, NULL, NULL, 0 }
};

FrExtensionType file_ext_type[] = {
	{ ".7z", "application/x-7z-compressed" },
	{ ".ace", "application/x-ace" },
	{ ".alz", "application/x-alz" },
	{ ".ar", "application/x-ar" },
	{ ".arj", "application/x-arj" },
	{ ".bin", "application/x-stuffit" },
	{ ".bz", "application/x-bzip" },
	{ ".bz2", "application/x-bzip" },
	{ ".cab", "application/vnd.ms-cab-compressed" },
	{ ".cbr", "application/x-cbr" },
	{ ".cbz", "application/x-cbz" },
	{ ".cpio", "application/x-cpio" },
	{ ".deb", "application/x-deb" },
	{ ".ear", "application/x-ear" },
	{ ".exe", "application/x-ms-dos-executable" },
	{ ".gz", "application/x-gzip" },
	{ ".iso", "application/x-cd-image" },
	{ ".jar", "application/x-java-archive" },
	{ ".lha", "application/x-lha" },
	{ ".lrz", "application/x-lrzip" },
	{ ".lzh", "application/x-lha" },
	{ ".lz", "application/x-lzip" },
	{ ".lzma", "application/x-lzma" },
	{ ".lzo", "application/x-lzop" },
	{ ".rar", "application/x-rar" },
	{ ".rpm", "application/x-rpm" },
	{ ".rz", "application/x-rzip" },
	{ ".sit", "application/x-stuffit" },
	{ ".tar", "application/x-tar" },
	{ ".tar.bz", "application/x-bzip-compressed-tar" },
	{ ".tar.bz2", "application/x-bzip-compressed-tar" },
	{ ".tar.gz", "application/x-compressed-tar" },
	{ ".tar.lrz", "application/x-lrzip-compressed-tar" },
	{ ".tar.lz", "application/x-lzip-compressed-tar" },
	{ ".tar.lzma", "application/x-lzma-compressed-tar" },
	{ ".tar.lzo", "application/x-lzop-compressed-tar" },
	{ ".tar.7z", "application/x-7z-compressed-tar" },
	{ ".tar.xz", "application/x-xz-compressed-tar" },
	{ ".tar.Z", "application/x-tarz" },
	{ ".taz", "application/x-tarz" },
	{ ".tbz", "application/x-bzip-compressed-tar" },
	{ ".tbz2", "application/x-bzip-compressed-tar" },
	{ ".tgz", "application/x-compressed-tar" },
	{ ".txz", "application/x-xz-compressed-tar" },
	{ ".tlz", "application/x-lzip-compressed-tar" },
	{ ".tzma", "application/x-lzma-compressed-tar" },
	{ ".tzo", "application/x-lzop-compressed-tar" },
	{ ".war", "application/x-war" },
	{ ".xz", "application/x-xz" },
	{ ".z", "application/x-gzip" },
	{ ".Z", "application/x-compress" },
	{ ".zip", "application/zip" },
	{ ".zoo", "application/x-zoo" },
	{ NULL, NULL }
};

int single_file_save_type[64];
int save_type[64];
int open_type[64];
int create_type[64];

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


/* -- Main -- */


static guint startup_id = 0;

/* argv[0] from main(); used as the command to restart the program */
static const char *program_argv0 = NULL;


static gboolean
startup_cb (gpointer data)
{
	g_source_remove (startup_id);
	startup_id = 0;

	initialize_data ();
	prepare_app ();

	return FALSE;
}


static void
fr_save_state (EggSMClient *client, GKeyFile *state, gpointer user_data)
{
	/* discard command is automatically set by EggSMClient */

	GList *window;
	const char *argv[2] = { NULL };
	guint i;

	/* restart command */
	argv[0] = program_argv0;
	argv[1] = NULL;

	egg_sm_client_set_restart_command (client, 1, argv);

	/* state */
	for (window = WindowList, i = 0; window; window = window->next, i++) {
		FrWindow *session = window->data;
		gchar *key;

		key = g_strdup_printf ("archive%d", i);
		if ((session->archive == NULL) || (session->archive->file == NULL)) {
			g_key_file_set_string (state, "Session", key, "");
		} else {
			gchar *uri;

			uri = g_file_get_uri (session->archive->file);
			g_key_file_set_string (state, "Session", key, uri);
			g_free (uri);
		}
		g_free (key);
	}

	g_key_file_set_integer (state, "Session", "archives", i);
}

int
main (int argc, char **argv)
{
	GError *error = NULL;
	EggSMClient *client = NULL;
	GOptionContext *context = NULL;

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

	g_set_application_name (_("File Roller"));
	gtk_window_set_default_icon_name ("file-roller");

	client = egg_sm_client_get ();
	g_signal_connect (client, "save-state", G_CALLBACK (fr_save_state), NULL);

	gtk_icon_theme_append_search_path (gtk_icon_theme_get_default (),
					   PKG_DATA_DIR G_DIR_SEPARATOR_S "icons");

	fr_stock_init ();
	/* init_session (argv); */
	startup_id = g_idle_add (startup_cb, NULL);
	gtk_main ();
	release_data ();

	return 0;
}

/* Initialize application data. */


static void
initialize_data (void)
{
	eel_mateconf_monitor_add ("/apps/file-roller");
	eel_mateconf_monitor_add (PREF_CAJA_CLICK_POLICY);

	ProgramsCache = g_hash_table_new_full (g_str_hash,
					       g_str_equal,
					       g_free,
					       NULL);
}

/* Free application data. */


void
command_done (CommandData *cdata)
{
	if (cdata == NULL)
		return;

	if ((cdata->temp_dir != NULL) && path_is_dir (cdata->temp_dir)) {
		char *argv[4];

		argv[0] = "rm";
		argv[1] = "-rf";
		argv[2] = cdata->temp_dir;
		argv[3] = NULL;
		g_spawn_sync (g_get_tmp_dir (), argv, NULL,
			      G_SPAWN_SEARCH_PATH,
			      NULL, NULL,
			      NULL, NULL, NULL,
			      NULL);
	}

	g_free (cdata->command);
	if (cdata->app != NULL)
		g_object_unref (cdata->app);
	path_list_free (cdata->file_list);
	g_free (cdata->temp_dir);
	if (cdata->process != NULL)
		g_object_unref (cdata->process);

	CommandList = g_list_remove (CommandList, cdata);
	g_free (cdata);
}


static void
release_data ()
{
	g_hash_table_destroy (ProgramsCache);

	eel_global_client_free ();

	while (CommandList != NULL) {
		CommandData *cdata = CommandList->data;
		command_done (cdata);
	}
}

/* Create the windows. */


static void
migrate_dir_from_to (const char *from_dir,
		     const char *to_dir)
{
	char *from_path;
	char *to_path;

	from_path = get_home_relative_path (from_dir);
	to_path = get_home_relative_path (to_dir);

	if (uri_is_dir (from_path) && ! uri_exists (to_path)) {
		char *line;
		char *e1;
		char *e2;

		e1 = g_shell_quote (from_path);
		e2 = g_shell_quote (to_path);
		line = g_strdup_printf ("mv -f %s %s", e1, e2);
		g_free (e1);
		g_free (e2);

		g_spawn_command_line_sync (line, NULL, NULL, NULL, NULL);
		g_free (line);
	}

	g_free (from_path);
	g_free (to_path);
}


static void
migrate_file_from_to (const char *from_file,
		      const char *to_file)
{
	char *from_path;
	char *to_path;

	from_path = get_home_relative_path (from_file);
	to_path = get_home_relative_path (to_file);

	if (uri_is_file (from_path) && ! uri_exists (to_path)) {
		char *line;
		char *e1;
		char *e2;

		e1 = g_shell_quote (from_path);
		e2 = g_shell_quote (to_path);
		line = g_strdup_printf ("mv -f %s %s", e1, e2);
		g_free (e1);
		g_free (e2);

		g_spawn_command_line_sync (line, NULL, NULL, NULL, NULL);
		g_free (line);
	}

	g_free (from_path);
	g_free (to_path);
}


static void
migrate_to_new_directories (void)
{
	migrate_dir_from_to  (OLD_RC_OPTIONS_DIR, RC_OPTIONS_DIR);
	migrate_file_from_to (OLD_RC_BOOKMARKS_FILE, RC_BOOKMARKS_FILE);
	migrate_file_from_to (OLD_RC_RECENT_FILE, RC_RECENT_FILE);

	eel_mateconf_set_boolean (PREF_MIGRATE_DIRECTORIES, FALSE);
}


/* -- FrRegisteredCommand -- */


FrRegisteredCommand *
fr_registered_command_new (GType command_type)
{
	FrRegisteredCommand  *reg_com;
	FrCommand            *command;
	const char          **mime_types;
	int                   i;

	reg_com = g_new0 (FrRegisteredCommand, 1);
	reg_com->ref = 1;
	reg_com->type = command_type;
	reg_com->caps = g_ptr_array_new ();
	reg_com->packages = g_ptr_array_new ();

	command = (FrCommand*) g_object_new (reg_com->type, NULL);
	mime_types = fr_command_get_mime_types (command);
	for (i = 0; mime_types[i] != NULL; i++) {
		const char         *mime_type;
		FrMimeTypeCap      *cap;
		FrMimeTypePackages *packages;

		mime_type = get_static_string (mime_types[i]);

		cap = g_new0 (FrMimeTypeCap, 1);
		cap->mime_type = mime_type;
		cap->current_capabilities = fr_command_get_capabilities (command, mime_type, TRUE);
		cap->potential_capabilities = fr_command_get_capabilities (command, mime_type, FALSE);
		g_ptr_array_add (reg_com->caps, cap);

		packages = g_new0 (FrMimeTypePackages, 1);
		packages->mime_type = mime_type;
		packages->packages = fr_command_get_packages (command, mime_type);
		g_ptr_array_add (reg_com->packages, packages);
	}

	g_object_unref (command);

	return reg_com;
}


void
fr_registered_command_ref (FrRegisteredCommand *reg_com)
{
	reg_com->ref++;
}


void
fr_registered_command_unref (FrRegisteredCommand *reg_com)
{
	if (--(reg_com->ref) != 0)
		return;

	g_ptr_array_foreach (reg_com->caps, (GFunc) g_free, NULL);
	g_ptr_array_free (reg_com->caps, TRUE);
	g_free (reg_com);
}


FrCommandCaps
fr_registered_command_get_capabilities (FrRegisteredCommand *reg_com,
				        const char          *mime_type)
{
	int i;

	for (i = 0; i < reg_com->caps->len; i++) {
		FrMimeTypeCap *cap;

		cap = g_ptr_array_index (reg_com->caps, i);
		if (strcmp (mime_type, cap->mime_type) == 0)
			return cap->current_capabilities;
	}

	return FR_COMMAND_CAN_DO_NOTHING;
}


FrCommandCaps
fr_registered_command_get_potential_capabilities (FrRegisteredCommand *reg_com,
						  const char          *mime_type)
{
	int i;

	if (mime_type == NULL)
		return FR_COMMAND_CAN_DO_NOTHING;

	for (i = 0; i < reg_com->caps->len; i++) {
		FrMimeTypeCap *cap;

		cap = g_ptr_array_index (reg_com->caps, i);
		if ((cap->mime_type != NULL) && (strcmp (mime_type, cap->mime_type) == 0))
			return cap->potential_capabilities;
	}

	return FR_COMMAND_CAN_DO_NOTHING;
}


void
register_command (GType command_type)
{
	if (Registered_Commands == NULL)
		Registered_Commands = g_ptr_array_sized_new (5);
	g_ptr_array_add (Registered_Commands, fr_registered_command_new (command_type));
}


gboolean
unregister_command (GType command_type)
{
	int i;

	for (i = 0; i < Registered_Commands->len; i++) {
		FrRegisteredCommand *command;

		command = g_ptr_array_index (Registered_Commands, i);
		if (command->type == command_type) {
			g_ptr_array_remove_index (Registered_Commands, i);
			fr_registered_command_unref (command);
			return TRUE;
		}
	}

	return FALSE;
}


static void
register_commands (void)
{
	/* The order here is important. Commands registered earlier have higher
	 * priority.  However commands that can read and write a file format
	 * have higher priority over commands that can only read the same
	 * format, regardless of the registration order. */

	register_command (FR_TYPE_COMMAND_TAR);
	register_command (FR_TYPE_COMMAND_CFILE);
	register_command (FR_TYPE_COMMAND_7Z);
	register_command (FR_TYPE_COMMAND_DPKG);

	register_command (FR_TYPE_COMMAND_ACE);
	register_command (FR_TYPE_COMMAND_ALZ);
	register_command (FR_TYPE_COMMAND_AR);
	register_command (FR_TYPE_COMMAND_ARJ);
	register_command (FR_TYPE_COMMAND_CPIO);
	register_command (FR_TYPE_COMMAND_ISO);
	register_command (FR_TYPE_COMMAND_JAR);
	register_command (FR_TYPE_COMMAND_LHA);
	register_command (FR_TYPE_COMMAND_RAR);
	register_command (FR_TYPE_COMMAND_RPM);
	register_command (FR_TYPE_COMMAND_UNSTUFF);
	register_command (FR_TYPE_COMMAND_ZIP);
	register_command (FR_TYPE_COMMAND_LRZIP);
	register_command (FR_TYPE_COMMAND_ZOO);
}


GType
get_command_type_from_mime_type (const char    *mime_type,
				 FrCommandCaps  requested_capabilities)
{
	int i;

	if (mime_type == NULL)
		return 0;

	for (i = 0; i < Registered_Commands->len; i++) {
		FrRegisteredCommand *command;
		FrCommandCaps        capabilities;

		command = g_ptr_array_index (Registered_Commands, i);
		capabilities = fr_registered_command_get_capabilities (command, mime_type);

		/* the command must support all the requested capabilities */
		if (((capabilities ^ requested_capabilities) & requested_capabilities) == 0)
			return command->type;
	}

	return 0;
}


GType
get_preferred_command_for_mime_type (const char    *mime_type,
				     FrCommandCaps  requested_capabilities)
{
	int i;

	for (i = 0; i < Registered_Commands->len; i++) {
		FrRegisteredCommand *command;
		FrCommandCaps        capabilities;

		command = g_ptr_array_index (Registered_Commands, i);
		capabilities = fr_registered_command_get_potential_capabilities (command, mime_type);

		/* the command must support all the requested capabilities */
		if (((capabilities ^ requested_capabilities) & requested_capabilities) == 0)
			return command->type;
	}

	return 0;
}


void
update_registered_commands_capabilities (void)
{
	int i;

	g_hash_table_remove_all (ProgramsCache);

	for (i = 0; i < Registered_Commands->len; i++) {
		FrRegisteredCommand *reg_com;
		FrCommand           *command;
		int                  j;

		reg_com = g_ptr_array_index (Registered_Commands, i);
		command = (FrCommand*) g_object_new (reg_com->type, NULL);
		for (j = 0; j < reg_com->caps->len; j++) {
			FrMimeTypeCap *cap = g_ptr_array_index (reg_com->caps, j);

			cap->current_capabilities = fr_command_get_capabilities (command, cap->mime_type, TRUE);
			cap->potential_capabilities = fr_command_get_capabilities (command, cap->mime_type, FALSE);
		}

		g_object_unref (command);
	}
}


const char *
get_mime_type_from_extension (const char *ext)
{
	int i;

	if (ext == NULL)
		return NULL;

	for (i = G_N_ELEMENTS (file_ext_type) - 1; i >= 0; i--) {
		if (file_ext_type[i].ext == NULL)
			continue;
		if (strcasecmp (ext, file_ext_type[i].ext) == 0)
			return get_static_string (file_ext_type[i].mime_type);
	}

	return NULL;
}


const char *
get_archive_filename_extension (const char *filename)
{
	const char *ext;
	int         i;

	if (filename == NULL)
		return NULL;

	ext = get_file_extension (filename);
	if (ext == NULL)
		return NULL;

	for (i = G_N_ELEMENTS (file_ext_type) - 1; i >= 0; i--) {
		if (file_ext_type[i].ext == NULL)
			continue;
		if (strcasecmp (ext, file_ext_type[i].ext) == 0)
			return ext;
	}

	return NULL;
}


int
get_mime_type_index (const char *mime_type)
{
	int i;

	for (i = 0; mime_type_desc[i].mime_type != NULL; i++)
		if (strcmp (mime_type_desc[i].mime_type, mime_type) == 0)
			return i;
	return -1;
}


static void
add_if_non_present (int *a,
	            int *n,
	            int  o)
{
	int i;

	for (i = 0; i < *n; i++) {
		if (a[i] == o)
			return;
	}
	a[*n] = o;
	*n = *n + 1;
}


static int
cmp_mime_type_by_extension (const void *p1,
			    const void *p2)
{
	int i1 = * (int*) p1;
	int i2 = * (int*) p2;

	return strcmp (mime_type_desc[i1].default_ext, mime_type_desc[i2].default_ext);
}


static int
cmp_mime_type_by_description (const void *p1,
			      const void *p2)
{
	int i1 = * (int*) p1;
	int i2 = * (int*) p2;

	return g_utf8_collate (_(mime_type_desc[i1].name), _(mime_type_desc[i2].name));
}


static void
sort_mime_types (int *a,
                 int(*compar)(const void *, const void *))
{
	int n = 0;

	while (a[n] != -1)
		n++;
	qsort (a, n, sizeof (int), compar);
}


void
sort_mime_types_by_extension (int *a)
{
	sort_mime_types (a, cmp_mime_type_by_extension);
}


void
sort_mime_types_by_description (int *a)
{
	sort_mime_types (a, cmp_mime_type_by_description);
}


static void
compute_supported_archive_types (void)
{
	int sf_i = 0, s_i = 0, o_i = 0, c_i = 0;
	int i;

	for (i = 0; i < Registered_Commands->len; i++) {
		FrRegisteredCommand *reg_com;
		int                  j;

		reg_com = g_ptr_array_index (Registered_Commands, i);
		for (j = 0; j < reg_com->caps->len; j++) {
			FrMimeTypeCap *cap;
			int            idx;

			cap = g_ptr_array_index (reg_com->caps, j);
			idx = get_mime_type_index (cap->mime_type);
			if (idx < 0) {
				g_warning ("mime type not recognized: %s", cap->mime_type);
				continue;
			}
			mime_type_desc[idx].capabilities |= cap->current_capabilities;
			if (cap->current_capabilities & FR_COMMAND_CAN_READ)
				add_if_non_present (open_type, &o_i, idx);
			if (cap->current_capabilities & FR_COMMAND_CAN_WRITE) {
				if (cap->current_capabilities & FR_COMMAND_CAN_ARCHIVE_MANY_FILES) {
					add_if_non_present (save_type, &s_i, idx);
					if (cap->current_capabilities & FR_COMMAND_CAN_WRITE)
						add_if_non_present (create_type, &c_i, idx);
				}
				add_if_non_present (single_file_save_type, &sf_i, idx);
			}
		}
	}

	open_type[o_i] = -1;
	save_type[s_i] = -1;
	single_file_save_type[sf_i] = -1;
	create_type[c_i] = -1;
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

static void
prepare_app (void)
{
	char *uri;
	char *extract_to_path = NULL;
	char *add_to_path = NULL;
	EggSMClient *client = NULL;

	/* create the config dir if necessary. */

	uri = get_home_relative_uri (RC_DIR);

	if (uri_is_file (uri)) { /* before the mateconf port this was a file, now it's folder. */
		GFile *file;

		file = g_file_new_for_uri (uri);
		g_file_delete (file, NULL, NULL);
		g_object_unref (file);
	}

	ensure_dir_exists (uri, 0700, NULL);
	g_free (uri);

	if (eel_mateconf_get_boolean (PREF_MIGRATE_DIRECTORIES, TRUE))
		migrate_to_new_directories ();

	register_commands ();
	compute_supported_archive_types ();

	client = egg_sm_client_get ();
	if (egg_sm_client_is_resumed (client)) {
		fr_restore_session (client);
		return;
	}

	/**/

	if (remaining_args == NULL) { /* No archive specified. */
		gtk_widget_show (fr_window_new ());
		return;
	}

	if (extract_to != NULL)
		extract_to_path = get_uri_from_command_line (extract_to);

	if (add_to != NULL)
		add_to_path = get_uri_from_command_line (add_to);

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

		fr_window_new_batch (FR_WINDOW (window));
		fr_window_set_batch__add (FR_WINDOW (window), add_to_path, file_list);
		fr_window_append_batch_action (FR_WINDOW (window),
					       FR_BATCH_ACTION_QUIT,
					       NULL,
					       NULL);
		fr_window_start_batch (FR_WINDOW (window));
	}
	else if ((extract_to != NULL)
		  || (extract == 1)
		  || (extract_here == 1)) { /* Extract all archives. */
		GtkWidget  *window;
		const char *archive;
		int         i = 0;

		window = fr_window_new ();
		if (default_url != NULL)
			fr_window_set_default_dir (FR_WINDOW (window), default_url, TRUE);

		fr_window_new_batch (FR_WINDOW (window));
		while ((archive = remaining_args[i++]) != NULL) {
			char *archive_uri;

			archive_uri = get_uri_from_command_line (archive);
			if (extract_here == 1)
				fr_window_set_batch__extract_here (FR_WINDOW (window),
								   archive_uri);
			else
				fr_window_set_batch__extract (FR_WINDOW (window),
							      archive_uri,
							      extract_to_path);
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
			gtk_widget_show (window);

			file = g_file_new_for_commandline_arg (filename);
			uri = g_file_get_uri (file);
			fr_window_archive_open (FR_WINDOW (window), uri, GTK_WINDOW (window));
			g_free (uri);
			g_object_unref (file);
		}
	}

	g_free (add_to_path);
	g_free (extract_to_path);
}

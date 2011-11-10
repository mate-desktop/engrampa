/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  File-Roller
 *
 *  Copyright (C) 2007 Free Software Foundation, Inc.
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
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307, USA.
 */

#include <config.h>
#include <math.h>
#include <string.h>

#include <glib/gi18n.h>
#include <gio/gio.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "actions.h"
#include "dlg-batch-add.h"
#include "dlg-delete.h"
#include "dlg-extract.h"
#include "dlg-open-with.h"
#include "dlg-ask-password.h"
#include "dlg-package-installer.h"
#include "dlg-update.h"
#include "eggtreemultidnd.h"
#include "fr-marshal.h"
#include "fr-list-model.h"
#include "fr-archive.h"
#include "fr-error.h"
#include "fr-stock.h"
#include "fr-window.h"
#include "file-data.h"
#include "file-utils.h"
#include "glib-utils.h"
#include "main.h"
#include "gtk-utils.h"
#include "mateconf-utils.h"
#include "open-file.h"
#include "typedefs.h"
#include "ui.h"


#define LAST_OUTPUT_DIALOG_NAME "last-output"
#define MAX_HISTORY_LEN 5
#define ACTIVITY_DELAY 100
#define ACTIVITY_PULSE_STEP (0.033)
#define MAX_MESSAGE_LENGTH 50

#define PROGRESS_DIALOG_DEFAULT_WIDTH 400
#define PROGRESS_TIMEOUT_MSECS 5000
#define PROGRESS_BAR_HEIGHT 10
#undef  LOG_PROGRESS

#define HIDE_PROGRESS_TIMEOUT_MSECS 500
#define DEFAULT_NAME_COLUMN_WIDTH 250
#define OTHER_COLUMNS_WIDTH 100
#define RECENT_ITEM_MAX_WIDTH 25

#define DEF_WIN_WIDTH 600
#define DEF_WIN_HEIGHT 480
#define DEF_SIDEBAR_WIDTH 200

#define FILE_LIST_ICON_SIZE GTK_ICON_SIZE_LARGE_TOOLBAR
#define DIR_TREE_ICON_SIZE GTK_ICON_SIZE_MENU

#define BAD_CHARS "/\\*"

static GHashTable     *pixbuf_hash = NULL;
static GHashTable     *tree_pixbuf_hash = NULL;
static GtkIconTheme   *icon_theme = NULL;
static int             file_list_icon_size = 0;
static int             dir_tree_icon_size = 0;

#define XDS_FILENAME "xds.txt"
#define MAX_XDS_ATOM_VAL_LEN 4096
#define XDS_ATOM   gdk_atom_intern  ("XdndDirectSave0", FALSE)
#define TEXT_ATOM  gdk_atom_intern  ("text/plain", FALSE)
#define OCTET_ATOM gdk_atom_intern  ("application/octet-stream", FALSE)
#define XFR_ATOM   gdk_atom_intern  ("XdndFileRoller0", FALSE)

#define FR_CLIPBOARD (gdk_atom_intern_static_string ("_FILE_ROLLER_SPECIAL_CLIPBOARD"))
#define FR_SPECIAL_URI_LIST (gdk_atom_intern_static_string ("application/file-roller-uri-list"))

static GtkTargetEntry clipboard_targets[] = {
	{ "application/file-roller-uri-list", 0, 1 }
};

static GtkTargetEntry target_table[] = {
	{ "XdndFileRoller0", 0, 0 },
	{ "text/uri-list", 0, 1 },
};

static GtkTargetEntry folder_tree_targets[] = {
	{ "XdndFileRoller0", 0, 0 },
	{ "XdndDirectSave0", 0, 2 }
};


typedef struct {
	FrBatchActionType type;
	void *            data;
	GFreeFunc         free_func;
} FRBatchAction;


typedef struct {
	guint      converting : 1;
	char      *temp_dir;
	FrArchive *new_archive;
	char      *password;
	gboolean   encrypt_header;
	guint      volume_size;
	char      *new_file;
} FRConvertData;


typedef enum {
	FR_CLIPBOARD_OP_CUT,
	FR_CLIPBOARD_OP_COPY
} FRClipboardOp;


typedef struct {
	GList    *file_list;
	char     *extract_to_dir;
	char     *base_dir;
	gboolean  skip_older;
	gboolean  overwrite;
	gboolean  junk_paths;
	char     *password;
	gboolean  extract_here;
} ExtractData;


typedef enum {
	FR_WINDOW_AREA_MENUBAR,
	FR_WINDOW_AREA_TOOLBAR,
	FR_WINDOW_AREA_LOCATIONBAR,
	FR_WINDOW_AREA_CONTENTS,
	FR_WINDOW_AREA_FILTERBAR,
	FR_WINDOW_AREA_STATUSBAR,
} FrWindowArea;


typedef enum {
	DIALOG_RESPONSE_NONE = 1,
	DIALOG_RESPONSE_OPEN_ARCHIVE,
	DIALOG_RESPONSE_OPEN_DESTINATION_FOLDER,
	DIALOG_RESPONSE_QUIT
} DialogResponse;


/* -- FrClipboardData -- */


typedef struct {
	int            refs;
	char          *archive_filename;
	char          *archive_password;
	FRClipboardOp  op;
	char          *base_dir;
	GList         *files;
	char          *tmp_dir;
	char          *current_dir;
} FrClipboardData;


static FrClipboardData*
fr_clipboard_data_new (void)
{
	FrClipboardData *data;

	data = g_new0 (FrClipboardData, 1);
	data->refs = 1;

	return data;
}


static FrClipboardData *
fr_clipboard_data_ref (FrClipboardData *clipboard_data)
{
	clipboard_data->refs++;
	return clipboard_data;
}


static void
fr_clipboard_data_unref (FrClipboardData *clipboard_data)
{
	if (clipboard_data == NULL)
		return;
	if (--clipboard_data->refs > 0)
		return;

	g_free (clipboard_data->archive_filename);
	g_free (clipboard_data->archive_password);
	g_free (clipboard_data->base_dir);
	g_free (clipboard_data->tmp_dir);
	g_free (clipboard_data->current_dir);
	g_list_foreach (clipboard_data->files, (GFunc) g_free, NULL);
	g_list_free (clipboard_data->files);
	g_free (clipboard_data);
}


static void
fr_clipboard_data_set_password (FrClipboardData *clipboard_data,
				const char      *password)
{
	if (clipboard_data->archive_password != password)
		g_free (clipboard_data->archive_password);
	if (password != NULL)
		clipboard_data->archive_password = g_strdup (password);
}


/**/

enum {
	ARCHIVE_LOADED,
	LAST_SIGNAL
};

static GtkWindowClass *parent_class = NULL;
static guint fr_window_signals[LAST_SIGNAL] = { 0 };

struct _FrWindowPrivateData {
	GtkWidget         *layout;
	GtkWidget         *contents;
	GtkWidget         *list_view;
	GtkListStore      *list_store;
	GtkWidget         *tree_view;
	GtkTreeStore      *tree_store;
	GtkWidget         *toolbar;
	GtkWidget         *statusbar;
	GtkWidget         *progress_bar;
	GtkWidget         *location_bar;
	GtkWidget         *location_entry;
	GtkWidget         *location_label;
	GtkWidget         *filter_bar;
	GtkWidget         *filter_entry;
	GtkWidget         *paned;
	GtkWidget         *sidepane;
	GtkTreePath       *tree_hover_path;
	GtkTreePath       *list_hover_path;
	GtkTreeViewColumn *filename_column;

	gboolean         filter_mode;
	gint             current_view_length;

	guint            help_message_cid;
	guint            list_info_cid;
	guint            progress_cid;
	char *           last_status_message;

	GtkWidget *      up_arrows[5];
	GtkWidget *      down_arrows[5];

	FrAction         action;
	gboolean         archive_present;
	gboolean         archive_new;        /* A new archive has been created
					      * but it doesn't contain any
					      * file yet.  The real file will
					      * be created only when the user
					      * adds some file to the
					      * archive.*/

	char *           archive_uri;
	char *           open_default_dir;    /* default directory to be used
					       * in the Open dialog. */
	char *           add_default_dir;     /* default directory to be used
					       * in the Add dialog. */
	char *           extract_default_dir; /* default directory to be used
					       * in the Extract dialog. */
	gboolean         freeze_default_dir;
	gboolean         asked_for_password;
	gboolean         ask_to_open_destination_after_extraction;
	gboolean         destroy_with_error_dialog;

	FRBatchAction    current_batch_action;

	gboolean         give_focus_to_the_list;
	gboolean         single_click;
	GtkTreePath     *path_clicked;

	FrWindowSortMethod sort_method;
	GtkSortType      sort_type;

	char *           last_location;

	gboolean         view_folders;
	FrWindowListMode list_mode;
	FrWindowListMode last_list_mode;
	GList *          history;
	GList *          history_current;
	char *           password;
	char *           password_for_paste;
	gboolean         encrypt_header;
	FrCompression    compression;
	guint            volume_size;

	guint            activity_timeout_handle;   /* activity timeout
						     * handle. */
	gint             activity_ref;              /* when > 0 some activity
						     * is present. */

	guint            update_timeout_handle;     /* update file list
						     * timeout handle. */

	FRConvertData    convert_data;

	gboolean         stoppable;
	gboolean         closing;

	FrClipboardData *clipboard_data;
	FrClipboardData *copy_data;

	FrArchive       *copy_from_archive;

	GtkActionGroup  *actions;

	GtkRecentManager *recent_manager;
	GtkWidget        *recent_chooser_menu;
	GtkWidget        *recent_chooser_toolbar;

	GtkWidget        *file_popup_menu;
	GtkWidget        *folder_popup_menu;
	GtkWidget        *sidebar_folder_popup_menu;
	GtkWidget        *mitem_recents_menu;
	GtkWidget        *recent_toolbar_menu;
	GtkAction        *open_action;

	/* dragged files data */

	char             *drag_destination_folder;
	char             *drag_base_dir;
	GError           *drag_error;
	GList            *drag_file_list;        /* the list of files we are
					 	  * dragging*/

	/* progress dialog data */

	GtkWidget        *progress_dialog;
	GtkWidget        *pd_action;
	GtkWidget        *pd_archive;
	GtkWidget        *pd_message;
	GtkWidget        *pd_progress_bar;
	GtkWidget        *pd_cancel_button;
	GtkWidget        *pd_close_button;
	GtkWidget        *pd_open_archive_button;
	GtkWidget        *pd_open_destination_button;
	GtkWidget        *pd_quit_button;
	gboolean          progress_pulse;
	guint             progress_timeout;  /* Timeout to display the progress dialog. */
	guint             hide_progress_timeout;  /* Timeout to hide the progress dialog. */
	FrAction          pd_last_action;
	char             *pd_last_archive;
	char             *working_archive;

	/* update dialog data */

	gpointer          update_dialog;
	GList            *open_files;

	/* batch mode data */

	gboolean          batch_mode;          /* whether we are in a non interactive
					 	* mode. */
	GList            *batch_action_list;   /* FRBatchAction * elements */
	GList            *batch_action;        /* current action. */

	/* misc */

	guint             cnxn_id[MATECONF_NOTIFICATIONS];

	gulong            theme_changed_handler_id;
	gboolean          non_interactive;
	char             *extract_here_dir;
	gboolean          extract_interact_use_default_dir;
	gboolean          update_dropped_files;
	gboolean          batch_adding_one_file;

	GtkWindow        *load_error_parent_window;
	gboolean          showing_error_dialog;
	GtkWindow        *error_dialog_parent;
};


/* -- fr_window_free_private_data -- */


static void
fr_window_remove_notifications (FrWindow *window)
{
	int i;

	for (i = 0; i < MATECONF_NOTIFICATIONS; i++)
		if (window->priv->cnxn_id[i] != -1)
			eel_mateconf_notification_remove (window->priv->cnxn_id[i]);
}


static void
fr_window_free_batch_data (FrWindow *window)
{
	GList *scan;

	for (scan = window->priv->batch_action_list; scan; scan = scan->next) {
		FRBatchAction *adata = scan->data;

		if ((adata->data != NULL) && (adata->free_func != NULL))
			(*adata->free_func) (adata->data);
		g_free (adata);
	}

	g_list_free (window->priv->batch_action_list);
	window->priv->batch_action_list = NULL;
	window->priv->batch_action = NULL;
}


static void
gh_unref_pixbuf (gpointer key,
		 gpointer value,
		 gpointer user_data)
{
	g_object_unref (value);
}


static void
fr_window_clipboard_remove_file_list (FrWindow *window,
				      GList    *file_list)
{
	GList *scan1;

	if (window->priv->copy_data == NULL)
		return;

	if (file_list == NULL) {
		fr_clipboard_data_unref	 (window->priv->copy_data);
		window->priv->copy_data = NULL;
		return;
	}

	for (scan1 = file_list; scan1; scan1 = scan1->next) {
		const char *name1 = scan1->data;
		GList      *scan2;

		for (scan2 = window->priv->copy_data->files; scan2;) {
			const char *name2 = scan2->data;

			if (strcmp (name1, name2) == 0) {
				GList *tmp = scan2->next;
				window->priv->copy_data->files = g_list_remove_link (window->priv->copy_data->files, scan2);
				g_free (scan2->data);
				g_list_free (scan2);
				scan2 = tmp;
			}
			else
				scan2 = scan2->next;
		}
	}

	if (window->priv->copy_data->files == NULL) {
		fr_clipboard_data_unref (window->priv->copy_data);
		window->priv->copy_data = NULL;
	}
}


static void
fr_window_history_clear (FrWindow *window)
{
	if (window->priv->history != NULL)
		path_list_free (window->priv->history);
	window->priv->history = NULL;
	window->priv->history_current = NULL;
	g_free (window->priv->last_location);
	window->priv->last_location = NULL;
}


static void
fr_window_free_open_files (FrWindow *window)
{
	GList *scan;

	for (scan = window->priv->open_files; scan; scan = scan->next) {
		OpenFile *file = scan->data;

		if (file->monitor != NULL)
			g_file_monitor_cancel (file->monitor);
		open_file_free (file);
	}
	g_list_free (window->priv->open_files);
	window->priv->open_files = NULL;
}


static void
fr_window_convert_data_free (FrWindow   *window,
			     gboolean    all)
{
	if (all) {
		g_free (window->priv->convert_data.new_file);
		window->priv->convert_data.new_file = NULL;
	}

	window->priv->convert_data.converting = FALSE;

	if (window->priv->convert_data.temp_dir != NULL) {
		g_free (window->priv->convert_data.temp_dir);
		window->priv->convert_data.temp_dir = NULL;
	}

	if (window->priv->convert_data.new_archive != NULL) {
		g_object_unref (window->priv->convert_data.new_archive);
		window->priv->convert_data.new_archive = NULL;
	}

	if (window->priv->convert_data.password != NULL) {
		g_free (window->priv->convert_data.password);
		window->priv->convert_data.password = NULL;
	}
}


static void
fr_window_free_private_data (FrWindow *window)
{
	FrWindowPrivateData *priv = window->priv;

	if (priv->update_timeout_handle != 0) {
		g_source_remove (priv->update_timeout_handle);
		priv->update_timeout_handle = 0;
	}

	fr_window_remove_notifications (window);

	if (priv->open_action != NULL) {
		g_object_unref (priv->open_action);
		priv->open_action = NULL;
	}

	if (priv->recent_toolbar_menu != NULL) {
		gtk_widget_destroy (priv->recent_toolbar_menu);
		priv->recent_toolbar_menu = NULL;
	}

	while (priv->activity_ref > 0)
		fr_window_stop_activity_mode (window);

	if (priv->progress_timeout != 0) {
		g_source_remove (priv->progress_timeout);
		priv->progress_timeout = 0;
	}

	if (priv->hide_progress_timeout != 0) {
		g_source_remove (priv->hide_progress_timeout);
		priv->hide_progress_timeout = 0;
	}

	if (priv->theme_changed_handler_id != 0)
		g_signal_handler_disconnect (icon_theme, priv->theme_changed_handler_id);

	fr_window_history_clear (window);

	g_free (priv->open_default_dir);
	g_free (priv->add_default_dir);
	g_free (priv->extract_default_dir);
	g_free (priv->archive_uri);

	g_free (priv->password);
	g_free (priv->password_for_paste);

	g_object_unref (priv->list_store);

	if (window->priv->clipboard_data != NULL) {
		fr_clipboard_data_unref (window->priv->clipboard_data);
		window->priv->clipboard_data = NULL;
	}
	if (window->priv->copy_data != NULL) {
		fr_clipboard_data_unref (window->priv->copy_data);
		window->priv->copy_data = NULL;
	}
	if (priv->copy_from_archive != NULL) {
		g_object_unref (priv->copy_from_archive);
		priv->copy_from_archive = NULL;
	}

	fr_window_free_open_files (window);

	fr_window_convert_data_free (window, TRUE);

	g_clear_error (&priv->drag_error);
	path_list_free (priv->drag_file_list);
	priv->drag_file_list = NULL;

	if (priv->file_popup_menu != NULL) {
		gtk_widget_destroy (priv->file_popup_menu);
		priv->file_popup_menu = NULL;
	}

	if (priv->folder_popup_menu != NULL) {
		gtk_widget_destroy (priv->folder_popup_menu);
		priv->folder_popup_menu = NULL;
	}

	if (priv->sidebar_folder_popup_menu != NULL) {
		gtk_widget_destroy (priv->sidebar_folder_popup_menu);
		priv->sidebar_folder_popup_menu = NULL;
	}

	g_free (window->priv->last_location);

	fr_window_free_batch_data (window);
	fr_window_reset_current_batch_action (window);

	g_free (priv->pd_last_archive);
	g_free (priv->extract_here_dir);
	g_free (priv->last_status_message);

	preferences_set_sort_method (priv->sort_method);
	preferences_set_sort_type (priv->sort_type);
	preferences_set_list_mode (priv->last_list_mode);
}


static void
fr_window_finalize (GObject *object)
{
	FrWindow *window = FR_WINDOW (object);

	fr_window_free_open_files (window);

	if (window->archive != NULL) {
		g_object_unref (window->archive);
		window->archive = NULL;
	}

	if (window->priv != NULL) {
		fr_window_free_private_data (window);
		g_free (window->priv);
		window->priv = NULL;
	}

	WindowList = g_list_remove (WindowList, window);

	G_OBJECT_CLASS (parent_class)->finalize (object);

	if (WindowList == NULL) {
		if (pixbuf_hash != NULL) {
			g_hash_table_foreach (pixbuf_hash,
					      gh_unref_pixbuf,
					      NULL);
			g_hash_table_destroy (pixbuf_hash);
		}
		if (tree_pixbuf_hash != NULL) {
			g_hash_table_foreach (tree_pixbuf_hash,
					      gh_unref_pixbuf,
					      NULL);
			g_hash_table_destroy (tree_pixbuf_hash);
		}

		gtk_main_quit ();
	}
}


static gboolean
close__step2 (gpointer data)
{
	gtk_widget_destroy (GTK_WIDGET (data));
	return FALSE;
}


void
fr_window_close (FrWindow *window)
{
	if (window->priv->activity_ref > 0)
		return;

	window->priv->closing = TRUE;

	if (gtk_widget_get_realized (GTK_WIDGET (window))) {
		int width, height;

		#if GTK_CHECK_VERSION(3, 0, 0)
			width = gdk_window_get_width(gtk_widget_get_window(GTK_WIDGET(window)));
			height = gdk_window_get_height(gtk_widget_get_window(GTK_WIDGET(window)));
		#else
			gdk_drawable_get_size(gtk_widget_get_window(GTK_WIDGET(window)), &width, &height);
		#endif

		eel_mateconf_set_integer (PREF_UI_WINDOW_WIDTH, width);
		eel_mateconf_set_integer (PREF_UI_WINDOW_HEIGHT, height);

		width = gtk_paned_get_position (GTK_PANED (window->priv->paned));
		if (width > 0)
			eel_mateconf_set_integer (PREF_UI_SIDEBAR_WIDTH, width);

		width = gtk_tree_view_column_get_width (window->priv->filename_column);
		if (width > 0)
			eel_mateconf_set_integer (PREF_NAME_COLUMN_WIDTH, width);
	}

	g_idle_add (close__step2, window);
}


static void
fr_window_class_init (FrWindowClass *class)
{
	GObjectClass   *gobject_class;
	GtkWidgetClass *widget_class;

	parent_class = g_type_class_peek_parent (class);

	fr_window_signals[ARCHIVE_LOADED] =
		g_signal_new ("archive-loaded",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (FrWindowClass, archive_loaded),
			      NULL, NULL,
			      fr_marshal_VOID__BOOLEAN,
			      G_TYPE_NONE, 1,
			      G_TYPE_BOOLEAN);

	gobject_class = (GObjectClass*) class;
	gobject_class->finalize = fr_window_finalize;

	widget_class = (GtkWidgetClass*) class;
}


static void fr_window_update_paste_command_sensitivity (FrWindow *, GtkClipboard *);


static void
clipboard_owner_change_cb (GtkClipboard *clipboard,
			   GdkEvent     *event,
			   gpointer      user_data)
{
	fr_window_update_paste_command_sensitivity ((FrWindow *) user_data, clipboard);
}


static void
fr_window_realized (GtkWidget *window,
		    gpointer  *data)
{
	GtkClipboard *clipboard;

	clipboard = gtk_widget_get_clipboard (window, FR_CLIPBOARD);
	g_signal_connect (clipboard,
			  "owner_change",
			  G_CALLBACK (clipboard_owner_change_cb),
			  window);
}


static void
fr_window_unrealized (GtkWidget *window,
		      gpointer  *data)
{
	GtkClipboard *clipboard;

	clipboard = gtk_widget_get_clipboard (window, FR_CLIPBOARD);
	g_signal_handlers_disconnect_by_func (clipboard,
					      G_CALLBACK (clipboard_owner_change_cb),
					      window);
}


static void
fr_window_init (FrWindow *window)
{
	window->priv = g_new0 (FrWindowPrivateData, 1);
	window->priv->update_dropped_files = FALSE;
	window->priv->filter_mode = FALSE;

	g_signal_connect (window,
			  "realize",
			  G_CALLBACK (fr_window_realized),
			  NULL);
	g_signal_connect (window,
			  "unrealize",
			  G_CALLBACK (fr_window_unrealized),
			  NULL);

	WindowList = g_list_prepend (WindowList, window);
}


GType
fr_window_get_type (void)
{
	static GType type = 0;

	if (! type) {
		GTypeInfo type_info = {
			sizeof (FrWindowClass),
			NULL,
			NULL,
			(GClassInitFunc) fr_window_class_init,
			NULL,
			NULL,
			sizeof (FrWindow),
			0,
			(GInstanceInitFunc) fr_window_init
		};

		type = g_type_register_static (GTK_TYPE_WINDOW,
					       "FrWindow",
					       &type_info,
					       0);
	}

	return type;
}


/* -- window history -- */


#if 0
static void
fr_window_history_print (FrWindow *window)
{
	GList *list;

	debug (DEBUG_INFO, "history:\n");
	for (list = window->priv->history; list; list = list->next)
		g_print ("\t%s %s\n",
			 (char*) list->data,
			 (list == window->priv->history_current)? "<-": "");
	g_print ("\n");
}
#endif


static void
fr_window_history_add (FrWindow   *window,
		       const char *path)
{
	if ((window->priv->history != NULL) && (window->priv->history_current != NULL)) {
		if (strcmp (window->priv->history_current->data, path) == 0)
			return;

		/* Add locations visited using the back button to the history
		 * list. */
		if (window->priv->history != window->priv->history_current) {
			GList *scan = window->priv->history->next;
			while (scan != window->priv->history_current->next) {
				window->priv->history = g_list_prepend (window->priv->history, g_strdup (scan->data));
				scan = scan->next;
			}
		}
	}

	window->priv->history = g_list_prepend (window->priv->history, g_strdup (path));
	window->priv->history_current = window->priv->history;
}


static void
fr_window_history_pop (FrWindow *window)
{
	GList *first;

	if (window->priv->history == NULL)
		return;

	first = window->priv->history;
	window->priv->history = g_list_remove_link (window->priv->history, first);
	if (window->priv->history_current == first)
		window->priv->history_current = window->priv->history;
	g_free (first->data);
	g_list_free (first);
}


/* -- window_update_file_list -- */


static GPtrArray *
fr_window_get_current_dir_list (FrWindow *window)
{
	GPtrArray *files;
	int        i;

	files = g_ptr_array_sized_new (128);

	for (i = 0; i < window->archive->command->files->len; i++) {
		FileData *fdata = g_ptr_array_index (window->archive->command->files, i);

		if (fdata->list_name == NULL)
			continue;
		g_ptr_array_add (files, fdata);
	}

	return files;
}


static gint
sort_by_name (gconstpointer  ptr1,
	      gconstpointer  ptr2)
{
	FileData *fdata1 = *((FileData **) ptr1);
	FileData *fdata2 = *((FileData **) ptr2);

	if (file_data_is_dir (fdata1) != file_data_is_dir (fdata2)) {
		if (file_data_is_dir (fdata1))
			return -1;
		else
			return 1;
	}

	return strcasecmp (fdata1->list_name, fdata2->list_name);
}


static gint
sort_by_size (gconstpointer  ptr1,
	      gconstpointer  ptr2)
{
	FileData *fdata1 = *((FileData **) ptr1);
	FileData *fdata2 = *((FileData **) ptr2);

	if (file_data_is_dir (fdata1) != file_data_is_dir (fdata2)) {
		if (file_data_is_dir (fdata1))
			return -1;
		else
			return 1;
	}
	else if (file_data_is_dir (fdata1) && file_data_is_dir (fdata2)) {
		if (fdata1->dir_size > fdata2->dir_size)
			return 1;
		else
			return -1;
	}

	if (fdata1->size == fdata2->size)
		return sort_by_name (ptr1, ptr2);
	else if (fdata1->size > fdata2->size)
		return 1;
	else
		return -1;
}


static gint
sort_by_type (gconstpointer  ptr1,
	      gconstpointer  ptr2)
{
	FileData    *fdata1 = *((FileData **) ptr1);
	FileData    *fdata2 = *((FileData **) ptr2);
	int          result;
	const char  *desc1, *desc2;

	if (file_data_is_dir (fdata1) != file_data_is_dir (fdata2)) {
		if (file_data_is_dir (fdata1))
			return -1;
		else
			return 1;
	}
	else if (file_data_is_dir (fdata1) && file_data_is_dir (fdata2))
		return sort_by_name (ptr1, ptr2);

	desc1 = g_content_type_get_description (fdata1->content_type);
	desc2 = g_content_type_get_description (fdata2->content_type);

	result = strcasecmp (desc1, desc2);
	if (result == 0)
		return sort_by_name (ptr1, ptr2);
	else
		return result;
}


static gint
sort_by_time (gconstpointer  ptr1,
	      gconstpointer  ptr2)
{
	FileData *fdata1 = *((FileData **) ptr1);
	FileData *fdata2 = *((FileData **) ptr2);

	if (file_data_is_dir (fdata1) != file_data_is_dir (fdata2)) {
		if (file_data_is_dir (fdata1))
			return -1;
		else
			return 1;
	}
	else if (file_data_is_dir (fdata1) && file_data_is_dir (fdata2))
		return sort_by_name (ptr1, ptr2);

	if (fdata1->modified == fdata2->modified)
		return sort_by_name (ptr1, ptr2);
	else if (fdata1->modified > fdata2->modified)
		return 1;
	else
		return -1;
}


static gint
sort_by_path (gconstpointer  ptr1,
	      gconstpointer  ptr2)
{
	FileData *fdata1 = *((FileData **) ptr1);
	FileData *fdata2 = *((FileData **) ptr2);
	int       result;

	if (file_data_is_dir (fdata1) != file_data_is_dir (fdata2)) {
		if (file_data_is_dir (fdata1))
			return -1;
		else
			return 1;
	}
	else if (file_data_is_dir (fdata1) && file_data_is_dir (fdata2))
		return sort_by_name (ptr1, ptr2);

	/* 2 files */

	result = strcasecmp (fdata1->path, fdata2->path);
	if (result == 0)
		return sort_by_name (ptr1, ptr2);
	else
		return result;
}


static guint64
get_dir_size (FrWindow   *window,
	      const char *current_dir,
	      const char *name)
{
	guint64  size;
	char    *dirname;
	int      dirname_l;
	int      i;

	dirname = g_strconcat (current_dir, name, "/", NULL);
	dirname_l = strlen (dirname);

	size = 0;
	for (i = 0; i < window->archive->command->files->len; i++) {
		FileData *fd = g_ptr_array_index (window->archive->command->files, i);

		if (strncmp (dirname, fd->full_path, dirname_l) == 0)
			size += fd->size;
	}

	g_free (dirname);

	return size;
}


static gboolean
file_data_respects_filter (FrWindow *window,
			   FileData *fdata)
{
	const char *filter;

	filter = gtk_entry_get_text (GTK_ENTRY (window->priv->filter_entry));
	if ((fdata == NULL) || (filter == NULL) || (*filter == '\0'))
		return TRUE;

	if (fdata->dir || (fdata->name == NULL))
		return FALSE;

	return strncasecmp (fdata->name, filter, strlen (filter)) == 0;
}


static gboolean
compute_file_list_name (FrWindow   *window,
			FileData   *fdata,
			const char *current_dir,
			int         current_dir_len,
			GHashTable *names_hash,
			gboolean   *different_name)
{
	register char *scan, *end;

	*different_name = FALSE;

	if (! file_data_respects_filter (window, fdata))
		return FALSE;

	if (window->priv->list_mode == FR_WINDOW_LIST_MODE_FLAT) {
		fdata->list_name = g_strdup (fdata->name);
		if (fdata->dir)
			fdata->dir_size = 0;
		return FALSE;
	}

	if (strncmp (fdata->full_path, current_dir, current_dir_len) != 0) {
		*different_name = TRUE;
		return FALSE;
	}

	if (strlen (fdata->full_path) == current_dir_len)
		return FALSE;

	scan = fdata->full_path + current_dir_len;
	end = strchr (scan, '/');
	if ((end == NULL) && ! fdata->dir) { /* file */
		fdata->list_name = g_strdup (scan);
	}
	else { /* folder */
		char *dir_name;

		if (end != NULL)
			dir_name = g_strndup (scan, end - scan);
		else
			dir_name = g_strdup (scan);

		/* avoid to insert duplicated folders */
		if (g_hash_table_lookup (names_hash, dir_name) != NULL) {
			g_free (dir_name);
			return FALSE;
		}
		g_hash_table_insert (names_hash, dir_name, GINT_TO_POINTER (1));

		if ((end != NULL) && (*(end + 1) != '\0'))
			fdata->list_dir = TRUE;
		fdata->list_name = dir_name;
		fdata->dir_size = get_dir_size (window, current_dir, dir_name);
	}

	return TRUE;
}


static void
fr_window_compute_list_names (FrWindow  *window,
			      GPtrArray *files)
{
	const char *current_dir;
	int         current_dir_len;
	GHashTable *names_hash;
	int         i;
	gboolean    visible_list_started = FALSE;
	gboolean    visible_list_completed = FALSE;
	gboolean    different_name;

	current_dir = fr_window_get_current_location (window);
	current_dir_len = strlen (current_dir);
	names_hash = g_hash_table_new (g_str_hash, g_str_equal);

	for (i = 0; i < files->len; i++) {
		FileData *fdata = g_ptr_array_index (files, i);

		g_free (fdata->list_name);
		fdata->list_name = NULL;
		fdata->list_dir = FALSE;

		/* the files array is sorted by path, when the visible list
		 * is started and we find a path that doesn't match the
		 * current_dir path, the following files can't match
		 * the current_dir path. */

		if (visible_list_completed)
			continue;

		if (compute_file_list_name (window, fdata, current_dir, current_dir_len, names_hash, &different_name)) {
			visible_list_started = TRUE;
		}
		else if (visible_list_started && different_name)
			visible_list_completed = TRUE;
	}

	g_hash_table_destroy (names_hash);
}


static gboolean
fr_window_dir_exists_in_archive (FrWindow   *window,
				 const char *dir_name)
{
	int dir_name_len;
	int i;

	if (dir_name == NULL)
		return FALSE;

	dir_name_len = strlen (dir_name);
	if (dir_name_len == 0)
		return TRUE;

	if (strcmp (dir_name, "/") == 0)
		return TRUE;

	for (i = 0; i < window->archive->command->files->len; i++) {
		FileData *fdata = g_ptr_array_index (window->archive->command->files, i);

		if (strncmp (dir_name, fdata->full_path, dir_name_len) == 0) {
			return TRUE;
		}
		else if (fdata->dir
			 && (fdata->full_path[strlen (fdata->full_path)] != '/')
			 && (strncmp (dir_name, fdata->full_path, dir_name_len - 1) == 0))
		{
			return TRUE;
		}
	}

	return FALSE;
}


static char *
get_parent_dir (const char *current_dir)
{
	char *dir;
	char *new_dir;
	char *retval;

	if (current_dir == NULL)
		return NULL;
	if (strcmp (current_dir, "/") == 0)
		return g_strdup ("/");

	dir = g_strdup (current_dir);
	dir[strlen (dir) - 1] = 0;
	new_dir = remove_level_from_path (dir);
	g_free (dir);

	if (new_dir[strlen (new_dir) - 1] == '/')
		retval = new_dir;
	else {
		retval = g_strconcat (new_dir, "/", NULL);
		g_free (new_dir);
	}

	return retval;
}


static void fr_window_update_statusbar_list_info (FrWindow *window);


static GdkPixbuf *
get_mime_type_icon (const char *mime_type)
{
	GdkPixbuf *pixbuf = NULL;

	pixbuf = g_hash_table_lookup (tree_pixbuf_hash, mime_type);
	if (pixbuf != NULL) {
		g_object_ref (G_OBJECT (pixbuf));
		return pixbuf;
	}

	pixbuf = get_mime_type_pixbuf (mime_type, file_list_icon_size, icon_theme);
	if (pixbuf == NULL)
		return NULL;

	pixbuf = gdk_pixbuf_copy (pixbuf);
	g_hash_table_insert (tree_pixbuf_hash, (gpointer) mime_type, pixbuf);
	g_object_ref (G_OBJECT (pixbuf));

	return pixbuf;
}


static GdkPixbuf *
get_icon (GtkWidget *widget,
	  FileData  *fdata)
{
	GdkPixbuf  *pixbuf = NULL;
	const char *content_type;
	GIcon      *icon;

	if (file_data_is_dir (fdata))
		content_type = MIME_TYPE_DIRECTORY;
	else
		content_type = fdata->content_type;

	/* look in the hash table. */

	pixbuf = g_hash_table_lookup (pixbuf_hash, content_type);
	if (pixbuf != NULL) {
		g_object_ref (G_OBJECT (pixbuf));
		return pixbuf;
	}

	icon = g_content_type_get_icon (content_type);
	pixbuf = get_icon_pixbuf (icon, file_list_icon_size, icon_theme);
	g_object_unref (icon);

	if (pixbuf == NULL)
		return NULL;

	pixbuf = gdk_pixbuf_copy (pixbuf);
	g_hash_table_insert (pixbuf_hash, (gpointer) content_type, pixbuf);
	g_object_ref (G_OBJECT (pixbuf));

	return pixbuf;
}


static GdkPixbuf *
get_emblem (GtkWidget *widget,
	    FileData  *fdata)
{
	GdkPixbuf *pixbuf = NULL;

	if (! fdata->encrypted)
		return NULL;

	/* encrypted */

	pixbuf = g_hash_table_lookup (pixbuf_hash, "emblem-nowrite");
	if (pixbuf != NULL) {
		g_object_ref (G_OBJECT (pixbuf));
		return pixbuf;
	}

	pixbuf = gtk_icon_theme_load_icon (icon_theme,
					   "emblem-nowrite",
					   file_list_icon_size,
					   0,
					   NULL);
	if (pixbuf == NULL)
		return NULL;

	pixbuf = gdk_pixbuf_copy (pixbuf);
	g_hash_table_insert (pixbuf_hash, (gpointer) "emblem-nowrite", pixbuf);
	g_object_ref (G_OBJECT (pixbuf));

	return pixbuf;
}


static int
get_column_from_sort_method (FrWindowSortMethod sort_method)
{
	switch (sort_method) {
	case FR_WINDOW_SORT_BY_NAME: return COLUMN_NAME;
	case FR_WINDOW_SORT_BY_SIZE: return COLUMN_SIZE;
	case FR_WINDOW_SORT_BY_TYPE: return COLUMN_TYPE;
	case FR_WINDOW_SORT_BY_TIME: return COLUMN_TIME;
	case FR_WINDOW_SORT_BY_PATH: return COLUMN_PATH;
	default:
		break;
	}

	return COLUMN_NAME;
}


static int
get_sort_method_from_column (int column_id)
{
	switch (column_id) {
	case COLUMN_NAME: return FR_WINDOW_SORT_BY_NAME;
	case COLUMN_SIZE: return FR_WINDOW_SORT_BY_SIZE;
	case COLUMN_TYPE: return FR_WINDOW_SORT_BY_TYPE;
	case COLUMN_TIME: return FR_WINDOW_SORT_BY_TIME;
	case COLUMN_PATH: return FR_WINDOW_SORT_BY_PATH;
	default:
		break;
	}

	return FR_WINDOW_SORT_BY_NAME;
}


static void
add_selected_from_list_view (GtkTreeModel *model,
			     GtkTreePath  *path,
			     GtkTreeIter  *iter,
			     gpointer      data)
{
	GList    **list = data;
	FileData  *fdata;

	gtk_tree_model_get (model, iter,
			    COLUMN_FILE_DATA, &fdata,
			    -1);
	*list = g_list_prepend (*list, fdata);
}


static void
add_selected_from_tree_view (GtkTreeModel *model,
			     GtkTreePath  *path,
			     GtkTreeIter  *iter,
			     gpointer      data)
{
	GList **list = data;
	char   *dir_path;

	gtk_tree_model_get (model, iter,
			    TREE_COLUMN_PATH, &dir_path,
			    -1);
	*list = g_list_prepend (*list, dir_path);
}


static void
add_selected_fd (GtkTreeModel *model,
		 GtkTreePath  *path,
		 GtkTreeIter  *iter,
		 gpointer      data)
{
	GList    **list = data;
	FileData  *fdata;

	gtk_tree_model_get (model, iter,
			    COLUMN_FILE_DATA, &fdata,
			    -1);
	if (! fdata->list_dir)
		*list = g_list_prepend (*list, fdata);
}


static GList *
get_selection_as_fd (FrWindow *window)
{
	GtkTreeSelection *selection;
	GList            *list = NULL;

	if (! gtk_widget_get_realized (window->priv->list_view))
		return NULL;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (window->priv->list_view));
	if (selection == NULL)
		return NULL;
	gtk_tree_selection_selected_foreach (selection, add_selected_fd, &list);

	return list;
}


static void
fr_window_update_statusbar_list_info (FrWindow *window)
{
	char    *info, *archive_info, *selected_info;
	char    *size_txt, *sel_size_txt;
	int      tot_n, sel_n;
	goffset  tot_size, sel_size;
	GList   *scan;

	if (window == NULL)
		return;

	if ((window->archive == NULL) || (window->archive->command == NULL)) {
		gtk_statusbar_pop (GTK_STATUSBAR (window->priv->statusbar), window->priv->list_info_cid);
		return;
	}

	tot_n = 0;
	tot_size = 0;

	if (window->priv->archive_present) {
		GPtrArray *files = fr_window_get_current_dir_list (window);
		int        i;

		for (i = 0; i < files->len; i++) {
			FileData *fd = g_ptr_array_index (files, i);

			tot_n++;
			if (! file_data_is_dir (fd))
				tot_size += fd->size;
			else
				tot_size += fd->dir_size;
		}
		g_ptr_array_free (files, TRUE);
	}

	sel_n = 0;
	sel_size = 0;

	if (window->priv->archive_present) {
		GList *selection = get_selection_as_fd (window);

		for (scan = selection; scan; scan = scan->next) {
			FileData *fd = scan->data;

			sel_n++;
			if (! file_data_is_dir (fd))
				sel_size += fd->size;
		}
		g_list_free (selection);
	}

	size_txt = g_format_size_for_display (tot_size);
	sel_size_txt = g_format_size_for_display (sel_size);

	if (tot_n == 0)
		archive_info = g_strdup ("");
	else
		archive_info = g_strdup_printf (ngettext ("%d object (%s)", "%d objects (%s)", tot_n), tot_n, size_txt);

	if (sel_n == 0)
		selected_info = g_strdup ("");
	else
		selected_info = g_strdup_printf (ngettext ("%d object selected (%s)", "%d objects selected (%s)", sel_n), sel_n, sel_size_txt);

	info = g_strconcat (archive_info,
			    ((sel_n == 0) ? NULL : ", "),
			    selected_info,
			    NULL);

	gtk_statusbar_push (GTK_STATUSBAR (window->priv->statusbar), window->priv->list_info_cid, info);

	g_free (size_txt);
	g_free (sel_size_txt);
	g_free (archive_info);
	g_free (selected_info);
	g_free (info);
}


static void
fr_window_populate_file_list (FrWindow  *window,
			      GPtrArray *files)
{
	int i;

	gtk_list_store_clear (window->priv->list_store);

	gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (window->priv->list_store),
	 				      GTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID,
	 				      GTK_SORT_ASCENDING);

	for (i = 0; i < files->len; i++) {
		FileData    *fdata = g_ptr_array_index (files, i);
		GtkTreeIter  iter;
		GdkPixbuf   *icon, *emblem;
		char        *utf8_name;

		if (fdata->list_name == NULL)
			continue;

		gtk_list_store_append (window->priv->list_store, &iter);

		icon = get_icon (GTK_WIDGET (window), fdata);
		utf8_name = g_filename_display_name (fdata->list_name);
		emblem = get_emblem (GTK_WIDGET (window), fdata);

		if (file_data_is_dir (fdata)) {
			char *utf8_path;
			char *tmp;
			char *s_size;
			char *s_time;

			if (fdata->list_dir)
				tmp = remove_ending_separator (fr_window_get_current_location (window));

			else
				tmp = remove_level_from_path (fdata->path);
			utf8_path = g_filename_display_name (tmp);
			g_free (tmp);

			s_size = g_format_size_for_display (fdata->dir_size);

			if (fdata->list_dir)
				s_time = g_strdup ("");
			else
				s_time = get_time_string (fdata->modified);

			gtk_list_store_set (window->priv->list_store, &iter,
					    COLUMN_FILE_DATA, fdata,
					    COLUMN_ICON, icon,
					    COLUMN_NAME, utf8_name,
					    COLUMN_EMBLEM, emblem,
					    COLUMN_TYPE, _("Folder"),
					    COLUMN_SIZE, s_size,
					    COLUMN_TIME, s_time,
					    COLUMN_PATH, utf8_path,
					    -1);
			g_free (utf8_path);
			g_free (s_size);
			g_free (s_time);
		}
		else {
			char       *utf8_path;
			char       *s_size;
			char       *s_time;
			const char *desc;

			utf8_path = g_filename_display_name (fdata->path);

			s_size = g_format_size_for_display (fdata->size);
			s_time = get_time_string (fdata->modified);
			desc = g_content_type_get_description (fdata->content_type);

			gtk_list_store_set (window->priv->list_store, &iter,
					    COLUMN_FILE_DATA, fdata,
					    COLUMN_ICON, icon,
					    COLUMN_NAME, utf8_name,
					    COLUMN_EMBLEM, emblem,
					    COLUMN_TYPE, desc,
					    COLUMN_SIZE, s_size,
					    COLUMN_TIME, s_time,
					    COLUMN_PATH, utf8_path,
					    -1);
			g_free (utf8_path);
			g_free (s_size);
			g_free (s_time);
		}
		g_free (utf8_name);
		if (icon != NULL)
			g_object_unref (icon);
		if (emblem != NULL)
			g_object_unref (emblem);
	}

	gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (window->priv->list_store),
					      get_column_from_sort_method (window->priv->sort_method),
					      window->priv->sort_type);

	fr_window_update_statusbar_list_info (window);
	fr_window_stop_activity_mode (window);
}


static int
path_compare (gconstpointer a,
	      gconstpointer b)
{
	char *path_a = *((char**) a);
	char *path_b = *((char**) b);

	return strcmp (path_a, path_b);
}


static gboolean
get_tree_iter_from_path (FrWindow    *window,
			 const char  *path,
			 GtkTreeIter *parent,
			 GtkTreeIter *iter)
{
	gboolean    result = FALSE;

	if (! gtk_tree_model_iter_children (GTK_TREE_MODEL (window->priv->tree_store), iter, parent))
		return FALSE;

	do {
		GtkTreeIter  tmp;
		char        *iter_path;

		if (get_tree_iter_from_path (window, path, iter, &tmp)) {
			*iter = tmp;
			return TRUE;
		}

		gtk_tree_model_get (GTK_TREE_MODEL (window->priv->tree_store),
				    iter,
				    TREE_COLUMN_PATH, &iter_path,
				    -1);

		if ((iter_path != NULL) && (strcmp (path, iter_path) == 0)) {
			result = TRUE;
			g_free (iter_path);
			break;
		}
		g_free (iter_path);
	} while (gtk_tree_model_iter_next (GTK_TREE_MODEL (window->priv->tree_store), iter));

	return result;
}


static void
set_sensitive (FrWindow   *window,
	       const char *action_name,
	       gboolean    sensitive)
{
	GtkAction *action;

	action = gtk_action_group_get_action (window->priv->actions, action_name);
	g_object_set (action, "sensitive", sensitive, NULL);
}


static void
fr_window_update_current_location (FrWindow *window)
{
	const char *current_dir = fr_window_get_current_location (window);
	char       *path;
	GtkTreeIter iter;

	if (window->priv->list_mode == FR_WINDOW_LIST_MODE_FLAT) {
		gtk_widget_hide (window->priv->location_bar);
		return;
	}

	gtk_widget_show (window->priv->location_bar);

	gtk_entry_set_text (GTK_ENTRY (window->priv->location_entry), window->priv->archive_present? current_dir: "");

	set_sensitive (window, "GoBack", window->priv->archive_present && (current_dir != NULL) && (window->priv->history_current != NULL) && (window->priv->history_current->next != NULL));
	set_sensitive (window, "GoForward", window->priv->archive_present && (current_dir != NULL) && (window->priv->history_current != NULL) && (window->priv->history_current->prev != NULL));
	set_sensitive (window, "GoUp", window->priv->archive_present && (current_dir != NULL) && (strcmp (current_dir, "/") != 0));
	set_sensitive (window, "GoHome", window->priv->archive_present);
	gtk_widget_set_sensitive (window->priv->location_entry, window->priv->archive_present);
	gtk_widget_set_sensitive (window->priv->location_label, window->priv->archive_present);
	gtk_widget_set_sensitive (window->priv->filter_entry, window->priv->archive_present);

#if 0
	fr_window_history_print (window);
#endif

	path = remove_ending_separator (current_dir);
	if (get_tree_iter_from_path (window, path, NULL, &iter)) {
		GtkTreeSelection *selection;
		GtkTreePath      *t_path;

		t_path = gtk_tree_model_get_path (GTK_TREE_MODEL (window->priv->tree_store), &iter);
		gtk_tree_view_expand_to_path (GTK_TREE_VIEW (window->priv->tree_view), t_path);
		gtk_tree_path_free (t_path);

		selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (window->priv->tree_view));
		gtk_tree_selection_select_iter (selection, &iter);
	}
	g_free (path);
}


void
fr_window_update_dir_tree (FrWindow *window)
{
	GPtrArray  *dirs;
	GHashTable *dir_cache;
	int         i;
	GdkPixbuf  *icon;

	gtk_tree_store_clear (window->priv->tree_store);

	if (! window->priv->view_folders
	    || ! window->priv->archive_present
	    || (window->priv->list_mode == FR_WINDOW_LIST_MODE_FLAT))
	{
		gtk_widget_set_sensitive (window->priv->tree_view, FALSE);
		gtk_widget_hide (window->priv->sidepane);
		return;
	}
	else {
		gtk_widget_set_sensitive (window->priv->tree_view, TRUE);
		if (! gtk_widget_get_visible (window->priv->sidepane))
			gtk_widget_show_all (window->priv->sidepane);
	}

	if (gtk_widget_get_realized (window->priv->tree_view))
		gtk_tree_view_scroll_to_point (GTK_TREE_VIEW (window->priv->tree_view), 0, 0);

	/**/

	dirs = g_ptr_array_sized_new (128);

	dir_cache = g_hash_table_new_full (g_str_hash, g_str_equal, NULL, NULL);
	for (i = 0; i < window->archive->command->files->len; i++) {
		FileData *fdata = g_ptr_array_index (window->archive->command->files, i);
		char     *dir;

		if (gtk_entry_get_text (GTK_ENTRY (window->priv->filter_entry)) != NULL) {
			if (! file_data_respects_filter (window, fdata))
				continue;
		}

		if (fdata->dir)
			dir = remove_ending_separator (fdata->full_path);
		else
			dir = remove_level_from_path (fdata->full_path);

		while ((dir != NULL) && (strcmp (dir, "/") != 0)) {
			char *new_dir;

			if (g_hash_table_lookup (dir_cache, dir) != NULL)
				break;

			new_dir = dir;
			g_ptr_array_add (dirs, new_dir);
			g_hash_table_replace (dir_cache, new_dir, "1");

			dir = remove_level_from_path (new_dir);
		}

		g_free (dir);
	}
	g_hash_table_destroy (dir_cache);

	g_ptr_array_sort (dirs, path_compare);
	dir_cache = g_hash_table_new_full (g_str_hash, g_str_equal, NULL, (GDestroyNotify) gtk_tree_path_free);

	/**/

	icon = get_mime_type_icon (MIME_TYPE_ARCHIVE);
	{
		GtkTreeIter  node;
		char        *uri;
		char        *name;

		uri = g_file_get_uri (window->archive->file);
		name = g_uri_display_basename (uri);

		gtk_tree_store_append (window->priv->tree_store, &node, NULL);
		gtk_tree_store_set (window->priv->tree_store, &node,
				    TREE_COLUMN_ICON, icon,
				    TREE_COLUMN_NAME, name,
				    TREE_COLUMN_PATH, "/",
				    TREE_COLUMN_WEIGHT, PANGO_WEIGHT_BOLD,
				    -1);
		g_hash_table_replace (dir_cache, "/", gtk_tree_model_get_path (GTK_TREE_MODEL (window->priv->tree_store), &node));

		g_free (name);
		g_free (uri);
	}
	g_object_unref (icon);

	/**/

	icon = get_mime_type_icon (MIME_TYPE_DIRECTORY);
	for (i = 0; i < dirs->len; i++) {
		char        *dir = g_ptr_array_index (dirs, i);
		char        *parent_dir;
		GtkTreePath *parent_path;
		GtkTreeIter  parent;
		GtkTreeIter  node;

		parent_dir = remove_level_from_path (dir);
		if (parent_dir == NULL)
			continue;

		parent_path = g_hash_table_lookup (dir_cache, parent_dir);
		gtk_tree_model_get_iter (GTK_TREE_MODEL (window->priv->tree_store),
					 &parent,
					 parent_path);
		gtk_tree_store_append (window->priv->tree_store, &node, &parent);
		gtk_tree_store_set (window->priv->tree_store, &node,
				    TREE_COLUMN_ICON, icon,
				    TREE_COLUMN_NAME, file_name_from_path (dir),
				    TREE_COLUMN_PATH, dir,
				    TREE_COLUMN_WEIGHT, PANGO_WEIGHT_NORMAL,
				    -1);
		g_hash_table_replace (dir_cache, dir, gtk_tree_model_get_path (GTK_TREE_MODEL (window->priv->tree_store), &node));

		g_free (parent_dir);
	}
	g_hash_table_destroy (dir_cache);
	if (icon != NULL)
		g_object_unref (icon);

	g_ptr_array_free (dirs, TRUE);

	fr_window_update_current_location (window);
}


static void
fr_window_update_filter_bar_visibility (FrWindow *window)
{
	const char *filter;

	filter = gtk_entry_get_text (GTK_ENTRY (window->priv->filter_entry));
	if ((filter == NULL) || (*filter == '\0'))
		gtk_widget_hide (window->priv->filter_bar);
	else
		gtk_widget_show (window->priv->filter_bar);
}


static void
fr_window_update_file_list (FrWindow *window,
			    gboolean  update_view)
{
	GPtrArray  *files;
	gboolean    free_files = FALSE;

	if (gtk_widget_get_realized (window->priv->list_view))
		gtk_tree_view_scroll_to_point (GTK_TREE_VIEW (window->priv->list_view), 0, 0);

	if (! window->priv->archive_present || window->priv->archive_new) {
		if (update_view)
			gtk_list_store_clear (window->priv->list_store);

		window->priv->current_view_length = 0;

		if (window->priv->archive_new) {
			gtk_widget_set_sensitive (window->priv->list_view, TRUE);
			gtk_widget_show_all (gtk_widget_get_parent (window->priv->list_view));
		}
		else {
			gtk_widget_set_sensitive (window->priv->list_view, FALSE);
			gtk_widget_hide_all (gtk_widget_get_parent (window->priv->list_view));
		}

		return;
	}
	else {
		gtk_widget_set_sensitive (window->priv->list_view, TRUE);
		if (! gtk_widget_get_visible (window->priv->list_view))
			gtk_widget_show_all (gtk_widget_get_parent (window->priv->list_view));
	}

	if (window->priv->give_focus_to_the_list) {
		gtk_widget_grab_focus (window->priv->list_view);
		window->priv->give_focus_to_the_list = FALSE;
	}

	/**/

	fr_window_start_activity_mode (window);

	if (window->priv->list_mode == FR_WINDOW_LIST_MODE_FLAT) {
		fr_window_compute_list_names (window, window->archive->command->files);
		files = window->archive->command->files;
		free_files = FALSE;
	}
	else {
		char *current_dir = g_strdup (fr_window_get_current_location (window));

		while (! fr_window_dir_exists_in_archive (window, current_dir)) {
			char *tmp;

			fr_window_history_pop (window);

			tmp = get_parent_dir (current_dir);
			g_free (current_dir);
			current_dir = tmp;

			fr_window_history_add (window, current_dir);
		}
		g_free (current_dir);

		fr_window_compute_list_names (window, window->archive->command->files);
		files = fr_window_get_current_dir_list (window);
		free_files = TRUE;
	}

	if (files != NULL)
		window->priv->current_view_length = files->len;
	else
		window->priv->current_view_length = 0;

	if (update_view)
		fr_window_populate_file_list (window, files);

	if (free_files)
		g_ptr_array_free (files, TRUE);
}


void
fr_window_update_list_order (FrWindow *window)
{
	gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (window->priv->list_store), get_column_from_sort_method (window->priv->sort_method), window->priv->sort_type);
}


static void
fr_window_update_title (FrWindow *window)
{
	if (! window->priv->archive_present)
		gtk_window_set_title (GTK_WINDOW (window), _("Archive Manager"));
	else {
		char *title;
		char *name;

		name = g_uri_display_basename (fr_window_get_archive_uri (window));
		title = g_strdup_printf ("%s %s",
					 name,
					 window->archive->read_only ? _("[read only]") : "");

		gtk_window_set_title (GTK_WINDOW (window), title);
		g_free (title);
		g_free (name);
	}
}


static void
check_whether_has_a_dir (GtkTreeModel *model,
			 GtkTreePath  *path,
			 GtkTreeIter  *iter,
			 gpointer      data)
{
	gboolean *has_a_dir = data;
	FileData *fdata;

	gtk_tree_model_get (model, iter,
			    COLUMN_FILE_DATA, &fdata,
			    -1);
	if (file_data_is_dir (fdata))
		*has_a_dir = TRUE;
}


static gboolean
selection_has_a_dir (FrWindow *window)
{
	GtkTreeSelection *selection;
	gboolean          has_a_dir = FALSE;

	if (! gtk_widget_get_realized (window->priv->list_view))
		return FALSE;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (window->priv->list_view));
	if (selection == NULL)
		return FALSE;

	gtk_tree_selection_selected_foreach (selection,
					     check_whether_has_a_dir,
					     &has_a_dir);

	return has_a_dir;
}


static void
set_active (FrWindow   *window,
	    const char *action_name,
	    gboolean    is_active)
{
	GtkAction *action;

	action = gtk_action_group_get_action (window->priv->actions, action_name);
	gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), is_active);
}


static void
fr_window_update_paste_command_sensitivity (FrWindow     *window,
					    GtkClipboard *clipboard)
{
	gboolean running;
	gboolean no_archive;
	gboolean ro;
	gboolean compr_file;

	if (window->priv->closing)
		return;

	if (clipboard == NULL)
		clipboard = gtk_widget_get_clipboard (GTK_WIDGET (window), FR_CLIPBOARD);
	running    = window->priv->activity_ref > 0;
	no_archive = (window->archive == NULL) || ! window->priv->archive_present;
	ro         = ! no_archive && window->archive->read_only;
	compr_file = ! no_archive && window->archive->is_compressed_file;

	set_sensitive (window, "Paste", ! no_archive && ! ro && ! running && ! compr_file && (window->priv->list_mode != FR_WINDOW_LIST_MODE_FLAT) && gtk_clipboard_wait_is_target_available (clipboard, FR_SPECIAL_URI_LIST));
}


static void
fr_window_update_sensitivity (FrWindow *window)
{
	gboolean no_archive;
	gboolean ro;
	gboolean file_op;
	gboolean running;
	gboolean compr_file;
	gboolean sel_not_null;
	gboolean one_file_selected;
	gboolean dir_selected;
	int      n_selected;

	if (window->priv->batch_mode)
		return;

	running           = window->priv->activity_ref > 0;
	no_archive        = (window->archive == NULL) || ! window->priv->archive_present;
	ro                = ! no_archive && window->archive->read_only;
	file_op           = ! no_archive && ! window->priv->archive_new  && ! running;
	compr_file        = ! no_archive && window->archive->is_compressed_file;
	n_selected        = fr_window_get_n_selected_files (window);
	sel_not_null      = n_selected > 0;
	one_file_selected = n_selected == 1;
	dir_selected      = selection_has_a_dir (window);

	set_sensitive (window, "AddFiles", ! no_archive && ! ro && ! running && ! compr_file);
	set_sensitive (window, "AddFiles_Toolbar", ! no_archive && ! ro && ! running && ! compr_file);
	set_sensitive (window, "AddFolder", ! no_archive && ! ro && ! running && ! compr_file);
	set_sensitive (window, "AddFolder_Toolbar", ! no_archive && ! ro && ! running && ! compr_file);
	set_sensitive (window, "Copy", ! no_archive && ! ro && ! running && ! compr_file && sel_not_null && (window->priv->list_mode != FR_WINDOW_LIST_MODE_FLAT));
	set_sensitive (window, "Cut", ! no_archive && ! ro && ! running && ! compr_file && sel_not_null && (window->priv->list_mode != FR_WINDOW_LIST_MODE_FLAT));
	set_sensitive (window, "Delete", ! no_archive && ! ro && ! window->priv->archive_new && ! running && ! compr_file);
	set_sensitive (window, "DeselectAll", ! no_archive && sel_not_null);
	set_sensitive (window, "Extract", file_op);
	set_sensitive (window, "Extract_Toolbar", file_op);
	set_sensitive (window, "Find", ! no_archive);
	set_sensitive (window, "LastOutput", ((window->archive != NULL)
					      && (window->archive->process != NULL)
					      && (window->archive->process->out.raw != NULL)));
	set_sensitive (window, "New", ! running);
	set_sensitive (window, "Open", ! running);
	set_sensitive (window, "Open_Toolbar", ! running);
	set_sensitive (window, "OpenSelection", file_op && sel_not_null && ! dir_selected);
	set_sensitive (window, "OpenFolder", file_op && one_file_selected && dir_selected);
	set_sensitive (window, "Password", ! running && (window->priv->asked_for_password || (! no_archive && window->archive->command->propPassword)));
	set_sensitive (window, "Properties", file_op);
	set_sensitive (window, "Close", !running || window->priv->stoppable);
	set_sensitive (window, "Reload", ! (no_archive || running));
	set_sensitive (window, "Rename", ! no_archive && ! ro && ! running && ! compr_file && one_file_selected);
	set_sensitive (window, "SaveAs", ! no_archive && ! compr_file && ! running);
	set_sensitive (window, "SelectAll", ! no_archive);
	set_sensitive (window, "Stop", running && window->priv->stoppable);
	set_sensitive (window, "TestArchive", ! no_archive && ! running && window->archive->command->propTest);
	set_sensitive (window, "ViewSelection", file_op && one_file_selected && ! dir_selected);
	set_sensitive (window, "ViewSelection_Toolbar", file_op && one_file_selected && ! dir_selected);

	if (window->priv->progress_dialog != NULL)
		gtk_dialog_set_response_sensitive (GTK_DIALOG (window->priv->progress_dialog),
						   GTK_RESPONSE_OK,
						   running && window->priv->stoppable);

	fr_window_update_paste_command_sensitivity (window, NULL);

	set_sensitive (window, "SelectAll", (window->priv->current_view_length > 0) && (window->priv->current_view_length != n_selected));
	set_sensitive (window, "DeselectAll", n_selected > 0);
	set_sensitive (window, "OpenRecentMenu", ! running);

	set_sensitive (window, "ViewFolders", (window->priv->list_mode == FR_WINDOW_LIST_MODE_AS_DIR));

	set_sensitive (window, "ViewAllFiles", ! window->priv->filter_mode);
	set_sensitive (window, "ViewAsFolder", ! window->priv->filter_mode);
}


static gboolean
location_entry_key_press_event_cb (GtkWidget   *widget,
				   GdkEventKey *event,
				   FrWindow    *window)
{
	if ((event->keyval == GDK_Return)
	    || (event->keyval == GDK_KP_Enter)
	    || (event->keyval == GDK_ISO_Enter))
	{
		fr_window_go_to_location (window, gtk_entry_get_text (GTK_ENTRY (window->priv->location_entry)), FALSE);
	}

	return FALSE;
}


static gboolean
real_close_progress_dialog (gpointer data)
{
	FrWindow *window = data;

	if (window->priv->hide_progress_timeout != 0) {
		g_source_remove (window->priv->hide_progress_timeout);
		window->priv->hide_progress_timeout = 0;
	}

	if (window->priv->progress_dialog != NULL)
		gtk_widget_hide (window->priv->progress_dialog);

	return FALSE;
}


static void
close_progress_dialog (FrWindow *window,
		       gboolean  close_now)
{
	if (window->priv->progress_timeout != 0) {
		g_source_remove (window->priv->progress_timeout);
		window->priv->progress_timeout = 0;
	}

	if (! window->priv->batch_mode && gtk_widget_get_mapped (GTK_WIDGET (window)))
		gtk_widget_hide (window->priv->progress_bar);

	if (window->priv->progress_dialog == NULL)
		return;

	if (close_now) {
		if (window->priv->hide_progress_timeout != 0) {
			g_source_remove (window->priv->hide_progress_timeout);
			window->priv->hide_progress_timeout = 0;
		}
		real_close_progress_dialog (window);
	}
	else {
		if (window->priv->hide_progress_timeout != 0)
			return;
		window->priv->hide_progress_timeout = g_timeout_add (HIDE_PROGRESS_TIMEOUT_MSECS,
								     real_close_progress_dialog,
								     window);
	}
}


static gboolean
progress_dialog_delete_event (GtkWidget *caller,
			      GdkEvent  *event,
			      FrWindow  *window)
{
	if (window->priv->stoppable) {
		activate_action_stop (NULL, window);
		close_progress_dialog (window, TRUE);
	}

	return TRUE;
}


static void
open_folder (GtkWindow  *parent,
	     const char *folder)
{
	GError *error = NULL;

	if (folder == NULL)
		return;

	if (! show_uri (gtk_window_get_screen (parent), folder, GDK_CURRENT_TIME, &error)) {
		GtkWidget *d;
		char      *utf8_name;
		char      *message;

		utf8_name = g_filename_display_name (folder);
		message = g_strdup_printf (_("Could not display the folder \"%s\""), utf8_name);
		g_free (utf8_name);

		d = _gtk_error_dialog_new (parent,
					   GTK_DIALOG_MODAL,
					   NULL,
					   message,
					   "%s",
					   error->message);
		gtk_dialog_run (GTK_DIALOG (d));
		gtk_widget_destroy (d);

		g_free (message);
		g_clear_error (&error);
	}
}


static void
fr_window_view_extraction_destination_folder (FrWindow *window)
{
	open_folder (GTK_WINDOW (window), fr_archive_get_last_extraction_destination (window->archive));
}


static void
progress_dialog_response (GtkDialog *dialog,
			  int        response_id,
			  FrWindow  *window)
{
	GtkWidget *new_window;

	switch (response_id) {
	case GTK_RESPONSE_CANCEL:
		if (window->priv->stoppable) {
			activate_action_stop (NULL, window);
			close_progress_dialog (window, TRUE);
		}
		break;
	case GTK_RESPONSE_CLOSE:
		close_progress_dialog (window, TRUE);
		break;
	case DIALOG_RESPONSE_OPEN_ARCHIVE:
		new_window = fr_window_new ();
		gtk_widget_show (new_window);
		fr_window_archive_open (FR_WINDOW (new_window), window->priv->convert_data.new_file, GTK_WINDOW (new_window));
		close_progress_dialog (window, TRUE);
		break;
	case DIALOG_RESPONSE_OPEN_DESTINATION_FOLDER:
		fr_window_view_extraction_destination_folder (window);
		close_progress_dialog (window, TRUE);
		break;
	case DIALOG_RESPONSE_QUIT:
		fr_window_close (window);
		break;
	default:
		break;
	}
}


static const char*
get_message_from_action (FrAction action)
{
	char *message = "";

	switch (action) {
	case FR_ACTION_CREATING_NEW_ARCHIVE:
		message = _("Creating archive");
		break;
	case FR_ACTION_LOADING_ARCHIVE:
		message = _("Loading archive");
		break;
	case FR_ACTION_LISTING_CONTENT:
		message = _("Reading archive");
		break;
	case FR_ACTION_DELETING_FILES:
		message = _("Deleting files from archive");
		break;
	case FR_ACTION_TESTING_ARCHIVE:
		message = _("Testing archive");
		break;
	case FR_ACTION_GETTING_FILE_LIST:
		message = _("Getting the file list");
		break;
	case FR_ACTION_COPYING_FILES_FROM_REMOTE:
		message = _("Copying the file list");
		break;
	case FR_ACTION_ADDING_FILES:
		message = _("Adding files to archive");
		break;
	case FR_ACTION_EXTRACTING_FILES:
		message = _("Extracting files from archive");
		break;
	case FR_ACTION_COPYING_FILES_TO_REMOTE:
		message = _("Copying the file list");
		break;
	case FR_ACTION_CREATING_ARCHIVE:
		message = _("Creating archive");
		break;
	case FR_ACTION_SAVING_REMOTE_ARCHIVE:
		message = _("Saving archive");
		break;
	default:
		message = "";
		break;
	}

	return message;
}


static void
progress_dialog__set_last_action (FrWindow *window,
				  FrAction  action)
{
	const char *title;
	char       *markup;

	window->priv->pd_last_action = action;
	title = get_message_from_action (window->priv->pd_last_action);
	gtk_window_set_title (GTK_WINDOW (window->priv->progress_dialog), title);
	markup = g_markup_printf_escaped ("<span weight=\"bold\" size=\"larger\">%s</span>", title);
	gtk_label_set_markup (GTK_LABEL (window->priv->pd_action), markup);
	g_free (markup);
}


static void
pd_update_archive_name (FrWindow *window)
{
	char *current_archive;

	if (window->priv->convert_data.converting)
		current_archive = window->priv->convert_data.new_file;
	else if (window->priv->working_archive != NULL)
		current_archive = window->priv->working_archive;
	else
		current_archive = window->priv->archive_uri;

	if (strcmp_null_tolerant (window->priv->pd_last_archive, current_archive) != 0) {
		g_free (window->priv->pd_last_archive);
		if (current_archive == NULL) {
			window->priv->pd_last_archive = NULL;
			gtk_label_set_text (GTK_LABEL (window->priv->pd_archive), "");
#ifdef LOG_PROGRESS
			g_print ("archive name > (none)\n");
#endif
		}
		else {
			char *name;

			window->priv->pd_last_archive = g_strdup (current_archive);

			name = g_uri_display_basename (window->priv->pd_last_archive);
			gtk_label_set_text (GTK_LABEL (window->priv->pd_archive), name);
#ifdef LOG_PROGRESS
			g_print ("archive name > %s\n", name);
#endif
			g_free (name);
		}
	}
}


static gboolean
fr_window_working_archive_cb (FrCommand  *command,
			      const char *archive_filename,
			      FrWindow   *window)
{
	g_free (window->priv->working_archive);
	window->priv->working_archive = NULL;
	if (archive_filename != NULL)
		window->priv->working_archive = g_strdup (archive_filename);
	pd_update_archive_name (window);

	return TRUE;
}


static gboolean
fr_window_message_cb (FrCommand  *command,
		      const char *msg,
		      FrWindow   *window)
{
	if (window->priv->progress_dialog == NULL)
		return TRUE;

	if (msg != NULL) {
		while (*msg == ' ')
			msg++;
		if (*msg == 0)
			msg = NULL;
	}

	if (msg == NULL) {
		gtk_label_set_text (GTK_LABEL (window->priv->pd_message), "");
	}
	else {
		char *utf8_msg;

		if (! g_utf8_validate (msg, -1, NULL))
			utf8_msg = g_locale_to_utf8 (msg, -1 , 0, 0, 0);
		else
			utf8_msg = g_strdup (msg);
		if (utf8_msg == NULL)
			return TRUE;

		if (g_utf8_validate (utf8_msg, -1, NULL))
			gtk_label_set_text (GTK_LABEL (window->priv->pd_message), utf8_msg);
#ifdef LOG_PROGRESS
		g_print ("message > %s\n", utf8_msg);
#endif
		g_free (utf8_msg);
	}

	if (window->priv->convert_data.converting) {
		if (window->priv->pd_last_action != FR_ACTION_CREATING_ARCHIVE)
			progress_dialog__set_last_action (window, FR_ACTION_CREATING_ARCHIVE);
	}
	else if (window->priv->pd_last_action != window->priv->action)
		progress_dialog__set_last_action (window, window->priv->action);

	pd_update_archive_name (window);

	return TRUE;
}


static void
create_the_progress_dialog (FrWindow *window)
{
	GtkWindow     *parent;
	GtkDialog     *d;
	GtkWidget     *vbox;
	GtkWidget     *align;
	GtkWidget     *progress_vbox;
	GtkWidget     *lbl;
	const char    *title;
	char          *markup;
	PangoAttrList *attr_list;

	if (window->priv->progress_dialog != NULL)
		return;

	if (window->priv->batch_mode)
		parent = NULL;
	else
		parent = GTK_WINDOW (window);

	window->priv->pd_last_action = window->priv->action;
	title = get_message_from_action (window->priv->pd_last_action);
	window->priv->progress_dialog = gtk_dialog_new_with_buttons (title,
								     parent,
								     GTK_DIALOG_DESTROY_WITH_PARENT,
								     NULL);

	window->priv->pd_quit_button = gtk_dialog_add_button (GTK_DIALOG (window->priv->progress_dialog), GTK_STOCK_QUIT, DIALOG_RESPONSE_QUIT);
	window->priv->pd_open_archive_button = gtk_dialog_add_button (GTK_DIALOG (window->priv->progress_dialog), _("_Open the Archive"), DIALOG_RESPONSE_OPEN_ARCHIVE);
	window->priv->pd_open_destination_button = gtk_dialog_add_button (GTK_DIALOG (window->priv->progress_dialog), _("_Show the Files"), DIALOG_RESPONSE_OPEN_DESTINATION_FOLDER);
	window->priv->pd_close_button = gtk_dialog_add_button (GTK_DIALOG (window->priv->progress_dialog), GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE);
	window->priv->pd_cancel_button = gtk_dialog_add_button (GTK_DIALOG (window->priv->progress_dialog), GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);

	d = GTK_DIALOG (window->priv->progress_dialog);
	gtk_dialog_set_has_separator (d, FALSE);
	gtk_window_set_resizable (GTK_WINDOW (d), TRUE);
	gtk_dialog_set_default_response (GTK_DIALOG (d), GTK_RESPONSE_OK);
	gtk_window_set_default_size (GTK_WINDOW (d), PROGRESS_DIALOG_DEFAULT_WIDTH, -1);

	/* Main */

	vbox = gtk_vbox_new (FALSE, 5);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);
	gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (d)), vbox, FALSE, FALSE, 10);

	/* action label */

	lbl = window->priv->pd_action = gtk_label_new ("");

	markup = g_markup_printf_escaped ("<span weight=\"bold\" size=\"larger\">%s</span>", title);
	gtk_label_set_markup (GTK_LABEL (lbl), markup);
	g_free (markup);

	align = gtk_alignment_new (0.0, 0.0, 1.0, 1.0);
	gtk_alignment_set_padding (GTK_ALIGNMENT (align), 0, 12, 0, 0);

	gtk_misc_set_alignment (GTK_MISC (lbl), 0.0, 0.5);
	gtk_misc_set_padding (GTK_MISC (lbl), 0, 0);
	gtk_label_set_ellipsize (GTK_LABEL (lbl), PANGO_ELLIPSIZE_END);

	gtk_container_add (GTK_CONTAINER (align), lbl);
	gtk_box_pack_start (GTK_BOX (vbox), align, TRUE, TRUE, 0);

	/* archive name */

	g_free (window->priv->pd_last_archive);
	window->priv->pd_last_archive = NULL;
	if (window->priv->archive_uri != NULL) {
		GtkWidget *hbox;
		char      *name;

		hbox = gtk_hbox_new (FALSE, 6);
		gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);

		lbl = gtk_label_new ("");
		markup = g_markup_printf_escaped ("<b>%s</b>", _("Archive:"));
		gtk_label_set_markup (GTK_LABEL (lbl), markup);
		g_free (markup);
		gtk_box_pack_start (GTK_BOX (hbox), lbl, FALSE, FALSE, 0);

		window->priv->pd_last_archive = g_strdup (window->priv->archive_uri);
		name = g_uri_display_basename (window->priv->pd_last_archive);
		lbl = window->priv->pd_archive = gtk_label_new (name);
		g_free (name);

		gtk_misc_set_alignment (GTK_MISC (lbl), 0.0, 0.5);
		gtk_label_set_ellipsize (GTK_LABEL (lbl), PANGO_ELLIPSIZE_END);
		gtk_box_pack_start (GTK_BOX (hbox), lbl, TRUE, TRUE, 0);
	}

	/* progress and details */

	align = gtk_alignment_new (0.0, 0.0, 1.0, 1.0);
	gtk_alignment_set_padding (GTK_ALIGNMENT (align), 0, 6, 0, 0);

	progress_vbox = gtk_vbox_new (FALSE, 6);
	gtk_container_add (GTK_CONTAINER (align), progress_vbox);
	gtk_box_pack_start (GTK_BOX (vbox), align, TRUE, TRUE, 0);

	/* progress bar */

	window->priv->pd_progress_bar = gtk_progress_bar_new ();
	gtk_progress_bar_set_pulse_step (GTK_PROGRESS_BAR (window->priv->pd_progress_bar), ACTIVITY_PULSE_STEP);
	gtk_box_pack_start (GTK_BOX (progress_vbox), window->priv->pd_progress_bar, TRUE, TRUE, 0);

	/* details label */

	lbl = window->priv->pd_message = gtk_label_new ("");

	attr_list = pango_attr_list_new ();
	pango_attr_list_insert (attr_list, pango_attr_style_new (PANGO_STYLE_ITALIC));
	gtk_label_set_attributes (GTK_LABEL (lbl), attr_list);
	pango_attr_list_unref (attr_list);

	gtk_misc_set_alignment (GTK_MISC (lbl), 0.0, 0.5);
	gtk_label_set_ellipsize (GTK_LABEL (lbl), PANGO_ELLIPSIZE_END);
	gtk_box_pack_start (GTK_BOX (progress_vbox), lbl, TRUE, TRUE, 0);

	gtk_widget_show_all (vbox);

	/* signals */

	g_signal_connect (G_OBJECT (window->priv->progress_dialog),
			  "response",
			  G_CALLBACK (progress_dialog_response),
			  window);
	g_signal_connect (G_OBJECT (window->priv->progress_dialog),
			  "delete_event",
			  G_CALLBACK (progress_dialog_delete_event),
			  window);
}


static gboolean
display_progress_dialog (gpointer data)
{
	FrWindow *window = data;

	if (window->priv->progress_timeout != 0)
		g_source_remove (window->priv->progress_timeout);

	if (window->priv->progress_dialog != NULL) {
		gtk_dialog_set_response_sensitive (GTK_DIALOG (window->priv->progress_dialog),
						   GTK_RESPONSE_OK,
						   window->priv->stoppable);
		if (! window->priv->non_interactive)
			gtk_widget_show (GTK_WIDGET (window));
		gtk_widget_hide (window->priv->progress_bar);
		gtk_widget_show (window->priv->progress_dialog);
		fr_window_message_cb (NULL, window->priv->last_status_message, window);
	}

	window->priv->progress_timeout = 0;

	return FALSE;
}


static void
open_progress_dialog (FrWindow *window,
		      gboolean  open_now)
{
	if (window->priv->hide_progress_timeout != 0) {
		g_source_remove (window->priv->hide_progress_timeout);
		window->priv->hide_progress_timeout = 0;
	}

	if (open_now) {
		if (window->priv->progress_timeout != 0)
			g_source_remove (window->priv->progress_timeout);
		window->priv->progress_timeout = 0;
	}

	if ((window->priv->progress_timeout != 0)
	    || ((window->priv->progress_dialog != NULL) && gtk_widget_get_visible (window->priv->progress_dialog)))
		return;

	if (! window->priv->batch_mode && ! open_now)
		gtk_widget_show (window->priv->progress_bar);

	create_the_progress_dialog (window);
	gtk_widget_show (window->priv->pd_cancel_button);
	gtk_widget_hide (window->priv->pd_open_archive_button);
	gtk_widget_hide (window->priv->pd_open_destination_button);
	gtk_widget_hide (window->priv->pd_quit_button);
	gtk_widget_hide (window->priv->pd_close_button);

	if (open_now)
		display_progress_dialog (window);
	else
		window->priv->progress_timeout = g_timeout_add (PROGRESS_TIMEOUT_MSECS,
								display_progress_dialog,
								window);
}


static gboolean
fr_window_progress_cb (FrCommand  *command,
		       double      fraction,
		       FrWindow   *window)
{
	window->priv->progress_pulse = (fraction < 0.0);
	if (! window->priv->progress_pulse) {
		fraction = CLAMP (fraction, 0.0, 1.0);
		if (window->priv->progress_dialog != NULL)
			gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (window->priv->pd_progress_bar), fraction);
		gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (window->priv->progress_bar), fraction);
#ifdef LOG_PROGRESS
		g_print ("progress > %2.2f\n", fraction);
#endif
	}
	return TRUE;
}


static void
open_progress_dialog_with_open_destination (FrWindow *window)
{
	window->priv->ask_to_open_destination_after_extraction = FALSE;

	if (window->priv->hide_progress_timeout != 0) {
		g_source_remove (window->priv->hide_progress_timeout);
		window->priv->hide_progress_timeout = 0;
	}
	if (window->priv->progress_timeout != 0) {
		g_source_remove (window->priv->progress_timeout);
		window->priv->progress_timeout = 0;
	}

	create_the_progress_dialog (window);
	gtk_widget_hide (window->priv->pd_cancel_button);
	gtk_widget_hide (window->priv->pd_open_archive_button);
	gtk_widget_show (window->priv->pd_open_destination_button);
	gtk_widget_show (window->priv->pd_quit_button);
	gtk_widget_show (window->priv->pd_close_button);
	display_progress_dialog (window);
	fr_window_progress_cb (NULL, 1.0, window);
	fr_window_message_cb (NULL, _("Extraction completed successfully"), window);
}


static void
open_progress_dialog_with_open_archive (FrWindow *window)
{
	if (window->priv->hide_progress_timeout != 0) {
		g_source_remove (window->priv->hide_progress_timeout);
		window->priv->hide_progress_timeout = 0;
	}
	if (window->priv->progress_timeout != 0) {
		g_source_remove (window->priv->progress_timeout);
		window->priv->progress_timeout = 0;
	}

	create_the_progress_dialog (window);
	gtk_widget_hide (window->priv->pd_cancel_button);
	gtk_widget_hide (window->priv->pd_open_destination_button);
	gtk_widget_show (window->priv->pd_open_archive_button);
	gtk_widget_show (window->priv->pd_close_button);
	display_progress_dialog (window);
	fr_window_progress_cb (NULL, 1.0, window);
	fr_window_message_cb (NULL, _("Archive created successfully"), window);
}


void
fr_window_push_message (FrWindow   *window,
			const char *msg)
{
	if (! gtk_widget_get_mapped (GTK_WIDGET (window)))
		return;

	g_free (window->priv->last_status_message);
	window->priv->last_status_message = g_strdup (msg);

	gtk_statusbar_push (GTK_STATUSBAR (window->priv->statusbar), window->priv->progress_cid, window->priv->last_status_message);
	if (window->priv->progress_dialog != NULL)
		gtk_label_set_text (GTK_LABEL (window->priv->pd_message), window->priv->last_status_message);
}


void
fr_window_pop_message (FrWindow *window)
{
	if (! gtk_widget_get_mapped (GTK_WIDGET (window)))
		return;
	gtk_statusbar_pop (GTK_STATUSBAR (window->priv->statusbar), window->priv->progress_cid);
	if (window->priv->progress_dialog != NULL)
		gtk_label_set_text (GTK_LABEL (window->priv->pd_message), "");
}


static void
action_started (FrArchive *archive,
		FrAction   action,
		gpointer   data)
{
	FrWindow   *window = data;
	const char *message;
	char       *full_msg;

	window->priv->action = action;
	fr_window_start_activity_mode (window);

#ifdef DEBUG
	debug (DEBUG_INFO, "%s [START] (FR::Window)\n", action_names[action]);
#endif

	message = get_message_from_action (action);
	full_msg = g_strdup_printf ("%s, %s", message, _("please wait..."));
	fr_window_push_message (window, full_msg);

	switch (action) {
	case FR_ACTION_EXTRACTING_FILES:
		open_progress_dialog (window, window->priv->ask_to_open_destination_after_extraction || window->priv->convert_data.converting || window->priv->batch_mode);
		break;
	default:
		open_progress_dialog (window, window->priv->batch_mode);
		break;
	}

	if (archive->command != NULL) {
		fr_command_progress (archive->command, -1.0);
		fr_command_message (archive->command, message);
	}

	g_free (full_msg);
}


static void
fr_window_add_to_recent_list (FrWindow *window,
			      char     *uri)
{
	if (window->priv->batch_mode)
		return;

	if (is_temp_dir (uri))
		return;

	if (window->archive->content_type != NULL) {
		GtkRecentData *recent_data;

		recent_data = g_new0 (GtkRecentData, 1);
		recent_data->mime_type = g_content_type_get_mime_type (window->archive->content_type);
		recent_data->app_name = "File Roller";
		recent_data->app_exec = "file-roller";
		gtk_recent_manager_add_full (window->priv->recent_manager, uri, recent_data);

		g_free (recent_data);
	}
	else
		gtk_recent_manager_add_item (window->priv->recent_manager, uri);
}


static void
fr_window_remove_from_recent_list (FrWindow *window,
				   char     *filename)
{
	if (filename != NULL)
		gtk_recent_manager_remove_item (window->priv->recent_manager, filename, NULL);
}


static void
error_dialog_response_cb (GtkDialog *dialog,
			  gint       arg1,
			  gpointer   user_data)
{
	FrWindow  *window = user_data;
	GtkWindow *dialog_parent = window->priv->error_dialog_parent;

	window->priv->showing_error_dialog = FALSE;
	window->priv->error_dialog_parent = NULL;

	if ((dialog_parent != NULL) && (gtk_widget_get_toplevel (GTK_WIDGET (dialog_parent)) != (GtkWidget*) dialog_parent))
		gtk_window_set_modal (dialog_parent, TRUE);
	gtk_widget_destroy (GTK_WIDGET (dialog));
	if (window->priv->destroy_with_error_dialog)
		gtk_widget_destroy (GTK_WIDGET (window));
}


static void
fr_window_show_error_dialog (FrWindow  *window,
			     GtkWidget *dialog,
			     GtkWindow *dialog_parent)
{
	close_progress_dialog (window, TRUE);

	if (dialog_parent != NULL)
		gtk_window_set_modal (dialog_parent, FALSE);
	g_signal_connect (dialog,
			  "response",
			  (window->priv->batch_mode) ? G_CALLBACK (gtk_main_quit) : G_CALLBACK (error_dialog_response_cb),
			  window);
	gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);
	gtk_widget_show (dialog);

	window->priv->showing_error_dialog = TRUE;
	window->priv->error_dialog_parent = dialog_parent;
}


void
fr_window_destroy_with_error_dialog (FrWindow *window)
{
	window->priv->destroy_with_error_dialog = TRUE;
}


static gboolean
handle_errors (FrWindow    *window,
	       FrArchive   *archive,
	       FrAction     action,
	       FrProcError *error)
{
	if (error->type == FR_PROC_ERROR_ASK_PASSWORD) {
		close_progress_dialog (window, TRUE);
		dlg_ask_password (window);
		return FALSE;
	}
	else if (error->type == FR_PROC_ERROR_UNSUPPORTED_FORMAT) {
		close_progress_dialog (window, TRUE);
		dlg_package_installer (window, archive, action);
		return FALSE;
	}
#if 0
	else if (error->type == FR_PROC_ERROR_BAD_CHARSET) {
		close_progress_dialog (window, TRUE);
		/* dlg_ask_archive_charset (window); FIXME: implement after feature freeze */
		return FALSE;
	}
#endif
	else if (error->type == FR_PROC_ERROR_STOPPED) {
		/* nothing */
	}
	else if (error->type != FR_PROC_ERROR_NONE) {
		char      *msg = NULL;
		char      *utf8_name;
		char      *details = NULL;
		GtkWindow *dialog_parent;
		GtkWidget *dialog;
		FrProcess *process = archive->process;
		GList     *output = NULL;

		if (window->priv->batch_mode) {
			dialog_parent = NULL;
			window->priv->load_error_parent_window = NULL;
		}
		else {
			dialog_parent = (GtkWindow *) window;
			if (window->priv->load_error_parent_window == NULL)
				window->priv->load_error_parent_window = (GtkWindow *) window;
		}

		if ((action == FR_ACTION_LISTING_CONTENT) || (action == FR_ACTION_LOADING_ARCHIVE))
			fr_window_archive_close (window);

		switch (action) {
		case FR_ACTION_CREATING_NEW_ARCHIVE:
			dialog_parent = window->priv->load_error_parent_window;
			msg = _("Could not create the archive");
			break;

		case FR_ACTION_EXTRACTING_FILES:
		case FR_ACTION_COPYING_FILES_TO_REMOTE:
			msg = _("An error occurred while extracting files.");
			break;

		case FR_ACTION_LOADING_ARCHIVE:
			dialog_parent = window->priv->load_error_parent_window;
			utf8_name = g_uri_display_basename (window->priv->archive_uri);
			msg = g_strdup_printf (_("Could not open \"%s\""), utf8_name);
			g_free (utf8_name);
			break;

		case FR_ACTION_LISTING_CONTENT:
			msg = _("An error occurred while loading the archive.");
			break;

		case FR_ACTION_DELETING_FILES:
			msg = _("An error occurred while deleting files from the archive.");
			break;

		case FR_ACTION_ADDING_FILES:
		case FR_ACTION_GETTING_FILE_LIST:
		case FR_ACTION_COPYING_FILES_FROM_REMOTE:
			msg = _("An error occurred while adding files to the archive.");
			break;

		case FR_ACTION_TESTING_ARCHIVE:
			msg = _("An error occurred while testing archive.");
			break;

		case FR_ACTION_SAVING_REMOTE_ARCHIVE:
			msg = _("An error occurred while saving the archive.");
			break;

		default:
			msg = _("An error occurred.");
			break;
		}

		switch (error->type) {
		case FR_PROC_ERROR_COMMAND_NOT_FOUND:
			details = _("Command not found.");
			break;
		case FR_PROC_ERROR_EXITED_ABNORMALLY:
			details = _("Command exited abnormally.");
			break;
		case FR_PROC_ERROR_SPAWN:
			details = error->gerror->message;
			break;
		default:
			if (error->gerror != NULL)
				details = error->gerror->message;
			else
				details = NULL;
			break;
		}

		if (error->type != FR_PROC_ERROR_GENERIC)
			output = (process->err.raw != NULL) ? process->err.raw : process->out.raw;

		dialog = _gtk_error_dialog_new (dialog_parent,
						0,
						output,
						msg,
						((details != NULL) ? "%s" : NULL),
						details);
		fr_window_show_error_dialog (window, dialog, dialog_parent);

		return FALSE;
	}

	return TRUE;
}


static void
convert__action_performed (FrArchive   *archive,
			   FrAction     action,
			   FrProcError *error,
			   gpointer     data)
{
	FrWindow *window = data;

#ifdef DEBUG
	debug (DEBUG_INFO, "%s [CONVERT::DONE] (FR::Window)\n", action_names[action]);
#endif

	if ((action == FR_ACTION_GETTING_FILE_LIST) || (action == FR_ACTION_ADDING_FILES)) {
		fr_window_stop_activity_mode (window);
		fr_window_pop_message (window);
		close_progress_dialog (window, FALSE);
	}

	if (action != FR_ACTION_ADDING_FILES)
		return;

	handle_errors (window, archive, action, error);

	if (error->type == FR_PROC_ERROR_NONE)
		open_progress_dialog_with_open_archive (window);

	remove_local_directory (window->priv->convert_data.temp_dir);
	fr_window_convert_data_free (window, FALSE);

	fr_window_update_sensitivity (window);
	fr_window_update_statusbar_list_info (window);
}


static void fr_window_exec_next_batch_action (FrWindow *window);


static void
action_performed (FrArchive   *archive,
		  FrAction     action,
		  FrProcError *error,
		  gpointer     data)
{
	FrWindow *window = data;
	gboolean  continue_batch = FALSE;
	char     *archive_dir;
	gboolean  temp_dir;

#ifdef DEBUG
	debug (DEBUG_INFO, "%s [DONE] (FR::Window)\n", action_names[action]);
#endif

	fr_window_stop_activity_mode (window);
	fr_window_pop_message (window);

	continue_batch = handle_errors (window, archive, action, error);

	if ((error->type == FR_PROC_ERROR_ASK_PASSWORD)
	    || (error->type == FR_PROC_ERROR_UNSUPPORTED_FORMAT)
	    /*|| (error->type == FR_PROC_ERROR_BAD_CHARSET)*/)
	{
		return;
	}

	switch (action) {
	case FR_ACTION_CREATING_NEW_ARCHIVE:
	case FR_ACTION_CREATING_ARCHIVE:
		close_progress_dialog (window, FALSE);
		if (error->type != FR_PROC_ERROR_STOPPED) {
			fr_window_history_clear (window);
			fr_window_go_to_location (window, "/", TRUE);
			fr_window_update_dir_tree (window);
			fr_window_update_title (window);
			fr_window_update_sensitivity (window);
		}
		break;

	case FR_ACTION_LOADING_ARCHIVE:
		close_progress_dialog (window, FALSE);
		if (error->type != FR_PROC_ERROR_NONE) {
			fr_window_remove_from_recent_list (window, window->priv->archive_uri);
			if (window->priv->non_interactive) {
				fr_window_archive_close (window);
				fr_window_stop_batch (window);
			}
		}
		else {
			fr_window_add_to_recent_list (window, window->priv->archive_uri);
			if (! window->priv->non_interactive)
				gtk_window_present (GTK_WINDOW (window));
		}
		continue_batch = FALSE;
		g_signal_emit (window,
			       fr_window_signals[ARCHIVE_LOADED],
			       0,
			       error->type == FR_PROC_ERROR_NONE);
		break;

	case FR_ACTION_LISTING_CONTENT:
		/* update the uri because multi-volume archives can have
		 * a different name after loading. */
		g_free (window->priv->archive_uri);
		window->priv->archive_uri = g_file_get_uri (window->archive->file);

		close_progress_dialog (window, FALSE);
		if (error->type != FR_PROC_ERROR_NONE) {
			fr_window_remove_from_recent_list (window, window->priv->archive_uri);
			fr_window_archive_close (window);
			fr_window_set_password (window, NULL);
			break;
		}

		archive_dir = remove_level_from_path (window->priv->archive_uri);
		temp_dir = is_temp_dir (archive_dir);
		if (! window->priv->archive_present) {
			window->priv->archive_present = TRUE;

			fr_window_history_clear (window);
			fr_window_history_add (window, "/");

			if (! temp_dir) {
				fr_window_set_open_default_dir (window, archive_dir);
				fr_window_set_add_default_dir (window, archive_dir);
				if (! window->priv->freeze_default_dir)
					fr_window_set_extract_default_dir (window, archive_dir, FALSE);
			}

			window->priv->archive_new = FALSE;
		}
		g_free (archive_dir);

		if (! temp_dir)
			fr_window_add_to_recent_list (window, window->priv->archive_uri);

		fr_window_update_title (window);
		fr_window_go_to_location (window, fr_window_get_current_location (window), TRUE);
		fr_window_update_dir_tree (window);
		if (! window->priv->batch_mode && window->priv->non_interactive)
			gtk_window_present (GTK_WINDOW (window));
		break;

	case FR_ACTION_DELETING_FILES:
		close_progress_dialog (window, FALSE);
		fr_window_archive_reload (window);
		return;

	case FR_ACTION_ADDING_FILES:
		close_progress_dialog (window, FALSE);

		/* update the uri because multi-volume archives can have
		 * a different name after creation. */
		g_free (window->priv->archive_uri);
		window->priv->archive_uri = g_file_get_uri (window->archive->file);

		if (error->type == FR_PROC_ERROR_NONE) {
			if (window->priv->archive_new)
				window->priv->archive_new = FALSE;
			fr_window_add_to_recent_list (window, window->priv->archive_uri);
		}
		if (! window->priv->batch_mode) {
			fr_window_archive_reload (window);
			return;
		}
		break;

	case FR_ACTION_TESTING_ARCHIVE:
		close_progress_dialog (window, FALSE);
		if (error->type == FR_PROC_ERROR_NONE)
			fr_window_view_last_output (window, _("Test Result"));
		return;

	case FR_ACTION_EXTRACTING_FILES:
		if (error->type != FR_PROC_ERROR_NONE) {
			if (window->priv->convert_data.converting) {
				remove_local_directory (window->priv->convert_data.temp_dir);
				fr_window_convert_data_free (window, TRUE);
			}
			break;
		}
		if (window->priv->convert_data.converting) {
			char *source_dir;

			source_dir = g_filename_to_uri (window->priv->convert_data.temp_dir, NULL, NULL);
			fr_archive_add_with_wildcard (
				  window->priv->convert_data.new_archive,
				  "*",
				  NULL,
				  NULL,
				  source_dir,
				  NULL,
				  FALSE,
				  TRUE,
				  window->priv->convert_data.password,
				  window->priv->convert_data.encrypt_header,
				  window->priv->compression,
				  window->priv->convert_data.volume_size);
			g_free (source_dir);
		}
		else {
			if (window->priv->ask_to_open_destination_after_extraction)
				open_progress_dialog_with_open_destination (window);
			else
				close_progress_dialog (window, FALSE);
		}
		break;

	default:
		close_progress_dialog (window, FALSE);
		continue_batch = FALSE;
		break;
	}

	if (window->priv->batch_action == NULL) {
		fr_window_update_sensitivity (window);
		fr_window_update_statusbar_list_info (window);
	}

	if (continue_batch) {
		if (error->type != FR_PROC_ERROR_NONE)
			fr_window_stop_batch (window);
		else
			fr_window_exec_next_batch_action (window);
	}
}


/* -- selections -- */


static GList *
get_dir_list_from_path (FrWindow *window,
	      		char     *path)
{
	char  *dirname;
	int    dirname_l;
	GList *list = NULL;
	int    i;

	if (path[strlen (path) - 1] != '/')
		dirname = g_strconcat (path, "/", NULL);
	else
		dirname = g_strdup (path);
	dirname_l = strlen (dirname);
	for (i = 0; i < window->archive->command->files->len; i++) {
		FileData *fd = g_ptr_array_index (window->archive->command->files, i);

		if (strncmp (dirname, fd->full_path, dirname_l) == 0)
			list = g_list_prepend (list, g_strdup (fd->original_path));
	}
	g_free (dirname);

	return g_list_reverse (list);
}


static GList *
get_dir_list_from_file_data (FrWindow *window,
				   FileData *fdata)
{
	char  *dirname;
	GList *list;

	dirname = g_strconcat (fr_window_get_current_location (window),
			       fdata->list_name,
			       NULL);
	list = get_dir_list_from_path (window, dirname);
	g_free (dirname);

	return list;
}


GList *
fr_window_get_file_list_selection (FrWindow *window,
				   gboolean  recursive,
				   gboolean *has_dirs)
{
	GtkTreeSelection *selection;
	GList            *selections = NULL, *list, *scan;

	g_return_val_if_fail (window != NULL, NULL);

	if (has_dirs != NULL)
		*has_dirs = FALSE;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (window->priv->list_view));
	if (selection == NULL)
		return NULL;
	gtk_tree_selection_selected_foreach (selection, add_selected_from_list_view, &selections);

	list = NULL;
	for (scan = selections; scan; scan = scan->next) {
		FileData *fd = scan->data;

		if (!fd)
			continue;

		if (file_data_is_dir (fd)) {
			if (has_dirs != NULL)
				*has_dirs = TRUE;

			if (recursive)
				list = g_list_concat (list, get_dir_list_from_file_data (window, fd));
		}
		else
			list = g_list_prepend (list, g_strdup (fd->original_path));
	}
	if (selections)
		g_list_free (selections);

	return g_list_reverse (list);
}


GList *
fr_window_get_folder_tree_selection (FrWindow *window,
				     gboolean  recursive,
				     gboolean *has_dirs)
{
	GtkTreeSelection *tree_selection;
	GList            *selections, *list, *scan;

	g_return_val_if_fail (window != NULL, NULL);

	if (has_dirs != NULL)
		*has_dirs = FALSE;

	tree_selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (window->priv->tree_view));
	if (tree_selection == NULL)
		return NULL;

	selections = NULL;
	gtk_tree_selection_selected_foreach (tree_selection, add_selected_from_tree_view, &selections);
	if (selections == NULL)
		return NULL;

	if (has_dirs != NULL)
		*has_dirs = TRUE;

	list = NULL;
	for (scan = selections; scan; scan = scan->next) {
		char *path = scan->data;

		if (recursive)
			list = g_list_concat (list, get_dir_list_from_path (window, path));
	}
	path_list_free (selections);

	return g_list_reverse (list);
}


GList *
fr_window_get_file_list_from_path_list (FrWindow *window,
					GList    *path_list,
					gboolean *has_dirs)
{
	GtkTreeModel *model;
	GList        *selections, *list, *scan;

	g_return_val_if_fail (window != NULL, NULL);

	model = GTK_TREE_MODEL (window->priv->list_store);
	selections = NULL;

	if (has_dirs != NULL)
		*has_dirs = FALSE;

	for (scan = path_list; scan; scan = scan->next) {
		GtkTreeRowReference *reference = scan->data;
		GtkTreePath         *path;
		GtkTreeIter          iter;
		FileData            *fdata;

		path = gtk_tree_row_reference_get_path (reference);
		if (path == NULL)
			continue;

		if (! gtk_tree_model_get_iter (model, &iter, path))
			continue;

		gtk_tree_model_get (model, &iter,
				    COLUMN_FILE_DATA, &fdata,
				    -1);

		selections = g_list_prepend (selections, fdata);
	}

	list = NULL;
	for (scan = selections; scan; scan = scan->next) {
		FileData *fd = scan->data;

		if (!fd)
			continue;

		if (file_data_is_dir (fd)) {
			if (has_dirs != NULL)
				*has_dirs = TRUE;
			list = g_list_concat (list, get_dir_list_from_file_data (window, fd));
		}
		else
			list = g_list_prepend (list, g_strdup (fd->original_path));
	}

	if (selections != NULL)
		g_list_free (selections);

	return g_list_reverse (list);
}


GList *
fr_window_get_file_list_pattern (FrWindow    *window,
				 const char  *pattern)
{
	GRegex **regexps;
	GList   *list;
	int      i;

	g_return_val_if_fail (window != NULL, NULL);

	regexps = search_util_get_regexps (pattern, G_REGEX_CASELESS);
	list = NULL;
	for (i = 0; i < window->archive->command->files->len; i++) {
		FileData *fd = g_ptr_array_index (window->archive->command->files, i);
		char     *utf8_name;

		/* FIXME: only files in the current location ? */

		if (fd == NULL)
			continue;

		utf8_name = g_filename_to_utf8 (fd->name, -1, NULL, NULL, NULL);
		if (match_regexps (regexps, utf8_name, 0))
			list = g_list_prepend (list, g_strdup (fd->original_path));
		g_free (utf8_name);
	}
	free_regexps (regexps);

	return g_list_reverse (list);
}


int
fr_window_get_n_selected_files (FrWindow *window)
{
	return _gtk_count_selected (gtk_tree_view_get_selection (GTK_TREE_VIEW (window->priv->list_view)));
}


/**/


static int
dir_tree_button_press_cb (GtkWidget      *widget,
			  GdkEventButton *event,
			  gpointer        data)
{
	FrWindow         *window = data;
	GtkTreeSelection *selection;

	if (event->window != gtk_tree_view_get_bin_window (GTK_TREE_VIEW (window->priv->tree_view)))
		return FALSE;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (window->priv->tree_view));
	if (selection == NULL)
		return FALSE;

	if ((event->type == GDK_BUTTON_PRESS) && (event->button == 3)) {
		GtkTreePath *path;
		GtkTreeIter  iter;

		if (gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (window->priv->tree_view),
						   event->x, event->y,
						   &path, NULL, NULL, NULL)) {

			if (! gtk_tree_model_get_iter (GTK_TREE_MODEL (window->priv->tree_store), &iter, path)) {
				gtk_tree_path_free (path);
				return FALSE;
			}
			gtk_tree_path_free (path);

			if (! gtk_tree_selection_iter_is_selected (selection, &iter)) {
				gtk_tree_selection_unselect_all (selection);
				gtk_tree_selection_select_iter (selection, &iter);
			}

			gtk_menu_popup (GTK_MENU (window->priv->sidebar_folder_popup_menu),
					NULL, NULL, NULL,
					window,
					event->button,
					event->time);
		}
		else
			gtk_tree_selection_unselect_all (selection);

		return TRUE;
	}
	else if ((event->type == GDK_BUTTON_PRESS) && (event->button == 8)) {
		fr_window_go_back (window);
		return TRUE;
	}
	else if ((event->type == GDK_BUTTON_PRESS) && (event->button == 9)) {
		fr_window_go_forward (window);
		return TRUE;
	}

	return FALSE;
}


static FileData *
fr_window_get_selected_item_from_file_list (FrWindow *window)
{
	GtkTreeSelection *tree_selection;
	GList            *selection;
	FileData         *fdata = NULL;

	g_return_val_if_fail (window != NULL, NULL);

	tree_selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (window->priv->list_view));
	if (tree_selection == NULL)
		return NULL;

	selection = NULL;
	gtk_tree_selection_selected_foreach (tree_selection, add_selected_from_list_view, &selection);
	if ((selection == NULL) || (selection->next != NULL)) {
		/* return NULL if the selection contains more than one entry. */
		g_list_free (selection);
		return NULL;
	}

	fdata = file_data_copy (selection->data);
	g_list_free (selection);

	return fdata;
}


static char *
fr_window_get_selected_folder_in_tree_view (FrWindow *window)
{
	GtkTreeSelection *tree_selection;
	GList            *selections;
	char             *path = NULL;

	g_return_val_if_fail (window != NULL, NULL);

	tree_selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (window->priv->tree_view));
	if (tree_selection == NULL)
		return NULL;

	selections = NULL;
	gtk_tree_selection_selected_foreach (tree_selection, add_selected_from_tree_view, &selections);

	if (selections != NULL) {
		path = selections->data;
		g_list_free (selections);
	}

	return path;
}


void
fr_window_current_folder_activated (FrWindow *window,
				   gboolean   from_sidebar)
{
	char *dir_path;

	if (! from_sidebar) {
		FileData *fdata;
		char     *dir_name;

		fdata = fr_window_get_selected_item_from_file_list (window);
		if ((fdata == NULL) || ! file_data_is_dir (fdata)) {
			file_data_free (fdata);
			return;
		}
		dir_name = g_strdup (fdata->list_name);
		dir_path = g_strconcat (fr_window_get_current_location (window),
					dir_name,
					"/",
					NULL);
		g_free (dir_name);
		file_data_free (fdata);
	}
	else
		dir_path = fr_window_get_selected_folder_in_tree_view (window);

	fr_window_go_to_location (window, dir_path, FALSE);

	g_free (dir_path);
}


static gboolean
row_activated_cb (GtkTreeView       *tree_view,
		  GtkTreePath       *path,
		  GtkTreeViewColumn *column,
		  gpointer           data)
{
	FrWindow    *window = data;
	FileData    *fdata;
	GtkTreeIter  iter;

	if (! gtk_tree_model_get_iter (GTK_TREE_MODEL (window->priv->list_store),
				       &iter,
				       path))
		return FALSE;

	gtk_tree_model_get (GTK_TREE_MODEL (window->priv->list_store), &iter,
			    COLUMN_FILE_DATA, &fdata,
			    -1);

	if (! file_data_is_dir (fdata)) {
		GList *list = g_list_prepend (NULL, fdata->original_path);
		fr_window_open_files (window, list, FALSE);
		g_list_free (list);
	}
	else if (window->priv->list_mode == FR_WINDOW_LIST_MODE_AS_DIR) {
		char *new_dir;
		new_dir = g_strconcat (fr_window_get_current_location (window),
				       fdata->list_name,
				       "/",
				       NULL);
		fr_window_go_to_location (window, new_dir, FALSE);
		g_free (new_dir);
	}

	return FALSE;
}


static int
file_button_press_cb (GtkWidget      *widget,
		      GdkEventButton *event,
		      gpointer        data)
{
	FrWindow         *window = data;
	GtkTreeSelection *selection;

	if (event->window != gtk_tree_view_get_bin_window (GTK_TREE_VIEW (window->priv->list_view)))
		return FALSE;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (window->priv->list_view));
	if (selection == NULL)
		return FALSE;

	if (window->priv->path_clicked != NULL) {
		gtk_tree_path_free (window->priv->path_clicked);
		window->priv->path_clicked = NULL;
	}

	if ((event->type == GDK_BUTTON_PRESS) && (event->button == 3)) {
		GtkTreePath *path;
		GtkTreeIter  iter;
		int          n_selected;

		if (gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (window->priv->list_view),
						   event->x, event->y,
						   &path, NULL, NULL, NULL)) {

			if (! gtk_tree_model_get_iter (GTK_TREE_MODEL (window->priv->list_store), &iter, path)) {
				gtk_tree_path_free (path);
				return FALSE;
			}
			gtk_tree_path_free (path);

			if (! gtk_tree_selection_iter_is_selected (selection, &iter)) {
				gtk_tree_selection_unselect_all (selection);
				gtk_tree_selection_select_iter (selection, &iter);
			}
		}
		else
			gtk_tree_selection_unselect_all (selection);

		n_selected = fr_window_get_n_selected_files (window);
		if ((n_selected == 1) && selection_has_a_dir (window))
			gtk_menu_popup (GTK_MENU (window->priv->folder_popup_menu),
					NULL, NULL, NULL,
					window,
					event->button,
					event->time);
		else
			gtk_menu_popup (GTK_MENU (window->priv->file_popup_menu),
					NULL, NULL, NULL,
					window,
					event->button,
					event->time);
		return TRUE;
	}
	else if ((event->type == GDK_BUTTON_PRESS) && (event->button == 1)) {
		GtkTreePath *path = NULL;

		if (! gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (window->priv->list_view),
						     event->x, event->y,
						     &path, NULL, NULL, NULL)) {
			gtk_tree_selection_unselect_all (selection);
		}

		if (window->priv->path_clicked != NULL) {
			gtk_tree_path_free (window->priv->path_clicked);
			window->priv->path_clicked = NULL;
		}

		if (path != NULL) {
			window->priv->path_clicked = gtk_tree_path_copy (path);
			gtk_tree_path_free (path);
		}

		return FALSE;
	}
	else if ((event->type == GDK_BUTTON_PRESS) && (event->button == 8)) {
		// go back
		fr_window_go_back (window);
		return TRUE;
	}
	else if ((event->type == GDK_BUTTON_PRESS) && (event->button == 9)) {
		// go forward
		fr_window_go_forward (window);
		return TRUE;
	}

	return FALSE;
}


static int
file_button_release_cb (GtkWidget      *widget,
			GdkEventButton *event,
			gpointer        data)
{
	FrWindow         *window = data;
	GtkTreeSelection *selection;

	if (event->window != gtk_tree_view_get_bin_window (GTK_TREE_VIEW (window->priv->list_view)))
		return FALSE;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (window->priv->list_view));
	if (selection == NULL)
		return FALSE;

	if (window->priv->path_clicked == NULL)
		return FALSE;

	if ((event->type == GDK_BUTTON_RELEASE)
	    && (event->button == 1)
	    && (window->priv->path_clicked != NULL)) {
		GtkTreePath *path = NULL;

		if (gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (window->priv->list_view),
						   event->x, event->y,
						   &path, NULL, NULL, NULL)) {

			if ((gtk_tree_path_compare (window->priv->path_clicked, path) == 0)
			    && window->priv->single_click
			    && ! ((event->state & GDK_CONTROL_MASK) || (event->state & GDK_SHIFT_MASK))) {
				gtk_tree_view_set_cursor (GTK_TREE_VIEW (widget),
							  path,
							  NULL,
							  FALSE);
				gtk_tree_view_row_activated (GTK_TREE_VIEW (widget),
							     path,
							     NULL);
			}
		}

		if (path != NULL)
			gtk_tree_path_free (path);
	}

	if (window->priv->path_clicked != NULL) {
		gtk_tree_path_free (window->priv->path_clicked);
		window->priv->path_clicked = NULL;
	}

	return FALSE;
}


static gboolean
file_motion_notify_callback (GtkWidget *widget,
			     GdkEventMotion *event,
			     gpointer user_data)
{
	FrWindow    *window = user_data;
	GdkCursor   *cursor;
	GtkTreePath *last_hover_path;
	GtkTreeIter  iter;

	if (! window->priv->single_click)
		return FALSE;

	if (event->window != gtk_tree_view_get_bin_window (GTK_TREE_VIEW (window->priv->list_view)))
		return FALSE;

	last_hover_path = window->priv->list_hover_path;

	gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (widget),
				       event->x, event->y,
				       &window->priv->list_hover_path,
				       NULL, NULL, NULL);

	if (window->priv->list_hover_path != NULL)
		cursor = gdk_cursor_new (GDK_HAND2);
	else
		cursor = NULL;

	gdk_window_set_cursor (event->window, cursor);

	/* only redraw if the hover row has changed */
	if (!(last_hover_path == NULL && window->priv->list_hover_path == NULL) &&
	    (!(last_hover_path != NULL && window->priv->list_hover_path != NULL) ||
	     gtk_tree_path_compare (last_hover_path, window->priv->list_hover_path)))
	{
		if (last_hover_path) {
			gtk_tree_model_get_iter (GTK_TREE_MODEL (window->priv->list_store),
						 &iter, last_hover_path);
			gtk_tree_model_row_changed (GTK_TREE_MODEL (window->priv->list_store),
						    last_hover_path, &iter);
		}

		if (window->priv->list_hover_path) {
			gtk_tree_model_get_iter (GTK_TREE_MODEL (window->priv->list_store),
						 &iter, window->priv->list_hover_path);
			gtk_tree_model_row_changed (GTK_TREE_MODEL (window->priv->list_store),
						    window->priv->list_hover_path, &iter);
		}
	}

	gtk_tree_path_free (last_hover_path);

 	return FALSE;
}


static gboolean
file_leave_notify_callback (GtkWidget *widget,
			    GdkEventCrossing *event,
			    gpointer user_data)
{
	FrWindow    *window = user_data;
	GtkTreeIter  iter;

	if (window->priv->single_click && (window->priv->list_hover_path != NULL)) {
		gtk_tree_model_get_iter (GTK_TREE_MODEL (window->priv->list_store),
					 &iter,
					 window->priv->list_hover_path);
		gtk_tree_model_row_changed (GTK_TREE_MODEL (window->priv->list_store),
					    window->priv->list_hover_path,
					    &iter);

		gtk_tree_path_free (window->priv->list_hover_path);
		window->priv->list_hover_path = NULL;
	}

	return FALSE;
}


/* -- drag and drop -- */


static GList *
get_uri_list_from_selection_data (char *uri_list)
{
	GList  *list = NULL;
	char  **uris;
	int     i;

	if (uri_list == NULL)
		return NULL;

	uris = g_uri_list_extract_uris (uri_list);
	for (i = 0; uris[i] != NULL; i++)
		list = g_list_prepend (list, g_strdup (uris[i]));
	g_strfreev (uris);

	return g_list_reverse (list);
}


static gboolean
fr_window_drag_motion (GtkWidget      *widget,
		       GdkDragContext *context,
		       gint            x,
		       gint            y,
		       guint           time,
		       gpointer        user_data)
{
	FrWindow  *window = user_data;

	if ((gtk_drag_get_source_widget (context) == window->priv->list_view)
	    || (gtk_drag_get_source_widget (context) == window->priv->tree_view))
	{
		gdk_drag_status (context, 0, time);
		return FALSE;
	}

	return TRUE;
}


static void fr_window_paste_from_clipboard_data (FrWindow *window, FrClipboardData *data);


static FrClipboardData*
get_clipboard_data_from_selection_data (FrWindow   *window,
					const char *data)
{
	FrClipboardData  *clipboard_data;
	char            **uris;
	int               i;

	clipboard_data = fr_clipboard_data_new ();

	uris = g_strsplit (data, "\r\n", -1);

	clipboard_data->archive_filename = g_strdup (uris[0]);
	if (window->priv->password_for_paste != NULL)
		clipboard_data->archive_password = g_strdup (window->priv->password_for_paste);
	else if (strcmp (uris[1], "") != 0)
		clipboard_data->archive_password = g_strdup (uris[1]);
	clipboard_data->op = (strcmp (uris[2], "copy") == 0) ? FR_CLIPBOARD_OP_COPY : FR_CLIPBOARD_OP_CUT;
	clipboard_data->base_dir = g_strdup (uris[3]);
	for (i = 4; uris[i] != NULL; i++)
		if (uris[i][0] != '\0')
			clipboard_data->files = g_list_prepend (clipboard_data->files, g_strdup (uris[i]));
	clipboard_data->files = g_list_reverse (clipboard_data->files);

	g_strfreev (uris);

	return clipboard_data;
}


static void
fr_window_drag_data_received  (GtkWidget          *widget,
			       GdkDragContext     *context,
			       gint                x,
			       gint                y,
			       GtkSelectionData   *data,
			       guint               info,
			       guint               time,
			       gpointer            extra_data)
{
	FrWindow  *window = extra_data;
	GList     *list;
	gboolean   one_file;
	gboolean   is_an_archive;

	debug (DEBUG_INFO, "::DragDataReceived -->\n");

	if ((gtk_drag_get_source_widget (context) == window->priv->list_view)
	    || (gtk_drag_get_source_widget (context) == window->priv->tree_view))
	{
		gtk_drag_finish (context, FALSE, FALSE, time);
		return;
	}

	if (! ((gtk_selection_data_get_length (data) >= 0) && (gtk_selection_data_get_format (data) == 8))) {
		gtk_drag_finish (context, FALSE, FALSE, time);
		return;
	}

	if (window->priv->activity_ref > 0) {
		gtk_drag_finish (context, FALSE, FALSE, time);
		return;
	}

	gtk_drag_finish (context, TRUE, FALSE, time);

	if (gtk_selection_data_get_target (data) == XFR_ATOM) {
		FrClipboardData *dnd_data;

		dnd_data = get_clipboard_data_from_selection_data (window, (char*) gtk_selection_data_get_data (data));
		dnd_data->current_dir = g_strdup (fr_window_get_current_location (window));
		fr_window_paste_from_clipboard_data (window, dnd_data);

		return;
	}

	list = get_uri_list_from_selection_data ((char*) gtk_selection_data_get_data (data));
	if (list == NULL) {
		GtkWidget *d;

		d = _gtk_error_dialog_new (GTK_WINDOW (window),
					   GTK_DIALOG_MODAL,
					   NULL,
					   _("Could not perform the operation"),
					   NULL);
		gtk_dialog_run (GTK_DIALOG (d));
		gtk_widget_destroy(d);

 		return;
	}

	one_file = (list->next == NULL);
	if (one_file)
		is_an_archive = uri_is_archive (list->data);
	else
		is_an_archive = FALSE;

	if (window->priv->archive_present
	    && (window->archive != NULL)
	    && ! window->archive->read_only
	    && ! window->archive->is_compressed_file)
	{
		if (one_file && is_an_archive) {
			GtkWidget *d;
			gint       r;

			d = _gtk_message_dialog_new (GTK_WINDOW (window),
						     GTK_DIALOG_MODAL,
						     GTK_STOCK_DIALOG_QUESTION,
						     _("Do you want to add this file to the current archive or open it as a new archive?"),
						     NULL,
						     GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
						     GTK_STOCK_ADD, 0,
						     GTK_STOCK_OPEN, 1,
						     NULL);

			gtk_dialog_set_default_response (GTK_DIALOG (d), 2);

			r = gtk_dialog_run (GTK_DIALOG (d));
			gtk_widget_destroy (GTK_WIDGET (d));

			if (r == 0)  /* Add */
				fr_window_archive_add_dropped_items (window, list, FALSE);
			else if (r == 1)  /* Open */
				fr_window_archive_open (window, list->data, GTK_WINDOW (window));
 		}
 		else
			fr_window_archive_add_dropped_items (window, list, FALSE);
	}
	else {
		if (one_file && is_an_archive)
			fr_window_archive_open (window, list->data, GTK_WINDOW (window));
		else {
			GtkWidget *d;
			int        r;

			d = _gtk_message_dialog_new (GTK_WINDOW (window),
						     GTK_DIALOG_MODAL,
						     GTK_STOCK_DIALOG_QUESTION,
						     _("Do you want to create a new archive with these files?"),
						     NULL,
						     GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
						     _("Create _Archive"), GTK_RESPONSE_YES,
						     NULL);

			gtk_dialog_set_default_response (GTK_DIALOG (d), GTK_RESPONSE_YES);
			r = gtk_dialog_run (GTK_DIALOG (d));
			gtk_widget_destroy (GTK_WIDGET (d));

			if (r == GTK_RESPONSE_YES) {
				char       *first_item;
				char       *folder;
				char       *local_path = NULL;
				char       *utf8_path = NULL;
				const char *archive_name;

				fr_window_free_batch_data (window);
				fr_window_append_batch_action (window,
							       FR_BATCH_ACTION_ADD,
							       path_list_dup (list),
							       (GFreeFunc) path_list_free);

				first_item = (char*) list->data;
				folder = remove_level_from_path (first_item);
				if (folder != NULL)
					fr_window_set_open_default_dir (window, folder);

				if ((list->next != NULL) && (folder != NULL)) {
					archive_name = file_name_from_path (folder);
				}
				else {
					if (uri_is_local (first_item)) {
						local_path = g_filename_from_uri (first_item, NULL, NULL);
						if (local_path)
							utf8_path = g_filename_to_utf8 (local_path, -1, NULL, NULL, NULL);
						if (!utf8_path)
							utf8_path= g_strdup (first_item);
						g_free (local_path);
					}
					else {
						utf8_path = g_strdup (first_item);
					}
					archive_name = file_name_from_path (utf8_path);
				}

				show_new_archive_dialog (window, archive_name);
				g_free (utf8_path);

				g_free (folder);
			}
		}
	}

	path_list_free (list);

	debug (DEBUG_INFO, "::DragDataReceived <--\n");
}


static gboolean
file_list_drag_begin (GtkWidget          *widget,
		      GdkDragContext     *context,
		      gpointer            data)
{
	FrWindow *window = data;

	debug (DEBUG_INFO, "::DragBegin -->\n");

	if (window->priv->activity_ref > 0)
		return FALSE;

	g_free (window->priv->drag_destination_folder);
	window->priv->drag_destination_folder = NULL;

	g_free (window->priv->drag_base_dir);
	window->priv->drag_base_dir = NULL;

	gdk_property_change (gdk_drag_context_get_source_window (context),
			     XDS_ATOM, TEXT_ATOM,
			     8, GDK_PROP_MODE_REPLACE,
			     (guchar *) XDS_FILENAME,
			     strlen (XDS_FILENAME));

	return TRUE;
}


static void
file_list_drag_end (GtkWidget      *widget,
		    GdkDragContext *context,
		    gpointer        data)
{
	FrWindow *window = data;

	debug (DEBUG_INFO, "::DragEnd -->\n");

	gdk_property_delete (gdk_drag_context_get_source_window (context), XDS_ATOM);

	if (window->priv->drag_error != NULL) {
		_gtk_error_dialog_run (GTK_WINDOW (window),
				       _("Extraction not performed"),
				       "%s",
				       window->priv->drag_error->message);
		g_clear_error (&window->priv->drag_error);
	}
	else if (window->priv->drag_destination_folder != NULL) {
		fr_window_archive_extract (window,
					   window->priv->drag_file_list,
					   window->priv->drag_destination_folder,
					   window->priv->drag_base_dir,
					   FALSE,
					   TRUE,
					   FALSE,
					   FALSE);
		path_list_free (window->priv->drag_file_list);
		window->priv->drag_file_list = NULL;
	}

	debug (DEBUG_INFO, "::DragEnd <--\n");
}


/* The following three functions taken from bugzilla
 * (http://bugzilla.mate.org/attachment.cgi?id=49362&action=view)
 * Author: Christian Neumair
 * Copyright: 2005 Free Software Foundation, Inc
 * License: GPL */
static char *
get_xds_atom_value (GdkDragContext *context)
{
	char *ret;

	g_return_val_if_fail (context != NULL, NULL);
	g_return_val_if_fail (gdk_drag_context_get_source_window (context) != NULL, NULL);

	if (gdk_property_get (gdk_drag_context_get_source_window (context),
			      XDS_ATOM, TEXT_ATOM,
			      0, MAX_XDS_ATOM_VAL_LEN,
			      FALSE, NULL, NULL, NULL,
			      (unsigned char **) &ret))
		return ret;

	return NULL;
}


static gboolean
context_offers_target (GdkDragContext *context,
		       GdkAtom target)
{
	return (g_list_find (gdk_drag_context_list_targets (context), target) != NULL);
}


static gboolean
caja_xds_dnd_is_valid_xds_context (GdkDragContext *context)
{
	char *tmp;
	gboolean ret;

	g_return_val_if_fail (context != NULL, FALSE);

	tmp = NULL;
	if (context_offers_target (context, XDS_ATOM)) {
		tmp = get_xds_atom_value (context);
	}

	ret = (tmp != NULL);
	g_free (tmp);

	return ret;
}


static char *
get_selection_data_from_clipboard_data (FrWindow        *window,
		      			FrClipboardData *data)
{
	GString *list;
	char    *local_filename;
	GList   *scan;

	list = g_string_new (NULL);

	local_filename = g_file_get_uri (window->archive->local_copy);
	g_string_append (list, local_filename);
	g_free (local_filename);

	g_string_append (list, "\r\n");
	if (window->priv->password != NULL)
		g_string_append (list, window->priv->password);
	g_string_append (list, "\r\n");
	g_string_append (list, (data->op == FR_CLIPBOARD_OP_COPY) ? "copy" : "cut");
	g_string_append (list, "\r\n");
	g_string_append (list, data->base_dir);
	g_string_append (list, "\r\n");
	for (scan = data->files; scan; scan = scan->next) {
		g_string_append (list, scan->data);
		g_string_append (list, "\r\n");
	}

	return g_string_free (list, FALSE);
}


static gboolean
fr_window_folder_tree_drag_data_get (GtkWidget        *widget,
				     GdkDragContext   *context,
				     GtkSelectionData *selection_data,
				     guint             info,
				     guint             time,
				     gpointer          user_data)
{
	FrWindow *window = user_data;
	GList    *file_list;
	char     *destination;
	char     *destination_folder;

	debug (DEBUG_INFO, "::DragDataGet -->\n");

	if (window->priv->activity_ref > 0)
		return FALSE;

	file_list = fr_window_get_folder_tree_selection (window, TRUE, NULL);
	if (file_list == NULL)
		return FALSE;

	if (gtk_selection_data_get_target (selection_data) == XFR_ATOM) {
		FrClipboardData *tmp;
		char            *data;

		tmp = fr_clipboard_data_new ();
		tmp->files = file_list;
		tmp->op = FR_CLIPBOARD_OP_COPY;
		tmp->base_dir = g_strdup (fr_window_get_current_location (window));

		data = get_selection_data_from_clipboard_data (window, tmp);
		gtk_selection_data_set (selection_data, XFR_ATOM, 8, (guchar *) data, strlen (data));

		fr_clipboard_data_unref (tmp);
		g_free (data);

		return TRUE;
	}

	if (! caja_xds_dnd_is_valid_xds_context (context))
		return FALSE;

	destination = get_xds_atom_value (context);
	g_return_val_if_fail (destination != NULL, FALSE);

	destination_folder = remove_level_from_path (destination);
	g_free (destination);

	/* check whether the extraction can be performed in the destination
	 * folder */

	g_clear_error (&window->priv->drag_error);

	if (! check_permissions (destination_folder, R_OK | W_OK)) {
		char *destination_folder_display_name;

		destination_folder_display_name = g_filename_display_name (destination_folder);
		window->priv->drag_error = g_error_new (FR_ERROR, 0, _("You don't have the right permissions to extract archives in the folder \"%s\""), destination_folder_display_name);
		g_free (destination_folder_display_name);
	}

	if (window->priv->drag_error == NULL) {
		g_free (window->priv->drag_destination_folder);
		g_free (window->priv->drag_base_dir);
		path_list_free (window->priv->drag_file_list);
		window->priv->drag_destination_folder = g_strdup (destination_folder);
		window->priv->drag_base_dir = fr_window_get_selected_folder_in_tree_view (window);
		window->priv->drag_file_list = file_list;
	}

	g_free (destination_folder);

	/* sends back the response */

	gtk_selection_data_set (selection_data, gtk_selection_data_get_target (selection_data), 8, (guchar *) ((window->priv->drag_error == NULL) ? "S" : "E"), 1);

	debug (DEBUG_INFO, "::DragDataGet <--\n");

	return TRUE;
}


gboolean
fr_window_file_list_drag_data_get (FrWindow         *window,
				   GdkDragContext   *context,
				   GtkSelectionData *selection_data,
				   GList            *path_list)
{
	char *destination;
	char *destination_folder;

	debug (DEBUG_INFO, "::DragDataGet -->\n");

	if (window->priv->path_clicked != NULL) {
		gtk_tree_path_free (window->priv->path_clicked);
		window->priv->path_clicked = NULL;
	}

	if (window->priv->activity_ref > 0)
		return FALSE;

	if (gtk_selection_data_get_target (selection_data) == XFR_ATOM) {
		FrClipboardData *tmp;
		char            *data;

		tmp = fr_clipboard_data_new ();
		tmp->files = fr_window_get_file_list_selection (window, TRUE, NULL);
		tmp->op = FR_CLIPBOARD_OP_COPY;
		tmp->base_dir = g_strdup (fr_window_get_current_location (window));

		data = get_selection_data_from_clipboard_data (window, tmp);
		gtk_selection_data_set (selection_data, XFR_ATOM, 8, (guchar *) data, strlen (data));

		fr_clipboard_data_unref (tmp);
		g_free (data);

		return TRUE;
	}

	if (! caja_xds_dnd_is_valid_xds_context (context))
		return FALSE;

	destination = get_xds_atom_value (context);
	g_return_val_if_fail (destination != NULL, FALSE);

	destination_folder = remove_level_from_path (destination);
	g_free (destination);

	/* check whether the extraction can be performed in the destination
	 * folder */

	g_clear_error (&window->priv->drag_error);

	if (! check_permissions (destination_folder, R_OK | W_OK)) {
		char *destination_folder_display_name;

		destination_folder_display_name = g_filename_display_name (destination_folder);
		window->priv->drag_error = g_error_new (FR_ERROR, 0, _("You don't have the right permissions to extract archives in the folder \"%s\""), destination_folder_display_name);
		g_free (destination_folder_display_name);
	}

	if (window->priv->drag_error == NULL) {
		g_free (window->priv->drag_destination_folder);
		g_free (window->priv->drag_base_dir);
		path_list_free (window->priv->drag_file_list);
		window->priv->drag_destination_folder = g_strdup (destination_folder);
		window->priv->drag_base_dir = g_strdup (fr_window_get_current_location (window));
		window->priv->drag_file_list = fr_window_get_file_list_from_path_list (window, path_list, NULL);
	}

	g_free (destination_folder);

	/* sends back the response */

	gtk_selection_data_set (selection_data, gtk_selection_data_get_target (selection_data), 8, (guchar *) ((window->priv->drag_error == NULL) ? "S" : "E"), 1);

	debug (DEBUG_INFO, "::DragDataGet <--\n");

	return TRUE;
}


/* -- window_new -- */


static void
fr_window_deactivate_filter (FrWindow *window)
{
	window->priv->filter_mode = FALSE;
	window->priv->list_mode = window->priv->last_list_mode;

	gtk_entry_set_text (GTK_ENTRY (window->priv->filter_entry), "");
	fr_window_update_filter_bar_visibility (window);

	gtk_list_store_clear (window->priv->list_store);

	fr_window_update_columns_visibility (window);
	fr_window_update_file_list (window, TRUE);
	fr_window_update_dir_tree (window);
	fr_window_update_current_location (window);
}


static gboolean
key_press_cb (GtkWidget   *widget,
	      GdkEventKey *event,
	      gpointer     data)
{
	FrWindow *window = data;
	gboolean  retval = FALSE;
	gboolean  alt;

	if (gtk_widget_has_focus (window->priv->location_entry))
		return FALSE;

	if (gtk_widget_has_focus (window->priv->filter_entry)) {
		switch (event->keyval) {
		case GDK_Escape:
			fr_window_deactivate_filter (window);
			retval = TRUE;
			break;
		default:
			break;
		}
		return retval;
	}

	alt = (event->state & GDK_MOD1_MASK) == GDK_MOD1_MASK;

	switch (event->keyval) {
	case GDK_Escape:
		activate_action_stop (NULL, window);
		if (window->priv->filter_mode)
			fr_window_deactivate_filter (window);
		retval = TRUE;
		break;

	case GDK_F10:
		if (event->state & GDK_SHIFT_MASK) {
			GtkTreeSelection *selection;

			selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (window->priv->list_view));
			if (selection == NULL)
				return FALSE;

			gtk_menu_popup (GTK_MENU (window->priv->file_popup_menu),
					NULL, NULL, NULL,
					window,
					3,
					GDK_CURRENT_TIME);
			retval = TRUE;
		}
		break;

	case GDK_Up:
	case GDK_KP_Up:
		if (alt) {
			fr_window_go_up_one_level (window);
			retval = TRUE;
		}
		break;

	case GDK_BackSpace:
		fr_window_go_up_one_level (window);
		retval = TRUE;
		break;

	case GDK_Right:
	case GDK_KP_Right:
		if (alt) {
			fr_window_go_forward (window);
			retval = TRUE;
		}
		break;

	case GDK_Left:
	case GDK_KP_Left:
		if (alt) {
			fr_window_go_back (window);
			retval = TRUE;
		}
		break;

	case GDK_Home:
	case GDK_KP_Home:
		if (alt) {
			fr_window_go_to_location (window, "/", FALSE);
			retval = TRUE;
		}
		break;

	default:
		break;
	}

	return retval;
}


static gboolean
dir_tree_selection_changed_cb (GtkTreeSelection *selection,
			       gpointer          user_data)
{
	FrWindow    *window = user_data;
	GtkTreeIter  iter;

	if (gtk_tree_selection_get_selected (selection, NULL, &iter)) {
		char *path;

		gtk_tree_model_get (GTK_TREE_MODEL (window->priv->tree_store),
				    &iter,
				    TREE_COLUMN_PATH, &path,
				    -1);
		fr_window_go_to_location (window, path, FALSE);
		g_free (path);
	}

	return FALSE;
}


static gboolean
selection_changed_cb (GtkTreeSelection *selection,
		      gpointer          user_data)
{
	FrWindow *window = user_data;

	fr_window_update_statusbar_list_info (window);
	fr_window_update_sensitivity (window);

	return FALSE;
}


static void
fr_window_delete_event_cb (GtkWidget *caller,
			   GdkEvent  *event,
			   FrWindow  *window)
{
	fr_window_close (window);
}


static gboolean
is_single_click_policy (void)
{
	char     *value;
	gboolean  result;

	value = eel_mateconf_get_string (PREF_CAJA_CLICK_POLICY, "double");
	result = strncmp (value, "single", 6) == 0;
	g_free (value);

	return result;
}


static void
filename_cell_data_func (GtkTreeViewColumn *column,
			 GtkCellRenderer   *renderer,
			 GtkTreeModel      *model,
			 GtkTreeIter       *iter,
			 FrWindow          *window)
{
	char           *text;
	GtkTreePath    *path;
	PangoUnderline  underline;

	gtk_tree_model_get (model, iter,
			    COLUMN_NAME, &text,
			    -1);

	if (window->priv->single_click) {
		path = gtk_tree_model_get_path (model, iter);

		if ((window->priv->list_hover_path == NULL)
		    || gtk_tree_path_compare (path, window->priv->list_hover_path))
			underline = PANGO_UNDERLINE_NONE;
		else
			underline = PANGO_UNDERLINE_SINGLE;

		gtk_tree_path_free (path);
	}
	else
		underline = PANGO_UNDERLINE_NONE;

	g_object_set (G_OBJECT (renderer),
		      "text", text,
		      "underline", underline,
		      NULL);

	g_free (text);
}


static void
add_dir_tree_columns (FrWindow    *window,
		      GtkTreeView *treeview)
{
	GtkCellRenderer   *renderer;
	GtkTreeViewColumn *column;
	GValue             value = { 0, };

	/* First column. */

	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_title (column, _("Folders"));

	/* icon */

	renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_tree_view_column_pack_start (column, renderer, FALSE);
	gtk_tree_view_column_set_attributes (column, renderer,
					     "pixbuf", TREE_COLUMN_ICON,
					     NULL);

	/* name */

	renderer = gtk_cell_renderer_text_new ();

	g_value_init (&value, PANGO_TYPE_ELLIPSIZE_MODE);
	g_value_set_enum (&value, PANGO_ELLIPSIZE_END);
	g_object_set_property (G_OBJECT (renderer), "ellipsize", &value);
	g_value_unset (&value);

	gtk_tree_view_column_pack_start (column,
					 renderer,
					 TRUE);
	gtk_tree_view_column_set_attributes (column, renderer,
					     "text", TREE_COLUMN_NAME,
					     "weight", TREE_COLUMN_WEIGHT,
					     NULL);

	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_column_set_sort_column_id (column, TREE_COLUMN_NAME);

	gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
}


static void
add_file_list_columns (FrWindow    *window,
		       GtkTreeView *treeview)
{
	static char       *titles[] = {NC_("File", "Size"),
				       NC_("File", "Type"),
				       NC_("File", "Date Modified"),
				       NC_("File", "Location")};
	GtkCellRenderer   *renderer;
	GtkTreeViewColumn *column;
	GValue             value = { 0, };
	int                i, j, w;

	/* First column. */

	window->priv->filename_column = column = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_title (column, C_("File", "Name"));

	/* emblem */

	renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_tree_view_column_pack_end (column, renderer, FALSE);
	gtk_tree_view_column_set_attributes (column, renderer,
					     "pixbuf", COLUMN_EMBLEM,
					     NULL);

	/* icon */

	renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_tree_view_column_pack_start (column, renderer, FALSE);
	gtk_tree_view_column_set_attributes (column, renderer,
					     "pixbuf", COLUMN_ICON,
					     NULL);

	/* name */

	window->priv->single_click = is_single_click_policy ();

	renderer = gtk_cell_renderer_text_new ();

	g_value_init (&value, PANGO_TYPE_ELLIPSIZE_MODE);
	g_value_set_enum (&value, PANGO_ELLIPSIZE_END);
	g_object_set_property (G_OBJECT (renderer), "ellipsize", &value);
	g_value_unset (&value);

	gtk_tree_view_column_pack_start (column,
					 renderer,
					 TRUE);
	gtk_tree_view_column_set_attributes (column, renderer,
					     "text", COLUMN_NAME,
					     NULL);

	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
	w = eel_mateconf_get_integer (PREF_NAME_COLUMN_WIDTH, DEFAULT_NAME_COLUMN_WIDTH);
	if (w <= 0)
		w = DEFAULT_NAME_COLUMN_WIDTH;
	gtk_tree_view_column_set_fixed_width (column, w);
	gtk_tree_view_column_set_resizable (column, TRUE);
	gtk_tree_view_column_set_sort_column_id (column, COLUMN_NAME);
	gtk_tree_view_column_set_cell_data_func (column, renderer,
						 (GtkTreeCellDataFunc) filename_cell_data_func,
						 window, NULL);

	gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

	/* Other columns */

	for (j = 0, i = COLUMN_SIZE; i < NUMBER_OF_COLUMNS; i++, j++) {
		GValue  value = { 0, };

		renderer = gtk_cell_renderer_text_new ();
		column = gtk_tree_view_column_new_with_attributes (_(titles[j]),
								   renderer,
								   "text", i,
								   NULL);

		gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
		gtk_tree_view_column_set_fixed_width (column, OTHER_COLUMNS_WIDTH);
		gtk_tree_view_column_set_resizable (column, TRUE);

		gtk_tree_view_column_set_sort_column_id (column, i);

		g_value_init (&value, PANGO_TYPE_ELLIPSIZE_MODE);
		g_value_set_enum (&value, PANGO_ELLIPSIZE_END);
		g_object_set_property (G_OBJECT (renderer), "ellipsize", &value);
		g_value_unset (&value);

		gtk_tree_view_append_column (treeview, column);
	}
}


static int
name_column_sort_func (GtkTreeModel *model,
		       GtkTreeIter  *a,
		       GtkTreeIter  *b,
		       gpointer      user_data)
{
	FileData *fdata1, *fdata2;

	gtk_tree_model_get (model, a, COLUMN_FILE_DATA, &fdata1, -1);
	gtk_tree_model_get (model, b, COLUMN_FILE_DATA, &fdata2, -1);

	return sort_by_name (&fdata1, &fdata2);
}


static int
size_column_sort_func (GtkTreeModel *model,
		       GtkTreeIter  *a,
		       GtkTreeIter  *b,
		       gpointer      user_data)
{
	FileData *fdata1, *fdata2;

	gtk_tree_model_get (model, a, COLUMN_FILE_DATA, &fdata1, -1);
	gtk_tree_model_get (model, b, COLUMN_FILE_DATA, &fdata2, -1);

	return sort_by_size (&fdata1, &fdata2);
}


static int
type_column_sort_func (GtkTreeModel *model,
		       GtkTreeIter  *a,
		       GtkTreeIter  *b,
		       gpointer      user_data)
{
	FileData *fdata1, *fdata2;

	gtk_tree_model_get (model, a, COLUMN_FILE_DATA, &fdata1, -1);
	gtk_tree_model_get (model, b, COLUMN_FILE_DATA, &fdata2, -1);

	return sort_by_type (&fdata1, &fdata2);
}


static int
time_column_sort_func (GtkTreeModel *model,
		       GtkTreeIter  *a,
		       GtkTreeIter  *b,
		       gpointer      user_data)
{
	FileData *fdata1, *fdata2;

	gtk_tree_model_get (model, a, COLUMN_FILE_DATA, &fdata1, -1);
	gtk_tree_model_get (model, b, COLUMN_FILE_DATA, &fdata2, -1);

	return sort_by_time (&fdata1, &fdata2);
}


static int
path_column_sort_func (GtkTreeModel *model,
		       GtkTreeIter  *a,
		       GtkTreeIter  *b,
		       gpointer      user_data)
{
	FileData *fdata1, *fdata2;

	gtk_tree_model_get (model, a, COLUMN_FILE_DATA, &fdata1, -1);
	gtk_tree_model_get (model, b, COLUMN_FILE_DATA, &fdata2, -1);

	return sort_by_path (&fdata1, &fdata2);
}


static int
no_sort_column_sort_func (GtkTreeModel *model,
			  GtkTreeIter  *a,
			  GtkTreeIter  *b,
			  gpointer      user_data)
{
	return -1;
}


static void
sort_column_changed_cb (GtkTreeSortable *sortable,
			gpointer         user_data)
{
	FrWindow    *window = user_data;
	GtkSortType  order;
	int          column_id;

	if (! gtk_tree_sortable_get_sort_column_id (sortable,
						    &column_id,
						    &order))
		return;

	window->priv->sort_method = get_sort_method_from_column (column_id);
	window->priv->sort_type = order;

	/*set_active (window, get_action_from_sort_method (window->priv->sort_method), TRUE);
	set_active (window, "SortReverseOrder", (window->priv->sort_type == GTK_SORT_DESCENDING));*/
}


static gboolean
fr_window_show_cb (GtkWidget *widget,
		   FrWindow  *window)
{
	fr_window_update_current_location (window);

	set_active (window, "ViewToolbar", eel_mateconf_get_boolean (PREF_UI_TOOLBAR, TRUE));
	set_active (window, "ViewStatusbar", eel_mateconf_get_boolean (PREF_UI_STATUSBAR, TRUE));

	window->priv->view_folders = eel_mateconf_get_boolean (PREF_UI_FOLDERS, FALSE);
	set_active (window, "ViewFolders", window->priv->view_folders);

	fr_window_update_filter_bar_visibility (window);

	return TRUE;
}


/* preferences changes notification callbacks */


static void
pref_history_len_changed (MateConfClient *client,
			  guint        cnxn_id,
			  MateConfEntry  *entry,
			  gpointer     user_data)
{
	FrWindow *window = user_data;

	gtk_recent_chooser_set_limit (GTK_RECENT_CHOOSER (window->priv->recent_chooser_menu), eel_mateconf_get_integer (PREF_UI_HISTORY_LEN, MAX_HISTORY_LEN));
	gtk_recent_chooser_set_limit (GTK_RECENT_CHOOSER (window->priv->recent_chooser_toolbar), eel_mateconf_get_integer (PREF_UI_HISTORY_LEN, MAX_HISTORY_LEN));
}


static void
pref_view_toolbar_changed (MateConfClient *client,
			   guint        cnxn_id,
			   MateConfEntry  *entry,
			   gpointer     user_data)
{
	FrWindow *window = user_data;

	g_return_if_fail (window != NULL);

	fr_window_set_toolbar_visibility (window, mateconf_value_get_bool (mateconf_entry_get_value (entry)));
}


static void
pref_view_statusbar_changed (MateConfClient *client,
			     guint        cnxn_id,
			     MateConfEntry  *entry,
			     gpointer     user_data)
{
	FrWindow *window = user_data;

	fr_window_set_statusbar_visibility (window, mateconf_value_get_bool (mateconf_entry_get_value (entry)));
}


static void
pref_view_folders_changed (MateConfClient *client,
			   guint        cnxn_id,
			   MateConfEntry  *entry,
			   gpointer     user_data)
{
	FrWindow *window = user_data;

	fr_window_set_folders_visibility (window, mateconf_value_get_bool (mateconf_entry_get_value (entry)));
}


static void
pref_show_field_changed (MateConfClient *client,
			 guint        cnxn_id,
			 MateConfEntry  *entry,
			 gpointer     user_data)
{
	FrWindow *window = user_data;

	fr_window_update_columns_visibility (window);
}


static void
pref_click_policy_changed (MateConfClient *client,
			   guint        cnxn_id,
			   MateConfEntry  *entry,
			   gpointer     user_data)
{
	FrWindow   *window = user_data;
	GdkWindow  *win = gtk_tree_view_get_bin_window (GTK_TREE_VIEW (window->priv->list_view));
	GdkDisplay *display;

	window->priv->single_click = is_single_click_policy ();

	gdk_window_set_cursor (win, NULL);
	display = gtk_widget_get_display (GTK_WIDGET (window->priv->list_view));
	if (display != NULL)
		gdk_display_flush (display);
}


static void gh_unref_pixbuf (gpointer  key,
			     gpointer  value,
			     gpointer  user_data);


static void
pref_use_mime_icons_changed (MateConfClient *client,
			     guint        cnxn_id,
			     MateConfEntry  *entry,
			     gpointer     user_data)
{
	FrWindow *window = user_data;

	if (pixbuf_hash != NULL) {
		g_hash_table_foreach (pixbuf_hash,
				      gh_unref_pixbuf,
				      NULL);
		g_hash_table_destroy (pixbuf_hash);
		pixbuf_hash = g_hash_table_new (g_str_hash, g_str_equal);
	}
	if (tree_pixbuf_hash != NULL) {
		g_hash_table_foreach (tree_pixbuf_hash,
				      gh_unref_pixbuf,
				      NULL);
		g_hash_table_destroy (tree_pixbuf_hash);
		tree_pixbuf_hash = g_hash_table_new (g_str_hash, g_str_equal);
	}

	fr_window_update_file_list (window, FALSE);
	fr_window_update_dir_tree (window);
}


static void
theme_changed_cb (GtkIconTheme *theme, FrWindow *window)
{
	int icon_width, icon_height;

	gtk_icon_size_lookup_for_settings (gtk_widget_get_settings (GTK_WIDGET (window)),
					   FILE_LIST_ICON_SIZE,
					   &icon_width, &icon_height);
	file_list_icon_size = MAX (icon_width, icon_height);

	gtk_icon_size_lookup_for_settings (gtk_widget_get_settings (GTK_WIDGET (window)),
					   DIR_TREE_ICON_SIZE,
					   &icon_width, &icon_height);
	dir_tree_icon_size = MAX (icon_width, icon_height);

	if (pixbuf_hash != NULL) {
		g_hash_table_foreach (pixbuf_hash,
				      gh_unref_pixbuf,
				      NULL);
		g_hash_table_destroy (pixbuf_hash);
		pixbuf_hash = g_hash_table_new (g_str_hash, g_str_equal);
	}
	if (tree_pixbuf_hash != NULL) {
		g_hash_table_foreach (tree_pixbuf_hash,
				      gh_unref_pixbuf,
				      NULL);
		g_hash_table_destroy (tree_pixbuf_hash);
		tree_pixbuf_hash = g_hash_table_new (g_str_hash, g_str_equal);
	}

	fr_window_update_file_list (window, TRUE);
	fr_window_update_dir_tree (window);
}


static gboolean
fr_window_stoppable_cb (FrCommand  *command,
			gboolean    stoppable,
			FrWindow   *window)
{
	window->priv->stoppable = stoppable;
	set_sensitive (window, "Stop", stoppable);
	if (window->priv->progress_dialog != NULL)
		gtk_dialog_set_response_sensitive (GTK_DIALOG (window->priv->progress_dialog),
						   GTK_RESPONSE_OK,
						   stoppable);
	return TRUE;
}


static gboolean
fr_window_fake_load (FrArchive *archive,
		     gpointer   data)
{
	/* fake loads are disabled to allow exact progress dialogs (#153281) */

	return FALSE;

#if 0
	FrWindow *window = data;
	gboolean  add_after_opening = FALSE;
	gboolean  extract_after_opening = FALSE;
	GList    *scan;

	/* fake loads are used only in batch mode to avoid unnecessary
	 * archive loadings. */

	if (! window->priv->batch_mode)
		return FALSE;

	/* Check whether there is an ADD or EXTRACT action in the batch list. */

	for (scan = window->priv->batch_action; scan; scan = scan->next) {
		FRBatchAction *action;

		action = (FRBatchAction *) scan->data;
		if (action->type == FR_BATCH_ACTION_ADD) {
			add_after_opening = TRUE;
			break;
		}
		if ((action->type == FR_BATCH_ACTION_EXTRACT)
		    || (action->type == FR_BATCH_ACTION_EXTRACT_HERE)
		    || (action->type == FR_BATCH_ACTION_EXTRACT_INTERACT))
		{
			extract_after_opening = TRUE;
			break;
		}
	}

	/* use fake load when in batch mode and the archive type supports all
	 * of the required features */

	return (window->priv->batch_mode
		&& ! (add_after_opening && window->priv->update_dropped_files && ! archive->command->propAddCanUpdate)
		&& ! (add_after_opening && ! window->priv->update_dropped_files && ! archive->command->propAddCanReplace)
		&& ! (extract_after_opening && !archive->command->propCanExtractAll));
#endif
}


static gboolean
fr_window_add_is_stoppable (FrArchive *archive,
			    gpointer   data)
{
	FrWindow *window = data;
	return window->priv->archive_new;
}


static void
menu_item_select_cb (GtkMenuItem *proxy,
		     FrWindow    *window)
{
	GtkAction *action;
	char      *message;

	action = gtk_activatable_get_related_action (GTK_ACTIVATABLE (proxy));
	g_return_if_fail (action != NULL);

	g_object_get (G_OBJECT (action), "tooltip", &message, NULL);
	if (message) {
		gtk_statusbar_push (GTK_STATUSBAR (window->priv->statusbar),
				    window->priv->help_message_cid, message);
		g_free (message);
	}
}


static void
menu_item_deselect_cb (GtkMenuItem *proxy,
		       FrWindow    *window)
{
	gtk_statusbar_pop (GTK_STATUSBAR (window->priv->statusbar),
			   window->priv->help_message_cid);
}


static void
disconnect_proxy_cb (GtkUIManager *manager,
		     GtkAction    *action,
		     GtkWidget    *proxy,
		     FrWindow     *window)
{
	if (GTK_IS_MENU_ITEM (proxy)) {
		g_signal_handlers_disconnect_by_func
			(proxy, G_CALLBACK (menu_item_select_cb), window);
		g_signal_handlers_disconnect_by_func
			(proxy, G_CALLBACK (menu_item_deselect_cb), window);
	}
}


static void
connect_proxy_cb (GtkUIManager *manager,
		  GtkAction    *action,
		  GtkWidget    *proxy,
		  FrWindow     *window)
{
	if (GTK_IS_MENU_ITEM (proxy)) {
		g_signal_connect (proxy, "select",
				  G_CALLBACK (menu_item_select_cb), window);
		g_signal_connect (proxy, "deselect",
				  G_CALLBACK (menu_item_deselect_cb), window);
	}
}


static void
view_as_radio_action (GtkAction      *action,
		      GtkRadioAction *current,
		      gpointer        data)
{
	FrWindow *window = data;
	fr_window_set_list_mode (window, gtk_radio_action_get_current_value (current));
}


static void
sort_by_radio_action (GtkAction      *action,
		      GtkRadioAction *current,
		      gpointer        data)
{
	FrWindow *window = data;

	window->priv->sort_method = gtk_radio_action_get_current_value (current);
	window->priv->sort_type = GTK_SORT_ASCENDING;
	fr_window_update_list_order (window);
}


static void
recent_chooser_item_activated_cb (GtkRecentChooser *chooser,
				  FrWindow         *window)
{
	char *uri;

	uri = gtk_recent_chooser_get_current_uri (chooser);
	if (uri != NULL) {
		fr_window_archive_open (window, uri, GTK_WINDOW (window));
		g_free (uri);
	}
}


static void
fr_window_init_recent_chooser (FrWindow         *window,
			       GtkRecentChooser *chooser)
{
	GtkRecentFilter *filter;
	int              i;

	g_return_if_fail (chooser != NULL);

	filter = gtk_recent_filter_new ();
	gtk_recent_filter_set_name (filter, _("All archives"));
	for (i = 0; open_type[i] != -1; i++)
		gtk_recent_filter_add_mime_type (filter, mime_type_desc[open_type[i]].mime_type);
	gtk_recent_filter_add_application (filter, "File Roller");
	gtk_recent_chooser_add_filter (chooser, filter);

	gtk_recent_chooser_set_local_only (chooser, FALSE);
	gtk_recent_chooser_set_limit (chooser, eel_mateconf_get_integer (PREF_UI_HISTORY_LEN, MAX_HISTORY_LEN));
	gtk_recent_chooser_set_show_not_found (chooser, TRUE);
	gtk_recent_chooser_set_sort_type (chooser, GTK_RECENT_SORT_MRU);

	g_signal_connect (G_OBJECT (chooser),
			  "item_activated",
			  G_CALLBACK (recent_chooser_item_activated_cb),
			  window);
}


static void
close_sidepane_button_clicked_cb (GtkButton *button,
				  FrWindow  *window)
{
	fr_window_set_folders_visibility (window, FALSE);
}


static void
fr_window_activate_filter (FrWindow *window)
{
	GtkTreeView       *tree_view = GTK_TREE_VIEW (window->priv->list_view);
	GtkTreeViewColumn *column;

	fr_window_update_filter_bar_visibility (window);
	window->priv->list_mode = FR_WINDOW_LIST_MODE_FLAT;

	gtk_list_store_clear (window->priv->list_store);

	column = gtk_tree_view_get_column (tree_view, 4);
	gtk_tree_view_column_set_visible (column, TRUE);

	fr_window_update_file_list (window, TRUE);
	fr_window_update_dir_tree (window);
	fr_window_update_current_location (window);
}


static void
filter_entry_activate_cb (GtkEntry *entry,
			  FrWindow *window)
{
	fr_window_activate_filter (window);
}


static void
filter_entry_icon_release_cb (GtkEntry             *entry,
			      GtkEntryIconPosition  icon_pos,
			      GdkEventButton       *event,
			      gpointer              user_data)
{
	FrWindow *window = FR_WINDOW (user_data);

	if ((event->button == 1) && (icon_pos == GTK_ENTRY_ICON_SECONDARY))
		fr_window_deactivate_filter (window);
}


static void
fr_window_attach (FrWindow      *window,
		  GtkWidget     *child,
		  FrWindowArea   area)
{
	int position;

	g_return_if_fail (window != NULL);
	g_return_if_fail (FR_IS_WINDOW (window));
	g_return_if_fail (child != NULL);
	g_return_if_fail (GTK_IS_WIDGET (child));

	switch (area) {
	case FR_WINDOW_AREA_MENUBAR:
		position = 0;
		break;
	case FR_WINDOW_AREA_TOOLBAR:
		position = 1;
		break;
	case FR_WINDOW_AREA_LOCATIONBAR:
		position = 2;
		break;
	case FR_WINDOW_AREA_CONTENTS:
		position = 3;
		if (window->priv->contents != NULL)
			gtk_widget_destroy (window->priv->contents);
		window->priv->contents = child;
		break;
	case FR_WINDOW_AREA_FILTERBAR:
		position = 4;
		break;
	case FR_WINDOW_AREA_STATUSBAR:
		position = 5;
		break;
	default:
		g_critical ("%s: area not recognized!", G_STRFUNC);
		return;
		break;
	}

	gtk_table_attach (GTK_TABLE (window->priv->layout),
			  child,
			  0, 1,
			  position, position + 1,
			  GTK_EXPAND | GTK_FILL,
			  ((area == FR_WINDOW_AREA_CONTENTS) ? GTK_EXPAND : 0) | GTK_FILL,
			  0, 0);
}


static void
set_action_important (GtkUIManager *ui,
		      const char   *action_name)
{
	GtkAction *action;

	action = gtk_ui_manager_get_action (ui, action_name);
	g_object_set (action, "is_important", TRUE, NULL);
	g_object_unref (action);
}


static void
fr_window_construct (FrWindow *window)
{
	GtkWidget        *menubar;
	GtkWidget        *toolbar;
	GtkWidget        *list_scrolled_window;
	GtkWidget        *location_box;
	GtkStatusbar     *statusbar;
	GtkWidget        *statusbar_box;
	GtkWidget        *filter_box;
	GtkWidget        *tree_scrolled_window;
	GtkWidget        *sidepane_title;
	GtkWidget        *sidepane_title_box;
	GtkWidget        *sidepane_title_label;
	GtkWidget        *close_sidepane_button;
	GtkTreeSelection *selection;
	int               i;
	int               icon_width, icon_height;
	GtkActionGroup   *actions;
	GtkUIManager     *ui;
	GtkToolItem      *open_recent_tool_item;
	GtkWidget        *menu_item;
	GError           *error = NULL;

	/* data common to all windows. */

	if (pixbuf_hash == NULL)
		pixbuf_hash = g_hash_table_new (g_str_hash, g_str_equal);
	if (tree_pixbuf_hash == NULL)
		tree_pixbuf_hash = g_hash_table_new (g_str_hash, g_str_equal);

	if (icon_theme == NULL)
		icon_theme = gtk_icon_theme_get_default ();

	/* Create the application. */

	window->priv->layout = gtk_table_new (4, 1, FALSE);
	gtk_container_add (GTK_CONTAINER (window), window->priv->layout);
	gtk_widget_show (window->priv->layout);

	gtk_window_set_title (GTK_WINDOW (window), _("Archive Manager"));

	g_signal_connect (G_OBJECT (window),
			  "delete_event",
			  G_CALLBACK (fr_window_delete_event_cb),
			  window);

	g_signal_connect (G_OBJECT (window),
			  "show",
			  G_CALLBACK (fr_window_show_cb),
			  window);

	window->priv->theme_changed_handler_id =
		g_signal_connect (icon_theme,
				  "changed",
				  G_CALLBACK (theme_changed_cb),
				  window);

	gtk_icon_size_lookup_for_settings (gtk_widget_get_settings (GTK_WIDGET (window)),
					   FILE_LIST_ICON_SIZE,
					   &icon_width, &icon_height);
	file_list_icon_size = MAX (icon_width, icon_height);

	gtk_icon_size_lookup_for_settings (gtk_widget_get_settings (GTK_WIDGET (window)),
					   DIR_TREE_ICON_SIZE,
					   &icon_width, &icon_height);
	dir_tree_icon_size = MAX (icon_width, icon_height);

	gtk_window_set_default_size (GTK_WINDOW (window),
				     eel_mateconf_get_integer (PREF_UI_WINDOW_WIDTH, DEF_WIN_WIDTH),
				     eel_mateconf_get_integer (PREF_UI_WINDOW_HEIGHT, DEF_WIN_HEIGHT));

	gtk_drag_dest_set (GTK_WIDGET (window),
			   GTK_DEST_DEFAULT_ALL,
			   target_table, G_N_ELEMENTS (target_table),
			   GDK_ACTION_COPY);

	g_signal_connect (G_OBJECT (window),
			  "drag_data_received",
			  G_CALLBACK (fr_window_drag_data_received),
			  window);
	g_signal_connect (G_OBJECT (window),
			  "drag_motion",
			  G_CALLBACK (fr_window_drag_motion),
			  window);

	g_signal_connect (G_OBJECT (window),
			  "key_press_event",
			  G_CALLBACK (key_press_cb),
			  window);

	/* Initialize Data. */

	window->archive = fr_archive_new ();
	g_signal_connect (G_OBJECT (window->archive),
			  "start",
			  G_CALLBACK (action_started),
			  window);
	g_signal_connect (G_OBJECT (window->archive),
			  "done",
			  G_CALLBACK (action_performed),
			  window);
	g_signal_connect (G_OBJECT (window->archive),
			  "progress",
			  G_CALLBACK (fr_window_progress_cb),
			  window);
	g_signal_connect (G_OBJECT (window->archive),
			  "message",
			  G_CALLBACK (fr_window_message_cb),
			  window);
	g_signal_connect (G_OBJECT (window->archive),
			  "stoppable",
			  G_CALLBACK (fr_window_stoppable_cb),
			  window);
	g_signal_connect (G_OBJECT (window->archive),
			  "working_archive",
			  G_CALLBACK (fr_window_working_archive_cb),
			  window);

	fr_archive_set_fake_load_func (window->archive,
				       fr_window_fake_load,
				       window);
	fr_archive_set_add_is_stoppable_func (window->archive,
					      fr_window_add_is_stoppable,
					      window);

	window->priv->sort_method = preferences_get_sort_method ();
	window->priv->sort_type = preferences_get_sort_type ();

	window->priv->list_mode = window->priv->last_list_mode = preferences_get_list_mode ();
	window->priv->history = NULL;
	window->priv->history_current = NULL;

	window->priv->action = FR_ACTION_NONE;

	eel_mateconf_set_boolean (PREF_LIST_SHOW_PATH, (window->priv->list_mode == FR_WINDOW_LIST_MODE_FLAT));

	window->priv->open_default_dir = g_strdup (get_home_uri ());
	window->priv->add_default_dir = g_strdup (get_home_uri ());
	window->priv->extract_default_dir = g_strdup (get_home_uri ());

	window->priv->give_focus_to_the_list = FALSE;

	window->priv->activity_ref = 0;
	window->priv->activity_timeout_handle = 0;

	window->priv->update_timeout_handle = 0;

	window->priv->archive_present = FALSE;
	window->priv->archive_new = FALSE;
	window->priv->archive_uri = NULL;

	window->priv->drag_destination_folder = NULL;
	window->priv->drag_base_dir = NULL;
	window->priv->drag_error = NULL;
	window->priv->drag_file_list = NULL;

	window->priv->batch_mode = FALSE;
	window->priv->batch_action_list = NULL;
	window->priv->batch_action = NULL;
	window->priv->extract_interact_use_default_dir = FALSE;
	window->priv->non_interactive = FALSE;

	window->priv->password = NULL;
	window->priv->compression = preferences_get_compression_level ();
	window->priv->encrypt_header = eel_mateconf_get_boolean (PREF_ENCRYPT_HEADER, FALSE);
	window->priv->volume_size = 0;

	window->priv->convert_data.converting = FALSE;
	window->priv->convert_data.temp_dir = NULL;
	window->priv->convert_data.new_archive = NULL;
	window->priv->convert_data.password = NULL;
	window->priv->convert_data.encrypt_header = FALSE;
	window->priv->convert_data.volume_size = 0;

	window->priv->stoppable = TRUE;

	window->priv->batch_adding_one_file = FALSE;

	window->priv->path_clicked = NULL;

	window->priv->current_view_length = 0;

	window->priv->current_batch_action.type = FR_BATCH_ACTION_NONE;
	window->priv->current_batch_action.data = NULL;
	window->priv->current_batch_action.free_func = NULL;

	window->priv->pd_last_archive = NULL;

	/* Create the widgets. */

	/* * File list. */

	window->priv->list_store = fr_list_model_new (NUMBER_OF_COLUMNS,
						      G_TYPE_POINTER,
						      GDK_TYPE_PIXBUF,
						      G_TYPE_STRING,
						      GDK_TYPE_PIXBUF,
						      G_TYPE_STRING,
						      G_TYPE_STRING,
						      G_TYPE_STRING,
						      G_TYPE_STRING);
	g_object_set_data (G_OBJECT (window->priv->list_store), "FrWindow", window);
	window->priv->list_view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (window->priv->list_store));

	gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (window->priv->list_view), TRUE);
	add_file_list_columns (window, GTK_TREE_VIEW (window->priv->list_view));
	gtk_tree_view_set_enable_search (GTK_TREE_VIEW (window->priv->list_view),
					 FALSE);
	gtk_tree_view_set_search_column (GTK_TREE_VIEW (window->priv->list_view),
					 COLUMN_NAME);

	gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (window->priv->list_store),
					 COLUMN_NAME, name_column_sort_func,
					 NULL, NULL);
	gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (window->priv->list_store),
					 COLUMN_SIZE, size_column_sort_func,
					 NULL, NULL);
	gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (window->priv->list_store),
					 COLUMN_TYPE, type_column_sort_func,
					 NULL, NULL);
	gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (window->priv->list_store),
					 COLUMN_TIME, time_column_sort_func,
					 NULL, NULL);
	gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (window->priv->list_store),
					 COLUMN_PATH, path_column_sort_func,
					 NULL, NULL);

	gtk_tree_sortable_set_default_sort_func (GTK_TREE_SORTABLE (window->priv->list_store),
						 no_sort_column_sort_func,
						 NULL, NULL);

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (window->priv->list_view));
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_MULTIPLE);

	g_signal_connect (selection,
			  "changed",
			  G_CALLBACK (selection_changed_cb),
			  window);
	g_signal_connect (G_OBJECT (window->priv->list_view),
			  "row_activated",
			  G_CALLBACK (row_activated_cb),
			  window);

	g_signal_connect (G_OBJECT (window->priv->list_view),
			  "button_press_event",
			  G_CALLBACK (file_button_press_cb),
			  window);
	g_signal_connect (G_OBJECT (window->priv->list_view),
			  "button_release_event",
			  G_CALLBACK (file_button_release_cb),
			  window);
	g_signal_connect (G_OBJECT (window->priv->list_view),
			  "motion_notify_event",
			  G_CALLBACK (file_motion_notify_callback),
			  window);
	g_signal_connect (G_OBJECT (window->priv->list_view),
			  "leave_notify_event",
			  G_CALLBACK (file_leave_notify_callback),
			  window);

	g_signal_connect (G_OBJECT (window->priv->list_store),
			  "sort_column_changed",
			  G_CALLBACK (sort_column_changed_cb),
			  window);

	g_signal_connect (G_OBJECT (window->priv->list_view),
			  "drag_begin",
			  G_CALLBACK (file_list_drag_begin),
			  window);
	g_signal_connect (G_OBJECT (window->priv->list_view),
			  "drag_end",
			  G_CALLBACK (file_list_drag_end),
			  window);
	egg_tree_multi_drag_add_drag_support (GTK_TREE_VIEW (window->priv->list_view));

	list_scrolled_window = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (list_scrolled_window),
					GTK_POLICY_AUTOMATIC,
					GTK_POLICY_AUTOMATIC);
	gtk_container_add (GTK_CONTAINER (list_scrolled_window), window->priv->list_view);

	/* filter bar */

	window->priv->filter_bar = filter_box = gtk_hbox_new (FALSE, 6);
	gtk_container_set_border_width (GTK_CONTAINER (filter_box), 3);
	fr_window_attach (FR_WINDOW (window), window->priv->filter_bar, FR_WINDOW_AREA_FILTERBAR);

	gtk_box_pack_start (GTK_BOX (filter_box),
			    gtk_label_new (_("Find:")), FALSE, FALSE, 0);

	/* * filter entry */

	window->priv->filter_entry = GTK_WIDGET (gtk_entry_new ());
	gtk_entry_set_icon_from_stock (GTK_ENTRY (window->priv->filter_entry),
				       GTK_ENTRY_ICON_SECONDARY,
				       GTK_STOCK_CLEAR);

	gtk_widget_set_size_request (window->priv->filter_entry, 300, -1);
	gtk_box_pack_start (GTK_BOX (filter_box),
			    window->priv->filter_entry, FALSE, FALSE, 6);

	g_signal_connect (G_OBJECT (window->priv->filter_entry),
			  "activate",
			  G_CALLBACK (filter_entry_activate_cb),
			  window);
	g_signal_connect (G_OBJECT (window->priv->filter_entry),
			  "icon-release",
			  G_CALLBACK (filter_entry_icon_release_cb),
			  window);

	gtk_widget_show_all (filter_box);

	/* tree view */

	window->priv->tree_store = gtk_tree_store_new (TREE_NUMBER_OF_COLUMNS,
						       G_TYPE_STRING,
						       GDK_TYPE_PIXBUF,
						       G_TYPE_STRING,
						       PANGO_TYPE_WEIGHT);
	window->priv->tree_view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (window->priv->tree_store));
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (window->priv->tree_view), FALSE);
	add_dir_tree_columns (window, GTK_TREE_VIEW (window->priv->tree_view));

	g_signal_connect (G_OBJECT (window->priv->tree_view),
			  "button_press_event",
			  G_CALLBACK (dir_tree_button_press_cb),
			  window);

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (window->priv->tree_view));
	g_signal_connect (selection,
			  "changed",
			  G_CALLBACK (dir_tree_selection_changed_cb),
			  window);

	g_signal_connect (G_OBJECT (window->priv->tree_view),
			  "drag_begin",
			  G_CALLBACK (file_list_drag_begin),
			  window);
	g_signal_connect (G_OBJECT (window->priv->tree_view),
			  "drag_end",
			  G_CALLBACK (file_list_drag_end),
			  window);
	g_signal_connect (G_OBJECT (window->priv->tree_view),
			  "drag_data_get",
			  G_CALLBACK (fr_window_folder_tree_drag_data_get),
			  window);
	gtk_drag_source_set (window->priv->tree_view,
			     GDK_BUTTON1_MASK,
			     folder_tree_targets, G_N_ELEMENTS (folder_tree_targets),
			     GDK_ACTION_COPY);

	tree_scrolled_window = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (tree_scrolled_window),
					GTK_POLICY_AUTOMATIC,
					GTK_POLICY_AUTOMATIC);
	gtk_container_add (GTK_CONTAINER (tree_scrolled_window), window->priv->tree_view);

	/* side pane */

	window->priv->sidepane = gtk_vbox_new (FALSE, 0);

	sidepane_title = gtk_frame_new (NULL);
	gtk_frame_set_shadow_type (GTK_FRAME (sidepane_title), GTK_SHADOW_ETCHED_IN);

	sidepane_title_box = gtk_hbox_new (FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (sidepane_title_box), 2);
	gtk_container_add (GTK_CONTAINER (sidepane_title), sidepane_title_box);
	sidepane_title_label = gtk_label_new (_("Folders"));

	gtk_misc_set_alignment (GTK_MISC (sidepane_title_label), 0.0, 0.5);
	gtk_box_pack_start (GTK_BOX (sidepane_title_box), sidepane_title_label, TRUE, TRUE, 0);

	close_sidepane_button = gtk_button_new ();
	gtk_container_add (GTK_CONTAINER (close_sidepane_button), gtk_image_new_from_stock (GTK_STOCK_CLOSE, GTK_ICON_SIZE_MENU));
	gtk_button_set_relief (GTK_BUTTON (close_sidepane_button), GTK_RELIEF_NONE);
	gtk_widget_set_tooltip_text (close_sidepane_button, _("Close the folders pane"));
	g_signal_connect (close_sidepane_button,
			  "clicked",
			  G_CALLBACK (close_sidepane_button_clicked_cb),
			  window);
	gtk_box_pack_end (GTK_BOX (sidepane_title_box), close_sidepane_button, FALSE, FALSE, 0);

	gtk_box_pack_start (GTK_BOX (window->priv->sidepane), sidepane_title, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (window->priv->sidepane), tree_scrolled_window, TRUE, TRUE, 0);

	/* main content */

	window->priv->paned = gtk_hpaned_new ();
	gtk_paned_pack1 (GTK_PANED (window->priv->paned), window->priv->sidepane, FALSE, TRUE);
	gtk_paned_pack2 (GTK_PANED (window->priv->paned), list_scrolled_window, TRUE, TRUE);
	gtk_paned_set_position (GTK_PANED (window->priv->paned), eel_mateconf_get_integer (PREF_UI_SIDEBAR_WIDTH, DEF_SIDEBAR_WIDTH));

	fr_window_attach (FR_WINDOW (window), window->priv->paned, FR_WINDOW_AREA_CONTENTS);
	gtk_widget_show_all (window->priv->paned);

	/* Build the menu and the toolbar. */

	ui = gtk_ui_manager_new ();

	window->priv->actions = actions = gtk_action_group_new ("Actions");
	gtk_action_group_set_translation_domain (actions, NULL);
	gtk_action_group_add_actions (actions,
				      action_entries,
				      n_action_entries,
				      window);
	gtk_action_group_add_toggle_actions (actions,
					     action_toggle_entries,
					     n_action_toggle_entries,
					     window);
	gtk_action_group_add_radio_actions (actions,
					    view_as_entries,
					    n_view_as_entries,
					    window->priv->list_mode,
					    G_CALLBACK (view_as_radio_action),
					    window);
	gtk_action_group_add_radio_actions (actions,
					    sort_by_entries,
					    n_sort_by_entries,
					    window->priv->sort_type,
					    G_CALLBACK (sort_by_radio_action),
					    window);

	g_signal_connect (ui, "connect_proxy",
			  G_CALLBACK (connect_proxy_cb), window);
	g_signal_connect (ui, "disconnect_proxy",
			  G_CALLBACK (disconnect_proxy_cb), window);

	gtk_ui_manager_insert_action_group (ui, actions, 0);
	gtk_window_add_accel_group (GTK_WINDOW (window),
				    gtk_ui_manager_get_accel_group (ui));

	/* Add a hidden short cut Ctrl-Q for power users */
	gtk_accel_group_connect (gtk_ui_manager_get_accel_group (ui),
				 GDK_q, GDK_CONTROL_MASK, 0,
				 g_cclosure_new_swap (G_CALLBACK (fr_window_close), window, NULL));


	if (!gtk_ui_manager_add_ui_from_string (ui, ui_info, -1, &error)) {
		g_message ("building menus failed: %s", error->message);
		g_error_free (error);
	}

	menubar = gtk_ui_manager_get_widget (ui, "/MenuBar");
	fr_window_attach (FR_WINDOW (window), menubar, FR_WINDOW_AREA_MENUBAR);
	gtk_widget_show (menubar);

	window->priv->toolbar = toolbar = gtk_ui_manager_get_widget (ui, "/ToolBar");
	gtk_toolbar_set_show_arrow (GTK_TOOLBAR (toolbar), TRUE);
	set_action_important (ui, "/ToolBar/Extract_Toolbar");

	/* location bar */

	window->priv->location_bar = gtk_ui_manager_get_widget (ui, "/LocationBar");
	gtk_toolbar_set_show_arrow (GTK_TOOLBAR (window->priv->location_bar), FALSE);
	gtk_toolbar_set_style (GTK_TOOLBAR (window->priv->location_bar), GTK_TOOLBAR_BOTH_HORIZ);
	set_action_important (ui, "/LocationBar/GoBack");

	/* current location */

	location_box = gtk_hbox_new (FALSE, 6);
	/* Translators: after the colon there is a folder name. */
	window->priv->location_label = gtk_label_new_with_mnemonic (_("_Location:"));
	gtk_box_pack_start (GTK_BOX (location_box),
			    window->priv->location_label, FALSE, FALSE, 5);

	window->priv->location_entry = gtk_entry_new ();
	gtk_entry_set_icon_from_stock (GTK_ENTRY (window->priv->location_entry),
				       GTK_ENTRY_ICON_PRIMARY,
				       GTK_STOCK_DIRECTORY);

	gtk_box_pack_start (GTK_BOX (location_box),
			    window->priv->location_entry, TRUE, TRUE, 5);

	g_signal_connect (G_OBJECT (window->priv->location_entry),
			  "key_press_event",
			  G_CALLBACK (location_entry_key_press_event_cb),
			  window);

	{
		GtkToolItem *tool_item;

		tool_item = gtk_separator_tool_item_new ();
		gtk_widget_show_all (GTK_WIDGET (tool_item));
		gtk_toolbar_insert (GTK_TOOLBAR (window->priv->location_bar), tool_item, -1);

		tool_item = gtk_tool_item_new ();
		gtk_tool_item_set_expand (tool_item, TRUE);
		gtk_container_add (GTK_CONTAINER (tool_item), location_box);
		gtk_widget_show_all (GTK_WIDGET (tool_item));
		gtk_toolbar_insert (GTK_TOOLBAR (window->priv->location_bar), tool_item, -1);
	}

	fr_window_attach (FR_WINDOW (window), window->priv->location_bar, FR_WINDOW_AREA_LOCATIONBAR);
	if (window->priv->list_mode == FR_WINDOW_LIST_MODE_FLAT)
		gtk_widget_hide (window->priv->location_bar);
	else
		gtk_widget_show (window->priv->location_bar);

	/* Recent manager */

	window->priv->recent_manager = gtk_recent_manager_get_default ();

	window->priv->recent_chooser_menu = gtk_recent_chooser_menu_new_for_manager (window->priv->recent_manager);
	gtk_recent_chooser_set_sort_type (GTK_RECENT_CHOOSER (window->priv->recent_chooser_menu), GTK_RECENT_SORT_MRU);
	fr_window_init_recent_chooser (window, GTK_RECENT_CHOOSER (window->priv->recent_chooser_menu));
	menu_item = gtk_ui_manager_get_widget (ui, "/MenuBar/Archive/OpenRecentMenu");
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (menu_item), window->priv->recent_chooser_menu);

	window->priv->recent_chooser_toolbar = gtk_recent_chooser_menu_new_for_manager (window->priv->recent_manager);
	fr_window_init_recent_chooser (window, GTK_RECENT_CHOOSER (window->priv->recent_chooser_toolbar));

	/* Add the recent menu tool item */

	open_recent_tool_item = gtk_menu_tool_button_new_from_stock (GTK_STOCK_OPEN);
	gtk_menu_tool_button_set_menu (GTK_MENU_TOOL_BUTTON (open_recent_tool_item), window->priv->recent_chooser_toolbar);
	gtk_tool_item_set_homogeneous (open_recent_tool_item, FALSE);
	gtk_widget_set_tooltip_text (GTK_WIDGET (open_recent_tool_item), _("Open archive"));
	gtk_menu_tool_button_set_arrow_tooltip_text (GTK_MENU_TOOL_BUTTON (open_recent_tool_item), _("Open a recently used archive"));

	window->priv->open_action = gtk_action_new ("Toolbar_Open", _("Open"), _("Open archive"), GTK_STOCK_OPEN);
	g_object_set (window->priv->open_action, "is_important", TRUE, NULL);
	g_signal_connect (window->priv->open_action,
			  "activate",
			  G_CALLBACK (activate_action_open),
			  window);
	gtk_activatable_set_related_action (GTK_ACTIVATABLE (open_recent_tool_item), window->priv->open_action);

	gtk_widget_show (GTK_WIDGET (open_recent_tool_item));
	gtk_toolbar_insert (GTK_TOOLBAR (toolbar), open_recent_tool_item, 1);

	/**/

	fr_window_attach (FR_WINDOW (window), window->priv->toolbar, FR_WINDOW_AREA_TOOLBAR);
	if (eel_mateconf_get_boolean (PREF_UI_TOOLBAR, TRUE))
		gtk_widget_show (toolbar);
	else
		gtk_widget_hide (toolbar);

	window->priv->file_popup_menu = gtk_ui_manager_get_widget (ui, "/FilePopupMenu");
	window->priv->folder_popup_menu = gtk_ui_manager_get_widget (ui, "/FolderPopupMenu");
	window->priv->sidebar_folder_popup_menu = gtk_ui_manager_get_widget (ui, "/SidebarFolderPopupMenu");

	/* Create the statusbar. */

	window->priv->statusbar = gtk_statusbar_new ();
	window->priv->help_message_cid = gtk_statusbar_get_context_id (GTK_STATUSBAR (window->priv->statusbar), "help_message");
	window->priv->list_info_cid = gtk_statusbar_get_context_id (GTK_STATUSBAR (window->priv->statusbar), "list_info");
	window->priv->progress_cid = gtk_statusbar_get_context_id (GTK_STATUSBAR (window->priv->statusbar), "progress");

	statusbar = GTK_STATUSBAR (window->priv->statusbar);
#if GTK_CHECK_VERSION (2, 19, 1)
	statusbar_box = gtk_statusbar_get_message_area (statusbar);
	gtk_box_set_homogeneous (GTK_BOX (statusbar_box), FALSE);
	gtk_box_set_spacing (GTK_BOX (statusbar_box), 4);
	gtk_box_set_child_packing (GTK_BOX (statusbar_box), gtk_statusbar_get_message_area (statusbar), TRUE, TRUE, 0, GTK_PACK_START );
#else
	statusbar_box = gtk_hbox_new (FALSE, 4);
	g_object_ref (statusbar->label);
	gtk_container_remove (GTK_CONTAINER (statusbar->frame), statusbar->label);
	gtk_box_pack_start (GTK_BOX (statusbar_box), statusbar->label, TRUE, TRUE, 0);
	g_object_unref (statusbar->label);
	gtk_container_add (GTK_CONTAINER (statusbar->frame), statusbar_box);
#endif

	window->priv->progress_bar = gtk_progress_bar_new ();
	gtk_progress_bar_set_pulse_step (GTK_PROGRESS_BAR (window->priv->progress_bar), ACTIVITY_PULSE_STEP);
	gtk_widget_set_size_request (window->priv->progress_bar, -1, PROGRESS_BAR_HEIGHT);
	{
		GtkWidget *vbox;

		vbox = gtk_vbox_new (FALSE, 0);
		gtk_box_pack_start (GTK_BOX (statusbar_box), vbox, FALSE, FALSE, 0);
		gtk_box_pack_start (GTK_BOX (vbox), window->priv->progress_bar, TRUE, TRUE, 1);
		gtk_widget_show (vbox);
	}
	gtk_widget_show (statusbar_box);
	gtk_statusbar_set_has_resize_grip (GTK_STATUSBAR (window->priv->statusbar), TRUE);

	fr_window_attach (FR_WINDOW (window), window->priv->statusbar, FR_WINDOW_AREA_STATUSBAR);
	if (eel_mateconf_get_boolean (PREF_UI_STATUSBAR, TRUE))
		gtk_widget_show (window->priv->statusbar);
	else
		gtk_widget_hide (window->priv->statusbar);

	/**/

	fr_window_update_title (window);
	fr_window_update_sensitivity (window);
	fr_window_update_file_list (window, FALSE);
	fr_window_update_dir_tree (window);
	fr_window_update_current_location (window);
	fr_window_update_columns_visibility (window);

	/* Add notification callbacks. */

	i = 0;

	window->priv->cnxn_id[i++] = eel_mateconf_notification_add (
					   PREF_UI_HISTORY_LEN,
					   pref_history_len_changed,
					   window);
	window->priv->cnxn_id[i++] = eel_mateconf_notification_add (
					   PREF_UI_TOOLBAR,
					   pref_view_toolbar_changed,
					   window);
	window->priv->cnxn_id[i++] = eel_mateconf_notification_add (
					   PREF_UI_STATUSBAR,
					   pref_view_statusbar_changed,
					   window);
	window->priv->cnxn_id[i++] = eel_mateconf_notification_add (
					   PREF_UI_FOLDERS,
					   pref_view_folders_changed,
					   window);
	window->priv->cnxn_id[i++] = eel_mateconf_notification_add (
					   PREF_LIST_SHOW_TYPE,
					   pref_show_field_changed,
					   window);
	window->priv->cnxn_id[i++] = eel_mateconf_notification_add (
					   PREF_LIST_SHOW_SIZE,
					   pref_show_field_changed,
					   window);
	window->priv->cnxn_id[i++] = eel_mateconf_notification_add (
					   PREF_LIST_SHOW_TIME,
					   pref_show_field_changed,
					   window);
	window->priv->cnxn_id[i++] = eel_mateconf_notification_add (
					   PREF_LIST_SHOW_PATH,
					   pref_show_field_changed,
					   window);
	window->priv->cnxn_id[i++] = eel_mateconf_notification_add (
					   PREF_LIST_USE_MIME_ICONS,
					   pref_use_mime_icons_changed,
					   window);

	window->priv->cnxn_id[i++] = eel_mateconf_notification_add (
					   PREF_CAJA_CLICK_POLICY,
					   pref_click_policy_changed,
					   window);

	/* Give focus to the list. */

	gtk_widget_grab_focus (window->priv->list_view);
}


GtkWidget *
fr_window_new (void)
{
	GtkWidget *window;

	window = g_object_new (FR_TYPE_WINDOW, NULL);
	fr_window_construct ((FrWindow*) window);

	return window;
}


static void
fr_window_set_archive_uri (FrWindow   *window,
			   const char *uri)
{
	if (window->priv->archive_uri != NULL)
			g_free (window->priv->archive_uri);
		window->priv->archive_uri = g_strdup (uri);
}


gboolean
fr_window_archive_new (FrWindow   *window,
		       const char *uri)
{
	g_return_val_if_fail (window != NULL, FALSE);

	if (! fr_archive_create (window->archive, uri)) {
		GtkWindow *file_sel = g_object_get_data (G_OBJECT (window), "fr_file_sel");

		window->priv->load_error_parent_window = file_sel;
		fr_archive_action_completed (window->archive,
					     FR_ACTION_CREATING_NEW_ARCHIVE,
					     FR_PROC_ERROR_GENERIC,
					     _("Archive type not supported."));

		return FALSE;
	}

	fr_window_set_archive_uri (window, uri);
	window->priv->archive_present = TRUE;
	window->priv->archive_new = TRUE;

	fr_archive_action_completed (window->archive,
				     FR_ACTION_CREATING_NEW_ARCHIVE,
				     FR_PROC_ERROR_NONE,
				     NULL);

	return TRUE;
}


FrWindow *
fr_window_archive_open (FrWindow   *current_window,
			const char *uri,
			GtkWindow  *parent)
{
	FrWindow *window = current_window;

	if (current_window->priv->archive_present)
		window = (FrWindow *) fr_window_new ();

	g_return_val_if_fail (window != NULL, FALSE);

	fr_window_archive_close (window);

	fr_window_set_archive_uri (window, uri);
	window->priv->archive_present = FALSE;
	window->priv->give_focus_to_the_list = TRUE;
	window->priv->load_error_parent_window = parent;

	fr_window_set_current_batch_action (window,
					    FR_BATCH_ACTION_LOAD,
					    g_strdup (window->priv->archive_uri),
					    (GFreeFunc) g_free);

	fr_archive_load (window->archive, window->priv->archive_uri, window->priv->password);

	return window;
}


void
fr_window_archive_close (FrWindow *window)
{
	g_return_if_fail (window != NULL);

	if (! window->priv->archive_new && ! window->priv->archive_present)
		return;

	fr_window_free_open_files (window);
	fr_clipboard_data_unref (window->priv->copy_data);
	window->priv->copy_data = NULL;

	fr_window_set_password (window, NULL);
	fr_window_set_volume_size(window, 0);
	fr_window_history_clear (window);

	window->priv->archive_new = FALSE;
	window->priv->archive_present = FALSE;

	fr_window_update_title (window);
	fr_window_update_sensitivity (window);
	fr_window_update_file_list (window, FALSE);
	fr_window_update_dir_tree (window);
	fr_window_update_current_location (window);
	fr_window_update_statusbar_list_info (window);
}


const char *
fr_window_get_archive_uri (FrWindow *window)
{
	g_return_val_if_fail (window != NULL, NULL);

	return window->priv->archive_uri;
}


const char *
fr_window_get_paste_archive_uri (FrWindow *window)
{
	g_return_val_if_fail (window != NULL, NULL);

	if (window->priv->clipboard_data != NULL)
		return window->priv->clipboard_data->archive_filename;
	else
		return NULL;
}


gboolean
fr_window_archive_is_present (FrWindow *window)
{
	g_return_val_if_fail (window != NULL, FALSE);

	return window->priv->archive_present;
}


typedef struct {
	char     *uri;
	char     *password;
	gboolean  encrypt_header;
	guint     volume_size;
} SaveAsData;


static SaveAsData *
save_as_data_new (const char *uri,
		  const char *password,
		  gboolean    encrypt_header,
	  	  guint       volume_size)
{
	SaveAsData *sdata;

	sdata = g_new0 (SaveAsData, 1);
	if (uri != NULL)
		sdata->uri = g_strdup (uri);
	if (password != NULL)
		sdata->password = g_strdup (password);
	sdata->encrypt_header = encrypt_header;
	sdata->volume_size = volume_size;

	return sdata;
}


static void
save_as_data_free (SaveAsData *sdata)
{
	if (sdata == NULL)
		return;
	g_free (sdata->uri);
	g_free (sdata->password);
	g_free (sdata);
}


void
fr_window_archive_save_as (FrWindow   *window,
			   const char *uri,
			   const char *password,
			   gboolean    encrypt_header,
			   guint       volume_size)
{
	g_return_if_fail (window != NULL);
	g_return_if_fail (uri != NULL);
	g_return_if_fail (window->archive != NULL);

	fr_window_convert_data_free (window, TRUE);
	window->priv->convert_data.new_file = g_strdup (uri);

	/* create the new archive */

	window->priv->convert_data.new_archive = fr_archive_new ();
	if (! fr_archive_create (window->priv->convert_data.new_archive, uri)) {
		GtkWidget *d;
		char      *utf8_name;
		char      *message;

		utf8_name = g_uri_display_basename (uri);
		message = g_strdup_printf (_("Could not save the archive \"%s\""), utf8_name);
		g_free (utf8_name);

		d = _gtk_error_dialog_new (GTK_WINDOW (window),
					   GTK_DIALOG_DESTROY_WITH_PARENT,
					   NULL,
					   message,
					   "%s",
					   _("Archive type not supported."));
		gtk_dialog_run (GTK_DIALOG (d));
		gtk_widget_destroy (d);

		g_free (message);

		g_object_unref (window->priv->convert_data.new_archive);
		window->priv->convert_data.new_archive = NULL;

		return;
	}

	g_return_if_fail (window->priv->convert_data.new_archive->command != NULL);

	if (password != NULL) {
		window->priv->convert_data.password = g_strdup (password);
		window->priv->convert_data.encrypt_header = encrypt_header;
	}
	else
		window->priv->convert_data.encrypt_header = FALSE;
	window->priv->convert_data.volume_size = volume_size;

	fr_window_set_current_batch_action (window,
					    FR_BATCH_ACTION_SAVE_AS,
					    save_as_data_new (uri, password, encrypt_header, volume_size),
					    (GFreeFunc) save_as_data_free);

	g_signal_connect (G_OBJECT (window->priv->convert_data.new_archive),
			  "start",
			  G_CALLBACK (action_started),
			  window);
	g_signal_connect (G_OBJECT (window->priv->convert_data.new_archive),
			  "done",
			  G_CALLBACK (convert__action_performed),
			  window);
	g_signal_connect (G_OBJECT (window->priv->convert_data.new_archive),
			  "progress",
			  G_CALLBACK (fr_window_progress_cb),
			  window);
	g_signal_connect (G_OBJECT (window->priv->convert_data.new_archive),
			  "message",
			  G_CALLBACK (fr_window_message_cb),
			  window);
	g_signal_connect (G_OBJECT (window->priv->convert_data.new_archive),
			  "stoppable",
			  G_CALLBACK (fr_window_stoppable_cb),
			  window);

	window->priv->convert_data.converting = TRUE;
	window->priv->convert_data.temp_dir = get_temp_work_dir (NULL);

	fr_process_clear (window->archive->process);
	fr_archive_extract_to_local (window->archive,
				     NULL,
				     window->priv->convert_data.temp_dir,
				     NULL,
				     TRUE,
				     FALSE,
				     FALSE,
				     window->priv->password);
	fr_process_start (window->archive->process);
}


void
fr_window_archive_reload (FrWindow *window)
{
	g_return_if_fail (window != NULL);

	if (window->priv->activity_ref > 0)
		return;
	if (window->priv->archive_new)
		return;

	fr_archive_reload (window->archive, window->priv->password);
}


void
fr_window_archive_rename (FrWindow   *window,
			  const char *uri)
{
	g_return_if_fail (window != NULL);

	if (window->priv->archive_new) {
		fr_window_archive_new (window, uri);
		return;
	}

	fr_archive_rename (window->archive, uri);
	fr_window_set_archive_uri (window, uri);

	fr_window_update_title (window);
	fr_window_add_to_recent_list (window, window->priv->archive_uri);
}


/**/


void
fr_window_archive_add_files (FrWindow *window,
			     GList    *file_list, /* GFile list */
			     gboolean  update)
{
	GFile *base;
	char  *base_dir;
	int    base_len;
	GList *files = NULL;
	GList *scan;
	char  *base_uri;

	base = g_file_get_parent ((GFile *) file_list->data);
	base_dir = g_file_get_path (base);
	base_len = 0;
	if (strcmp (base_dir, "/") != 0)
		base_len = strlen (base_dir);

	for (scan = file_list; scan; scan = scan->next) {
		GFile *file = scan->data;
		char  *path;
		char  *rel_path;

		path = g_file_get_path (file);
		rel_path = g_strdup (path + base_len + 1);
		files = g_list_prepend (files, rel_path);

		g_free (path);
	}

	base_uri = g_file_get_uri (base);

	fr_archive_add_files (window->archive,
			      files,
			      base_uri,
			      fr_window_get_current_location (window),
			      update,
			      window->priv->password,
			      window->priv->encrypt_header,
			      window->priv->compression,
			      window->priv->volume_size);

	g_free (base_uri);
	path_list_free (files);
	g_free (base_dir);
	g_object_unref (base);
}


void
fr_window_archive_add_with_wildcard (FrWindow      *window,
				     const char    *include_files,
				     const char    *exclude_files,
				     const char    *exclude_folders,
				     const char    *base_dir,
				     const char    *dest_dir,
				     gboolean       update,
				     gboolean       follow_links)
{
	fr_archive_add_with_wildcard (window->archive,
				      include_files,
				      exclude_files,
				      exclude_folders,
				      base_dir,
				      (dest_dir == NULL)? fr_window_get_current_location (window): dest_dir,
				      update,
				      follow_links,
				      window->priv->password,
				      window->priv->encrypt_header,
				      window->priv->compression,
				      window->priv->volume_size);
}


void
fr_window_archive_add_directory (FrWindow      *window,
				 const char    *directory,
				 const char    *base_dir,
				 const char    *dest_dir,
				 gboolean       update)
{
	fr_archive_add_directory (window->archive,
				  directory,
				  base_dir,
				  (dest_dir == NULL)? fr_window_get_current_location (window): dest_dir,
				  update,
				  window->priv->password,
				  window->priv->encrypt_header,
				  window->priv->compression,
				  window->priv->volume_size);
}


void
fr_window_archive_add_items (FrWindow      *window,
			     GList         *item_list,
			     const char    *base_dir,
			     const char    *dest_dir,
			     gboolean       update)
{
	fr_archive_add_items (window->archive,
			      item_list,
			      base_dir,
			      (dest_dir == NULL)? fr_window_get_current_location (window): dest_dir,
			      update,
			      window->priv->password,
			      window->priv->encrypt_header,
			      window->priv->compression,
			      window->priv->volume_size);
}


void
fr_window_archive_add_dropped_items (FrWindow *window,
				     GList    *item_list,
				     gboolean  update)
{
	fr_archive_add_dropped_items (window->archive,
				      item_list,
				      fr_window_get_current_location (window),
				      fr_window_get_current_location (window),
				      update,
				      window->priv->password,
				      window->priv->encrypt_header,
				      window->priv->compression,
				      window->priv->volume_size);
}


void
fr_window_archive_remove (FrWindow      *window,
			  GList         *file_list)
{
	fr_window_clipboard_remove_file_list (window, file_list);

	fr_process_clear (window->archive->process);
	fr_archive_remove (window->archive, file_list, window->priv->compression);
	fr_process_start (window->archive->process);
}


/* -- window_archive_extract -- */


static ExtractData*
extract_data_new (GList      *file_list,
		  const char *extract_to_dir,
		  const char *base_dir,
		  gboolean    skip_older,
		  gboolean    overwrite,
		  gboolean    junk_paths,
		  gboolean    extract_here)
{
	ExtractData *edata;

	edata = g_new0 (ExtractData, 1);
	edata->file_list = path_list_dup (file_list);
	if (extract_to_dir != NULL)
		edata->extract_to_dir = g_strdup (extract_to_dir);
	edata->skip_older = skip_older;
	edata->overwrite = overwrite;
	edata->junk_paths = junk_paths;
	if (base_dir != NULL)
		edata->base_dir = g_strdup (base_dir);
	edata->extract_here = extract_here;

	return edata;
}


static ExtractData*
extract_to_data_new (const char *extract_to_dir)
{
	return extract_data_new (NULL,
				 extract_to_dir,
				 NULL,
				 FALSE,
				 TRUE,
				 FALSE,
				 FALSE);
}


static void
extract_data_free (ExtractData *edata)
{
	g_return_if_fail (edata != NULL);

	path_list_free (edata->file_list);
	g_free (edata->extract_to_dir);
	g_free (edata->base_dir);

	g_free (edata);
}


static gboolean
archive_is_encrypted (FrWindow *window,
		      GList    *file_list)
{
	gboolean encrypted = FALSE;

	if (file_list == NULL) {
		int i;

		for (i = 0; ! encrypted && i < window->archive->command->files->len; i++) {
			FileData *fdata = g_ptr_array_index (window->archive->command->files, i);

			if (fdata->encrypted)
				encrypted = TRUE;
		}
	}
	else {

		GHashTable *file_hash;
		int         i;
		GList      *scan;

		file_hash = g_hash_table_new (g_str_hash, g_str_equal);
		for (i = 0; i < window->archive->command->files->len; i++) {
			FileData *fdata = g_ptr_array_index (window->archive->command->files, i);
			g_hash_table_insert (file_hash, fdata->original_path, fdata);
		}

		for (scan = file_list; ! encrypted && scan; scan = scan->next) {
			char     *filename = scan->data;
			FileData *fdata;

			fdata = g_hash_table_lookup (file_hash, filename);
			g_return_val_if_fail (fdata != NULL, FALSE);

			if (fdata->encrypted)
				encrypted = TRUE;
		}

		g_hash_table_destroy (file_hash);
	}

	return encrypted;
}


void
fr_window_archive_extract_here (FrWindow   *window,
				gboolean    skip_older,
				gboolean    overwrite,
				gboolean    junk_paths)
{
	ExtractData *edata;

	edata = extract_data_new (NULL,
				  NULL,
				  NULL,
				  skip_older,
				  overwrite,
				  junk_paths,
				  TRUE);
	fr_window_set_current_batch_action (window,
					    FR_BATCH_ACTION_EXTRACT,
					    edata,
					    (GFreeFunc) extract_data_free);

	if (archive_is_encrypted (window, NULL) && (window->priv->password == NULL)) {
		dlg_ask_password (window);
		return;
	}

	window->priv->ask_to_open_destination_after_extraction = FALSE;

	fr_process_clear (window->archive->process);
	if (fr_archive_extract_here (window->archive,
				     edata->skip_older,
				     edata->overwrite,
				     edata->junk_paths,
				     window->priv->password))
	{
		fr_process_start (window->archive->process);
	}
}


void
fr_window_archive_extract (FrWindow   *window,
			   GList      *file_list,
			   const char *extract_to_dir,
			   const char *base_dir,
			   gboolean    skip_older,
			   gboolean    overwrite,
			   gboolean    junk_paths,
			   gboolean    ask_to_open_destination)
{
	ExtractData *edata;
	gboolean     do_not_extract = FALSE;
	GError      *error = NULL;

	edata = extract_data_new (file_list,
				  extract_to_dir,
				  base_dir,
				  skip_older,
				  overwrite,
				  junk_paths,
				  FALSE);

	fr_window_set_current_batch_action (window,
					    FR_BATCH_ACTION_EXTRACT,
					    edata,
					    (GFreeFunc) extract_data_free);

	if (archive_is_encrypted (window, edata->file_list) && (window->priv->password == NULL)) {
		dlg_ask_password (window);
		return;
	}

	if (! uri_is_dir (edata->extract_to_dir)) {
		if (! ForceDirectoryCreation) {
			GtkWidget *d;
			int        r;
			char      *folder_name;
			char      *msg;

			folder_name = g_filename_display_name (edata->extract_to_dir);
			msg = g_strdup_printf (_("Destination folder \"%s\" does not exist.\n\nDo you want to create it?"), folder_name);
			g_free (folder_name);

			d = _gtk_message_dialog_new (GTK_WINDOW (window),
						     GTK_DIALOG_MODAL,
						     GTK_STOCK_DIALOG_QUESTION,
						     msg,
						     NULL,
						     GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
						     _("Create _Folder"), GTK_RESPONSE_YES,
						     NULL);

			gtk_dialog_set_default_response (GTK_DIALOG (d), GTK_RESPONSE_YES);
			r = gtk_dialog_run (GTK_DIALOG (d));
			gtk_widget_destroy (GTK_WIDGET (d));

			g_free (msg);

			if (r != GTK_RESPONSE_YES)
				do_not_extract = TRUE;
		}

		if (! do_not_extract && ! ensure_dir_exists (edata->extract_to_dir, 0755, &error)) {
			GtkWidget  *d;

			d = _gtk_error_dialog_new (GTK_WINDOW (window),
						   0,
						   NULL,
						   _("Extraction not performed"),
						   _("Could not create the destination folder: %s."),
						   error->message);
			g_clear_error (&error);
			fr_window_show_error_dialog (window, d, GTK_WINDOW (window));
			fr_window_stop_batch (window);

			return;
		}
	}

	if (do_not_extract) {
		GtkWidget *d;

		d = _gtk_message_dialog_new (GTK_WINDOW (window),
					     0,
					     GTK_STOCK_DIALOG_WARNING,
					     _("Extraction not performed"),
					     NULL,
					     GTK_STOCK_OK, GTK_RESPONSE_OK,
					     NULL);
		gtk_dialog_set_default_response (GTK_DIALOG (d), GTK_RESPONSE_OK);
		fr_window_show_error_dialog (window, d, GTK_WINDOW (window));
		fr_window_stop_batch (window);

		return;
	}

	window->priv->ask_to_open_destination_after_extraction = ask_to_open_destination;

	fr_process_clear (window->archive->process);
	fr_archive_extract (window->archive,
			    edata->file_list,
			    edata->extract_to_dir,
			    edata->base_dir,
			    edata->skip_older,
			    edata->overwrite,
			    edata->junk_paths,
			    window->priv->password);
	fr_process_start (window->archive->process);
}


void
fr_window_archive_test (FrWindow *window)
{
	fr_window_set_current_batch_action (window,
					    FR_BATCH_ACTION_TEST,
					    NULL,
					    NULL);
	fr_archive_test (window->archive, window->priv->password);
}


void
fr_window_set_password (FrWindow   *window,
			const char *password)
{
	g_return_if_fail (window != NULL);

	if (window->priv->password != NULL) {
		g_free (window->priv->password);
		window->priv->password = NULL;
	}

	if ((password != NULL) && (password[0] != '\0'))
		window->priv->password = g_strdup (password);
}

void
fr_window_set_password_for_paste (FrWindow   *window,
				  const char *password)
{
	g_return_if_fail (window != NULL);

	if (window->priv->password_for_paste != NULL) {
		g_free (window->priv->password_for_paste);
		window->priv->password_for_paste = NULL;
	}

	if ((password != NULL) && (password[0] != '\0'))
		window->priv->password_for_paste = g_strdup (password);
}

const char *
fr_window_get_password (FrWindow *window)
{
	g_return_val_if_fail (window != NULL, NULL);

	return window->priv->password;
}


void
fr_window_set_encrypt_header (FrWindow *window,
			      gboolean  encrypt_header)
{
	g_return_if_fail (window != NULL);

	window->priv->encrypt_header = encrypt_header;
}


gboolean
fr_window_get_encrypt_header (FrWindow *window)
{
	return window->priv->encrypt_header;
}


void
fr_window_set_compression (FrWindow      *window,
			   FrCompression  compression)
{
	g_return_if_fail (window != NULL);

	window->priv->compression = compression;
}


FrCompression
fr_window_get_compression (FrWindow *window)
{
	return window->priv->compression;
}


void
fr_window_set_volume_size (FrWindow *window,
			   guint     volume_size)
{
	g_return_if_fail (window != NULL);

	window->priv->volume_size = volume_size;
}


guint
fr_window_get_volume_size (FrWindow *window)
{
	return window->priv->volume_size;
}


void
fr_window_go_to_location (FrWindow   *window,
			  const char *path,
			  gboolean    force_update)
{
	char *dir;

	g_return_if_fail (window != NULL);
	g_return_if_fail (path != NULL);

	if (force_update) {
		g_free (window->priv->last_location);
		window->priv->last_location = NULL;
	}

	if (path[strlen (path) - 1] != '/')
		dir = g_strconcat (path, "/", NULL);
	else
		dir = g_strdup (path);

	if ((window->priv->last_location == NULL) || (strcmp (window->priv->last_location, dir) != 0)) {
		g_free (window->priv->last_location);
		window->priv->last_location = dir;

		fr_window_history_add (window, dir);
		fr_window_update_file_list (window, TRUE);
		fr_window_update_current_location (window);
	}
	else
		g_free (dir);
}


const char *
fr_window_get_current_location (FrWindow *window)
{
	if (window->priv->history_current == NULL) {
		fr_window_history_add (window, "/");
		return window->priv->history_current->data;
	}
	else
		return (const char*) window->priv->history_current->data;
}


void
fr_window_go_up_one_level (FrWindow *window)
{
	char *parent_dir;

	g_return_if_fail (window != NULL);

	parent_dir = get_parent_dir (fr_window_get_current_location (window));
	fr_window_go_to_location (window, parent_dir, FALSE);
	g_free (parent_dir);
}


void
fr_window_go_back (FrWindow *window)
{
	g_return_if_fail (window != NULL);

	if (window->priv->history == NULL)
		return;
	if (window->priv->history_current == NULL)
		return;
	if (window->priv->history_current->next == NULL)
		return;
	window->priv->history_current = window->priv->history_current->next;

	fr_window_go_to_location (window, window->priv->history_current->data, FALSE);
}


void
fr_window_go_forward (FrWindow *window)
{
	g_return_if_fail (window != NULL);

	if (window->priv->history == NULL)
		return;
	if (window->priv->history_current == NULL)
		return;
	if (window->priv->history_current->prev == NULL)
		return;
	window->priv->history_current = window->priv->history_current->prev;

	fr_window_go_to_location (window, window->priv->history_current->data, FALSE);
}


void
fr_window_set_list_mode (FrWindow         *window,
			 FrWindowListMode  list_mode)
{
	g_return_if_fail (window != NULL);

	window->priv->list_mode = window->priv->last_list_mode = list_mode;
	if (window->priv->list_mode == FR_WINDOW_LIST_MODE_FLAT) {
		fr_window_history_clear (window);
		fr_window_history_add (window, "/");
	}

	preferences_set_list_mode (window->priv->last_list_mode);
	eel_mateconf_set_boolean (PREF_LIST_SHOW_PATH, (window->priv->list_mode == FR_WINDOW_LIST_MODE_FLAT));

	fr_window_update_file_list (window, TRUE);
	fr_window_update_dir_tree (window);
	fr_window_update_current_location (window);
}


GtkTreeModel *
fr_window_get_list_store (FrWindow *window)
{
	return GTK_TREE_MODEL (window->priv->list_store);
}


void
fr_window_find (FrWindow *window)
{
	window->priv->filter_mode = TRUE;
	gtk_widget_show (window->priv->filter_bar);
	gtk_widget_grab_focus (window->priv->filter_entry);
}


void
fr_window_select_all (FrWindow *window)
{
	gtk_tree_selection_select_all (gtk_tree_view_get_selection (GTK_TREE_VIEW (window->priv->list_view)));
}


void
fr_window_unselect_all (FrWindow *window)
{
	gtk_tree_selection_unselect_all (gtk_tree_view_get_selection (GTK_TREE_VIEW (window->priv->list_view)));
}


void
fr_window_set_sort_type (FrWindow     *window,
			 GtkSortType   sort_type)
{
	window->priv->sort_type = sort_type;
	fr_window_update_list_order (window);
}


void
fr_window_stop (FrWindow *window)
{
	if (! window->priv->stoppable)
		return;

	if (window->priv->activity_ref > 0)
		fr_archive_stop (window->archive);

	if (window->priv->convert_data.converting)
		fr_window_convert_data_free (window, TRUE);
}


/* -- start/stop activity mode -- */


static int
activity_cb (gpointer data)
{
	FrWindow *window = data;

	if ((window->priv->pd_progress_bar != NULL) && window->priv->progress_pulse)
		gtk_progress_bar_pulse (GTK_PROGRESS_BAR (window->priv->pd_progress_bar));
	if (window->priv->progress_pulse)
		gtk_progress_bar_pulse (GTK_PROGRESS_BAR (window->priv->progress_bar));

	return TRUE;
}


void
fr_window_start_activity_mode (FrWindow *window)
{
	g_return_if_fail (window != NULL);

	if (window->priv->activity_ref++ > 0)
		return;

	window->priv->activity_timeout_handle = g_timeout_add (ACTIVITY_DELAY,
							       activity_cb,
							       window);
	fr_window_update_sensitivity (window);
}


void
fr_window_stop_activity_mode (FrWindow *window)
{
	g_return_if_fail (window != NULL);

	if (window->priv->activity_ref == 0)
		return;

	window->priv->activity_ref--;

	if (window->priv->activity_ref > 0)
		return;

	if (window->priv->activity_timeout_handle == 0)
		return;

	g_source_remove (window->priv->activity_timeout_handle);
	window->priv->activity_timeout_handle = 0;

	if (! gtk_widget_get_realized (GTK_WIDGET (window)))
		return;

	if (window->priv->progress_dialog != NULL)
		gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (window->priv->pd_progress_bar), 0.0);

	if (! window->priv->batch_mode) {
		if (window->priv->progress_bar != NULL)
			gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (window->priv->progress_bar), 0.0);
		fr_window_update_sensitivity (window);
	}
}


static gboolean
last_output_window__unrealize_cb (GtkWidget  *widget,
				  gpointer    data)
{
	pref_util_save_window_geometry (GTK_WINDOW (widget), LAST_OUTPUT_DIALOG_NAME);
	return FALSE;
}


void
fr_window_view_last_output (FrWindow   *window,
			    const char *title)
{
	GtkWidget     *dialog;
	GtkWidget     *vbox;
	GtkWidget     *text_view;
	GtkWidget     *scrolled;
	GtkTextBuffer *text_buffer;
	GtkTextIter    iter;
	GList         *scan;

	if (title == NULL)
		title = _("Last Output");

	dialog = gtk_dialog_new_with_buttons (title,
					      GTK_WINDOW (window),
					      GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
					      GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
					      NULL);
	gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_CLOSE);

	gtk_window_set_resizable (GTK_WINDOW (dialog), TRUE);
	gtk_dialog_set_has_separator (GTK_DIALOG (dialog), FALSE);
	gtk_container_set_border_width (GTK_CONTAINER (dialog), 6);
	gtk_container_set_border_width (GTK_CONTAINER (gtk_dialog_get_content_area (GTK_DIALOG (dialog))), 6);
	gtk_box_set_spacing (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))), 8);

	gtk_widget_set_size_request (dialog, 500, 300);

	/* Add text */

	scrolled = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
					GTK_POLICY_AUTOMATIC,
					GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled),
					     GTK_SHADOW_ETCHED_IN);

	text_buffer = gtk_text_buffer_new (NULL);
	gtk_text_buffer_create_tag (text_buffer, "monospace",
				    "family", "monospace", NULL);

	text_view = gtk_text_view_new_with_buffer (text_buffer);
	g_object_unref (text_buffer);
	gtk_text_view_set_editable (GTK_TEXT_VIEW (text_view), FALSE);
	gtk_text_view_set_cursor_visible (GTK_TEXT_VIEW (text_view), FALSE);

	/**/

	vbox = gtk_vbox_new (FALSE, 6);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 5);

	gtk_container_add (GTK_CONTAINER (scrolled), text_view);
	gtk_box_pack_start (GTK_BOX (vbox), scrolled,
			    TRUE, TRUE, 0);

	gtk_widget_show_all (vbox);
	gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
			    vbox,
			    TRUE, TRUE, 0);

	/* signals */

	g_signal_connect (G_OBJECT (dialog),
			  "response",
			  G_CALLBACK (gtk_widget_destroy),
			  NULL);

	g_signal_connect (G_OBJECT (dialog),
			  "unrealize",
			  G_CALLBACK (last_output_window__unrealize_cb),
			  NULL);

	/**/

	gtk_text_buffer_get_iter_at_offset (text_buffer, &iter, 0);
	scan = window->archive->process->out.raw;
	for (; scan; scan = scan->next) {
		char        *line = scan->data;
		char        *utf8_line;
		gsize        bytes_written;

		utf8_line = g_locale_to_utf8 (line, -1, NULL, &bytes_written, NULL);
		gtk_text_buffer_insert_with_tags_by_name (text_buffer,
							  &iter,
							  utf8_line,
							  bytes_written,
							  "monospace", NULL);
		g_free (utf8_line);
		gtk_text_buffer_insert (text_buffer, &iter, "\n", 1);
	}

	/**/

	pref_util_restore_window_geometry (GTK_WINDOW (dialog), LAST_OUTPUT_DIALOG_NAME);
}


/* -- fr_window_rename_selection -- */


typedef struct {
	char     *path_to_rename;
	char     *old_name;
	char     *new_name;
	char     *current_dir;
	gboolean  is_dir;
	gboolean  dir_in_archive;
	char     *original_path;
} RenameData;


static RenameData*
rename_data_new (const char *path_to_rename,
		 const char *old_name,
		 const char *new_name,
		 const char *current_dir,
		 gboolean    is_dir,
		 gboolean    dir_in_archive,
		 const char *original_path)
{
	RenameData *rdata;

	rdata = g_new0 (RenameData, 1);
	rdata->path_to_rename = g_strdup (path_to_rename);
	if (old_name != NULL)
		rdata->old_name = g_strdup (old_name);
	if (new_name != NULL)
		rdata->new_name = g_strdup (new_name);
	if (current_dir != NULL)
		rdata->current_dir = g_strdup (current_dir);
	rdata->is_dir = is_dir;
	rdata->dir_in_archive = dir_in_archive;
	if (original_path != NULL)
		rdata->original_path = g_strdup (original_path);

	return rdata;
}


static void
rename_data_free (RenameData *rdata)
{
	g_return_if_fail (rdata != NULL);

	g_free (rdata->path_to_rename);
	g_free (rdata->old_name);
	g_free (rdata->new_name);
	g_free (rdata->current_dir);
	g_free (rdata->original_path);
	g_free (rdata);
}


static void
rename_selection (FrWindow   *window,
		  const char *path_to_rename,
		  const char *old_name,
		  const char *new_name,
		  const char *current_dir,
		  gboolean    is_dir,
		  gboolean    dir_in_archive,
		  const char *original_path)
{
	FrArchive  *archive = window->archive;
	RenameData *rdata;
	char       *tmp_dir;
	GList      *file_list;
	gboolean    added_dir;
	char       *new_dirname;
	GList      *new_file_list;
	GList      *scan;

	rdata = rename_data_new (path_to_rename,
				 old_name,
				 new_name,
				 current_dir,
				 is_dir,
				 dir_in_archive,
				 original_path);
	fr_window_set_current_batch_action (window,
					    FR_BATCH_ACTION_RENAME,
					    rdata,
					    (GFreeFunc) rename_data_free);

	fr_process_clear (archive->process);

	tmp_dir = get_temp_work_dir (NULL);

	if (is_dir)
		file_list = get_dir_list_from_path (window, rdata->path_to_rename);
	else
		file_list = g_list_append (NULL, g_strdup (rdata->path_to_rename));

	fr_archive_extract_to_local (archive,
				     file_list,
				     tmp_dir,
				     NULL,
				     FALSE,
				     TRUE,
				     FALSE,
				     window->priv->password);

	/* temporarily add the dir to rename to the list if it's stored in the
	 * archive, this way it will be removed from the archive... */
	added_dir = FALSE;
	if (is_dir && dir_in_archive && ! g_list_find_custom (file_list, original_path, (GCompareFunc) strcmp)) {
		file_list = g_list_prepend (file_list, g_strdup (original_path));
		added_dir = TRUE;
	}

	fr_archive_remove (archive, file_list, window->priv->compression);
	fr_window_clipboard_remove_file_list (window, file_list);

	/* ...and remove it from the list again */
	if (added_dir) {
		GList *tmp;

		tmp = file_list;
		file_list = g_list_remove_link (file_list, tmp);

		g_free (tmp->data);
		g_list_free (tmp);
	}

	/* rename the files. */

	new_dirname = g_build_filename (rdata->current_dir + 1, rdata->new_name, "/", NULL);
	new_file_list = NULL;
	if (rdata->is_dir) {
		char *old_path;
		char *new_path;

		old_path = g_build_filename (tmp_dir, rdata->current_dir, rdata->old_name, NULL);
		new_path = g_build_filename (tmp_dir, rdata->current_dir, rdata->new_name, NULL);

		fr_process_begin_command (archive->process, "mv");
		fr_process_add_arg (archive->process, "-f");
		fr_process_add_arg (archive->process, old_path);
		fr_process_add_arg (archive->process, new_path);
		fr_process_end_command (archive->process);

		g_free (old_path);
		g_free (new_path);
	}

	for (scan = file_list; scan; scan = scan->next) {
		const char *current_dir_relative = rdata->current_dir + 1;
		const char *filename = (char*) scan->data;
		char       *old_path = NULL, *common = NULL, *new_path = NULL;
		char       *new_filename;

		old_path = g_build_filename (tmp_dir, filename, NULL);

		if (strlen (filename) > (strlen (rdata->current_dir) + strlen (rdata->old_name)))
			common = g_strdup (filename + strlen (rdata->current_dir) + strlen (rdata->old_name));
		new_path = g_build_filename (tmp_dir, rdata->current_dir, rdata->new_name, common, NULL);

		if (! rdata->is_dir) {
			fr_process_begin_command (archive->process, "mv");
			fr_process_add_arg (archive->process, "-f");
			fr_process_add_arg (archive->process, old_path);
			fr_process_add_arg (archive->process, new_path);
			fr_process_end_command (archive->process);
		}

		new_filename = g_build_filename (current_dir_relative, rdata->new_name, common, NULL);
		new_file_list = g_list_prepend (new_file_list, new_filename);

		g_free (old_path);
		g_free (common);
		g_free (new_path);
	}
	new_file_list = g_list_reverse (new_file_list);

	/* FIXME: this is broken for tar archives.
	if (is_dir && dir_in_archive && ! g_list_find_custom (new_file_list, new_dirname, (GCompareFunc) strcmp))
		new_file_list = g_list_prepend (new_file_list, g_build_filename (rdata->current_dir + 1, rdata->new_name, NULL));
	*/

	fr_archive_add (archive,
			new_file_list,
			tmp_dir,
			NULL,
			FALSE,
			FALSE,
			window->priv->password,
			window->priv->encrypt_header,
			window->priv->compression,
			window->priv->volume_size);

	g_free (new_dirname);
	path_list_free (new_file_list);
	path_list_free (file_list);

	/* remove the tmp dir */

	fr_process_begin_command (archive->process, "rm");
	fr_process_set_working_dir (archive->process, g_get_tmp_dir ());
	fr_process_set_sticky (archive->process, TRUE);
	fr_process_add_arg (archive->process, "-rf");
	fr_process_add_arg (archive->process, tmp_dir);
	fr_process_end_command (archive->process);

	fr_process_start (archive->process);

	g_free (tmp_dir);
}


static gboolean
valid_name (const char  *new_name,
	    const char  *old_name,
	    char       **reason)
{
	char     *utf8_new_name;
	gboolean  retval = TRUE;

	new_name = eat_spaces (new_name);
	utf8_new_name = g_filename_display_name (new_name);

	if (*new_name == '\0') {
		/* Translators: the name references to a filename.  This message can appear when renaming a file. */
		*reason = g_strdup_printf ("%s\n\n%s", _("The new name is void."), _("Please use a different name."));
		retval = FALSE;
	}
	else if (strcmp (new_name, old_name) == 0) {
		/* Translators: the name references to a filename.  This message can appear when renaming a file. */
		*reason = g_strdup_printf ("%s\n\n%s", _("The new name is equal to the old one."), _("Please use a different name."));
		retval = FALSE;
	}
	else if (strchrs (new_name, BAD_CHARS)) {
		/* Translators: the name references to a filename.  This message can appear when renaming a file. */
		*reason = g_strdup_printf (_("The name \"%s\" is not valid because it cannot contain the characters: %s\n\n%s"), utf8_new_name, BAD_CHARS, _("Please use a different name."));
		retval = FALSE;
	}

	g_free (utf8_new_name);

	return retval;
}


static gboolean
name_is_present (FrWindow    *window,
		 const char  *current_dir,
		 const char  *new_name,
		 char       **reason)
{
	gboolean  retval = FALSE;
	int       i;
	char     *new_filename;
	int       new_filename_l;

	*reason = NULL;

	new_filename = g_build_filename (current_dir, new_name, NULL);
	new_filename_l = strlen (new_filename);

	for (i = 0; i < window->archive->command->files->len; i++) {
		FileData   *fdata = g_ptr_array_index (window->archive->command->files, i);
		const char *filename = fdata->full_path;

		if ((strncmp (filename, new_filename, new_filename_l) == 0)
		    && ((filename[new_filename_l] == '\0')
			|| (filename[new_filename_l] == G_DIR_SEPARATOR))) {
			char *utf8_name = g_filename_display_name (new_name);

			if (filename[new_filename_l] == G_DIR_SEPARATOR)
				*reason = g_strdup_printf (_("A folder named \"%s\" already exists.\n\n%s"), utf8_name, _("Please use a different name."));
			else
				*reason = g_strdup_printf (_("A file named \"%s\" already exists.\n\n%s"), utf8_name, _("Please use a different name."));

			retval = TRUE;
			break;
		}
	}

	g_free (new_filename);

	return retval;
}


void
fr_window_rename_selection (FrWindow *window,
			    gboolean  from_sidebar)
{
	char     *path_to_rename;
	char     *parent_dir;
	char     *old_name;
	gboolean  renaming_dir = FALSE;
	gboolean  dir_in_archive = FALSE;
	char     *original_path = NULL;
	char     *utf8_old_name;
	char     *utf8_new_name;

	if (from_sidebar) {
		path_to_rename = fr_window_get_selected_folder_in_tree_view (window);
		if (path_to_rename == NULL)
			return;
		parent_dir = remove_level_from_path (path_to_rename);
		old_name = g_strdup (file_name_from_path (path_to_rename));
		renaming_dir = TRUE;
	}
	else {
		FileData *selected_item;

		selected_item = fr_window_get_selected_item_from_file_list (window);
		if (selected_item == NULL)
			return;

		renaming_dir = file_data_is_dir (selected_item);
		dir_in_archive = selected_item->dir;
		original_path = g_strdup (selected_item->original_path);

		if (renaming_dir && ! dir_in_archive) {
			parent_dir = g_strdup (fr_window_get_current_location (window));
			old_name = g_strdup (selected_item->list_name);
			path_to_rename = g_build_filename (parent_dir, old_name, NULL);
		}
		else {
			if (renaming_dir) {
				path_to_rename = remove_ending_separator (selected_item->full_path);
				parent_dir = remove_level_from_path (path_to_rename);
			}
			else {
				path_to_rename = g_strdup (selected_item->original_path);
				parent_dir = remove_level_from_path (selected_item->full_path);
			}
			old_name = g_strdup (selected_item->name);
		}

		file_data_free (selected_item);
	}

 retry__rename_selection:
	utf8_old_name = g_locale_to_utf8 (old_name, -1 ,0 ,0 ,0);
	utf8_new_name = _gtk_request_dialog_run (GTK_WINDOW (window),
						 (GTK_DIALOG_DESTROY_WITH_PARENT
						  | GTK_DIALOG_MODAL),
						 _("Rename"),
						 (renaming_dir ? _("New folder name") : _("New file name")),
						 utf8_old_name,
						 1024,
						 GTK_STOCK_CANCEL,
						 _("_Rename"));
	g_free (utf8_old_name);

	if (utf8_new_name != NULL) {
		char *new_name;
		char *reason = NULL;

		new_name = g_filename_from_utf8 (utf8_new_name, -1, 0, 0, 0);
		g_free (utf8_new_name);

		if (! valid_name (new_name, old_name, &reason)) {
			char      *utf8_name = g_filename_display_name (new_name);
			GtkWidget *dlg;

			dlg = _gtk_error_dialog_new (GTK_WINDOW (window),
						     GTK_DIALOG_DESTROY_WITH_PARENT,
						     NULL,
						     (renaming_dir ? _("Could not rename the folder") : _("Could not rename the file")),
						     "%s",
						     reason);
			gtk_dialog_run (GTK_DIALOG (dlg));
			gtk_widget_destroy (dlg);

			g_free (reason);
			g_free (utf8_name);
			g_free (new_name);

			goto retry__rename_selection;
		}

		if (name_is_present (window, parent_dir, new_name, &reason)) {
			GtkWidget *dlg;
			int        r;

			dlg = _gtk_message_dialog_new (GTK_WINDOW (window),
						       GTK_DIALOG_MODAL,
						       GTK_STOCK_DIALOG_QUESTION,
						       (renaming_dir ? _("Could not rename the folder") : _("Could not rename the file")),
						       reason,
						       GTK_STOCK_CLOSE, GTK_RESPONSE_OK,
						       NULL);
			r = gtk_dialog_run (GTK_DIALOG (dlg));
			gtk_widget_destroy (dlg);
			g_free (reason);
			g_free (new_name);
			goto retry__rename_selection;
		}

		rename_selection (window,
				  path_to_rename,
				  old_name,
				  new_name,
				  parent_dir,
				  renaming_dir,
				  dir_in_archive,
				  original_path);

		g_free (new_name);
	}

	g_free (old_name);
	g_free (parent_dir);
	g_free (path_to_rename);
	g_free (original_path);
}


static void
fr_clipboard_get (GtkClipboard     *clipboard,
		  GtkSelectionData *selection_data,
		  guint             info,
		  gpointer          user_data_or_owner)
{
	FrWindow *window = user_data_or_owner;
	char     *data;

	if (gtk_selection_data_get_target (selection_data) != FR_SPECIAL_URI_LIST)
		return;

	data = get_selection_data_from_clipboard_data (window, window->priv->copy_data);
	gtk_selection_data_set (selection_data,
				gtk_selection_data_get_target (selection_data),
				8,
				(guchar *) data,
				strlen (data));
	g_free (data);
}


static void
fr_clipboard_clear (GtkClipboard *clipboard,
		    gpointer      user_data_or_owner)
{
	FrWindow *window = user_data_or_owner;

	if (window->priv->copy_data != NULL) {
		fr_clipboard_data_unref (window->priv->copy_data);
		window->priv->copy_data = NULL;
	}
}


GList *
fr_window_get_selection (FrWindow   *window,
		  	 gboolean    from_sidebar,
		  	 char      **return_base_dir)
{
	GList *files;
	char  *base_dir;

	if (from_sidebar) {
		char *selected_folder;
		char *parent_folder;

		files = fr_window_get_folder_tree_selection (window, TRUE, NULL);
		selected_folder = fr_window_get_selected_folder_in_tree_view (window);
		parent_folder = remove_level_from_path (selected_folder);
		if (parent_folder == NULL)
			base_dir = g_strdup ("/");
		else if (parent_folder[strlen (parent_folder) - 1] == '/')
			base_dir = g_strdup (parent_folder);
		else
			base_dir = g_strconcat (parent_folder, "/", NULL);
		g_free (selected_folder);
		g_free (parent_folder);
	}
	else {
		files = fr_window_get_file_list_selection (window, TRUE, NULL);
		base_dir = g_strdup (fr_window_get_current_location (window));
	}

	if (return_base_dir)
		*return_base_dir = base_dir;
	else
		g_free (base_dir);

	return files;
}


static void
fr_window_copy_or_cut_selection (FrWindow      *window,
				 FRClipboardOp  op,
			  	 gboolean       from_sidebar)
{
	GList        *files;
	char         *base_dir;
	GtkClipboard *clipboard;

	files = fr_window_get_selection (window, from_sidebar, &base_dir);

	if (window->priv->copy_data != NULL)
		fr_clipboard_data_unref (window->priv->copy_data);
	window->priv->copy_data = fr_clipboard_data_new ();
	window->priv->copy_data->files = files;
	window->priv->copy_data->op = op;
	window->priv->copy_data->base_dir = base_dir;

	clipboard = gtk_clipboard_get (FR_CLIPBOARD);
	gtk_clipboard_set_with_owner (clipboard,
				      clipboard_targets,
				      G_N_ELEMENTS (clipboard_targets),
				      fr_clipboard_get,
				      fr_clipboard_clear,
				      G_OBJECT (window));

	fr_window_update_sensitivity (window);
}


void
fr_window_copy_selection (FrWindow *window,
			  gboolean  from_sidebar)
{
	fr_window_copy_or_cut_selection (window, FR_CLIPBOARD_OP_COPY, from_sidebar);
}


void
fr_window_cut_selection (FrWindow *window,
			 gboolean  from_sidebar)
{
	fr_window_copy_or_cut_selection (window, FR_CLIPBOARD_OP_CUT, from_sidebar);
}


static gboolean
always_fake_load (FrArchive *archive,
		  gpointer   data)
{
	return TRUE;
}


static void
add_pasted_files (FrWindow        *window,
		  FrClipboardData *data)
{
	const char *current_dir_relative = data->current_dir + 1;
	GList      *scan;
	GList      *new_file_list = NULL;

	if (window->priv->password_for_paste != NULL) {
		g_free (window->priv->password_for_paste);
		window->priv->password_for_paste = NULL;
	}

	fr_process_clear (window->archive->process);
	for (scan = data->files; scan; scan = scan->next) {
		const char *old_name = (char*) scan->data;
		char       *new_name = g_build_filename (current_dir_relative, old_name + strlen (data->base_dir) - 1, NULL);

		/* skip folders */

		if ((strcmp (old_name, new_name) != 0)
		    && (old_name[strlen (old_name) - 1] != '/'))
		{
			fr_process_begin_command (window->archive->process, "mv");
			fr_process_set_working_dir (window->archive->process, data->tmp_dir);
			fr_process_add_arg (window->archive->process, "-f");
			fr_process_add_arg (window->archive->process, old_name);
			fr_process_add_arg (window->archive->process, new_name);
			fr_process_end_command (window->archive->process);
		}

		new_file_list = g_list_prepend (new_file_list, new_name);
	}

	fr_archive_add (window->archive,
			new_file_list,
			data->tmp_dir,
			NULL,
			FALSE,
			FALSE,
			window->priv->password,
			window->priv->encrypt_header,
			window->priv->compression,
			window->priv->volume_size);

	path_list_free (new_file_list);

	/* remove the tmp dir */

	fr_process_begin_command (window->archive->process, "rm");
	fr_process_set_working_dir (window->archive->process, g_get_tmp_dir ());
	fr_process_set_sticky (window->archive->process, TRUE);
	fr_process_add_arg (window->archive->process, "-rf");
	fr_process_add_arg (window->archive->process, data->tmp_dir);
	fr_process_end_command (window->archive->process);

	fr_process_start (window->archive->process);
}


static void
copy_from_archive_action_performed_cb (FrArchive   *archive,
				       FrAction     action,
				       FrProcError *error,
				       gpointer     data)
{
	FrWindow *window = data;
	gboolean  continue_batch = FALSE;

#ifdef DEBUG
	debug (DEBUG_INFO, "%s [DONE] (FR::Window)\n", action_names[action]);
#endif

	fr_window_stop_activity_mode (window);
	fr_window_pop_message (window);
	close_progress_dialog (window, FALSE);

	if (error->type == FR_PROC_ERROR_ASK_PASSWORD) {
		dlg_ask_password_for_paste_operation (window);
		return;
	}

	continue_batch = handle_errors (window, archive, action, error);

	if (error->type != FR_PROC_ERROR_NONE) {
		fr_clipboard_data_unref (window->priv->clipboard_data);
		window->priv->clipboard_data = NULL;
		return;
	}

	switch (action) {
	case FR_ACTION_LISTING_CONTENT:
		fr_process_clear (window->priv->copy_from_archive->process);
		fr_archive_extract_to_local (window->priv->copy_from_archive,
					     window->priv->clipboard_data->files,
					     window->priv->clipboard_data->tmp_dir,
					     NULL,
					     FALSE,
					     TRUE,
					     FALSE,
					     window->priv->clipboard_data->archive_password);
		fr_process_start (window->priv->copy_from_archive->process);
		break;

	case FR_ACTION_EXTRACTING_FILES:
		if (window->priv->clipboard_data->op == FR_CLIPBOARD_OP_CUT) {
			fr_process_clear (window->priv->copy_from_archive->process);
			fr_archive_remove (window->priv->copy_from_archive,
					   window->priv->clipboard_data->files,
					   window->priv->compression);
			fr_process_start (window->priv->copy_from_archive->process);
		}
		else
			add_pasted_files (window, window->priv->clipboard_data);
		break;

	case FR_ACTION_DELETING_FILES:
		add_pasted_files (window, window->priv->clipboard_data);
		break;

	default:
		break;
	}
}


static void
fr_window_paste_from_clipboard_data (FrWindow        *window,
				     FrClipboardData *data)
{
	const char *current_dir_relative;
	GHashTable *created_dirs;
	GList      *scan;

	if (window->priv->password_for_paste != NULL)
		fr_clipboard_data_set_password (data, window->priv->password_for_paste);

	if (window->priv->clipboard_data != data) {
		fr_clipboard_data_unref (window->priv->clipboard_data);
		window->priv->clipboard_data = data;
	}

	fr_window_set_current_batch_action (window,
					    FR_BATCH_ACTION_PASTE,
					    fr_clipboard_data_ref (data),
					    (GFreeFunc) fr_clipboard_data_unref);

	current_dir_relative = data->current_dir + 1;

	data->tmp_dir = get_temp_work_dir (NULL);
	created_dirs = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
	for (scan = data->files; scan; scan = scan->next) {
		const char *old_name = (char*) scan->data;
		char       *new_name = g_build_filename (current_dir_relative, old_name + strlen (data->base_dir) - 1, NULL);
		char       *dir = remove_level_from_path (new_name);

		if ((dir != NULL) && (g_hash_table_lookup (created_dirs, dir) == NULL)) {
			char *dir_path;

			dir_path = g_build_filename (data->tmp_dir, dir, NULL);
			debug (DEBUG_INFO, "mktree %s\n", dir_path);
			make_directory_tree_from_path (dir_path, 0700, NULL);

			g_free (dir_path);
			g_hash_table_replace (created_dirs, g_strdup (dir), "1");
		}

		g_free (dir);
		g_free (new_name);
	}
	g_hash_table_destroy (created_dirs);

	/**/

	if (window->priv->copy_from_archive == NULL) {
		window->priv->copy_from_archive = fr_archive_new ();
		g_signal_connect (G_OBJECT (window->priv->copy_from_archive),
				  "start",
				  G_CALLBACK (action_started),
				  window);
		g_signal_connect (G_OBJECT (window->priv->copy_from_archive),
				  "done",
				  G_CALLBACK (copy_from_archive_action_performed_cb),
				  window);
		g_signal_connect (G_OBJECT (window->priv->copy_from_archive),
				  "progress",
				  G_CALLBACK (fr_window_progress_cb),
				  window);
		g_signal_connect (G_OBJECT (window->priv->copy_from_archive),
				  "message",
				  G_CALLBACK (fr_window_message_cb),
				  window);
		g_signal_connect (G_OBJECT (window->priv->copy_from_archive),
				  "stoppable",
				  G_CALLBACK (fr_window_stoppable_cb),
				  window);
		fr_archive_set_fake_load_func (window->priv->copy_from_archive, always_fake_load, NULL);
	}
	fr_archive_load_local (window->priv->copy_from_archive,
			       data->archive_filename,
			       data->archive_password);
}


static void
fr_window_paste_selection_to (FrWindow   *window,
			      const char *current_dir)
{
	GtkClipboard     *clipboard;
	GtkSelectionData *selection_data;
	FrClipboardData  *paste_data;

	clipboard = gtk_clipboard_get (FR_CLIPBOARD);
	selection_data = gtk_clipboard_wait_for_contents (clipboard, FR_SPECIAL_URI_LIST);
	if (selection_data == NULL)
		return;

	paste_data = get_clipboard_data_from_selection_data (window, (char*) gtk_selection_data_get_data (selection_data));
	paste_data->current_dir = g_strdup (current_dir);
	fr_window_paste_from_clipboard_data (window, paste_data);

	gtk_selection_data_free (selection_data);
}


void
fr_window_paste_selection (FrWindow *window,
			   gboolean  from_sidebar)
{
	char *utf8_path, *utf8_old_path, *destination;
	char *current_dir;

	if (window->priv->list_mode == FR_WINDOW_LIST_MODE_FLAT)
		return;

	/**/

	utf8_old_path = g_filename_to_utf8 (fr_window_get_current_location (window), -1, NULL, NULL, NULL);
	utf8_path = _gtk_request_dialog_run (GTK_WINDOW (window),
					       (GTK_DIALOG_DESTROY_WITH_PARENT
						| GTK_DIALOG_MODAL),
					       _("Paste Selection"),
					       _("Destination folder"),
					       utf8_old_path,
					       1024,
					       GTK_STOCK_CANCEL,
					       GTK_STOCK_PASTE);
	g_free (utf8_old_path);
	if (utf8_path == NULL)
		return;

	destination = g_filename_from_utf8 (utf8_path, -1, NULL, NULL, NULL);
	g_free (utf8_path);

	if (destination[0] != '/')
		current_dir = build_uri (fr_window_get_current_location (window), destination, NULL);
	else
		current_dir = g_strdup (destination);
	g_free (destination);

	fr_window_paste_selection_to (window, current_dir);

	g_free (current_dir);
}


/* -- fr_window_open_files -- */


void
fr_window_open_files_with_command (FrWindow *window,
				   GList    *file_list,
				   char     *command)
{
	GAppInfo *app;
	GError   *error = NULL;

	app = g_app_info_create_from_commandline (command, NULL, G_APP_INFO_CREATE_NONE, &error);
	if (error != NULL) {
		_gtk_error_dialog_run (GTK_WINDOW (window),
				       _("Could not perform the operation"),
				       "%s",
				       error->message);
		g_clear_error (&error);
		return;
	}

	fr_window_open_files_with_application (window, file_list, app);
}


void
fr_window_open_files_with_application (FrWindow *window,
				       GList    *file_list,
				       GAppInfo *app)
{
	GList  *uris = NULL, *scan;
	GError *error = NULL;

	if (window->priv->activity_ref > 0)
		return;

	for (scan = file_list; scan; scan = scan->next)
		uris = g_list_prepend (uris, g_filename_to_uri (scan->data, NULL, NULL));

	if (! g_app_info_launch_uris (app, uris, NULL, &error)) {
		_gtk_error_dialog_run (GTK_WINDOW (window),
				       _("Could not perform the operation"),
				       "%s",
				       error->message);
		g_clear_error (&error);
	}
	else {
		char       *uri;
		const char *mime_type;

		uri = g_filename_to_uri (file_list->data, NULL, NULL);
		mime_type = get_file_mime_type (uri, FALSE);
		if (mime_type != NULL)
			g_app_info_set_as_default_for_type (app, mime_type, NULL);
		g_free (uri);
	}

	path_list_free (uris);
}


typedef struct {
	FrWindow    *window;
	GList       *file_list;
	gboolean     ask_application;
	CommandData *cdata;
} OpenFilesData;


static OpenFilesData*
open_files_data_new (FrWindow *window,
		     GList    *file_list,
		     gboolean  ask_application)

{
	OpenFilesData *odata;
	GList         *scan;

	odata = g_new0 (OpenFilesData, 1);
	odata->window = window;
	odata->file_list = path_list_dup (file_list);
	odata->ask_application = ask_application;
	odata->cdata = g_new0 (CommandData, 1);
	odata->cdata->temp_dir = get_temp_work_dir (NULL);
	odata->cdata->file_list = NULL;
	for (scan = file_list; scan; scan = scan->next) {
		char *file = scan->data;
		char *filename;

		filename = g_strconcat (odata->cdata->temp_dir,
					"/",
					file,
					NULL);
		odata->cdata->file_list = g_list_prepend (odata->cdata->file_list, filename);
	}

	/* Add to CommandList so the cdata is released on exit. */
	CommandList = g_list_prepend (CommandList, odata->cdata);

	return odata;
}


static void
open_files_data_free (OpenFilesData *odata)
{
	g_return_if_fail (odata != NULL);

	path_list_free (odata->file_list);
	g_free (odata);
}


void
fr_window_update_dialog_closed (FrWindow *window)
{
	window->priv->update_dialog = NULL;
}


gboolean
fr_window_update_files (FrWindow *window,
			GList    *file_list)
{
	GList *scan;

	if (window->priv->activity_ref > 0)
		return FALSE;

	if (window->archive->read_only)
		return FALSE;

	fr_process_clear (window->archive->process);

	for (scan = file_list; scan; scan = scan->next) {
		OpenFile *file = scan->data;
		GList    *local_file_list;

		local_file_list = g_list_append (NULL, file->path);
		fr_archive_add (window->archive,
				local_file_list,
				file->temp_dir,
				"/",
				FALSE,
				FALSE,
				window->priv->password,
				window->priv->encrypt_header,
				window->priv->compression,
				window->priv->volume_size);
		g_list_free (local_file_list);
	}

	fr_process_start (window->archive->process);

	return TRUE;
}


static void
open_file_modified_cb (GFileMonitor     *monitor,
		       GFile            *monitor_file,
		       GFile            *other_file,
		       GFileMonitorEvent event_type,
		       gpointer          user_data)
{
	FrWindow *window = user_data;
	char     *monitor_uri;
	OpenFile *file;
	GList    *scan;

	if ((event_type != G_FILE_MONITOR_EVENT_CHANGED)
	    && (event_type != G_FILE_MONITOR_EVENT_CREATED))
	{
		return;
	}

	monitor_uri = g_file_get_uri (monitor_file);
	file = NULL;
	for (scan = window->priv->open_files; scan; scan = scan->next) {
		OpenFile *test = scan->data;
		if (uricmp (test->extracted_uri, monitor_uri) == 0) {
			file = test;
			break;
		}
	}
	g_free (monitor_uri);

	g_return_if_fail (file != NULL);

	if (window->priv->update_dialog == NULL)
		window->priv->update_dialog = dlg_update (window);
	dlg_update_add_file (window->priv->update_dialog, file);
}


static void
fr_window_monitor_open_file (FrWindow *window,
			     OpenFile *file)
{
	GFile *f;

	window->priv->open_files = g_list_prepend (window->priv->open_files, file);
	f = g_file_new_for_uri (file->extracted_uri);
	file->monitor = g_file_monitor_file (f, 0, NULL, NULL);
	g_signal_connect (file->monitor,
			  "changed",
			  G_CALLBACK (open_file_modified_cb),
			  window);
	g_object_unref (f);
}


static void
monitor_extracted_files (OpenFilesData *odata)
{
	FrWindow *window = odata->window;
	GList    *scan1, *scan2;

	for (scan1 = odata->file_list, scan2 = odata->cdata->file_list;
	     scan1 && scan2;
	     scan1 = scan1->next, scan2 = scan2->next)
	{
		OpenFile   *ofile;
		const char *file = scan1->data;
		const char *extracted_path = scan2->data;

		ofile = open_file_new (file, extracted_path, odata->cdata->temp_dir);
		if (ofile != NULL)
			fr_window_monitor_open_file (window, ofile);
	}
}


static gboolean
fr_window_open_extracted_files (OpenFilesData *odata)
{
	GList      *file_list = odata->cdata->file_list;
	gboolean    result = FALSE;
	const char *first_file;
	const char *first_mime_type;
	GAppInfo   *app;
	GList      *files_to_open = NULL;
	GError     *error = NULL;

	g_return_val_if_fail (file_list != NULL, FALSE);

	first_file = (char*) file_list->data;
	if (first_file == NULL)
		return FALSE;

	if (! odata->window->archive->read_only)
		monitor_extracted_files (odata);

	if (odata->ask_application) {
		dlg_open_with (odata->window, file_list);
		return FALSE;
	}

	first_mime_type = get_file_mime_type_for_path (first_file, FALSE);
	app = g_app_info_get_default_for_type (first_mime_type, FALSE);

	if (app == NULL) {
		dlg_open_with (odata->window, file_list);
		return FALSE;
	}

	files_to_open = g_list_append (files_to_open, g_filename_to_uri (first_file, NULL, NULL));

	if (g_app_info_supports_files (app)) {
		GList *scan;

		for (scan = file_list->next; scan; scan = scan->next) {
			const char *path = scan->data;
			const char *mime_type;

			mime_type = get_file_mime_type_for_path (path, FALSE);
			if (mime_type == NULL)
				continue;

			if (strcmp (mime_type, first_mime_type) == 0) {
				files_to_open = g_list_append (files_to_open, g_filename_to_uri (path, NULL, NULL));
			}
			else {
				GAppInfo *app2;

				app2 = g_app_info_get_default_for_type (mime_type, FALSE);
				if (g_app_info_equal (app, app2))
					files_to_open = g_list_append (files_to_open, g_filename_to_uri (path, NULL, NULL));
				g_object_unref (app2);
			}
		}
	}

	result = g_app_info_launch_uris (app, files_to_open, NULL, &error);
	if (! result) {
		_gtk_error_dialog_run (GTK_WINDOW (odata->window),
				       _("Could not perform the operation"),
				       "%s",
				       error->message);
		g_clear_error (&error);
	}

	g_object_unref (app);
	path_list_free (files_to_open);

	return result;
}


static void
fr_window_open_files__extract_done_cb (FrArchive   *archive,
				       FrAction     action,
				       FrProcError *error,
				       gpointer     callback_data)
{
	OpenFilesData *odata = callback_data;

	g_signal_handlers_disconnect_matched (G_OBJECT (archive),
					      G_SIGNAL_MATCH_DATA,
					      0,
					      0, NULL,
					      0,
					      odata);

	if (error->type == FR_PROC_ERROR_NONE)
		fr_window_open_extracted_files (odata);
}


void
fr_window_open_files (FrWindow *window,
		      GList    *file_list,
		      gboolean  ask_application)
{
	OpenFilesData *odata;

	if (window->priv->activity_ref > 0)
		return;

	odata = open_files_data_new (window, file_list, ask_application);
	fr_window_set_current_batch_action (window,
					    FR_BATCH_ACTION_OPEN_FILES,
					    odata,
					    (GFreeFunc) open_files_data_free);

	g_signal_connect (G_OBJECT (window->archive),
			  "done",
			  G_CALLBACK (fr_window_open_files__extract_done_cb),
			  odata);

	fr_process_clear (window->archive->process);
	fr_archive_extract_to_local (window->archive,
				     odata->file_list,
				     odata->cdata->temp_dir,
				     NULL,
				     FALSE,
				     TRUE,
				     FALSE,
				     window->priv->password);
	fr_process_start (window->archive->process);
}


/**/


static char*
get_default_dir (const char *dir)
{
	if (! is_temp_dir (dir))
		return g_strdup (dir);
	else
		return NULL;
}


void
fr_window_set_open_default_dir (FrWindow   *window,
				const char *default_dir)
{
	g_return_if_fail (window != NULL);
	g_return_if_fail (default_dir != NULL);

	if (window->priv->open_default_dir != NULL)
		g_free (window->priv->open_default_dir);
	window->priv->open_default_dir = get_default_dir (default_dir);
}


const char *
fr_window_get_open_default_dir (FrWindow *window)
{
	if (window->priv->open_default_dir == NULL)
		return get_home_uri ();
	else
		return  window->priv->open_default_dir;
}


void
fr_window_set_add_default_dir (FrWindow   *window,
			       const char *default_dir)
{
	g_return_if_fail (window != NULL);
	g_return_if_fail (default_dir != NULL);

	if (window->priv->add_default_dir != NULL)
		g_free (window->priv->add_default_dir);
	window->priv->add_default_dir = get_default_dir (default_dir);
}


const char *
fr_window_get_add_default_dir (FrWindow *window)
{
	if (window->priv->add_default_dir == NULL)
		return get_home_uri ();
	else
		return  window->priv->add_default_dir;
}


void
fr_window_set_extract_default_dir (FrWindow   *window,
				   const char *default_dir,
				   gboolean    freeze)
{
	g_return_if_fail (window != NULL);
	g_return_if_fail (default_dir != NULL);

	/* do not change this dir while it's used by the non-interactive
	 * extraction operation. */
	if (window->priv->extract_interact_use_default_dir)
		return;

	window->priv->extract_interact_use_default_dir = freeze;

	if (window->priv->extract_default_dir != NULL)
		g_free (window->priv->extract_default_dir);
	window->priv->extract_default_dir = get_default_dir (default_dir);
}


const char *
fr_window_get_extract_default_dir (FrWindow *window)
{
	if (window->priv->extract_default_dir == NULL)
		return get_home_uri ();
	else
		return  window->priv->extract_default_dir;
}


void
fr_window_set_default_dir (FrWindow   *window,
			   const char *default_dir,
			   gboolean    freeze)
{
	g_return_if_fail (window != NULL);
	g_return_if_fail (default_dir != NULL);

	window->priv->freeze_default_dir = freeze;

	fr_window_set_open_default_dir (window, default_dir);
	fr_window_set_add_default_dir (window, default_dir);
	fr_window_set_extract_default_dir (window, default_dir, FALSE);
}


void
fr_window_update_columns_visibility (FrWindow *window)
{
	GtkTreeView       *tree_view = GTK_TREE_VIEW (window->priv->list_view);
	GtkTreeViewColumn *column;

	column = gtk_tree_view_get_column (tree_view, 1);
	gtk_tree_view_column_set_visible (column, eel_mateconf_get_boolean (PREF_LIST_SHOW_SIZE, TRUE));

	column = gtk_tree_view_get_column (tree_view, 2);
	gtk_tree_view_column_set_visible (column, eel_mateconf_get_boolean (PREF_LIST_SHOW_TYPE, TRUE));

	column = gtk_tree_view_get_column (tree_view, 3);
	gtk_tree_view_column_set_visible (column, eel_mateconf_get_boolean (PREF_LIST_SHOW_TIME, TRUE));

	column = gtk_tree_view_get_column (tree_view, 4);
	gtk_tree_view_column_set_visible (column, eel_mateconf_get_boolean (PREF_LIST_SHOW_PATH, TRUE));
}


void
fr_window_set_toolbar_visibility (FrWindow *window,
				  gboolean  visible)
{
	g_return_if_fail (window != NULL);

	if (visible)
		gtk_widget_show (window->priv->toolbar);
	else
		gtk_widget_hide (window->priv->toolbar);

	set_active (window, "ViewToolbar", visible);
}


void
fr_window_set_statusbar_visibility  (FrWindow *window,
				     gboolean  visible)
{
	g_return_if_fail (window != NULL);

	if (visible)
		gtk_widget_show (window->priv->statusbar);
	else
		gtk_widget_hide (window->priv->statusbar);

	set_active (window, "ViewStatusbar", visible);
}


void
fr_window_set_folders_visibility (FrWindow   *window,
				  gboolean    value)
{
	g_return_if_fail (window != NULL);

	window->priv->view_folders = value;
	fr_window_update_dir_tree (window);

	set_active (window, "ViewFolders", window->priv->view_folders);
}


/* -- batch mode procedures -- */


static void fr_window_exec_current_batch_action (FrWindow *window);


static void
fr_window_exec_batch_action (FrWindow      *window,
			     FRBatchAction *action)
{
	ExtractData   *edata;
	RenameData    *rdata;
	OpenFilesData *odata;
	SaveAsData    *sdata;

	switch (action->type) {
	case FR_BATCH_ACTION_LOAD:
		debug (DEBUG_INFO, "[BATCH] LOAD\n");

		if (! uri_exists ((char*) action->data))
			fr_window_archive_new (window, (char*) action->data);
		else
			fr_window_archive_open (window, (char*) action->data, GTK_WINDOW (window));
		break;

	case FR_BATCH_ACTION_ADD:
		debug (DEBUG_INFO, "[BATCH] ADD\n");

		fr_window_archive_add_dropped_items (window, (GList*) action->data, FALSE);
		break;

	case FR_BATCH_ACTION_OPEN:
		debug (DEBUG_INFO, "[BATCH] OPEN\n");

		fr_window_push_message (window, _("Add files to an archive"));
		dlg_batch_add_files (window, (GList*) action->data);
		break;

	case FR_BATCH_ACTION_EXTRACT:
		debug (DEBUG_INFO, "[BATCH] EXTRACT\n");

		edata = action->data;
		fr_window_archive_extract (window,
					   edata->file_list,
					   edata->extract_to_dir,
					   edata->base_dir,
					   edata->skip_older,
					   edata->overwrite,
					   edata->junk_paths,
					   TRUE);
		break;

	case FR_BATCH_ACTION_EXTRACT_HERE:
		debug (DEBUG_INFO, "[BATCH] EXTRACT HERE\n");

		edata = action->data;
		fr_window_archive_extract_here (window,
						FALSE,
						TRUE,
						FALSE);
		break;

	case FR_BATCH_ACTION_EXTRACT_INTERACT:
		debug (DEBUG_INFO, "[BATCH] EXTRACT_INTERACT\n");

		if (window->priv->extract_interact_use_default_dir
		    && (window->priv->extract_default_dir != NULL))
		{
			fr_window_archive_extract (window,
						   NULL,
						   window->priv->extract_default_dir,
						   NULL,
						   FALSE,
						   TRUE,
						   FALSE,
						   TRUE);
		}
		else {
			fr_window_push_message (window, _("Extract archive"));
			dlg_extract (NULL, window);
		}
		break;

	case FR_BATCH_ACTION_RENAME:
		debug (DEBUG_INFO, "[BATCH] RENAME\n");

		rdata = action->data;
		rename_selection (window,
				  rdata->path_to_rename,
				  rdata->old_name,
				  rdata->new_name,
				  rdata->current_dir,
				  rdata->is_dir,
				  rdata->dir_in_archive,
				  rdata->original_path);
		break;

	case FR_BATCH_ACTION_PASTE:
		debug (DEBUG_INFO, "[BATCH] PASTE\n");

		fr_window_paste_from_clipboard_data (window, (FrClipboardData*) action->data);
		break;

	case FR_BATCH_ACTION_OPEN_FILES:
		debug (DEBUG_INFO, "[BATCH] OPEN FILES\n");

		odata = action->data;
		fr_window_open_files (window, odata->file_list, odata->ask_application);
		break;

	case FR_BATCH_ACTION_SAVE_AS:
		debug (DEBUG_INFO, "[BATCH] SAVE_AS\n");

		sdata = action->data;
		fr_window_archive_save_as (window,
					   sdata->uri,
					   sdata->password,
					   sdata->encrypt_header,
					   sdata->volume_size);
		break;

	case FR_BATCH_ACTION_TEST:
		debug (DEBUG_INFO, "[BATCH] TEST\n");

		fr_window_archive_test (window);
		break;

	case FR_BATCH_ACTION_CLOSE:
		debug (DEBUG_INFO, "[BATCH] CLOSE\n");

		fr_window_archive_close (window);
		fr_window_exec_next_batch_action (window);
		break;

	case FR_BATCH_ACTION_QUIT:
		debug (DEBUG_INFO, "[BATCH] QUIT\n");

		gtk_widget_destroy (GTK_WIDGET (window));
		break;

	default:
		break;
	}
}


void
fr_window_reset_current_batch_action (FrWindow *window)
{
	FRBatchAction *adata = &window->priv->current_batch_action;

	if ((adata->data != NULL) && (adata->free_func != NULL))
		(*adata->free_func) (adata->data);
	adata->type = FR_BATCH_ACTION_NONE;
	adata->data = NULL;
	adata->free_func = NULL;
}


void
fr_window_set_current_batch_action (FrWindow          *window,
				    FrBatchActionType  action,
				    void              *data,
				    GFreeFunc          free_func)
{
	FRBatchAction *adata = &window->priv->current_batch_action;

	fr_window_reset_current_batch_action (window);

	adata->type = action;
	adata->data = data;
	adata->free_func = free_func;
}


void
fr_window_restart_current_batch_action (FrWindow *window)
{
	fr_window_exec_batch_action (window, &window->priv->current_batch_action);
}


void
fr_window_append_batch_action (FrWindow          *window,
			       FrBatchActionType  action,
			       void              *data,
			       GFreeFunc          free_func)
{
	FRBatchAction *a_desc;

	g_return_if_fail (window != NULL);

	a_desc = g_new0 (FRBatchAction, 1);
	a_desc->type = action;
	a_desc->data = data;
	a_desc->free_func = free_func;

	window->priv->batch_action_list = g_list_append (window->priv->batch_action_list, a_desc);
}


static void
fr_window_exec_current_batch_action (FrWindow *window)
{
	FRBatchAction *action;

	if (window->priv->batch_action == NULL) {
		window->priv->batch_mode = FALSE;
		return;
	}
	action = (FRBatchAction *) window->priv->batch_action->data;
	fr_window_exec_batch_action (window, action);
}


static void
fr_window_exec_next_batch_action (FrWindow *window)
{
	if (window->priv->batch_action != NULL)
		window->priv->batch_action = g_list_next (window->priv->batch_action);
	else
		window->priv->batch_action = window->priv->batch_action_list;
	fr_window_exec_current_batch_action (window);
}


void
fr_window_start_batch (FrWindow *window)
{
	g_return_if_fail (window != NULL);

	if (window->priv->batch_mode)
		return;

	if (window->priv->batch_action_list == NULL)
		return;

	window->priv->batch_mode = TRUE;
	window->priv->batch_action = window->priv->batch_action_list;
	window->archive->can_create_compressed_file = window->priv->batch_adding_one_file;

	fr_window_exec_current_batch_action (window);
}


void
fr_window_stop_batch (FrWindow *window)
{
	if (! window->priv->non_interactive)
		return;

	window->priv->extract_interact_use_default_dir = FALSE;
	window->archive->can_create_compressed_file = FALSE;

	if (window->priv->batch_mode) {
		if (! window->priv->showing_error_dialog) {
			gtk_widget_destroy (GTK_WIDGET (window));
			return;
		}
	}
	else {
		gtk_window_present (GTK_WINDOW (window));
		fr_window_archive_close (window);
	}

	window->priv->batch_mode = FALSE;
}


void
fr_window_resume_batch (FrWindow *window)
{
	fr_window_exec_current_batch_action (window);
}


gboolean
fr_window_is_batch_mode (FrWindow *window)
{
	return window->priv->batch_mode;
}


void
fr_window_new_batch (FrWindow *window)
{
	fr_window_free_batch_data (window);
	window->priv->non_interactive = TRUE;
}


void
fr_window_set_batch__extract_here (FrWindow   *window,
				   const char *filename)
{
	g_return_if_fail (window != NULL);
	g_return_if_fail (filename != NULL);

	fr_window_append_batch_action (window,
				       FR_BATCH_ACTION_LOAD,
				       g_strdup (filename),
				       (GFreeFunc) g_free);
	fr_window_append_batch_action (window,
				       FR_BATCH_ACTION_EXTRACT_HERE,
				       extract_to_data_new (NULL),
				       (GFreeFunc) extract_data_free);
	fr_window_append_batch_action (window,
				       FR_BATCH_ACTION_CLOSE,
				       NULL,
				       NULL);
}


void
fr_window_set_batch__extract (FrWindow   *window,
			      const char *filename,
			      const char *dest_dir)
{
	g_return_if_fail (window != NULL);
	g_return_if_fail (filename != NULL);

	fr_window_append_batch_action (window,
				       FR_BATCH_ACTION_LOAD,
				       g_strdup (filename),
				       (GFreeFunc) g_free);
	if (dest_dir != NULL)
		fr_window_append_batch_action (window,
					       FR_BATCH_ACTION_EXTRACT,
					       extract_to_data_new (dest_dir),
					       (GFreeFunc) extract_data_free);
	else
		fr_window_append_batch_action (window,
					       FR_BATCH_ACTION_EXTRACT_INTERACT,
					       NULL,
					       NULL);
	fr_window_append_batch_action (window,
				       FR_BATCH_ACTION_CLOSE,
				       NULL,
				       NULL);
}


void
fr_window_set_batch__add (FrWindow   *window,
			  const char *archive,
			  GList      *file_list)
{
	window->priv->batch_adding_one_file = (file_list->next == NULL) && (uri_is_file (file_list->data));

	if (archive != NULL)
		fr_window_append_batch_action (window,
					       FR_BATCH_ACTION_LOAD,
					       g_strdup (archive),
					       (GFreeFunc) g_free);
	else
		fr_window_append_batch_action (window,
					       FR_BATCH_ACTION_OPEN,
					       file_list,
					       NULL);
	fr_window_append_batch_action (window,
				       FR_BATCH_ACTION_ADD,
				       file_list,
				       NULL);
	fr_window_append_batch_action (window,
				       FR_BATCH_ACTION_CLOSE,
				       NULL,
				       NULL);
}

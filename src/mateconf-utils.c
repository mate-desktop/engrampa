/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  File-Roller
 *
 *  Copyright (C) 2001, 2002, 2003 Free Software Foundation, Inc.
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

/* eel-mateconf-extensions.c - Stuff to make MateConf easier to use.

   Copyright (C) 2000, 2001 Eazel, Inc.

   The Mate Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Mate Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Mate Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.

   Authors: Ramiro Estrugo <ramiro@eazel.com>
*/

/* Modified by Paolo Bacchilega <paolo.bacch@tin.it> for File Roller. */

#include <config.h>
#include <string.h>
#include <errno.h>

#include <mateconf/mateconf-client.h>
#include <mateconf/mateconf.h>
#include "mateconf-utils.h"
#include "gtk-utils.h"
#include "fr-error.h"

static MateConfClient *global_mateconf_client = NULL;


void
eel_global_client_free (void)
{
	if (global_mateconf_client == NULL) {
		return;
	}

	g_object_unref (global_mateconf_client);
	global_mateconf_client = NULL;
}


MateConfClient *
eel_mateconf_client_get_global (void)
{
	/* Initialize mateconf if needed */
	if (!mateconf_is_initialized ()) {
		char   *argv[] = { "eel-preferences", NULL };
		GError *error = NULL;

		if (!mateconf_init (1, argv, &error)) {
			if (eel_mateconf_handle_error (&error)) {
				return NULL;
			}
		}
	}

	if (global_mateconf_client == NULL)
		global_mateconf_client = mateconf_client_get_default ();

	return global_mateconf_client;
}


gboolean
eel_mateconf_handle_error (GError **error)
{
	static gboolean shown_dialog = FALSE;

	g_return_val_if_fail (error != NULL, FALSE);

	if (*error != NULL) {
		g_warning ("MateConf error:\n  %s", (*error)->message);
		if (! shown_dialog) {
			GtkWidget *d;
			shown_dialog = TRUE;
			d = _gtk_error_dialog_new (NULL,
						   GTK_DIALOG_DESTROY_WITH_PARENT,
						   NULL,
						   "MateConf error",
						   "MateConf error: %s\n"
						   "All further errors "
						   "shown only on terminal",
						   (*error)->message);
			g_signal_connect (d, "response",
					  G_CALLBACK (gtk_widget_destroy), 
					  NULL);

			gtk_widget_show (d);
		}

		g_error_free (*error);
		*error = NULL;

		return TRUE;
	}

	return FALSE;
}


static gboolean
check_type (const char      *key,
	    MateConfValue      *val, 
	    MateConfValueType   t, 
	    GError         **err)
{
	if (val->type != t) {
		g_set_error (err,
			     FR_ERROR,
			     errno,
			     "Type mismatch for key %s",
			     key);
		return FALSE;
	} else
		return TRUE;
}


void
eel_mateconf_set_boolean (const char *key,
		       gboolean    boolean_value)
{
	MateConfClient *client;
	GError      *error = NULL;

	g_return_if_fail (key != NULL);

	client = eel_mateconf_client_get_global ();
	g_return_if_fail (client != NULL);

	mateconf_client_set_bool (client, key, boolean_value, &error);
	eel_mateconf_handle_error (&error);
}


gboolean
eel_mateconf_get_boolean (const char *key,
		       gboolean    def)
{
	GError      *error = NULL;
	gboolean     result = def;
	MateConfClient *client;
	MateConfValue  *val;

	g_return_val_if_fail (key != NULL, def);

	client = eel_mateconf_client_get_global ();
	g_return_val_if_fail (client != NULL, def);

	val = mateconf_client_get (client, key, &error);

	if (val != NULL) {
		if (check_type (key, val, MATECONF_VALUE_BOOL, &error))
			result = mateconf_value_get_bool (val);
		else
			eel_mateconf_handle_error (&error);
		mateconf_value_free (val);

	} else if (error != NULL)
		eel_mateconf_handle_error (&error);

	return result;
}


void
eel_mateconf_set_integer (const char *key,
		       int         int_value)
{
	MateConfClient *client;
	GError      *error = NULL;

	g_return_if_fail (key != NULL);

	client = eel_mateconf_client_get_global ();
	g_return_if_fail (client != NULL);

	mateconf_client_set_int (client, key, int_value, &error);
	eel_mateconf_handle_error (&error);
}


int
eel_mateconf_get_integer (const char *key,
		       int         def)
{
	GError      *error = NULL;
	int          result = def;
	MateConfClient *client;
	MateConfValue  *val;

	g_return_val_if_fail (key != NULL, def);

	client = eel_mateconf_client_get_global ();
	g_return_val_if_fail (client != NULL, def);

	val = mateconf_client_get (client, key, &error);

	if (val != NULL) {
		if (check_type (key, val, MATECONF_VALUE_INT, &error))
			result = mateconf_value_get_int (val);
		else
			eel_mateconf_handle_error (&error);
		mateconf_value_free (val);

	} else if (error != NULL)
		eel_mateconf_handle_error (&error);

	return result;
}


void
eel_mateconf_set_float (const char *key,
		     float       float_value)
{
	MateConfClient *client;
	GError *error = NULL;

	g_return_if_fail (key != NULL);

	client = eel_mateconf_client_get_global ();
	g_return_if_fail (client != NULL);

	mateconf_client_set_float (client, key, float_value, &error);
	eel_mateconf_handle_error (&error);
}


float
eel_mateconf_get_float (const char *key,
		     float       def)
{
	GError      *error = NULL;
	float        result = def;
	MateConfClient *client;
	MateConfValue  *val;

	g_return_val_if_fail (key != NULL, def);

	client = eel_mateconf_client_get_global ();
	g_return_val_if_fail (client != NULL, def);

	val = mateconf_client_get (client, key, &error);

	if (val != NULL) {
		if (check_type (key, val, MATECONF_VALUE_FLOAT, &error))
			result = mateconf_value_get_float (val);
		else
			eel_mateconf_handle_error (&error);
		mateconf_value_free (val);

	} else if (error != NULL)
		eel_mateconf_handle_error (&error);

	return result;
}


void
eel_mateconf_set_string (const char *key,
		      const char *string_value)
{
	MateConfClient *client;
	GError *error = NULL;

	g_return_if_fail (key != NULL);

	client = eel_mateconf_client_get_global ();
	g_return_if_fail (client != NULL);

	mateconf_client_set_string (client, key, string_value, &error);
	eel_mateconf_handle_error (&error);
}


char *
eel_mateconf_get_string (const char *key,
		      const char *def)
{
	GError      *error = NULL;
	char        *result;
	MateConfClient *client;
	char        *val;

	if (def != NULL)
		result = g_strdup (def);
	else
		result = NULL;

	g_return_val_if_fail (key != NULL, result);

	client = eel_mateconf_client_get_global ();
	g_return_val_if_fail (client != NULL, result);

	val = mateconf_client_get_string (client, key, &error);

	if (val != NULL) {
		g_return_val_if_fail (error == NULL, result);
		g_free (result);
		result = g_strdup (val);
		g_free (val);
	} else if (error != NULL)
		eel_mateconf_handle_error (&error);

	return result;
}


void
eel_mateconf_set_locale_string (const char *key,
			     const char *string_value)
{
	char *utf8;

	utf8 = g_locale_to_utf8 (string_value, -1, 0, 0, 0);

	if (utf8 != NULL) {
		eel_mateconf_set_string (key, utf8);
		g_free (utf8);
	}
}


char *
eel_mateconf_get_locale_string (const char *key,
			     const char *def)
{
	char *utf8;
	char *result;

	utf8 = eel_mateconf_get_string (key, def);

	if (utf8 == NULL)
		return NULL;

	result = g_locale_from_utf8 (utf8, -1, 0, 0, 0);
	g_free (utf8);

	return result;
}


void
eel_mateconf_set_string_list (const char *key,
			   const GSList *slist)
{
	MateConfClient *client;
	GError *error;

	g_return_if_fail (key != NULL);

	client = eel_mateconf_client_get_global ();
	g_return_if_fail (client != NULL);

	error = NULL;
	mateconf_client_set_list (client, key, MATECONF_VALUE_STRING,
			       /* Need cast cause of MateConf api bug */
			       (GSList *) slist,
			       &error);
	eel_mateconf_handle_error (&error);
}


GSList *
eel_mateconf_get_string_list (const char *key)
{
	GSList *slist;
	MateConfClient *client;
	GError *error;

	g_return_val_if_fail (key != NULL, NULL);

	client = eel_mateconf_client_get_global ();
	g_return_val_if_fail (client != NULL, NULL);

	error = NULL;
	slist = mateconf_client_get_list (client, key, MATECONF_VALUE_STRING, &error);
	if (eel_mateconf_handle_error (&error)) {
		slist = NULL;
	}

	return slist;
}


GSList *
eel_mateconf_get_locale_string_list (const char *key)
{
	GSList *utf8_slist, *slist, *scan;

	utf8_slist = eel_mateconf_get_string_list (key);

	slist = NULL;
	for (scan = utf8_slist; scan; scan = scan->next) {
		char *utf8 = scan->data;
		char *locale = g_locale_from_utf8 (utf8, -1, 0, 0, 0);
		slist = g_slist_prepend (slist, locale);
	}

	g_slist_foreach (utf8_slist, (GFunc) g_free, NULL);
	g_slist_free (utf8_slist);

	return g_slist_reverse (slist);
}


void
eel_mateconf_set_locale_string_list (const char   *key,
				  const GSList *string_list_value)
{
	GSList       *utf8_slist;
	const GSList *scan;

	utf8_slist = NULL;
	for (scan = string_list_value; scan; scan = scan->next) {
		char *locale = scan->data;
		char *utf8 = g_locale_to_utf8 (locale, -1, 0, 0, 0);
		utf8_slist = g_slist_prepend (utf8_slist, utf8);
	}

	utf8_slist = g_slist_reverse (utf8_slist);

	eel_mateconf_set_string_list (key, utf8_slist);

	g_slist_foreach (utf8_slist, (GFunc) g_free, NULL);
	g_slist_free (utf8_slist);
}


gboolean
eel_mateconf_is_default (const char *key)
{
	gboolean result;
	MateConfValue *value;
	GError *error = NULL;

	g_return_val_if_fail (key != NULL, FALSE);

	value = mateconf_client_get_without_default  (eel_mateconf_client_get_global (), key, &error);

	if (eel_mateconf_handle_error (&error)) {
		if (value != NULL) {
			mateconf_value_free (value);
		}
		return FALSE;
	}

	result = (value == NULL);
	eel_mateconf_value_free (value);
	return result;
}


gboolean
eel_mateconf_monitor_add (const char *directory)
{
	GError *error = NULL;
	MateConfClient *client;

	g_return_val_if_fail (directory != NULL, FALSE);

	client = mateconf_client_get_default ();
	g_return_val_if_fail (client != NULL, FALSE);

	mateconf_client_add_dir (client,
			      directory,
			      MATECONF_CLIENT_PRELOAD_NONE,
			      &error);

	if (eel_mateconf_handle_error (&error)) {
		return FALSE;
	}

	return TRUE;
}


gboolean
eel_mateconf_monitor_remove (const char *directory)
{
	GError *error = NULL;
	MateConfClient *client;

	if (directory == NULL) {
		return FALSE;
	}

	client = mateconf_client_get_default ();
	g_return_val_if_fail (client != NULL, FALSE);

	mateconf_client_remove_dir (client,
				 directory,
				 &error);

	if (eel_mateconf_handle_error (&error)) {
		return FALSE;
	}

	return TRUE;
}


void
eel_mateconf_preload_cache (const char             *directory,
			 MateConfClientPreloadType  preload_type)
{
	GError *error = NULL;
	MateConfClient *client;

	if (directory == NULL) {
		return;
	}

	client = mateconf_client_get_default ();
	g_return_if_fail (client != NULL);

	mateconf_client_preload (client,
			      directory,
			      preload_type,
			      &error);

	eel_mateconf_handle_error (&error);
}


void
eel_mateconf_suggest_sync (void)
{
	MateConfClient *client;
	GError *error = NULL;

	client = eel_mateconf_client_get_global ();
	g_return_if_fail (client != NULL);

	mateconf_client_suggest_sync (client, &error);
	eel_mateconf_handle_error (&error);
}


MateConfValue*
eel_mateconf_get_value (const char *key)
{
	MateConfValue *value = NULL;
	MateConfClient *client;
	GError *error = NULL;

	g_return_val_if_fail (key != NULL, NULL);

	client = eel_mateconf_client_get_global ();
	g_return_val_if_fail (client != NULL, NULL);

	value = mateconf_client_get (client, key, &error);

	if (eel_mateconf_handle_error (&error)) {
		if (value != NULL) {
			mateconf_value_free (value);
			value = NULL;
		}
	}

	return value;
}


MateConfValue*
eel_mateconf_get_default_value (const char *key)
{
	MateConfValue *value = NULL;
	MateConfClient *client;
	GError *error = NULL;

	g_return_val_if_fail (key != NULL, NULL);

	client = eel_mateconf_client_get_global ();
	g_return_val_if_fail (client != NULL, NULL);

	value = mateconf_client_get_default_from_schema (client, key, &error);

	if (eel_mateconf_handle_error (&error)) {
		if (value != NULL) {
			mateconf_value_free (value);
			value = NULL;
		}
	}

	return value;
}


static int
eel_strcmp (const char *string_a, const char *string_b)
{
	/* FIXME bugzilla.eazel.com 5450: Maybe we need to make this
	 * treat 'NULL < ""', or have a flavor that does that. If we
	 * didn't have code that already relies on 'NULL == ""', I
	 * would change it right now.
	 */
	return strcmp (string_a == NULL ? "" : string_a,
		       string_b == NULL ? "" : string_b);
}


static gboolean
eel_str_is_equal (const char *string_a, const char *string_b)
{
	/* FIXME bugzilla.eazel.com 5450: Maybe we need to make this
	 * treat 'NULL != ""', or have a flavor that does that. If we
	 * didn't have code that already relies on 'NULL == ""', I
	 * would change it right now.
	 */
	return eel_strcmp (string_a, string_b) == 0;
}


static gboolean
simple_value_is_equal (const MateConfValue *a,
		       const MateConfValue *b)
{
	g_return_val_if_fail (a != NULL, FALSE);
	g_return_val_if_fail (b != NULL, FALSE);

	switch (a->type) {
	case MATECONF_VALUE_STRING:
		return eel_str_is_equal (mateconf_value_get_string (a),
					 mateconf_value_get_string (b));
		break;

	case MATECONF_VALUE_INT:
		return mateconf_value_get_int (a) ==
			mateconf_value_get_int (b);
		break;

	case MATECONF_VALUE_FLOAT:
		return mateconf_value_get_float (a) ==
			mateconf_value_get_float (b);
		break;

	case MATECONF_VALUE_BOOL:
		return mateconf_value_get_bool (a) ==
			mateconf_value_get_bool (b);
		break;
	default:
		g_assert_not_reached ();
		break;
	}

	return FALSE;
}


gboolean
eel_mateconf_value_is_equal (const MateConfValue *a,
			  const MateConfValue *b)
{
	GSList *node_a;
	GSList *node_b;

	if (a == NULL && b == NULL) {
		return TRUE;
	}

	if (a == NULL || b == NULL) {
		return FALSE;
	}

	if (a->type != b->type) {
		return FALSE;
	}

	switch (a->type) {
	case MATECONF_VALUE_STRING:
	case MATECONF_VALUE_INT:
	case MATECONF_VALUE_FLOAT:
	case MATECONF_VALUE_BOOL:
		return simple_value_is_equal (a, b);
		break;

	case MATECONF_VALUE_LIST:
		if (mateconf_value_get_list_type (a) !=
		    mateconf_value_get_list_type (b)) {
			return FALSE;
		}

		node_a = mateconf_value_get_list (a);
		node_b = mateconf_value_get_list (b);

		if (node_a == NULL && node_b == NULL) {
			return TRUE;
		}

		if (g_slist_length (node_a) !=
		    g_slist_length (node_b)) {
			return FALSE;
		}

		for (;
		     node_a != NULL && node_b != NULL;
		     node_a = node_a->next, node_b = node_b->next) {
			g_assert (node_a->data != NULL);
			g_assert (node_b->data != NULL);
			if (!simple_value_is_equal (node_a->data, node_b->data)) {
				return FALSE;
			}
		}

		return TRUE;
	default:
		/* FIXME: pair ? */
		g_assert (0);
		break;
	}

	g_assert_not_reached ();
	return FALSE;
}


void
eel_mateconf_value_free (MateConfValue *value)
{
	if (value == NULL) {
		return;
	}

	mateconf_value_free (value);
}


guint
eel_mateconf_notification_add (const char *key,
			    MateConfClientNotifyFunc notification_callback,
			    gpointer callback_data)
{
	guint notification_id;
	MateConfClient *client;
	GError *error = NULL;

	g_return_val_if_fail (key != NULL, EEL_MATECONF_UNDEFINED_CONNECTION);
	g_return_val_if_fail (notification_callback != NULL, EEL_MATECONF_UNDEFINED_CONNECTION);

	client = eel_mateconf_client_get_global ();
	g_return_val_if_fail (client != NULL, EEL_MATECONF_UNDEFINED_CONNECTION);

	notification_id = mateconf_client_notify_add (client,
						   key,
						   notification_callback,
						   callback_data,
						   NULL,
						   &error);

	if (eel_mateconf_handle_error (&error)) {
		if (notification_id != EEL_MATECONF_UNDEFINED_CONNECTION) {
			mateconf_client_notify_remove (client, notification_id);
			notification_id = EEL_MATECONF_UNDEFINED_CONNECTION;
		}
	}

	return notification_id;
}


void
eel_mateconf_notification_remove (guint notification_id)
{
	MateConfClient *client;

	if (notification_id == EEL_MATECONF_UNDEFINED_CONNECTION) {
		return;
	}

	client = eel_mateconf_client_get_global ();
	g_return_if_fail (client != NULL);

	mateconf_client_notify_remove (client, notification_id);
}


GSList *
eel_mateconf_value_get_string_list (const MateConfValue *value)
{
 	GSList *result;
 	const GSList *slist;
 	const GSList *node;
	const char *string;
	const MateConfValue *next_value;

	if (value == NULL) {
		return NULL;
	}

	g_return_val_if_fail (value->type == MATECONF_VALUE_LIST, NULL);
	g_return_val_if_fail (mateconf_value_get_list_type (value) == MATECONF_VALUE_STRING, NULL);

	slist = mateconf_value_get_list (value);
	result = NULL;
	for (node = slist; node != NULL; node = node->next) {
		next_value = node->data;
		g_return_val_if_fail (next_value != NULL, NULL);
		g_return_val_if_fail (next_value->type == MATECONF_VALUE_STRING, NULL);
		string = mateconf_value_get_string (next_value);
		result = g_slist_append (result, g_strdup (string));
	}
	return result;
}


void
eel_mateconf_value_set_string_list (MateConfValue *value,
				 const GSList *string_list)
{
 	const GSList *node;
	MateConfValue *next_value;
 	GSList *value_list;

	g_return_if_fail (value->type == MATECONF_VALUE_LIST);
	g_return_if_fail (mateconf_value_get_list_type (value) == MATECONF_VALUE_STRING);

	value_list = NULL;
	for (node = string_list; node != NULL; node = node->next) {
		next_value = mateconf_value_new (MATECONF_VALUE_STRING);
		mateconf_value_set_string (next_value, node->data);
		value_list = g_slist_append (value_list, next_value);
	}

	mateconf_value_set_list (value, value_list);

	for (node = value_list; node != NULL; node = node->next) {
		mateconf_value_free (node->data);
	}
	g_slist_free (value_list);
}

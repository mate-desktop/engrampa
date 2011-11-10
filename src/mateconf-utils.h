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

/* eel-mateconf-extensions.h - Stuff to make MateConf easier to use.

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

#ifndef MATECONF_UTILS_H
#define MATECONF_UTILS_H

#include <glib.h>
#include <mateconf/mateconf.h>
#include <mateconf/mateconf-client.h>

G_BEGIN_DECLS

#define EEL_MATECONF_UNDEFINED_CONNECTION 0

MateConfClient *eel_mateconf_client_get_global     (void);

void         eel_global_client_free          (void);

gboolean     eel_mateconf_handle_error          (GError                **error);

gboolean     eel_mateconf_get_boolean           (const char             *key,
					      gboolean                def_val);

void         eel_mateconf_set_boolean           (const char             *key,
					      gboolean                value);

int          eel_mateconf_get_integer           (const char             *key,
					      int                     def_val);

void         eel_mateconf_set_integer           (const char             *key,
					      int                     value);

float        eel_mateconf_get_float             (const char             *key,
					      float                   def_val);

void         eel_mateconf_set_float             (const char             *key,
					      float                   value);

char *       eel_mateconf_get_string            (const char             *key,
					      const char             *def_val);

void         eel_mateconf_set_string            (const char             *key,
					      const char             *value);

char *       eel_mateconf_get_locale_string     (const char             *key,
					      const char             *def_val);

void         eel_mateconf_set_locale_string     (const char             *key,
					      const char             *value);

GSList *     eel_mateconf_get_string_list       (const char             *key);

void         eel_mateconf_set_string_list       (const char             *key,
					      const GSList           *string_list_value);

GSList *     eel_mateconf_get_locale_string_list(const char             *key);

void         eel_mateconf_set_locale_string_list(const char             *key,
					      const GSList           *string_list_value);

gboolean     eel_mateconf_is_default            (const char             *key);

gboolean     eel_mateconf_monitor_add           (const char             *directory);

gboolean     eel_mateconf_monitor_remove        (const char             *directory);

void         eel_mateconf_preload_cache         (const char             *directory,
					      MateConfClientPreloadType  preload_type);

void         eel_mateconf_suggest_sync          (void);

MateConfValue*  eel_mateconf_get_value             (const char             *key);

MateConfValue*  eel_mateconf_get_default_value     (const char             *key);

gboolean     eel_mateconf_value_is_equal        (const MateConfValue       *a,
					      const MateConfValue       *b);

void         eel_mateconf_value_free            (MateConfValue             *value);

guint        eel_mateconf_notification_add      (const char             *key,
					      MateConfClientNotifyFunc   notification_callback,
					      gpointer                callback_data);

void         eel_mateconf_notification_remove   (guint                   notification_id);

GSList *     eel_mateconf_value_get_string_list (const MateConfValue       *value);

void         eel_mateconf_value_set_string_list (MateConfValue             *value,
					      const GSList           *string_list);

G_END_DECLS

#endif /* MATECONF_UTILS_H */

/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  File-Roller
 *
 *  Copyright (C) 2005 Free Software Foundation, Inc.
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

#ifndef FR_LIST_MODEL_H
#define FR_LIST_MODEL_H

#include <gtk/gtk.h>

#define FR_TYPE_LIST_MODEL            (fr_list_model_get_type ())
#define FR_LIST_MODEL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), FR_TYPE_LIST_MODEL, FRListModel))
#define FR_LIST_MODEL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), FR_TYPE_LIST_MODEL, FRListModelClass))
#define FR_IS_LIST_MODEL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), FR_TYPE_LIST_MODEL))
#define FR_IS_LIST_MODEL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), FR_TYPE_LIST_MODEL))
#define FR_LIST_MODEL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), FR_TYPE_LIST_MODEL, FRListModelClass))

typedef struct FRListModel {
	GtkListStore __parent;
} FRListModel;

typedef struct FRListModelClass {
	GtkListStoreClass __parent_class;
} FRListModelClass;

GType         fr_list_model_get_type (void);
GtkListStore *fr_list_model_new      (int n_columns, ...);

#endif /* FR_LIST_MODEL_H */

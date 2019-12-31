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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef FR_COMMAND_NOMARCH_H
#define FR_COMMAND_NOMARCH_H

#include <glib.h>
#include "fr-command.h"
#include "fr-process.h"

#define FR_TYPE_COMMAND_NOMARCH            (fr_command_nomarch_get_type ())
#define FR_COMMAND_NOMARCH(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), FR_TYPE_COMMAND_NOMARCH, FrCommandNomarch))
#define FR_COMMAND_NOMARCH_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), FR_TYPE_COMMAND_NOMARCH, FrCommandNomarchClass))
#define FR_IS_COMMAND_NOMARCH(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), FR_TYPE_COMMAND_NOMARCH))
#define FR_IS_COMMAND_NOMARCH_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), FR_TYPE_COMMAND_NOMARCH))
#define FR_COMMAND_NOMARCH_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), FR_TYPE_COMMAND_NOMARCH, FrCommandNomarchClass))

typedef struct _FrCommandNomarch       FrCommandNomarch;
typedef struct _FrCommandNomarchClass  FrCommandNomarchClass;

struct _FrCommandNomarch
{
	FrCommand  __parent;

	FileData *fdata;
};

struct _FrCommandNomarchClass
{
	FrCommandClass __parent_class;
};

GType fr_command_nomarch_get_type (void);

#endif /* FR_COMMAND_NOMARCH_H */

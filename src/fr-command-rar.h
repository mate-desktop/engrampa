/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  Engrampa
 *
 *  Copyright (C) 2001 The Free Software Foundation, Inc.
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

#ifndef FR_COMMAND_RAR_H
#define FR_COMMAND_RAR_H

#include <glib.h>
#include "file-data.h"
#include "fr-command.h"
#include "fr-process.h"

#define FR_TYPE_COMMAND_RAR            (fr_command_rar_get_type ())
#define FR_COMMAND_RAR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), FR_TYPE_COMMAND_RAR, FrCommandRar))
#define FR_COMMAND_RAR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), FR_TYPE_COMMAND_RAR, FrCommandRarClass))
#define FR_IS_COMMAND_RAR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), FR_TYPE_COMMAND_RAR))
#define FR_IS_COMMAND_RAR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), FR_TYPE_COMMAND_RAR))
#define FR_COMMAND_RAR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), FR_TYPE_COMMAND_RAR, FrCommandRarClass))

typedef struct _FrCommandRar       FrCommandRar;
typedef struct _FrCommandRarClass  FrCommandRarClass;

typedef enum
{
	FR_COMMAND_RAR_TYPE_RAR4 = 1<<0,
	FR_COMMAND_RAR_TYPE_RAR5 = 1<<1,
	FR_COMMAND_RAR_TYPE_UNRAR_FREE = 1<<2,
	/* RAR5 + empty CRC for directories */
	FR_COMMAND_RAR_TYPE_RAR7 = (1<<3) | FR_COMMAND_RAR_TYPE_RAR5,
} FrCommandRarType;

struct _FrCommandRar
{
	FrCommand  __parent;

	gboolean  list_started;
	gboolean  rar4_odd_line;
	FrCommandRarType output_type;
	FileData *fdata;
};

struct _FrCommandRarClass
{
	FrCommandClass __parent_class;
};

GType fr_command_rar_get_type (void);

#endif /* FR_COMMAND_RAR_H */

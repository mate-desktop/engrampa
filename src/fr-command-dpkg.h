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
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307, USA.
 */

#ifndef FR_COMMAND_DPKG_H
#define FR_COMMAND_DPKG_H

#include <glib.h>
#include "fr-command.h"
#include "fr-process.h"

#define FR_TYPE_COMMAND_DPKG            (fr_command_dpkg_get_type ())
#define FR_COMMAND_DPKG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), FR_TYPE_COMMAND_DPKG, FrCommandDpkg))
#define FR_COMMAND_DPKG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), FR_TYPE_COMMAND_DPKG, FrCommandDpkgClass))
#define FR_IS_COMMAND_DPKG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), FR_TYPE_COMMAND_DPKG))
#define FR_IS_COMMAND_DPKG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), FR_TYPE_COMMAND_DPKG))
#define FR_COMMAND_DPKG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), FR_TYPE_COMMAND_DPKG, FrCommandDpkgClass))

typedef struct _FrCommandDpkg       FrCommandDpkg;
typedef struct _FrCommandDpkgClass  FrCommandDpkgClass;

struct _FrCommandDpkg
{
	FrCommand  __parent;
	gboolean   is_empty;
};

struct _FrCommandDpkgClass
{
	FrCommandClass __parent_class;
};

GType fr_command_dpkg_get_type (void);

#endif /* FR_COMMAND_DPKG_H */

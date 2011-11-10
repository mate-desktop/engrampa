/*
 *  File-Roller
 * 
 *  Copyright (C) 2004 Free Software Foundation, Inc.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  Author: Paolo Bacchilega <paobac@cvs.mate.org>
 * 
 */

#ifndef CAJA_FILEROLLER_H
#define CAJA_FILEROLLER_H

#include <glib-object.h>

G_BEGIN_DECLS

#define CAJA_TYPE_FR  (caja_fr_get_type ())
#define CAJA_FR(o)    (G_TYPE_CHECK_INSTANCE_CAST ((o), CAJA_TYPE_FR, CajaFr))
#define CAJA_IS_FR(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), CAJA_TYPE_FR))

typedef struct _CajaFr      CajaFr;
typedef struct _CajaFrClass CajaFrClass;

struct _CajaFr {
	GObject __parent;
};

struct _CajaFrClass {
	GObjectClass __parent;
};

GType caja_fr_get_type      (void);
void  caja_fr_register_type (GTypeModule *module);

G_END_DECLS

#endif /* CAJA_FILEROLLER_H */

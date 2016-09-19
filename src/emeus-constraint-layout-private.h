/* emeus-constraint-layout-private.h: The constraint layout manager
 *
 * Copyright 2016  Endless
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "emeus-constraint-layout.h"
#include "emeus-types-private.h"

G_BEGIN_DECLS

SimplexSolver * emeus_constraint_layout_get_solver      (EmeusConstraintLayout *layout);

gboolean        emeus_constraint_layout_has_child_data  (EmeusConstraintLayout *layout,
                                                         GtkWidget             *widget);

G_END_DECLS

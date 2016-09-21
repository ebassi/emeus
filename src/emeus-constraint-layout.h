/* emeus-constraint-layout.h: The constraint layout manager
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

#include <emeus-types.h>
#include <emeus-constraint.h>

G_BEGIN_DECLS

#define EMEUS_TYPE_CONSTRAINT_LAYOUT (emeus_constraint_layout_get_type())

EMEUS_AVAILABLE_IN_1_0
G_DECLARE_FINAL_TYPE (EmeusConstraintLayout, emeus_constraint_layout, EMEUS, CONSTRAINT_LAYOUT, GtkContainer)

EMEUS_AVAILABLE_IN_1_0
GtkWidget *     emeus_constraint_layout_new     (void);
EMEUS_AVAILABLE_IN_1_0
void            emeus_constraint_layout_pack    (EmeusConstraintLayout *layout,
                                                 GtkWidget             *child,
                                                 EmeusConstraint       *first_constraint,
                                                 ...);

#define EMEUS_TYPE_CONSTRAINT_LAYOUT_CHILD (emeus_constraint_layout_child_get_type())

EMEUS_AVAILABLE_IN_1_0
G_DECLARE_FINAL_TYPE (EmeusConstraintLayoutChild, emeus_constraint_layout_child, EMEUS, CONSTRAINT_LAYOUT_CHILD, GtkBin)

EMEUS_AVAILABLE_IN_1_0
GtkWidget *     emeus_constraint_layout_child_new                       (void);

EMEUS_AVAILABLE_IN_1_0
int             emeus_constraint_layout_child_get_top                   (EmeusConstraintLayoutChild *child);
EMEUS_AVAILABLE_IN_1_0
int             emeus_constraint_layout_child_get_right                 (EmeusConstraintLayoutChild *child);
EMEUS_AVAILABLE_IN_1_0
int             emeus_constraint_layout_child_get_bottom                (EmeusConstraintLayoutChild *child);
EMEUS_AVAILABLE_IN_1_0
int             emeus_constraint_layout_child_get_left                  (EmeusConstraintLayoutChild *child);
EMEUS_AVAILABLE_IN_1_0
int             emeus_constraint_layout_child_get_width                 (EmeusConstraintLayoutChild *child);
EMEUS_AVAILABLE_IN_1_0
int             emeus_constraint_layout_child_get_height                (EmeusConstraintLayoutChild *child);
EMEUS_AVAILABLE_IN_1_0
int             emeus_constraint_layout_child_get_center_x              (EmeusConstraintLayoutChild *child);
EMEUS_AVAILABLE_IN_1_0
int             emeus_constraint_layout_child_get_center_y              (EmeusConstraintLayoutChild *child);
EMEUS_AVAILABLE_IN_1_0
void            emeus_constraint_layout_child_set_intrinsic_width       (EmeusConstraintLayoutChild *child,
                                                                         int                         width);
EMEUS_AVAILABLE_IN_1_0
void            emeus_constraint_layout_child_set_intrinsic_height      (EmeusConstraintLayoutChild *child,
                                                                         int                         height);
EMEUS_AVAILABLE_IN_1_0
void            emeus_constraint_layout_child_add_constraint            (EmeusConstraintLayoutChild *child,
                                                                         EmeusConstraint            *constraint);
EMEUS_AVAILABLE_IN_1_0
void            emeus_constraint_layout_child_remove_constraint         (EmeusConstraintLayoutChild *child,
                                                                         EmeusConstraint            *constraint);
EMEUS_AVAILABLE_IN_1_0
void            emeus_constraint_layout_child_clear_constraints         (EmeusConstraintLayoutChild *child);

G_END_DECLS

/* emeus-constraint.h: The base constraint object
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

G_BEGIN_DECLS

#define EMEUS_TYPE_CONSTRAINT (emeus_constraint_get_type())

/**
 * EmeusConstraint:
 *
 * The representation of a constraint inside an #EmeusConstraintLayout.
 *
 * The contents of the `EmeusConstraint` structure are private and should
 * never be accessed directly.
 *
 * Since: 1.0
 */
EMEUS_AVAILABLE_IN_1_0
G_DECLARE_FINAL_TYPE (EmeusConstraint, emeus_constraint, EMEUS, CONSTRAINT, GInitiallyUnowned)

EMEUS_AVAILABLE_IN_1_0
EmeusConstraint *               emeus_constraint_new                    (gpointer                 target_object,
                                                                         EmeusConstraintAttribute target_attribute,
                                                                         EmeusConstraintRelation  relation,
                                                                         gpointer                 source_object,
                                                                         EmeusConstraintAttribute source_attribute,
                                                                         double                   multiplier,
                                                                         double                   constant,
                                                                         int                      strength);
EMEUS_AVAILABLE_IN_1_0
EmeusConstraint *               emeus_constraint_new_constant           (gpointer                 target_object,
                                                                         EmeusConstraintAttribute target_attribute,
                                                                         EmeusConstraintRelation  relation,
                                                                         double                   constant,
                                                                         int                      strength);

EMEUS_AVAILABLE_IN_1_0
gpointer                        emeus_constraint_get_target_object      (EmeusConstraint         *constraint);
EMEUS_AVAILABLE_IN_1_0
EmeusConstraintAttribute        emeus_constraint_get_target_attribute   (EmeusConstraint         *constraint);
EMEUS_AVAILABLE_IN_1_0
EmeusConstraintRelation         emeus_constraint_get_relation           (EmeusConstraint         *constraint);
EMEUS_AVAILABLE_IN_1_0
gpointer                        emeus_constraint_get_source_object      (EmeusConstraint         *constraint);
EMEUS_AVAILABLE_IN_1_0
EmeusConstraintAttribute        emeus_constraint_get_source_attribute   (EmeusConstraint         *constraint);
EMEUS_AVAILABLE_IN_1_0
double                          emeus_constraint_get_multiplier         (EmeusConstraint         *constraint);
EMEUS_AVAILABLE_IN_1_0
double                          emeus_constraint_get_constant           (EmeusConstraint         *constraint);
EMEUS_AVAILABLE_IN_1_0
int                             emeus_constraint_get_strength           (EmeusConstraint         *constraint);
EMEUS_AVAILABLE_IN_1_0
gboolean                        emeus_constraint_is_required            (EmeusConstraint         *constraint);

EMEUS_AVAILABLE_IN_1_0
gboolean                        emeus_constraint_is_attached            (EmeusConstraint         *constraint);

EMEUS_AVAILABLE_IN_1_0
void                            emeus_constraint_set_active             (EmeusConstraint         *constraint,
                                                                         gboolean                 active);
EMEUS_AVAILABLE_IN_1_0
gboolean                        emeus_constraint_get_active             (EmeusConstraint         *constraint);

G_END_DECLS

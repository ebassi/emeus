/* emeus-types.h
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

#include <gtk/gtk.h>

#include <emeus-version-macros.h>

G_BEGIN_DECLS

typedef enum {
  EMEUS_CONSTRAINT_STRENGTH_WEAK     = 1,
  EMEUS_CONSTRAINT_STRENGTH_MEDIUM   = 1000,
  EMEUS_CONSTRAINT_STRENGTH_STRONG   = 1000000,
  EMEUS_CONSTRAINT_STRENGTH_REQUIRED = 1001001000
} EmeusConstraintStrength;

#define EMEUS_TYPE_CONSTRAINT_STRENGTH (emeus_constraint_strength_get_type())

EMEUS_AVAILABLE_IN_1_0
GType emeus_constraint_strength_get_type (void) G_GNUC_CONST;

typedef enum
{
  EMEUS_CONSTRAINT_RELATION_LE = -1,
  EMEUS_CONSTRAINT_RELATION_EQ =  0,
  EMEUS_CONSTRAINT_RELATION_GE =  1
} EmeusConstraintRelation;

#define EMEUS_TYPE_CONSTRAINT_RELATION (emeus_constraint_relation_get_type())

EMEUS_AVAILABLE_IN_1_0
GType emeus_constraint_relation_get_type (void) G_GNUC_CONST;

typedef enum
{
  EMEUS_CONSTRAINT_ATTRIBUTE_INVALID,

  EMEUS_CONSTRAINT_ATTRIBUTE_LEFT,
  EMEUS_CONSTRAINT_ATTRIBUTE_RIGHT,
  EMEUS_CONSTRAINT_ATTRIBUTE_TOP,
  EMEUS_CONSTRAINT_ATTRIBUTE_BOTTOM,
  EMEUS_CONSTRAINT_ATTRIBUTE_START,
  EMEUS_CONSTRAINT_ATTRIBUTE_END,
  EMEUS_CONSTRAINT_ATTRIBUTE_WIDTH,
  EMEUS_CONSTRAINT_ATTRIBUTE_HEIGHT,
  EMEUS_CONSTRAINT_ATTRIBUTE_CENTER_X,
  EMEUS_CONSTRAINT_ATTRIBUTE_CENTER_Y,
  EMEUS_CONSTRAINT_ATTRIBUTE_BASELINE
} EmeusConstraintAttribute;

#define EMEUS_TYPE_CONSTRAINT_ATTRIBUTE (emeus_constraint_attribute_get_type())

EMEUS_AVAILABLE_IN_1_0
GType emeus_constraint_attribute_get_type (void) G_GNUC_CONST;

G_END_DECLS

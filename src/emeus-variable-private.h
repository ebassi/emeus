/* emeus-variable-private.h: A symbolic value
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

#include "emeus-types-private.h"

G_BEGIN_DECLS

static inline bool
variable_is_dummy (const Variable *variable)
{
  return variable->type == VARIABLE_DUMMY;
}

static inline bool
variable_is_objective (const Variable *variable)
{
  return variable->type == VARIABLE_OBJECTIVE;
}

static inline bool
variable_is_slack (const Variable *variable)
{
  return variable->type == VARIABLE_SLACK;
}

static inline bool
variable_is_external (const Variable *variable)
{
  return variable->is_external;
}

static inline bool
variable_is_pivotable (const Variable *variable)
{
  return variable->is_pivotable;
}

static inline gboolean
variable_is_restricted (const Variable *variable)
{
  return variable->is_restricted;
}

static inline double
variable_get_value (const Variable *variable)
{
  if (variable_is_dummy (variable) ||
      variable_is_objective (variable) ||
      variable_is_slack (variable))
    return 0.0;

  return variable->value;
}

Variable *variable_new (SimplexSolver *solver,
                        VariableType type);
Variable *variable_ref (Variable *variable);
void variable_unref (Variable *variable);

void variable_set_value (Variable *variable,
                         double value);

char *variable_to_string (const Variable *variable);

void variable_set_name (Variable *variable,
                        const char *name);

void variable_set_prefix (Variable *variable,
                          const char *prefix);

G_END_DECLS

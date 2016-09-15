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
void      variable_unref (Variable *variable);

G_END_DECLS

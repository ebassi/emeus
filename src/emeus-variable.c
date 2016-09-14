#include "config.h"

#include "emeus-variable-private.h"

#include <math.h>
#include <float.h>

static void
dummy_variable_init (Variable *v)
{
  v->is_external = FALSE;
  v->is_pivotable = FALSE;
  v->is_restricted = TRUE;
}

static void
objective_variable_init (Variable *v)
{
  v->is_external = FALSE;
  v->is_pivotable = FALSE;
  v->is_restricted = FALSE;
}

static void
slack_variable_init (Variable *v)
{
  v->is_external = FALSE;
  v->is_pivotable = TRUE;
  v->is_restricted = TRUE;
}

static void
regular_variable_init (Variable *v)
{
  v->is_external = TRUE;
  v->is_pivotable = FALSE;
  v->is_restricted = FALSE;
}

Variable *
variable_new (VariableType  type,
              const char   *name,
              double        value)
{
  Variable *res = g_slice_new (Variable);

  res->type = type;
  res->name = g_strdup (name);
  res->ref_count = 1;

  switch (type)
    {
    case VARIABLE_DUMMY:
      dummy_variable_init (res);
      break;

    case VARIABLE_OBJECTIVE:
      objective_variable_init (res);
      break;

    case VARIABLE_SLACK:
      slack_variable_init (res);
      break;

    case VARIABLE_REGULAR:
      regular_variable_init (res);
      res->value = value;
      break;
    }

  return res;
}

static void
variable_free (Variable *variable)
{
  if (variable == NULL)
    return;

  g_free (variable->name);
  g_slice_free (Variable, variable);
}

Variable *
variable_ref (Variable *variable)
{
  if (variable == NULL)
    return NULL;

  variable->ref_count += 1;

  return variable;
}

void
variable_unref (Variable *variable)
{
  if (variable == NULL)
    return;

  variable->ref_count -= 1;
  if (variable->ref_count == 0)
    variable_free (variable);
}

Variable *
variable_clone (const Variable *variable)
{
  Variable *res = g_slice_dup (Variable, variable);

  if (res == NULL)
    return NULL;

  res->name = g_strdup (variable->name);
  res->ref_count = 1;

  return res;
}

const char *
variable_get_name (const Variable *variable)
{
  return variable->name;
}

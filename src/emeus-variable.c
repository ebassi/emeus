#include "config.h"

#include "emeus-variable-private.h"

#include <math.h>
#include <float.h>

static inline void
dummy_variable_init (Variable   *v,
                     const char *name)
{
  v->v_type = VARIABLE_TYPE_DUMMY;
  v->name = g_quark_from_string (name);
  v->is_dummy = TRUE;
  v->is_external = FALSE;
  v->is_pivotable = FALSE;
  v->is_restricted = TRUE;
  v->value = 0.0;
}

static inline void
objective_variable_init (Variable   *v,
                         const char *name)
{
  v->v_type = VARIABLE_TYPE_OBJECTIVE;
  v->name = g_quark_from_string (name);
  v->is_dummy = FALSE;
  v->is_external = FALSE;
  v->is_pivotable = FALSE;
  v->is_restricted = FALSE;
  v->value = 0.0;
}

static inline void
slack_variable_init (Variable   *v,
                     const char *name)
{
  v->v_type = VARIABLE_TYPE_SLACK;
  v->name = g_quark_from_string (name);
  v->is_dummy = FALSE;
  v->is_external = FALSE;
  v->is_pivotable = TRUE;
  v->is_restricted = TRUE;
  v->value = 0.0;
}

static inline void
regular_variable_init (Variable   *v,
                       const char *name,
                       double      value)
{
  v->v_type = VARIABLE_TYPE_REGULAR;
  v->name = g_quark_from_string (name);
  v->is_dummy = FALSE;
  v->is_external = TRUE;
  v->is_pivotable = FALSE;
  v->is_restricted = FALSE;
  v->value = value;
}

gpointer
variable_new (VariableType  v_type,
              const char   *name,
              double        value)
{
  gpointer res = g_slice_new (Variable);

  switch (v_type)
    {
    case VARIABLE_TYPE_DUMMY:
      dummy_variable_init (res, name);
      break;

    case VARIABLE_TYPE_OBJECTIVE:
      objective_variable_init (res, name);
      break;

    case VARIABLE_TYPE_SLACK:
      slack_variable_init (res, name);
      break;

    case VARIABLE_TYPE_REGULAR:
      regular_variable_init (res, name, value);
      break;
    }

  return res;
}

gpointer
variable_copy (gpointer variable)
{
  if (variable == NULL)
    return NULL;

  return g_slice_dup (Variable, variable);
}

void
variable_free (gpointer variable)
{
  if (variable == NULL)
    return;

  g_slice_free (Variable, variable);
}

guint
variable_hash (gconstpointer v)
{
  return GPOINTER_TO_UINT (v);
}

gboolean
variable_equal (gconstpointer v1,
                gconstpointer v2)
{
  const Variable *av1 = v1;
  const Variable *av2 = v2;

  if (av1 == av2)
    return TRUE;

  if (av1->v_type == av2->v_type)
    return TRUE;

  if (av1->name == av2->name)
    return TRUE;

  if (av1->v_type == VARIABLE_TYPE_REGULAR)
    {
      if (fabs (av1->value - av2->value) < DBL_EPSILON)
        return TRUE;
    }

  return FALSE;
}

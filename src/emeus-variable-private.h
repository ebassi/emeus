#pragma once

#include "emeus-types-private.h"

G_BEGIN_DECLS

static inline gboolean
variable_is_dummy (Variable *variable)
{
  return variable->v_type == VARIABLE_TYPE_DUMMY;
}

static inline gboolean
variable_is_objective (Variable *variable)
{
  return variable->v_type == VARIABLE_TYPE_OBJECTIVE;
}

static inline gboolean
variable_is_slack (Variable *variable)
{
  return variable->v_type == VARIABLE_TYPE_SLACK;
}

static inline gboolean
variable_is_regular (Variable *variable)
{
  return variable->v_type == VARIABLE_TYPE_REGULAR;
}

gpointer        variable_new    (VariableType v_type,
                                 const char  *name,
                                 double       value);
gpointer        variable_copy   (gpointer     variable);
void            variable_free   (gpointer     variable);

guint           variable_hash   (gconstpointer v);
gboolean        variable_equal  (gconstpointer v1,
                                 gconstpointer v2);

char *          variable_to_string      (const Variable *variable);

G_END_DECLS

#pragma once

#include "emeus-types-private.h"

static inline gboolean
simplex_constraint_is_edit (SimplexConstraint *constraint)
{
  return constraint->c_type == CONSTRAINT_TYPE_EDIT;
}

static inline gboolean
simplex_constraint_is_stay (SimplexConstraint *constraint)
{
  return constraint->c_type == CONSTRAINT_TYPE_STAY;
}

static inline gboolean
simplex_constraint_is_regular (SimplexConstraint *constraint)
{
  return constraint->c_type == CONSTRAINT_TYPE_REGULAR;
}

SimplexConstraint *     simplex_constraint_new  (ConstraintType  c_type,
                                                 Variable       *param1,
                                                 OperatorType    op_type,
                                                 Expression     *param2,
                                                 int             strength,
                                                 double          weight);

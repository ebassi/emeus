#include "config.h"

#include "emeus-simplex-constraint-private.h"
#include "emeus-expression-private.h"
#include "emeus-variable-private.h"

SimplexConstraint *
simplex_constraint_new (ConstraintType  c_type,
                        Variable       *variable,
                        OperatorType    op_type,
                        Expression     *expression,
                        int             strength,
                        double          weight)
{
  SimplexConstraint *res;

  res = g_slice_new (SimplexConstraint);
  res->c_type = c_type;
  res->variable = variable;
  res->op_type = op_type;
  res->expression = expression;
  res->strength = strength;
  res->weight = weight;
  res->is_inequality = op_type != OPERATOR_TYPE_EQ;

  return res;
}

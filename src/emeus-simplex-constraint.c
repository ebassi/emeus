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

static const char *operators[] = {
  "<=",
  "==",
  ">="
};

static const char *
strength_to_string (int strength)
{
  if (strength >= STRENGTH_REQUIRED)
    return "required";

  if (strength >= STRENGTH_STRONG)
    return "strong";

  if (strength >= STRENGTH_MEDIUM)
    return "medium";

  return "weak";
}

char *
simplex_constraint_to_string (const SimplexConstraint *constraint)
{
  GString *buf;
  char *str;

  if (constraint == NULL)
    return NULL;

  buf = g_string_new (NULL);

  g_string_append (buf, strength_to_string (constraint->strength));
  g_string_append_printf (buf, " {%g} ", constraint->weight);
  g_string_append (buf, "(");

  str = expression_to_string (constraint->expression);

  if (str != NULL)
    g_string_append (buf, str);

  g_free (str);

  g_string_append (buf, ") ");

  g_string_append (buf, operators[constraint->op_type + 1]);

  g_string_append (buf, " 0");

  return g_string_free (buf, FALSE);
}

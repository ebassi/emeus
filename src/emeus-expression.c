#include "config.h"

#include "emeus-expression-private.h"
#include "emeus-variable-private.h"

#include <float.h>
#include <math.h>

static void     expression_set_variable         (Expression *expression,
                                                 Variable   *variable,
                                                 double      value);

Expression *
expression_new (Variable *variable,
                double    value,
                double    constant)
{
  Expression *res = g_slice_new (Expression);

  res->constant = constant;

  if (variable != NULL)
    expression_set_variable (res, variable, constant);

  return res;
}

void
expression_free (Expression *expression)
{
  if (expression == NULL)
    return;

  if (expression->terms != NULL)
    g_hash_table_unref (expression->terms);

  g_slice_free (Expression, expression);
}

Expression *
expression_copy (Expression *expression)
{
  Expression *res;
  GHashTableIter iter;
  gpointer key_p, value_p;

  if (expression == NULL)
    return NULL;

  res = expression_new (NULL, 0.0, expression->constant);
  if (expression->terms == NULL)
    return res;

  res->terms = g_hash_table_new_full (variable_hash, variable_equal, variable_free, g_free);

  g_hash_table_iter_init (&iter, expression->terms);
  while (g_hash_table_iter_next (&iter, &key_p, &value_p))
    {
      Variable *variable = key_p;
      double *value = value_p;

      g_hash_table_insert (res->terms, variable_copy (variable), g_memdup (value, sizeof (double)));
    }

  return res;
}

gboolean
expression_is_constant (Expression *expression)
{
  return expression->terms == NULL;
}

char *
expression_to_string (const Expression *expression)
{
  GString *buf;
  gboolean needs_plus = FALSE;
  GHashTableIter iter;
  gpointer key_p, value_p;

  if (expression == NULL)
    return NULL;

  buf = g_string_new (NULL);

  if (fabs (expression->constant - 0.0) > DBL_EPSILON || expression->terms == NULL)
    {
      g_string_append_printf (buf, "%g", expression->constant);

      if (expression->terms == NULL)
        return g_string_free (buf, FALSE);

      needs_plus = TRUE;
    }

  g_hash_table_iter_init (&iter, expression->terms);
  while (g_hash_table_iter_next (&iter, &key_p, &value_p))
    {
      Variable *v = key_p;
      double *coeff = value_p;
      char *var_str;

      if (needs_plus)
        g_string_append (buf, " + ");

      var_str = variable_to_string (v);

      g_string_append_printf (buf, "%g * %s", *coeff, var_str);

      g_free (var_str);

      needs_plus = TRUE;
    }

  return g_string_free (buf, FALSE);
}

static void
expression_set_variable (Expression *expression,
                         Variable   *variable,
                         double      value)
{
  double *value_p;

  if (expression->terms == NULL)
    expression->terms = g_hash_table_new_full (variable_hash, variable_equal, variable_free, g_free);

  value_p = g_new (double, 1);
  *value_p = value;

  g_hash_table_insert (expression->terms, variable_copy (variable), value_p);
}

void
expression_add_variable (Expression *expression,
                         Variable   *variable,
                         double      coefficient)
{
  double *new_coefficient;

  if (expression->terms == NULL || g_hash_table_lookup (expression->terms, variable) == NULL)
    {
      if (fabs (coefficient - 0.0) > DBL_EPSILON)
        expression_set_variable (expression, variable, coefficient);

      return;
    }

  new_coefficient = g_hash_table_lookup (expression->terms, variable);
  g_assert (new_coefficient != NULL);

  *new_coefficient = *new_coefficient + coefficient;

  if (fabs (*new_coefficient - 0.0) < DBL_EPSILON)
    expression_remove_variable (expression, variable);
  else
    expression_set_variable (expression, variable, *new_coefficient);
}

gboolean
expression_has_variable (Expression *expression,
                         Variable   *variable)
{
  if (expression->terms == NULL)
    return FALSE;

  return g_hash_table_lookup (expression->terms, variable) != NULL;
}

void
expression_remove_variable (Expression *expression,
                            Variable   *variable)
{
  if (expression->terms == NULL)
    return;

  g_hash_table_remove (expression->terms, variable);
}

double
expression_get_coefficient (Expression *expression,
                            Variable   *variable)
{
  double *coeff;

  if (expression->terms == NULL)
    return 0.0;

  coeff = g_hash_table_lookup (expression->terms, variable);
  if (coeff == NULL)
    return 0.0;

  return *coeff;
}

void
expression_multiply (Expression *expression,
                     double      value)
{
  GHashTableIter iter;
  gpointer value_p;

  expression->constant *= value;

  if (expression->terms == NULL)
    return;

  g_hash_table_iter_init (&iter, expression->terms);
  while (g_hash_table_iter_next (&iter, NULL, &value_p))
    {
      double *coefficient = value_p;

      *coefficient = *coefficient * value;
    }
}

double
expression_new_subject (Expression *expression,
                        Variable   *subject)
{
  double value;
  
  if (!expression_has_variable (expression, subject))
    return 0.0;

  value = expression_get_coefficient (expression, subject);
  expression_remove_variable (expression, subject);

  if (fabs (value - 0.0) < DBL_EPSILON)
    return 0.0;

  expression_multiply (expression, 1.0 / value);

  return 1.0 / value;
}

void
expression_change_subject (Expression *expression,
                           Variable   *old_subject,
                           Variable   *new_subject)
{
  expression_set_variable (expression, old_subject, expression_new_subject (expression, new_subject));
}

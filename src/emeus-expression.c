#include "config.h"

#include "emeus-expression-private.h"
#include "emeus-variable-private.h"

#include <glib.h>
#include <math.h>
#include <float.h>

Term *
term_new (Variable *variable,
          double coefficient)
{
  Term *t = g_slice_new (Term);

  t->variable = variable_ref (variable);
  t->coefficient = coefficient;

  return t;
}

Term *
term_clone (const Term *term)
{
  Term *t = g_slice_new (Term);

  t->variable = variable_clone (term->variable);
  t->coefficient = term->coefficient;

  return t;
}

void
term_free (Term *term)
{
  if (term == NULL)
    return;

  variable_unref (term->variable);

  g_slice_free (Term, term);
}

gboolean
term_equal (gconstpointer v1,
            gconstpointer v2)
{
  const Term *a = v1;
  const Term *b = v2;

  if (a == b)
    return TRUE;

  if (a->variable == b->variable &&
      fabs (a->coefficient - b->coefficient) < DBL_EPSILON)
    return TRUE;

  if (g_strcmp0 (variable_get_name (a->variable), variable_get_name (b->variable)) != 0)
    return FALSE;

  if (fabs (variable_get_value (a->variable) - variable_get_value (b->variable)) > DBL_EPSILON)
    return FALSE;

  if (fabs (a->coefficient - b->coefficient) > DBL_EPSILON)
    return FALSE;

  return TRUE;
}

Expression *
expression_new (Variable *variable,
                double value,
                double constant)
{
  Expression *res = g_slice_new (Expression);

  res->constant = constant;
  res->terms = NULL;

  if (variable != NULL)
    expression_add_variable (res, variable, value);

  return res;
}

Expression *
expression_new_from_variable (Variable *variable)
{
  return expression_new (variable, 1.0, 0.0);
}

Expression *
expression_new_from_constant (double constant)
{
  return expression_new (NULL, 1.0, constant);
}

Expression *
expression_new_empty (void)
{
  return expression_new (NULL, 1.0, 0.0);
}

Expression *
expression_ref (Expression *expression)
{
  if (expression == NULL)
    return NULL;

  expression->ref_count += 1;

  return expression;
}

void
expression_unref (Expression *expression)
{
  if (expression == NULL)
    return;

  expression->ref_count -= 1;

  if (expression->ref_count == 0)
    {
      if (expression->terms != NULL)
        g_hash_table_unref (expression->terms);

      g_slice_free (Expression, expression);
    }
}

static void
expression_add_term (Expression *expression,
                     Term *term)
{
  if (expression->terms == NULL)
    expression->terms = g_hash_table_new_full (NULL, term_equal, (GDestroyNotify) term_free, NULL);

  g_hash_table_add (expression->terms, term);
}

void
expression_add_variable (Expression *expression,
                         Variable *variable,
                         double value)
{
  Term *term = term_new (variable, value);

  expression_add_term (expression, term);
}

void
expression_remove_variable (Expression *expression,
                            Variable *variable)
{
  GHashTableIter iter;
  gpointer key_p;

  if (expression->terms == NULL)
    return;

  g_hash_table_iter_init (&iter, expression->terms);
  while (g_hash_table_iter_next (&iter, &key_p, NULL))
    {
      Term *t = key_p;

      if (t->variable == variable)
        g_hash_table_iter_remove (&iter);
    }
}

Expression *
expression_clone (const Expression *expression)
{
  GHashTableIter iter;
  Expression *res;
  gpointer key_p;

  if (expression == NULL)
    return NULL;

  res = g_slice_new (Expression);
  res->constant = expression->constant;

  if (expression->terms == NULL)
    return res;

  g_hash_table_iter_init (&iter, expression->terms);
  while (g_hash_table_iter_next (&iter, &key_p, NULL))
    expression_add_term (res, term_clone (key_p));

  return res;
}

GPtrArray *
expression_get_terms_as_array (const Expression *expression)
{
  GHashTableIter iter;
  GPtrArray *res;
  gpointer key_p;

  if (expression->terms == NULL)
    return NULL;

  res = g_ptr_array_new_full (g_hash_table_size (expression->terms), (GDestroyNotify) term_free);

  g_hash_table_iter_init (&iter, expression->terms);
  while (g_hash_table_iter_next (&iter, &key_p, NULL))
    g_ptr_array_add (res, term_clone (key_p));

  return res;
}

double
expression_get_value (const Expression *expression)
{
  GHashTableIter iter;
  gpointer key_p;
  double res;

  res = expression->constant;

  if (expression->terms == NULL)
    return res;

  g_hash_table_iter_init (&iter, expression->terms);
  while (g_hash_table_iter_next (&iter, &key_p, NULL))
    {
      const Term *t = key_p;

      res += term_get_value (t);
    }

  return res;
}

void
expression_terms_foreach (Expression *expression,
                          ExpressionForeachTermFunc func,
                          gpointer data)
{
  GHashTableIter iter;
  gpointer key_p;

  if (expression->terms == NULL)
    return;

  g_hash_table_iter_init (&iter, expression->terms);
  while (g_hash_table_iter_next (&iter, &key_p, NULL))
    func (key_p, data);
}

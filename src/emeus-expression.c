#include "config.h"

#include "emeus-expression-private.h"
#include "emeus-variable-private.h"
#include "emeus-simplex-solver-private.h"

#include <glib.h>
#include <math.h>
#include <float.h>

static Term *
term_new (Variable *variable,
          double coefficient)
{
  Term *t = g_slice_new (Term);

  t->variable = variable_ref (variable);
  t->coefficient = coefficient;

  return t;
}

static void
term_free (Term *term)
{
  if (term == NULL)
    return;

  variable_unref (term->variable);

  g_slice_free (Term, term);
}

static Expression *
expression_new_full (SimplexSolver *solver,
                     Variable *variable,
                     double coefficient,
                     double constant)
{
  Expression *res = g_slice_new (Expression);

  res->solver = solver;
  res->constant = constant;
  res->terms = NULL;
  res->ref_count = 1;

  if (variable != NULL)
    expression_add_variable (res, variable, coefficient);

  return res;
}

Expression *
expression_new (SimplexSolver *solver,
                double constant)
{
  return expression_new_full (solver, NULL, 0.0, constant);
}

Expression *
expression_new_from_variable (Variable *variable)
{
  return expression_new_full (variable->solver, variable, 1.0, 0.0);
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
    expression->terms = g_hash_table_new_full (NULL, NULL, NULL, (GDestroyNotify) term_free);

  g_hash_table_insert (expression->terms, term->variable, term);

  simplex_solver_add_variable (expression->solver, term->variable);
}

void
expression_add_variable (Expression *expression,
                         Variable *variable,
                         double coefficient)
{
  if (expression->terms != NULL)
    {
      Term *t = g_hash_table_lookup (expression->terms, variable);

      if (t != NULL)
        {
          if (coefficient == 0.0)
            {
              simplex_solver_remove_variable (expression->solver, t->variable);
              g_hash_table_remove (expression->terms, t);
            }
          else
            t->coefficient = coefficient;

          return;
        }
    }

  expression_add_term (expression, term_new (variable, coefficient));
}

void
expression_remove_variable (Expression *expression,
                            Variable *variable)
{
  if (expression->terms == NULL)
    return;

  simplex_solver_remove_variable (expression->solver, variable);
  g_hash_table_remove (expression->terms, variable);
}

void
expression_set_coefficient (Expression *expression,
                            Variable *variable,
                            double coefficient)
{
  if (coefficient == 0.0)
    expression_remove_variable (expression, variable);
  else
    {
      Term *t = g_hash_table_lookup (expression->terms, variable);

      if (t != NULL)
        t->coefficient = coefficient;
      else
        expression_add_variable (expression, variable, coefficient);
    }
}

double
expression_get_coefficient (const Expression *expression,
                            Variable *variable)
{
  Term *t;

  if (expression->terms == NULL)
    return 0.0;

  t = g_hash_table_lookup (expression->terms, variable);
  if (t == NULL)
    return 0.0;

  return term_get_coefficient (t);
}

double
expression_get_value (const Expression *expression)
{
  GHashTableIter iter;
  gpointer value_p;
  double res;

  res = expression->constant;

  if (expression->terms == NULL)
    return res;

  g_hash_table_iter_init (&iter, expression->terms);
  while (g_hash_table_iter_next (&iter, NULL, &value_p))
    {
      const Term *t = value_p;

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
  gpointer value_p;

  if (expression->terms == NULL)
    return;

  g_hash_table_iter_init (&iter, expression->terms);
  while (g_hash_table_iter_next (&iter, NULL, &value_p))
    func (value_p, data);
}

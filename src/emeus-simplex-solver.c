#include "config.h"

#include "emeus-simplex-solver-private.h"

#include "emeus-types-private.h"
#include "emeus-variable-private.h"
#include "emeus-expression-private.h"

#include <glib.h>
#include <string.h>

void
simplex_solver_init (SimplexSolver *solver)
{
  memset (solver, 0, sizeof (SimplexSolver));

  /* HashTable<Variable, HashSet<Variable>>; owns keys and values */
  solver->columns = g_hash_table_new_full (NULL, NULL,
                                           (GDestroyNotify) variable_unref,
                                           (GDestroyNotify) g_hash_table_unref);

  /* HashTable<Variable, Expression>; owns keys and values */
  solver->rows = g_hash_table_new_full (NULL, NULL,
                                        (GDestroyNotify) variable_unref,
                                        (GDestroyNotify) expression_unref);

  /* HashTable<Variable, Expression>; does not own keys or values */
  solver->external_rows = g_hash_table_new (NULL, NULL);

  /* HashSet<Variable>; does not own keys */
  solver->infeasible_rows = g_hash_table_new (NULL, NULL);

  /* HashSet<Variable>; does not own keys */
  solver->updated_externals = g_hash_table_new (NULL, NULL);
}

void
simplex_solver_clear (SimplexSolver *solver)
{
  g_clear_pointer (&solver->columns, g_hash_table_unref);
  g_clear_pointer (&solver->rows, g_hash_table_unref);
  g_clear_pointer (&solver->external_rows, g_hash_table_unref);
}

static GHashTable *
simplex_solver_get_column_set (SimplexSolver *solver,
                               Variable *param_var)
{
  GHashTable *res;

  res = g_hash_table_lookup (solver->columns, param_var);
  if (res == NULL)
    {
      res = g_hash_table_new_full (NULL, NULL,
                                   (GDestroyNotify) variable_unref,
                                   NULL);
      g_hash_table_insert (solver->columns, variable_ref (param_var), res);
    }

  return res;
}

static gboolean
simplex_solver_has_column_set (SimplexSolver *solver,
                               Variable *param_var)
{
  return g_hash_table_lookup (solver->columns, param_var) != NULL;
}

static void
simplex_solver_insert_column_variable (SimplexSolver *solver,
                                       Variable *param_var,
                                       Variable *row_var)
{
  GHashTable *row_set;

  row_set = simplex_solver_get_column_set (solver, param_var);
  g_hash_table_add (row_set, variable_ref (row_var));
}

typedef struct {
  Variable *variable;
  SimplexSolver *solver;
} ForeachClosure;

static void
insert_expression_columns (Term *term,
                           gpointer data_)
{
  ForeachClosure *data = data_;

  simplex_solver_insert_column_variable (data->solver,
                                         term_get_variable (term),
                                         data->variable);
}

static void
simplex_solver_add_row (SimplexSolver *solver,
                        Variable *variable,
                        Expression *expression)
{
  ForeachClosure data;

  g_hash_table_insert (solver->rows, variable_ref (variable), expression_ref (expression));

  data.variable = variable;
  data.solver = solver;
  expression_terms_foreach (expression,
                            insert_expression_columns,
                            &data);

  if (variable_is_external (variable))
    g_hash_table_insert (solver->external_rows, variable, expression);
}

static void
simplex_solver_remove_column (SimplexSolver *solver,
                              Variable *variable)
{
  GHashTable *row_set = g_hash_table_lookup (solver->columns, variable);
  GHashTableIter iter;
  gpointer key_p;

  if (row_set == NULL)
    goto out;

  g_hash_table_iter_init (&iter, row_set);
  while (g_hash_table_iter_next (&iter, &key_p, NULL))
    {
      Expression *e = key_p;

      expression_remove_variable (e, variable);
    }

  g_hash_table_remove (row_set, variable);

out:
  if (variable_is_external (variable))
    g_hash_table_remove (solver->external_rows, variable);
}

static void
remove_expression_columns (Term *term,
                           gpointer data_)
{
  ForeachClosure *data = data_;

  GHashTable *row_set = g_hash_table_lookup (data->solver->columns, term_get_variable (term));

  if (row_set != NULL)
    g_hash_table_remove (row_set, data->variable);
}

static Expression *
simplex_solver_remove_row (SimplexSolver *solver,
                           Variable *variable)
{
  Expression *e = g_hash_table_lookup (solver->rows, variable);
  ForeachClosure data;

  g_assert (e != NULL);

  expression_ref (e);

  data.variable = variable;
  data.solver = solver;
  expression_terms_foreach (e,
                            remove_expression_columns,
                            &data);

  g_hash_table_remove (solver->infeasible_rows, variable);

  if (variable_is_external (variable))
    g_hash_table_remove (solver->external_rows, variable);

  g_hash_table_remove (solver->rows, variable);

  return e;
}

static void
expression_substitute_out (SimplexSolver *solver,
                           Expression *expression,
                           Variable *out_variable,
                           Expression *new_expression,
                           Variable *subject)
{
  double coefficient = expression_get_coefficient (expression, out_variable);

  expression_remove_variable (expression, out_variable);

  expression->constant += (coefficient * new_expression->constant);
}

static void
simplex_solver_substitute_out (SimplexSolver *solver,
                               Variable *old_variable,
                               Expression *expression)
{
  GHashTable *row_set = g_hash_table_lookup (solver->columns, old_variable);

  if (row_set == NULL)
    {
      GHashTableIter iter;
      gpointer key_p;

      g_hash_table_iter_init (&iter, solver->rows);
      while (g_hash_table_iter_next (&iter, &key_p, NULL))
        {
          Variable *variable = key_p;
          Expression *e = g_hash_table_lookup (solver->rows, variable);

          expression_substitute_out (solver, e, old_variable, expression, variable);

          if (variable_is_external (variable))
            g_hash_table_add (solver->updated_externals, variable);

          if (variable_is_restricted (variable) && expression_get_constant (e) < 0)
            g_hash_table_add (solver->infeasible_rows, variable);
        }

      if (variable_is_external (old_variable))
        g_hash_table_insert (solver->external_rows, variable_ref (old_variable), expression);

      g_hash_table_remove (solver->columns, old_variable);
    }
}

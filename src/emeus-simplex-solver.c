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

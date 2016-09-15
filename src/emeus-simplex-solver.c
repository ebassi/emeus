#include "config.h"

#include "emeus-simplex-solver-private.h"

#include "emeus-types-private.h"
#include "emeus-variable-private.h"
#include "emeus-expression-private.h"

#include <glib.h>
#include <string.h>

Constraint *
constraint_ref (Constraint *constraint)
{
  constraint->ref_count += 1;

  return constraint;
}

void
constraint_unref (Constraint *constraint)
{
  constraint->ref_count -= 1;

  if (constraint->ref_count == 0)
    {
      expression_unref (constraint->expression);
      g_slice_free (Constraint, constraint);
    }
}

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

  solver->needs_solving = false;
  solver->auto_solve = false;
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
  return g_hash_table_lookup (solver->columns, param_var);
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

  if (simplex_solver_has_column_set (solver, param_var))
    return;

  row_set = simplex_solver_get_column_set (solver, param_var);
  if (row_set == NULL)
    {
      row_set = g_hash_table_new_full (NULL, NULL,
                                       (GDestroyNotify) variable_unref,
                                       NULL);
      g_hash_table_insert (solver->columns, variable_ref (param_var), row_set);
    }

  if (row_var != NULL)
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

void
simplex_solver_add_variable (SimplexSolver *solver,
                             Variable *variable,
                             Variable *subject)
{
  if (subject != NULL)
    simplex_solver_insert_column_variable (solver, variable, subject);
}

void
simplex_solver_remove_variable (SimplexSolver *solver,
                                Variable *variable,
                                Variable *subject)
{
  GHashTable *row_set = g_hash_table_lookup (solver->columns, variable);

  if (row_set != NULL && subject != NULL)
    g_hash_table_remove (row_set, subject);
}

void
simplex_solver_update_variable (SimplexSolver *solver,
                                Variable *variable)
{
}

Variable *
simplex_solver_create_variable (SimplexSolver *solver)
{
  return variable_new (solver, VARIABLE_REGULAR);
}

Expression *
simplex_solver_create_expression (SimplexSolver *solver,
                                  double constant)
{
  return expression_new (solver, constant);
}

static void
simplex_solver_add_constraint_internal (SimplexSolver *solver,
                                        Constraint *constraint)
{
  solver->needs_solving = true;

  if (solver->auto_solve)
    simplex_solver_resolve (solver);
}

Constraint *
simplex_solver_add_constraint (SimplexSolver *solver,
                               Expression *expression,
                               OperatorType op,
                               StrengthType strength)
{
  Constraint *res;

  res = g_slice_new (Constraint);
  res->solver = solver;
  res->expression = expression_ref (expression);
  res->op_type = op;
  res->strength = strength;
  res->is_stay = false;
  res->is_edit = false;

  simplex_solver_add_constraint_internal (solver, res);

  return res;
}

Constraint *
simplex_solver_add_stay_constraint (SimplexSolver *solver,
                                    Variable *variable,
                                    StrengthType strength)
{
  Constraint *res;

  res = g_slice_new (Constraint);
  res->solver = solver;
  res->expression = expression_new_from_variable (variable);
  res->op_type = OPERATOR_TYPE_EQ;
  res->strength = strength;
  res->is_stay = true;
  res->is_edit = false;

  simplex_solver_add_constraint_internal (solver, res);

  return res;
}

Constraint *
simplex_solver_add_edit_constraint (SimplexSolver *solver,
                                    Variable *variable,
                                    StrengthType strength)
{
  Constraint *res;

  res = g_slice_new (Constraint);
  res->solver = solver;
  res->expression = expression_new_from_variable (variable);
  res->op_type = OPERATOR_TYPE_EQ;
  res->strength = strength;
  res->is_stay = false;
  res->is_edit = true;

  simplex_solver_add_constraint_internal (solver, res);

  return res;
}

void
simplex_solver_remove_constraint (SimplexSolver *solver,
                                  Constraint *constraint)
{
  constraint_unref (constraint);
}

void
simplex_solver_suggest_value (SimplexSolver *solver,
                              Variable *variable,
                              double value)
{
}

void
simplex_solver_resolve (SimplexSolver *solver)
{
}

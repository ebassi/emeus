/* emeus-simplex-solver.c: The constraint solver
 *
 * Copyright 2016  Endless
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include "emeus-simplex-solver-private.h"

#include "emeus-types-private.h"
#include "emeus-variable-private.h"
#include "emeus-expression-private.h"

#include <glib.h>
#include <string.h>
#include <math.h>
#include <float.h>

typedef struct {
  Constraint *constraint;

  Variable *eplus;
  Variable *eminus;

  double prev_edit_constraint;

  int index_;
} EditInfo;

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

  solver->stay_plus_error_vars = g_ptr_array_new ();
  solver->stay_minus_error_vars = g_ptr_array_new ();

  /* The rows table owns the objective variable */
  solver->objective = variable_new (solver, VARIABLE_OBJECTIVE);
  g_hash_table_insert (solver->rows, solver->objective, expression_new (solver, 0.0));

  solver->needs_solving = false;
  solver->auto_solve = false;
}

void
simplex_solver_clear (SimplexSolver *solver)
{
  g_clear_pointer (&solver->columns, g_hash_table_unref);
  g_clear_pointer (&solver->rows, g_hash_table_unref);
  g_clear_pointer (&solver->external_rows, g_hash_table_unref);
  g_clear_pointer (&solver->infeasible_rows, g_hash_table_unref);
  g_clear_pointer (&solver->updated_externals, g_hash_table_unref);

  g_clear_pointer (&solver->stay_plus_error_vars, g_ptr_array_unref);
  g_clear_pointer (&solver->stay_minus_error_vars, g_ptr_array_unref);
}

static GHashTable *
simplex_solver_get_column_set (SimplexSolver *solver,
                               Variable *param_var)
{
  return g_hash_table_lookup (solver->columns, param_var);
}

static bool
simplex_solver_column_has_key (SimplexSolver *solver,
                               Variable *subject)
{
  return g_hash_table_lookup (solver->columns, subject) != NULL;
}

static void
simplex_solver_insert_column_variable (SimplexSolver *solver,
                                       Variable *param_var,
                                       Variable *row_var)
{
  GHashTable *row_set;

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

static void
simplex_solver_insert_error_variable (SimplexSolver *solver,
                                      Constraint *constraint,
                                      Variable *variable)
{
  GHashTable *constraint_set;

  constraint_set = g_hash_table_lookup (solver->error_vars, constraint);
  if (constraint_set == NULL)
    {
      constraint_set = g_hash_table_new (NULL, NULL);
      g_hash_table_insert (solver->error_vars, constraint, constraint_set);
    }

  g_hash_table_add (constraint_set, variable);
}

static void
simplex_solver_reset_stay_constraints (SimplexSolver *solver)
{
  int i;

  for (i = 0; i < solver->stay_plus_error_vars->len; i++)
    {
      Variable *variable = g_ptr_array_index (solver->stay_plus_error_vars, i);
      Expression *expression;

      expression = g_hash_table_lookup (solver->rows, variable);
      if (expression == NULL)
        {
          variable = g_ptr_array_index (solver->stay_minus_error_vars, i);
          expression = g_hash_table_lookup (solver->rows, variable);
        }

      if (expression != NULL)
        expression_set_constant (expression, 0.0);
    }
}

static void
simplex_solver_set_external_variables (SimplexSolver *solver)
{
  GHashTableIter iter;
  gpointer key_p;

  g_hash_table_iter_init (&iter, solver->updated_externals);
  while (g_hash_table_iter_next (&iter, &key_p, NULL))
    {
      Variable *variable = key_p;
      Expression *expression;

      expression = g_hash_table_lookup (solver->external_rows, variable);
      if (expression == NULL)
        {
          variable_set_value (variable, 0.0);
          return;
        }

      variable_set_value (variable, expression_get_constant (expression));
    }

  g_hash_table_remove_all (solver->updated_externals);
  solver->needs_solving = false;
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
  GHashTableIter iter;
  gpointer key_p;

  if (row_set == NULL)
    goto out;

  g_hash_table_iter_init (&iter, row_set);
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

out:
  if (variable_is_external (old_variable))
    g_hash_table_insert (solver->external_rows, variable_ref (old_variable), expression);

  g_hash_table_remove (solver->columns, old_variable);
}

static void
simplex_solver_pivot (SimplexSolver *solver,
                      Variable *entry_var,
                      Variable *exit_var)
{
  Expression *expr;

  expr = simplex_solver_remove_row (solver, exit_var);
  expression_change_subject (expr, exit_var, entry_var);

  simplex_solver_substitute_out (solver, entry_var, expr);
  simplex_solver_add_row (solver, entry_var, expr);
}

typedef struct {
  double objective_coefficient;
  Variable *entry_variable;
} NegativeClosure;

static void
find_negative_coefficient (Term *term,
                           gpointer data_)
{
  Variable *variable = term_get_variable (term);
  double coefficient = term_get_coefficient (term);
  NegativeClosure *data = data_;

  if (variable_is_pivotable (variable) && coefficient < data->objective_coefficient)
    {
      data->objective_coefficient = coefficient;
      data->entry_variable = variable;
    }
}

static void
simplex_solver_optimize (SimplexSolver *solver,
                         Variable *z)
{
  Variable *entry, *exit;
  Expression *z_row;

  solver->optimize_count += 1;

  z_row = g_hash_table_lookup (solver->rows, z);
  g_assert (z_row != NULL);

  entry = exit = NULL;

  while (true)
    {
      NegativeClosure data;
      GHashTable *column_vars;
      GHashTableIter iter;
      gpointer key_p;
      double min_ratio;
      double r;

      data.objective_coefficient = 0.0;
      data.entry_variable = NULL;

      expression_terms_foreach (z_row, find_negative_coefficient, &data);

      if (data.objective_coefficient >= -DBL_EPSILON)
        return;

      entry = data.entry_variable;

      min_ratio = DBL_MAX;
      r = 0;

      column_vars = simplex_solver_get_column_set (solver, entry);
      g_hash_table_iter_init (&iter, column_vars);
      while (g_hash_table_iter_next (&iter, &key_p, NULL))
        {
          Variable *v = key_p;
          Expression *expr;
          double coeff;

          if (!variable_is_pivotable (v))
            continue;

          expr = g_hash_table_lookup (solver->rows, v);
          coeff = expression_get_coefficient (expr, entry);
          if (coeff < 0.0)
            {
              r = -1.0 * expression_get_constant (expr) / coeff;
              if (r < min_ratio ||
                  (fabs (r - min_ratio) < DBL_EPSILON && v < entry))
                {
                  min_ratio = r;
                  exit = v;
                }
            }
        }

      if (fabs (min_ratio - DBL_MAX) < DBL_EPSILON)
        g_critical ("Unbounded objective variable during optimization");
      else
        simplex_solver_pivot (solver, entry, exit);
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
  if (variable_is_external (variable))
    g_hash_table_add (solver->external_vars, variable);
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

static void
update_externals (Term *term,
                  gpointer data)
{
  Variable *variable = term_get_variable (term);
  SimplexSolver *solver = data;

  if (variable_is_external (variable))
    simplex_solver_update_variable (solver, variable);
}

Constraint *
simplex_solver_add_constraint (SimplexSolver *solver,
                               Variable *variable,
                               OperatorType op,
                               Expression *expression,
                               StrengthType strength)
{
  Constraint *res;
  Expression *expr;

  /* Turn:
   *
   *   attr OP expression
   *
   * into:
   *
   *   attr - expression OP 0
   */
  expr = expression_new_from_variable (variable);
  expression_add_expression (expr, expression, -1.0, NULL);
  expression_unref (expression);

  expression_terms_foreach (expression, update_externals, solver);

  res = g_slice_new (Constraint);
  res->solver = solver;
  res->expression = expr;
  res->op_type = op;
  res->strength = strength;
  res->is_stay = false;
  res->is_edit = false;

  simplex_solver_add_constraint_internal (solver, res);

  return res;
}

Constraint *
simplex_solver_add_stay_variable (SimplexSolver *solver,
                                  Variable *variable,
                                  StrengthType strength)
{
  Constraint *res;
  Expression *expr;

  /* Turn stay constraint from:
   *
   *   attr == value
   *
   * into:
   *
   *   attr - value == 0
   */
  expr = expression_new_from_variable (variable);
  expression_plus (expr, variable_get_value (variable) * -1.0);

  res = g_slice_new (Constraint);
  res->solver = solver;
  res->expression = expr;
  res->op_type = OPERATOR_TYPE_EQ;
  res->strength = strength;
  res->is_stay = true;
  res->is_edit = false;

  simplex_solver_add_constraint_internal (solver, res);

  return res;
}

bool
simplex_solver_has_stay_variable (SimplexSolver *solver,
                                  Variable *variable)
{
  return false;
}

Constraint *
simplex_solver_add_edit_variable (SimplexSolver *solver,
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

bool
simplex_solver_has_edit_variable (SimplexSolver *solver,
                                  Variable *variable)
{
  return false;
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
  if (!solver->needs_solving)
    return;

  solver->needs_solving = false;
}

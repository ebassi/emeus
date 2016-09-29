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
#include "emeus-utils-private.h"

#include <glib.h>
#include <string.h>
#include <math.h>
#include <float.h>

typedef struct {
  Constraint *constraint;

  Variable *eplus;
  Variable *eminus;

  double prev_constant;
} EditInfo;

typedef struct {
  Constraint *constraint;
} StayInfo;

typedef struct {
  /* HashSet<Variable>, owns a reference */
  GHashTable *set;
} ColumnSet;

static const char *operators[] = {
  "<=",
  "==",
  ">=",
};

static const char *
operator_to_string (OperatorType o)
{
  return operators[o + 1];
}

static const char *
strength_to_string (StrengthType s)
{
  if (s >= STRENGTH_REQUIRED)
    return "required";

  if (s >= STRENGTH_STRONG)
    return "strong";

  if (s >= STRENGTH_MEDIUM)
    return "medium";

  return "weak";
}

static void
constraint_free (Constraint *constraint)
{
  if (constraint != NULL)
    {
      expression_unref (constraint->expression);

      if (constraint->is_edit || constraint->is_stay)
        {
          g_assert (constraint->variable != NULL);
          variable_unref (constraint->variable);
        }

      g_slice_free (Constraint, constraint);
    }
}

static char *
constraint_to_string (const Constraint *constraint)
{
  GString *buf = g_string_new (NULL);
  char *str;

  if (constraint->is_stay)
    g_string_append (buf, "[stay]");

  if (constraint->is_edit)
    g_string_append (buf, "[edit]");

  str = expression_to_string (constraint->expression);
  g_string_append (buf, str);
  g_free (str);

  g_string_append (buf, " ");
  g_string_append (buf, operator_to_string (constraint->op_type));
  g_string_append (buf, " 0.0 ");

  g_string_append_printf (buf, "[strength:%s]", strength_to_string (constraint->strength));

  return g_string_free (buf, FALSE);
}

static void
edit_info_free (gpointer data)
{
  g_slice_free (EditInfo, data);
}

static void
stay_info_free (gpointer data)
{
  g_slice_free (StayInfo, data);
}

static void
column_set_free (gpointer data)
{
  ColumnSet *column_set = data;

  if (data == NULL)
    return;

  g_hash_table_unref (column_set->set);
  g_slice_free (ColumnSet, column_set);
}

static ColumnSet *
column_set_new (void)
{
  ColumnSet *res = g_slice_new (ColumnSet);

  res->set = g_hash_table_new_full (NULL, NULL, (GDestroyNotify) variable_unref, NULL);

  return res;
}

static void
column_set_add_variable (ColumnSet *set,
                         Variable *variable)
{
  g_hash_table_add (set->set, variable_ref (variable));
}

static void
column_set_remove_variable (ColumnSet *set,
                            Variable *variable)
{
  g_hash_table_remove (set->set, variable);
}

static void
column_set_iter_init (ColumnSet *set,
                      GHashTableIter *iter)
{
  g_hash_table_iter_init (iter, set->set);
}

static int
column_set_get_size (ColumnSet *set)
{
  return g_hash_table_size (set->set);
}

void
simplex_solver_init (SimplexSolver *solver)
{
  if (solver->initialized)
    {
      g_critical ("The SimplexSolver %p has already been initialized");
      return;
    }

  memset (solver, 0, sizeof (SimplexSolver));

  /* HashTable<Variable, ColumnSet>; owns keys and values */
  solver->columns = g_hash_table_new_full (NULL, NULL,
                                           (GDestroyNotify) variable_unref,
                                           column_set_free);

  /* HashTable<Variable, Expression>; owns keys and values */
  solver->rows = g_hash_table_new_full (NULL, NULL,
                                        (GDestroyNotify) variable_unref,
                                        (GDestroyNotify) expression_unref);

  /* HashSet<Variable>; owns keys */
  solver->external_rows = g_hash_table_new_full (NULL, NULL,
                                                 (GDestroyNotify) variable_unref,
                                                 NULL);

  /* HashSet<Variable>; owns keys */
  solver->infeasible_rows = g_hash_table_new_full (NULL, NULL,
                                                   (GDestroyNotify) variable_unref,
                                                   NULL);

  /* HashSet<Variable>; owns keys */
  solver->external_parametric_vars = g_hash_table_new_full (NULL, NULL,
                                                            (GDestroyNotify) variable_unref,
                                                            NULL);

  /* Vec<Variable> */
  solver->stay_plus_error_vars = g_ptr_array_new ();
  solver->stay_minus_error_vars = g_ptr_array_new ();

  /* HashTable<Constraint, HashSet<Variable>> */
  solver->error_vars = g_hash_table_new_full (NULL, NULL,
                                              NULL,
                                              (GDestroyNotify) g_hash_table_unref);

  /* HashTable<Constraint, Variable> */
  solver->marker_vars = g_hash_table_new_full (NULL, NULL,
                                               NULL,
                                               (GDestroyNotify) variable_unref);

  /* HashTable<Variable, EditInfo>; does not own keys, but owns values */
  solver->edit_var_map = g_hash_table_new_full (NULL, NULL,
                                                NULL,
                                                edit_info_free);

  /* HashTable<Variable, StayInfo>; does not own keys, but owns values */
  solver->stay_var_map = g_hash_table_new_full (NULL, NULL,
                                                NULL,
                                                stay_info_free);

  /* The rows table owns the objective variable */
  solver->objective = variable_new (solver, VARIABLE_OBJECTIVE);
  variable_set_name (solver->objective, "Z");
  g_hash_table_insert (solver->rows, solver->objective, expression_new (solver, 0.0));

  solver->slack_counter = 0;
  solver->dummy_counter = 0;
  solver->artificial_counter = 0;

  solver->needs_solving = false;
  solver->auto_solve = true;
  solver->initialized = true;
}

void
simplex_solver_clear (SimplexSolver *solver)
{
  if (!solver->initialized)
    return;

  solver->initialized = false;

#ifdef EMEUS_ENABLE_DEBUG
  {
    g_print ("Solver [%p]:\n"
             "- Rows: %d, Columns: %d\n"
             "- Slack variables: %d\n"
             "- Error variables: %d (plus: %d, minus: %d)\n"
             "- Marker variables: %d\n"
             "- Infeasible rows: %d\n"
             "- External rows: %d\n"
             "- Edit: %d, Stay: %d\n",
             solver,
             g_hash_table_size (solver->rows),
             g_hash_table_size (solver->columns),
             solver->slack_counter,
             g_hash_table_size (solver->error_vars),
             solver->stay_plus_error_vars->len,
             solver->stay_minus_error_vars->len,
             g_hash_table_size (solver->marker_vars),
             g_hash_table_size (solver->infeasible_rows),
             g_hash_table_size (solver->external_rows),
             g_hash_table_size (solver->edit_var_map),
             g_hash_table_size (solver->stay_var_map));

    GHashTableIter iter;
    gpointer key_p, value_p;

    g_print ("Tableau:\n");

    g_hash_table_iter_init (&iter, solver->rows);
    while (g_hash_table_iter_next (&iter, &key_p, &value_p))
      {
        Variable *v = key_p;
        Expression *e = value_p;

        char *str1 = variable_to_string (v);
        char *str2 = expression_to_string (e);

        g_print ("- %s <=> %s\n", str1, str2);

        g_free (str1);
        g_free (str2);
      }
  }
#endif

  solver->objective = NULL;

  solver->needs_solving = false;
  solver->auto_solve = true;

  solver->slack_counter = 0;
  solver->dummy_counter = 0;
  solver->artificial_counter = 0;

  g_clear_pointer (&solver->stay_plus_error_vars, g_ptr_array_unref);
  g_clear_pointer (&solver->stay_minus_error_vars, g_ptr_array_unref);

  g_clear_pointer (&solver->external_rows, g_hash_table_unref);
  g_clear_pointer (&solver->infeasible_rows, g_hash_table_unref);
  g_clear_pointer (&solver->external_parametric_vars, g_hash_table_unref);
  g_clear_pointer (&solver->error_vars, g_hash_table_unref);
  g_clear_pointer (&solver->marker_vars, g_hash_table_unref);
  g_clear_pointer (&solver->edit_var_map, g_hash_table_unref);
  g_clear_pointer (&solver->stay_var_map, g_hash_table_unref);

  /* The columns need to be deleted last, for reference counting */
  g_clear_pointer (&solver->rows, g_hash_table_unref);
  g_clear_pointer (&solver->columns, g_hash_table_unref);
}

static ColumnSet *
simplex_solver_get_column_set (SimplexSolver *solver,
                               Variable *param_var)
{
  if (!solver->initialized)
    return NULL;

  return g_hash_table_lookup (solver->columns, param_var);
}

static bool
simplex_solver_column_has_key (SimplexSolver *solver,
                               Variable *subject)
{
  if (!solver->initialized)
    return false;

  return g_hash_table_contains (solver->columns, subject);
}

static void
simplex_solver_insert_column_variable (SimplexSolver *solver,
                                       Variable *param_var,
                                       Variable *row_var)
{
  ColumnSet *set;

  if (!solver->initialized)
    return;

  set = simplex_solver_get_column_set (solver, param_var);
  if (set == NULL)
    {
      set = column_set_new ();
      g_hash_table_insert (solver->columns, variable_ref (param_var), set);
    }

  if (row_var != NULL)
    column_set_add_variable (set, row_var);
}

static void
simplex_solver_insert_error_variable (SimplexSolver *solver,
                                      Constraint *constraint,
                                      Variable *variable)
{
  GHashTable *constraint_set;

  if (!solver->initialized)
    return;

  constraint_set = g_hash_table_lookup (solver->error_vars, constraint);
  if (constraint_set == NULL)
    {
      constraint_set = g_hash_table_new (NULL, NULL);
      g_hash_table_insert (solver->error_vars, constraint, constraint_set);
    }

  g_hash_table_add (constraint_set, variable);
}

static void
simplex_solver_reset_stay_constants (SimplexSolver *solver)
{
  int i;

  g_assert (solver->stay_plus_error_vars->len == solver->stay_minus_error_vars->len);

  for (i = 0; i < solver->stay_plus_error_vars->len; i++)
    {
      Variable *p_var = g_ptr_array_index (solver->stay_plus_error_vars, i);
      Variable *m_var = g_ptr_array_index (solver->stay_minus_error_vars, i);
      Expression *expression;

      expression = g_hash_table_lookup (solver->rows, p_var);
      if (expression == NULL)
        expression = g_hash_table_lookup (solver->rows, m_var);

      if (expression != NULL)
        expression_set_constant (expression, 0.0);
    }
}

static void
simplex_solver_set_external_variables (SimplexSolver *solver)
{
  GHashTableIter iter;
  gpointer key_p;

  g_hash_table_iter_init (&iter, solver->external_parametric_vars);
  while (g_hash_table_iter_next (&iter, &key_p, NULL))
    {
      Variable *variable = key_p;

      if (g_hash_table_contains (solver->rows, variable))
        continue;

      variable_set_value (variable, 0.0);
    }

  g_hash_table_iter_init (&iter, solver->external_rows);
  while (g_hash_table_iter_next (&iter, &key_p, NULL))
    {
      Variable *variable = key_p;
      Expression *expression;

      expression = g_hash_table_lookup (solver->rows, variable);

      variable_set_value (variable, expression_get_constant (expression));
    }

  solver->needs_solving = false;
}

typedef struct {
  SimplexSolver *solver;
  Variable *subject;
} ForeachClosure;

static bool
insert_expression_columns (Term *term,
                           gpointer data_)
{
  Variable *variable = term_get_variable (term);
  ForeachClosure *data = data_;

  simplex_solver_insert_column_variable (data->solver,
                                         variable,
                                         data->subject);

  if (variable_is_external (variable))
    g_hash_table_add (data->solver->external_parametric_vars, variable_ref (variable));

  return true;
}

static void
simplex_solver_add_row (SimplexSolver *solver,
                        Variable *variable,
                        Expression *expression)
{
  ForeachClosure data;

  if (!solver->initialized)
    return;

  g_hash_table_insert (solver->rows, variable_ref (variable), expression_ref (expression));

  data.subject = variable;
  data.solver = solver;
  expression_terms_foreach (expression,
                            insert_expression_columns,
                            &data);

  if (variable_is_external (variable))
    g_hash_table_add (solver->external_rows, variable_ref (variable));
}

static void
simplex_solver_remove_column (SimplexSolver *solver,
                              Variable *variable)
{
  ColumnSet *set;
  GHashTableIter iter;
  gpointer key_p;

  set = g_hash_table_lookup (solver->columns, variable);
  if (set == NULL)
    goto out;

  column_set_iter_init (set, &iter);
  while (g_hash_table_iter_next (&iter, &key_p, NULL))
    {
      Variable *v = key_p;
      Expression *e = g_hash_table_lookup (solver->rows, v);

      expression_remove_variable (e, variable, NULL);
    }

out:
  if (variable_is_external (variable))
    {
      g_hash_table_remove (solver->external_rows, variable);
      g_hash_table_remove (solver->external_parametric_vars, variable);
    }
}

static bool
remove_expression_columns (Term *term,
                           gpointer data_)
{
  ForeachClosure *data = data_;
  ColumnSet *set = g_hash_table_lookup (data->solver->columns, term_get_variable (term));

  if (set != NULL)
    column_set_remove_variable (set, data->subject);

  return true;
}

static Expression *
simplex_solver_remove_row (SimplexSolver *solver,
                           Variable *variable)
{
  Expression *e;
  ForeachClosure data;

  if (!solver->initialized)
    return NULL;

  e = g_hash_table_lookup (solver->rows, variable);
  g_assert (e != NULL);

  expression_ref (e);

  data.solver = solver;
  data.subject = variable;
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
simplex_solver_substitute_out (SimplexSolver *solver,
                               Variable *old_variable,
                               Expression *expression)
{
  ColumnSet *set;

  if (!solver->initialized)
    return;

  set = g_hash_table_lookup (solver->columns, old_variable);
  if (set != NULL)
    {
      GHashTableIter iter;
      gpointer key_p;

      column_set_iter_init (set, &iter);
      while (g_hash_table_iter_next (&iter, &key_p, NULL))
        {
          Variable *variable = key_p;
          Expression *e = g_hash_table_lookup (solver->rows, variable);

          expression_substitute_out (e, old_variable, expression, variable);

          if (variable_is_restricted (variable) && expression_get_constant (e) < 0)
            g_hash_table_add (solver->infeasible_rows, variable_ref (variable));
        }
    }

  if (variable_is_external (old_variable))
    {
      g_hash_table_add (solver->external_rows, variable_ref (old_variable));
      g_hash_table_remove (solver->external_parametric_vars, old_variable);
    }

  g_hash_table_remove (solver->columns, old_variable);
}

static void
simplex_solver_pivot (SimplexSolver *solver,
                      Variable *entry_var,
                      Variable *exit_var)
{
  Expression *expr;

  if (!solver->initialized)
    return;

  if (entry_var == NULL)
    g_critical ("No entry variable for pivot");
  if (exit_var == NULL)
    g_critical ("No exit variable for pivot");

  expr = simplex_solver_remove_row (solver, exit_var);
  expression_change_subject (expr, exit_var, entry_var);

  simplex_solver_substitute_out (solver, entry_var, expr);
  simplex_solver_add_row (solver, entry_var, expr);

  expression_unref (expr);
}

typedef struct {
  double objective_coefficient;
  Variable *entry_variable;
} NegativeClosure;

static bool
find_negative_coefficient (Term *term,
                           gpointer data_)
{
  Variable *variable = term_get_variable (term);
  double coefficient = term_get_coefficient (term);
  NegativeClosure *data = data_;

  if (variable_is_pivotable (variable) && coefficient < data->objective_coefficient)
    {
      /* Stop at the first negative coefficient */
      data->objective_coefficient = coefficient;
      data->entry_variable = variable;
      return false;
    }

  return true;
}

static void
simplex_solver_optimize (SimplexSolver *solver,
                         Variable *z)
{
  Variable *entry, *exit;
  Expression *z_row;

  if (!solver->initialized)
    return;

  z_row = g_hash_table_lookup (solver->rows, z);
  g_assert (z_row != NULL);

  entry = exit = NULL;

  solver->optimize_count += 1;

#ifdef EMEUS_ENABLE_DEBUG
  gint64 start_time = g_get_monotonic_time ();
#endif

  while (true)
    {
      NegativeClosure data;
      ColumnSet *column_vars;
      GHashTableIter iter;
      gpointer key_p;
      double min_ratio;
      double r;

      data.objective_coefficient = 0.0;
      data.entry_variable = NULL;

      expression_terms_foreach (z_row, find_negative_coefficient, &data);

      if (data.objective_coefficient >= -DBL_EPSILON || data.entry_variable == NULL)
        break;

      entry = data.entry_variable;

      min_ratio = DBL_MAX;
      r = 0;

      column_vars = simplex_solver_get_column_set (solver, entry);
      if (column_vars == NULL)
        break;

      column_set_iter_init (column_vars, &iter);
      while (g_hash_table_iter_next (&iter, &key_p, NULL))
        {
          Variable *v = key_p;

          if (variable_is_pivotable (v))
            {
              Expression *expr = g_hash_table_lookup (solver->rows, v);
              double coeff = expression_get_coefficient (expr, entry);
              if (coeff < 0.0)
                {
                  r = -1.0 * expression_get_constant (expr) / coeff;
                  if (r < min_ratio)
                    {
                      min_ratio = r;
                      exit = v;
                    }
                }
            }
        }

      if (approx_val (min_ratio, DBL_MAX))
        g_critical ("Unbounded objective variable during optimization");
      else
        simplex_solver_pivot (solver, entry, exit);
    }

#ifdef EMEUS_ENABLE_DEBUG
  g_print ("optimize.time := %.3f us (pass:%d)\n",
           (float) (g_get_monotonic_time () - start_time),
           solver->optimize_count);
#endif
}

typedef struct {
  SimplexSolver *solver;
  Expression *expr;
} ReplaceClosure;

static bool
replace_terms (Term *term,
               gpointer data_)
{
  Variable *v = term_get_variable (term);
  double c = term_get_coefficient (term);
  ReplaceClosure *data = data_;

  Expression *e = g_hash_table_lookup (data->solver->rows, v);

  if (e == NULL)
    expression_add_variable (data->expr, v, c, NULL);
  else
    expression_add_expression (data->expr, e, c, NULL);

  return true;
}

static Expression *
simplex_solver_normalize_expression (SimplexSolver *solver,
                                     Constraint *constraint,
                                     Variable **eplus_p,
                                     Variable **eminus_p,
                                     double *prev_constant_p)
{
  Expression *cn_expr = constraint->expression;
  Expression *expr;
  Variable *slack_var, *dummy_var;
  Variable *eplus, *eminus;
  ReplaceClosure data;

  if (eplus_p != NULL)
    *eplus_p = NULL;
  if (eminus_p != NULL)
    *eminus_p = NULL;
  if (prev_constant_p != NULL)
    *prev_constant_p = 0.0;

  expr = expression_new (solver, expression_get_constant (cn_expr));

  data.solver = solver;
  data.expr = expr;
  expression_terms_foreach (cn_expr, replace_terms, &data);

  if (constraint_is_inequality (constraint))
    {
      /* If the constraint is an inequality, we add a slack variable to
       * turn it into an equality, e.g. from
       *
       *   expr >= 0
       *
       * to
       *
       *   expr - slack = 0
       *
       * Additionally, if the constraint is not required we add an
       * error variable:
       *
       *   expr - slack + error = 0
       */
      solver->slack_counter += 1;

      slack_var = variable_new (solver, VARIABLE_SLACK);
      expression_set_variable (expr, slack_var, -1.0);

      g_hash_table_insert (solver->marker_vars, constraint, slack_var);

      if (!constraint_is_required (constraint))
        {
          Expression *z_row;

          solver->slack_counter += 1;

          eminus = variable_new (solver, VARIABLE_SLACK);
          variable_set_name (eminus, "em");
          expression_set_variable (expr, eminus, 1.0);
          variable_unref (eminus);

          z_row = g_hash_table_lookup (solver->rows, solver->objective);
          expression_set_variable (z_row, eminus, constraint->strength);

          simplex_solver_insert_error_variable (solver, constraint, eminus);
          simplex_solver_note_added_variable (solver, eminus, solver->objective);
        }
    }
  else 
    {
      if (constraint_is_required (constraint))
        {
          /* If the constraint is required, we use a dummy marker variable;
           * the dummy won't be allowed to enter the basis of the tableau
           * when pivoting.
           */
          solver->dummy_counter += 1;

          dummy_var = variable_new (solver, VARIABLE_DUMMY);

          if (eplus_p != NULL)
            *eplus_p = dummy_var;
          if (eminus_p != NULL)
            *eminus_p = dummy_var;
          if (prev_constant_p != NULL)
            *prev_constant_p = expression_get_constant (cn_expr);

          expression_set_variable (expr, dummy_var, 1.0);
          g_hash_table_insert (solver->marker_vars, constraint, dummy_var);
        }
      else
        {
          Expression *z_row;

          /* Since the constraint is a non-required equality, we need to
           * add error variables around it, i.e. turn it from:
           *
           *   expr = 0
           *
           * to:
           *
           *   expr - eplus + eminus = 0
           */
          solver->slack_counter += 1;

          eplus = variable_new (solver, VARIABLE_SLACK);
          variable_set_name (eplus, "ep");
          eminus = variable_new (solver, VARIABLE_SLACK);
          variable_set_name (eminus, "em");

          expression_set_variable (expr, eplus, -1.0);
          expression_set_variable (expr, eminus, 1.0);

          g_hash_table_insert (solver->marker_vars, constraint, eplus);

          z_row = g_hash_table_lookup (solver->rows, solver->objective);
          expression_set_variable (z_row, eplus, constraint->strength);
          expression_set_variable (z_row, eminus, constraint->strength);
          simplex_solver_note_added_variable (solver, eplus, solver->objective);
          simplex_solver_note_added_variable (solver, eminus, solver->objective);

          simplex_solver_insert_error_variable (solver, constraint, eplus);
          simplex_solver_insert_error_variable (solver, constraint, eminus);

          if (constraint_is_stay (constraint))
            {
              g_ptr_array_add (solver->stay_plus_error_vars, eplus);
              g_ptr_array_add (solver->stay_minus_error_vars, eminus);
            }
          else if (constraint_is_edit (constraint))
            {
              if (eplus_p != NULL)
                *eplus_p = eplus;
              if (eminus_p != NULL)
                *eminus_p = eminus;
              if (prev_constant_p != NULL)
                *prev_constant_p = expression_get_constant (cn_expr);
            }
        }
    }

  if (expression_get_constant (expr) < 0.0)
    expression_times (expr, -1.0);

  return expr;
}

typedef struct {
  Variable *entry;
  double ratio;
  Expression *z_row;
} RatioClosure;

static bool
find_ratio (Term *term,
            gpointer data_)
{
  Variable *v = term_get_variable (term);
  double cd = term_get_coefficient (term);
  RatioClosure *data = data_;

  if (cd > 0.0 && variable_is_pivotable (v))
    {
      double zc = expression_get_coefficient (data->z_row, v);
      double r = zc / cd;

      if (r < data->ratio)
        {
          data->entry = v;
          data->ratio = r;
        }
    }

  return true;
}

static void
simplex_solver_dual_optimize (SimplexSolver *solver)
{
  Expression *z_row = g_hash_table_lookup (solver->rows, solver->objective);
  GHashTableIter iter;
  gpointer key_p, value_p;

#ifdef EMEUS_ENABLE_DEBUG
  gint64 start_time = g_get_monotonic_time ();
#endif

  g_hash_table_iter_init (&iter, solver->infeasible_rows);
  while (g_hash_table_iter_next (&iter, &key_p, &value_p))
    {
      Variable *entry_var, *exit_var;
      Expression *expr;
      RatioClosure data;

      exit_var = variable_ref (key_p);
      g_hash_table_iter_remove (&iter);

      expr = g_hash_table_lookup (solver->rows, exit_var);
      if (expr == NULL)
        {
          variable_unref (exit_var);
          continue;
        }

      if (expression_get_constant (expr) >= 0.0)
        {
          variable_unref (exit_var);
          continue;
        }

      data.ratio = DBL_MAX;
      data.entry = NULL;
      data.z_row = z_row;
      expression_terms_foreach (expr, find_ratio, &data);

      entry_var = data.entry;

      if (entry_var != NULL && !approx_val (data.ratio, DBL_MAX))
        simplex_solver_pivot (solver, entry_var, exit_var);
    }

#ifdef EMEUS_ENABLE_DEBUG
  g_print ("dual_optimize.time := %.3f us\n", (float) (g_get_monotonic_time () - start_time));
#endif
}

static void
simplex_solver_delta_edit_constant (SimplexSolver *solver,
                                    double delta,
                                    Variable *plus_error_var,
                                    Variable *minus_error_var)
{
  Expression *plus_expr, *minus_expr;
  ColumnSet *column_set;
  GHashTableIter iter;
  gpointer key_p;

  if (!solver->initialized)
    return;
  
  plus_expr = g_hash_table_lookup (solver->rows, plus_error_var);
  if (plus_expr != NULL)
    {
      double new_constant = expression_get_constant (plus_expr) + delta;

      expression_set_constant (plus_expr, new_constant);

      if (new_constant < 0.0)
        g_hash_table_add (solver->infeasible_rows, variable_ref (plus_error_var));

      return;
    }

  minus_expr = g_hash_table_lookup (solver->rows, minus_error_var);
  if (minus_expr != NULL)
    {
      double new_constant = expression_get_constant (minus_expr) - delta;

      expression_set_constant (minus_expr, new_constant);

      if (new_constant < 0.0)
        g_hash_table_add (solver->infeasible_rows, variable_ref (minus_error_var));

      return;
    }

  column_set = g_hash_table_lookup (solver->columns, minus_error_var);
  if (column_set == NULL)
    {
      g_critical ("Columns are unset during delta edit");
      return;
    }

  column_set_iter_init (column_set, &iter);
  while (g_hash_table_iter_next (&iter, &key_p, NULL))
    {
      Variable *basic_var = key_p;
      Expression *expr;
      double c, new_constant;

      expr = g_hash_table_lookup (solver->rows, basic_var);
      c = expression_get_coefficient (expr, minus_error_var);

      new_constant = expression_get_constant (expr) + (c * delta);
      expression_set_constant (expr, new_constant);

      if (variable_is_restricted (basic_var) && new_constant < 0.0)
        g_hash_table_add (solver->infeasible_rows, variable_ref (basic_var));
    }
}

typedef struct {
  SimplexSolver *solver;
  Variable *subject;
  bool found_unrestricted;
  bool found_new_restricted;
  Variable *retval;
  double coefficient;
} ChooseClosure;

static bool
find_subject_restricted (Term *term,
                         gpointer data_)
{
  Variable *v = term_get_variable (term);
  double c = term_get_coefficient (term);
  ChooseClosure *data = data_;

  if (data->found_unrestricted)
    {
      if (!variable_is_restricted (v))
        {
          if (!simplex_solver_column_has_key (data->solver, v))
            {
              data->retval = v;
              return false;
            }
        }
    }
  else
    {
      if (variable_is_restricted (v))
        {
          if (!data->found_new_restricted && !variable_is_dummy (v) && c < 0.0)
            {
              ColumnSet *set = g_hash_table_lookup (data->solver->columns, v);

              if (set == NULL ||
                  (column_set_get_size (set) == 1 &&
                   simplex_solver_column_has_key (data->solver, data->solver->objective)))
                {
                  data->subject = v;
                  data->found_new_restricted = true;
                }
            }
        }
      else
        {
          data->subject = v;
          data->found_unrestricted = true;
        }
    }

  return true;
}

static bool
find_subject_dummy (Term *term,
                    gpointer data_)
{
  Variable *v = term_get_variable (term);
  double c = term_get_coefficient (term);
  ChooseClosure *data = data_;

  if (!variable_is_dummy (v))
    {
      data->retval = NULL;
      return false;
    }

  if (!simplex_solver_column_has_key (data->solver, v))
    {
      data->subject = v;
      data->coefficient = c;
    }

  return true;
}

static Variable *
simplex_solver_choose_subject (SimplexSolver *solver,
                               Expression *expression)
{
  ChooseClosure data;

  if (!solver->initialized)
    return NULL;

  data.solver = solver;
  data.subject = NULL;
  data.retval = NULL;
  data.found_unrestricted = false;
  data.found_new_restricted = false;
  data.coefficient = 0.0;
  expression_terms_foreach (expression, find_subject_restricted, &data);

  if (data.retval != NULL)
    return data.retval;

  if (data.subject != NULL)
    return data.subject;

  data.solver = solver;
  data.subject = NULL;
  data.retval = NULL;
  data.found_unrestricted = false;
  data.found_new_restricted = false;
  data.coefficient = 0.0;
  expression_terms_foreach (expression, find_subject_dummy, &data);

  if (data.retval != NULL)
    return data.retval;

  if (approx_val (expression_get_constant (expression), 0.0))
    {
      g_critical ("Unable to satisfy a required constraint");
      return NULL;
    }

  if (data.coefficient > 0.0)
    expression_times (expression, -1.0);

  return data.subject;
}

static bool
simplex_solver_try_adding_directly (SimplexSolver *solver,
                                    Expression *expression)
{
  Variable *subject;

  if (!solver->initialized)
    return false;

  subject = simplex_solver_choose_subject (solver, expression);
  if (subject == NULL)
    return false;

  expression_new_subject (expression, subject);
  if (simplex_solver_column_has_key (solver, subject))
    simplex_solver_substitute_out (solver, subject, expression);

  simplex_solver_add_row (solver, subject, expression);

  return true;
}

static void
simplex_solver_add_with_artificial_variable (SimplexSolver *solver,
                                             Expression *expression)
{
  Variable *av, *az;
  Expression *az_row;
  Expression *az_tableau_row;
  Expression *e;

  if (!solver->initialized)
    return;
  
  av = variable_new (solver, VARIABLE_SLACK);
  solver->artificial_counter += 1;

  az = variable_new (solver, VARIABLE_OBJECTIVE);
  variable_set_name (az, "az");

  az_row = expression_clone (expression);

  simplex_solver_add_row (solver, az, az_row);
  simplex_solver_add_row (solver, av, expression);
  simplex_solver_optimize (solver, az);

  variable_unref (av);
  variable_unref (az);
  expression_unref (az_row);

  az_tableau_row = g_hash_table_lookup (solver->rows, az);
  if (!approx_val (expression_get_constant (az_tableau_row), 0.0))
    {
      simplex_solver_remove_row (solver, az);
      simplex_solver_remove_column (solver, av);

      g_critical ("Unable to satisfy a required constraint");
      return;
    }

  e = g_hash_table_lookup (solver->rows, av);
  if (e != NULL)
    {
      Variable *entry_var;

      if (expression_is_constant (e))
        {
          simplex_solver_remove_row (solver, av);
          simplex_solver_remove_row (solver, az);
          return;
        }

      entry_var = expression_get_pivotable_variable (e);
      simplex_solver_pivot (solver, entry_var, av);
    }

  g_assert (!g_hash_table_contains (solver->rows, av));

  simplex_solver_remove_column (solver, av);
  simplex_solver_remove_row (solver, az);
}

void
simplex_solver_note_added_variable (SimplexSolver *solver,
                                    Variable *variable,
                                    Variable *subject)
{
  if (!solver->initialized)
    return;

  if (subject != NULL)
    simplex_solver_insert_column_variable (solver, variable, subject);
}

void
simplex_solver_note_removed_variable (SimplexSolver *solver,
                                      Variable *variable,
                                      Variable *subject)
{
  ColumnSet *set;

  if (!solver->initialized)
    return;

  set = g_hash_table_lookup (solver->columns, variable);
  if (set != NULL && subject != NULL)
    column_set_remove_variable (set, subject);
}

Variable *
simplex_solver_create_variable (SimplexSolver *solver)
{
  if (!solver->initialized)
    {
      g_critical ("SimplexSolver %p is not initialized.", solver);
      return NULL;
    }

  return variable_new (solver, VARIABLE_REGULAR);
}

Expression *
simplex_solver_create_expression (SimplexSolver *solver,
                                  double constant)
{
  if (!solver->initialized)
    {
      g_critical ("SimplexSolver %p is not initialized.", solver);
      return NULL;
    }

  return expression_new (solver, constant);
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

  if (!solver->initialized)
    {
      g_critical ("SimplexSolver %p has not been initialized.", solver);
      return NULL;
    }

  res = g_slice_new0 (Constraint);
  res->solver = solver;
  res->strength = strength;
  res->is_edit = false;
  res->is_stay = false;
  res->op_type = op;

  if (expression == NULL)
    res->expression = expression_new_from_variable (variable);
  else
    {
      res->expression = expression_ref (expression);

      switch (res->op_type)
        {
        case OPERATOR_TYPE_EQ:
          expression_add_variable (res->expression, variable, -1.0, NULL);
          break;

        case OPERATOR_TYPE_LE:
          expression_add_variable (res->expression, variable, -1.0, NULL);
          break;

        case OPERATOR_TYPE_GE:
          expression_times (res->expression, -1.0);
          expression_add_variable (res->expression, variable, 1.0, NULL);
          break;
        }
    }

  expr = simplex_solver_normalize_expression (solver, res, NULL, NULL, NULL);

  if (!simplex_solver_try_adding_directly (solver, expr))
    simplex_solver_add_with_artificial_variable (solver, expr);

  solver->needs_solving = true;

  if (solver->auto_solve)
    {
      simplex_solver_optimize (solver, solver->objective);
      simplex_solver_set_external_variables (solver);
    }

#ifdef EMEUS_ENABLE_DEBUG
  {
    char *str1 = constraint_to_string (res);
    char *str2 = expression_to_string (expr);

    g_print ("Adding constraint: %s (normalized expression: %s)\n", str1, str2);

    g_free (str1);
    g_free (str2);
  }
#endif

  expression_unref (expr);

  return res;
}

Constraint *
simplex_solver_add_stay_variable (SimplexSolver *solver,
                                  Variable *variable,
                                  StrengthType strength)
{
  Constraint *res;
  Expression *expr;
  StayInfo *si;

  if (!solver->initialized)
    {
      g_critical ("SimplexSolver %p has not been initialized.", solver);
      return NULL;
    }

  res = g_slice_new0 (Constraint);
  res->solver = solver;
  res->variable = variable_ref (variable);
  res->op_type = OPERATOR_TYPE_EQ;
  res->strength = strength;
  res->is_stay = true;
  res->is_edit = false;

  res->expression = expression_new (solver, variable_get_value (res->variable));
  expression_add_variable (res->expression, res->variable, -1.0, NULL);

  si = g_slice_new (StayInfo);
  si->constraint = res;
  g_hash_table_insert (solver->stay_var_map, res->variable, si);

  expr = simplex_solver_normalize_expression (solver, res, NULL, NULL, NULL);

  if (!simplex_solver_try_adding_directly (solver, expr))
    simplex_solver_add_with_artificial_variable (solver, expr);

  solver->needs_solving = true;

  if (solver->auto_solve)
    {
      simplex_solver_optimize (solver, solver->objective);
      simplex_solver_set_external_variables (solver);
    }

#ifdef EMEUS_ENABLE_DEBUG
  {
    char *str1 = variable_to_string (res->variable);
    char *str2 = expression_to_string (expr);

    g_print ("Adding stay variable '%s' = %g (normalized expression: %s)\n",
             str1,
             variable_get_value (res->variable),
             str2);

    g_free (str1);
    g_free (str2);
  }
#endif

  expression_unref (expr);

  return res;
}

bool
simplex_solver_has_stay_variable (SimplexSolver *solver,
                                  Variable *variable)
{
  if (!solver->initialized)
    return false;

  return g_hash_table_contains (solver->stay_var_map, variable);
}

Constraint *
simplex_solver_add_edit_variable (SimplexSolver *solver,
                                  Variable *variable,
                                  StrengthType strength)
{
  Constraint *res;
  Expression *expr;
  EditInfo *ei;

  if (!solver->initialized)
    return NULL;

  res = g_slice_new (Constraint);
  res->solver = solver;
  res->variable = variable_ref (variable);
  res->op_type = OPERATOR_TYPE_EQ;
  res->strength = strength;
  res->is_stay = false;
  res->is_edit = true;

  res->expression = expression_new (solver, variable_get_value (variable));
  expression_add_variable (res->expression, variable, -1.0, NULL);

  ei = g_slice_new (EditInfo);
  ei->constraint = res;
  ei->eplus = NULL;
  ei->eminus = NULL;
  ei->prev_constant = 0.0;

  expr = simplex_solver_normalize_expression (solver, res,
                                              &ei->eplus,
                                              &ei->eminus,
                                              &ei->prev_constant);
  g_hash_table_insert (solver->edit_var_map, variable, ei);

  if (!simplex_solver_try_adding_directly (solver, expr))
    simplex_solver_add_with_artificial_variable (solver, expr);

  solver->needs_solving = true;

  if (solver->auto_solve)
    {
      simplex_solver_optimize (solver, solver->objective);
      simplex_solver_set_external_variables (solver);
    }

#ifdef EMEUS_ENABLE_DEBUG
  {
    char *str1 = constraint_to_string (res);
    char *str2 = expression_to_string (expr);

    g_print ("Adding edit constraint: %s (normalized expression: %s)\n", str1, str2);

    g_free (str1);
    g_free (str2);
  }
#endif

  expression_unref (expr);

  return res;
}

bool
simplex_solver_has_edit_variable (SimplexSolver *solver,
                                  Variable *variable)
{
  if (!solver->initialized)
    return false;

  return g_hash_table_contains (solver->edit_var_map, variable);
}

void
simplex_solver_remove_constraint (SimplexSolver *solver,
                                  Constraint *constraint)
{
  Expression *z_row;
  GHashTable *error_vars;
  GHashTableIter iter;
  gpointer key_p;
  Variable *marker;

  if (!solver->initialized)
    return;

  solver->needs_solving = true;

  simplex_solver_reset_stay_constants (solver);

  z_row = g_hash_table_lookup (solver->rows, solver->objective);
  error_vars = g_hash_table_lookup (solver->error_vars, constraint);

  if (error_vars != NULL)
    {
      g_hash_table_iter_init (&iter, error_vars);
      while (g_hash_table_iter_next (&iter, &key_p, NULL))
        {
          Variable *v = key_p;
          Expression *e;

          e = g_hash_table_lookup (solver->rows, v);

          if (e == NULL)
            expression_add_variable (z_row,
                                     v,
                                     constraint->strength,
                                     solver->objective);
          else
            expression_add_expression (z_row,
                                       e,
                                       constraint->strength,
                                       solver->objective);
        }
    }

  marker = g_hash_table_lookup (solver->marker_vars, constraint);
  if (marker == NULL)
    {
      g_critical ("Constraint %p not found", constraint);
      return;
    }

  variable_ref (marker);
  g_hash_table_remove (solver->marker_vars, constraint);

  if (g_hash_table_lookup (solver->rows, marker) == NULL)
    {
      ColumnSet *set = g_hash_table_lookup (solver->columns, marker);
      Variable *exit_var = NULL;
      double min_ratio = 0;

      if (set == NULL)
        goto no_columns;

      column_set_iter_init (set, &iter);
      while (g_hash_table_iter_next (&iter, &key_p, NULL))
        {
          Variable *v = key_p;

          if (variable_is_restricted (v))
            {
              Expression *e = g_hash_table_lookup (solver->rows, v);
              double coeff = expression_get_coefficient (e, marker);

              if (coeff < 0.0)
                {
                  double r = -expression_get_constant (e) / coeff;

                  if (exit_var == NULL ||
                      r < min_ratio ||
                      approx_val (r, min_ratio))
                    {
                      min_ratio = r;
                      exit_var = v;
                    }
                }
            }
        }

      if (exit_var == NULL)
        {
          column_set_iter_init (set, &iter);
          while (g_hash_table_iter_next (&iter, &key_p, NULL))
            {
              Variable *v = key_p;

              if (variable_is_restricted (v))
                {
                  Expression *e = g_hash_table_lookup (solver->rows, v);
                  double coeff = expression_get_coefficient (e, marker);
                  double r = 0.0;
                  
                  if (!approx_val (coeff, 0.0))
                    r = expression_get_constant (e) / coeff;

                  if (exit_var == NULL || r < min_ratio)
                    {
                      min_ratio = r;
                      exit_var = v;
                    }
                }
            }
        }

      if (exit_var == NULL)
        {
          if (column_set_get_size (set) == 0)
            simplex_solver_remove_column (solver, marker);
          else
            {
              column_set_iter_init (set, &iter);
              while (g_hash_table_iter_next (&iter, &key_p, NULL))
                {
                  Variable *v = key_p;

                  if (v != solver->objective)
                    {
                      exit_var = v;
                      break;
                    }
                }
            }
        }

      if (exit_var != NULL)
        simplex_solver_pivot (solver, marker, exit_var);
    }

no_columns:
  if (g_hash_table_lookup (solver->rows, marker) == NULL)
    simplex_solver_remove_row (solver, marker);

  if (error_vars != NULL)
    {
      g_hash_table_iter_init (&iter, error_vars);
      while (g_hash_table_iter_next (&iter, &key_p, NULL))
        {
          Variable *v = key_p;

          if (v != marker)
            simplex_solver_remove_column (solver, v);
        }
    }

  if (constraint_is_stay (constraint))
    {
      if (error_vars != NULL)
        {
          int i = 0;

          for (i = 0; i < solver->stay_plus_error_vars->len; i++)
            {
              Variable *eplus = g_ptr_array_index (solver->stay_plus_error_vars, i);
              Variable *eminus = g_ptr_array_index (solver->stay_minus_error_vars, i);

              g_hash_table_remove (error_vars, eplus);
              g_hash_table_remove (error_vars, eminus);
            }
        }
    }
  else if (constraint_is_edit (constraint))
    {
      EditInfo *ei = g_hash_table_lookup (solver->edit_var_map, constraint->variable);

      simplex_solver_remove_column (solver, ei->eminus);

      g_hash_table_remove (solver->edit_var_map, constraint->variable);
    }

  if (error_vars != NULL)
    g_hash_table_remove (solver->error_vars, constraint);

  if (solver->auto_solve)
    {
      simplex_solver_optimize (solver, solver->objective);
      simplex_solver_set_external_variables (solver);
    }

  constraint_free (constraint);
}

void
simplex_solver_suggest_value (SimplexSolver *solver,
                              Variable *variable,
                              double value)
{
  EditInfo *ei;

  if (!solver->initialized)
    {
      char *str = variable_to_string (variable);

      g_critical ("Unable to suggest value '%g' for variable '%s': the "
                  "SimplexSolver %p is not initialized.",
                  value, str, solver);

      g_free (str);
      return;
    }

  ei = g_hash_table_lookup (solver->edit_var_map, variable);
  if (ei == NULL)
    {
      g_critical ("Suggesting value '%g' but variable %p is not editable",
                  value, variable);
      return;
    }

  ei->prev_constant = value - ei->prev_constant;

  simplex_solver_delta_edit_constant (solver, ei->prev_constant, ei->eplus, ei->eminus);
}

void
simplex_solver_resolve (SimplexSolver *solver)
{
  if (!solver->initialized)
    {
      g_critical ("Unable to resolve the simplex: the SimplexSolver %p "
                  "is not initialized",
                  solver);
      return;
    }

#ifdef EMEUS_ENABLE_DEBUG
  gint64 start_time = g_get_monotonic_time ();
#endif

  simplex_solver_dual_optimize (solver);
  simplex_solver_set_external_variables (solver);

  g_hash_table_remove_all (solver->infeasible_rows);

  simplex_solver_reset_stay_constants (solver);

#ifdef EMEUS_ENABLE_DEBUG
  g_print ("resolve.time := %.3f us\n", (float) g_get_monotonic_time () - start_time);
#endif

  solver->needs_solving = false;
}

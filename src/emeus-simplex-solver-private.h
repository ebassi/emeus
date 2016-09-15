#pragma once

#include "emeus-types-private.h"

G_BEGIN_DECLS

struct _SimplexSolver {
  /* HashTable<Variable, HashSet<Variable>> */
  GHashTable *columns;

  /* HashTable<Variable, Expression> */
  GHashTable *rows;

  /* Sets */
  GHashTable *infeasible_rows;
  GHashTable *external_rows;
  GHashTable *external_vars;
  GHashTable *updated_externals;

  GPtrArray *stay_error_vars;

  GHashTable *error_vars;
  GHashTable *marker_vars;

  GHashTable *edit_var_map;

  int slack_counter;
  int artificial_counter;
  int dummy_counter;
  int optimize_count;

  bool auto_solve;
  bool needs_solving;
};

void simplex_solver_init (SimplexSolver *solver);
void simplex_solver_clear (SimplexSolver *solver);

Variable *simplex_solver_create_variable (SimplexSolver *solver);
Expression *simplex_solver_create_expression (SimplexSolver *solver,
                                              double constant);

Constraint *simplex_solver_add_constraint (SimplexSolver *solver,
                                           Expression *expression,
                                           OperatorType op,
                                           StrengthType strength);

Constraint *simplex_solver_add_stay_constraint (SimplexSolver *solver,
                                                Variable *variable,
                                                StrengthType strength);

Constraint *simplex_solver_add_edit_constraint (SimplexSolver *solver,
                                                Variable *variable,
                                                StrengthType strength);

void simplex_solver_suggest_value (SimplexSolver *solver,
                                   Variable *variable,
                                   double value);

void simplex_solver_resolve (SimplexSolver *solver);

/* Internal */
void simplex_solver_add_variable (SimplexSolver *solver,
                                  Variable *variable);
void simplex_solver_remove_variable (SimplexSolver *solver,
                                     Variable *variable);

G_END_DECLS

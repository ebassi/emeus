#pragma once

#include "emeus-types-private.h"

typedef struct {
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

  Variable objective;
  GHashTable *edit_var_map;

  int slack_counter;
  int artificial_counter;
  int dummy_counter;
  int optimize_count;

  bool auto_solve;
  bool needs_solving;
} SimplexSolver;

void simplex_solver_init (SimplexSolver *solver);
void simplex_solver_clear (SimplexSolver *solver);

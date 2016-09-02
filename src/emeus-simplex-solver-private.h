#pragma once

#include "emeus-types-private.h"

typedef struct {
  GHashTable *columns;
  GHashTable *rows;

  GHashTable *infeasible_rows;
  GHashTable *external_rows;
  GHashTable *external_vars;

  GPtrArray *stay_error_vars;

  GHashTable *error_vars;
  GHashTable *marker_vars;

  Variable objective;
  GHashTable *edit_var_map;

  int slack_counter;
  int artificial_counter;
  int dummy_counter;
  int optimize_count;

  gboolean auto_solve : 1;
  gboolean needs_solving : 1;
} SimplexSolver;

SimplexSolver *         simplex_solver_new      (void);

#pragma once

#include <stdbool.h>
#include <glib-object.h>

G_BEGIN_DECLS

typedef struct _SimplexSolver   SimplexSolver;

typedef enum {
  VARIABLE_DUMMY     = 'd',
  VARIABLE_OBJECTIVE = 'o',
  VARIABLE_SLACK     = 's',
  VARIABLE_REGULAR   = 'v'
} VariableType;

typedef struct {
  int ref_count;

  VariableType type;

  double value;

  bool is_external;
  bool is_pivotable;
  bool is_restricted;

  SimplexSolver *solver;
} Variable;

typedef struct {
  double coefficient;

  Variable *variable;
} Term;

typedef struct {
  int ref_count;

  double constant;

  /* HashTable<Variable,Term> */
  GHashTable *terms;

  SimplexSolver *solver;
} Expression;

typedef enum {
  OPERATOR_TYPE_LE = -1,
  OPERATOR_TYPE_EQ = 0,
  OPERATOR_TYPE_GE = 1
} OperatorType;

typedef enum {
  STRENGTH_REQUIRED = 1001001000,
  STRENGTH_STRONG = 1000000,
  STRENGTH_MEDIUM = 1000,
  STRENGTH_WEAK = 1
} StrengthType;

typedef struct {
  int ref_count;

  /* While the Constraint's normal form is expressed in terms of a linear
   * equation between two terms, e.g.:
   *
   *   x = y * coefficient + constant
   *
   * we use a single expression and compare with zero:
   *
   *   x - (y * coefficient + constant) = 0
   *
   * as this simplifies tableau we set up for the simplex solver
   */
  Expression *expression;
  OperatorType op_type;

  StrengthType strength;

  bool is_edit;
  bool is_stay;

  SimplexSolver *solver;
} Constraint;

G_END_DECLS

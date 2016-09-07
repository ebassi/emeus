#pragma once

#include <glib-object.h>

G_BEGIN_DECLS

typedef enum {
  VARIABLE_TYPE_DUMMY,
  VARIABLE_TYPE_OBJECTIVE,
  VARIABLE_TYPE_SLACK,
  VARIABLE_TYPE_REGULAR
} VariableType;

typedef struct {
  VariableType v_type;

  GQuark name;

  double value;

  gboolean is_external : 1;
  gboolean is_pivotable : 1;
  gboolean is_restricted : 1;
} Variable;

typedef struct {
  double constant;

  GHashTable *terms;
} Expression;

typedef enum {
  CONSTRAINT_TYPE_EDIT,
  CONSTRAINT_TYPE_STAY,
  CONSTRAINT_TYPE_REGULAR
} ConstraintType;

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
  ConstraintType c_type;

  Variable *variable;
  OperatorType op_type;
  Expression *expression;

  int strength;
  double weight;

  gboolean is_inequality : 1;
} SimplexConstraint;

G_END_DECLS

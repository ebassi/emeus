#pragma once

#include <stdbool.h>
#include <glib-object.h>

G_BEGIN_DECLS

typedef enum {
  VARIABLE_DUMMY     = 'd',
  VARIABLE_OBJECTIVE = 'o',
  VARIABLE_SLACK     = 's',
  VARIABLE_REGULAR   = 'v'
} VariableType;

typedef struct {
  int ref_count;

  VariableType type;
  char *name;

  double value;

  bool is_external;
  bool is_pivotable;
  bool is_restricted;
} Variable;

typedef struct {
  double coefficient;

  Variable *variable;
} Term;

typedef struct {
  int ref_count;

  double constant;

  GHashTable *terms;
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

  Expression *expression;
  OperatorType op_type;

  StrengthType strength;
  double weight;

  bool is_edit;
  bool is_slack;
  bool is_inequality;
} Constraint;

G_END_DECLS

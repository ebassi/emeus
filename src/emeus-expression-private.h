#pragma once

#include "emeus-types-private.h"
#include "emeus-variable-private.h"

G_BEGIN_DECLS

static inline double
term_get_value (const Term *term)
{
  if (term == NULL)
    return 0.0;

  return variable_get_value (term->variable) * term->coefficient;
}

static inline Variable *
term_get_variable (const Term *term)
{
  return term->variable;
}

static inline double
term_get_coefficient (const Term *term)
{
  return term->coefficient;
}

static inline bool
expression_is_constant (const Expression *expression)
{
  return expression->terms == NULL;
}

static inline double
expression_get_constant (const Expression *expression)
{
  return expression->constant;
}

Expression *expression_new (SimplexSolver *solver,
                            double constant);

Expression *expression_new_from_variable (Variable *variable);

Expression *expression_ref (Expression *expression);
void expression_unref (Expression *expression);

void expression_set_constant (Expression *expression,
                              double constant);

void expression_add_variable (Expression *expression,
                              Variable *variable,
                              double value);

void expression_remove_variable (Expression *expression,
                                 Variable *variable);

bool expression_has_variable (const Expression *expression,
                              Variable *variable);

void expression_add_expression (Expression *a,
                                Expression *b);

Expression *expression_plus_variable (Expression *expression,
                                      Variable *variable);
Expression *expression_times_constant (Expression *expression,
                                       double constant);

void expression_set_coefficient (Expression *expression,
                                 Variable *variable,
                                 double coefficient);

double expression_get_coefficient (const Expression *expression,
                                   Variable *variable);

double expression_get_value (const Expression *expression);

typedef void (* ExpressionForeachTermFunc) (Term *term, gpointer data);

void expression_terms_foreach (Expression *expression,
                               ExpressionForeachTermFunc func,
                               gpointer data);

G_END_DECLS

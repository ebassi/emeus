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

Term *term_new (Variable *variable,
                double    coefficient);
Term *term_clone (const Term *term);
void term_free (Term *term);
gboolean term_equal (gconstpointer v1,
                     gconstpointer v2);

static inline bool
expression_is_constant (const Expression *expression)
{
  return expression->terms == NULL;
}

Expression *expression_new (Variable *variable,
                            double value,
                            double constant);
Expression *expression_new_empty (void);
Expression *expression_new_from_variable (Variable *variable);
Expression *expression_new_from_constant (double constant);

Expression *expression_ref (Expression *expression);
void expression_unref (Expression *expression);

Expression *expression_clone (const Expression *expression);

GPtrArray *expression_get_terms_as_array (const Expression *expression);

void expression_add_variable (Expression *expression,
                              Variable *variable,
                              double value);

double expression_get_value (const Expression *expression);

G_END_DECLS

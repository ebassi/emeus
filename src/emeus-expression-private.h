/* emeus-expression-private.h: A set of terms and a constant
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

Expression *expression_clone (Expression *expression);

Expression *expression_ref (Expression *expression);
void expression_unref (Expression *expression);

void expression_set_constant (Expression *expression,
                              double constant);

void expression_set_variable (Expression *expression,
                              Variable *variable,
                              double coefficient);

void expression_add_variable (Expression *expression,
                              Variable *variable,
                              double value);

void expression_remove_variable (Expression *expression,
                                 Variable *variable);

bool expression_has_variable (const Expression *expression,
                              Variable *variable);

void expression_add_variable_with_subject (Expression *expression,
                                           Variable *variable,
                                           double value,
                                           Variable *subject);

void expression_add_expression (Expression *a,
                                Expression *b,
                                double n,
                                Variable *subject);

Expression *expression_plus (Expression *expression,
                             double constant);
Expression *expression_times (Expression *expression,
                              double multiplier);

Expression *expression_plus_variable (Expression *expression,
                                      Variable *variable);

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

void expression_change_subject (Expression *expression,
                                Variable *old_subject,
                                Variable *new_subject);

double expression_new_subject (Expression *expression,
                               Variable *subject);

Variable *expression_get_pivotable_variable (Expression *expression);

G_END_DECLS

#pragma once

#include "emeus-types-private.h"

G_BEGIN_DECLS

Expression *    expression_new                  (Variable   *variable,
                                                 double      value,
                                                 double      constant);

void            expression_free                 (Expression *expression);

Expression *    expression_copy                 (Expression *expression);

gboolean        expression_is_constant          (Expression *expression);

void            expression_add_variable         (Expression *expression,
                                                 Variable   *variable,
                                                 double      coefficient);
void            expression_remove_variable      (Expression *expression,
                                                 Variable   *variable);
gboolean        expression_has_variable         (Expression *expression,
                                                 Variable   *variable);

double          expression_get_coefficient      (Expression *expression,
                                                 Variable   *variable);

void            expression_multiply             (Expression *expression,
                                                 double      value);

double          expression_new_subject          (Expression *expression,
                                                 Variable   *subject);
void            expression_change_subject       (Expression *expression,
                                                 Variable   *old_subject,
                                                 Variable   *new_subject);

G_END_DECLS

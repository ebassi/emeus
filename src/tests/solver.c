#include "emeus-expression-private.h"
#include "emeus-simplex-solver-private.h"
#include "emeus-types-private.h"

#include "emeus-test-utils.h"

static void
emeus_solver_simple (void)
{
  SimplexSolver solver = SIMPLEX_SOLVER_INIT;

  simplex_solver_init (&solver);

  Variable *x = simplex_solver_create_variable (&solver, "x", 167.0);
  Variable *y = simplex_solver_create_variable (&solver, "y", 2.0);

  Expression *e = expression_new_from_variable (y);

  simplex_solver_add_constraint (&solver,
                                 x, OPERATOR_TYPE_EQ, e,
                                 STRENGTH_REQUIRED);

  double x_value = variable_get_value (x);
  double y_value = variable_get_value (y);

  emeus_assert_almost_equals (x_value, y_value);
  emeus_assert_almost_equals (x_value, 0.0);
  emeus_assert_almost_equals (y_value, 0.0);

  expression_unref (e);
  variable_unref (y);
  variable_unref (x);

  simplex_solver_clear (&solver);
}

static void
emeus_solver_stay (void)
{
  SimplexSolver solver = SIMPLEX_SOLVER_INIT;

  simplex_solver_init (&solver);

  Variable *x = simplex_solver_create_variable (&solver, "x", 5.0);
  Variable *y = simplex_solver_create_variable (&solver, "y", 10.0);

  simplex_solver_add_stay_variable (&solver, x, STRENGTH_WEAK);
  simplex_solver_add_stay_variable (&solver, y, STRENGTH_WEAK);

  double x_value = variable_get_value (x);
  double y_value = variable_get_value (y);

  emeus_assert_almost_equals (x_value,  5.0);
  emeus_assert_almost_equals (y_value, 10.0);

  simplex_solver_clear (&solver);
}

static void
emeus_solver_variable_geq_constant (void)
{
  SimplexSolver solver = SIMPLEX_SOLVER_INIT;

  simplex_solver_init (&solver);

  Variable *x = simplex_solver_create_variable (&solver, "x", 10.0);
  Expression *e = simplex_solver_create_expression (&solver, 100.0);

  simplex_solver_add_constraint (&solver, x, OPERATOR_TYPE_GE, e, STRENGTH_REQUIRED);

  double x_value = variable_get_value (x);

  emeus_assert_almost_equals (x_value, 100.0);

  expression_unref (e);
  variable_unref (x);

  simplex_solver_clear (&solver);
}

static void
emeus_solver_variable_leq_constant (void)
{
  SimplexSolver solver = SIMPLEX_SOLVER_INIT;

  simplex_solver_init (&solver);

  Variable *x = simplex_solver_create_variable (&solver, "x", 100.0);
  Expression *e = simplex_solver_create_expression (&solver, 10.0);

  simplex_solver_add_constraint (&solver, x, OPERATOR_TYPE_LE, e, STRENGTH_REQUIRED);

  double x_value = variable_get_value (x);

  emeus_assert_almost_equals (x_value, 10.0);

  expression_unref (e);
  variable_unref (x);

  simplex_solver_clear (&solver);
}

static void
emeus_solver_variable_eq_constant (void)
{
  SimplexSolver solver = SIMPLEX_SOLVER_INIT;

  simplex_solver_init (&solver);

  Variable *x = simplex_solver_create_variable (&solver, "x", 10.0);
  Expression *e = simplex_solver_create_expression (&solver, 100.0);

  simplex_solver_add_constraint (&solver, x, OPERATOR_TYPE_EQ, e, STRENGTH_REQUIRED);

  double x_value = variable_get_value (x);

  emeus_assert_almost_equals (x_value, 100.0);

  expression_unref (e);
  variable_unref (x);

  simplex_solver_clear (&solver);
}

static void
emeus_solver_eq_with_stay (void)
{
  SimplexSolver solver = SIMPLEX_SOLVER_INIT;

  simplex_solver_init (&solver);

  Variable *x = simplex_solver_create_variable (&solver, "x", 10.0);
  Variable *width = simplex_solver_create_variable (&solver, "width", 10.0);
  Variable *right_min = simplex_solver_create_variable (&solver, "rightMin", 100.0);
  Expression *right = expression_plus_variable (expression_new_from_variable (x), width);

  simplex_solver_add_stay_variable (&solver, width, STRENGTH_WEAK);
  simplex_solver_add_stay_variable (&solver, right_min, STRENGTH_WEAK);
  simplex_solver_add_constraint (&solver, right_min, OPERATOR_TYPE_EQ, right, STRENGTH_REQUIRED);

  double x_value = variable_get_value (x);
  double width_value = variable_get_value (width);

  emeus_assert_almost_equals (x_value, 90.0);
  emeus_assert_almost_equals (width_value, 10.0);

  expression_unref (right);
  variable_unref (right_min);
  variable_unref (width);
  variable_unref (x);

  simplex_solver_clear (&solver);
}

static void
emeus_solver_cassowary (void)
{
  SimplexSolver solver = SIMPLEX_SOLVER_INIT;

  simplex_solver_init (&solver);

  Variable *x = simplex_solver_create_variable (&solver, "x", 0.0);
  Variable *y = simplex_solver_create_variable (&solver, "y", 0.0);

  simplex_solver_add_constraint (&solver,
                                 x, OPERATOR_TYPE_LE, expression_new_from_variable (y),
                                 STRENGTH_REQUIRED);
  simplex_solver_add_constraint (&solver,
                                 y, OPERATOR_TYPE_EQ, expression_plus (expression_new_from_variable (x), 3.0),
                                 STRENGTH_REQUIRED);
  simplex_solver_add_constraint (&solver,
                                 x, OPERATOR_TYPE_EQ, expression_new_from_constant (10.0),
                                 STRENGTH_WEAK);
  simplex_solver_add_constraint (&solver,
                                 y, OPERATOR_TYPE_EQ, expression_new_from_constant (10.0),
                                 STRENGTH_WEAK);

  double x_val = variable_get_value (x);
  double y_val = variable_get_value (y);

  if (g_test_verbose ())
    g_test_message ("x = %g, y = %g", x_val, y_val);

  g_assert_true ((emeus_fuzzy_equals (x_val, 10, 1e-8) && emeus_fuzzy_equals (y_val, 13, 1e-8)) ||
                 (emeus_fuzzy_equals (x_val,  7, 1e-8) && emeus_fuzzy_equals (y_val, 10, 1e-9)));

  simplex_solver_clear (&solver);
}

static void
emeus_solver_edit_var_required (void)
{
  SimplexSolver solver = SIMPLEX_SOLVER_INIT;

  simplex_solver_init (&solver);

  Variable *a = simplex_solver_create_variable (&solver, "a", 0.0);
  simplex_solver_add_stay_variable (&solver, a, STRENGTH_STRONG);

  emeus_assert_almost_equals (variable_get_value (a), 0.0);

  simplex_solver_add_edit_variable (&solver, a, STRENGTH_REQUIRED);
  simplex_solver_begin_edit (&solver);
  simplex_solver_suggest_value (&solver, a, 2.0);
  simplex_solver_end_edit (&solver);

  emeus_assert_almost_equals (variable_get_value (a), 2.0);

  simplex_solver_clear (&solver);
}

static void
emeus_solver_edit_var_suggest (void)
{
  SimplexSolver solver = SIMPLEX_SOLVER_INIT;

  simplex_solver_init (&solver);

  Variable *a = simplex_solver_create_variable (&solver, "a", 0.0);
  Variable *b = simplex_solver_create_variable (&solver, "b", 0.0);

  simplex_solver_add_stay_variable (&solver, a, STRENGTH_STRONG);
  simplex_solver_add_constraint (&solver, a, OPERATOR_TYPE_EQ, expression_new_from_variable (b), STRENGTH_REQUIRED);
  simplex_solver_resolve (&solver);

  emeus_assert_almost_equals (variable_get_value (a), 0.0);
  emeus_assert_almost_equals (variable_get_value (b), 0.0);

  simplex_solver_add_edit_variable (&solver, a, STRENGTH_REQUIRED);
  simplex_solver_begin_edit (&solver);
  simplex_solver_suggest_value (&solver, a, 2.0);
  simplex_solver_resolve (&solver);

  emeus_assert_almost_equals (variable_get_value (a), 2.0);
  emeus_assert_almost_equals (variable_get_value (b), 2.0);

  simplex_solver_suggest_value (&solver, a, 10.0);
  simplex_solver_resolve (&solver);

  emeus_assert_almost_equals (variable_get_value (a), 10.0);
  emeus_assert_almost_equals (variable_get_value (b), 10.0);

  simplex_solver_clear (&solver);
}

static void
emeus_solver_paper (void)
{
  SimplexSolver solver = SIMPLEX_SOLVER_INIT;

  simplex_solver_init (&solver);

  Variable *left = simplex_solver_create_variable (&solver, "left", 0.0);
  Variable *middle = simplex_solver_create_variable (&solver, "middle", 0.0);
  Variable *right = simplex_solver_create_variable (&solver, "right", 0.0);

  Expression *expr;

  expr = expression_divide (expression_plus_variable (expression_new_from_variable (left), right), 2.0);
  simplex_solver_add_constraint (&solver, middle, OPERATOR_TYPE_EQ, expr, STRENGTH_REQUIRED);

  expr = expression_plus (expression_new_from_variable (left), 10.0);
  simplex_solver_add_constraint (&solver, right, OPERATOR_TYPE_EQ, expr, STRENGTH_REQUIRED);

  expr = expression_new_from_constant (100.0);
  simplex_solver_add_constraint (&solver, right, OPERATOR_TYPE_LE, expr, STRENGTH_REQUIRED);

  expr = expression_new_from_constant (0.0);
  simplex_solver_add_constraint (&solver, left, OPERATOR_TYPE_GE, expr, STRENGTH_REQUIRED);

  if (g_test_verbose ())
    g_test_message ("Check constraints hold");

  emeus_assert_almost_equals (variable_get_value (middle),
                              (variable_get_value (left) + variable_get_value (right)) / 2.0);
  emeus_assert_almost_equals (variable_get_value (right),
                              variable_get_value (left) + 10.0);
  g_assert_cmpfloat (variable_get_value (right), <=, 100.0);
  g_assert_cmpfloat (variable_get_value (left), >=, 0.0);

  variable_set_value (middle, 45.0);
  simplex_solver_add_stay_variable (&solver, middle, STRENGTH_WEAK);

  if (g_test_verbose ())
    g_test_message ("Check constraints hold after setting middle");

  emeus_assert_almost_equals (variable_get_value (middle),
                              (variable_get_value (left) + variable_get_value (right)) / 2.0);
  emeus_assert_almost_equals (variable_get_value (right),
                              variable_get_value (left) + 10.0);
  g_assert_cmpfloat (variable_get_value (right), <=, 100.0);
  g_assert_cmpfloat (variable_get_value (left), >=, 0.0);

  if (g_test_verbose ())
    g_test_message ("Check values after setting middle");

  emeus_assert_almost_equals (variable_get_value (left), 40.0);
  emeus_assert_almost_equals (variable_get_value (middle), 45.0);
  emeus_assert_almost_equals (variable_get_value (right), 50.0);

  simplex_solver_clear (&solver);
}

typedef struct {
  Variable *left;
  Variable *width;
} Button;

static char *
button_to_string (const Button *button)
{
  char *left_s = variable_to_string (button->left);
  char *width_s = variable_to_string (button->width);
  char *res = g_strdup_printf ("%s:%s", left_s, width_s);

  g_free (left_s);
  g_free (width_s);

  return res;
}

static void
button_init (Button *b,
             const char *prefix,
             SimplexSolver *solver)
{
  b->left = simplex_solver_create_variable (solver, "left", 0);
  variable_set_prefix (b->left, prefix);

  b->width = simplex_solver_create_variable (solver, "width", 0);
  variable_set_prefix (b->width, prefix);
}

static void
emeus_solver_buttons (void)
{
  SimplexSolver solver = SIMPLEX_SOLVER_INIT;

  simplex_solver_init (&solver);

  Button b1, b2;
  button_init (&b1, "b1", &solver);
  button_init (&b2, "b2", &solver);

  Variable *left_limit = simplex_solver_create_variable (&solver, "left", 0);
  Variable *right_limit = simplex_solver_create_variable (&solver, "width", 0);

  variable_set_prefix (left_limit, "super");
  variable_set_prefix (right_limit, "super");

  simplex_solver_add_stay_variable (&solver, left_limit, STRENGTH_REQUIRED);
  simplex_solver_add_stay_variable (&solver, right_limit, STRENGTH_WEAK);

  simplex_solver_add_constraint (&solver,
                                 b1.width, OPERATOR_TYPE_EQ, expression_new_from_variable (b2.width),
                                 STRENGTH_REQUIRED);
  simplex_solver_add_constraint (&solver,
                                 b1.left, OPERATOR_TYPE_EQ, expression_plus (expression_new_from_variable (left_limit), 50),
                                 STRENGTH_REQUIRED);
  simplex_solver_add_constraint (&solver,
                                 right_limit, OPERATOR_TYPE_EQ,
                                 expression_plus (expression_plus_variable (expression_new_from_variable (b2.left), b2.width), 50),
                                 STRENGTH_REQUIRED);
  simplex_solver_add_constraint (&solver,
                                 b2.left, OPERATOR_TYPE_GE,
                                 expression_plus (expression_plus_variable (expression_new_from_variable (b1.left), b1.width), 100),
                                 STRENGTH_REQUIRED);
  simplex_solver_add_constraint (&solver,
                                 b1.width, OPERATOR_TYPE_GE, expression_new_from_constant (87.0),
                                 STRENGTH_REQUIRED);
  simplex_solver_add_constraint (&solver,
                                 b1.width, OPERATOR_TYPE_EQ, expression_new_from_constant (87.0),
                                 STRENGTH_STRONG);
  simplex_solver_add_constraint (&solver,
                                 b2.width, OPERATOR_TYPE_GE, expression_new_from_constant (113.0),
                                 STRENGTH_REQUIRED);
  simplex_solver_add_constraint (&solver,
                                 b2.width, OPERATOR_TYPE_EQ, expression_new_from_constant (113.0),
                                 STRENGTH_STRONG);

  g_test_message ("right_limit := 0 (preferred size)");

  {
    char *b1_s = button_to_string (&b1);
    char *b2_s = button_to_string (&b2);
    char *l_s = variable_to_string (left_limit);
    char *r_s = variable_to_string (right_limit);

    g_test_message ("b1 := %s, b2 := %s, left := %s, right := %s", b1_s, b2_s, l_s, r_s);

    g_free (b1_s);
    g_free (b2_s);
    g_free (l_s);
    g_free (r_s);
  }

  emeus_assert_almost_equals (variable_get_value (b1.left), 50.0);
  emeus_assert_almost_equals (variable_get_value (b1.width), 113.0);
  emeus_assert_almost_equals (variable_get_value (b2.left), 263.0);
  emeus_assert_almost_equals (variable_get_value (b2.width), 113.0);
  emeus_assert_almost_equals (variable_get_value (right_limit), 426.0);

  Constraint *stay;

  g_test_message ("right_limit := 500");
  variable_set_value (right_limit, 500.0);
  stay = simplex_solver_add_stay_variable (&solver, right_limit, STRENGTH_REQUIRED);

  {
    char *b1_s = button_to_string (&b1);
    char *b2_s = button_to_string (&b2);
    char *l_s = variable_to_string (left_limit);
    char *r_s = variable_to_string (right_limit);

    g_test_message ("b1 := %s, b2 := %s, left := %s, right := %s", b1_s, b2_s, l_s, r_s);

    g_free (b1_s);
    g_free (b2_s);
    g_free (l_s);
    g_free (r_s);
  }

  emeus_assert_almost_equals (variable_get_value (b1.left), 50.0);
  emeus_assert_almost_equals (variable_get_value (b1.width), 113.0);
  emeus_assert_almost_equals (variable_get_value (b2.left), 337.0);
  emeus_assert_almost_equals (variable_get_value (b2.width), 113.0);
  emeus_assert_almost_equals (variable_get_value (right_limit), 500.0);

  simplex_solver_remove_constraint (&solver, stay);

  g_test_message ("right_limit := 700");
  variable_set_value (right_limit, 700.0);
  stay = simplex_solver_add_stay_variable (&solver, right_limit, STRENGTH_REQUIRED);

  {
    char *b1_s = button_to_string (&b1);
    char *b2_s = button_to_string (&b2);
    char *l_s = variable_to_string (left_limit);
    char *r_s = variable_to_string (right_limit);

    g_test_message ("b1 := %s, b2 := %s, left := %s, right := %s", b1_s, b2_s, l_s, r_s);

    g_free (b1_s);
    g_free (b2_s);
    g_free (l_s);
    g_free (r_s);
  }

  emeus_assert_almost_equals (variable_get_value (b1.left), 50.0);
  emeus_assert_almost_equals (variable_get_value (b1.width), 113.0);
  emeus_assert_almost_equals (variable_get_value (b2.left), 537.0);
  emeus_assert_almost_equals (variable_get_value (b2.width), 113.0);
  emeus_assert_almost_equals (variable_get_value (right_limit), 700.0);

  simplex_solver_remove_constraint (&solver, stay);

  g_test_message ("right_limit := 600");
  variable_set_value (right_limit, 600.0);
  stay = simplex_solver_add_stay_variable (&solver, right_limit, STRENGTH_REQUIRED);

  {
    char *b1_s = button_to_string (&b1);
    char *b2_s = button_to_string (&b2);
    char *l_s = variable_to_string (left_limit);
    char *r_s = variable_to_string (right_limit);

    g_test_message ("b1 := %s, b2 := %s, left := %s, right := %s", b1_s, b2_s, l_s, r_s);

    g_free (b1_s);
    g_free (b2_s);
    g_free (l_s);
    g_free (r_s);
  }

  emeus_assert_almost_equals (variable_get_value (b1.left), 50.0);
  emeus_assert_almost_equals (variable_get_value (b1.width), 113.0);
  emeus_assert_almost_equals (variable_get_value (b2.left), 437.0);
  emeus_assert_almost_equals (variable_get_value (b2.width), 113.0);
  emeus_assert_almost_equals (variable_get_value (right_limit), 600.0);

  simplex_solver_remove_constraint (&solver, stay);

  g_test_message ("right_limit := 0 (preferred size, again)");

  simplex_solver_add_edit_variable (&solver, right_limit, STRENGTH_WEAK);
  simplex_solver_begin_edit (&solver);
  simplex_solver_suggest_value (&solver, right_limit, 0);
  simplex_solver_resolve (&solver);

  {
    char *b1_s = button_to_string (&b1);
    char *b2_s = button_to_string (&b2);
    char *l_s = variable_to_string (left_limit);
    char *r_s = variable_to_string (right_limit);

    g_test_message ("b1 := %s, b2 := %s, left := %s, right := %s", b1_s, b2_s, l_s, r_s);

    g_free (b1_s);
    g_free (b2_s);
    g_free (l_s);
    g_free (r_s);
  }

  emeus_assert_almost_equals (variable_get_value (b1.left), 50.0);
  emeus_assert_almost_equals (variable_get_value (b1.width), 113.0);
  emeus_assert_almost_equals (variable_get_value (b2.left), 263.0);
  emeus_assert_almost_equals (variable_get_value (b2.width), 113.0);
  emeus_assert_almost_equals (variable_get_value (right_limit), 426.0);

  simplex_solver_clear (&solver);
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/emeus/solver/simple", emeus_solver_simple);
  g_test_add_func ("/emeus/solver/stay", emeus_solver_stay);
  g_test_add_func ("/emeus/solver/edit-var-required", emeus_solver_edit_var_required);
  g_test_add_func ("/emeus/solver/edit-var-suggest", emeus_solver_edit_var_suggest);
  g_test_add_func ("/emeus/solver/variable-geq-constant", emeus_solver_variable_geq_constant);
  g_test_add_func ("/emeus/solver/variable-leq-constant", emeus_solver_variable_leq_constant);
  g_test_add_func ("/emeus/solver/variable-eq-constant", emeus_solver_variable_eq_constant);
  g_test_add_func ("/emeus/solver/eq-with-stay", emeus_solver_eq_with_stay);
  g_test_add_func ("/emeus/solver/cassowary", emeus_solver_cassowary);
  g_test_add_func ("/emeus/solver/paper", emeus_solver_paper);
  g_test_add_func ("/emeus/solver/buttons", emeus_solver_buttons);

  return g_test_run ();
}

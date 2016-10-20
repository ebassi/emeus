#include "emeus-test-utils.h"

#include "emeus-expression-private.h"
#include "emeus-simplex-solver-private.h"
#include "emeus-types-private.h"
#include "emeus-variable-private.h"
#include "emeus-vfl-parser-private.h"

static struct {
  const char *id;
  const char *expression;
} vfl_valid[] = {
  { "standard-space", "[button]-[textField]", },
  { "width-constraint", "[button(>=50)]", },
  { "connection-superview", "|-50-[purpleBox]-50-|", },
  { "vertical-layout", "V:[topField]-10-[bottomField]", },
  { "flush-views", "[maroonView][blueView]", },
  { "priority", "[button(100@strong)]", },
  { "equal-widths", "[button1(==button2)]", },
  { "multiple-predicates", "[flexibleButton(>=70,<=100)]", },
  { "complete-line", "|-[find]-[findNext]-[findField(>=20)]-|", },
  { "dot-name", "[button1(==button2.width)]", },
  { "grid-1", "H:|-8-[view1(==view2)]-12-[view2]-8-|", },
  { "grid-2", "H:|-8-[view3]-8-|", },
  { "grid-3", "V:|-8-[view1]-12-[view3(==view1,view2)]-8-|", },
};

static struct {
  const char *id;
  const char *expression;
  VflError error_id;
} vfl_invalid[] = {
  { "orientation-invalid", "V|[backgroundBox]|", VFL_ERROR_INVALID_SYMBOL, },
  { "missing-view-terminator", "[backgroundBox)", VFL_ERROR_INVALID_SYMBOL, },
  { "invalid-predicate", "[backgroundBox(]", VFL_ERROR_INVALID_SYMBOL, },
  { "invalid-view", "[[", VFL_ERROR_INVALID_VIEW, },
  { "invalid-super-view", "[view]|", VFL_ERROR_INVALID_SYMBOL, },
  { "trailing-spacing", "[view]-", VFL_ERROR_INVALID_SYMBOL, },
  { "leading-spacing", "-[view]", VFL_ERROR_INVALID_SYMBOL, },
  { "view-invalid-identifier-1", "[9ab]", VFL_ERROR_INVALID_VIEW, },
  { "view-invalid-identifier-2", "[-a]", VFL_ERROR_INVALID_VIEW, },
  { "predicate-wrong-relation", "[view(>30)]", VFL_ERROR_INVALID_RELATION, },
  { "predicate-wrong-priority", "[view(>=30@foo)]", VFL_ERROR_INVALID_PRIORITY, },
  { "predicate-wrong-attribute", "[view1(==view2.height)]", VFL_ERROR_INVALID_ATTRIBUTE, },
};

static void
vfl_parser_valid (gconstpointer data)
{
  int idx = GPOINTER_TO_INT (data);
  VflParser *parser = vfl_parser_new ();
  GError *error = NULL;
  VflConstraint *constraints;
  int n_constraints = 0;

  g_test_message ("Parsing [valid]: '%s'", vfl_valid[idx].expression);

  vfl_parser_parse_line (parser, vfl_valid[idx].expression, -1, &error);
  g_assert_no_error (error);

  constraints = vfl_parser_get_constraints (parser, &n_constraints);
  g_assert (constraints != NULL);
  g_assert (n_constraints != 0);

  g_free (constraints);

  vfl_parser_free (parser);
}

static void
vfl_parser_invalid (gconstpointer data)
{
  int idx = GPOINTER_TO_INT (data);
  VflParser *parser = vfl_parser_new ();
  GError *error = NULL;

  g_test_message ("Parsing [invalid]: '%s'", vfl_invalid[idx].expression);

  vfl_parser_parse_line (parser, vfl_invalid[idx].expression, -1, &error);
  g_assert_error (error, VFL_ERROR, vfl_invalid[idx].error_id);

  g_test_message ("Error: %s", error->message);

  g_error_free (error);
  vfl_parser_free (parser);
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  for (int i = 0; i < G_N_ELEMENTS (vfl_invalid); i++)
    {
      char *path = g_strconcat ("/vfl/invalid/", vfl_invalid[i].id, NULL);

      g_test_add_data_func (path, GINT_TO_POINTER (i), vfl_parser_invalid);

      g_free (path);
    }

  for (int i = 0; i < G_N_ELEMENTS (vfl_valid); i++)
    {
      char *path = g_strconcat ("/vfl/valid/", vfl_valid[i].id, NULL);

      g_test_add_data_func (path, GINT_TO_POINTER (i), vfl_parser_valid);

      g_free (path);
    }

  return g_test_run ();
}

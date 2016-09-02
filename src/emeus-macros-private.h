#pragma once

#include <glib-object.h>

#define EMEUS_ENUM_VALUE(EnumValue,EnumNick) { EnumValue, #EnumValue, EnumNick },

#define EMEUS_DEFINE_ENUM_TYPE(TypeName,type_name,values) \
GType \
type_name ## _get_type (void) \
{ \
  static volatile gsize emeus_define_id__volatile = 0; \
  if (g_once_init_enter (&emeus_define_id__volatile)) \
    { \
      static const GEnumValue v[] = { \
        values \
        { 0, NULL, NULL }, \
      }; \
      GType emeus_define_id = \
        g_enum_register_static (g_intern_static_string (#TypeName), v); \
      g_once_init_leave (&emeus_define_id__volatile, emeus_define_id); \
    } \
  return emeus_define_id__volatile; \
}

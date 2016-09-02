/* emeus-version-macros.h
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

#include <emeus-version.h>

#define EMEUS_CHECK_VERSION(major,minor,micro) \
  (((major) > EMEUS_MAJOR_VERSION) || \
   ((major) == EMEUS_MAJOR_VERSION && (minor) > EMEUS_MINOR_VERSION) || \
   ((major) == EMEUS_MAJOR_VERSION && (minor) == EMEUS_MINOR_VERSION && (micro) >= EMEUS_MICRO_VERSION))

#ifndef _EMEUS_PUBLIC
# define _EMEUS_PUBLIC extern
#endif

#ifdef EMEUS_DISABLE_DEPRECATION_WARNINGS
# define EMEUS_DEPRECATED _EMEUS_PUBLIC
# define EMEUS_DEPRECATED_FOR(f) _EMEUS_PUBLIC
# define EMEUS_UNAVAILABLE(maj,min) _EMEUS_PUBLIC
#else
# define EMEUS_DEPRECATED G_DEPRECATED _EMEUS_PUBLIC
# define EMEUS_DEPRECATED_FOR(f) G_DEPRECATED_FOR(f) _EMEUS_PUBLIC
# define EMEUS_UNAVAILABLE(maj,min) G_UNAVAILABLE(maj,min) _EMEUS_PUBLIC
#endif

#define EMEUS_VERSION_1_0       (G_ENCODE_VERSION (1, 0))

#if (EMEUS_MINOR_VERSION % 2)
# define EMEUS_VERSION_CUR_STABLE      (G_ENCODE_VERSION (EMEUS_MAJOR_VERSION, EMEUS_MINOR_VERSION + 1))
#else
# define EMEUS_VERSION_CUR_STABLE      (G_ENCODE_VERSION (EMEUS_MAJOR_VERSION, EMEUS_MINOR_VERSION))
#endif

#if (EMEUS_MINOR_VERSION % 2)
# define EMEUS_VERSION_PREV_STABLE     (G_ENCODE_VERSION (EMEUS_MAJOR_VERSION, EMEUS_MINOR_VERSION - 1))
#else
# define EMEUS_VERSION_PREV_STABLE     (G_ENCODE_VERSION (EMEUS_MAJOR_VERSION, EMEUS_MINOR_VERSION - 2))
#endif

#ifndef EMEUS_VERSION_MIN_REQUIRED
# define EMEUS_VERSION_MIN_REQUIRED    (EMEUS_VERSION_CUR_STABLE)
#endif

#ifndef EMEUS_VERSION_MAX_ALLOWED
# if EMEUS_VERSION_MIN_REQUIRED > EMEUS_VERSION_PREV_STABLE
#  define EMEUS_VERSION_MAX_ALLOWED    EMEUS_VERSION_MIN_REQUIRED
# else
#  define EMEUS_VERSION_MAX_ALLOWED    EMEUS_VERSION_CUR_STABLE
# endif
#endif

#if EMEUS_VERSION_MIN_REQUIRED >= EMEUS_VERSION_1_0
# define EMEUS_DEPRECATED_IN_1_0              EMEUS_DEPRECATED
# define EMEUS_DEPRECATED_IN_1_0_FOR(f)       EMEUS_DEPRECATED_FOR(f)
#else
# define EMEUS_DEPRECATED_IN_1_0              _EMEUS_PUBLIC
# define EMEUS_DEPRECATED_IN_1_0_FOR(f)       _EMEUS_PUBLIC
#endif

#if EMEUS_VERSION_MAX_ALLOWED < EMEUS_VERSION_3_0
# define EMEUS_AVAILABLE_IN_1_0               EMEUS_UNAVAILABLE(1, 0)
#else
# define EMEUS_AVAILABLE_IN_1_0               _EMEUS_PUBLIC
#endif

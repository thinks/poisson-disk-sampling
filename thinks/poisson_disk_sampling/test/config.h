// Copyright(C) Tommy Hinks <tommy.hinks@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#if __cplusplus >= 201402L // C++14 or later.
#define _CONSTEXPR constexpr
#else
#define _CONSTEXPR inline
#endif

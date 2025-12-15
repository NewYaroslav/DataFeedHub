#pragma once
#ifndef _DFH_UTILS_HPP_INCLUDED
#define _DFH_UTILS_HPP_INCLUDED

/// \file utils.hpp
/// \brief Единственная точка подключения для вспомогательных утилит DataFeedHub.
/// \details Собирает мелкие компоненты без доменной логики: парсеры символов и бирж,
/// SIMD/выравнивание, фиксированную арифметику, работу со строками и битовыми масками,
/// используемые другими доменами библиотеки.

//------------------------------------------------------------------------------
// Utility domain
//------------------------------------------------------------------------------

#include "utils/DynamicBitset.hpp"
#include "utils/aligned_allocator.hpp"
#include "utils/binance_parser.hpp"
#include "utils/bybit_parser.hpp"
#include "utils/enum_utils.hpp"
#include "utils/fixed_point.hpp"
#include "utils/math_utils.hpp"
#include "utils/simdcomp.hpp"
#include "utils/sse_double_int64_utils.hpp"
#include "utils/string_utils.hpp"
#include "utils/symbol_key_utils.hpp"
#include "utils/vbyte.hpp"
#include "utils/zip_utils.hpp"

#endif // _DFH_UTILS_HPP_INCLUDED

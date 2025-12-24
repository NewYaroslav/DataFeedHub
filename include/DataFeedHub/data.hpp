#pragma once
#ifndef _DTH_DATA_HPP_INCLUDED
#define _DTH_DATA_HPP_INCLUDED

/// \file data.hpp
/// \brief Основной include для домена данных DataFeedHub.
/// \details Собирает в одном месте ключевые типы, используемые при хранении, сериализации
/// и передаче рыночной информации между модулями. Включает:
/// - общие перечисления и флаги (`common.hpp`)
/// - структуры и кодеки тиков (`ticks.hpp`)
/// - структуры OHLCV-баров (`bars.hpp`)
/// - информацию о ставках финансирования (`funding.hpp`)
/// - типы для торговли и ордерной логики (`trading.hpp`)

//------------------------------------------------------------------------------
// Standard headers
//------------------------------------------------------------------------------

#include <array>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <functional>
#include <iostream>
#include <list>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>
#include <deque>
#include <list>
#include <map>
#include <unordered_map>

//------------------------------------------------------------------------------
// Third-party libraries
//------------------------------------------------------------------------------

#include <nlohmann/json.hpp>

//------------------------------------------------------------------------------
// Utility headers
//------------------------------------------------------------------------------

#include "utils/string_utils.hpp"
#include "utils/enum_utils.hpp"

//------------------------------------------------------------------------------
// Core data structures
//------------------------------------------------------------------------------

#include "data/common.hpp"
#include "data/ticks.hpp"
#include "data/ticks/TickConversions.hpp"
#if defined(DFH_USE_JSON) && defined(DFH_USE_NLOHMANN_JSON)
#include "data/ticks/TickJson.hpp"
#endif
#include "data/bars.hpp"
#include "data/funding.hpp"
#include "data/trading.hpp"

#endif // _DTH_DATA_HPP_INCLUDED

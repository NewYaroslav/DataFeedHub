#pragma once
#ifndef _DTH_DATA_HPP_INCLUDED
#define _DTH_DATA_HPP_INCLUDED

/// \file data.hpp
/// \brief Central include for all core data structures in the DataFeedHub domain.
///
/// This header aggregates commonly used data definitions, including:
/// - Common enums and constants (`common.hpp`)
/// - Tick data structures (`ticks.hpp`)
/// - Bar data structures (OHLCV) (`bars.hpp`)
/// - Funding rate data (`funding.hpp`)
/// - Trading-related types (`trading.hpp`)
///
/// These structures are used throughout the system for data storage, serialization,
/// and communication between modules.

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
#include <unordered_map>
#include <vector>

//------------------------------------------------------------------------------
// Third-party libraries
//------------------------------------------------------------------------------

#include <nlohmann/json.hpp>

//------------------------------------------------------------------------------
// Utility headers
//------------------------------------------------------------------------------

#include "utils/string_utils.hpp"

//------------------------------------------------------------------------------
// Core data structures
//------------------------------------------------------------------------------

#include "data/common.hpp"
#include "data/ticks.hpp"
#include "data/bars.hpp"
#include "data/funding.hpp"
#include "data/trading.hpp"

#endif // _DTH_DATA_HPP_INCLUDED

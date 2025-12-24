#pragma once
#ifndef _DTH_DATA_TICKS_HPP_INCLUDED
#define _DTH_DATA_TICKS_HPP_INCLUDED

/// \file ticks.hpp
/// \brief Umbrella-заголовок, объединяющий все типы и конфигурации тик-домена.

#include <array>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <list>
#include <map>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#if defined(DFH_USE_JSON) && defined(DFH_USE_NLOHMANN_JSON)
#include <nlohmann/json.hpp>
#endif

#include "common/enums.hpp"

#include "ticks/enums.hpp"
#include "ticks/flags.hpp"
#include "ticks/BidAskRestoreConfig.hpp"
#include "ticks/MarketTick.hpp"
#include "ticks/QuoteTick.hpp"
#include "ticks/QuoteTickVol.hpp"
#include "ticks/TradeTick.hpp"
#include "ticks/QuoteTickL1.hpp"
#include "ticks/ValueTick.hpp"
#include "ticks/SingleTick.hpp"
#include "ticks/TickSequence.hpp"
#include "ticks/TickMetadata.hpp"
#include "ticks/TickCodecConfig.hpp"
#include "ticks/TickSpan.hpp"
#include "ticks/TickConversions.hpp"
#if defined(DFH_USE_JSON) && defined(DFH_USE_NLOHMANN_JSON)
#include "ticks/TickJson.hpp"
#endif

#endif // _DTH_DATA_TICKS_HPP_INCLUDED

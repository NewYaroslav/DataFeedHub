#pragma once
#ifndef _DFH_DATA_BID_ASK_RESTORE_CONFIG_HPP_INCLUDED
#define _DFH_DATA_BID_ASK_RESTORE_CONFIG_HPP_INCLUDED

/// \file BidAskRestoreConfig.hpp
/// \brief Bid/ask price restoration configuration for historical market data.

namespace dfh {

    /// \struct BidAskRestoreConfig
    /// \brief Parameters used to reconstruct bid/ask series when only last prices are stored.
    struct BidAskRestoreConfig {
        BidAskModel mode{BidAskModel::NONE}; ///< Restoration algorithm selection.
        std::uint16_t fixed_spread{0};       ///< Fixed spread in price points for FIXED_SPREAD mode.
        std::uint16_t price_digits{0};       ///< Decimal precision for normalized prices.
    };

    static_assert(std::is_trivially_copyable_v<BidAskRestoreConfig>,
                  "BidAskRestoreConfig must remain a POD for shared-memory use.");

#if defined(DFH_USE_JSON) && defined(DFH_USE_NLOHMANN_JSON)

    /// \brief Serializes BidAskRestoreConfig to JSON.
    inline void to_json(nlohmann::json& j, const BidAskRestoreConfig& cfg) {
        j = nlohmann::json{
            {"mode", static_cast<std::uint32_t>(cfg.mode)},
            {"fixed_spread", cfg.fixed_spread},
            {"price_digits", cfg.price_digits}
        };
    }

    /// \brief Deserializes BidAskRestoreConfig from JSON.
    inline void from_json(const nlohmann::json& j, BidAskRestoreConfig& cfg) {
        std::uint32_t raw_mode = 0;
        j.at("mode").get_to(raw_mode);
        cfg.mode = static_cast<BidAskModel>(raw_mode);
        j.at("fixed_spread").get_to(cfg.fixed_spread);
        j.at("price_digits").get_to(cfg.price_digits);
    }

#endif // DFH_USE_JSON && DFH_USE_NLOHMANN_JSON

} // namespace dfh

#endif // _DFH_DATA_BID_ASK_RESTORE_CONFIG_HPP_INCLUDED

#pragma once
#ifndef _DFH_FUNDING_METADATA_HPP_INCLUDED
#define _DFH_FUNDING_METADATA_HPP_INCLUDED

/// \file FundingMetadata.hpp
/// \brief Defines the structure for funding metadata including funding period and next funding time.

namespace dfh {

	/// \struct FundingMetadata
    /// \brief Represents metadata for funding data.
	struct FundingMetadata {
        uint64_t start_ts;         ///< First funding timestamp (milliseconds since Unix epoch).
        uint64_t end_ts;           ///< Last funding timestamp (milliseconds since Unix epoch).
        uint64_t next_ts;  		   ///< Timestamp of the next funding event (milliseconds since Unix epoch).
		uint64_t period_ms;	       ///< Duration of the funding period in milliseconds.
        uint32_t symbol_id;        ///< Unique symbol identifier.
        uint32_t provider_id;      ///< Data provider identifier (exchange ID).
    };

} // namespace dfh

#endif // _DFH_FUNDING_METADATA_HPP_INCLUDED

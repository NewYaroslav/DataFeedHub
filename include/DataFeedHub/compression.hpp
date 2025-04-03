#pragma once
#ifndef _DFH_COMPRESSION_HPP_INCLUDED
#define _DFH_COMPRESSION_HPP_INCLUDED

/// \file compression.hpp
/// \brief High-level aggregated include for the DataFeedHub compression domain.
///
/// Provides access to all components for binary serialization and compression
/// of tick and bar market data. Includes:
/// - `compression/utils.hpp`: low-level encoding utilities (zig-zag, frequency, etc.)
/// - `compression/ticks.hpp`: tick serializers and compressors
/// - `compression/bars.hpp`: bar serializers
///
/// This header also includes required data and utility modules.

//------------------------------------------------------------------------------
// Standard headers
//------------------------------------------------------------------------------

#include <fstream>

//------------------------------------------------------------------------------
// Internal utility modules
//------------------------------------------------------------------------------

#include "utils.hpp"

//------------------------------------------------------------------------------
// Internal data definitions
//------------------------------------------------------------------------------

#include "data.hpp"

//------------------------------------------------------------------------------
// Compression domain
//------------------------------------------------------------------------------

#include "compression/utils.hpp"
#include "compression/ticks.hpp"
#include "compression/bars.hpp"

#endif // _DFH_COMPRESSION_HPP_INCLUDED

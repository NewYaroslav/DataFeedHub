#pragma once
#ifndef _DFH_TRANSFORM_HPP_INCLUDED
#define _DFH_TRANSFORM_HPP_INCLUDED

/// \file transform.hpp
/// \brief High-level data transformation utilities for bar and tick processing.
///
/// Provides access to transformation modules such as bar aggregation, resampling,
/// and gap-filling. These utilities are used in time series normalization and
/// preprocessing for analytics and model input preparation.

//------------------------------------------------------------------------------
// Standard headers
//------------------------------------------------------------------------------
#include <algorithm>
#include <vector>

//------------------------------------------------------------------------------
// Third-party libraries
//------------------------------------------------------------------------------

#include <time_shield.hpp>

//------------------------------------------------------------------------------
// Internal core data definitions
//------------------------------------------------------------------------------

#include "data.hpp"

//------------------------------------------------------------------------------
// Transform domain
//------------------------------------------------------------------------------

#include "transform/bars.hpp"
#include "transform/common.hpp"

#endif // _DFH_TRANSFORM_HPP_INCLUDED

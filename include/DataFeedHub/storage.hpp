#pragma once
#ifndef _DFH_STORAGE_HPP_INCLUDED
#define _DFH_STORAGE_HPP_INCLUDED

/// \file storage.hpp
/// \brief Central include for the DataFeedHub storage module.
///
/// Provides access to interfaces, common types, factory methods,
/// and backend implementations used for storing market data.

#include <mutex>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <stdexcept>
#include <filesystem>
#include <thread>

#include "data.hpp"
#include "utils.hpp"
#include "transform.hpp"
#include "compression.hpp"

//------------------------------------------------------------------------------
// Storage domain
//------------------------------------------------------------------------------

#include "storage/common.hpp"
#include "storage/mdbx.hpp"
#include "storage/factory.hpp"

#endif // _DFH_STORAGE_HPP_INCLUDED

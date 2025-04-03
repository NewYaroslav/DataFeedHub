#pragma once
#ifndef _DFH_STORAGE_HPP_INCLUDED
#define _DFH_STORAGE_HPP_INCLUDED

/// \file storage.hpp
/// \brief Central include for the DataFeedHub storage module.
/// \details Provides access to interfaces, common types, factory methods,
/// and backend implementations used for storing market data.

//------------------------------------------------------------------------------
// Standard Library Headers
//------------------------------------------------------------------------------

#include <mutex>
#include <filesystem>
#include <thread>

//------------------------------------------------------------------------------
// Core Module Headers
//------------------------------------------------------------------------------

#include "data.hpp"
#include "utils.hpp"
#include "transform.hpp"
#include "compression.hpp"

//------------------------------------------------------------------------------
// Storage Domain Headers
//------------------------------------------------------------------------------

#include "storage/common.hpp"
#include "storage/mdbx.hpp"
#include "storage/factory.hpp"

#endif // _DFH_STORAGE_HPP_INCLUDED

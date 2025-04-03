#pragma once
#ifndef _DFH_STORAGE_MDBX_HPP_INCLUDED
#define _DFH_STORAGE_MDBX_HPP_INCLUDED

/// \file mdbx.hpp
/// \brief Entry point for MDBX-based storage backend.
///
/// Includes core components for configuring, connecting, and using
/// MDBX as a backend for market data storage. This header provides
/// access to configuration (`MDBXConfig`), connection management
/// (`MDBXConnection`), transactions (`MDBXTransaction`), and the
/// main storage interface implementation (`MDBXMarketDataStorage`).

#include <mdbx.h>

#include "mdbx/MDBXConfig.hpp"
#include "mdbx/MDBXConnection.hpp"
#include "mdbx/MDBXTransaction.hpp"
#include "mdbx/MDBXMarketDataStorage.hpp"

#endif // _DFH_STORAGE_MDBX_HPP_INCLUDED

#pragma once
#ifndef _DFH_STORAGE_INTERFACES_HPP_INCLUDED
#define _DFH_STORAGE_INTERFACES_HPP_INCLUDED

/// \file interfaces.hpp
/// \brief Central include for storage-related interface declarations.
///
/// Aggregates all abstract interfaces used in the storage module,
/// including configuration, connection, transaction, and market data storage.

#include "interfaces/IConfig.hpp"
#include "interfaces/IConnection.hpp"
#include "interfaces/ITransaction.hpp"
#include "interfaces/IMarketDataStorage.hpp"
//#include "interfaces/ITransactionGuardAccess.hpp"

#endif // _DFH_STORAGE_INTERFACES_HPP_INCLUDED

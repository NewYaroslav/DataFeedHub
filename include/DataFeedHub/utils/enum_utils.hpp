#pragma once
#ifndef _DFH_UTILS_ENUM_UTILS_HPP_INCLUDED
#define _DFH_UTILS_ENUM_UTILS_HPP_INCLUDED

/// \file enum_utils.hpp
/// \brief Contains utility functions for working with enums.

namespace dfh {

    template <typename EnumType>
    EnumType to_enum(const std::string& str);

} // namespace dfh

#endif // _DFH_UTILS_ENUM_UTILS_HPP_INCLUDED

#pragma once
#ifndef _DFH_UTILS_ENUM_UTILS_HPP_INCLUDED
#define _DFH_UTILS_ENUM_UTILS_HPP_INCLUDED

/// \file enum_utils.hpp
/// \brief Утилиты для разбора строковых имен в значения перечислений.

#include <string>

namespace dfh {

    template <typename EnumType>
    /// \brief Преобразует строковое имя в значение перечисления `EnumType`.
    /// \param str Строковое имя элемента перечисления (ожидается точное совпадение).
    /// \return Значение перечисления, соответствующее этой строке.
    EnumType to_enum(const std::string& str);

} // namespace dfh

#endif // _DFH_UTILS_ENUM_UTILS_HPP_INCLUDED

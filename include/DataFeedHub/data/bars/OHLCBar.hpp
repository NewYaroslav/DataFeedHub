#pragma once
#ifndef _DFH_DATA_OHLC_BAR_HPP_INCLUDED
#define _DFH_DATA_OHLC_BAR_HPP_INCLUDED

/// \file OHLCBar.hpp
/// \brief Описывает простую структуру OHLC-бара без объёма.

namespace dfh {

    /// \struct OHLCBar
    /// \brief Структура бара с ценами Open-High-Low-Close без объёмов.
    struct OHLCBar {
        uint64_t time_ms; ///< Время начала бара в миллисекундах от эпохи Unix
        double open;      ///< Цена открытия
        double high;      ///< Максимальная цена
        double low;       ///< Минимальная цена
        double close;     ///< Цена закрытия

        /// \brief Конструктор по умолчанию, обнуляющий все поля.
        OHLCBar() : time_ms(0), open(0.0), high(0.0), low(0.0), close(0.0) {}

        /// \brief Конструктор с явным указанием цен и времени.
        /// \param o Цена открытия.
        /// \param h Максимальная цена.
        /// \param l Минимальная цена.
        /// \param c Цена закрытия.
        /// \param ts Время старта бара в миллисекундах.
        OHLCBar(double o, double h, double l, double c, uint64_t ts)
            : time_ms(ts), open(o), high(h), low(l), close(c) {}
    };

} // namespace dfh

#endif // _DFH_DATA_OHLC_BAR_HPP_INCLUDED

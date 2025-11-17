#pragma once
#ifndef _DFH_UTILS_DYNAMIC_BITSET_HPP_INCLUDED
#define _DFH_UTILS_DYNAMIC_BITSET_HPP_INCLUDED

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>

/// \file DynamicBitset.hpp
/// \brief Dynamic bitset implementation with optimized operations.

namespace dfh::utils {

    /// \class DynamicBitset
    /// \brief A class for managing a dynamic bitset with optimized performance.
    class DynamicBitset {
    public:
        /// \brief Constructor
        /// \param num_bits Number of bits in the bitset
        explicit DynamicBitset(size_t num_bits = 0) {
            resize(num_bits);
        }

        /// \brief Resize the bitset
        /// \param num_bits New size of the bitset
        void resize(size_t num_bits) {
            bits.resize((num_bits + 63) / 64, 0);
            this->num_bits = num_bits;
        }

        /// \brief Set a specific bit
        /// \param pos Bit position
        void set(size_t pos) {
            if (pos >= num_bits) throw std::out_of_range("Bit position out of range");
            bits[pos >> 6] |= (1ULL << (pos & 63));
        }

        /// \brief Set a specific bit
        /// \param pos Bit position
        /// \param value Boolean value to set
        void set(size_t pos, bool value = true) {
            if (pos >= num_bits) throw std::out_of_range("Bit position out of range");
            size_t block = pos >> 6;
            size_t offset = pos & 63;
            bits[block] = value ? bits[block] | (1ULL << offset) : bits[block] & ~(1ULL << offset);
        }

        /// \brief Reset a specific bit
        /// \param pos Bit position
        void reset(size_t pos) {
            if (pos >= num_bits) throw std::out_of_range("Bit position out of range");
            bits[pos >> 6] &= ~(1ULL << (pos & 63));
        }

        /// \brief Reset all bits
        void reset() {
            std::fill(bits.begin(), bits.end(), 0ULL);
        }

        /// \brief Test a specific bit
        /// \param pos Bit position
        /// \return Boolean value of the bit
        bool is_set(size_t pos) const {
            if (pos >= num_bits) throw std::out_of_range("Bit position out of range");
            return (bits[pos >> 6] >> (pos & 63)) & 1ULL;
        }

        /// \brief Bitwise AND
        /// \param other Another bitset
        /// \return Resulting bitset
        DynamicBitset operator&(const DynamicBitset& other) const {
            check_size(other);
            DynamicBitset result(num_bits);
            for (size_t i = 0; i < bits.size(); ++i) {
                result.bits[i] = bits[i] & other.bits[i];
            }
            return result;
        }

        /// \brief Bitwise OR
        /// \param other Another bitset
        /// \return Resulting bitset
        DynamicBitset operator|(const DynamicBitset& other) const {
            check_size(other);
            DynamicBitset result(num_bits);
            for (size_t i = 0; i < bits.size(); ++i) {
                result.bits[i] = bits[i] | other.bits[i];
            }
            return result;
        }

        /// \brief Logical left shift
        /// \param shift Number of bits to shift
        /// \return Resulting bitset
        DynamicBitset operator<<(size_t shift) const {
            DynamicBitset result(num_bits);
            if (shift >= num_bits) return result;

            size_t block_shift = shift >> 6;
            size_t bit_shift = shift & 63;
            size_t inverse_shift = 64 - bit_shift;

            for (size_t i = 0; i < bits.size(); ++i) {
                size_t target_block = i + block_shift;
                if (target_block < bits.size()) {
                    result.bits[target_block] |= bits[i] << bit_shift;
                    target_block++;
                    if (bit_shift != 0 && target_block < bits.size()) {
                        result.bits[target_block] |= bits[i] >> inverse_shift;
                    }
                }
            }
            return result;
        }

        /// \brief Logical right shift
        /// \param shift Number of bits to shift
        /// \return Resulting bitset
        DynamicBitset operator>>(size_t shift) const {
            DynamicBitset result(num_bits);
            if (shift >= num_bits) return result;

            size_t block_shift = shift >> 6;
            size_t bit_shift = shift & 63;
            size_t inverse_shift = 64 - bit_shift;
            size_t target_block = 0;

            for (size_t i = block_shift; i < bits.size(); ++i) {
                target_block = i - block_shift;
                result.bits[target_block] |= bits[i] >> bit_shift;
                if (bit_shift != 0 && i > block_shift) {
                    target_block--;
                    result.bits[target_block] |= bits[i] << inverse_shift;
                }
            }
            return result;
        }

        /// \brief Bitwise AND assignment
        /// \param other Another bitset
        /// \return Reference to this bitset
        DynamicBitset& operator&=(const DynamicBitset& other) {
            check_size(other);
            for (size_t i = 0; i < bits.size(); ++i) {
                bits[i] &= other.bits[i];
            }
            return *this;
        }

        /// \brief Bitwise OR assignment
        /// \param other Another bitset
        /// \return Reference to this bitset
        DynamicBitset& operator|=(const DynamicBitset& other) {
            check_size(other);
            for (size_t i = 0; i < bits.size(); ++i) {
                bits[i] |= other.bits[i];
            }
            return *this;
        }

        /// \brief Logical left shift assignment
        /// \param shift Number of bits to shift
        /// \return Reference to this bitset
        DynamicBitset& operator<<=(size_t shift) {
            if (shift >= num_bits) {
                reset();
                return *this;
            }
            size_t block_shift = shift >> 6;
            size_t bit_shift = shift & 63;
            size_t num_blocks = bits.size();
            size_t inverse_shift = 64 - bit_shift;

            if (block_shift > 0) {
                for (size_t i = num_blocks; i-- > block_shift;) {
                    bits[i] = bits[i - block_shift];
                }
                std::fill(bits.begin(), bits.begin() + block_shift, 0ULL);
            }

            if (bit_shift > 0) {
                uint64_t carry = 0;
                uint64_t temp;
                for (size_t i = 0; i < num_blocks; ++i) {
                    temp = bits[i] >> inverse_shift;
                    bits[i] = (bits[i] << bit_shift) | carry;
                    carry = temp;
                }
            }
            return *this;
        }

        /// \brief Logical right shift assignment
        /// \param shift Number of bits to shift
        /// \return Reference to this bitset
        DynamicBitset& operator>>=(size_t shift) {
            if (shift >= num_bits) {
                reset();
                return *this;
            }
            size_t block_shift = shift >> 6;
            size_t bit_shift = shift & 63;
            size_t num_blocks = bits.size();
            size_t inverse_shift = 64 - bit_shift;

            if (block_shift > 0) {
                for (size_t i = 0; i < num_blocks - block_shift; ++i) {
                    bits[i] = bits[i + block_shift];
                }
                std::fill(bits.end() - block_shift, bits.end(), 0ULL);
            }

            if (bit_shift > 0) {
                uint64_t carry = 0;
                uint64_t temp;
                for (size_t i = num_blocks; i-- > 0;) {
                    temp = bits[i] << inverse_shift;
                    bits[i] = (bits[i] >> bit_shift) | carry;
                    carry = temp;
                }
            }
            return *this;
        }

        /// \brief Get indices of set bits
        /// \return Vector containing indices where bits are set to 1
        std::vector<size_t> indices_of_set_bits() const {
            std::vector<size_t> indices;
            indices.reserve(num_bits);
            for (size_t i = 0; i < num_bits; ++i) {
                if (is_set(i)) indices.push_back(i);
            }
            return indices;
        }

        size_t size() const {
            return num_bits;
        }

        void clear() {
            bits.clear();
            num_bits = 0;
        }

    private:
        std::vector<uint64_t> bits;
        size_t num_bits = 0;

        // Ensures both bitsets have identical sizes.
        void check_size(const DynamicBitset& other) const {
            if (num_bits != other.num_bits) {
                throw std::invalid_argument("Bitsets must be of the same size");
            }
        }
    };

};

#endif // _DFH_UTILS_DYNAMIC_BITSET_HPP_INCLUDED

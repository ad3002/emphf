#pragma once

#include <xmmintrin.h>
#include <vector>
#include <cassert>
#include <ostream>
#include <istream>
#include <utility>
#include <cstdint>

namespace emphf {

    static const uint64_t ones_step_4 = 0x1111111111111111ULL;
    static const uint64_t ones_step_8 = 0x0101010101010101ULL;

    class bitpair_vector {
    public:
        bitpair_vector() noexcept
            : m_size(0)
        {}

        explicit bitpair_vector(uint64_t n)
            : m_size(0)
        {
            resize(n);
        }

        void resize(uint64_t n)
        {
            assert(n >= size());
            m_size = n;
            m_bits.resize((m_size + 31) / 32);
        }

        size_t size() const noexcept
        {
            return m_size;
        }

        size_t mem_size() const noexcept
        {
            return m_bits.size() * sizeof(m_bits[0]);
        }

        uint64_t operator[](uint64_t pos) const noexcept
        {
            return (m_bits[pos / 32] >> ((pos % 32) * 2)) & 3;
        }

        void set(uint64_t pos, uint64_t val)
        {
            assert(val < 4);
            uint64_t word_pos = pos / 32;
            uint64_t word_offset = (pos % 32) * 2;
            m_bits[word_pos] &= ~(3ULL << word_offset);
            m_bits[word_pos] |= val << word_offset;
        }

        uint64_t range_nonzeros(uint64_t begin, uint64_t end) const
        {
            assert(begin <= end);
            assert(end <= size());

            uint64_t word_begin = begin / 32;
            uint64_t offset_begin = (begin % 32) * 2;
            uint64_t word_end = end / 32;
            uint64_t offset_end = (end % 32) * 2;
            uint64_t r = 0;

            if (word_begin == word_end) {
                uint64_t mask = ((uint64_t(1) << offset_end) - 1) & ~((uint64_t(1) << offset_begin) - 1);
                r += nonzero_pairs(m_bits[word_begin] & mask);
                return r;
            }

            uint64_t word = (m_bits[word_begin] >> offset_begin) << offset_begin;
            r += nonzero_pairs(word);

            for (uint64_t w = word_begin + 1; w < word_end; ++w) {
                r += nonzero_pairs(m_bits[w]);
            }

            uint64_t mask = (uint64_t(1) << offset_end) - 1;
            r += nonzero_pairs(m_bits[word_end] & mask);

            return r;
        }

        void swap(bitpair_vector& other) noexcept
        {
            std::swap(m_size, other.m_size);
            m_bits.swap(other.m_bits);
        }

        void save(std::ostream& os) const
        {
            os.write(reinterpret_cast<const char*>(&m_size), sizeof(m_size));
            os.write(reinterpret_cast<const char*>(m_bits.data()), static_cast<std::streamsize>(sizeof(m_bits[0]) * m_bits.size()));
        }

        void load(std::istream& is)
        {
            is.read(reinterpret_cast<char*>(&m_size), sizeof(m_size));
            m_bits.resize((m_size + 31) / 32);
            is.read(reinterpret_cast<char*>(m_bits.data()), static_cast<std::streamsize>(sizeof(m_bits[0]) * m_bits.size()));
        }

        const std::vector<uint64_t>& data() const noexcept
        {
            return m_bits;
        }

    protected:
        std::vector<uint64_t> m_bits;
        uint64_t m_size;

    private:
        static constexpr uint64_t nonzero_pairs(uint64_t x) noexcept
        {
            x = (x | (x >> 1)) & (0x5 * ones_step_4);

#if EMPHF_USE_POPCOUNT
            return static_cast<uint64_t>(__builtin_popcountll(x));
#else
            x = (x & 3 * ones_step_4) + ((x >> 2) & 3 * ones_step_4);
            x = (x + (x >> 4)) & 0x0f * ones_step_8;
            return (x * ones_step_8) >> 56;
#endif
        }
    };

}

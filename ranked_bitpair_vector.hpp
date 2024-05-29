#pragma once

#include <cstdint>
#include <vector>
#include <cassert>
#include <ostream>
#include <istream>
#include "emphf_config.hpp"
#include "bitpair_vector.hpp"

namespace emphf {

    class ranked_bitpair_vector {
    public:
        ranked_bitpair_vector() noexcept = default;

        void build(bitpair_vector&& bv) noexcept
        {
            m_bv.swap(bv);
            m_block_ranks.clear();

            uint64_t cur_rank = 0;
            const auto& words = m_bv.data();
            for (size_t i = 0; i < words.size(); ++i) {
                if (((i * 32) % pairs_per_block) == 0) {
                    m_block_ranks.push_back(cur_rank);
                }
                cur_rank += nonzero_pairs(words[i]);
            }
        }

        size_t size() const noexcept
        {
            return m_bv.size();
        }

        size_t mem_size() const noexcept
        {
            return m_bv.mem_size() + m_block_ranks.size() * sizeof(m_block_ranks[0]);
        }

        uint64_t operator[](uint64_t pos) const noexcept
        {
            return m_bv[pos];
        }

        uint64_t rank(uint64_t pos) const noexcept
        {
            uint64_t word_idx = pos / 32;
            uint64_t word_offset = pos % 32;
            uint64_t block = pos / pairs_per_block;
            uint64_t r = m_block_ranks[block];

            for (uint64_t w = block * pairs_per_block / 32; w < word_idx; ++w) {
                r += nonzero_pairs(m_bv.data()[w]);
            }

            uint64_t mask = (static_cast<uint64_t>(1) << (word_offset * 2)) - 1;
            r += nonzero_pairs(m_bv.data()[word_idx] & mask);

            return r;
        }

        void swap(ranked_bitpair_vector& other) noexcept
        {
            m_bv.swap(other.m_bv);
            m_block_ranks.swap(other.m_block_ranks);
        }

        void save(std::ostream& os) const
        {
            m_bv.save(os);
            assert(m_block_ranks.size() == (m_bv.size() + pairs_per_block - 1) / pairs_per_block);
            os.write(reinterpret_cast<const char*>(m_block_ranks.data()),
                     static_cast<std::streamsize>(sizeof(m_block_ranks[0]) * m_block_ranks.size()));
        }

        void load(std::istream& is)
        {
            m_bv.load(is);
            m_block_ranks.resize((m_bv.size() + pairs_per_block - 1) / pairs_per_block);
            is.read(reinterpret_cast<char*>(m_block_ranks.data()),
                    static_cast<std::streamsize>(sizeof(m_block_ranks[0]) * m_block_ranks.size()));
        }

    protected:
        static constexpr uint64_t pairs_per_block = 512;
        bitpair_vector m_bv;
        std::vector<uint64_t> m_block_ranks;

    private:
        static constexpr uint64_t nonzero_pairs(uint64_t x) noexcept
        {
            constexpr uint64_t ones_step_4 = 0x1111111111111111ULL;
            constexpr uint64_t ones_step_8 = 0x0101010101010101ULL;

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

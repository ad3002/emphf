#pragma once

#include <cstdint>
#include <tuple>
#include <algorithm>
#include <cstring>
#include "common.hpp"
#include "platform_compat.hpp"

namespace emphf {

    // Cross-platform safe unaligned load function
    EMPHF_FORCE_INLINE uint64_t unaligned_load64(const uint8_t* from) noexcept
    {
        uint64_t tmp;
        
#if EMPHF_ARCH_X86_64
        // On x86_64, use direct memcpy - it's optimized and safe
        std::memcpy(&tmp, from, sizeof(tmp));
#elif EMPHF_ARCH_ARM64
        // On ARM64, unaligned access is handled by hardware, use memcpy
        std::memcpy(&tmp, from, sizeof(tmp));
#else
        // For unknown architectures, use byte-by-byte copy to be safe
        for (size_t i = 0; i < 8; ++i) {
            reinterpret_cast<uint8_t*>(&tmp)[i] = from[i];
        }
#endif
        
        return tmp;
    }

    struct jenkins64_hasher {

        using seed_t = uint64_t;
        using hash_t = uint64_t;
        using hash_triple_t = std::tuple<hash_t, hash_t, hash_t>;

        jenkins64_hasher() noexcept = default;

        explicit jenkins64_hasher(uint64_t seed) noexcept
            : m_seed(seed) {}

        template <typename Rng>
        static jenkins64_hasher generate(Rng& rng)
        {
            return jenkins64_hasher(rng());
        }

        // Adapted from http://www.burtleburtle.net/bob/c/lookup8.c
        hash_triple_t operator()(byte_range_t s) const noexcept
        {
            hash_triple_t h(m_seed, m_seed, 0x9e3779b97f4a7c13ULL);

            size_t len = static_cast<size_t>(s.second - s.first);
            const uint8_t* cur = s.first;
            const uint8_t* end = s.second;

            while (end - cur >= 24) {
                std::get<0>(h) += unaligned_load64(cur);
                cur += 8;
                std::get<1>(h) += unaligned_load64(cur);
                cur += 8;
                std::get<2>(h) += unaligned_load64(cur);
                cur += 8;

                mix(h);
            }

            std::get<2>(h) += len;

            switch (end - cur) {
                case 23: std::get<2>(h) += (uint64_t(cur[22]) << 56); [[fallthrough]];
                case 22: std::get<2>(h) += (uint64_t(cur[21]) << 48); [[fallthrough]];
                case 21: std::get<2>(h) += (uint64_t(cur[20]) << 40); [[fallthrough]];
                case 20: std::get<2>(h) += (uint64_t(cur[19]) << 32); [[fallthrough]];
                case 19: std::get<2>(h) += (uint64_t(cur[18]) << 24); [[fallthrough]];
                case 18: std::get<2>(h) += (uint64_t(cur[17]) << 16); [[fallthrough]];
                case 17: std::get<2>(h) += (uint64_t(cur[16]) << 8);  [[fallthrough]];
                // the first byte of c is reserved for the length
                case 16: std::get<1>(h) += (uint64_t(cur[15]) << 56); [[fallthrough]];
                case 15: std::get<1>(h) += (uint64_t(cur[14]) << 48); [[fallthrough]];
                case 14: std::get<1>(h) += (uint64_t(cur[13]) << 40); [[fallthrough]];
                case 13: std::get<1>(h) += (uint64_t(cur[12]) << 32); [[fallthrough]];
                case 12: std::get<1>(h) += (uint64_t(cur[11]) << 24); [[fallthrough]]; 
                case 11: std::get<1>(h) += (uint64_t(cur[10]) << 16); [[fallthrough]];
                case 10: std::get<1>(h) += (uint64_t(cur[9]) << 8);   [[fallthrough]];
                case  9: std::get<1>(h) += (uint64_t(cur[8]));        [[fallthrough]];
                case  8: std::get<0>(h) += (uint64_t(cur[7]) << 56);  [[fallthrough]];
                case  7: std::get<0>(h) += (uint64_t(cur[6]) << 48);  [[fallthrough]];
                case  6: std::get<0>(h) += (uint64_t(cur[5]) << 40);  [[fallthrough]];
                case  5: std::get<0>(h) += (uint64_t(cur[4]) << 32);  [[fallthrough]];
                case  4: std::get<0>(h) += (uint64_t(cur[3]) << 24);  [[fallthrough]];
                case  3: std::get<0>(h) += (uint64_t(cur[2]) << 16);  [[fallthrough]];
                case  2: std::get<0>(h) += (uint64_t(cur[1]) << 8);   [[fallthrough]];
                case  1: std::get<0>(h) += (uint64_t(cur[0]));        [[fallthrough]];
                case  0: break; // nothing to add
                default: assert(false);
            }

            mix(h);

            return h;
        }

        // rehash a hash triple
        hash_triple_t operator()(hash_triple_t h) const noexcept
        {
            std::get<0>(h) += m_seed;
            std::get<1>(h) += m_seed;
            std::get<2>(h) += 0x9e3779b97f4a7c13ULL;

            mix(h);

            return h;
        }

        void swap(jenkins64_hasher& other) noexcept
        {
            std::swap(m_seed, other.m_seed);
        }

        void save(std::ostream& os) const
        {
            os.write(reinterpret_cast<const char*>(&m_seed), sizeof(m_seed));
        }

        void load(std::istream& is)
        {
            is.read(reinterpret_cast<char*>(&m_seed), sizeof(m_seed));
        }

        seed_t seed() const noexcept
        {
            return m_seed;
        }

    protected:
        static void mix(hash_triple_t& h) noexcept
        {
            uint64_t& a = std::get<0>(h);
            uint64_t& b = std::get<1>(h);
            uint64_t& c = std::get<2>(h);

            a -= b; a -= c; a ^= (c >> 43);
            b -= c; b -= a; b ^= (a << 9);
            c -= a; c -= b; c ^= (b >> 8);
            a -= b; a -= c; a ^= (c >> 38);
            b -= c; b -= a; b ^= (a << 23);
            c -= a; c -= b; c ^= (b >> 5);
            a -= b; a -= c; a ^= (c >> 35);
            b -= c; b -= a; b ^= (a << 49);
            c -= a; c -= b; c ^= (b >> 11);
            a -= b; a -= c; a ^= (c >> 12);
            b -= c; b -= a; b ^= (a << 18);
            c -= a; c -= b; c ^= (b >> 22);
        }

        seed_t m_seed;
    };

    struct jenkins32_hasher {

        using seed_t = uint32_t;
        using hash_t = uint32_t;
        using hash_triple_t = std::tuple<hash_t, hash_t, hash_t>;

        jenkins32_hasher() noexcept = default;

        explicit jenkins32_hasher(uint32_t seed) noexcept
            : m_seed(seed) {}

        template <typename Rng>
        static jenkins32_hasher generate(Rng& rng)
        {
            return jenkins32_hasher(static_cast<uint32_t>(rng()));
        }

        hash_triple_t operator()(byte_range_t s) const noexcept
        {
            auto h64 = jenkins64_hasher(seed64())(s);
            return hash_triple_t(static_cast<uint32_t>(std::get<0>(h64)),
                                 static_cast<uint32_t>(std::get<1>(h64)),
                                 static_cast<uint32_t>(std::get<2>(h64)));
        }

        hash_triple_t operator()(hash_triple_t h) const noexcept
        {
            auto h64 = jenkins64_hasher::hash_triple_t(static_cast<uint64_t>(std::get<0>(h)),
                                                       static_cast<uint64_t>(std::get<1>(h)),
                                                       static_cast<uint64_t>(std::get<2>(h)));
            h64 = jenkins64_hasher(seed64())(h64);
            return hash_triple_t(static_cast<uint32_t>(std::get<0>(h64)),
                                 static_cast<uint32_t>(std::get<1>(h64)),
                                 static_cast<uint32_t>(std::get<2>(h64)));
        }

        void swap(jenkins32_hasher& other) noexcept
        {
            std::swap(m_seed, other.m_seed);
        }

        void save(std::ostream& os) const
        {
            os.write(reinterpret_cast<const char*>(&m_seed), sizeof(m_seed));
        }

        void load(std::istream& is)
        {
            is.read(reinterpret_cast<char*>(&m_seed), sizeof(m_seed));
        }

        seed_t seed() const noexcept
        {
            return m_seed;
        }

    protected:
        uint64_t seed64() const noexcept
        {
            return (static_cast<uint64_t>(m_seed) << 32) | m_seed;
        }

        seed_t m_seed;
    };

}

#pragma once

#include <random>
#include <cmath>
#include <limits>
#include <iterator>
#include <stdexcept>
#include <utility>

#include "bitpair_vector.hpp"
#include "ranked_bitpair_vector.hpp"
#include "perfutils.hpp"

namespace emphf {

    template <typename BaseHasher>
    class mphf {
    public:
        mphf() noexcept = default;

        template <typename HypergraphSorter, typename Range, typename Adaptor>
        mphf(HypergraphSorter& sorter, size_t n,
             const Range& input_range, Adaptor adaptor,
             double gamma = 1.23)
            : m_n(n)
            , m_hash_domain((static_cast<size_t>(std::ceil(static_cast<double>(m_n) * gamma)) + 2) / 3)
        {
            using node_t = typename HypergraphSorter::node_t;
            using hyperedge = typename HypergraphSorter::hyperedge;
            using value_type = decltype(*std::begin(input_range));

            size_t nodes_domain = m_hash_domain * 3;

            if (nodes_domain >= std::numeric_limits<node_t>::max()) {
                throw std::invalid_argument("Too many nodes for node_t");
            }

            auto edge_gen = [&](value_type s) {
                auto hashes = m_hasher(adaptor(s));
                return hyperedge(static_cast<node_t>(std::get<0>(hashes) % m_hash_domain),
                                 static_cast<node_t>(m_hash_domain + (std::get<1>(hashes) % m_hash_domain)),
                                 static_cast<node_t>(2 * m_hash_domain + (std::get<2>(hashes) % m_hash_domain)));
            };

            std::mt19937_64 rng(37); // deterministic seed

            for (size_t trial = 0; ; ++trial) {
                logger() << "Hypergraph generation: trial " << trial << std::endl;
                m_hasher = BaseHasher::generate(rng);
                if (sorter.try_generate_and_sort(input_range, edge_gen, m_n, m_hash_domain)) break;
            }

            auto peeling_order = sorter.get_peeling_order();
            bitpair_vector bv(nodes_domain);

            logger() << "Assigning values" << std::endl;
            for (auto edge = peeling_order.first; edge != peeling_order.second; ++edge) {
                uint64_t target = orientation(*edge);
                uint64_t assigned = bv[edge->v1] + bv[edge->v2];

                // "assigned values" must be nonzeros to be ranked, so
                // if the result is 0 we assign 3
                bv.set(edge->v0, ((target - assigned + 9) % 3) ?: 3);
            }

            m_bv.build(std::move(bv));
        }

        uint64_t size() const noexcept
        {
            return m_n;
        }

        const BaseHasher& base_hasher() const noexcept
        {
            return m_hasher;
        }

        template <typename T, typename Adaptor>
        uint64_t lookup(T val, Adaptor adaptor) const noexcept
        {
            auto hashes = m_hasher(adaptor(val));
            uint64_t nodes[3] = {std::get<0>(hashes) % m_hash_domain,
                                 m_hash_domain + (std::get<1>(hashes) % m_hash_domain),
                                 2 * m_hash_domain + (std::get<2>(hashes) % m_hash_domain)};

            uint64_t hidx = (m_bv[nodes[0]] + m_bv[nodes[1]] + m_bv[nodes[2]]) % 3;
            return m_bv.rank(nodes[hidx]);
        }

        void swap(mphf& other) noexcept
        {
            std::swap(m_n, other.m_n);
            std::swap(m_hash_domain, other.m_hash_domain);
            m_hasher.swap(other.m_hasher);
            m_bv.swap(other.m_bv);
        }

        void save(std::ostream& os) const
        {
            os.write(reinterpret_cast<const char*>(&m_n), sizeof(m_n));
            os.write(reinterpret_cast<const char*>(&m_hash_domain), sizeof(m_hash_domain));
            m_hasher.save(os);
            m_bv.save(os);
        }

        void load(std::istream& is)
        {
            is.read(reinterpret_cast<char*>(&m_n), sizeof(m_n));
            is.read(reinterpret_cast<char*>(&m_hash_domain), sizeof(m_hash_domain));
            m_hasher.load(is);
            m_bv.load(is);
        }

    private:
        uint64_t m_n = 0;
        uint64_t m_hash_domain = 0;
        BaseHasher m_hasher;
        ranked_bitpair_vector m_bv;
    };
}

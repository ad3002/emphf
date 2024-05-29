#pragma once

#include <cassert>
#include <cstdint>
#include <tuple>
#include <cmath>
#include <vector>
#include <iterator>
#include <algorithm>
#include <stdexcept>

#include "common.hpp"
#include "hypergraph.hpp"
#include "perfutils.hpp"

namespace emphf {

    template <typename HypergraphType>
    class hypergraph_sorter_seq {
    public:
        using hg = HypergraphType;
        using node_t = typename hg::node_t;
        using hyperedge = typename hg::hyperedge;
        using xored_adj_list = typename hg::xored_adj_list;

        hypergraph_sorter_seq() noexcept = default;

        template <typename Range, typename EdgeGenerator>
        bool try_generate_and_sort(const Range& input_range,
                                   const EdgeGenerator& edge_gen,
                                   size_t n,
                                   size_t hash_domain,
                                   bool verbose = true)
        {
            using std::get;
            size_t m = hash_domain * 3;

            m_peeling_order.clear();
            m_peeling_order.reserve(n);
            std::vector<xored_adj_list> adj_lists(m);

            if (verbose) {
                logger() << "Generating hyperedges and populating adjacency lists" << std::endl;
            }

            for (const auto& val : input_range) {
                auto edge = edge_gen(val);
                assert(orientation(edge) == 0);

                adj_lists[edge.v0].add_edge(edge);

                std::swap(edge.v0, edge.v1);
                adj_lists[edge.v0].add_edge(edge);

                std::swap(edge.v0, edge.v2);
                adj_lists[edge.v0].add_edge(edge);
            }

            if (verbose) {
                logger() << "Peeling" << std::endl;
            }

            auto visit = [&](node_t v0) {
                if (adj_lists[v0].degree == 1) {
                    auto edge = adj_lists[v0].edge_from(v0);
                    m_peeling_order.push_back(edge);

                    edge = canonicalize_edge(edge);
                    adj_lists[edge.v0].delete_edge(edge);

                    std::swap(edge.v0, edge.v1);
                    adj_lists[edge.v0].delete_edge(edge);

                    std::swap(edge.v0, edge.v2);
                    adj_lists[edge.v0].delete_edge(edge);
                }
            };

            size_t queue_position = 0;
            for (node_t v0 = 0; v0 < m; ++v0) {
                visit(v0);

                while (queue_position < m_peeling_order.size()) {
                    const auto& cur_edge = m_peeling_order[queue_position];

                    visit(cur_edge.v1);
                    visit(cur_edge.v2);
                    queue_position += 1;
                }
            }

            if (m_peeling_order.size() < n) {
                if (verbose) {
                    logger() << "Hypergraph is not peelable: " << (n - m_peeling_order.size()) << " edges remaining" << std::endl;
                }
                return false;
            }

            assert(m_peeling_order.size() == n);

            return true;
        }

        using peeling_iterator = typename std::vector<hyperedge>::const_reverse_iterator;

        std::pair<peeling_iterator, peeling_iterator> get_peeling_order() const noexcept
        {
            return {m_peeling_order.crbegin(), m_peeling_order.crend()};
        }

    private:
        std::vector<hyperedge> m_peeling_order;
    };
}

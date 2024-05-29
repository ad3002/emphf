#pragma once

#include <tuple>
#include <iostream>
#include <cassert>
#include <algorithm>

namespace emphf {

    template <typename NodeType>
    struct hypergraph {

        using node_t = NodeType; // last value is used as sentinel

        struct hyperedge {
            NodeType v0, v1, v2;

            hyperedge() noexcept = default;

            hyperedge(NodeType v0_, NodeType v1_, NodeType v2_) noexcept
                : v0(v0_), v1(v1_), v2(v2_) {}

            friend inline std::ostream& operator<<(std::ostream& os, const hyperedge& t) {
                os << "(" << t.v0 << ", " << t.v1 << ", " << t.v2 << ")";
                return os;
            }

            friend inline bool operator<(const hyperedge& lhs, const hyperedge& rhs) noexcept {
                return std::tie(lhs.v0, lhs.v1, lhs.v2) < std::tie(rhs.v0, rhs.v1, rhs.v2);
            }

            friend inline bool operator==(const hyperedge& lhs, const hyperedge& rhs) noexcept {
                return lhs.v0 == rhs.v0 && lhs.v1 == rhs.v1 && lhs.v2 == rhs.v2;
            }

            friend inline bool operator!=(const hyperedge& lhs, const hyperedge& rhs) noexcept {
                return !(lhs == rhs);
            }
        };

        static hyperedge sentinel() noexcept {
            return hyperedge(-node_t(1), -node_t(1), -node_t(1));
        }

        struct xored_adj_list {
            node_t degree;
            node_t v1s;
            node_t v2s;

            xored_adj_list(node_t degree_ = 0, node_t v1s_ = 0, node_t v2s_ = 0) noexcept
                : degree(degree_), v1s(v1s_), v2s(v2s_) {}

            void add_edge(const hyperedge& edge) noexcept {
                degree += 1;
                xor_edge(edge);
            }

            void delete_edge(const hyperedge& edge) noexcept {
                assert(degree >= 1);
                degree -= 1;
                xor_edge(edge);
            }

            hyperedge edge_from(node_t v0) const noexcept {
                assert(degree == 1);
                return hyperedge(v0, v1s, v2s);
            }

        private:
            void xor_edge(const hyperedge& edge) noexcept {
                assert(edge.v1 < edge.v2);
                v1s ^= edge.v1;
                v2s ^= edge.v2;
            }
        };
    };

    // a brief note about hyperedge orientations: throughout the
    // code we keep the invariant that for every hyperedge (v0,
    // v1, v2) it holds v1 < v2. This leaves only three
    // orientations, which we index with 0, 1, and 2 depending on
    // whether v0 is the first, second, or third smallest node. We
    // call the 0-orientation "canonical".
    template <typename HyperEdge>
    constexpr unsigned orientation(const HyperEdge& t) noexcept {
        assert(t.v1 <= t.v2);
        return (t.v0 > t.v1) + (t.v0 > t.v2);
    }

    template <typename HyperEdge>
    HyperEdge canonicalize_edge(HyperEdge t) noexcept {
        assert(t.v1 <= t.v2);
        if (t.v0 > t.v2) {
            std::swap(t.v0, t.v2);
        }

        if (t.v0 > t.v1) {
            std::swap(t.v0, t.v1);
        }

        assert(orientation(t) == 0);
        return t;
    }
}

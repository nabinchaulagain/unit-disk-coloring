#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace udg {

using Vertex = std::uint32_t;
using Graph = std::vector<std::vector<Vertex>>;

struct ColoringResult {
    std::vector<std::uint32_t> colors;
    std::uint32_t color_count = 0;
};

ColoringResult first_fit(const Graph& graph, const std::vector<Vertex>& order);

ColoringResult random_order_first_fit(const Graph& graph, std::uint64_t seed);

ColoringResult dsatur(const Graph& graph);

bool is_valid_coloring(const Graph& graph, const ColoringResult& coloring);

}

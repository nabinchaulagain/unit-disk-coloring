#pragma once

#include "udg/coloring.hpp"

#include <istream>
#include <ostream>

namespace udg {

Graph read_edge_list(std::istream& input);
Graph read_sorted_edge_list(std::istream& input);
void write_edge_list(const Graph& graph, std::ostream& output);

}

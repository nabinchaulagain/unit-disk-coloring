#pragma once

#include "udg/unit_disk_graph.hpp"

#include <cstddef>
#include <istream>
#include <vector>

namespace udg {

std::vector<Point> read_points_csv(std::istream& input, std::size_t expected_vertices);

}

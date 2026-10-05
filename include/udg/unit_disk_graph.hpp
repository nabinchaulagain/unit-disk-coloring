#pragma once

#include "udg/coloring.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace udg {

struct Point {
    double x;
    double y;
};

std::vector<Point> generate_uniform_points(std::size_t count,
                                           double width,
                                           double height,
                                           std::uint64_t seed);

Graph build_unit_disk_graph(const std::vector<Point>& points, double threshold);

}

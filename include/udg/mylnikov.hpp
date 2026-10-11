#pragma once

#include "udg/unit_disk_graph.hpp"

#include <cstddef>
#include <istream>
#include <vector>

namespace udg {

struct GeographicBounds {
  double min_latitude;
  double max_latitude;
  double min_longitude;
  double max_longitude;
};

struct GeographicPoint {
  double latitude;
  double longitude;
};

struct MylnikovSelection {
  std::vector<GeographicPoint> coordinates;
  std::size_t rows_read = 0;
  std::size_t invalid_rows = 0;
};

MylnikovSelection read_mylnikov_csv(std::istream &input,
                                    const GeographicBounds &bounds);

std::vector<Point>
project_equirectangular(const std::vector<GeographicPoint> &coordinates,
                        double reference_latitude, double reference_longitude);

}

#include "udg/mylnikov.hpp"

#include <cmath>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace udg {

namespace {

constexpr double earth_radius_meters = 6371008.8;
constexpr double pi = 3.141592653589793238462643383279502884;

std::vector<std::string> parse_csv_row(const std::string &row) {
  std::vector<std::string> fields;
  std::string field;
  bool quoted = false;

  for (std::size_t i = 0; i < row.size(); ++i) {
    const char character = row[i];
    if (character == '"') {
      if (quoted && i + 1 < row.size() && row[i + 1] == '"') {
        field.push_back('"');
        ++i;
      } else {
        quoted = !quoted;
      }
    } else if (character == ',' && !quoted) {
      fields.push_back(field);
      field.clear();
    } else if (character != '\r') {
      field.push_back(character);
    }
  }
  if (quoted) {
    throw std::runtime_error("unterminated quoted field in CSV row");
  }
  fields.push_back(field);
  return fields;
}

double parse_coordinate(const std::string &text) {
  std::size_t consumed = 0;
  const double value = std::stod(text, &consumed);
  if (consumed != text.size() || !std::isfinite(value)) {
    throw std::invalid_argument("invalid coordinate");
  }
  return value;
}

void validate_bounds(const GeographicBounds &bounds) {
  const bool finite = std::isfinite(bounds.min_latitude) &&
                      std::isfinite(bounds.max_latitude) &&
                      std::isfinite(bounds.min_longitude) &&
                      std::isfinite(bounds.max_longitude);
  if (!finite || bounds.min_latitude < -90.0 || bounds.max_latitude > 90.0 ||
      bounds.min_longitude < -180.0 || bounds.max_longitude > 180.0 ||
      bounds.min_latitude >= bounds.max_latitude ||
      bounds.min_longitude >= bounds.max_longitude) {
    throw std::invalid_argument("invalid geographic bounds");
  }
}

}

MylnikovSelection read_mylnikov_csv(std::istream &input,
                                    const GeographicBounds &bounds) {
  validate_bounds(bounds);

  std::string header_line;
  if (!std::getline(input, header_line)) {
    throw std::runtime_error("Mylnikov CSV is empty");
  }
  auto header = parse_csv_row(header_line);
  if (!header.empty() && header.front().size() >= 3 &&
      static_cast<unsigned char>(header.front()[0]) == 0xefU &&
      static_cast<unsigned char>(header.front()[1]) == 0xbbU &&
      static_cast<unsigned char>(header.front()[2]) == 0xbfU) {
    header.front().erase(0, 3);
  }

  std::size_t latitude_column = header.size();
  std::size_t longitude_column = header.size();
  for (std::size_t i = 0; i < header.size(); ++i) {
    if (header[i] == "lat") {
      latitude_column = i;
    } else if (header[i] == "lon") {
      longitude_column = i;
    }
  }
  if (latitude_column == header.size() || longitude_column == header.size()) {
    throw std::runtime_error("Mylnikov CSV must contain lat and lon columns");
  }

  MylnikovSelection selection;
  std::string row;
  while (std::getline(input, row)) {
    if (row.empty()) {
      continue;
    }
    ++selection.rows_read;
    try {
      const auto fields = parse_csv_row(row);
      if (latitude_column >= fields.size() ||
          longitude_column >= fields.size()) {
        ++selection.invalid_rows;
        continue;
      }
      const double latitude = parse_coordinate(fields[latitude_column]);
      const double longitude = parse_coordinate(fields[longitude_column]);
      if (latitude < -90.0 || latitude > 90.0 || longitude < -180.0 ||
          longitude > 180.0) {
        ++selection.invalid_rows;
        continue;
      }
      if (latitude >= bounds.min_latitude && latitude < bounds.max_latitude &&
          longitude >= bounds.min_longitude &&
          longitude < bounds.max_longitude) {
        selection.coordinates.push_back({latitude, longitude});
      }
    } catch (const std::exception &) {
      ++selection.invalid_rows;
    }
  }

  if (selection.coordinates.size() > std::numeric_limits<Vertex>::max()) {
    throw std::runtime_error(
        "selected row count exceeds the 32-bit vertex-ID limit");
  }
  return selection;
}

std::vector<Point>
project_equirectangular(const std::vector<GeographicPoint> &coordinates,
                        double reference_latitude, double reference_longitude) {
  if (!std::isfinite(reference_latitude) ||
      !std::isfinite(reference_longitude) || reference_latitude < -90.0 ||
      reference_latitude > 90.0 || reference_longitude < -180.0 ||
      reference_longitude > 180.0) {
    throw std::invalid_argument("invalid projection reference coordinate");
  }

  const double radians_per_degree = pi / 180.0;
  const double latitude_scale = earth_radius_meters * radians_per_degree;
  const double longitude_scale =
      latitude_scale * std::cos(reference_latitude * radians_per_degree);

  std::vector<Point> points;
  points.reserve(coordinates.size());
  for (const auto &coordinate : coordinates) {
    if (!std::isfinite(coordinate.latitude) ||
        !std::isfinite(coordinate.longitude) || coordinate.latitude < -90.0 ||
        coordinate.latitude > 90.0 || coordinate.longitude < -180.0 ||
        coordinate.longitude > 180.0) {
      throw std::invalid_argument("invalid geographic coordinate");
    }
    points.push_back(
        {(coordinate.longitude - reference_longitude) * longitude_scale,
         (coordinate.latitude - reference_latitude) * latitude_scale});
  }
  return points;
}

}

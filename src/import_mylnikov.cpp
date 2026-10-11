#include "udg/graph_io.hpp"
#include "udg/mylnikov.hpp"
#include "udg/unit_disk_graph.hpp"

#include <chrono>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

struct Options {
  std::string input_path;
  std::string output_path;
  std::string points_path;
  std::string metrics_path;
  udg::GeographicBounds bounds{};
  double threshold_meters = 0.0;
  bool has_min_latitude = false;
  bool has_max_latitude = false;
  bool has_min_longitude = false;
  bool has_max_longitude = false;
  bool has_threshold = false;
};

double parse_double(const std::string &text, const char *option) {
  std::size_t consumed = 0;
  const double value = std::stod(text, &consumed);
  if (consumed != text.size() || !std::isfinite(value)) {
    throw std::invalid_argument(std::string("invalid value for ") + option);
  }
  return value;
}

Options parse_options(int argc, char **argv) {
  Options options;
  for (int i = 1; i < argc; ++i) {
    const std::string argument = argv[i];
    auto require_value = [&](const char *name) -> std::string {
      if (++i >= argc) {
        throw std::invalid_argument(std::string("missing value for ") + name);
      }
      return argv[i];
    };

    if (argument == "--input") {
      options.input_path = require_value("--input");
    } else if (argument == "--output") {
      options.output_path = require_value("--output");
    } else if (argument == "--points") {
      options.points_path = require_value("--points");
    } else if (argument == "--metrics") {
      options.metrics_path = require_value("--metrics");
    } else if (argument == "--min-lat") {
      options.bounds.min_latitude =
          parse_double(require_value("--min-lat"), "--min-lat");
      options.has_min_latitude = true;
    } else if (argument == "--max-lat") {
      options.bounds.max_latitude =
          parse_double(require_value("--max-lat"), "--max-lat");
      options.has_max_latitude = true;
    } else if (argument == "--min-lon") {
      options.bounds.min_longitude =
          parse_double(require_value("--min-lon"), "--min-lon");
      options.has_min_longitude = true;
    } else if (argument == "--max-lon") {
      options.bounds.max_longitude =
          parse_double(require_value("--max-lon"), "--max-lon");
      options.has_max_longitude = true;
    } else if (argument == "--threshold-meters") {
      options.threshold_meters = parse_double(
          require_value("--threshold-meters"), "--threshold-meters");
      options.has_threshold = true;
    } else if (argument == "--help" || argument == "-h") {
      std::cout
          << "Usage: import_mylnikov --input CSV --output GRAPH --min-lat A "
             "--max-lat B --min-lon C --max-lon D --threshold-meters R "
             "[options]\n"
          << "  --points FILE   Write selected and projected coordinates to "
             "CSV\n"
          << "  --metrics FILE  Write graph-construction measurements to CSV\n";
      std::exit(0);
    } else {
      throw std::invalid_argument("unknown option: " + argument);
    }
  }

  if (options.input_path.empty()) {
    throw std::invalid_argument("--input is required");
  }
  if (options.output_path.empty()) {
    throw std::invalid_argument("--output is required");
  }
  if (!options.has_min_latitude || !options.has_max_latitude ||
      !options.has_min_longitude || !options.has_max_longitude) {
    throw std::invalid_argument("all four geographic bounds are required");
  }
  if (!options.has_threshold || options.threshold_meters <= 0.0) {
    throw std::invalid_argument(
        "--threshold-meters must be a finite positive value");
  }
  return options;
}

std::uint64_t edge_count(const udg::Graph &graph) {
  std::uint64_t degree_sum = 0;
  for (const auto &neighbors : graph) {
    degree_sum += neighbors.size();
  }
  return degree_sum / 2;
}

} // namespace

int main(int argc, char **argv) {
  try {
    const Options options = parse_options(argc, argv);
    std::ifstream input(options.input_path);
    if (!input) {
      throw std::runtime_error("cannot open input file: " + options.input_path);
    }

    const auto started = std::chrono::steady_clock::now();
    const auto selection = udg::read_mylnikov_csv(input, options.bounds);
    if (selection.coordinates.empty()) {
      throw std::runtime_error(
          "no records fall inside the selected geographic bounds");
    }
    const double reference_latitude =
        (options.bounds.min_latitude + options.bounds.max_latitude) / 2.0;
    const double reference_longitude =
        (options.bounds.min_longitude + options.bounds.max_longitude) / 2.0;
    const auto points = udg::project_equirectangular(
        selection.coordinates, reference_latitude, reference_longitude);
    const auto graph =
        udg::build_unit_disk_graph(points, options.threshold_meters);
    const auto finished = std::chrono::steady_clock::now();
    const double milliseconds =
        std::chrono::duration<double, std::milli>(finished - started).count();

    std::ofstream graph_file(options.output_path);
    if (!graph_file) {
      throw std::runtime_error("cannot open graph output: " +
                               options.output_path);
    }
    udg::write_edge_list(graph, graph_file);

    if (!options.points_path.empty()) {
      std::ofstream points_file(options.points_path);
      if (!points_file) {
        throw std::runtime_error("cannot open points output: " +
                                 options.points_path);
      }
      points_file << "vertex,latitude,longitude,x_meters,y_meters\n"
                  << std::setprecision(17);
      for (std::size_t vertex = 0; vertex < points.size(); ++vertex) {
        points_file << vertex << ',' << selection.coordinates[vertex].latitude
                    << ',' << selection.coordinates[vertex].longitude << ','
                    << points[vertex].x << ',' << points[vertex].y << '\n';
      }
    }

    if (!options.metrics_path.empty()) {
      std::ofstream metrics_file(options.metrics_path);
      if (!metrics_file) {
        throw std::runtime_error("cannot open metrics output: " +
                                 options.metrics_path);
      }
      metrics_file
          << "dataset,rows_read,invalid_rows,vertices,edges,threshold_meters,"
             "min_latitude,"
             "max_latitude,min_longitude,max_longitude,reference_latitude,"
             "reference_longitude,generation_ms\n"
          << "mylnikov_wifi," << selection.rows_read << ','
          << selection.invalid_rows << ',' << graph.size() << ','
          << edge_count(graph) << ',' << std::setprecision(17)
          << options.threshold_meters << ',' << options.bounds.min_latitude
          << ',' << options.bounds.max_latitude << ','
          << options.bounds.min_longitude << ',' << options.bounds.max_longitude
          << ',' << reference_latitude << ',' << reference_longitude << ','
          << std::fixed << std::setprecision(6) << milliseconds << '\n';
    }

    std::cerr << "dataset=mylnikov_wifi rows_read=" << selection.rows_read
              << " invalid_rows=" << selection.invalid_rows
              << " vertices=" << graph.size() << " edges=" << edge_count(graph)
              << " threshold_meters=" << options.threshold_meters
              << " generation_ms=" << std::fixed << std::setprecision(3)
              << milliseconds << '\n';
    return 0;
  } catch (const std::exception &error) {
    std::cerr << "error: " << error.what() << '\n';
    return 1;
  }
}

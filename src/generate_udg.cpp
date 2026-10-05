#include "udg/graph_io.hpp"
#include "udg/unit_disk_graph.hpp"

#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

namespace {

struct Options {
    std::size_t vertices = 0;
    double side = 0.0;
    double threshold = 1.0;
    std::uint64_t seed = 6405;
    std::string output_path;
    std::string points_path;
    std::string metrics_path;
    bool has_vertices = false;
    bool has_side = false;
};

std::uint64_t parse_u64(const std::string& text, const char* option) {
    std::size_t consumed = 0;
    const auto value = std::stoull(text, &consumed);
    if (consumed != text.size()) {
        throw std::invalid_argument(std::string("invalid value for ") + option);
    }
    return value;
}

double parse_positive_double(const std::string& text, const char* option) {
    std::size_t consumed = 0;
    const double value = std::stod(text, &consumed);
    if (consumed != text.size() || !std::isfinite(value) || value <= 0.0) {
        throw std::invalid_argument(std::string(option) + " must be a finite positive value");
    }
    return value;
}

Options parse_options(int argc, char** argv) {
    Options options;
    for (int i = 1; i < argc; ++i) {
        const std::string argument = argv[i];
        auto require_value = [&](const char* name) -> std::string {
            if (++i >= argc) {
                throw std::invalid_argument(std::string("missing value for ") + name);
            }
            return argv[i];
        };

        if (argument == "--vertices") {
            const std::uint64_t value = parse_u64(require_value("--vertices"), "--vertices");
            if (value > std::numeric_limits<udg::Vertex>::max() ||
                value > std::numeric_limits<std::size_t>::max()) {
                throw std::invalid_argument("--vertices exceeds the supported limit");
            }
            options.vertices = static_cast<std::size_t>(value);
            options.has_vertices = true;
        } else if (argument == "--side") {
            options.side = parse_positive_double(require_value("--side"), "--side");
            options.has_side = true;
        } else if (argument == "--threshold") {
            options.threshold =
                parse_positive_double(require_value("--threshold"), "--threshold");
        } else if (argument == "--seed") {
            options.seed = parse_u64(require_value("--seed"), "--seed");
        } else if (argument == "--output") {
            options.output_path = require_value("--output");
        } else if (argument == "--points") {
            options.points_path = require_value("--points");
        } else if (argument == "--metrics") {
            options.metrics_path = require_value("--metrics");
        } else if (argument == "--help" || argument == "-h") {
            std::cout
                << "Usage: generate_udg --vertices N --side L --output GRAPH [options]\n"
                << "  --threshold D  Adjacency distance threshold (default: 1)\n"
                << "  --seed S       Random seed (default: 6405)\n"
                << "  --points FILE  Write generated coordinates to CSV\n"
                << "  --metrics FILE Write graph-construction measurements to CSV\n";
            std::exit(0);
        } else {
            throw std::invalid_argument("unknown option: " + argument);
        }
    }

    if (!options.has_vertices) {
        throw std::invalid_argument("--vertices is required");
    }
    if (!options.has_side) {
        throw std::invalid_argument("--side is required");
    }
    if (options.output_path.empty()) {
        throw std::invalid_argument("--output is required");
    }
    return options;
}

std::uint64_t edge_count(const udg::Graph& graph) {
    std::uint64_t degree_sum = 0;
    for (const auto& neighbors : graph) {
        degree_sum += neighbors.size();
    }
    return degree_sum / 2;
}

}

int main(int argc, char** argv) {
    try {
        const Options options = parse_options(argc, argv);

        const auto started = std::chrono::steady_clock::now();
        const auto points = udg::generate_uniform_points(
            options.vertices, options.side, options.side, options.seed);
        const auto graph = udg::build_unit_disk_graph(points, options.threshold);
        const auto finished = std::chrono::steady_clock::now();
        const double milliseconds =
            std::chrono::duration<double, std::milli>(finished - started).count();

        std::ofstream graph_file(options.output_path);
        if (!graph_file) {
            throw std::runtime_error("cannot open graph output: " + options.output_path);
        }
        udg::write_edge_list(graph, graph_file);

        if (!options.points_path.empty()) {
            std::ofstream points_file(options.points_path);
            if (!points_file) {
                throw std::runtime_error("cannot open points output: " + options.points_path);
            }
            points_file << "vertex,x,y\n" << std::setprecision(17);
            for (std::size_t vertex = 0; vertex < points.size(); ++vertex) {
                points_file << vertex << ',' << points[vertex].x << ',' << points[vertex].y
                            << '\n';
            }
        }

        if (!options.metrics_path.empty()) {
            std::ofstream metrics_file(options.metrics_path);
            if (!metrics_file) {
                throw std::runtime_error("cannot open metrics output: " + options.metrics_path);
            }
            metrics_file
                << "generator,distribution,vertices,edges,threshold,width,height,seed,"
                   "generation_ms\n"
                << "spatial_grid,uniform," << graph.size() << ',' << edge_count(graph) << ','
                << std::setprecision(17) << options.threshold << ',' << options.side << ','
                << options.side << ',' << options.seed << ',' << std::fixed
                << std::setprecision(6) << milliseconds << '\n';
        }

        std::cerr << "distribution=uniform vertices=" << graph.size()
                  << " edges=" << edge_count(graph) << " threshold=" << options.threshold
                  << " side=" << options.side << " seed=" << options.seed
                  << " generation_ms=" << std::fixed << std::setprecision(3) << milliseconds
                  << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << '\n';
        return 1;
    }
}

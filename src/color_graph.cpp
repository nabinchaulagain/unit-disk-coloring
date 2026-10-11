#include "udg/coloring.hpp"
#include "udg/graph_io.hpp"
#include "udg/point_io.hpp"

#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

struct Options {
    std::string algorithm;
    std::string input_path;
    std::string points_path;
    std::string output_path;
    std::string colors_path;
    std::uint64_t seed = 6405;
};

std::uint64_t parse_seed(const std::string& text) {
    std::size_t consumed = 0;
    const std::uint64_t value = std::stoull(text, &consumed);
    if (consumed != text.size()) {
        throw std::invalid_argument("invalid value for --seed");
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

        if (argument == "--algorithm") {
            options.algorithm = require_value("--algorithm");
        } else if (argument == "--input") {
            options.input_path = require_value("--input");
        } else if (argument == "--points") {
            options.points_path = require_value("--points");
        } else if (argument == "--output") {
            options.output_path = require_value("--output");
        } else if (argument == "--colors") {
            options.colors_path = require_value("--colors");
        } else if (argument == "--seed") {
            options.seed = parse_seed(require_value("--seed"));
        } else if (argument == "--help" || argument == "-h") {
            std::cout << "Usage: color_graph --algorithm NAME --input GRAPH [options]\n"
                      << "  --algorithm NAME  geometric-first-fit, smallest-last, "
                         "random-first-fit or dsatur\n"
                      << "  --points FILE     Coordinate CSV required by "
                         "geometric-first-fit\n"
                      << "  --seed S          Random-first-fit seed (default: 6405)\n"
                      << "  --output FILE     Write one-row measurement CSV (default: "
                         "stdout)\n"
                      << "  --colors FILE     Write vertex,color assignments\n";
            std::exit(0);
        } else {
            throw std::invalid_argument("unknown option: " + argument);
        }
    }

    if (options.algorithm.empty()) {
        throw std::invalid_argument("--algorithm is required");
    }
    if (options.input_path.empty()) {
        throw std::invalid_argument("--input is required");
    }
    if (options.algorithm == "geometric-first-fit" && options.points_path.empty()) {
        throw std::invalid_argument("--points is required for geometric-first-fit");
    }
    if (options.algorithm != "geometric-first-fit" && options.algorithm != "smallest-last" &&
        options.algorithm != "random-first-fit" && options.algorithm != "dsatur") {
        throw std::invalid_argument("unknown algorithm: " + options.algorithm);
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
        std::ifstream input(options.input_path);
        if (!input) {
            throw std::runtime_error("cannot open input file: " + options.input_path);
        }
        const udg::Graph graph = udg::read_edge_list(input);

        std::vector<udg::Point> points;
        if (options.algorithm == "geometric-first-fit") {
            std::ifstream point_input(options.points_path);
            if (!point_input) {
                throw std::runtime_error("cannot open points file: " + options.points_path);
            }
            points = udg::read_points_csv(point_input, graph.size());
        }

        const auto started = std::chrono::steady_clock::now();
        udg::ColoringResult coloring;
        if (options.algorithm == "geometric-first-fit") {
            coloring = udg::geometric_first_fit(graph, points);
        } else if (options.algorithm == "smallest-last") {
            coloring = udg::smallest_last(graph);
        } else if (options.algorithm == "random-first-fit") {
            coloring = udg::random_order_first_fit(graph, options.seed);
        } else {
            coloring = udg::dsatur(graph);
        }
        const auto finished = std::chrono::steady_clock::now();

        const bool valid = udg::is_valid_coloring(graph, coloring);
        if (!valid) {
            throw std::logic_error("coloring algorithm returned an invalid result");
        }
        const double milliseconds =
            std::chrono::duration<double, std::milli>(finished - started).count();

        std::ofstream output_file;
        std::ostream* output = &std::cout;
        if (!options.output_path.empty()) {
            output_file.open(options.output_path);
            if (!output_file) {
                throw std::runtime_error("cannot open output file: " + options.output_path);
            }
            output = &output_file;
        }
        *output << "algorithm,vertices,edges,colors,coloring_ms,valid,seed\n"
                << options.algorithm << ',' << graph.size() << ',' << edge_count(graph) << ','
                << coloring.color_count << ',' << std::fixed << std::setprecision(6) << milliseconds
                << ",1,";
        if (options.algorithm == "random-first-fit") {
            *output << options.seed;
        }
        *output << '\n';

        if (!options.colors_path.empty()) {
            std::ofstream colors_file(options.colors_path);
            if (!colors_file) {
                throw std::runtime_error("cannot open colors file: " + options.colors_path);
            }
            colors_file << "vertex,color\n";
            for (std::size_t vertex = 0; vertex < coloring.colors.size(); ++vertex) {
                colors_file << vertex << ',' << coloring.colors[vertex] + 1 << '\n';
            }
        }

        std::cerr << "algorithm=" << options.algorithm << " vertices=" << graph.size()
                  << " edges=" << edge_count(graph) << " colors=" << coloring.color_count
                  << " coloring_ms=" << std::fixed << std::setprecision(3) << milliseconds
                  << " valid=1\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << '\n';
        return 1;
    }
}

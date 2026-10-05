#include "udg/coloring.hpp"
#include "udg/graph_io.hpp"

#include <chrono>
#include <cstdlib>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

struct Options {
    std::string input_path;
    std::string output_path;
    std::string colors_path;
};

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

        if (argument == "--input") {
            options.input_path = require_value("--input");
        } else if (argument == "--output") {
            options.output_path = require_value("--output");
        } else if (argument == "--colors") {
            options.colors_path = require_value("--colors");
        } else if (argument == "--help" || argument == "-h") {
            std::cout
                << "Usage: dsatur --input GRAPH [options]\n"
                << "  --output FILE  Write the one-row measurement CSV to FILE (default: stdout)\n"
                << "  --colors FILE  Write vertex,color assignments to FILE\n";
            std::exit(0);
        } else {
            throw std::invalid_argument("unknown option: " + argument);
        }
    }

    if (options.input_path.empty()) {
        throw std::invalid_argument("--input is required");
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

        const auto started = std::chrono::steady_clock::now();
        const auto coloring = udg::dsatur(graph);
        const auto finished = std::chrono::steady_clock::now();
        const bool valid = udg::is_valid_coloring(graph, coloring);
        if (!valid) {
            throw std::logic_error("internal error: DSATUR returned an invalid coloring");
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
        *output << "algorithm,vertices,edges,colors,coloring_ms,valid\n"
                << "dsatur," << graph.size() << ',' << edge_count(graph) << ','
                << coloring.color_count << ',' << std::fixed << std::setprecision(6)
                << milliseconds << ",1\n";

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

        std::cerr << "vertices=" << graph.size() << " edges=" << edge_count(graph)
                  << " colors=" << coloring.color_count << " coloring_ms=" << std::fixed
                  << std::setprecision(3) << milliseconds << " valid=1\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << '\n';
        return 1;
    }
}

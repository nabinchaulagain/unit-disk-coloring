#include "udg/graph_io.hpp"

#include <algorithm>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace udg {

namespace {

bool next_data_line(std::istream& input, std::string& line) {
    while (std::getline(input, line)) {
        const auto first = line.find_first_not_of(" \t\r");
        if (first != std::string::npos && line[first] != '#') {
            return true;
        }
    }
    return false;
}

}

Graph read_edge_list(std::istream& input) {
    std::string line;
    if (!next_data_line(input, line)) {
        throw std::runtime_error("edge-list input is empty");
    }

    std::uint64_t n64 = 0;
    std::uint64_t declared_edges = 0;
    {
        std::istringstream header(line);
        if (!(header >> n64 >> declared_edges)) {
            throw std::runtime_error("expected edge-list header: n m");
        }
    }
    if (n64 > std::numeric_limits<Vertex>::max()) {
        throw std::runtime_error("vertex count exceeds the 32-bit vertex-ID limit");
    }

    std::vector<std::pair<Vertex, Vertex>> edges;
    if (declared_edges <= edges.max_size()) {
        edges.reserve(static_cast<std::size_t>(declared_edges));
    }

    for (std::uint64_t i = 0; i < declared_edges; ++i) {
        if (!next_data_line(input, line)) {
            throw std::runtime_error("edge list ended before the declared edge count");
        }

        std::uint64_t u64 = 0;
        std::uint64_t v64 = 0;
        std::istringstream row(line);
        if (!(row >> u64 >> v64) || u64 >= n64 || v64 >= n64) {
            throw std::runtime_error("invalid or out-of-range edge endpoint");
        }
        if (u64 == v64) {
            throw std::runtime_error("self-loops are not allowed in a unit disk graph");
        }

        Vertex u = static_cast<Vertex>(u64);
        Vertex v = static_cast<Vertex>(v64);
        if (v < u) {
            std::swap(u, v);
        }
        edges.emplace_back(u, v);
    }

    if (next_data_line(input, line)) {
        throw std::runtime_error("edge list has data after the declared edge count");
    }

    std::sort(edges.begin(), edges.end());
    edges.erase(std::unique(edges.begin(), edges.end()), edges.end());

    Graph graph(static_cast<std::size_t>(n64));
    for (const auto& [u, v] : edges) {
        graph[u].push_back(v);
        graph[v].push_back(u);
    }
    return graph;
}

Graph read_sorted_edge_list(std::istream& input) {
    std::uint64_t n64 = 0;
    std::uint64_t declared_edges = 0;
    if (!(input >> n64 >> declared_edges)) {
        throw std::runtime_error("expected edge-list header: n m");
    }
    if (n64 > std::numeric_limits<Vertex>::max()) {
        throw std::runtime_error("vertex count exceeds the 32-bit vertex-ID limit");
    }

    Graph graph(static_cast<std::size_t>(n64));
    Vertex previous_u = 0;
    Vertex previous_v = 0;
    bool has_previous = false;

    for (std::uint64_t i = 0; i < declared_edges; ++i) {
        std::uint64_t u64 = 0;
        std::uint64_t v64 = 0;
        if (!(input >> u64 >> v64)) {
            throw std::runtime_error("edge list ended before the declared edge count");
        }
        if (u64 >= n64 || v64 >= n64 || u64 >= v64) {
            throw std::runtime_error("sorted edge list requires endpoints u < v");
        }

        const Vertex u = static_cast<Vertex>(u64);
        const Vertex v = static_cast<Vertex>(v64);
        if (has_previous && (u < previous_u || (u == previous_u && v <= previous_v))) {
            throw std::runtime_error(
                "sorted edge list must contain strictly increasing unique edges");
        }

        graph[u].push_back(v);
        graph[v].push_back(u);
        previous_u = u;
        previous_v = v;
        has_previous = true;
    }

    std::string extra;
    if (input >> extra) {
        throw std::runtime_error("edge list has data after the declared edge count");
    }
    return graph;
}

void write_edge_list(const Graph& graph, std::ostream& output) {
    std::uint64_t edges = 0;
    for (std::size_t vertex = 0; vertex < graph.size(); ++vertex) {
        for (const Vertex neighbor : graph[vertex]) {
            if (neighbor >= graph.size()) {
                throw std::invalid_argument("graph contains an out-of-range endpoint");
            }
            if (vertex < neighbor) {
                ++edges;
            }
        }
    }

    output << graph.size() << ' ' << edges << '\n';
    for (std::size_t vertex = 0; vertex < graph.size(); ++vertex) {
        for (const Vertex neighbor : graph[vertex]) {
            if (vertex < neighbor) {
                output << vertex << ' ' << neighbor << '\n';
            }
        }
    }
}

}

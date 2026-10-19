#include "udg/coloring.hpp"
#include "udg/graph_io.hpp"
#include "udg/mylnikov.hpp"
#include "udg/point_io.hpp"
#include "udg/unit_disk_graph.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>

namespace {

void expect(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error("test failed: " + message);
    }
}

udg::Graph graph_from_edges(std::size_t n,
                            const std::vector<std::pair<udg::Vertex, udg::Vertex>>& edges) {
    udg::Graph graph(n);
    for (const auto& [u, v] : edges) {
        graph[u].push_back(v);
        graph[v].push_back(u);
    }
    return graph;
}

std::set<std::pair<udg::Vertex, udg::Vertex>> edge_set(const udg::Graph& graph) {
    std::set<std::pair<udg::Vertex, udg::Vertex>> edges;
    for (std::size_t vertex = 0; vertex < graph.size(); ++vertex) {
        for (const auto neighbor : graph[vertex]) {
            if (vertex < neighbor) {
                edges.emplace(static_cast<udg::Vertex>(vertex), neighbor);
            }
        }
    }
    return edges;
}

udg::Graph all_pairs_unit_disk_graph(const std::vector<udg::Point>& points, double threshold) {
    udg::Graph graph(points.size());
    const double squared_threshold = threshold * threshold;
    for (std::size_t u = 0; u < points.size(); ++u) {
        for (std::size_t v = u + 1; v < points.size(); ++v) {
            const double x_difference = points[u].x - points[v].x;
            const double y_difference = points[u].y - points[v].y;
            if (x_difference * x_difference + y_difference * y_difference <= squared_threshold) {
                graph[u].push_back(static_cast<udg::Vertex>(v));
                graph[v].push_back(static_cast<udg::Vertex>(u));
            }
        }
    }
    return graph;
}

void test_empty_graph() {
    const udg::Graph graph;
    const auto result = udg::first_fit(graph, {});
    expect(result.color_count == 0, "empty graph should use zero colors");
    expect(udg::is_valid_coloring(graph, result), "empty coloring should be valid");
}

void test_clique() {
    const auto triangle = graph_from_edges(3, {{0, 1}, {0, 2}, {1, 2}});
    const auto result = udg::first_fit(triangle, {2, 0, 1});
    expect(result.color_count == 3, "triangle should use three colors");
    expect(udg::is_valid_coloring(triangle, result), "triangle coloring should be valid");
}

void test_order_sensitivity() {
    const auto path = graph_from_edges(4, {{0, 1}, {1, 2}, {2, 3}});
    const auto two_colors = udg::first_fit(path, {0, 1, 2, 3});
    const auto three_colors = udg::first_fit(path, {0, 3, 1, 2});
    expect(two_colors.color_count == 2, "natural path order should use two colors");
    expect(three_colors.color_count == 3, "adversarial path order should use three colors");
    expect(udg::is_valid_coloring(path, two_colors), "two-color path result should be valid");
    expect(udg::is_valid_coloring(path, three_colors), "three-color path result should be valid");
}

void test_seed_reproducibility() {
    const auto graph = graph_from_edges(5, {{0, 1}, {1, 2}, {2, 3}, {3, 4}, {4, 0}});
    const auto first = udg::random_order_first_fit(graph, 12345);
    const auto second = udg::random_order_first_fit(graph, 12345);
    expect(first.colors == second.colors, "equal seeds should produce equal color assignments");
    expect(first.color_count == second.color_count, "equal seeds should produce equal counts");
}

void test_dsatur_standard_graphs() {
    const auto triangle = graph_from_edges(3, {{0, 1}, {0, 2}, {1, 2}});
    const auto triangle_result = udg::dsatur(triangle);
    expect(triangle_result.color_count == 3, "DSATUR should use three colors on a triangle");
    expect(udg::is_valid_coloring(triangle, triangle_result),
           "DSATUR triangle coloring should be valid");

    const auto cycle = graph_from_edges(5, {{0, 1}, {1, 2}, {2, 3}, {3, 4}, {4, 0}});
    const auto cycle_result = udg::dsatur(cycle);
    expect(cycle_result.color_count == 3, "DSATUR should use three colors on an odd cycle");
    expect(udg::is_valid_coloring(cycle, cycle_result),
           "DSATUR odd-cycle coloring should be valid");

    const auto complete_bipartite = graph_from_edges(
        6, {{0, 3}, {0, 4}, {0, 5}, {1, 3}, {1, 4}, {1, 5}, {2, 3}, {2, 4}, {2, 5}});
    const auto bipartite_result = udg::dsatur(complete_bipartite);
    expect(bipartite_result.color_count == 2,
           "DSATUR should use two colors on a complete bipartite graph");
    expect(udg::is_valid_coloring(complete_bipartite, bipartite_result),
           "DSATUR bipartite coloring should be valid");
}

void test_geometric_first_fit() {
    const std::vector<udg::Point> points{{0.0, 0.0}, {0.6, 0.7}, {1.2, 0.0}, {0.7, -0.8}};
    const auto graph = udg::build_unit_disk_graph(points, 1.0);
    const auto result = udg::geometric_first_fit(graph, points);
    expect(result.color_count == 3, "geometric first-fit should follow x, y and vertex-ID order");
    expect(udg::is_valid_coloring(graph, result), "geometric first-fit coloring should be valid");

    const std::vector<udg::Point> tied_points{{0.0, 2.0}, {0.0, 0.0}, {0.0, 1.0}};
    const auto tied_graph = udg::build_unit_disk_graph(tied_points, 1.0);
    const auto tied_result = udg::geometric_first_fit(tied_graph, tied_points);
    expect(tied_result.color_count == 2,
           "geometric first-fit should break equal-x ties by y coordinate");
    expect(udg::is_valid_coloring(tied_graph, tied_result),
           "geometric first-fit tied-coordinate coloring should be valid");
}

void test_smallest_last() {
    const auto path = graph_from_edges(6, {{0, 1}, {1, 2}, {2, 3}, {3, 4}, {4, 5}});
    const auto path_result = udg::smallest_last(path);
    expect(path_result.color_count == 2, "smallest-last should use two colors on a path");
    expect(udg::is_valid_coloring(path, path_result),
           "smallest-last path coloring should be valid");

    const auto cycle = graph_from_edges(5, {{0, 1}, {1, 2}, {2, 3}, {3, 4}, {4, 0}});
    const auto cycle_result = udg::smallest_last(cycle);
    expect(cycle_result.color_count == 3, "smallest-last should use three colors on an odd cycle");
    expect(udg::is_valid_coloring(cycle, cycle_result),
           "smallest-last odd-cycle coloring should be valid");

    const auto triangle = graph_from_edges(3, {{0, 1}, {0, 2}, {1, 2}});
    const auto triangle_result = udg::smallest_last(triangle);
    expect(triangle_result.color_count == 3, "smallest-last should use three colors on a triangle");
}

udg::ColoringResult naive_dsatur(const udg::Graph& graph) {
    const auto uncolored = std::numeric_limits<std::uint32_t>::max();
    udg::ColoringResult result;
    result.colors.assign(graph.size(), uncolored);

    for (std::size_t step = 0; step < graph.size(); ++step) {
        udg::Vertex best = 0;
        std::size_t best_saturation = 0;
        std::size_t best_degree = 0;
        bool found = false;

        for (std::size_t vertex = 0; vertex < graph.size(); ++vertex) {
            if (result.colors[vertex] != uncolored) {
                continue;
            }
            std::unordered_set<std::uint32_t> neighbor_colors;
            std::size_t degree = 0;
            for (const auto neighbor : graph[vertex]) {
                if (result.colors[neighbor] == uncolored) {
                    ++degree;
                } else {
                    neighbor_colors.insert(result.colors[neighbor]);
                }
            }
            const bool is_better =
                !found || neighbor_colors.size() > best_saturation ||
                (neighbor_colors.size() == best_saturation && degree > best_degree) ||
                (neighbor_colors.size() == best_saturation && degree == best_degree &&
                 vertex < best);
            if (is_better) {
                best = static_cast<udg::Vertex>(vertex);
                best_saturation = neighbor_colors.size();
                best_degree = degree;
                found = true;
            }
        }

        std::vector<bool> used(graph.size(), false);
        for (const auto neighbor : graph[best]) {
            if (result.colors[neighbor] != uncolored) {
                used[result.colors[neighbor]] = true;
            }
        }
        std::uint32_t color = 0;
        while (color < used.size() && used[color]) {
            ++color;
        }
        result.colors[best] = color;
        result.color_count = std::max(result.color_count, color + 1);
    }
    return result;
}

void test_random_unit_disk_graphs() {
    for (std::size_t sample = 0; sample < 100; ++sample) {
        const std::size_t n = 1 + sample % 40;
        const double side = 2.0 + 0.5 * static_cast<double>(sample % 5);
        const auto points = udg::generate_uniform_points(n, side, side, 6405 + sample);
        const auto graph = udg::build_unit_disk_graph(points, 1.0);
        const auto all_pairs_graph = all_pairs_unit_disk_graph(points, 1.0);
        expect(edge_set(graph) == edge_set(all_pairs_graph),
               "spatial-grid construction should match all-pairs UDG construction");

        const auto expected = naive_dsatur(graph);
        const auto actual = udg::dsatur(graph);
        expect(actual.colors == expected.colors,
               "indexed-heap DSATUR should match the naive deterministic reference");
        expect(udg::is_valid_coloring(graph, actual),
               "DSATUR unit-disk-graph coloring should be valid");

        const auto geometric = udg::geometric_first_fit(graph, points);
        expect(udg::is_valid_coloring(graph, geometric),
               "geometric first-fit unit-disk-graph coloring should be valid");

        const auto smallest_last = udg::smallest_last(graph);
        expect(udg::is_valid_coloring(graph, smallest_last),
               "smallest-last unit-disk-graph coloring should be valid");
    }
}

void test_unit_disk_threshold_boundary() {
    const std::vector<udg::Point> points{{0.0, 0.0}, {1.0, 0.0}, {1.000000001, 0.0}, {0.0, 1.0}};
    const auto graph = udg::build_unit_disk_graph(points, 1.0);
    const std::set<std::pair<udg::Vertex, udg::Vertex>> expected{{0, 1}, {0, 3}, {1, 2}};
    expect(edge_set(graph) == expected,
           "UDG construction should include threshold edges and exclude longer "
           "edges");
}

void test_uniform_point_reproducibility() {
    const auto first = udg::generate_uniform_points(50, 10.0, 10.0, 6405);
    const auto second = udg::generate_uniform_points(50, 10.0, 10.0, 6405);
    expect(first.size() == second.size(), "equal seeds should produce equal point counts");
    for (std::size_t i = 0; i < first.size(); ++i) {
        expect(first[i].x == second[i].x && first[i].y == second[i].y,
               "equal seeds should reproduce the same points");
    }
}

void test_mylnikov_selection_and_projection() {
    std::istringstream input("id,bssid,lat,lon,updated,data\n"
                             "1,AA,48.500000,2.500000,0,0\n"
                             "2,BB,48.500000,2.501000,0,0\n"
                             "3,CC,49.000000,2.500000,0,0\n"
                             "4,DD,invalid,2.500000,0,0\n");
    const udg::GeographicBounds bounds{48.0, 49.0, 2.0, 3.0};
    const auto selection = udg::read_mylnikov_csv(input, bounds);
    expect(selection.rows_read == 4, "Mylnikov reader should count data rows");
    expect(selection.invalid_rows == 1, "Mylnikov reader should count invalid coordinates");
    expect(selection.coordinates.size() == 2,
           "Mylnikov reader should use half-open geographic bounds");

    const auto points = udg::project_equirectangular(selection.coordinates, 48.5, 2.5);
    expect(points.size() == 2, "projection should preserve the selected point count");
    expect(std::abs(points[0].x) < 1e-9 && std::abs(points[0].y) < 1e-9,
           "projection reference point should map to the origin");
    expect(points[1].x > 73.0 && points[1].x < 75.0,
           "one millidegree of longitude near Paris should project to about 74 "
           "meters");

    const auto graph = udg::build_unit_disk_graph(points, 100.0);
    expect(edge_set(graph) == std::set<std::pair<udg::Vertex, udg::Vertex>>{{0, 1}},
           "projected access points within 100 meters should be adjacent");
}

void test_mylnikov_duplicate_coordinates_remain_distinct() {
    std::istringstream input("id,bssid,lat,lon,updated,data\n"
                             "1,AA,48.1,2.1,0,0\n"
                             "2,BB,48.1,2.1,0,0\n");
    const udg::GeographicBounds bounds{48.0, 49.0, 2.0, 3.0};
    const auto selection = udg::read_mylnikov_csv(input, bounds);
    expect(selection.coordinates.size() == 2,
           "separate access-point records should remain separate vertices");
    const auto points = udg::project_equirectangular(selection.coordinates, 48.5, 2.5);
    const auto graph = udg::build_unit_disk_graph(points, 1.0);
    expect(edge_set(graph) == std::set<std::pair<udg::Vertex, udg::Vertex>>{{0, 1}},
           "co-located access points should be adjacent");
}

void test_point_csv_reader() {
    std::istringstream input("vertex,latitude,longitude,x_meters,y_meters\n"
                             "1,48.5,2.501,74.0,0.0\n"
                             "0,48.5,2.5,0.0,0.0\n");
    const auto points = udg::read_points_csv(input, 2);
    expect(points.size() == 2, "point CSV reader should preserve vertex count");
    expect(points[0].x == 0.0 && points[0].y == 0.0,
           "point CSV reader should place rows by vertex ID");
    expect(points[1].x == 74.0 && points[1].y == 0.0,
           "point CSV reader should accept projected meter columns");
}

void test_edge_list_reader() {
    std::istringstream input("# path with a duplicate edge\n4 4\n0 1\n1 2\n2 3\n1 0\n");
    const auto graph = udg::read_edge_list(input);
    expect(graph.size() == 4, "reader should preserve vertex count");
    expect(graph[0].size() == 1, "reader should remove duplicate undirected edges");
    expect(graph[1].size() == 2, "reader should build both adjacency directions");
}

void test_sorted_edge_list_reader() {
    std::istringstream input("4 3\n0 1\n1 2\n2 3\n");
    const auto graph = udg::read_sorted_edge_list(input);
    expect(graph.size() == 4, "sorted reader should preserve vertex count");
    expect(graph[1].size() == 2, "sorted reader should build both adjacency directions");

    std::istringstream unsorted("4 2\n1 2\n0 1\n");
    bool threw = false;
    try {
        static_cast<void>(udg::read_sorted_edge_list(unsorted));
    } catch (const std::runtime_error&) {
        threw = true;
    }
    expect(threw, "sorted reader should reject an unsorted edge list");
}

void test_invalid_order() {
    const udg::Graph graph(2);
    bool threw = false;
    try {
        static_cast<void>(udg::first_fit(graph, {0, 0}));
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    expect(threw, "duplicate vertex in an order should be rejected");
}

}

int main() {
    test_empty_graph();
    test_clique();
    test_order_sensitivity();
    test_seed_reproducibility();
    test_dsatur_standard_graphs();
    test_geometric_first_fit();
    test_smallest_last();
    test_random_unit_disk_graphs();
    test_unit_disk_threshold_boundary();
    test_uniform_point_reproducibility();
    test_mylnikov_selection_and_projection();
    test_mylnikov_duplicate_coordinates_remain_distinct();
    test_point_csv_reader();
    test_edge_list_reader();
    test_sorted_edge_list_reader();
    test_invalid_order();
}

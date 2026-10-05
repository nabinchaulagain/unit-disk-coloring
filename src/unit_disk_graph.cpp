#include "udg/unit_disk_graph.hpp"

#include <cmath>
#include <limits>
#include <random>
#include <stdexcept>
#include <unordered_map>
#include <utility>

namespace udg {

namespace {

struct Cell {
    std::int64_t x;
    std::int64_t y;

    bool operator==(const Cell& other) const {
        return x == other.x && y == other.y;
    }
};

struct CellHash {
    std::size_t operator()(const Cell& cell) const {
        std::uint64_t x = static_cast<std::uint64_t>(cell.x);
        std::uint64_t y = static_cast<std::uint64_t>(cell.y);
        x ^= x >> 30U;
        x *= 0xbf58476d1ce4e5b9ULL;
        x ^= x >> 27U;
        x *= 0x94d049bb133111ebULL;
        x ^= x >> 31U;
        y ^= y >> 30U;
        y *= 0xbf58476d1ce4e5b9ULL;
        y ^= y >> 27U;
        y *= 0x94d049bb133111ebULL;
        y ^= y >> 31U;
        return static_cast<std::size_t>(x ^ (y + 0x9e3779b97f4a7c15ULL + (x << 6U) +
                                             (x >> 2U)));
    }
};

std::int64_t cell_coordinate(double value, double threshold) {
    const double coordinate = std::floor(value / threshold);
    const double lower = static_cast<double>(std::numeric_limits<std::int64_t>::min() + 1);
    const double upper = static_cast<double>(std::numeric_limits<std::int64_t>::max() - 1);
    if (!std::isfinite(coordinate) || coordinate < lower || coordinate > upper) {
        throw std::invalid_argument("point coordinate is outside the supported grid range");
    }
    return static_cast<std::int64_t>(coordinate);
}

}

std::vector<Point> generate_uniform_points(std::size_t count,
                                           double width,
                                           double height,
                                           std::uint64_t seed) {
    if (!std::isfinite(width) || !std::isfinite(height) || width <= 0.0 || height <= 0.0) {
        throw std::invalid_argument("width and height must be finite positive values");
    }
    if (count > std::numeric_limits<Vertex>::max()) {
        throw std::invalid_argument("point count exceeds the 32-bit vertex-ID limit");
    }

    std::mt19937_64 generator(seed);
    std::uniform_real_distribution<double> x_distribution(0.0, width);
    std::uniform_real_distribution<double> y_distribution(0.0, height);

    std::vector<Point> points;
    points.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        points.push_back({x_distribution(generator), y_distribution(generator)});
    }
    return points;
}

Graph build_unit_disk_graph(const std::vector<Point>& points, double threshold) {
    if (!std::isfinite(threshold) || threshold <= 0.0) {
        throw std::invalid_argument("distance threshold must be a finite positive value");
    }
    if (points.size() > std::numeric_limits<Vertex>::max()) {
        throw std::invalid_argument("point count exceeds the 32-bit vertex-ID limit");
    }

    Graph graph(points.size());
    std::unordered_map<Cell, std::vector<Vertex>, CellHash> cells;
    cells.reserve(points.size());
    const double squared_threshold = threshold * threshold;
    if (!std::isfinite(squared_threshold)) {
        throw std::invalid_argument("distance threshold is too large");
    }

    for (std::size_t i = 0; i < points.size(); ++i) {
        const Point point = points[i];
        if (!std::isfinite(point.x) || !std::isfinite(point.y)) {
            throw std::invalid_argument("point coordinates must be finite");
        }
        const Cell cell{cell_coordinate(point.x, threshold),
                        cell_coordinate(point.y, threshold)};

        for (std::int64_t dx = -1; dx <= 1; ++dx) {
            for (std::int64_t dy = -1; dy <= 1; ++dy) {
                const Cell neighbor_cell{cell.x + dx, cell.y + dy};
                const auto found = cells.find(neighbor_cell);
                if (found == cells.end()) {
                    continue;
                }
                for (const Vertex j : found->second) {
                    const double x_difference = point.x - points[j].x;
                    const double y_difference = point.y - points[j].y;
                    const double squared_distance =
                        x_difference * x_difference + y_difference * y_difference;
                    if (squared_distance <= squared_threshold) {
                        const Vertex vertex = static_cast<Vertex>(i);
                        graph[vertex].push_back(j);
                        graph[j].push_back(vertex);
                    }
                }
            }
        }
        cells[cell].push_back(static_cast<Vertex>(i));
    }

    return graph;
}

}

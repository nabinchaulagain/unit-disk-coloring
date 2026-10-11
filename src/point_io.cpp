#include "udg/point_io.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace udg {

namespace {

std::vector<std::string> split_csv_row(const std::string& row) {
    std::vector<std::string> fields;
    std::string field;
    for (const char character : row) {
        if (character == ',') {
            fields.push_back(field);
            field.clear();
        } else if (character != '\r') {
            field.push_back(character);
        }
    }
    fields.push_back(field);
    return fields;
}

std::size_t find_column(const std::vector<std::string>& header, const std::string& primary,
                        const std::string& alternative = {}) {
    for (std::size_t column = 0; column < header.size(); ++column) {
        if (header[column] == primary || (!alternative.empty() && header[column] == alternative)) {
            return column;
        }
    }
    return header.size();
}

std::size_t parse_vertex(const std::string& text, std::size_t expected_vertices) {
    std::size_t consumed = 0;
    const std::uint64_t value = std::stoull(text, &consumed);
    if (consumed != text.size() || value >= expected_vertices ||
        value > std::numeric_limits<Vertex>::max()) {
        throw std::runtime_error("point CSV contains an invalid vertex ID");
    }
    return static_cast<std::size_t>(value);
}

double parse_coordinate(const std::string& text) {
    std::size_t consumed = 0;
    const double value = std::stod(text, &consumed);
    if (consumed != text.size() || !std::isfinite(value)) {
        throw std::runtime_error("point CSV contains an invalid coordinate");
    }
    return value;
}

}

std::vector<Point> read_points_csv(std::istream& input, std::size_t expected_vertices) {
    std::string line;
    if (!std::getline(input, line)) {
        throw std::runtime_error("point CSV is empty");
    }
    const auto header = split_csv_row(line);
    const std::size_t vertex_column = find_column(header, "vertex");
    const std::size_t x_column = find_column(header, "x", "x_meters");
    const std::size_t y_column = find_column(header, "y", "y_meters");
    if (vertex_column == header.size() || x_column == header.size() || y_column == header.size()) {
        throw std::runtime_error("point CSV must contain vertex, x and y columns");
    }

    std::vector<Point> points(expected_vertices);
    std::vector<bool> seen(expected_vertices, false);
    std::size_t rows = 0;
    while (std::getline(input, line)) {
        if (line.empty()) {
            continue;
        }
        const auto fields = split_csv_row(line);
        const std::size_t last_required = std::max(vertex_column, std::max(x_column, y_column));
        if (fields.size() <= last_required) {
            throw std::runtime_error("point CSV row has too few columns");
        }
        const std::size_t vertex = parse_vertex(fields[vertex_column], expected_vertices);
        if (seen[vertex]) {
            throw std::runtime_error("point CSV contains a duplicate vertex ID");
        }
        points[vertex] = {parse_coordinate(fields[x_column]), parse_coordinate(fields[y_column])};
        seen[vertex] = true;
        ++rows;
    }

    if (rows != expected_vertices) {
        throw std::runtime_error("point CSV must contain one row per graph vertex");
    }
    return points;
}

}

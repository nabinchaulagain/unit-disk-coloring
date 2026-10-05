#include "udg/coloring.hpp"

#include <algorithm>
#include <limits>
#include <numeric>
#include <random>
#include <stdexcept>
#include <unordered_set>

namespace udg {

namespace {

void validate_order(std::size_t n, const std::vector<Vertex>& order) {
    if (order.size() != n) {
        throw std::invalid_argument("coloring order must contain every vertex exactly once");
    }

    std::vector<bool> seen(n, false);
    for (const Vertex vertex : order) {
        if (vertex >= n || seen[vertex]) {
            throw std::invalid_argument("coloring order is not a vertex permutation");
        }
        seen[vertex] = true;
    }
}

class IndexedMaxHeap {
public:
    IndexedMaxHeap(const std::vector<std::uint32_t>& saturation,
                   const std::vector<std::size_t>& uncolored_degree)
        : saturation_(saturation),
          uncolored_degree_(uncolored_degree),
          heap_(saturation.size()),
          position_(saturation.size()) {
        std::iota(heap_.begin(), heap_.end(), Vertex{0});
        std::iota(position_.begin(), position_.end(), std::size_t{0});
        for (std::size_t i = heap_.size() / 2; i > 0; --i) {
            sift_down(i - 1);
        }
    }

    Vertex pop_max() {
        if (heap_.empty()) {
            throw std::logic_error("cannot remove a vertex from an empty heap");
        }
        const Vertex result = heap_.front();
        swap_positions(0, heap_.size() - 1);
        heap_.pop_back();
        position_[result] = not_in_heap;
        if (!heap_.empty()) {
            sift_down(0);
        }
        return result;
    }

    void saturation_increased(Vertex vertex) { sift_up(position_of(vertex)); }

    void degree_decreased(Vertex vertex) { sift_down(position_of(vertex)); }

private:
    static constexpr std::size_t not_in_heap = std::numeric_limits<std::size_t>::max();

    bool higher_priority(Vertex left, Vertex right) const {
        if (saturation_[left] != saturation_[right]) {
            return saturation_[left] > saturation_[right];
        }
        if (uncolored_degree_[left] != uncolored_degree_[right]) {
            return uncolored_degree_[left] > uncolored_degree_[right];
        }
        return left < right;
    }

    std::size_t position_of(Vertex vertex) const {
        if (vertex >= position_.size() || position_[vertex] == not_in_heap) {
            throw std::logic_error("attempted to update a colored vertex");
        }
        return position_[vertex];
    }

    void swap_positions(std::size_t left, std::size_t right) {
        std::swap(heap_[left], heap_[right]);
        position_[heap_[left]] = left;
        position_[heap_[right]] = right;
    }

    void sift_up(std::size_t index) {
        while (index > 0) {
            const std::size_t parent = (index - 1) / 2;
            if (!higher_priority(heap_[index], heap_[parent])) {
                break;
            }
            swap_positions(index, parent);
            index = parent;
        }
    }

    void sift_down(std::size_t index) {
        while (true) {
            const std::size_t left = 2 * index + 1;
            const std::size_t right = left + 1;
            std::size_t best = index;
            if (left < heap_.size() && higher_priority(heap_[left], heap_[best])) {
                best = left;
            }
            if (right < heap_.size() && higher_priority(heap_[right], heap_[best])) {
                best = right;
            }
            if (best == index) {
                break;
            }
            swap_positions(index, best);
            index = best;
        }
    }

    const std::vector<std::uint32_t>& saturation_;
    const std::vector<std::size_t>& uncolored_degree_;
    std::vector<Vertex> heap_;
    std::vector<std::size_t> position_;
};

}

ColoringResult first_fit(const Graph& graph, const std::vector<Vertex>& order) {
    const std::size_t n = graph.size();
    validate_order(n, order);

    const auto uncolored = std::numeric_limits<std::uint32_t>::max();
    ColoringResult result;
    result.colors.assign(n, uncolored);

    std::vector<std::size_t> marked_at(n, n);

    for (std::size_t step = 0; step < n; ++step) {
        const Vertex vertex = order[step];
        for (const Vertex neighbor : graph[vertex]) {
            if (neighbor >= n) {
                throw std::invalid_argument("graph contains an out-of-range endpoint");
            }
            const std::uint32_t neighbor_color = result.colors[neighbor];
            if (neighbor_color != uncolored) {
                marked_at[neighbor_color] = step;
            }
        }

        std::uint32_t color = 0;
        while (color < n && marked_at[color] == step) {
            ++color;
        }
        if (color == n) {
            throw std::logic_error("first-fit failed to find an available color");
        }

        result.colors[vertex] = color;
        result.color_count = std::max(result.color_count, color + 1);
    }

    return result;
}

ColoringResult random_order_first_fit(const Graph& graph, std::uint64_t seed) {
    std::vector<Vertex> order(graph.size());
    std::iota(order.begin(), order.end(), Vertex{0});

    std::mt19937_64 generator(seed);
    std::shuffle(order.begin(), order.end(), generator);
    return first_fit(graph, order);
}

ColoringResult dsatur(const Graph& graph) {
    const std::size_t n = graph.size();
    if (n > std::numeric_limits<Vertex>::max()) {
        throw std::invalid_argument("graph exceeds the 32-bit vertex-ID limit");
    }

    const auto uncolored = std::numeric_limits<std::uint32_t>::max();
    ColoringResult result;
    result.colors.assign(n, uncolored);
    if (n == 0) {
        return result;
    }

    std::vector<std::uint32_t> saturation(n, 0);
    std::vector<std::size_t> uncolored_degree(n, 0);
    for (std::size_t vertex = 0; vertex < n; ++vertex) {
        uncolored_degree[vertex] = graph[vertex].size();
        for (const Vertex neighbor : graph[vertex]) {
            if (neighbor >= n || neighbor == vertex) {
                throw std::invalid_argument("DSATUR requires a loop-free graph with valid endpoints");
            }
        }
    }

    IndexedMaxHeap candidates(saturation, uncolored_degree);
    std::unordered_set<std::uint64_t> seen_neighbor_colors;
    seen_neighbor_colors.max_load_factor(0.8F);

    std::vector<std::size_t> marked_at(n, n);

    for (std::size_t step = 0; step < n; ++step) {
        const Vertex vertex = candidates.pop_max();

        for (const Vertex neighbor : graph[vertex]) {
            const std::uint32_t neighbor_color = result.colors[neighbor];
            if (neighbor_color != uncolored) {
                marked_at[neighbor_color] = step;
            }
        }

        std::uint32_t color = 0;
        while (color < n && marked_at[color] == step) {
            ++color;
        }
        if (color == n) {
            throw std::logic_error("DSATUR failed to find an available color");
        }
        result.colors[vertex] = color;
        result.color_count = std::max(result.color_count, color + 1);

        for (const Vertex neighbor : graph[vertex]) {
            if (result.colors[neighbor] != uncolored) {
                continue;
            }

            if (uncolored_degree[neighbor] == 0) {
                throw std::invalid_argument("DSATUR requires a symmetric simple graph");
            }
            --uncolored_degree[neighbor];

            const std::uint64_t key =
                (static_cast<std::uint64_t>(neighbor) << 32U) | color;
            const bool is_new_color = seen_neighbor_colors.insert(key).second;
            if (is_new_color) {
                ++saturation[neighbor];
                candidates.saturation_increased(neighbor);
            } else {
                candidates.degree_decreased(neighbor);
            }
        }
    }

    return result;
}

bool is_valid_coloring(const Graph& graph, const ColoringResult& coloring) {
    if (coloring.colors.size() != graph.size()) {
        return false;
    }
    if (graph.empty()) {
        return coloring.color_count == 0;
    }
    if (coloring.color_count == 0) {
        return false;
    }

    for (std::size_t vertex = 0; vertex < graph.size(); ++vertex) {
        if (coloring.colors[vertex] >= coloring.color_count) {
            return false;
        }
        for (const Vertex neighbor : graph[vertex]) {
            if (neighbor >= graph.size() ||
                coloring.colors[vertex] == coloring.colors[neighbor]) {
                return false;
            }
        }
    }
    return true;
}

}

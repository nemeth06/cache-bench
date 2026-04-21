#pragma once

#include "plugin.hpp"
#include "utils.hpp"

#include <numeric>
#include <string>
#include <vector>

namespace matrix_data {

constexpr size_t rows = 2048;
constexpr size_t cols = 2048;

inline const std::vector<int>& matrix() {
    static const std::vector<int> m = []() {
        std::vector<int> tmp(rows * cols);
        std::iota(tmp.begin(), tmp.end(), 0);
        return tmp;
    }();
    return m;
}

}

struct MatrixTraversal {
    static std::string group_name() { return "matrix_traversal (2048x2048 ints)"; }
    static std::string name_a()     { return "row-major (cache-friendly)"; }
    static std::string name_b()     { return "col-major (cache-hostile)"; }

    static void run_a() {
        using namespace matrix_data;
        long long sum = 0;
        
        for (size_t r = 0; r < rows; ++r) {
            for (size_t c = 0; c < cols; ++c) {
                sum += matrix()[r * cols + c];
            }
        }
        
        do_not_optimize(sum);
    }

    static void run_b() {
        using namespace matrix_data;
        long long sum = 0;
        
        for (size_t c = 0; c < cols; ++c) {
            for (size_t r = 0; r < rows; ++r) {
                sum += matrix()[r * cols + c];
            }
        }
        
        do_not_optimize(sum);
    }
};
#pragma once

#include "plugin.hpp"
#include "utils.hpp"

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <map>
#include <random>
#include <string>
#include <vector>

namespace predecessor_data {

constexpr size_t max_n  = 50'000;

inline const std::vector<int>& keys() {
    static const std::vector<int> v = []() {
        std::mt19937 rng(42);
        std::uniform_int_distribution<int> dist(0, static_cast<int>(max_n) * 10);
        
        std::vector<int> tmp(max_n);
        for (auto& k : tmp) {
            k = dist(rng);
        }
        return tmp;
    }();
    return v;
}

inline void run_vector(size_t n) {
    std::vector<int> vec;
    vec.reserve(n);
    long long checksum = 0;
    
    for (size_t i = 0; i < n; ++i) {
        int key = keys()[i];
        auto it = std::lower_bound(vec.begin(), vec.end(), key);
        
        if (it != vec.begin()) {
            checksum += *std::prev(it);
        }
        vec.insert(it, key);
    }
    
    do_not_optimize(checksum);
    do_not_optimize(vec);
}

inline void run_map(size_t n) {
    std::map<int, int> m;
    long long checksum = 0;
    
    for (size_t i = 0; i < n; ++i) {
        int key = keys()[i];
        auto it = m.upper_bound(key);
        
        if (it != m.begin()) {
            checksum += std::prev(it)->first;
        }
        m[key]++;
    }
    
    do_not_optimize(checksum);
    do_not_optimize(m.size());
}

} // namespace predecessor_data

template <size_t N>
struct PredecessorBench {
    static std::string group_name() { return "predecessor_query (N=" + std::to_string(N) + ")"; }
    static std::string name_a()     { return "sorted vector (O(n) insert)"; }
    static std::string name_b()     { return "std::map (O(log n))"; }

    static void run_a() { predecessor_data::run_vector(N); }
    static void run_b() { predecessor_data::run_map(N); }
};
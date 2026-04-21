#pragma once

#include "plugin.hpp"
#include "utils.hpp"

#include <list>
#include <numeric>
#include <vector>
#include <algorithm>
#include <random>

// This data is constructed once at startup, not inside the timed section

namespace vector_vs_list_data {

constexpr int N = 1'000'000;

inline const std::vector<int>& vec() {
    static const std::vector<int> v = [](){
        std::vector<int> tmp(N);
        std::iota(tmp.begin(), tmp.end(), 1);
        return tmp;
    }();
    return v;
}

inline const std::list<int>& lst() {
    static const std::list<int> l = [](){
        // Allocate nodes (likely in contiguous memory)
        std::list<int> temp(vec().begin(), vec().end());

        // Collect iterators to all nodes
        std::vector<std::list<int>::iterator> iters;
        iters.reserve(N);
        for (auto it = temp.begin(); it != temp.end(); ++it) {
            iters.push_back(it);
        }

        // Shuffle the iterators
        std::mt19937 gen(1337);
        std::shuffle(iters.begin(), iters.end(), gen);

        // Relink the nodes in a randomized order using splice.
        // This alters the next/prev pointers without allocating new memory,
        // forcing the CPU to jump randomly across the memory arena.
        std::list<int> fragmented_list;
        for (auto it : iters) {
            fragmented_list.splice(fragmented_list.end(), temp, it);
        }

        return fragmented_list;
    }();
    return l;
}

}

// Plugin struct
struct VectorVsList {
    static std::string group_name() { return "vector_vs_list  (N=1,000,000)"; }
    static std::string name_a()     { return "std::vector  sum"; }
    static std::string name_b()     { return "std::list    sum"; }

    static void run_a() {
        long long sum = 0;
        for (int x : vector_vs_list_data::vec()) sum += x;
        do_not_optimize(sum);
    }

    static void run_b() {
        long long sum = 0;
        for (int x : vector_vs_list_data::lst()) sum += x;
        do_not_optimize(sum);
    }
};

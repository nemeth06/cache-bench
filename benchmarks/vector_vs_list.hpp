#pragma once

#include "plugin.hpp"
#include "utils.hpp"

#include <list>
#include <numeric>
#include <vector>

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
    static const std::list<int> l(vec().begin(), vec().end());
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

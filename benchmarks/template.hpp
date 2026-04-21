#pragma once

/*
 * TEMPLATE: Copy this file to create a new benchmark.
 *
 * 1. Copy to benchmarks/my_bench.hpp
 * 2. Rename the struct
 * 3. Set group_name, name_a, name_b
 * 4. Implement run_a() and run_b()
 * 5. In src/main.cpp:
 * #include "benchmarks/my_bench.hpp"
 * runner.add_plugin<MyBench>();
 */

#include "plugin.hpp"
#include "utils.hpp" 

#include <string>

// Put shared data here so it is constructed once at startup rather 
// than inside the timed section.
namespace my_bench_data {
    // inline const std::vector<int>& input() { static ... }
}

struct MyBench {
    static std::string group_name() { return "my_benchmark_group"; }
    static std::string name_a()     { return "algorithm_a"; }
    static std::string name_b()     { return "algorithm_b"; }

    // Both functions must do the same logical work for a fair comparison.
    // Wrap output values in do_not_optimize() to prevent dead-code elimination.
    
    static void run_a() {
        int result = 0;
        // ... first algorithm ...
        do_not_optimize(result);
    }

    static void run_b() {
        int result = 0;
        // ... second algorithm ...
        do_not_optimize(result);
    }
};
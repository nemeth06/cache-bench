#pragma once

#include "counters.hpp"
#include "utils.hpp"

#include <algorithm>
#include <chrono>
#include <string>
#include <vector>

// Results produced by a single benchmark run.
struct BenchmarkResult {
    std::string name;         // Display name for the table row
    std::string group;        // Group name (used for table separators)

    double    time_ms      = 0.0;   // Median wall-clock time per iteration (ms)
    long long l1_misses    = 0;     // Median L1 cache misses per iteration
    long long llc_misses   = 0;     // Median LLC misses per iteration
    long long branch_mispr = 0;     // Median branch mispredictions per iteration
    double    ipc          = 0.0;   // Median instructions per cycle
    bool      counters_ok  = false; // False when perf counters are unavailable
};

// Run a benchmark function, returning median measurements across iterations.
/// @param name       Row label shown in the output table.
/// @param group      Group label (pairs in the same group are printed together).
/// @param fn         The workload to benchmark.  Must be callable with no args.
/// @param perf       PerfCounters instance (shared across all benchmarks).
/// @param warmup     Number of unmeasured warmup iterations.
/// @param iterations Number of measured iterations.
template <typename Fn>
BenchmarkResult run_benchmark(const std::string& name,
                               const std::string& group,
                               Fn&&               fn,
                               PerfCounters&      perf,
                               int                warmup     = 3,
                               int                iterations = 20)
{
    // Warmup
    for (int i = 0; i < warmup; ++i) fn();

    // Store results of an iteration
    struct Sample {
        double    time_ms;
        long long l1, llc, br, cyc, ins;
    };
    std::vector<Sample> samples(static_cast<std::size_t>(iterations));

    for (int i = 0; i < iterations; ++i) {
        flush_cache();   // cold-cache conditions for each iteration

        perf.start();
        auto t0 = std::chrono::steady_clock::now();

        fn();

        auto t1 = std::chrono::steady_clock::now();
        perf.stop();

        auto c = perf.read();
        samples[static_cast<std::size_t>(i)] = {
            std::chrono::duration<double, std::milli>(t1 - t0).count(),
            c.l1_misses, c.llc_misses, c.branch_mispr, c.cycles, c.instructions
        };
    }

    // Median helper
    const std::size_t mid = static_cast<std::size_t>(iterations) / 2;

    auto get_median = [&](auto field) {
        // Deduce the type returned by the field accessor
        using value_t = std::decay_t<decltype(field(samples.front()))>;
        
        std::vector<value_t> v;
        v.reserve(samples.size());
        for (const auto& s : samples) {
            v.push_back(field(s));
        }
        
        // no sorting is needed, nth_element is O(n)
        std::nth_element(v.begin(), v.begin() + mid, v.end());
        return v[mid];
    };

    // Compute median of each field
    BenchmarkResult result;
    result.name        = name;
    result.group       = group;
    result.counters_ok = perf.available();

    result.time_ms      = get_median([](const Sample& s){ return s.time_ms; });
    result.l1_misses    = get_median([](const Sample& s){ return s.l1;      });
    result.llc_misses   = get_median([](const Sample& s){ return s.llc;     });
    result.branch_mispr = get_median([](const Sample& s){ return s.br;      });

    // IPC is a ratio
    // Compute per-iteration first, then take the median of the ratios
    result.ipc = get_median([](const Sample& s) -> double {
        return (s.cyc > 0) ? static_cast<double>(s.ins) / static_cast<double>(s.cyc)
                           : 0.0;
    });
    return result;
}


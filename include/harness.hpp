#pragma once

#include "counters.hpp"
#include "utils.hpp"

#include <algorithm>
#include <chrono>
#include <string>
#include <vector>
#include <type_traits>

struct BenchmarkResult {
    std::string name;
    std::string group;

    double    time_ms      = 0.0;
    long long l1_misses    = 0;
    long long llc_misses   = 0;
    long long branch_mispr = 0;
    double    ipc          = 0.0;
    bool      counters_ok  = false;
};

// Runs a benchmark workload and returns the median measurements.
// 'warmup' iterations are executed but not recorded.
template <typename Fn>
BenchmarkResult run_benchmark(const std::string& name,
                              const std::string& group,
                              Fn&&               fn,
                              PerfCounters&      perf,
                              size_t             warmup     = 3,
                              size_t             iterations = 20)
{
    for (size_t i = 0; i < warmup; ++i) {
        fn();
    }

    struct Sample {
        double    time_ms;
        long long l1, llc, br, cyc, ins;
    };
    std::vector<Sample> samples(iterations);

    for (size_t i = 0; i < iterations; ++i) {
        flush_cache(); // Ensure cold-cache conditions

        perf.start();
        auto t0 = std::chrono::steady_clock::now();

        fn();

        auto t1 = std::chrono::steady_clock::now();
        perf.stop();

        auto c = perf.read();
        samples[i] = {
            std::chrono::duration<double, std::milli>(t1 - t0).count(),
            c.l1_misses, c.llc_misses, c.branch_mispr, c.cycles, c.instructions
        };
    }

    auto get_median = [&](auto extract) {
        using value_t = std::decay_t<decltype(extract(samples.front()))>;
        
        std::vector<value_t> vals;
        vals.reserve(samples.size());
        for (const auto& s : samples) {
            vals.push_back(extract(s));
        }
        
        size_t mid = vals.size() / 2;
        std::nth_element(vals.begin(), vals.begin() + mid, vals.end());
        return vals[mid];
    };

    BenchmarkResult res;
    res.name        = name;
    res.group       = group;
    res.counters_ok = perf.available();

    res.time_ms      = get_median([](const auto& s) { return s.time_ms; });
    res.l1_misses    = get_median([](const auto& s) { return s.l1; });
    res.llc_misses   = get_median([](const auto& s) { return s.llc; });
    res.branch_mispr = get_median([](const auto& s) { return s.br; });

    // Compute IPC per-iteration first, then take the median
    res.ipc = get_median([](const auto& s) {
        return (s.cyc > 0) ? static_cast<double>(s.ins) / static_cast<double>(s.cyc) : 0.0;
    });

    return res;
}
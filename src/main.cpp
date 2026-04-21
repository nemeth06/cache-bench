#include "counters.hpp"
#include "harness.hpp"
#include "plugin.hpp"
#include "print.hpp"

#include "benchmarks/vector_vs_list.hpp"
#include "benchmarks/matrix.hpp"
#include "benchmarks/aos_vs_soa.hpp"
#include "benchmarks/segtree.hpp"
#include "benchmarks/predecessor.hpp"

#include <iostream>
#include <vector>


int main()
{
    // Init counters
    PerfCounters perf;
    if (!perf.available()) {
        std::cerr
            << "[cache-bench] perf_event_open unavailable — showing time only.\n"
            << "              Run with sudo, or set:\n"
            << "              /proc/sys/kernel/perf_event_paranoid to 1\n\n";
    }

    // Register benchmarks
    BenchmarkRunner runner;

    runner.add_plugin<VectorVsList>();
    runner.add_plugin<AosVsSoa>();
    runner.add_plugin<Segtree>();
    runner.add_plugin<MatrixTraversal>();

    runner.add_plugin<PredecessorBench<500>>(1, 5);
    runner.add_plugin<PredecessorBench<2000>>(1, 5);
    runner.add_plugin<PredecessorBench<10000>>(1, 5);
    runner.add_plugin<PredecessorBench<50000>>(1, 5);

    // Run all pairs and collect results
    std::vector<BenchmarkResult> results;
    results.reserve(runner.pairs().size() * 2);

    for (const auto& p : runner.pairs()) {
        results.push_back(
            run_benchmark(p.name_a, p.group_name, p.fn_a, perf,
                          p.warmup, p.iterations));
        results.push_back(
            run_benchmark(p.name_b, p.group_name, p.fn_b, perf,
                          p.warmup, p.iterations));
    }

    // Print results
    print_table(results);
    
}

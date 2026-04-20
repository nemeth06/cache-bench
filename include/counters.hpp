#pragma once
#include <cstdint>

// Raw hardware counter values for a single measurement interval.
struct Counts {
    std::int64_t l1_misses    = 0;  // L1 data cache misses
    std::int64_t llc_misses   = 0;  //  Last-level cache misses
    std::int64_t branch_mispr = 0;  // Branch mispredictions
    std::int64_t cycles       = 0;  // CPU cycles elapsed
    std::int64_t instructions = 0;  // Instructions retired
};

// RAII wrapper around Linux perf_event_open(2).

class PerfCounters {
public:
    PerfCounters();   // Opens file descriptors for all tracked events.
    ~PerfCounters();  // Closes all file descriptors.

    // Non-copyable, non-movable (owns raw fds).
    PerfCounters(const PerfCounters&)            = delete;
    PerfCounters& operator=(const PerfCounters&) = delete;

    // Reset and enable all counters.  Call immediately before the code under test.
    void start();

    // Disable all counters, freezing their values.  Call immediately after.
    void stop();

    // Read the accumulated counts.  Call after stop().
    Counts read() const;

private:
    int fd_l1_misses_    = -1;
    int fd_llc_misses_   = -1;
    int fd_branch_mispr_ = -1;
    int fd_cycles_       = -1;
    int fd_instructions_ = -1;
};

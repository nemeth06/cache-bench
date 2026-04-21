#pragma once

#include <cstddef>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

// Prevent the compiler from optimising away a computed value.
// Use this to ensure benchmark work is performed at -O2.
template <typename T>
inline void do_not_optimize(T const& val) {
    __asm__ volatile("" : : "r,m"(val) : "memory");
}


// Sweep ~32 MiB through memory to evict all CPU cache levels before a
// cold-cache benchmark run.  The buffer is static so it is allocated once.
inline void flush_cache()
{
    constexpr std::size_t SIZE = 32ULL * 1024 * 1024;
    static std::vector<char> buf(SIZE, 1);
    volatile char sink = 0;
    for (std::size_t i = 0; i < SIZE; i += 64)
        sink = buf[i];
    (void)sink;
}


// Format a non-negative integer with thousands separators.
// Returns "--" for negative values (used when counters are unavailable).
inline std::string fmt_int(long long n) {
    if (n < 0) return "--";
    
    std::string s = std::to_string(n);
    for (int i = s.length() - 3; i > 0; i -= 3) {
        s.insert(i, ",");
    }
    return s;
}

// Format a floating-point value to a fixed number of decimal places.
inline std::string fmt_fp(double v, int decimals = 1) {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(decimals) << v;
    return ss.str();
}
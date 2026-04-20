#pragma once

#include <concepts>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

// Plugin concept
//
// A type satisfies BenchmarkPlugin when it provides five static members:
//   - group_name() -> std::string   label for the group separator row
//   - name_a()     -> std::string   label for the first implementation's row
//   - name_b()     -> std::string   label for the second implementation's row
//   - run_a()                      the first implementation (no args, no return)
//   - run_b()                      the second implementation

template <typename T>
concept BenchmarkPlugin = requires {
    { T::group_name() } -> std::convertible_to<std::string>;
    { T::name_a()     } -> std::convertible_to<std::string>;
    { T::name_b()     } -> std::convertible_to<std::string>;
    T::run_a();
    T::run_b();
};

struct BenchmarkPair {
    std::string           group_name;
    std::string           name_a;
    std::string           name_b;
    std::function<void()> fn_a;
    std::function<void()> fn_b;
    int                   warmup     = 3;
    int                   iterations = 20;
};

class BenchmarkRunner {
public:
    // Register a benchmark pair from a type satisfying BenchmarkPlugin.
    template <BenchmarkPlugin Plugin>
    void add_plugin(int warmup = 3, int iterations = 20) {
        pairs_.push_back({
            Plugin::group_name(),
            Plugin::name_a(),
            Plugin::name_b(),
            &Plugin::run_a,
            &Plugin::run_b,
            warmup,
            iterations
        });
    }
    
    const std::vector<BenchmarkPair>& pairs() const { return pairs_; }

private:
    std::vector<BenchmarkPair> pairs_;
};

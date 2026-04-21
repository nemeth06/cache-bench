#pragma once

#include "harness.hpp"
#include "utils.hpp"

#include <iostream>
#include <string>
#include <vector>

namespace col {
    constexpr int name = 30;
    constexpr int time = 12;
    constexpr int l1   = 13;
    constexpr int llc  = 13;
    constexpr int br   = 13;
    constexpr int ipc  = 7;
}

namespace bx {
    constexpr const char* h  = "═";
    constexpr const char* v  = "║";
    constexpr const char* tl = "╔";
    constexpr const char* tr = "╗";
    constexpr const char* bl = "╚";
    constexpr const char* br = "╝";
    constexpr const char* lt = "╠";
    constexpr const char* rt = "╣";
    constexpr const char* tt = "╦";
    constexpr const char* bt = "╩";
    constexpr const char* cr = "╬";
}

inline std::string rept(const char* s, int n) {
    std::string r;
    for (int i = 0; i < n; ++i) r += s;
    return r;
}

inline std::string pad_right(std::string s, size_t w) {
    if (s.size() < w) s.append(w - s.size(), ' ');
    return s.size() > w ? s.substr(0, w) : s;
}

inline std::string pad_left(std::string s, size_t w) {
    if (s.size() < w) s.insert(0, w - s.size(), ' ');
    return s.size() > w ? s.substr(0, w) : s;
}

enum class Sep { Top, Mid, Bot };

inline std::string h_sep(Sep kind = Sep::Mid) {
    const char* l  = (kind == Sep::Top) ? bx::tl : (kind == Sep::Bot) ? bx::bl : bx::lt;
    const char* r  = (kind == Sep::Top) ? bx::tr : (kind == Sep::Bot) ? bx::br : bx::rt;
    const char* cx = (kind == Sep::Top) ? bx::tt : (kind == Sep::Bot) ? bx::bt : bx::cr;

    return std::string(l)
        + rept(bx::h, col::name) + cx
        + rept(bx::h, col::time) + cx
        + rept(bx::h, col::l1)   + cx
        + rept(bx::h, col::llc)  + cx
        + rept(bx::h, col::br)   + cx
        + rept(bx::h, col::ipc)  + r;
}

inline std::string group_sep(const std::string& name) {
    constexpr int inner_w = col::name + col::time + col::l1 + col::llc + col::br + col::ipc + 5;
    std::string label = "  \xe2\x96\xb8 " + name;
    
    size_t display_width = label.size() - 2;
    if (display_width < inner_w) {
        label.append(inner_w - display_width, ' ');
    } else {
        label = label.substr(0, inner_w + 2);
    }
    return std::string(bx::v) + label + bx::v;
}

inline std::string data_row(const BenchmarkResult& r) {
    auto format_cnt = [&](long long v) { return r.counters_ok ? fmt_int(v) : "--"; };
    auto format_ipc = [&]()            { return r.counters_ok ? fmt_fp(r.ipc) : "--"; };

    return std::string(bx::v)
        + " " + pad_right(r.name,                    col::name - 2) + " " + bx::v
        + " " + pad_left(fmt_fp(r.time_ms),          col::time - 2) + " " + bx::v
        + " " + pad_left(format_cnt(r.l1_misses),    col::l1 - 2)   + " " + bx::v
        + " " + pad_left(format_cnt(r.llc_misses),   col::llc - 2)  + " " + bx::v
        + " " + pad_left(format_cnt(r.branch_mispr), col::br - 2)   + " " + bx::v
        + " " + pad_left(format_ipc(),               col::ipc - 2)  + " " + bx::v;
}

inline std::string header_row() {
    return std::string(bx::v)
        + " " + pad_right("Benchmark",    col::name - 2) + " " + bx::v
        + " " + pad_right("Time (ms)",    col::time - 2) + " " + bx::v
        + " " + pad_right("L1 Misses",    col::l1 - 2)   + " " + bx::v
        + " " + pad_right("LLC Misses",   col::llc - 2)  + " " + bx::v
        + " " + pad_right("Branch Mispr", col::br - 2)   + " " + bx::v
        + " " + pad_right("IPC",          col::ipc - 2)  + " " + bx::v;
}

inline void print_table(const std::vector<BenchmarkResult>& results) {
    std::cout << h_sep(Sep::Top) << '\n';
    std::cout << header_row() << '\n';

    std::string current_group;
    for (const auto& r : results) {
        if (r.group != current_group) {
            std::cout << h_sep(Sep::Mid) << '\n';
            std::cout << group_sep(r.group) << '\n';
            std::cout << h_sep(Sep::Mid) << '\n';
            current_group = r.group;
        }
        std::cout << data_row(r) << '\n';
    }

    std::cout << h_sep(Sep::Bot) << '\n';
}
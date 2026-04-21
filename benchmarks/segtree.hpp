#pragma once

#include "plugin.hpp"
#include "utils.hpp"

#include <algorithm>
#include <memory>
#include <numeric>
#include <random>
#include <string>
#include <vector>

namespace segtree_data {

constexpr size_t N         = 1'000'000;
constexpr size_t N_QUERIES = 100'000;

struct Query { 
    size_t l, r; 
};

inline const std::vector<int>& input() {
    static const std::vector<int> v = []() {
        std::vector<int> tmp(N);
        std::iota(tmp.begin(), tmp.end(), 1);
        return tmp;
    }();
    return v;
}

inline const std::vector<Query>& queries() {
    static const std::vector<Query> q = []() {
        std::mt19937 rng(42);
        std::uniform_int_distribution<size_t> dist(0, N - 1);
        
        std::vector<Query> tmp(N_QUERIES);
        for (auto& qry : tmp) {
            size_t a = dist(rng);
            size_t b = dist(rng);
            if (a > b) std::swap(a, b);
            qry = {a, b};
        }
        return tmp;
    }();
    return q;
}

struct Node {
    long long val = 0;
    std::unique_ptr<Node> left;
    std::unique_ptr<Node> right;
};

inline Node* build_rec(const std::vector<int>& d, size_t lo, size_t hi) {
    auto* n = new Node{};
    if (lo == hi) { 
        n->val = d[lo]; 
        return n; 
    }
    
    size_t mid = lo + (hi - lo) / 2;
    n->left.reset(build_rec(d, lo, mid));
    n->right.reset(build_rec(d, mid + 1, hi));
    n->val = n->left->val + n->right->val;
    
    return n;
}

inline long long query_rec(const Node* n, size_t lo, size_t hi, size_t l, size_t r) {
    if (r < lo || hi < l) return 0;
    if (l <= lo && hi <= r) return n->val;
    
    size_t mid = lo + (hi - lo) / 2;
    return query_rec(n->left.get(), lo, mid, l, r) +
           query_rec(n->right.get(), mid + 1, hi, l, r);
}

inline const std::unique_ptr<Node>& rec_tree() {
    static std::unique_ptr<Node> t(build_rec(input(), 0, N - 1));
    return t;
}

inline const std::vector<long long>& flat_tree() {
    static const std::vector<long long> t = []() {
        std::vector<long long> tmp(2 * N, 0);
        const auto& d = input();
        
        for (size_t i = 0; i < N; ++i) {
            tmp[N + i] = d[i];
        }
        for (size_t i = N - 1; i > 0; --i) {
            tmp[i] = tmp[2 * i] + tmp[2 * i + 1];
        }
        
        return tmp;
    }();
    return t;
}

inline long long query_flat(const std::vector<long long>& t, size_t l, size_t r) {
    long long res = 0;
    for (l += N, r += N + 1; l < r; l >>= 1, r >>= 1) {
        if (l & 1) res += t[l++];
        if (r & 1) res += t[--r];
    }
    return res;
}

}

struct Segtree {
    static std::string group_name() { return "segtree (N=1,000,000, Q=100,000 range-sum queries)"; }
    static std::string name_a()     { return "recursive (heap nodes)"; }
    static std::string name_b()     { return "iterative (flat array)"; }

    static void run_a() {
        using namespace segtree_data;
        long long total = 0;
        
        for (const auto& q : queries()) {
            total += query_rec(rec_tree().get(), 0, N - 1, q.l, q.r);
        }
        
        do_not_optimize(total);
    }

    static void run_b() {
        using namespace segtree_data;
        long long total = 0;
        
        for (const auto& q : queries()) {
            total += query_flat(flat_tree(), q.l, q.r);
        }
        
        do_not_optimize(total);
    }
};
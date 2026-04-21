#pragma once

#include "plugin.hpp"
#include "utils.hpp"

#include <string>
#include <vector>

// struct of arrays vs array of structs

namespace aos_soa_data {

constexpr size_t N = 1'000'000;

struct Particle { 
    float x, y, z, w; 
};

struct ParticlesSoA { 
    std::vector<float> x, y, z, w; 
};

inline const std::vector<Particle>& aos() {
    static const std::vector<Particle> v = []() {
        std::vector<Particle> tmp(N);
        for (size_t i = 0; i < N; ++i) {
            float f = static_cast<float>(i);
            tmp[i] = { f, f * 2.0f, f * 3.0f, f * 4.0f };
        }
        return tmp;
    }();
    return v;
}

inline const ParticlesSoA& soa() {
    static const ParticlesSoA s = []() {
        ParticlesSoA tmp;
        tmp.x.resize(N); 
        tmp.y.resize(N); 
        tmp.z.resize(N); 
        tmp.w.resize(N);
        
        for (size_t i = 0; i < N; ++i) {
            float f = static_cast<float>(i);
            tmp.x[i] = f;
            tmp.y[i] = f * 2.0f;
            tmp.z[i] = f * 3.0f;
            tmp.w[i] = f * 4.0f;
        }
        return tmp;
    }();
    return s;
}

}

struct AosVsSoa {
    static std::string group_name() { return "aos_vs_soa (N=1,000,000, sum x field only)"; }
    static std::string name_a()     { return "AoS (struct{x,y,z,w}[])"; }
    static std::string name_b()     { return "SoA (x[], y[], z[], w[])"; }

    static void run_a() {
        double sum = 0;
        for (const auto& p : aos_soa_data::aos()) {
            sum += p.x;
        }
        do_not_optimize(sum);
    }

    static void run_b() {
        double sum = 0;
        for (float v : aos_soa_data::soa().x) {
            sum += v;
        }
        do_not_optimize(sum);
    }
};
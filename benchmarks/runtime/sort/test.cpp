#include <toggle/toggle.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

enum class Distribution { uniform, exponential, zipfian, bits_exp };

struct Case {
    Distribution distribution;
    const char* name;
    uint64_t num_keys = 0;
    double parameter = 0.0;
};

uint64_t splitmix64(uint64_t x) {
    uint64_t z = x + 0x9E3779B97F4A7C15ULL;
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

double unit_random(uint64_t x) {
    return (static_cast<double>(x) + 0.5) * 0x1p-64;
}

uint64_t make_key(const Case& c, uint64_t i, uint64_t n) {
    switch (c.distribution) {
        case Distribution::uniform: {
            const uint64_t h = splitmix64(i * 0x9E3779B97F4A7C15ULL);
            if (c.num_keys == 0) return h; // full uint64_t domain
            return splitmix64(h % std::min(c.num_keys, n));
        }
        case Distribution::exponential: {
            const double u = unit_random(splitmix64(i * 0x94D049BB133111EBULL));
            const double x = -std::log1p(-std::min(u, 1.0 - 1e-16)) / c.parameter;
            return x >= static_cast<double>(n - 1) ? n - 1 : static_cast<uint64_t>(x);
        }
        case Distribution::zipfian: {
            const double u = unit_random(splitmix64(i * 0x243F6A8885A308D3ULL));
            const double k = static_cast<double>(n);
            double x;
            if (c.parameter == 1.0) {
                x = std::exp(u * std::log(k)) - 1.0;
            } else {
                const double a = 1.0 - c.parameter;
                x = std::pow(u * (std::pow(k, a) - 1.0) + 1.0, 1.0 / a) - 1.0;
            }
            return x >= k - 1.0 ? n - 1 : static_cast<uint64_t>(std::max(0.0, x));
        }
        case Distribution::bits_exp: {
            uint64_t key = 0;
            const uint64_t rate = static_cast<uint64_t>(c.parameter);
            for (uint64_t bit = 0; bit < 64; ++bit) {
                const uint64_t h = splitmix64(i * 0x9E3779B97F4A7C15ULL +
                                              bit * 0xBF58476D1CE4E5B9ULL);
                if (h % rate != 0) key |= uint64_t{1} << bit;
            }
            return key;
        }
    }
    return 0;
}

int main(int argc, char** argv) {
    const uint64_t n = (argc >= 2) ? std::stoull(argv[1]) : 1000000000;
    const uint32_t num_rounds = (argc >= 3) ? std::stoul(argv[2]) : 5;

    std::cout << "==================================================================\n";
    std::cout << "Threads: " << toggle::num_workers() << "  Rounds: " << num_rounds << '\n';
    const std::vector<Case> cases = {
        {Distribution::uniform, "uniform_num_keys=10000000", 10000000},
        {Distribution::uniform, "uniform_num_keys=1000", 1000},
        {Distribution::uniform, "uniform_num_keys=max"},
        {Distribution::exponential, "exponential_lambda=0.00002", 0, 0.00002},
        {Distribution::exponential, "exponential_lambda=0.00007", 0, 0.00007},
        {Distribution::zipfian, "zipfian_s=0.8", 0, 0.8},
        {Distribution::zipfian, "zipfian_s=1.2", 0, 1.2},
        {Distribution::bits_exp, "bits_exp_rate=30", 0, 30},
        {Distribution::bits_exp, "bits_exp_rate=100", 0, 100},
    };

    for (const Case& c : cases) {
        std::cout << "Case: " << c.name << "  n: " << n << '\n';
        toggle::Timer timer;
        double total = 0.0;
        for (uint32_t round = 0; round <= num_rounds; ++round) {
            toggle::Array<uint64_t> input(n);
            toggle::parallel_for(0, n, [&](uint64_t i) { input[i] = make_key(c, i, n); }, 1024);
            timer.start();
            toggle::sample_sort(input);
            const double elapsed = timer.stop();
            if (round == 0) std::cout << "    Warmup: " << elapsed << '\n';
            else {
                total += elapsed;
                std::cout << "    Round " << round << " time = " << elapsed << " sec\n";
            }
            if (round == num_rounds) { std::cout << "Running time = " << total / num_rounds << " sec\n"; }
        }
    }
    return 0;
}

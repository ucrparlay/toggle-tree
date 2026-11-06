#include <toggle/toggle.h>

int main(int argc, char** argv) {
    const size_t n = (argc >= 2) ? std::atoi(argv[1]) : 1000000000;
    const uint32_t num_rounds = (argc >= 3) ? std::atoi(argv[2]) : 5;

    std::cout << "==================================================================\n";
    std::cout << "Threads: " << toggle::num_workers() << "  Rounds: " << num_rounds << "\n";
    toggle::Timer t; double ttt = 0;
    auto input1 = toggle::random_permutation<uint64_t>(n, 0);
    auto input2 = toggle::random_permutation<uint64_t>(n, 1);

    for (uint32_t round = 0; round <= num_rounds; round++) {
        t.start();
        auto primes = toggle::scan<toggle::mul, uint64_t(1)>(0, n, [&](size_t) { return 1000000009ULL; });
        auto hash1 = toggle::scan<toggle::sum, uint64_t(0)>(0, n, [&](size_t i) { return uint64_t(input1[i]) * primes[i]; });
        auto hash2 = toggle::scan<toggle::sum, uint64_t(0)>(0, n, [&](size_t i) { return uint64_t(input2[i]) * primes[i]; });
        double tt = t.stop();
        if (round == 0) { std::cout << "    Warmup: " << tt << "\n"; }
        else { ttt += tt; std::cout << "    Round " << round << " time = " << tt << " sec\n"; }
        if (round == num_rounds) { std::cout << "Running time = " << ttt/num_rounds << " sec\n"; }
    }
    return 0;
}

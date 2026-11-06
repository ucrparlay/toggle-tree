#include <toggle/toggle.h>

int main(int argc, char** argv) {
    const size_t n = (argc >= 2) ? std::stoull(argv[1]) : 1000000000;
    const uint32_t num_rounds = (argc >= 3) ? std::atoi(argv[2]) : 5;

    std::cout << "==================================================================\n";
    std::cout << "Threads: " << toggle::num_workers() << "  Rounds: " << num_rounds << "\n";
    toggle::Timer t; double ttt = 0;
    auto input = toggle::random_permutation<uint32_t>(n);

    for (uint32_t round = 0; round <= num_rounds; round++) {
        t.start();
        auto result = toggle::pack<uint32_t>(0, n, [&](uint64_t i) { return toggle::hash64(i) % 10 == 0; }, [&](uint64_t i) { return input[i]; });
        double tt = t.stop();
        if (round == 0) { std::cout << "    Warmup: " << tt << "\n"; }
        else { ttt += tt; std::cout << "    Round " << round << " time = " << tt << " sec\n"; }
        if (round == num_rounds) { std::cout << "Running time = " << ttt/num_rounds << " sec\n"; }
    }
    return 0;
}

#include <toggle/toggle.h>

int main(int argc, char** argv) {
    const size_t n = (argc >= 2) ? std::atoi(argv[1]) : 1000000000;
    const uint32_t num_rounds = (argc >= 3) ? std::atoi(argv[2]) : 5;

    std::cout << "==================================================================\n";
    std::cout << "Threads: " << toggle::num_workers() << "  Rounds: " << num_rounds << "\n";
    toggle::Timer t; double ttt = 0;

    for (uint32_t round = 0; round <= num_rounds; round++) {
        t.start();
        auto result = toggle::reduce<toggle::sum, uint64_t(0)>(0, n, [&](uint64_t i) {
            return (toggle::unhash64(toggle::hash64(i)) != i) + (toggle::unhash32(toggle::hash32(uint32_t(i))) != uint32_t(i));
        });
        double tt = t.stop();
        if (result != 0) { std::cout << "    Error: " << result << " hash/unhash mismatches\n"; }
        if (round == 0) { std::cout << "    Warmup: " << tt << "\n"; }
        else { ttt += tt; std::cout << "    Round " << round << " time = " << tt << " sec\n"; }
        if (round == num_rounds) { std::cout << "Running time = " << ttt/num_rounds << " sec\n"; }
    }
    return 0;
}

#include <toggle/toggle.h>
#include <vector>
#include <algorithm>

template <class IS>
void test_index_set(IS& is, uint64_t n) {
    toggle::Timer t;
    auto perm = toggle::random_permutation<uint32_t>(n);
    for (uint32_t batch = 10; batch <= n; batch *= 10) {
        double tt1 = 0, tt2 = 0; uint32_t r;
        for (r = 0; r < std::min<uint32_t>(10000, n / batch); r++) {
            t.start();
            toggle::parallel_for(0, batch, [&](uint64_t i) { is.insert(perm[i + r * batch]); }, 1);
            tt1 += t.stop();
            t.start();
            thread_local uint64_t me = 0;
            auto consume = [&](uint64_t i) { me += i; if (me == UINT64_MAX) std::cout << "A"; };
            is.consume_each(consume);
            tt2 += t.stop();
        }
        std::cout << "    Batch " << batch;
        for (uint64_t i = batch; i<n; i*=10) std::cout << " ";
        std::cout <<  "  time = " << (1000000000 * (tt1 + tt2)) / (r * batch) << " sec/G [";
        std::cout << (1000000000 * tt1) / (r * batch) << " + " << (1000000000 * tt2) / (r * batch) << "]\n";
    }
}

int main(int argc, char** argv) {
    const size_t n = (argc >= 2) ? std::stoull(argv[1]) : 1000000000;

    std::cout << "==================================================================\n";
    std::cout << "ours\n";
    auto is_tot = toggle::IndexSet(n, 0); is_tot.set_granularity(1);
    test_index_set(is_tot, n);

    return 0;
}

#include "kinoko_random.hpp"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <initializer_list>

int main() {
    kinoko::ScriptRandom rng;
    // Published MS CRT sequence for the default seed of 1.
    for (const int expected : {41, 18467, 6334, 26500, 19169}) {
        if (rng.next() != expected) return 1;
    }
    for (const unsigned seed : {0u, 1u, 123456789u, 0xffffffffu}) {
        rng.seed(seed);
#ifdef _MSC_VER
        std::srand(seed);
#endif
        for (int i = 0; i < 10000; ++i) {
            const int value = rng.next();
            if (value < 0 || value > 32767) return 2;
#ifdef _MSC_VER
            if (value != std::rand()) return 3;
#endif
            // Original InitStarD: rand()/32767 * 3 + 3.
            const float speed = value / 32767.0f * 3.0f + 3.0f;
            if (!std::isfinite(speed) || speed < 3 || speed > 6) return 4;
        }
    }
    std::puts("script random: CRT sequence and original star speed bounds passed");
}

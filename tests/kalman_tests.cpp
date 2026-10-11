#include "kalman.hpp"
#include <iostream>
#include <random>

static void require(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}
int main() {
    try {
        ScalarKalman filter;
        require(filter.update(25, .01, 4) == 25, "initialization");
        for (int i = 0; i < 500; ++i)
            require(std::abs(filter.update(25, .01, 4) - 25) < 1e-10, "constant signal");
        filter.reset();
        std::mt19937 engine(12345);
        std::normal_distribution<double> noise(0, 2);
        double rawError = 0, filteredError = 0;
        for (int i = 0; i < 2000; ++i) {
            const double raw = 25 + noise(engine);
            const double estimate = filter.update(raw, .01, 4);
            require(std::isfinite(estimate) && filter.variance() >= 0, "finite covariance");
            if (i > 100) {
                rawError += (raw - 25) * (raw - 25);
                filteredError += (estimate - 25) * (estimate - 25);
            }
        }
        require(filteredError < rawError * .25, "noise reduction");
        filter.reset();
        require(filter.update(10, 0, 1) == 10, "reset");
        bool threw = false;
        try { filter.update(10, 1, 0); } catch (const std::invalid_argument&) { threw = true; }
        require(threw, "reject zero measurement variance");
        std::cout << "PASS: initialization, constant signal, reset, variance, noise reduction, invalid input\n"
                  << "Filtered/raw squared error ratio: " << filteredError / rawError << '\n';
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}

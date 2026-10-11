#pragma once
#include <cmath>
#include <stdexcept>

// Scalar random-walk model for a synthetic temperature sensor, not image data.
class ScalarKalman {
public:
    double update(double measurement, double processVariance, double measurementVariance) {
        if (!std::isfinite(measurement) || !std::isfinite(processVariance) ||
            !std::isfinite(measurementVariance) || processVariance < 0 || measurementVariance <= 0)
            throw std::invalid_argument("Invalid Kalman input");
        if (!initialized_) {
            estimate_ = measurement;
            variance_ = measurementVariance;
            initialized_ = true;
            return estimate_;
        }
        const double prior = variance_ + processVariance;
        const double gain = prior / (prior + measurementVariance);
        estimate_ += gain * (measurement - estimate_);
        // Joseph covariance update improves numerical robustness.
        variance_ = (1 - gain) * (1 - gain) * prior + gain * gain * measurementVariance;
        return estimate_;
    }
    void reset() { initialized_ = false; estimate_ = 0; variance_ = 1; }
    double variance() const { return variance_; }
private:
    bool initialized_ = false;
    double estimate_ = 0, variance_ = 1;
};

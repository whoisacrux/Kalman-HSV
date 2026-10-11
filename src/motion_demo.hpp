#pragma once
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <deque>
#include <random>
#include <string>

// Self-contained synthetic experiment. Its input is generated here; it has
// no camera measurements, detector, device control, or external-input API.
struct MotionSample {
    double time=0, truthX=0, truthY=0, measuredX=0, measuredY=0;
    double filteredX=0, filteredY=0, futureX=0, futureY=0;
    double futureSigmaX=0, futureSigmaY=0;
    bool hasMeasurement=true;
};

class MotionDemo {
public:
    static constexpr double dt=1.0/30.0;
    static constexpr double horizon=0.4;
    static constexpr int totalSteps=600;
    MotionDemo() { reset(); }
    void reset() {
        engine_.seed(42); noise_.reset(); step_=0; missingSteps_=0;
        rawSquared_=0; filteredSquared_=0; measuredCount_=0; history_.clear();
        sample_={}; makeTruth();
        sample_.measuredX=sample_.truthX+noise_(engine_);
        sample_.measuredY=sample_.truthY+noise_(engine_);
        x_={sample_.measuredX,0,144,0,0,400};
        y_={sample_.measuredY,0,144,0,0,400};
        publish();
    }
    void step() {
        if (finished()) return;
        ++step_; sample_.time=step_*dt; makeTruth();
        x_.predict(); y_.predict();
        sample_.hasMeasurement=missingSteps_==0;
        if (sample_.hasMeasurement) {
            sample_.measuredX=sample_.truthX+noise_(engine_);
            sample_.measuredY=sample_.truthY+noise_(engine_);
            x_.correct(sample_.measuredX); y_.correct(sample_.measuredY);
            rawSquared_+=square(sample_.measuredX-sample_.truthX)+square(sample_.measuredY-sample_.truthY);
            filteredSquared_+=square(x_.position-sample_.truthX)+square(y_.position-sample_.truthY);
            ++measuredCount_;
        } else {
            --missingSteps_;
        }
        publish();
    }
    void interruptMeasurements() {
        if (finished()) return;
        missingSteps_=60; // Exactly two simulated seconds at 30 Hz.
        sample_.hasMeasurement=false;
    }
    void nextPath() { path_=(path_+1)%3; reset(); }
    bool finished() const { return step_>=totalSteps; }
    bool measurementGap() const { return missingSteps_>0 || !sample_.hasMeasurement; }
    const MotionSample& sample() const { return sample_; }
    const std::deque<MotionSample>& history() const { return history_; }
    double rawRmse() const { return measuredCount_ ? std::sqrt(rawSquared_/measuredCount_) : 0; }
    double filteredRmse() const { return measuredCount_ ? std::sqrt(filteredSquared_/measuredCount_) : 0; }
    std::size_t measuredCount() const { return measuredCount_; }
    std::string pathName() const {
        return path_==0 ? "Curved" : path_==1 ? "Straight" : "Circle";
    }
private:
    static double square(double value) { return value*value; }
    struct Axis {
        double position=0, velocity=0, a=144, b=0, c=0, d=400;
        void predict() {
            const double q=450, h=MotionDemo::dt;
            position+=h*velocity;
            const double na=a+h*(b+c)+h*h*d+q*h*h*h/3;
            const double nb=b+h*d+q*h*h/2;
            const double nc=c+h*d+q*h*h/2;
            d+=q*h; a=na; b=nb; c=nc;
        }
        void correct(double observation) {
            const double r=144, pa=a, pb=b, pc=c, pd=d;
            const double k0=pa/(pa+r), k1=pc/(pa+r);
            const double innovation=observation-position;
            position+=k0*innovation; velocity+=k1*innovation;
            // Joseph update keeps the covariance positive in finite precision.
            a=(1-k0)*(1-k0)*pa+k0*k0*r;
            b=(1-k0)*(pb-k1*pa)+k0*k1*r;
            c=(1-k0)*(pc-k1*pa)+k0*k1*r;
            d=pd-k1*(pb+pc)+k1*k1*(pa+r);
            b=c=(b+c)/2;
        }
        double futureSigma() const {
            const double h=MotionDemo::horizon;
            return std::sqrt(std::max(0.0,a+h*(b+c)+h*h*d+450*h*h*h/3));
        }
    };
    void makeTruth() {
        const double t=sample_.time;
        if (path_==2) {
            sample_.truthX=500+250*std::cos(.4*t);
            sample_.truthY=250+150*std::sin(.4*t);
        } else {
            sample_.truthX=120+37*t;
            sample_.truthY=path_==1 ? 160+8*t : 250+120*std::sin(.4*t)+24*std::sin(.95*t);
        }
    }
    void publish() {
        sample_.filteredX=x_.position; sample_.filteredY=y_.position;
        sample_.futureX=x_.position+horizon*x_.velocity;
        sample_.futureY=y_.position+horizon*y_.velocity;
        sample_.futureSigmaX=x_.futureSigma(); sample_.futureSigmaY=y_.futureSigma();
        history_.push_back(sample_);
        if (history_.size()>150) history_.pop_front();
    }
    int path_=0, step_=0, missingSteps_=0;
    Axis x_,y_;
    MotionSample sample_;
    std::deque<MotionSample> history_;
    std::mt19937 engine_{42};
    std::normal_distribution<double> noise_{0,12};
    double rawSquared_=0, filteredSquared_=0;
    std::size_t measuredCount_=0;
};

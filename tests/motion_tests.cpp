#include "motion_demo.hpp"
#include <iostream>
#include <stdexcept>

static void require(bool value,const char* message) {
    if (!value) throw std::runtime_error(message);
}
int main() {
    try {
        MotionDemo demo;
        for (int i=0;i<90;++i) demo.step();
        const auto baseline=demo.sample();
        require(demo.filteredRmse()<demo.rawRmse(),"filter should reduce error on default early path");
        demo.interruptMeasurements();
        const auto count=demo.measuredCount();
        for (int i=0;i<60;++i) {
            demo.step();
            require(!demo.sample().hasMeasurement,"measurement gap must last 60 frames");
            require(std::isfinite(demo.sample().futureX),"finite prediction during missing measurements");
        }
        require(demo.measuredCount()==count,"missing frames must not enter paired RMSE");
        require(demo.sample().futureSigmaX>baseline.futureSigmaX,"uncertainty must grow without observations");
        const double missingSigma=demo.sample().futureSigmaX;
        demo.step();
        require(demo.sample().hasMeasurement,"measurements must resume");
        require(demo.sample().futureSigmaX<missingSigma,"observation should reduce uncertainty");
        demo.reset();
        for (int i=0;i<90;++i) demo.step();
        require(std::abs(demo.sample().filteredX-baseline.filteredX)<1e-10,"reset must repeat the experiment");
        for (int path=0;path<3;++path) {
            demo.reset();
            for (int i=0;i<MotionDemo::totalSteps;++i) {
                demo.step(); const auto& s=demo.sample();
                require(std::isfinite(s.futureX)&&std::isfinite(s.futureY),"finite states");
                require(s.futureSigmaX>0&&s.futureSigmaY>0,"positive uncertainty");
                require(demo.history().size()<=150,"bounded history");
            }
            require(demo.finished(),"experiment ends after 600 samples");
            const double end=demo.sample().time; demo.step();
            require(demo.sample().time==end,"finished experiment must not advance");
            std::cout<<demo.pathName()<<": raw RMSE="<<demo.rawRmse()
                     <<", filtered RMSE="<<demo.filteredRmse()<<" simulation units\n";
            demo.nextPath();
        }
        std::cout<<"PASS: dropout, reacquisition, covariance, reset, paths, bounded history, end state\n";
    } catch (const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
}

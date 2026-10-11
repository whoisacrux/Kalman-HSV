#include "kalman.hpp"
#include "motion_demo.hpp"
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <deque>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <vector>

namespace {
using Clock = std::chrono::steady_clock;
const std::string WINDOW = "HSV + Kalman | Learning Lab";
const std::string CONTROLS = "Lab Controls";
const cv::Scalar BG(24, 20, 17), PANEL(39, 33, 28), WHITE(235, 239, 241);
const cv::Scalar MUTED(157, 150, 140), ACCENT(180, 224, 73), BLUE(239, 164, 83);

void label(cv::Mat& img, const std::string& s, int x, int y,
           double scale = .55, cv::Scalar color = WHITE, int thickness = 1) {
    cv::putText(img, s, {x,y}, cv::FONT_HERSHEY_SIMPLEX, scale, color, thickness, cv::LINE_AA);
}
std::string fixed(double value, int decimals = 2) {
    std::ostringstream out; out << std::fixed << std::setprecision(decimals) << value; return out.str();
}
void tile(cv::Mat& canvas, const cv::Mat& image, cv::Rect area) {
    cv::rectangle(canvas, area, cv::Scalar(0,0,0), cv::FILLED);
    if (image.empty()) return;
    const double ratio = std::min(double(area.width)/image.cols, double(area.height)/image.rows);
    cv::Mat scaled;
    cv::resize(image, scaled, {}, ratio, ratio, cv::INTER_AREA);
    cv::Rect dest(area.x + (area.width-scaled.cols)/2, area.y+(area.height-scaled.rows)/2,
                  scaled.cols, scaled.rows);
    scaled.copyTo(canvas(dest));
}
struct Sample { double time, truth, measured, filtered; };

struct App {
    int tab = 0, cameraIndex = 0;
    bool demo = false, paused = false, openingAttempted = false;
    bool cameraFailed = false, mirror = true;
    cv::VideoCapture camera;
    cv::Mat lastFrame;
    int hLow=35, hHigh=85, sLow=70, sHigh=255, vLow=40, vHigh=255, clean=1;
    int noiseTenths=20, qMilli=10, rTenths=40;
    ScalarKalman filter;
    MotionDemo motion;
    std::mt19937 engine{42};
    std::normal_distribution<double> normal{0,1};
    std::deque<Sample> history;
    double simTime=0, rawSse=0, filteredSse=0;
    unsigned long sampleCount=0;
    std::string status="Ready. Use 1 / 2 / 3 to switch views.";

    void resetSimulation() {
        filter.reset(); history.clear(); engine.seed(42); normal.reset();
        simTime=0; rawSse=0; filteredSse=0; sampleCount=0;
        status="Simulation reset.";
    }
    bool openCamera() {
        camera.release(); openingAttempted=true; cameraFailed=false;
#ifdef _WIN32
        bool opened=camera.open(cameraIndex,cv::CAP_DSHOW);
        if (!opened) opened=camera.open(cameraIndex,cv::CAP_MSMF);
#else
        bool opened=camera.open(cameraIndex,cv::CAP_ANY);
#endif
        if (!opened) { cameraFailed=true; status="Camera unavailable. N: next index | O: retry | D: demo"; return false; }
        camera.set(cv::CAP_PROP_FRAME_WIDTH,640);
        camera.set(cv::CAP_PROP_FRAME_HEIGHT,480);
        status="Camera "+std::to_string(cameraIndex)+" opened.";
        return true;
    }
    cv::Mat colorDemo() const {
        cv::Mat frame(480,640,CV_8UC3,cv::Scalar(42,42,42));
        const std::vector<cv::Scalar> colors={{40,190,70},{220,90,35},{35,45,220},{40,210,220},{210,50,190}};
        for (int i=0;i<5;++i) {
            cv::rectangle(frame,{25+i*122,90,100,250},colors[static_cast<size_t>(i)],cv::FILLED);
            label(frame,std::to_string(i+1),60+i*122,380,.8);
        }
        label(frame,"SYNTHETIC COLOR CHART",24,40,.7);
        return frame;
    }
    void drawHSV(cv::Mat& canvas) {
        if (!demo && !openingAttempted) openCamera();
        if (demo) lastFrame=colorDemo();
        else if (camera.isOpened()) {
            cv::Mat frame;
            if (camera.read(frame) && !frame.empty()) {
                if (frame.channels()==4) cv::cvtColor(frame,frame,cv::COLOR_BGRA2BGR);
                else if (frame.channels()==1) cv::cvtColor(frame,frame,cv::COLOR_GRAY2BGR);
                if (mirror) cv::flip(frame,frame,1);
                lastFrame=frame;
            } else {
                camera.release(); lastFrame.release(); cameraFailed=true;
                status="Camera disconnected or empty frame. O: retry | N: next camera";
            }
        }
        label(canvas,"HSV COLOR EXPLORER",28,150,.8,ACCENT,2);
        label(canvas,"Change HSV limits in Lab Controls. White pixels pass the selected color range.",28,180,.52,MUTED);
        label(canvas,demo ? "SOURCE / DEMO CHART" : "SOURCE / USB CAMERA "+std::to_string(cameraIndex),28,222,.55);
        label(canvas,"BINARY COLOR MASK",612,222,.55);
        if (cameraFailed && !demo) {
            cv::rectangle(canvas,{28,240,1124,370},PANEL,cv::FILLED);
            label(canvas,"Camera could not be read",60,320,.85,WHITE,2);
            label(canvas,"Close Teams / Zoom. Check Windows camera permissions.",60,365,.62,MUTED);
            label(canvas,"N: try next camera (0..5)     O: retry     D: demo chart",60,410,.62,ACCENT);
            return;
        }
        if (lastFrame.empty()) return;
        cv::Mat hsv,mask;
        cv::cvtColor(lastFrame,hsv,cv::COLOR_BGR2HSV);
        const int loS=std::min(sLow,sHigh), hiS=std::max(sLow,sHigh);
        const int loV=std::min(vLow,vHigh), hiV=std::max(vLow,vHigh);
        if (hLow<=hHigh) cv::inRange(hsv,cv::Scalar(hLow,loS,loV),cv::Scalar(hHigh,hiS,hiV),mask);
        else {
            cv::Mat second;
            cv::inRange(hsv,cv::Scalar(hLow,loS,loV),cv::Scalar(179,hiS,hiV),mask);
            cv::inRange(hsv,cv::Scalar(0,loS,loV),cv::Scalar(hHigh,hiS,hiV),second);
            cv::bitwise_or(mask,second,mask);
        }
        if (clean>0) {
            const auto kernel=cv::getStructuringElement(cv::MORPH_ELLIPSE,{2*clean+1,2*clean+1});
            cv::morphologyEx(mask,mask,cv::MORPH_OPEN,kernel);
            cv::morphologyEx(mask,mask,cv::MORPH_CLOSE,kernel);
        }
        cv::Mat coloredMask; cv::cvtColor(mask,coloredMask,cv::COLOR_GRAY2BGR);
        tile(canvas,lastFrame,{28,240,540,370});
        tile(canvas,coloredMask,{612,240,540,370});
        const double coverage=100.*cv::countNonZero(mask)/double(mask.total());
        label(canvas,"Selected pixels: "+fixed(coverage,1)+"%",28,648,.67,ACCENT);
        label(canvas,"H: 0..179   S,V: 0..255 | H low > high wraps around hue",28,682,.52,MUTED);
        label(canvas,"N next camera   O reconnect   D demo/camera   M mirror",28,712,.52,MUTED);
    }
    void stepSimulation() {
        simTime+=.1; // One sample represents 0.1 seconds, independent of webcam timing.
        const double truth=25+2*std::sin(simTime*.23)+(std::fmod(simTime,40.)>=20?3:0);
        const double measured=truth+normal(engine)*noiseTenths/10.;
        const double filtered=filter.update(measured,std::max(qMilli,1)/1000.,std::max(rTenths,1)/10.);
        history.push_back({simTime,truth,measured,filtered});
        if (history.size()>300) history.pop_front();
        rawSse+=(measured-truth)*(measured-truth);
        filteredSse+=(filtered-truth)*(filtered-truth); ++sampleCount;
    }
    void drawSimulation(cv::Mat& canvas) const {
        label(canvas,"KALMAN / SYNTHETIC TEMPERATURE SENSOR",28,150,.75,ACCENT,2);
        label(canvas,"A standalone 1D experiment. Camera pixels are not used in this simulation.",28,182,.52,MUTED);
        const cv::Rect plot(80,235,1050,360);
        cv::rectangle(canvas,plot,PANEL,cv::FILLED);
        for (int temp=5;temp<=45;temp+=5) {
            int y=plot.y+plot.height-(temp-5)*plot.height/40;
            cv::line(canvas,{plot.x,y},{plot.x+plot.width,y},cv::Scalar(60,51,44));
            label(canvas,std::to_string(temp),38,y+5,.48,MUTED);
        }
        auto yOf=[&](double value){return plot.y+plot.height-int((std::clamp(value,5.,45.)-5)*plot.height/40.);};
        for (size_t i=1;i<history.size();++i) {
            int x0=plot.x+int(i-1)*plot.width/299, x1=plot.x+int(i)*plot.width/299;
            cv::line(canvas,{x0,yOf(history[i-1].measured)},{x1,yOf(history[i].measured)},cv::Scalar(90,85,140),1,cv::LINE_AA);
            cv::line(canvas,{x0,yOf(history[i-1].truth)},{x1,yOf(history[i].truth)},BLUE,2,cv::LINE_AA);
            cv::line(canvas,{x0,yOf(history[i-1].filtered)},{x1,yOf(history[i].filtered)},ACCENT,2,cv::LINE_AA);
        }
        label(canvas,"Truth",80,215,.5,BLUE);
        label(canvas,"Noisy measurement",220,215,.5,cv::Scalar(160,145,220));
        label(canvas,"Kalman estimate",480,215,.5,ACCENT);
        label(canvas,"Last 300 samples / 30 simulated seconds | Y: temperature (C), clipped to 5..45",80,624,.48,MUTED);
        if (sampleCount) {
            label(canvas,"Raw RMSE: "+fixed(std::sqrt(rawSse/sampleCount))+" C",28,666,.6);
            label(canvas,"Filtered RMSE: "+fixed(std::sqrt(filteredSse/sampleCount))+" C",390,666,.6,ACCENT);
            label(canvas,"t = "+fixed(simTime,1)+" s",870,666,.6);
        }
        label(canvas,"SPACE pause   R reset   E export visible samples to simulation.csv",28,705,.53,MUTED);
    }
    void exportSimulation() {
        std::ofstream file("simulation.csv");
        if (!file) { status="Could not write simulation.csv (check folder permissions)."; return; }
        file << "time_seconds,truth_c,measurement_c,kalman_c\n" << std::setprecision(9);
        for (const auto& s:history) file<<s.time<<','<<s.truth<<','<<s.measured<<','<<s.filtered<<'\n';
        file.close(); status=file ? "Saved visible simulation samples to simulation.csv." : "CSV write failed.";
    }
    void drawMotion(cv::Mat& canvas) const {
        const auto& s=motion.sample();
        const cv::Scalar observed(125,163,245), forecast(250,179,194);
        label(canvas,"2D KALMAN / SYNTHETIC MOTION",28,150,.75,ACCENT,2);
        label(canvas,"Independent simulation | No camera input | Noise sigma: 12 units | Forecast: +0.4 s",28,181,.51,MUTED);
        cv::circle(canvas,{37,213},6,WHITE,cv::FILLED,cv::LINE_AA);
        label(canvas,"True object",52,218,.48);
        cv::rectangle(canvas,{218,205,15,15},observed,1);
        label(canvas,"Measurement",241,218,.48,observed);
        cv::rectangle(canvas,{439,205,15,15},ACCENT,2);
        label(canvas,"Kalman + trail",462,218,.48,ACCENT);
        cv::drawMarker(canvas,{691,213},forecast,cv::MARKER_TILTED_CROSS,12,2);
        label(canvas,"Next position",707,218,.48,forecast);
        label(canvas,motion.measurementGap()?"LOST / PREDICT ONLY":"TRACKING / UPDATE",944,218,.40,
              motion.measurementGap()?observed:ACCENT);
        const cv::Rect plot(28,237,1124,340);
        cv::rectangle(canvas,plot,PANEL,cv::FILLED);
        cv::Mat scene=canvas(plot);
        auto point=[&](double x,double y) {
            return cv::Point(static_cast<int>(std::lround(x*plot.width/1000.0)),
                             static_cast<int>(std::lround(y*plot.height/500.0)));
        };
        for (int x=100;x<1000;x+=100)
            cv::line(scene,point(x,0),point(x,500),cv::Scalar(55,47,40),1);
        for (int y=100;y<500;y+=100)
            cv::line(scene,point(0,y),point(1000,y),cv::Scalar(55,47,40),1);
        const auto& trail=motion.history();
        for (std::size_t i=1;i<trail.size();++i) {
            if (!trail[i].hasMeasurement && i%2==0) continue;
            cv::line(scene,point(trail[i-1].filteredX,trail[i-1].filteredY),
                     point(trail[i].filteredX,trail[i].filteredY),ACCENT,2,cv::LINE_AA);
        }
        const auto estimated=point(s.filteredX,s.filteredY), predicted=point(s.futureX,s.futureY);
        const int rx=std::clamp(static_cast<int>(std::lround(2*s.futureSigmaX*plot.width/1000.0)),1,2*plot.width);
        const int ry=std::clamp(static_cast<int>(std::lround(2*s.futureSigmaY*plot.height/500.0)),1,2*plot.height);
        cv::ellipse(scene,predicted,{rx,ry},0,0,360,forecast,1,cv::LINE_AA);
        for (int k=0;k<10;k+=2) {
            const cv::Point a=estimated+(predicted-estimated)*(k/10.0);
            const cv::Point b=estimated+(predicted-estimated)*((k+1)/10.0);
            cv::line(scene,a,b,forecast,1,cv::LINE_AA);
        }
        cv::circle(scene,point(s.truthX,s.truthY),10,WHITE,cv::FILLED,cv::LINE_AA);
        if (s.hasMeasurement) {
            const auto m=point(s.measuredX,s.measuredY);
            cv::rectangle(scene,{m.x-15,m.y-15,30,30},observed,1,cv::LINE_AA);
        }
        cv::rectangle(scene,{estimated.x-18,estimated.y-18,36,36},ACCENT,2,cv::LINE_AA);
        cv::drawMarker(scene,predicted,forecast,cv::MARKER_TILTED_CROSS,16,2,cv::LINE_AA);
        label(scene,motion.pathName()+" path",14,24,.49,MUTED);
        if (motion.finished()) label(scene,"20 s COMPLETE - R to replay",360,170,.6,WHITE,2);
        label(canvas,"RMSE on observed frames: raw "+fixed(motion.rawRmse())+
              " / filtered "+fixed(motion.filteredRmse())+" units",28,610,.51);
        label(canvas,"t = "+fixed(s.time,1)+" / 20.0 s",919,610,.51);
        const double error=std::hypot(s.filteredX-s.truthX,s.filteredY-s.truthY);
        label(canvas,"Current error: "+fixed(error)+" units | Ellipse: +/-2 sigma along X and Y",28,643,.49,MUTED);
        auto button=[&](cv::Rect bounds,const std::string& text) {
            cv::rectangle(canvas,bounds,PANEL,cv::FILLED);
            label(canvas,text,bounds.x+12,bounds.y+24,.48);
        };
        button({28,675,184,40},paused?"SPACE Resume":"SPACE Pause");
        button({224,675,164,40},"R Restart");
        button({404,675,248,40},"L Lose data (2 s)");
        button({668,675,222,40},"P Change path");
    }
    static void mouse(int event,int x,int y,int,void* context) {
        if (event!=cv::EVENT_LBUTTONDOWN) return;
        auto& a=*static_cast<App*>(context);
        if (y>=76 && y<=116) {
            if (x>=28 && x<280) a.tab=0;
            if (x>=296 && x<600) a.tab=1;
            if (x>=616 && x<946) a.tab=2;
        }
        if (a.tab==2 && y>=675 && y<=715) {
            if (x>=28 && x<212) a.paused=!a.paused;
            if (x>=224 && x<388) { a.motion.reset(); a.paused=false; }
            if (x>=404 && x<652) { a.motion.interruptMeasurements(); a.paused=false; }
            if (x>=668 && x<890) { a.motion.nextPath(); a.paused=false; }
        }
    }
    void controls() {
        cv::namedWindow(CONTROLS,cv::WINDOW_NORMAL);
        cv::resizeWindow(CONTROLS,440,570);
        auto slider=[](const char* name,int value,int maximum) {
            cv::createTrackbar(name,CONTROLS,nullptr,maximum);
            cv::setTrackbarPos(name,CONTROLS,value);
        };
        slider("H low",hLow,179); slider("H high",hHigh,179);
        slider("S low",sLow,255); slider("S high",sHigh,255);
        slider("V low",vLow,255); slider("V high",vHigh,255);
        slider("Clean radius",clean,5);
        slider("Noise x0.1",noiseTenths,50);
        slider("Q x0.001 (min 1)",qMilli,500);
        slider("R x0.1 (min 1)",rTenths,250);
    }
    void readControls() {
        auto get=[](const char* name){return cv::getTrackbarPos(name,CONTROLS);};
        hLow=get("H low"); hHigh=get("H high"); sLow=get("S low"); sHigh=get("S high");
        vLow=get("V low"); vHigh=get("V high"); clean=get("Clean radius");
        int n=get("Noise x0.1"),q=get("Q x0.001 (min 1)"),r=get("R x0.1 (min 1)");
        if (n!=noiseTenths || q!=qMilli || r!=rTenths) {
            noiseTenths=n; qMilli=q; rTenths=r; resetSimulation();
        }
    }
    void run() {
        cv::namedWindow(WINDOW,cv::WINDOW_AUTOSIZE); controls();
        cv::setMouseCallback(WINDOW,mouse,this);
        auto lastStep=Clock::now(), lastMotion=Clock::now();
        for (;;) {
            readControls();
            const auto now=Clock::now();
            if (tab==1 && !paused && std::chrono::duration<double>(now-lastStep).count()>=.1) {
                stepSimulation(); lastStep=now;
            }
            if (tab!=1 || paused) lastStep=now;
            if (tab==2 && !paused && std::chrono::duration<double>(now-lastMotion).count()>=MotionDemo::dt) {
                motion.step(); lastMotion=now;
            }
            if (tab!=2 || paused) lastMotion=now;
            cv::Mat canvas(790,1180,CV_8UC3,BG);
            label(canvas,"VISION + SIGNAL LAB",28,46,.95,WHITE,2);
            label(canvas,"C++ / OpenCV",930,45,.55,MUTED);
            cv::rectangle(canvas,{28,76,252,40},tab==0?ACCENT:PANEL,cv::FILLED);
            cv::rectangle(canvas,{296,76,304,40},tab==1?ACCENT:PANEL,cv::FILLED);
            cv::rectangle(canvas,{616,76,330,40},tab==2?ACCENT:PANEL,cv::FILLED);
            label(canvas,"1   HSV / WEBCAM",42,103,.55,tab==0?BG:WHITE);
            label(canvas,"2   KALMAN SIMULATION",310,103,.55,tab==1?BG:WHITE);
            label(canvas,"3   2D MOTION SIMULATION",630,103,.53,tab==2?BG:WHITE);
            if (tab==0) drawHSV(canvas);
            else if (tab==1) drawSimulation(canvas);
            else drawMotion(canvas);
            cv::rectangle(canvas,{0,740,1180,50},PANEL,cv::FILLED);
            label(canvas,status,28,772,.46,MUTED);
            label(canvas,paused?"PAUSED | ESC exit":"ESC exit",965,772,.46,MUTED);
            cv::imshow(WINDOW,canvas);
            const int key=cv::waitKey(16)&255;
            if (key==27 || key=='q') break;
            if (key=='1') tab=0;
            if (key=='2') tab=1;
            if (key=='3') tab=2;
            if (key==' ' && tab!=0) paused=!paused;
            if (key=='r' && tab==1) resetSimulation();
            if (key=='r' && tab==2) { motion.reset(); paused=false; }
            if (key=='l' && tab==2) { motion.interruptMeasurements(); paused=false; }
            if (key=='p' && tab==2) { motion.nextPath(); paused=false; }
            if (key=='e' && tab==1) exportSimulation();
            if (key=='m' && tab==0) mirror=!mirror;
            if (key=='d' && tab==0) {
                demo=!demo; camera.release(); lastFrame.release(); openingAttempted=false;
                status=demo?"Demo chart enabled.":"Opening camera...";
            }
            if ((key=='n' || key=='o') && tab==0) {
                demo=false; lastFrame.release();
                if (key=='n') cameraIndex=(cameraIndex+1)%6;
                openCamera();
            }
            if (cv::getWindowProperty(WINDOW,cv::WND_PROP_VISIBLE)<1 ||
                cv::getWindowProperty(CONTROLS,cv::WND_PROP_VISIBLE)<1) break;
        }
        camera.release(); cv::destroyAllWindows();
    }
};
}

int main(int argc,char** argv) {
    try {
        App app;
        for (int i=1;i<argc;++i) {
            const std::string arg=argv[i];
            if (arg=="--demo") app.demo=true;
            else if (arg=="--simulation") { app.tab=1; app.demo=true; }
            else if (arg=="--motion") { app.tab=2; app.demo=true; }
            else if (arg=="--camera" && i+1<argc) {
                std::string value=argv[++i]; size_t consumed=0;
                app.cameraIndex=std::stoi(value,&consumed);
                if (consumed!=value.size() || app.cameraIndex<0 || app.cameraIndex>99)
                    throw std::invalid_argument("Camera index must be 0..99");
            } else if (arg=="--help") {
                std::cout<<"hsv_kalman_lab [--camera 0] [--demo] [--simulation] [--motion]\n"; return 0;
            } else throw std::invalid_argument("Unknown/incomplete argument: "+arg);
        }
        app.run();
    } catch (const std::exception& e) { std::cerr<<"Error: "<<e.what()<<'\n'; return 1; }
}

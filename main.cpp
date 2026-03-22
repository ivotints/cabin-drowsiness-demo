#include "drowsiness_monitor.h"
#include "alert_sound.h"
#include "cascade_loader.h"

#include <opencv2/opencv.hpp>

#include <algorithm>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

struct Options {
    int cameraIndex = 0;
    std::string faceCascadePath;
    std::string eyeCascadePath;
};

static long long toMs(const std::chrono::steady_clock::duration& d) {
    return std::chrono::duration_cast<std::chrono::milliseconds>(d).count();
}

static Options parseArgs(int argc, char** argv) {
    Options opt;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        auto takeValue = [&](std::string& target) {
            if (i + 1 >= argc) {
                throw std::runtime_error("Missing value after " + arg);
            }
            target = argv[++i];
        };

        if (arg == "--camera") {
            if (i + 1 >= argc) throw std::runtime_error("Missing value after --camera");
            opt.cameraIndex = std::stoi(argv[++i]);
        } else if (arg == "--face-cascade") {
            takeValue(opt.faceCascadePath);
        } else if (arg == "--eye-cascade") {
            takeValue(opt.eyeCascadePath);
        } else if (arg == "--help" || arg == "-h") {
            std::cout <<
                "Usage:\n"
                "  ./cabin_drowsiness_demo [--camera N] [--face-cascade PATH] [--eye-cascade PATH]\n\n"
                "Example:\n"
                "  ./cabin_drowsiness_demo --camera 0\n";
            std::exit(0);
        } else {
            throw std::runtime_error("Unknown argument: " + arg);
        }
    }

    return opt;
}

static void drawLabel(cv::Mat& frame,
                      const std::string& text,
                      const cv::Point& origin,
                      const cv::Scalar& color) {
    int baseline = 0;
    const double fontScale = 0.6;
    const int thickness = 1;
    const int fontFace = cv::FONT_HERSHEY_SIMPLEX;

    auto size = cv::getTextSize(text, fontFace, fontScale, thickness, &baseline);
    cv::rectangle(frame,
                  cv::Rect(origin.x, origin.y - size.height - 10, size.width + 12, size.height + 12),
                  cv::Scalar(0, 0, 0),
                  cv::FILLED);
    cv::putText(frame, text, origin, fontFace, fontScale, color, thickness, cv::LINE_AA);
}

int main(int argc, char** argv) {
    try {
        const Options opt = parseArgs(argc, argv);

        const std::string faceCascadePath =
            opt.faceCascadePath.empty() ? defaultFaceCascade() : opt.faceCascadePath;
        const std::string eyeCascadePath =
            opt.eyeCascadePath.empty() ? defaultEyeCascade() : opt.eyeCascadePath;
        const std::string alertSoundPath = defaultAlertSoundPath();

        cv::CascadeClassifier faceCascade;
        cv::CascadeClassifier eyeCascade;

        if (!faceCascade.load(faceCascadePath)) {
            throw std::runtime_error("Failed to load face cascade: " + faceCascadePath);
        }
        if (!eyeCascade.load(eyeCascadePath)) {
            throw std::runtime_error("Failed to load eye cascade: " + eyeCascadePath);
        }

        cv::VideoCapture cap(opt.cameraIndex);
        if (!cap.isOpened()) {
            throw std::runtime_error("Cannot open camera index " + std::to_string(opt.cameraIndex));
        }

        // Use 640x480 for more consistent webcam capture quality and performance.
        cap.set(cv::CAP_PROP_FRAME_WIDTH, 640);
        cap.set(cv::CAP_PROP_FRAME_HEIGHT, 480);

        DrowsinessMonitor monitor;
        DrowsinessState prevState = DrowsinessState::Unknown;

        int frames = 0;
        double fps = 0.0;
        auto fpsTick = Clock::now();

        std::cout << "Running cabin drowsiness demo\n"
                  << "Face cascade: " << faceCascadePath << "\n"
                  << "Eye  cascade: " << eyeCascadePath << "\n"
                  << "Alert sound: " << alertSoundPath << "\n"
                  << "Press ESC or q to exit.\n";

        cv::namedWindow("Cabin Drowsiness Demo", cv::WINDOW_NORMAL | cv::WINDOW_KEEPRATIO);
        cv::resizeWindow("Cabin Drowsiness Demo", 1280, 720);

        while (true) {
            const double visible = cv::getWindowProperty("Cabin Drowsiness Demo", cv::WND_PROP_VISIBLE);
            if (visible < 1.0) {
                break;
            }

            cv::Mat frame;
            if (!cap.read(frame) || frame.empty()) {
                std::cerr << "Failed to read frame\n";
                continue;
            }

            ++frames;
            const auto now = Clock::now();

            if (toMs(now - fpsTick) >= 1000) {
                fps = frames * 1000.0 / std::max(1LL, toMs(now - fpsTick));
                frames = 0;
                fpsTick = now;
            }

            cv::Mat gray;
            cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
            cv::equalizeHist(gray, gray);

            std::vector<cv::Rect> faces;
            faceCascade.detectMultiScale(
                gray,
                faces,
                1.1,
                4,
                cv::CASCADE_SCALE_IMAGE,
                cv::Size(80, 80)
            );

            bool eyesVisible = false;
            cv::Rect bestFace;

            if (!faces.empty()) {
                bestFace = *std::max_element(
                    faces.begin(),
                    faces.end(),
                    [](const cv::Rect& a, const cv::Rect& b) {
                        return a.area() < b.area();
                    }
                );

                cv::rectangle(frame, bestFace, cv::Scalar(0, 255, 0), 2);

                cv::Mat faceROI = gray(bestFace);
                std::vector<cv::Rect> eyes;
                eyeCascade.detectMultiScale(
                    faceROI,
                    eyes,
                    1.1,
                    3,
                    cv::CASCADE_SCALE_IMAGE,
                    cv::Size(20, 20)
                );

                for (const auto& e : eyes) {
                    cv::Point eyeCenter(bestFace.x + e.x + e.width / 2,
                                        bestFace.y + e.y + e.height / 2);
                    cv::Size eyeSize(e.width / 2, e.height / 2);
                    cv::ellipse(frame, eyeCenter, eyeSize, 0, 0, 360, cv::Scalar(255, 0, 0), 2);
                }

                eyesVisible = (eyes.size() >= 1);
            }

            const auto result = monitor.update(eyesVisible, now);

            if (result.state == DrowsinessState::Alert && prevState != DrowsinessState::Alert) {
                playAlertSound(alertSoundPath);
            }
            prevState = result.state;

            cv::Scalar stateColor(255, 255, 255);
            if (result.state == DrowsinessState::OK)      stateColor = cv::Scalar(0, 255, 0);
            if (result.state == DrowsinessState::Drowsy)  stateColor = cv::Scalar(0, 165, 255);
            if (result.state == DrowsinessState::Alert)   stateColor = cv::Scalar(0, 0, 255);

            drawLabel(frame,
                      "State: " + stateToString(result.state),
                      cv::Point(20, 40),
                      stateColor);

            drawLabel(frame,
                      "Eyes visible: " + std::string(result.eyesVisible ? "yes" : "no"),
                      cv::Point(20, 75),
                      cv::Scalar(255, 255, 255));

            drawLabel(frame,
                      "Closed ms: " + std::to_string(result.closedMs),
                      cv::Point(20, 110),
                      cv::Scalar(255, 255, 255));

            drawLabel(frame,
                      "Blinks: " + std::to_string(result.blinkCount),
                      cv::Point(20, 145),
                      cv::Scalar(255, 255, 255));

            drawLabel(frame,
                      "FPS: " + cv::format("%.1f", fps),
                      cv::Point(20, 180),
                      cv::Scalar(255, 255, 255));

            if (result.state == DrowsinessState::Drowsy || result.state == DrowsinessState::Alert) {
                const std::string warn = (result.state == DrowsinessState::Drowsy)
                    ? "Drowsiness detected"
                    : "ALERT: eyes closed too long";
                drawLabel(frame, warn, cv::Point(20, 225), stateColor);
            }

            if (!faces.empty() && !eyesVisible) {
                drawLabel(frame,
                          "No eyes detected in face ROI",
                          cv::Point(20, 260),
                          cv::Scalar(255, 255, 255));
            } else if (faces.empty()) {
                drawLabel(frame,
                          "No face detected",
                          cv::Point(20, 260),
                          cv::Scalar(255, 255, 255));
            }

            cv::imshow("Cabin Drowsiness Demo", frame);

            const int key = cv::waitKey(1);
            if (key == 27 || key == 'q' || key == 'Q') {
                break;
            }
        }

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
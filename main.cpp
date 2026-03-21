#include <opencv2/opencv.hpp>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using Clock = std::chrono::steady_clock;

enum class DrowsinessState {
    Unknown,
    OK,
    Drowsy,
    Alert
};

static std::string stateToString(DrowsinessState state) {
    switch (state) {
        case DrowsinessState::Unknown: return "UNKNOWN";
        case DrowsinessState::OK:      return "OK";
        case DrowsinessState::Drowsy:   return "DROWSY";
        case DrowsinessState::Alert:    return "ALERT";
    }
    return "UNKNOWN";
}

static long long toMs(const Clock::duration& d) {
    return std::chrono::duration_cast<std::chrono::milliseconds>(d).count();
}

static std::optional<std::string> firstExistingPath(const std::vector<std::string>& candidates) {
    for (const auto& p : candidates) {
        if (!p.empty() && fs::exists(p)) {
            return p;
        }
    }
    return std::nullopt;
}

struct Options {
    int cameraIndex = 0;
    std::string faceCascadePath;
    std::string eyeCascadePath;
};

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

class DrowsinessMonitor {
public:
    struct Result {
        DrowsinessState state = DrowsinessState::Unknown;
        bool eyesVisible = false;
        bool eyesClosed = false;
        int blinkCount = 0;
        long long closedMs = 0;
    };

    Result update(bool eyesVisible, Clock::time_point now) {
        result_.eyesVisible = eyesVisible;

        if (eyesVisible) {
            if (eyesClosed_) {
                const auto closedDurationMs = toMs(now - closedSince_);
                if (closedDurationMs >= blinkMinMs_ && closedDurationMs <= blinkMaxMs_) {
                    ++blinkCount_;
                }
            }

            eyesClosed_ = false;
            result_.eyesClosed = false;
            result_.closedMs = 0;
            result_.state = DrowsinessState::OK;
        } else {
            if (!eyesClosed_) {
                eyesClosed_ = true;
                closedSince_ = now;
            }

            result_.eyesClosed = true;
            result_.closedMs = toMs(now - closedSince_);

            if (result_.closedMs >= alertThresholdMs_) {
                result_.state = DrowsinessState::Alert;
            } else if (result_.closedMs >= drowsyThresholdMs_) {
                result_.state = DrowsinessState::Drowsy;
            } else {
                result_.state = DrowsinessState::OK;
            }
        }

        result_.blinkCount = blinkCount_;
        return result_;
    }

private:
    bool eyesClosed_ = false;
    Clock::time_point closedSince_{};
    int blinkCount_ = 0;

    Result result_{};

    const long long blinkMinMs_ = 50;
    const long long blinkMaxMs_ = 700;
    const long long drowsyThresholdMs_ = 1200;
    const long long alertThresholdMs_ = 2500;
};

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

static std::string defaultFaceCascade() {
    const auto path = firstExistingPath({
        "/usr/share/opencv4/haarcascades/haarcascade_frontalface_default.xml",
        "/usr/share/opencv/haarcascades/haarcascade_frontalface_default.xml",
        "/usr/local/share/opencv4/haarcascades/haarcascade_frontalface_default.xml"
    });
    if (!path) {
        throw std::runtime_error(
            "Cannot find haarcascade_frontalface_default.xml. "
            "Pass --face-cascade PATH.");
    }
    return *path;
}

static std::string defaultEyeCascade() {
    const auto path = firstExistingPath({
        "/usr/share/opencv4/haarcascades/haarcascade_eye_tree_eyeglasses.xml",
        "/usr/share/opencv/haarcascades/haarcascade_eye_tree_eyeglasses.xml",
        "/usr/local/share/opencv4/haarcascades/haarcascade_eye_tree_eyeglasses.xml"
    });
    if (!path) {
        throw std::runtime_error(
            "Cannot find haarcascade_eye_tree_eyeglasses.xml. "
            "Pass --eye-cascade PATH.");
    }
    return *path;
}

int main(int argc, char** argv) {
    try {
        const Options opt = parseArgs(argc, argv);

        const std::string faceCascadePath =
            opt.faceCascadePath.empty() ? defaultFaceCascade() : opt.faceCascadePath;
        const std::string eyeCascadePath =
            opt.eyeCascadePath.empty() ? defaultEyeCascade() : opt.eyeCascadePath;

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

        cap.set(cv::CAP_PROP_FRAME_WIDTH, 1280);
        cap.set(cv::CAP_PROP_FRAME_HEIGHT, 720);

        DrowsinessMonitor monitor;

        int frames = 0;
        double fps = 0.0;
        auto fpsTick = Clock::now();

        std::cout << "Running cabin drowsiness demo\n"
                  << "Face cascade: " << faceCascadePath << "\n"
                  << "Eye  cascade: " << eyeCascadePath << "\n"
                  << "Press ESC or q to exit.\n";

        cv::namedWindow("Cabin Drowsiness Demo", cv::WINDOW_NORMAL | cv::WINDOW_KEEPRATIO);
        cv::resizeWindow("Cabin Drowsiness Demo", 1280, 720);

        while (true) {
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
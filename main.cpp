/**
 * @file main.cpp
 * @brief AUTOSAR compliant cabin drowsiness detection system main application
 * @version 1.0
 * @date 2026-03-22
 */

#include "drowsiness_monitor.h"
#include "alert_sound.h"
#include "cascade_loader.h"

#include <opencv2/opencv.hpp>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

// AUTOSAR: Use explicit namespace
namespace drowsiness_detection {

    /**
     * @brief Application error codes
     */
    enum class AppError : std::uint8_t {
        Success = 0U,
        InvalidArguments = 1U,
        CascadeLoadFailed = 2U,
        CameraInitFailed = 3U,
        OpenCVError = 4U,
        SystemError = 5U
    };

    /**
     * @brief Application configuration structure
     */
    struct ApplicationConfig final {
        std::int32_t cameraIndex{0};
        std::string faceCascadePath{};
        std::string eyeCascadePath{};

        // AUTOSAR: Explicit default operations
        ApplicationConfig() noexcept = default;
        ApplicationConfig(const ApplicationConfig&) = default;
        ApplicationConfig& operator=(const ApplicationConfig&) = default;
        ApplicationConfig(ApplicationConfig&&) noexcept = default;
        ApplicationConfig& operator=(ApplicationConfig&&) noexcept = default;
        ~ApplicationConfig() = default;
    };

    /**
     * @brief Application state structure
     */
    struct ApplicationState final {
        cv::VideoCapture camera{};
        cv::CascadeClassifier faceCascade{};
        cv::CascadeClassifier eyeCascade{};
        DrowsinessMonitor monitor{};
        DrowsinessState previousState{DrowsinessState::Unknown};
        std::uint32_t frameCount{0U};
        double fps{0.0};
        TimePoint fpsLastTick{Clock::now()};

        ApplicationState() noexcept = default;

        // AUTOSAR: Move-only resource management
        ApplicationState(const ApplicationState&) = delete;
        ApplicationState& operator=(const ApplicationState&) = delete;
        ApplicationState(ApplicationState&&) noexcept = default;
        ApplicationState& operator=(ApplicationState&&) noexcept = default;
        ~ApplicationState() = default;
    };

    /**
     * @brief Convert application error to string
     * @param error Application error code
     * @return String representation
     */
    static std::string appErrorToString(const AppError error) noexcept {
        switch (error) {
            case AppError::Success:
                return std::string{"Success"};
            case AppError::InvalidArguments:
                return std::string{"Invalid command line arguments"};
            case AppError::CascadeLoadFailed:
                return std::string{"Failed to load cascade files"};
            case AppError::CameraInitFailed:
                return std::string{"Camera initialization failed"};
            case AppError::OpenCVError:
                return std::string{"OpenCV operation error"};
            case AppError::SystemError:
                return std::string{"System error"};
            default:
                return std::string{"Unknown application error"};
        }
    }

    /**
     * @brief Calculate milliseconds from duration
     * @param duration Time duration
     * @return Milliseconds as int64_t
     */
    static std::int64_t durationToMs(const Clock::duration& duration) noexcept {
        const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration);
        return static_cast<std::int64_t>(ms.count());
    }

    /**
     * @brief Parse command line arguments
     * @param argc Argument count
     * @param argv Argument values
     * @param config Output configuration
     * @return AppError indicating parse result
     */
    static AppError parseCommandLineArguments(
        const int argc,
        char* const argv[],
        ApplicationConfig& config) noexcept {

        if (argc < 0 || argv == nullptr) {
            return AppError::InvalidArguments;
        }

        // AUTOSAR: Use explicit loop with bounds checking
        for (int i = 1; i < argc; ++i) {
            if (argv[i] == nullptr) {
                return AppError::InvalidArguments;
            }

            const std::string arg{argv[i]};

            if (arg == "--camera") {
                if (i + 1 >= argc || argv[i + 1] == nullptr) {
                    return AppError::InvalidArguments;
                }

                // AUTOSAR: Safe string to integer conversion
                try {
                    const std::string valueStr{argv[i + 1]};
                    const long value = std::stol(valueStr);

                    // AUTOSAR: Validate range
                    if (value < 0 || value > std::numeric_limits<std::int32_t>::max()) {
                        return AppError::InvalidArguments;
                    }

                    config.cameraIndex = static_cast<std::int32_t>(value);
                    ++i; // Skip next argument as it was consumed
                } catch (...) {
                    return AppError::InvalidArguments;
                }
            } else if (arg == "--face-cascade") {
                if (i + 1 >= argc || argv[i + 1] == nullptr) {
                    return AppError::InvalidArguments;
                }
                config.faceCascadePath = std::string{argv[i + 1]};
                ++i;
            } else if (arg == "--eye-cascade") {
                if (i + 1 >= argc || argv[i + 1] == nullptr) {
                    return AppError::InvalidArguments;
                }
                config.eyeCascadePath = std::string{argv[i + 1]};
                ++i;
            } else if (arg == "--help" || arg == "-h") {
                std::cout <<
                    "AUTOSAR Compliant Cabin Drowsiness Detection System\n"
                    "Usage:\n"
                    "  ./cabin_drowsiness_demo [--camera N] [--face-cascade PATH] [--eye-cascade PATH]\n\n"
                    "Options:\n"
                    "  --camera N          Camera index (default: 0)\n"
                    "  --face-cascade PATH Path to face cascade file\n"
                    "  --eye-cascade PATH  Path to eye cascade file\n"
                    "  --help, -h          Show this help message\n\n"
                    "Example:\n"
                    "  ./cabin_drowsiness_demo --camera 0\n";
                std::exit(0);
            } else {
                return AppError::InvalidArguments;
            }
        }

        return AppError::Success;
    }

    /**
     * @brief Initialize application state with configuration
     * @param config Application configuration
     * @param state Application state to initialize
     * @return AppError indicating initialization result
     */
    static AppError initializeApplication(
        const ApplicationConfig& config,
        ApplicationState& state) noexcept {

        // AUTOSAR: Initialize cascade paths with error handling
        std::string faceCascadePath = config.faceCascadePath;
        std::string eyeCascadePath = config.eyeCascadePath;

        if (faceCascadePath.empty()) {
            const CascadeResult faceResult = getDefaultFaceCascade();
            if (!faceResult.isSuccess()) {
                return AppError::CascadeLoadFailed;
            }
            faceCascadePath = faceResult.path;
        }

        if (eyeCascadePath.empty()) {
            const CascadeResult eyeResult = getDefaultEyeCascade();
            if (!eyeResult.isSuccess()) {
                return AppError::CascadeLoadFailed;
            }
            eyeCascadePath = eyeResult.path;
        }

        // AUTOSAR: Load cascade files with validation
        if (!state.faceCascade.load(faceCascadePath)) {
            return AppError::CascadeLoadFailed;
        }

        if (!state.eyeCascade.load(eyeCascadePath)) {
            return AppError::CascadeLoadFailed;
        }

        // AUTOSAR: Initialize camera with error checking
        state.camera.open(config.cameraIndex);
        if (!state.camera.isOpened()) {
            return AppError::CameraInitFailed;
        }

        // AUTOSAR: Configure camera with explicit values
        static constexpr int CAMERA_WIDTH = 640;
        static constexpr int CAMERA_HEIGHT = 480;

        const bool widthSet = state.camera.set(cv::CAP_PROP_FRAME_WIDTH, CAMERA_WIDTH);
        const bool heightSet = state.camera.set(cv::CAP_PROP_FRAME_HEIGHT, CAMERA_HEIGHT);

        if (!widthSet || !heightSet) {
            // Note: Non-critical error, continue with default resolution
            std::cout << "Warning: Could not set preferred camera resolution\n";
        }

        // AUTOSAR: Display initialization information
        std::cout << "AUTOSAR Compliant Cabin Drowsiness Detection System\n"
                  << "Face cascade: " << faceCascadePath << "\n"
                  << "Eye cascade: " << eyeCascadePath << "\n";

        return AppError::Success;
    }

    /**
     * @brief Draw text label on frame with background
     * @param frame Output frame
     * @param text Text to draw
     * @param origin Label origin point
     * @param color Text color
     */
    static void drawTextLabel(
        cv::Mat& frame,
        const std::string& text,
        const cv::Point& origin,
        const cv::Scalar& color) noexcept {

        // AUTOSAR: Use explicit constants
        static constexpr int BASELINE = 0;
        static constexpr double FONT_SCALE = 0.6;
        static constexpr int THICKNESS = 1;
        static constexpr int FONT_FACE = cv::FONT_HERSHEY_SIMPLEX;
        static constexpr int PADDING = 12;
        static constexpr int VERTICAL_PADDING = 10;

        int baseline = BASELINE;
        const cv::Size textSize = cv::getTextSize(text, FONT_FACE, FONT_SCALE, THICKNESS, &baseline);

        const cv::Rect backgroundRect(
            origin.x,
            origin.y - textSize.height - VERTICAL_PADDING,
            textSize.width + PADDING,
            textSize.height + PADDING
        );

        cv::rectangle(frame, backgroundRect, cv::Scalar(0, 0, 0), cv::FILLED);
        cv::putText(frame, text, origin, FONT_FACE, FONT_SCALE, color, THICKNESS, cv::LINE_AA);
    }

    /**
     * @brief Process single frame for drowsiness detection
     * @param frame Input frame
     * @param state Application state
     * @param currentTime Current timestamp
     * @return AppError indicating processing result
     */
    static AppError processFrame(
        cv::Mat& frame,
        ApplicationState& state,
        const TimePoint currentTime) noexcept {

        if (frame.empty()) {
            return AppError::OpenCVError;
        }

        // AUTOSAR: Preprocess frame
        cv::Mat grayFrame;
        cv::cvtColor(frame, grayFrame, cv::COLOR_BGR2GRAY);
        cv::equalizeHist(grayFrame, grayFrame);

        // AUTOSAR: Face detection with explicit parameters
        std::vector<cv::Rect> faces;
        state.faceCascade.detectMultiScale(
            grayFrame,
            faces,
            1.1,                    // Scale factor
            4,                      // Min neighbors
            cv::CASCADE_SCALE_IMAGE, // Flags
            cv::Size(80, 80)        // Min size
        );

        bool eyesVisible = false;
        cv::Rect bestFace;

        if (!faces.empty()) {
            // AUTOSAR: Find largest face
            bestFace = *std::max_element(
                faces.begin(),
                faces.end(),
                [](const cv::Rect& a, const cv::Rect& b) noexcept {
                    return a.area() < b.area();
                }
            );

            cv::rectangle(frame, bestFace, cv::Scalar(0, 255, 0), 2);

            // AUTOSAR: Eye detection in face ROI
            if (bestFace.x >= 0 && bestFace.y >= 0 &&
                bestFace.x + bestFace.width <= grayFrame.cols &&
                bestFace.y + bestFace.height <= grayFrame.rows) {

                const cv::Mat faceROI = grayFrame(bestFace);
                std::vector<cv::Rect> eyes;
                state.eyeCascade.detectMultiScale(
                    faceROI,
                    eyes,
                    1.1,                    // Scale factor
                    3,                      // Min neighbors
                    cv::CASCADE_SCALE_IMAGE, // Flags
                    cv::Size(20, 20)        // Min size
                );

                // AUTOSAR: Draw detected eyes
                for (const auto& eye : eyes) {
                    const cv::Point eyeCenter(
                        bestFace.x + eye.x + eye.width / 2,
                        bestFace.y + eye.y + eye.height / 2
                    );
                    const cv::Size eyeSize(eye.width / 2, eye.height / 2);
                    cv::ellipse(frame, eyeCenter, eyeSize, 0, 0, 360, cv::Scalar(255, 0, 0), 2);
                }

                eyesVisible = (eyes.size() >= 1U);
            }
        }

        // AUTOSAR: Update monitor with explicit error handling
        const auto result = state.monitor.update(eyesVisible, currentTime);
        if (result.error != ErrorCode::Success) {
            return AppError::SystemError;
        }

        // AUTOSAR: Handle state change for audio alert
        if (result.state == DrowsinessState::Alert &&
            state.previousState != DrowsinessState::Alert) {

            const AudioResult audioPath = getDefaultAlertSoundPath();
            if (audioPath.isSuccess()) {
                const AudioError playResult = playAlertSoundSafe(audioPath.path);
                if (playResult != AudioError::Success) {
                    std::cout << "Audio warning: " << audioErrorToString(playResult) << "\n";
                }
            }
        }
        state.previousState = result.state;

        // AUTOSAR: Determine state color
        cv::Scalar stateColor(255, 255, 255);  // Default white
        switch (result.state) {
            case DrowsinessState::OK:
                stateColor = cv::Scalar(0, 255, 0);    // Green
                break;
            case DrowsinessState::Drowsy:
                stateColor = cv::Scalar(0, 165, 255);  // Orange
                break;
            case DrowsinessState::Alert:
                stateColor = cv::Scalar(0, 0, 255);    // Red
                break;
            case DrowsinessState::Unknown:
            default:
                // Keep default white
                break;
        }

        // AUTOSAR: Display status information
        drawTextLabel(frame,
                      "State: " + stateToString(result.state),
                      cv::Point(20, 40),
                      stateColor);

        drawTextLabel(frame,
                      "Eyes visible: " + std::string(result.eyesVisible ? "yes" : "no"),
                      cv::Point(20, 75),
                      cv::Scalar(255, 255, 255));

        drawTextLabel(frame,
                      "Closed ms: " + std::to_string(result.closedMs),
                      cv::Point(20, 110),
                      cv::Scalar(255, 255, 255));

        drawTextLabel(frame,
                      "Blinks: " + std::to_string(result.blinkCount),
                      cv::Point(20, 145),
                      cv::Scalar(255, 255, 255));

        drawTextLabel(frame,
                      "FPS: " + cv::format("%.1f", state.fps),
                      cv::Point(20, 180),
                      cv::Scalar(255, 255, 255));

        // AUTOSAR: Display warnings based on state
        if (result.state == DrowsinessState::Drowsy || result.state == DrowsinessState::Alert) {
            const std::string warning = (result.state == DrowsinessState::Drowsy)
                ? "Drowsiness detected"
                : "ALERT: eyes closed too long";
            drawTextLabel(frame, warning, cv::Point(20, 225), stateColor);
        }

        // AUTOSAR: Display detection status
        if (!faces.empty() && !eyesVisible) {
            drawTextLabel(frame,
                          "No eyes detected in face ROI",
                          cv::Point(20, 260),
                          cv::Scalar(255, 255, 255));
        } else if (faces.empty()) {
            drawTextLabel(frame,
                          "No face detected",
                          cv::Point(20, 260),
                          cv::Scalar(255, 255, 255));
        }

        return AppError::Success;
    }

    /**
     * @brief Update FPS calculation
     * @param state Application state
     * @param currentTime Current timestamp
     */
    static void updateFPS(ApplicationState& state, const TimePoint currentTime) noexcept {
        ++state.frameCount;

        const auto elapsed = durationToMs(currentTime - state.fpsLastTick);
        static constexpr std::int64_t FPS_UPDATE_INTERVAL_MS = 1000;

        if (elapsed >= FPS_UPDATE_INTERVAL_MS) {
            const std::int64_t safeElapsed = std::max(elapsed, std::int64_t{1});
            state.fps = static_cast<double>(state.frameCount) * 1000.0 / static_cast<double>(safeElapsed);
            state.frameCount = 0U;
            state.fpsLastTick = currentTime;
        }
    }

    /**
     * @brief Check if window is still open (AUTOSAR compliant)
     * @param windowName Name of the window to check
     * @param frameCount Number of frames processed (for stability check)
     * @return True if window is open, false if closed or error occurred
     */
    static bool checkWindowOpen(const char* windowName, std::uint32_t frameCount) {
        // AUTOSAR: Only check window properties after sufficient initialization
        static constexpr std::uint32_t MIN_FRAMES_FOR_STABILITY = 15U;
        if (frameCount <= MIN_FRAMES_FOR_STABILITY) {
            return true;  // Assume open during initialization
        }

        // AUTOSAR: OpenCV getWindowProperty can throw exceptions in certain builds
        // We need to handle this defensively while maintaining AUTOSAR principles
        // This is a contained use of exception handling for third-party library safety

        bool windowIsOpen = true;  // Default assumption

        // AUTOSAR: Defensive programming - contain potential OpenCV exceptions
        // Note: This violates pure AUTOSAR no-exceptions rule, but is necessary
        // for interfacing with OpenCV library. In production automotive code,
        // this should be replaced with a display system service API that
        // provides window state without exceptions.
        try {
            const double visibility = cv::getWindowProperty(windowName, cv::WND_PROP_VISIBLE);

            // Window is considered closed if visibility is non-positive or indicates error
            if (visibility <= 0.0 || visibility != visibility) {  // Check for NaN
                windowIsOpen = false;
            }
        } catch (...) {
            // AUTOSAR: Any exception from OpenCV indicates window issue
            // In automotive systems, this would be logged as a system event
            windowIsOpen = false;
        }

        return windowIsOpen;
    }

    /**
     * @brief Main application loop
     * @param state Application state
     * @return AppError indicating loop result
     */
    static AppError runMainLoop(ApplicationState& state) noexcept {
        static constexpr char WINDOW_NAME[] = "AUTOSAR Cabin Drowsiness Demo";

        cv::namedWindow(WINDOW_NAME, cv::WINDOW_NORMAL | cv::WINDOW_KEEPRATIO);
        cv::resizeWindow(WINDOW_NAME, 1280, 720);

        std::cout << "Press ESC, 'q', or close window (X) to exit.\n";

        std::uint32_t totalFrameCount = 0U;

        while (true) {
            const TimePoint currentTime = Clock::now();

            cv::Mat frame;
            if (!state.camera.read(frame) || frame.empty()) {
                std::cerr << "Warning: Failed to read camera frame\n";
                continue;
            }

            updateFPS(state, currentTime);
            ++totalFrameCount;

            const AppError processResult = processFrame(frame, state, currentTime);
            if (processResult != AppError::Success) {
                std::cerr << "Frame processing error: " << appErrorToString(processResult) << "\n";
                continue;
            }

            cv::imshow(WINDOW_NAME, frame);

            // AUTOSAR: Handle keyboard input with explicit key codes
            const int key = cv::waitKey(1);
            static constexpr int ESC_KEY = 27;
            static constexpr int Q_KEY_LOWER = 'q';
            static constexpr int Q_KEY_UPPER = 'Q';

            if (key == ESC_KEY || key == Q_KEY_LOWER || key == Q_KEY_UPPER) {
                break;
            }

            // AUTOSAR: Check if window was closed by X button
            // This uses a defensive approach to avoid OpenCV exceptions
            if (!checkWindowOpen(WINDOW_NAME, totalFrameCount)) {
                break;
            }
        }

        // AUTOSAR: Safe window cleanup with exception handling
        // OpenCV window destruction can also throw exceptions in certain situations
        try {
            cv::destroyWindow(WINDOW_NAME);
        } catch (...) {
            // AUTOSAR: Log the issue but don't fail the application
            // In production automotive systems, this would be reported as a system event
            std::cerr << "Warning: Failed to properly destroy OpenCV window\n";
        }

        return AppError::Success;
    }

} // namespace drowsiness_detection

/**
 * @brief AUTOSAR compliant main function
 * @param argc Argument count
 * @param argv Argument vector
 * @return Exit code (0 for success, non-zero for error)
 */
int main(int argc, char** argv) {
    using namespace drowsiness_detection;

    // AUTOSAR: Explicit error handling without exceptions
    ApplicationConfig config;
    const AppError parseResult = parseCommandLineArguments(argc, argv, config);
    if (parseResult != AppError::Success) {
        std::cerr << "Error: " << appErrorToString(parseResult) << "\n";
        return static_cast<int>(parseResult);
    }

    ApplicationState state;
    const AppError initResult = initializeApplication(config, state);
    if (initResult != AppError::Success) {
        std::cerr << "Error: " << appErrorToString(initResult) << "\n";
        return static_cast<int>(initResult);
    }

    const AppError runResult = runMainLoop(state);
    if (runResult != AppError::Success) {
        std::cerr << "Error: " << appErrorToString(runResult) << "\n";
        return static_cast<int>(runResult);
    }

    return static_cast<int>(AppError::Success);
}
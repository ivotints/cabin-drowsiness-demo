#include "drowsiness_monitor.h"

#include <chrono>
#include <iostream>

using namespace drowsiness_detection;
using Clock = drowsiness_detection::Clock;

static bool expect(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << "\n";
        return false;
    }
    return true;
}

int main() {
    bool success = true;

    // stateToString checks
    success &= expect(stateToString(DrowsinessState::Unknown) == "UNKNOWN", "Unknown state string");
    success &= expect(stateToString(DrowsinessState::OK) == "OK", "OK state string");
    success &= expect(stateToString(DrowsinessState::Drowsy) == "DROWSY", "Drowsy state string");
    success &= expect(stateToString(DrowsinessState::Alert) == "ALERT", "Alert state string");

    // errorToString checks
    success &= expect(errorToString(ErrorCode::Success) == "Success", "Success error string");
    success &= expect(errorToString(ErrorCode::InvalidInput) == "Invalid input parameter", "Invalid input error string");
    success &= expect(errorToString(ErrorCode::TimingError) == "Timing validation failed", "Timing error string");
    success &= expect(errorToString(ErrorCode::StateError) == "State machine error", "State error string");

    // DrowsinessMonitor lifecycle tests
    DrowsinessMonitor monitor;
    expect(monitor.getBlinkCount() == 0u, "Initial blink count should be zero");
    expect(monitor.reset() == ErrorCode::Success, "Reset should return success");
    expect(monitor.getBlinkCount() == 0u, "Blink count after reset should be zero");

    const auto t0 = Clock::now();
    const auto t1 = t0 + std::chrono::milliseconds(100);
    const auto t2 = t1 + std::chrono::milliseconds(50);

    // eyes open -> ok
    {
        const auto result = monitor.update(true, t1);
        success &= expect(result.state == DrowsinessState::OK, "Eyes visible should set state OK");
        success &= expect(!result.eyesClosed, "Eyes closed false when visible");
        success &= expect(result.blinkCount == 0u, "No blink counted yet");
    }

    // blink detection boundary at 50 ms
    {
        auto r1 = monitor.update(false, t1);
        success &= expect(r1.eyesClosed, "Eyes should be closed after false input");

        auto r2 = monitor.update(true, t2);
        success &= expect(r2.state == DrowsinessState::OK, "After short closure state should be OK");
        success &= expect(r2.blinkCount == 1u, "Blink count should be incremented for 50ms closure");
        success &= expect(monitor.getBlinkCount() == 1u, "Internal blink count should be 1");
    }

    // drowsy and alert thresholds
    {
        monitor.reset();
        const auto t_start = t0 + std::chrono::milliseconds(500);
        monitor.update(false, t_start); // start closure

        const auto t_drowsy = t_start + std::chrono::milliseconds(1300);
        auto drowsyResult = monitor.update(false, t_drowsy);
        success &= expect(drowsyResult.state == DrowsinessState::Drowsy, "Closed >1200ms should be DROWSY");

        const auto t_alert = t_start + std::chrono::milliseconds(2600);
        auto alertResult = monitor.update(false, t_alert);
        success &= expect(alertResult.state == DrowsinessState::Alert, "Closed >2500ms should be ALERT");
    }

    // timing error validation
    {
        monitor.reset();
        const auto t_start = t0 + std::chrono::milliseconds(1000);
        monitor.update(false, t_start);
        const auto t_invalid = t_start - std::chrono::milliseconds(500);
        auto invalidResult = monitor.update(false, t_invalid);
        success &= expect(invalidResult.error == ErrorCode::TimingError, "Backward time should return TimingError");
    }

    if (!success) {
        std::cerr << "Some drowsiness tests failed\n";
        return 1;
    }

    std::cout << "All drowsiness tests passed\n";
    return 0;
}

#pragma once

#include <chrono>
#include <string>

using Clock = std::chrono::steady_clock;

enum class DrowsinessState {
    Unknown,
    OK,
    Drowsy,
    Alert
};

std::string stateToString(DrowsinessState state);

class DrowsinessMonitor {
public:
    struct Result {
        DrowsinessState state = DrowsinessState::Unknown;
        bool eyesVisible = false;
        bool eyesClosed = false;
        int blinkCount = 0;
        long long closedMs = 0;
    };

    Result update(bool eyesVisible, Clock::time_point now);

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

#include "drowsiness_monitor.h"

#include <chrono>

std::string stateToString(DrowsinessState state) {
    switch (state) {
        case DrowsinessState::Unknown: return "UNKNOWN";
        case DrowsinessState::OK:      return "OK";
        case DrowsinessState::Drowsy:   return "DROWSY";
        case DrowsinessState::Alert:    return "ALERT";
    }
    return "UNKNOWN";
}

DrowsinessMonitor::Result DrowsinessMonitor::update(bool eyesVisible, Clock::time_point now) {
    result_.eyesVisible = eyesVisible;

    if (eyesVisible) {
        if (eyesClosed_) {
            const auto closedDurationMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - closedSince_).count();
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
        result_.closedMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - closedSince_).count();

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

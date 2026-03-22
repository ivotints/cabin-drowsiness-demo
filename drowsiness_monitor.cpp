/**
 * @file drowsiness_monitor.cpp
 * @brief AUTOSAR compliant drowsiness detection monitor implementation
 * @version 1.0
 * @date 2026-03-22
 */

#include "drowsiness_monitor.h"

#include <algorithm>
#include <chrono>

namespace drowsiness_detection {

    std::string stateToString(const DrowsinessState state) noexcept {
        // AUTOSAR: Use switch with explicit cases and default
        switch (state) {
            case DrowsinessState::Unknown:
                return std::string{"UNKNOWN"};
            case DrowsinessState::OK:
                return std::string{"OK"};
            case DrowsinessState::Drowsy:
                return std::string{"DROWSY"};
            case DrowsinessState::Alert:
                return std::string{"ALERT"};
            default:
                // AUTOSAR: Always handle default case
                return std::string{"INVALID_STATE"};
        }
    }

    std::string errorToString(const ErrorCode error) noexcept {
        switch (error) {
            case ErrorCode::Success:
                return std::string{"Success"};
            case ErrorCode::InvalidInput:
                return std::string{"Invalid input parameter"};
            case ErrorCode::TimingError:
                return std::string{"Timing validation failed"};
            case ErrorCode::StateError:
                return std::string{"State machine error"};
            default:
                return std::string{"Unknown error"};
        }
    }

    // AUTOSAR: Explicit constructor with member initialization
    DrowsinessMonitor::DrowsinessMonitor() noexcept
        : eyesClosed_{false}
        , closedSince_{}
        , blinkCount_{0U}
        , currentResult_{} {
        // AUTOSAR: Explicit initialization in constructor body if needed
        currentResult_.state = DrowsinessState::Unknown;
        currentResult_.error = ErrorCode::Success;
    }

    DrowsinessMonitor::Result DrowsinessMonitor::update(
        const bool eyesVisible,
        const TimePoint currentTime) noexcept {

        // AUTOSAR: Validate inputs first
        const ErrorCode timingError = validateTiming(currentTime);
        if (timingError != ErrorCode::Success) {
            currentResult_.error = timingError;
            return currentResult_;
        }

        // AUTOSAR: Update result with current input
        currentResult_.eyesVisible = eyesVisible;
        currentResult_.error = ErrorCode::Success;

        if (eyesVisible) {
            // Eyes are visible - handle potential blink completion
            if (eyesClosed_) {
                const std::int64_t closedDurationMs = calculateDurationMs(closedSince_, currentTime);

                // AUTOSAR: Use explicit comparisons with constants
                if (closedDurationMs >= BLINK_MIN_MS && closedDurationMs <= BLINK_MAX_MS) {
                    // AUTOSAR: Check for overflow before incrementing
                    if (blinkCount_ < std::numeric_limits<std::uint32_t>::max()) {
                        ++blinkCount_;
                    }
                }
            }

            // Reset closed eye state
            eyesClosed_ = false;
            currentResult_.eyesClosed = false;
            currentResult_.closedMs = 0;
            currentResult_.state = DrowsinessState::OK;
        } else {
            // Eyes are not visible
            if (!eyesClosed_) {
                // Start tracking closed eyes
                eyesClosed_ = true;
                closedSince_ = currentTime;
            }

            currentResult_.eyesClosed = true;
            currentResult_.closedMs = calculateDurationMs(closedSince_, currentTime);

            // AUTOSAR: Determine state based on explicit thresholds
            if (currentResult_.closedMs >= ALERT_THRESHOLD_MS) {
                currentResult_.state = DrowsinessState::Alert;
            } else if (currentResult_.closedMs >= DROWSY_THRESHOLD_MS) {
                currentResult_.state = DrowsinessState::Drowsy;
            } else {
                currentResult_.state = DrowsinessState::OK;
            }
        }

        currentResult_.blinkCount = blinkCount_;
        return currentResult_;
    }

    ErrorCode DrowsinessMonitor::reset() noexcept {
        // AUTOSAR: Explicit reset of all state variables
        eyesClosed_ = false;
        closedSince_ = TimePoint{};
        blinkCount_ = 0U;

        // Reset result structure
        currentResult_ = Result{};
        currentResult_.state = DrowsinessState::Unknown;
        currentResult_.error = ErrorCode::Success;

        return ErrorCode::Success;
    }

    std::uint32_t DrowsinessMonitor::getBlinkCount() const noexcept {
        return blinkCount_;
    }

    // AUTOSAR: Private helper methods with explicit error handling
    std::int64_t DrowsinessMonitor::calculateDurationMs(
        const TimePoint start,
        const TimePoint end) noexcept {

        // AUTOSAR: Handle potential time calculation errors
        if (end < start) {
            return 0;
        }

        const auto duration = end - start;
        const auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(duration);

        // AUTOSAR: Return explicit type
        return static_cast<std::int64_t>(milliseconds.count());
    }

    ErrorCode DrowsinessMonitor::validateTiming(const TimePoint currentTime) const noexcept {
        // AUTOSAR: Validate that time is progressing forward
        if (eyesClosed_ && (currentTime < closedSince_)) {
            return ErrorCode::TimingError;
        }

        // AUTOSAR: Additional timing validations can be added here
        static const auto EPOCH = TimePoint{};
        if (currentTime <= EPOCH) {
            return ErrorCode::TimingError;
        }

        return ErrorCode::Success;
    }

} // namespace drowsiness_detection

/**
 * @file drowsiness_monitor.h
 * @brief AUTOSAR compliant drowsiness detection monitor interface
 * @version 1.0
 * @date 2026-03-22
 */

#pragma once

#include <chrono>
#include <cstdint>
#include <string>

// AUTOSAR: Use explicit namespace to avoid global namespace pollution
namespace drowsiness_detection {

    // AUTOSAR: Use strong typing with scoped enums
    using Clock = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;
    using Milliseconds = std::chrono::milliseconds;

    // AUTOSAR: Use explicit underlying type for enums
    enum class DrowsinessState : std::uint8_t {
        Unknown = 0U,
        OK = 1U,
        Drowsy = 2U,
        Alert = 3U
    };

    // AUTOSAR: Use explicit result type instead of exceptions
    enum class ErrorCode : std::uint8_t {
        Success = 0U,
        InvalidInput = 1U,
        TimingError = 2U,
        StateError = 3U
    };

    /**
     * @brief Convert drowsiness state to string representation
     * @param state The drowsiness state to convert
     * @return String representation of the state
     * @note AUTOSAR compliant - no exceptions, pure function
     */
    std::string stateToString(DrowsinessState state) noexcept;

    /**
     * @brief Convert error code to string representation
     * @param error The error code to convert
     * @return String representation of the error
     * @note AUTOSAR compliant - no exceptions, pure function
     */
    std::string errorToString(ErrorCode error) noexcept;

    /**
     * @brief AUTOSAR compliant drowsiness monitoring class
     * @details Monitors eye visibility and detects drowsiness patterns
     *          according to automotive safety standards
     */
    class DrowsinessMonitor final {
    public:
        /**
         * @brief Result structure for monitor updates
         * @note All members use explicit types for AUTOSAR compliance
         */
        struct Result final {
            DrowsinessState state{DrowsinessState::Unknown};
            bool eyesVisible{false};
            bool eyesClosed{false};
            std::uint32_t blinkCount{0U};
            std::int64_t closedMs{0};
            ErrorCode error{ErrorCode::Success};

            // AUTOSAR: Explicit default constructor
            Result() noexcept = default;

            // AUTOSAR: Disable copy/move for safety-critical code simplicity
            Result(const Result&) = default;
            Result& operator=(const Result&) = default;
            Result(Result&&) = default;
            Result& operator=(Result&&) = default;
            ~Result() = default;
        };

        /**
         * @brief Default constructor
         * @note AUTOSAR compliant initialization
         */
        DrowsinessMonitor() noexcept;

        /**
         * @brief Deleted copy constructor for unique ownership
         * @note AUTOSAR principle - clear resource ownership
         */
        DrowsinessMonitor(const DrowsinessMonitor&) = delete;
        DrowsinessMonitor& operator=(const DrowsinessMonitor&) = delete;

        /**
         * @brief Move constructor
         */
        DrowsinessMonitor(DrowsinessMonitor&&) noexcept = default;
        DrowsinessMonitor& operator=(DrowsinessMonitor&&) noexcept = default;

        /**
         * @brief Destructor
         */
        ~DrowsinessMonitor() = default;

        /**
         * @brief Update monitor with current eye visibility state
         * @param eyesVisible True if eyes are currently visible
         * @param currentTime Current timestamp
         * @return Result containing current drowsiness state and metrics
         * @note AUTOSAR compliant - no exceptions, explicit error handling
         */
        Result update(bool eyesVisible, TimePoint currentTime) noexcept;

        /**
         * @brief Reset monitor to initial state
         * @return ErrorCode indicating success or failure
         */
        ErrorCode reset() noexcept;

        /**
         * @brief Get current blink count
         * @return Number of detected blinks
         */
        std::uint32_t getBlinkCount() const noexcept;

    private:
        // AUTOSAR: Use explicit types and const for configuration
        static constexpr std::int64_t BLINK_MIN_MS{50};
        static constexpr std::int64_t BLINK_MAX_MS{700};
        static constexpr std::int64_t DROWSY_THRESHOLD_MS{1200};
        static constexpr std::int64_t ALERT_THRESHOLD_MS{2500};

        // AUTOSAR: Use explicit member initialization
        bool eyesClosed_{false};
        TimePoint closedSince_{};
        std::uint32_t blinkCount_{0U};
        Result currentResult_{};

        /**
         * @brief Calculate time difference in milliseconds
         * @param start Start time point
         * @param end End time point
         * @return Time difference in milliseconds
         */
        static std::int64_t calculateDurationMs(TimePoint start, TimePoint end) noexcept;

        /**
         * @brief Validate timing parameters
         * @param currentTime Current timestamp
         * @return ErrorCode indicating validity
         */
        ErrorCode validateTiming(TimePoint currentTime) const noexcept;
    };

} // namespace drowsiness_detection

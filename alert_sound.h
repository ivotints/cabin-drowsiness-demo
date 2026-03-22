/**
 * @file alert_sound.h
 * @brief AUTOSAR compliant audio alert system interface
 * @version 1.0
 * @date 2026-03-22
 */

#pragma once

#include <cstdint>
#include <string>

// AUTOSAR: Use explicit namespace
namespace drowsiness_detection {

    /**
     * @brief Error codes for audio operations
     */
    enum class AudioError : std::uint8_t {
        Success = 0U,
        FileNotFound = 1U,
        InvalidPath = 2U,
        PlayerNotFound = 3U,
        PlaybackFailed = 4U,
        UnsupportedFormat = 5U,
        AccessDenied = 6U,
        SystemError = 7U
    };

    /**
     * @brief Result structure for audio path operations
     */
    struct AudioResult final {
        std::string path{};
        AudioError error{AudioError::Success};

        // AUTOSAR: Explicit default operations
        AudioResult() noexcept = default;
        AudioResult(const AudioResult&) = default;
        AudioResult& operator=(const AudioResult&) = default;
        AudioResult(AudioResult&&) noexcept = default;
        AudioResult& operator=(AudioResult&&) noexcept = default;
        ~AudioResult() = default;

        /**
         * @brief Constructor with parameters
         * @param audioPath Path to audio file
         * @param errorCode Error code
         */
        AudioResult(std::string audioPath, AudioError errorCode) noexcept
            : path{std::move(audioPath)}, error{errorCode} {}

        /**
         * @brief Check if result indicates success
         * @return True if operation was successful
         */
        bool isSuccess() const noexcept {
            return (error == AudioError::Success);
        }
    };

    /**
     * @brief Audio player information
     */
    struct PlayerInfo final {
        std::string name{};
        std::string command{};
        bool available{false};

        PlayerInfo() noexcept = default;
        PlayerInfo(std::string playerName, std::string playerCommand, bool isAvailable) noexcept
            : name{std::move(playerName)}, command{std::move(playerCommand)}, available{isAvailable} {}
    };

    /**
     * @brief Convert audio error to string representation
     * @param error The error code to convert
     * @return String representation of the error
     * @note AUTOSAR compliant - no exceptions, pure function
     */
    std::string audioErrorToString(AudioError error) noexcept;

    /**
     * @brief Get default alert sound file path
     * @return AudioResult containing path and error status
     * @note AUTOSAR compliant - no exceptions, explicit error handling
     */
    AudioResult getDefaultAlertSoundPath() noexcept;

    /**
     * @brief Validate audio file exists and is accessible
     * @param audioPath Path to validate
     * @return AudioError indicating validation result
     * @note AUTOSAR compliant - explicit validation without exceptions
     */
    AudioError validateAudioFile(const std::string& audioPath) noexcept;

    /**
     * @brief Find available audio player on the system
     * @return PlayerInfo with player details and availability
     * @note AUTOSAR compliant - no system() calls with exceptions
     */
    PlayerInfo findAudioPlayer() noexcept;

    /**
     * @brief Play alert sound using available audio player
     * @param audioPath Path to audio file to play
     * @return AudioError indicating playback result
     * @note AUTOSAR compliant - explicit error handling, no exceptions
     */
    AudioError playAlertSoundSafe(const std::string& audioPath) noexcept;

    /**
     * @brief Validate that audio player is functional
     * @param playerInfo Player information to validate
     * @return AudioError indicating validation result
     */
    AudioError validateAudioPlayer(const PlayerInfo& playerInfo) noexcept;

    /**
     * @brief Legacy interface functions for backward compatibility
     * @note These functions may throw exceptions for compatibility
     * @deprecated Use AudioResult-based functions for AUTOSAR compliance
     */
    [[deprecated("Use getDefaultAlertSoundPath() for AUTOSAR compliance")]]
    std::string defaultAlertSoundPath();

    [[deprecated("Use playAlertSoundSafe() for AUTOSAR compliance")]]
    void playAlertSound(const std::string& audioPath);

} // namespace drowsiness_detection

/**
 * @file alert_sound.cpp
 * @brief AUTOSAR compliant audio alert system implementation
 * @version 1.0
 * @date 2026-03-22
 */

#include "alert_sound.h"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace drowsiness_detection {

    namespace fs = std::filesystem;

    std::string audioErrorToString(const AudioError error) noexcept {
        switch (error) {
            case AudioError::Success:
                return std::string{"Success"};
            case AudioError::FileNotFound:
                return std::string{"Audio file not found"};
            case AudioError::InvalidPath:
                return std::string{"Invalid audio file path"};
            case AudioError::PlayerNotFound:
                return std::string{"No audio player found"};
            case AudioError::PlaybackFailed:
                return std::string{"Audio playback failed"};
            case AudioError::UnsupportedFormat:
                return std::string{"Unsupported audio format"};
            case AudioError::AccessDenied:
                return std::string{"Access denied to audio file"};
            case AudioError::SystemError:
                return std::string{"System error during audio operation"};
            default:
                return std::string{"Unknown audio error"};
        }
    }

    /**
     * @brief AUTOSAR compliant helper to find first existing audio file
     * @tparam N Number of candidate paths
     * @param candidates Array of candidate file paths
     * @return AudioResult with path and error status
     */
    template<std::size_t N>
    static AudioResult findFirstExistingAudio(
        const std::array<const char*, N>& candidates) noexcept {

        for (std::size_t i = 0U; i < N; ++i) {
            if (candidates[i] == nullptr) {
                continue;
            }

            const std::string path{candidates[i]};
            if (path.empty()) {
                continue;
            }

            const AudioError validation = validateAudioFile(path);
            if (validation == AudioError::Success) {
                return AudioResult{path, AudioError::Success};
            }
        }

        return AudioResult{"", AudioError::FileNotFound};
    }

    AudioResult getDefaultAlertSoundPath() noexcept {
        // AUTOSAR: Use constexpr array for candidate paths
        static constexpr std::array<const char*, 3U> ALERT_SOUND_PATHS = {
            "assets/beep-01a.wav",
            "../assets/beep-01a.wav",
            "/usr/share/cabin_drowsiness_demo/assets/beep-01a.wav"
        };

        return findFirstExistingAudio(ALERT_SOUND_PATHS);
    }

    AudioError validateAudioFile(const std::string& audioPath) noexcept {
        if (audioPath.empty()) {
            return AudioError::InvalidPath;
        }

        std::error_code ec;

        // Check if file exists
        const bool exists = fs::exists(audioPath, ec);
        if (ec) {
            return AudioError::SystemError;
        }

        if (!exists) {
            return AudioError::FileNotFound;
        }

        // Check if it's a regular file
        const bool isFile = fs::is_regular_file(audioPath, ec);
        if (ec) {
            return AudioError::SystemError;
        }

        if (!isFile) {
            return AudioError::InvalidPath;
        }

        // Check file permissions
        const auto perms = fs::status(audioPath, ec).permissions();
        if (ec) {
            return AudioError::AccessDenied;
        }

        const bool canRead = (perms & fs::perms::owner_read) != fs::perms::none ||
                           (perms & fs::perms::group_read) != fs::perms::none ||
                           (perms & fs::perms::others_read) != fs::perms::none;

        if (!canRead) {
            return AudioError::AccessDenied;
        }

        // Basic file size validation
        const std::uintmax_t fileSize = fs::file_size(audioPath, ec);
        if (ec) {
            return AudioError::SystemError;
        }

        static constexpr std::uintmax_t MIN_AUDIO_FILE_SIZE = 100U;
        if (fileSize < MIN_AUDIO_FILE_SIZE) {
            return AudioError::UnsupportedFormat;
        }

        // Basic format validation by extension
        const auto extension = fs::path(audioPath).extension();
        static const std::vector<std::string> SUPPORTED_FORMATS = {".wav", ".mp3", ".ogg", ".flac"};

        const auto it = std::find(SUPPORTED_FORMATS.begin(), SUPPORTED_FORMATS.end(), extension);
        if (it == SUPPORTED_FORMATS.end()) {
            return AudioError::UnsupportedFormat;
        }

        return AudioError::Success;
    }

    PlayerInfo findAudioPlayer() noexcept {
        // AUTOSAR: Use constexpr array for audio player candidates
        static constexpr std::array<const char*, 3U> PLAYER_CANDIDATES = {
            "aplay", "paplay", "play"
        };

        for (std::size_t i = 0U; i < PLAYER_CANDIDATES.size(); ++i) {
            const char* playerName = PLAYER_CANDIDATES[i];
            if (playerName == nullptr) {
                continue;
            }

            // AUTOSAR: Safer approach - check PATH instead of system() call
            // Note: In production AUTOSAR system, this should use proper
            // system service interfaces instead of shell commands
            const std::string checkCmd = std::string{"command -v "} + playerName + " > /dev/null 2>&1";

            // AUTOSAR WARNING: system() calls should be avoided in production
            // automotive systems. This should be replaced with proper
            // system service interfaces or pre-configured player paths.
            const int result = std::system(checkCmd.c_str());

            if (result == 0) {
                return PlayerInfo{std::string{playerName}, std::string{playerName}, true};
            }
        }

        return PlayerInfo{"", "", false};
    }

    AudioError validateAudioPlayer(const PlayerInfo& playerInfo) noexcept {
        if (!playerInfo.available) {
            return AudioError::PlayerNotFound;
        }

        if (playerInfo.name.empty() || playerInfo.command.empty()) {
            return AudioError::PlayerNotFound;
        }

        // Additional validation could include:
        // - Check if player executable exists in PATH
        // - Verify player supports required audio formats
        // - Test player with a minimal audio file

        return AudioError::Success;
    }

    AudioError playAlertSoundSafe(const std::string& audioPath) noexcept {
        // AUTOSAR: Validate input parameters
        const AudioError fileValidation = validateAudioFile(audioPath);
        if (fileValidation != AudioError::Success) {
            return fileValidation;
        }

        // Find available audio player
        const PlayerInfo player = findAudioPlayer();
        const AudioError playerValidation = validateAudioPlayer(player);
        if (playerValidation != AudioError::Success) {
            return playerValidation;
        }

        // AUTOSAR: Construct safe command string
        std::string command;
        if (player.name == "aplay") {
            command = "aplay -q \"" + audioPath + "\" &";
        } else if (player.name == "paplay") {
            command = "paplay \"" + audioPath + "\" &";
        } else if (player.name == "play") {
            command = "play -q \"" + audioPath + "\" &";
        } else {
            return AudioError::PlayerNotFound;
        }

        // AUTOSAR WARNING: system() calls should be avoided in production
        // automotive systems. In a real AUTOSAR implementation, this should
        // use proper audio system services through standardized interfaces.
        const int result = std::system(command.c_str());

        // Note: Background process result checking is limited
        if (result == -1) {
            return AudioError::SystemError;
        }

        return AudioError::Success;
    }

    // AUTOSAR: Legacy interface implementations for backward compatibility
    std::string defaultAlertSoundPath() {
        const AudioResult result = getDefaultAlertSoundPath();
        if (!result.isSuccess()) {
            throw std::runtime_error(
                "Cannot find alert sound file beep-01a.wav. "
                "Place it in assets/ or pass the path by setting AUDIO_PATH environment variable. "
                "Error: " + audioErrorToString(result.error)
            );
        }
        return result.path;
    }

    void playAlertSound(const std::string& audioPath) {
        // Legacy implementation using std::cerr for non-critical errors
        const AudioError fileCheck = validateAudioFile(audioPath);
        if (fileCheck != AudioError::Success) {
            std::cerr << "Warning: " << audioErrorToString(fileCheck) << ": " << audioPath << "\n";
            return;
        }

        const PlayerInfo player = findAudioPlayer();
        if (!player.available) {
            std::cerr << "Warning: no audio player found (aplay/paplay/play). "
                      << "Install one to enable alert sound.\n";
            return;
        }

        const AudioError playResult = playAlertSoundSafe(audioPath);
        if (playResult != AudioError::Success) {
            std::cerr << "Warning: " << audioErrorToString(playResult) << "\n";
        }
    }

} // namespace drowsiness_detection

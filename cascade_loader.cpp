/**
 * @file cascade_loader.cpp
 * @brief AUTOSAR compliant OpenCV cascade loader implementation
 * @version 1.0
 * @date 2026-03-22
 */

#include "cascade_loader.h"

#include <algorithm>
#include <array>
#include <filesystem>
#include <stdexcept>

namespace drowsiness_detection {

    namespace fs = std::filesystem;

    std::string cascadeErrorToString(const CascadeError error) noexcept {
        switch (error) {
            case CascadeError::Success:
                return std::string{"Success"};
            case CascadeError::FileNotFound:
                return std::string{"File not found"};
            case CascadeError::InvalidPath:
                return std::string{"Invalid file path"};
            case CascadeError::AccessDenied:
                return std::string{"Access denied"};
            case CascadeError::CorruptedFile:
                return std::string{"Corrupted or invalid file"};
            case CascadeError::UnknownError:
                return std::string{"Unknown error"};
            default:
                return std::string{"Invalid error code"};
        }
    }

    /**
     * @brief AUTOSAR compliant helper to find first existing file path
     * @tparam N Number of candidate paths
     * @param candidates Array of candidate file paths
     * @return CascadeResult with path and error status
     */
    template<std::size_t N>
    static CascadeResult findFirstExistingPath(
        const std::array<const char*, N>& candidates) noexcept {

        // AUTOSAR: Explicit iteration with range checking
        for (std::size_t i = 0U; i < N; ++i) {
            if (candidates[i] == nullptr) {
                continue;  // Skip null entries
            }

            const std::string path{candidates[i]};
            if (path.empty()) {
                continue;  // Skip empty paths
            }

            // AUTOSAR: Use explicit error handling instead of exceptions
            std::error_code ec;
            const bool exists = fs::exists(path, ec);

            if (ec) {
                // Filesystem error occurred
                continue;
            }

            if (exists) {
                return CascadeResult{path, CascadeError::Success};
            }
        }

        return CascadeResult{"", CascadeError::FileNotFound};
    }

    CascadeResult getDefaultFaceCascade() noexcept {
        // AUTOSAR: Use constexpr array for candidate paths
        static constexpr std::array<const char*, 3U> FACE_CASCADE_PATHS = {
            "/usr/share/opencv4/haarcascades/haarcascade_frontalface_default.xml",
            "/usr/share/opencv/haarcascades/haarcascade_frontalface_default.xml",
            "/usr/local/share/opencv4/haarcascades/haarcascade_frontalface_default.xml"
        };

        return findFirstExistingPath(FACE_CASCADE_PATHS);
    }

    CascadeResult getDefaultEyeCascade() noexcept {
        // AUTOSAR: Use constexpr array for candidate paths
        static constexpr std::array<const char*, 3U> EYE_CASCADE_PATHS = {
            "/usr/share/opencv4/haarcascades/haarcascade_eye_tree_eyeglasses.xml",
            "/usr/share/opencv/haarcascades/haarcascade_eye_tree_eyeglasses.xml",
            "/usr/local/share/opencv4/haarcascades/haarcascade_eye_tree_eyeglasses.xml"
        };

        return findFirstExistingPath(EYE_CASCADE_PATHS);
    }

    CascadeError validateCascadeFile(const std::string& cascadePath) noexcept {
        if (cascadePath.empty()) {
            return CascadeError::InvalidPath;
        }

        // AUTOSAR: Explicit error handling without exceptions
        std::error_code ec;

        // Check if path exists
        const bool exists = fs::exists(cascadePath, ec);
        if (ec) {
            return CascadeError::UnknownError;
        }

        if (!exists) {
            return CascadeError::FileNotFound;
        }

        // Check if it's a regular file
        const bool isFile = fs::is_regular_file(cascadePath, ec);
        if (ec) {
            return CascadeError::UnknownError;
        }

        if (!isFile) {
            return CascadeError::InvalidPath;
        }

        // Check file permissions (read access)
        const auto perms = fs::status(cascadePath, ec).permissions();
        if (ec) {
            return CascadeError::AccessDenied;
        }

        const bool canRead = (perms & fs::perms::owner_read) != fs::perms::none ||
                           (perms & fs::perms::group_read) != fs::perms::none ||
                           (perms & fs::perms::others_read) != fs::perms::none;

        if (!canRead) {
            return CascadeError::AccessDenied;
        }

        // Basic file size validation (XML files should have minimum size)
        const std::uintmax_t fileSize = fs::file_size(cascadePath, ec);
        if (ec) {
            return CascadeError::UnknownError;
        }

        // AUTOSAR: Use explicit constants
        static constexpr std::uintmax_t MIN_CASCADE_FILE_SIZE = 100U;
        if (fileSize < MIN_CASCADE_FILE_SIZE) {
            return CascadeError::CorruptedFile;
        }

        return CascadeError::Success;
    }

    // AUTOSAR: Legacy interface implementations for backward compatibility
    // These should be replaced with AUTOSAR compliant versions
    std::string defaultFaceCascade() {
        const CascadeResult result = getDefaultFaceCascade();
        if (!result.isSuccess()) {
            throw std::runtime_error(
                "Cannot find haarcascade_frontalface_default.xml. "
                "Pass --face-cascade PATH. Error: " + cascadeErrorToString(result.error)
            );
        }
        return result.path;
    }

    std::string defaultEyeCascade() {
        const CascadeResult result = getDefaultEyeCascade();
        if (!result.isSuccess()) {
            throw std::runtime_error(
                "Cannot find haarcascade_eye_tree_eyeglasses.xml. "
                "Pass --eye-cascade PATH. Error: " + cascadeErrorToString(result.error)
            );
        }
        return result.path;
    }

} // namespace drowsiness_detection

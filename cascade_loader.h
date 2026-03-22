/**
 * @file cascade_loader.h
 * @brief AUTOSAR compliant OpenCV cascade loader interface
 * @version 1.0
 * @date 2026-03-22
 */

#pragma once

#include <cstdint>
#include <string>

// AUTOSAR: Use explicit namespace
namespace drowsiness_detection {

    /**
     * @brief Error codes for cascade loading operations
     */
    enum class CascadeError : std::uint8_t {
        Success = 0U,
        FileNotFound = 1U,
        InvalidPath = 2U,
        AccessDenied = 3U,
        CorruptedFile = 4U,
        UnknownError = 5U
    };

    /**
     * @brief Result structure for cascade path operations
     */
    struct CascadeResult final {
        std::string path{};
        CascadeError error{CascadeError::Success};

        // AUTOSAR: Explicit default operations
        CascadeResult() noexcept = default;
        CascadeResult(const CascadeResult&) = default;
        CascadeResult& operator=(const CascadeResult&) = default;
        CascadeResult(CascadeResult&&) noexcept = default;
        CascadeResult& operator=(CascadeResult&&) noexcept = default;
        ~CascadeResult() = default;

        /**
         * @brief Constructor with parameters
         * @param cascadePath Path to cascade file
         * @param errorCode Error code
         */
        CascadeResult(std::string cascadePath, CascadeError errorCode) noexcept
            : path{std::move(cascadePath)}, error{errorCode} {}

        /**
         * @brief Check if result indicates success
         * @return True if operation was successful
         */
        bool isSuccess() const noexcept {
            return (error == CascadeError::Success);
        }
    };

    /**
     * @brief Convert cascade error to string representation
     * @param error The error code to convert
     * @return String representation of the error
     * @note AUTOSAR compliant - no exceptions, pure function
     */
    std::string cascadeErrorToString(CascadeError error) noexcept;

    /**
     * @brief Get default face cascade file path
     * @return CascadeResult containing path and error status
     * @note AUTOSAR compliant - no exceptions, explicit error handling
     */
    CascadeResult getDefaultFaceCascade() noexcept;

    /**
     * @brief Get default eye cascade file path
     * @return CascadeResult containing path and error status
     * @note AUTOSAR compliant - no exceptions, explicit error handling
     */
    CascadeResult getDefaultEyeCascade() noexcept;

    /**
     * @brief Validate cascade file exists and is accessible
     * @param cascadePath Path to validate
     * @return CascadeError indicating validation result
     * @note AUTOSAR compliant - explicit validation without exceptions
     */
    CascadeError validateCascadeFile(const std::string& cascadePath) noexcept;

    /**
     * @brief Legacy interface functions for backward compatibility
     * @note These functions throw std::runtime_error on failure for compatibility
     * @deprecated Use CascadeResult-based functions for AUTOSAR compliance
     */
    [[deprecated("Use getDefaultFaceCascade() for AUTOSAR compliance")]]
    std::string defaultFaceCascade();

    [[deprecated("Use getDefaultEyeCascade() for AUTOSAR compliance")]]
    std::string defaultEyeCascade();

} // namespace drowsiness_detection

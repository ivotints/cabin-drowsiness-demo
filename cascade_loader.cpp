#include "cascade_loader.h"

#include <filesystem>
#include <optional>
#include <stdexcept>
#include <vector>

namespace fs = std::filesystem;

static std::optional<std::string> firstExistingPath(const std::vector<std::string>& candidates) {
    for (const auto& p : candidates) {
        if (!p.empty() && fs::exists(p)) {
            return p;
        }
    }
    return std::nullopt;
}

std::string defaultFaceCascade() {
    const auto path = firstExistingPath({
        "/usr/share/opencv4/haarcascades/haarcascade_frontalface_default.xml",
        "/usr/share/opencv/haarcascades/haarcascade_frontalface_default.xml",
        "/usr/local/share/opencv4/haarcascades/haarcascade_frontalface_default.xml"
    });
    if (!path) {
        throw std::runtime_error(
            "Cannot find haarcascade_frontalface_default.xml. "
            "Pass --face-cascade PATH.");
    }
    return *path;
}

std::string defaultEyeCascade() {
    const auto path = firstExistingPath({
        "/usr/share/opencv4/haarcascades/haarcascade_eye_tree_eyeglasses.xml",
        "/usr/share/opencv/haarcascades/haarcascade_eye_tree_eyeglasses.xml",
        "/usr/local/share/opencv4/haarcascades/haarcascade_eye_tree_eyeglasses.xml"
    });
    if (!path) {
        throw std::runtime_error(
            "Cannot find haarcascade_eye_tree_eyeglasses.xml. "
            "Pass --eye-cascade PATH.");
    }
    return *path;
}

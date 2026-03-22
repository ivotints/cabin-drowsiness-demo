#include "alert_sound.h"

#include <filesystem>
#include <iostream>
#include <optional>
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

static std::string findAudioPlayer() {
    const std::vector<std::string> candidates = {"aplay", "paplay", "play"};
    for (const auto& name : candidates) {
        const std::string cmd = "command -v " + name + " > /dev/null 2>&1";
        if (std::system(cmd.c_str()) == 0) {
            return name;
        }
    }
    return "";
}

std::string defaultAlertSoundPath() {
    const auto path = firstExistingPath({
        "assets/beep-01a.wav",
        "../assets/beep-01a.wav",
        "/usr/share/cabin_drowsiness_demo/assets/beep-01a.wav"
    });
    if (!path) {
        throw std::runtime_error(
            "Cannot find alert sound file beep-01a.wav. "
            "Place it in assets/ or pass the path by setting AUDIO_PATH environment variable.");
    }
    return *path;
}

void playAlertSound(const std::string& audioPath) {
    if (!fs::exists(audioPath)) {
        std::cerr << "Warning: alert sound not found: " << audioPath << "\n";
        return;
    }

    const auto player = findAudioPlayer();
    if (player.empty()) {
        std::cerr << "Warning: no audio player found (aplay/paplay/play). Install one to enable alert sound.\n";
        return;
    }

    std::string cmd;
    if (player == "aplay") {
        cmd = "aplay -q \"" + audioPath + "\" &";
    } else if (player == "paplay") {
        cmd = "paplay \"" + audioPath + "\" &";
    } else {
        cmd = "play -q \"" + audioPath + "\" &";
    }

    std::system(cmd.c_str());
}

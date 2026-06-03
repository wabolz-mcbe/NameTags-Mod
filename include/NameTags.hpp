#pragma once

#include "beatsaber-hook/shared/utils/logging.hpp"
#include "beatsaber-hook/shared/utils/hooking.hpp"
#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"
#include "beatsaber-hook/shared/config/config-utils.hpp"

// Unity / UnityEngine types
#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/Transform.hpp"
#include "UnityEngine/Vector3.hpp"
#include "UnityEngine/Color.hpp"
#include "UnityEngine/Canvas.hpp"
#include "UnityEngine/UI/Text.hpp"
#include "UnityEngine/Camera.hpp"
#include "UnityEngine/XR/XRDevice.hpp"

// Gorilla Tag types
#include "GorillaTag/VRRig.hpp"
#include "GorillaTag/PhotonNetworkController.hpp"
#include "Photon/Realtime/Player.hpp"

// Standard
#include <string>
#include <unordered_map>
#include <chrono>

static ModInfo modInfo;

// Logger helper
static Logger& getLogger() {
    static Logger* logger = new Logger(modInfo, LoggerOptions(false, true));
    return *logger;
}

// Platform enum
enum class Platform {
    Quest,
    Steam,
    Unknown
};

// Per-player nametag data
struct NametagData {
    UnityEngine::GameObject* canvasObj  = nullptr;
    UnityEngine::UI::Text*   nameText   = nullptr;
    UnityEngine::UI::Text*   statsText  = nullptr;
    long long                lastPingMs = 0;
    int                      lastFps    = 0;
    int                      lastHz     = 0;
    Platform                 platform   = Platform::Unknown;
    std::string              username;
};

// Map from VRRig pointer → nametag data
static std::unordered_map<void*, NametagData> nametagMap;

// ── Color helpers ────────────────────────────────────────────────────────────

// Good → yellow → bad thresholds
static UnityEngine::Color colorForPing(long long ms) {
    if (ms < 60)  return {0.2f, 1.0f, 0.2f, 1.0f};   // green
    if (ms < 120) return {1.0f, 1.0f, 0.0f, 1.0f};   // yellow
    if (ms < 200) return {1.0f, 0.5f, 0.0f, 1.0f};   // orange
    return              {1.0f, 0.2f, 0.2f, 1.0f};    // red
}

static UnityEngine::Color colorForFps(int fps) {
    if (fps >= 72) return {0.2f, 1.0f, 0.2f, 1.0f};
    if (fps >= 45) return {1.0f, 1.0f, 0.0f, 1.0f};
    if (fps >= 30) return {1.0f, 0.5f, 0.0f, 1.0f};
    return              {1.0f, 0.2f, 0.2f, 1.0f};
}

static UnityEngine::Color colorForHz(int hz) {
    if (hz >= 90) return {0.4f, 0.8f, 1.0f, 1.0f};   // cyan  – high refresh
    if (hz >= 72) return {0.2f, 1.0f, 0.2f, 1.0f};   // green
    return              {1.0f, 1.0f, 0.0f, 1.0f};    // yellow
}

static UnityEngine::Color colorForPlatform(Platform p) {
    switch (p) {
        case Platform::Quest: return {0.4f, 0.6f, 1.0f, 1.0f};  // Meta blue
        case Platform::Steam: return {0.4f, 0.8f, 0.6f, 1.0f};  // Steam teal
        default:              return {0.8f, 0.8f, 0.8f, 1.0f};  // grey
    }
}

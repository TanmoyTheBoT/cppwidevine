#pragma once

#include <cstdint>

namespace widevine {

/**
 * Widevine Device Types
 */
enum class DeviceType {
    CHROME = 1,
    ANDROID = 2
};

/**
 * Widevine License Types
 */
enum class LicenseType {
    STREAMING = 1,  // Normal one-time-use license
    OFFLINE = 2,    // Offline-use license for downloaded content
    AUTOMATIC = 3   // License type decision left to provider
};

} // namespace widevine

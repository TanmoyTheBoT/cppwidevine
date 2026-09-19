#pragma once

#include <widevine/types.h>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace widevine {

/**
 * Widevine Device
 *
 * Represents a Widevine device with credentials (private key and client ID).
 * Devices can be loaded from .wvd files (Widevine Device format).
 *
 * WVD Format:
 * - Magic: "WVD\0" (4 bytes)
 * - Version: 1 or 2 (uint8_t)
 * - Type: CHROME=1, ANDROID=2 (uint8_t)
 * - Security Level: L1=1, L3=3 (uint8_t)
 * - Flags: Reserved (uint8_t)
 * - Private Key Length: (uint16_t)
 * - Private Key: RSA private key in PKCS#8 DER format
 * - Client ID Length: (uint16_t)
 * - Client ID: Device certificate (DER encoded)
 */
class Device {
public:
    /**
     * Load device from .wvd file
     * @param path Path to .wvd file
     * @return Device instance
     * @throws std::runtime_error if file cannot be loaded or parsed
     */
    static Device from_wvd(const std::string& path);

    /**
     * Parse device from .wvd file data
     * @param data Raw .wvd file bytes
     * @return Device instance
     * @throws std::runtime_error if data is invalid
     */
    static Device from_wvd_data(const std::vector<uint8_t>& data);

    /**
     * Create device from components
     * @param type Device type (CHROME or ANDROID)
     * @param security_level Security level (1=L1, 3=L3)
     * @param private_key RSA private key (PKCS#8 DER format)
     * @param client_id Client identification blob (DER encoded certificate)
     */
    Device(
        DeviceType type,
        uint8_t security_level,
        std::vector<uint8_t> private_key,
        std::vector<uint8_t> client_id
    );

    ~Device();

    // Allow copy and move
    Device(const Device& other);
    Device& operator=(const Device& other);
    Device(Device&& other) noexcept;
    Device& operator=(Device&& other) noexcept;

    /**
     * Get device type
     */
    DeviceType type() const;

    /**
     * Get security level (1=L1, 3=L3)
     */
    uint8_t security_level() const;

    /**
     * Get private key (PKCS#8 DER format)
     */
    const std::vector<uint8_t>& private_key() const;

    /**
     * Get client ID blob (DER encoded certificate)
     */
    const std::vector<uint8_t>& client_id() const;

    /**
     * Get system ID from client certificate
     * Returns the Widevine system ID (16 bytes)
     */
    std::vector<uint8_t> system_id() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace widevine

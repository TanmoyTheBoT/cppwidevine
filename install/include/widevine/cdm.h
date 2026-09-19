#pragma once

#include <widevine/types.h>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace widevine {

// Forward declarations
class Device;
class PSSH;
class Session;

/**
 * Key information extracted from license
 */
struct Key {
    std::vector<uint8_t> kid;      // Key ID
    std::vector<uint8_t> key;      // Key value (decrypted)
    std::string type;               // "CONTENT", "SIGNING", etc.

    // Helper to get hex string representations
    std::string kid_hex() const;
    std::string key_hex() const;
};

/**
 * Widevine CDM (Content Decryption Module)
 *
 * This is the main interface for interacting with Widevine DRM.
 *
 * Basic usage:
 * 1. Load a device from .wvd file
 * 2. Create a session
 * 3. Get license challenge from PSSH
 * 4. Send challenge to license server
 * 5. Parse license response to extract keys
 *
 * Example:
 *   auto device = Device::from_wvd("device.wvd");
 *   CDM cdm(device);
 *
 *   auto session = cdm.open_session();
 *   auto pssh = PSSH::from_base64(pssh_b64);
 *   auto challenge = cdm.get_license_challenge(session, pssh);
 *
 *   // Send challenge to server, get response
 *   cdm.parse_license(session, license_response);
 *   auto keys = cdm.get_keys(session);
 */
class CDM {
public:
    /**
     * Create a CDM instance with a device
     */
    explicit CDM(const Device& device);
    ~CDM();

    // No copy
    CDM(const CDM&) = delete;
    CDM& operator=(const CDM&) = delete;

    /**
     * Open a new session
     * Returns a session ID that must be used for all subsequent operations
     */
    std::vector<uint8_t> open_session();

    /**
     * Close a session and free resources
     */
    void close_session(const std::vector<uint8_t>& session_id);

    /**
     * Set a service certificate for privacy mode
     * Optional - enables encrypted client ID in license requests
     */
    void set_service_certificate(
        const std::vector<uint8_t>& session_id,
        const std::vector<uint8_t>& certificate
    );

    /**
     * Generate a license challenge (request) to send to license server
     *
     * @param session_id Session identifier from open_session()
     * @param pssh PSSH object containing init data
     * @param license_type Type of license (STREAMING, OFFLINE, AUTOMATIC)
     * @param privacy_mode Encrypt client ID using service certificate
     * @return Signed license request bytes to send to server
     */
    std::vector<uint8_t> get_license_challenge(
        const std::vector<uint8_t>& session_id,
        const PSSH& pssh,
        LicenseType license_type = LicenseType::STREAMING,
        bool privacy_mode = true
    );

    /**
     * Parse license response from server and extract keys
     *
     * @param session_id Session identifier
     * @param license_response Raw bytes from license server response
     */
    void parse_license(
        const std::vector<uint8_t>& session_id,
        const std::vector<uint8_t>& license_response
    );

    /**
     * Get decrypted keys from parsed license
     *
     * @param session_id Session identifier
     * @param type Optional key type filter ("CONTENT", "SIGNING", etc.)
     * @return List of decrypted keys
     */
    std::vector<Key> get_keys(
        const std::vector<uint8_t>& session_id,
        const std::string& type = ""
    ) const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace widevine

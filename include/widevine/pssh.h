#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace widevine {

/**
 * PSSH (Protection System Specific Header)
 *
 * Parses and extracts Widevine-specific init data from PSSH boxes.
 *
 * PSSH Box Structure:
 * - Size (4 bytes, big-endian)
 * - Type: 'pssh' (4 bytes)
 * - Version (1 byte): 0 or 1
 * - Flags (3 bytes)
 * - System ID (16 bytes): Widevine = edef8ba979d64acea3c827dcd51d21ed
 * - [Version 1 only] KID count (4 bytes) + KIDs (16 bytes each)
 * - Data size (4 bytes)
 * - Data (N bytes): Widevine init data
 */
class PSSH {
public:
    /**
     * Parse PSSH from base64 string
     * @param base64 Base64-encoded PSSH box
     * @return PSSH instance
     * @throws std::runtime_error if parsing fails
     */
    static PSSH from_base64(const std::string& base64);

    /**
     * Parse PSSH from raw bytes
     * @param data Raw PSSH box bytes
     * @return PSSH instance
     * @throws std::runtime_error if parsing fails
     */
    static PSSH from_bytes(const std::vector<uint8_t>& data);

    ~PSSH();

    // Allow copy and move
    PSSH(const PSSH& other);
    PSSH& operator=(const PSSH& other);
    PSSH(PSSH&& other) noexcept;
    PSSH& operator=(PSSH&& other) noexcept;

    /**
     * Get PSSH version (0 or 1)
     */
    uint8_t version() const;

    /**
     * Get system ID (16 bytes)
     * For Widevine: edef8ba979d64acea3c827dcd51d21ed
     */
    std::vector<uint8_t> system_id() const;

    /**
     * Get key IDs (only for version 1 PSSH)
     */
    std::vector<std::vector<uint8_t>> key_ids() const;

    /**
     * Get init data (Widevine-specific data from PSSH)
     * This is what gets sent in the license request
     */
    std::vector<uint8_t> init_data() const;

    /**
     * Get original PSSH box bytes
     */
    std::vector<uint8_t> raw() const;

    /**
     * Check if this is a Widevine PSSH
     * Returns true if system ID matches Widevine
     */
    bool is_widevine() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;

    explicit PSSH(std::unique_ptr<Impl> impl);
};

} // namespace widevine

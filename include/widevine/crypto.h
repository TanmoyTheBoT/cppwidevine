#pragma once

#include <vector>
#include <cstdint>
#include <string>
#include <memory>

namespace widevine {
namespace crypto {

/**
 * RSA Operations
 */
class RSA {
public:
    /**
     * Load RSA private key from PKCS#8 DER format
     */
    static std::unique_ptr<RSA> from_pkcs8_der(const std::vector<uint8_t>& der);

    ~RSA();

    /**
     * Sign data using RSA-PSS with SHA1
     * Used for signing license requests
     */
    std::vector<uint8_t> sign_pss_sha1(const std::vector<uint8_t>& data) const;

    /**
     * Decrypt data using RSA-OAEP with SHA1
     * Used for decrypting session keys from license response
     */
    std::vector<uint8_t> decrypt_oaep_sha1(const std::vector<uint8_t>& ciphertext) const;

    /**
     * Get public key in DER format
     */
    std::vector<uint8_t> public_key_der() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;

    explicit RSA(std::unique_ptr<Impl> impl);
};

/**
 * HMAC-SHA256
 */
std::vector<uint8_t> hmac_sha256(
    const std::vector<uint8_t>& key,
    const std::vector<uint8_t>& data
);

/**
 * SHA1 hash
 */
std::vector<uint8_t> sha1(const std::vector<uint8_t>& data);

/**
 * SHA256 hash
 */
std::vector<uint8_t> sha256(const std::vector<uint8_t>& data);

/**
 * AES-128-CBC Decryption
 */
std::vector<uint8_t> aes_cbc_decrypt(
    const std::vector<uint8_t>& key,
    const std::vector<uint8_t>& iv,
    const std::vector<uint8_t>& ciphertext
);

/**
 * AES-128-CBC Encryption
 */
std::vector<uint8_t> aes_cbc_encrypt(
    const std::vector<uint8_t>& key,
    const std::vector<uint8_t>& iv,
    const std::vector<uint8_t>& plaintext
);

/**
 * CMAC-AES key derivation (used for deriving session keys)
 */
std::vector<uint8_t> cmac_aes(
    const std::vector<uint8_t>& key,
    const std::vector<uint8_t>& data
);

/**
 * Generate random bytes
 */
std::vector<uint8_t> random_bytes(size_t length);

} // namespace crypto
} // namespace widevine

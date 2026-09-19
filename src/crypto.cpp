#include <widevine/crypto.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/sha.h>
#include <openssl/rand.h>
#include <openssl/aes.h>
#include <openssl/cmac.h>
#include <stdexcept>
#include <cstring>

namespace widevine {
namespace crypto {

// RSA Implementation
class RSA::Impl {
public:
    EVP_PKEY* pkey{nullptr};

    explicit Impl(EVP_PKEY* key) : pkey(key) {}

    ~Impl() {
        if (pkey) EVP_PKEY_free(pkey);
    }
};

std::unique_ptr<RSA> RSA::from_pkcs8_der(const std::vector<uint8_t>& der) {
    const unsigned char* data = der.data();
    EVP_PKEY* pkey = d2i_PrivateKey(EVP_PKEY_RSA, nullptr, &data, der.size());

    if (!pkey) {
        throw std::runtime_error("Failed to load RSA private key from PKCS#8 DER");
    }

    return std::unique_ptr<RSA>(new RSA(std::make_unique<Impl>(pkey)));
}

RSA::RSA(std::unique_ptr<Impl> impl) : impl_(std::move(impl)) {}

RSA::~RSA() = default;

std::vector<uint8_t> RSA::sign_pss_sha1(const std::vector<uint8_t>& data) const {
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) {
        throw std::runtime_error("Failed to create EVP context");
    }

    EVP_PKEY_CTX* pkey_ctx = nullptr;

    // Initialize signing with SHA1
    if (EVP_DigestSignInit(ctx, &pkey_ctx, EVP_sha1(), nullptr, impl_->pkey) != 1) {
        EVP_MD_CTX_free(ctx);
        throw std::runtime_error("Failed to initialize signing");
    }

    // Set RSA-PSS padding
    if (EVP_PKEY_CTX_set_rsa_padding(pkey_ctx, RSA_PKCS1_PSS_PADDING) != 1) {
        EVP_MD_CTX_free(ctx);
        throw std::runtime_error("Failed to set PSS padding");
    }

    // Set salt length to match hash length (SHA1 = 20 bytes)
    if (EVP_PKEY_CTX_set_rsa_pss_saltlen(pkey_ctx, 20) != 1) {
        EVP_MD_CTX_free(ctx);
        throw std::runtime_error("Failed to set salt length");
    }

    // Update with data
    if (EVP_DigestSignUpdate(ctx, data.data(), data.size()) != 1) {
        EVP_MD_CTX_free(ctx);
        throw std::runtime_error("Failed to update digest");
    }

    // Get signature length
    size_t sig_len = 0;
    if (EVP_DigestSignFinal(ctx, nullptr, &sig_len) != 1) {
        EVP_MD_CTX_free(ctx);
        throw std::runtime_error("Failed to get signature length");
    }

    // Get signature
    std::vector<uint8_t> signature(sig_len);
    if (EVP_DigestSignFinal(ctx, signature.data(), &sig_len) != 1) {
        EVP_MD_CTX_free(ctx);
        throw std::runtime_error("Failed to sign");
    }

    EVP_MD_CTX_free(ctx);
    signature.resize(sig_len);
    return signature;
}

std::vector<uint8_t> RSA::decrypt_oaep_sha1(const std::vector<uint8_t>& ciphertext) const {
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new(impl_->pkey, nullptr);
    if (!ctx) {
        throw std::runtime_error("Failed to create PKEY context");
    }

    if (EVP_PKEY_decrypt_init(ctx) != 1) {
        EVP_PKEY_CTX_free(ctx);
        throw std::runtime_error("Failed to initialize decryption");
    }

    // Set RSA-OAEP padding
    if (EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_OAEP_PADDING) != 1) {
        EVP_PKEY_CTX_free(ctx);
        throw std::runtime_error("Failed to set OAEP padding");
    }

    // Set OAEP hash to SHA1
    if (EVP_PKEY_CTX_set_rsa_oaep_md(ctx, EVP_sha1()) != 1) {
        EVP_PKEY_CTX_free(ctx);
        throw std::runtime_error("Failed to set OAEP hash");
    }

    // Set MGF1 hash to SHA1
    if (EVP_PKEY_CTX_set_rsa_mgf1_md(ctx, EVP_sha1()) != 1) {
        EVP_PKEY_CTX_free(ctx);
        throw std::runtime_error("Failed to set MGF1 hash");
    }

    // Get output length
    size_t out_len = 0;
    if (EVP_PKEY_decrypt(ctx, nullptr, &out_len, ciphertext.data(), ciphertext.size()) != 1) {
        EVP_PKEY_CTX_free(ctx);
        throw std::runtime_error("Failed to get output length");
    }

    // Decrypt
    std::vector<uint8_t> plaintext(out_len);
    if (EVP_PKEY_decrypt(ctx, plaintext.data(), &out_len, ciphertext.data(), ciphertext.size()) != 1) {
        EVP_PKEY_CTX_free(ctx);
        throw std::runtime_error("Failed to decrypt");
    }

    EVP_PKEY_CTX_free(ctx);
    plaintext.resize(out_len);
    return plaintext;
}

std::vector<uint8_t> RSA::public_key_der() const {
    unsigned char* der = nullptr;
    int len = i2d_PUBKEY(impl_->pkey, &der);

    if (len < 0) {
        throw std::runtime_error("Failed to encode public key");
    }

    std::vector<uint8_t> result(der, der + len);
    OPENSSL_free(der);
    return result;
}

// Hash functions
std::vector<uint8_t> sha1(const std::vector<uint8_t>& data) {
    std::vector<uint8_t> hash(SHA_DIGEST_LENGTH);
    SHA1(data.data(), data.size(), hash.data());
    return hash;
}

std::vector<uint8_t> sha256(const std::vector<uint8_t>& data) {
    std::vector<uint8_t> hash(SHA256_DIGEST_LENGTH);
    SHA256(data.data(), data.size(), hash.data());
    return hash;
}

// HMAC
std::vector<uint8_t> hmac_sha256(
    const std::vector<uint8_t>& key,
    const std::vector<uint8_t>& data
) {
    std::vector<uint8_t> result(SHA256_DIGEST_LENGTH);
    unsigned int len = 0;

    HMAC(EVP_sha256(), key.data(), key.size(),
         data.data(), data.size(),
         result.data(), &len);

    result.resize(len);
    return result;
}

// AES CBC
std::vector<uint8_t> aes_cbc_decrypt(
    const std::vector<uint8_t>& key,
    const std::vector<uint8_t>& iv,
    const std::vector<uint8_t>& ciphertext
) {
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        throw std::runtime_error("Failed to create cipher context");
    }

    if (EVP_DecryptInit_ex(ctx, EVP_aes_128_cbc(), nullptr, key.data(), iv.data()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Failed to initialize decryption");
    }

    std::vector<uint8_t> plaintext(ciphertext.size() + AES_BLOCK_SIZE);
    int len = 0;
    int plaintext_len = 0;

    if (EVP_DecryptUpdate(ctx, plaintext.data(), &len, ciphertext.data(), ciphertext.size()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Failed to decrypt");
    }
    plaintext_len = len;

    if (EVP_DecryptFinal_ex(ctx, plaintext.data() + len, &len) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Failed to finalize decryption");
    }
    plaintext_len += len;

    EVP_CIPHER_CTX_free(ctx);
    plaintext.resize(plaintext_len);
    return plaintext;
}

std::vector<uint8_t> aes_cbc_encrypt(
    const std::vector<uint8_t>& key,
    const std::vector<uint8_t>& iv,
    const std::vector<uint8_t>& plaintext
) {
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        throw std::runtime_error("Failed to create cipher context");
    }

    if (EVP_EncryptInit_ex(ctx, EVP_aes_128_cbc(), nullptr, key.data(), iv.data()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Failed to initialize encryption");
    }

    std::vector<uint8_t> ciphertext(plaintext.size() + AES_BLOCK_SIZE);
    int len = 0;
    int ciphertext_len = 0;

    if (EVP_EncryptUpdate(ctx, ciphertext.data(), &len, plaintext.data(), plaintext.size()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Failed to encrypt");
    }
    ciphertext_len = len;

    if (EVP_EncryptFinal_ex(ctx, ciphertext.data() + len, &len) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Failed to finalize encryption");
    }
    ciphertext_len += len;

    EVP_CIPHER_CTX_free(ctx);
    ciphertext.resize(ciphertext_len);
    return ciphertext;
}

// CMAC-AES
std::vector<uint8_t> cmac_aes(
    const std::vector<uint8_t>& key,
    const std::vector<uint8_t>& data
) {
    CMAC_CTX* ctx = CMAC_CTX_new();
    if (!ctx) {
        throw std::runtime_error("Failed to create CMAC context");
    }

    if (CMAC_Init(ctx, key.data(), key.size(), EVP_aes_128_cbc(), nullptr) != 1) {
        CMAC_CTX_free(ctx);
        throw std::runtime_error("Failed to initialize CMAC");
    }

    if (CMAC_Update(ctx, data.data(), data.size()) != 1) {
        CMAC_CTX_free(ctx);
        throw std::runtime_error("Failed to update CMAC");
    }

    std::vector<uint8_t> result(16); // AES-128 output
    size_t len = 0;

    if (CMAC_Final(ctx, result.data(), &len) != 1) {
        CMAC_CTX_free(ctx);
        throw std::runtime_error("Failed to finalize CMAC");
    }

    CMAC_CTX_free(ctx);
    result.resize(len);
    return result;
}

// Random bytes
std::vector<uint8_t> random_bytes(size_t length) {
    std::vector<uint8_t> result(length);
    if (RAND_bytes(result.data(), length) != 1) {
        throw std::runtime_error("Failed to generate random bytes");
    }
    return result;
}

} // namespace crypto
} // namespace widevine

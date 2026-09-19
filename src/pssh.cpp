#include <widevine/pssh.h>
#include <stdexcept>
#include <cstring>
#include <sstream>
#include <iomanip>

namespace widevine {

// Base64 decoding
static std::vector<uint8_t> base64_decode(const std::string& input) {
    static const std::string base64_chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    std::vector<uint8_t> result;
    std::vector<int> T(256, -1);
    for (int i = 0; i < 64; i++) T[base64_chars[i]] = i;

    int val = 0, valb = -8;
    for (unsigned char c : input) {
        if (T[c] == -1) break;
        val = (val << 6) + T[c];
        valb += 6;
        if (valb >= 0) {
            result.push_back((val >> valb) & 0xFF);
            valb -= 8;
        }
    }
    return result;
}

static uint32_t read_be32(const uint8_t* data) {
    return (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];
}

class PSSH::Impl {
public:
    std::vector<uint8_t> raw_data_;
    uint8_t version_{0};
    std::vector<uint8_t> system_id_;
    std::vector<std::vector<uint8_t>> key_ids_;
    std::vector<uint8_t> init_data_;

    explicit Impl(const std::vector<uint8_t>& data) : raw_data_(data) {
        parse();
    }

    void parse() {
        if (raw_data_.size() < 32) {
            throw std::runtime_error("PSSH too small");
        }

        size_t offset = 0;

        // Size (skip, we have the data)
        offset += 4;

        // Type 'pssh'
        if (raw_data_[offset] != 'p' || raw_data_[offset+1] != 's' ||
            raw_data_[offset+2] != 's' || raw_data_[offset+3] != 'h') {
            throw std::runtime_error("Invalid PSSH magic");
        }
        offset += 4;

        // Version
        version_ = raw_data_[offset++];

        // Flags (skip 3 bytes)
        offset += 3;

        // System ID (16 bytes)
        system_id_.assign(raw_data_.begin() + offset,
                         raw_data_.begin() + offset + 16);
        offset += 16;

        if (version_ == 1) {
            // KID count
            if (offset + 4 > raw_data_.size()) {
                throw std::runtime_error("Truncated PSSH (KID count)");
            }
            uint32_t kid_count = read_be32(&raw_data_[offset]);
            offset += 4;

            // Read KIDs
            for (uint32_t i = 0; i < kid_count; i++) {
                if (offset + 16 > raw_data_.size()) {
                    throw std::runtime_error("Truncated PSSH (KID data)");
                }
                key_ids_.emplace_back(raw_data_.begin() + offset,
                                     raw_data_.begin() + offset + 16);
                offset += 16;
            }
        }

        // Data size
        if (offset + 4 > raw_data_.size()) {
            throw std::runtime_error("Truncated PSSH (data size)");
        }
        uint32_t data_size = read_be32(&raw_data_[offset]);
        offset += 4;

        // Init data
        if (offset + data_size > raw_data_.size()) {
            throw std::runtime_error("Truncated PSSH (init data)");
        }
        init_data_.assign(raw_data_.begin() + offset,
                         raw_data_.begin() + offset + data_size);
    }
};

PSSH::PSSH(std::unique_ptr<Impl> impl) : impl_(std::move(impl)) {}

PSSH PSSH::from_base64(const std::string& base64) {
    auto data = base64_decode(base64);
    return from_bytes(data);
}

PSSH PSSH::from_bytes(const std::vector<uint8_t>& data) {
    return PSSH(std::make_unique<Impl>(data));
}

PSSH::~PSSH() = default;

PSSH::PSSH(const PSSH& other)
    : impl_(std::make_unique<Impl>(*other.impl_)) {}

PSSH& PSSH::operator=(const PSSH& other) {
    if (this != &other) {
        impl_ = std::make_unique<Impl>(*other.impl_);
    }
    return *this;
}

PSSH::PSSH(PSSH&& other) noexcept = default;
PSSH& PSSH::operator=(PSSH&& other) noexcept = default;

uint8_t PSSH::version() const {
    return impl_->version_;
}

std::vector<uint8_t> PSSH::system_id() const {
    return impl_->system_id_;
}

std::vector<std::vector<uint8_t>> PSSH::key_ids() const {
    return impl_->key_ids_;
}

std::vector<uint8_t> PSSH::init_data() const {
    return impl_->init_data_;
}

std::vector<uint8_t> PSSH::raw() const {
    return impl_->raw_data_;
}

bool PSSH::is_widevine() const {
    // Widevine system ID: edef8ba979d64acea3c827dcd51d21ed
    static const uint8_t widevine_id[] = {
        0xed, 0xef, 0x8b, 0xa9, 0x79, 0xd6, 0x4a, 0xce,
        0xa3, 0xc8, 0x27, 0xdc, 0xd5, 0x1d, 0x21, 0xed
    };

    if (impl_->system_id_.size() != 16) {
        return false;
    }

    return std::memcmp(impl_->system_id_.data(), widevine_id, 16) == 0;
}

} // namespace widevine

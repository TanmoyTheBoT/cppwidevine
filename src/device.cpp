#include <widevine/device.h>
#include <widevine/cdm.h>
#include <fstream>
#include <stdexcept>
#include <cstring>

namespace widevine {

class Device::Impl {
public:
    DeviceType type_;
    uint8_t security_level_;
    std::vector<uint8_t> private_key_;
    std::vector<uint8_t> client_id_;

    Impl(DeviceType type, uint8_t security_level,
         std::vector<uint8_t> private_key, std::vector<uint8_t> client_id)
        : type_(type), security_level_(security_level),
          private_key_(std::move(private_key)), client_id_(std::move(client_id)) {}
};

// WVD file format (version 2):
// Offset  Size  Field
// 0       3     Magic "WVD"
// 3       1     Version (2)
// 4       1     Type (1=CHROME, 2=ANDROID)
// 5       1     Security level
// 6       1     Flags
// 7       1     Reserved padding
// 8       2     Private key length (uint16_t, little-endian)
// 10      N     Private key (PKCS#8 DER)
// 10+N    2     Client ID length (uint16_t, little-endian)
// 12+N    M     Client ID (DER certificate)

static uint16_t read_le16(const uint8_t* data) {
    return data[0] | (data[1] << 8);
}

Device Device::from_wvd_data(const std::vector<uint8_t>& data) {
    if (data.size() < 8) {
        throw std::runtime_error("WVD file too small");
    }

    // Check magic
    if (data[0] != 'W' || data[1] != 'V' || data[2] != 'D') {
        throw std::runtime_error("Invalid WVD magic");
    }

    uint8_t version = data[3];
    if (version != 1 && version != 2) {
        throw std::runtime_error("Unsupported WVD version: " + std::to_string(version));
    }

    size_t offset = 4;

    // Type
    uint8_t type_val = data[offset++];
    DeviceType type;
    if (type_val == 1) {
        type = DeviceType::CHROME;
    } else if (type_val == 2) {
        type = DeviceType::ANDROID;
    } else {
        throw std::runtime_error("Unknown device type: " + std::to_string(type_val));
    }

    // Security level
    uint8_t security_level = data[offset++];

    // Flags (skip)
    offset++;

    // Padding (skip for version 2)
    if (version == 2) {
        offset++;
    }

    // Private key length
    if (offset + 2 > data.size()) {
        throw std::runtime_error("Truncated WVD file (private_key length)");
    }
    uint16_t private_key_len = read_le16(&data[offset]);
    offset += 2;

    // Private key
    if (offset + private_key_len > data.size()) {
        throw std::runtime_error("Truncated WVD file (private_key data)");
    }
    std::vector<uint8_t> private_key(data.begin() + offset,
                                     data.begin() + offset + private_key_len);
    offset += private_key_len;

    // Client ID length
    if (offset + 2 > data.size()) {
        throw std::runtime_error("Truncated WVD file (client_id length)");
    }
    uint16_t client_id_len = read_le16(&data[offset]);
    offset += 2;

    // Client ID
    if (offset + client_id_len > data.size()) {
        throw std::runtime_error("Truncated WVD file (client_id data)");
    }
    std::vector<uint8_t> client_id(data.begin() + offset,
                                   data.begin() + offset + client_id_len);

    return Device(type, security_level, std::move(private_key), std::move(client_id));
}

Device Device::from_wvd(const std::string& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) {
        throw std::runtime_error("Cannot open WVD file: " + path);
    }

    auto size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> data(size);
    if (!file.read(reinterpret_cast<char*>(data.data()), size)) {
        throw std::runtime_error("Failed to read WVD file: " + path);
    }

    return from_wvd_data(data);
}

Device::Device(DeviceType type, uint8_t security_level,
               std::vector<uint8_t> private_key,
               std::vector<uint8_t> client_id)
    : impl_(std::make_unique<Impl>(type, security_level,
                                   std::move(private_key),
                                   std::move(client_id))) {}

Device::~Device() = default;

Device::Device(const Device& other)
    : impl_(std::make_unique<Impl>(*other.impl_)) {}

Device& Device::operator=(const Device& other) {
    if (this != &other) {
        impl_ = std::make_unique<Impl>(*other.impl_);
    }
    return *this;
}

Device::Device(Device&& other) noexcept = default;
Device& Device::operator=(Device&& other) noexcept = default;

DeviceType Device::type() const {
    return impl_->type_;
}

uint8_t Device::security_level() const {
    return impl_->security_level_;
}

const std::vector<uint8_t>& Device::private_key() const {
    return impl_->private_key_;
}

const std::vector<uint8_t>& Device::client_id() const {
    return impl_->client_id_;
}

std::vector<uint8_t> Device::system_id() const {
    // Widevine system ID: edef8ba979d64acea3c827dcd51d21ed
    return {
        0xed, 0xef, 0x8b, 0xa9, 0x79, 0xd6, 0x4a, 0xce,
        0xa3, 0xc8, 0x27, 0xdc, 0xd5, 0x1d, 0x21, 0xed
    };
}

} // namespace widevine

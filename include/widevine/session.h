#pragma once

#include <widevine/cdm.h>
#include <widevine/device.h>
#include <widevine/crypto.h>
#include <vector>
#include <map>
#include <cstdint>

namespace widevine {

// Internal session state
struct SessionState {
    std::vector<uint8_t> id;
    uint32_t number{0};
    std::vector<Key> keys;
    std::vector<uint8_t> service_certificate;

    // Context data for key derivation
    struct Context {
        std::vector<uint8_t> enc_context;
        std::vector<uint8_t> mac_context;
    };
    std::map<std::vector<uint8_t>, Context> contexts; // request_id -> context
};

} // namespace widevine

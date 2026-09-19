#include <widevine/cdm.h>
#include <stdexcept>

namespace widevine {

// Helper for hex conversion
std::string Key::kid_hex() const {
    std::string result;
    for (auto byte : kid) {
        char hex[3];
        snprintf(hex, sizeof(hex), "%02x", byte);
        result += hex;
    }
    return result;
}

std::string Key::key_hex() const {
    std::string result;
    for (auto byte : key) {
        char hex[3];
        snprintf(hex, sizeof(hex), "%02x", byte);
        result += hex;
    }
    return result;
}

} // namespace widevine

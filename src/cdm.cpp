#include <widevine/cdm.h>
#include <stdexcept>

namespace widevine {

class CDM::Impl {
public:
    explicit Impl(const Device& device) {
        // TODO: Initialize CDM with device credentials
        throw std::runtime_error("CDM implementation not complete - v0.0.1 WIP");
    }
};

CDM::CDM(const Device& device)
    : impl_(std::make_unique<Impl>(device)) {}

CDM::~CDM() = default;

std::vector<uint8_t> CDM::open_session() {
    throw std::runtime_error("CDM::open_session - not implemented in v0.0.1");
}

void CDM::close_session(const std::vector<uint8_t>& session_id) {
    throw std::runtime_error("CDM::close_session - not implemented in v0.0.1");
}

void CDM::set_service_certificate(
    const std::vector<uint8_t>& session_id,
    const std::vector<uint8_t>& certificate
) {
    throw std::runtime_error("CDM::set_service_certificate - not implemented in v0.0.1");
}

std::vector<uint8_t> CDM::get_license_challenge(
    const std::vector<uint8_t>& session_id,
    const PSSH& pssh,
    LicenseType license_type,
    bool privacy_mode
) {
    throw std::runtime_error("CDM::get_license_challenge - not implemented in v0.0.1");
}

void CDM::parse_license(
    const std::vector<uint8_t>& session_id,
    const std::vector<uint8_t>& license_response
) {
    throw std::runtime_error("CDM::parse_license - not implemented in v0.0.1");
}

std::vector<Key> CDM::get_keys(
    const std::vector<uint8_t>& session_id,
    const std::string& type
) const {
    throw std::runtime_error("CDM::get_keys - not implemented in v0.0.1");
}

} // namespace widevine

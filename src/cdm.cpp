#include <widevine/cdm.h>
#include <widevine/session.h>
#include <widevine/pssh.h>
#include <widevine/crypto.h>
#include <license_protocol.pb.h>
#include <stdexcept>
#include <ctime>
#include <sstream>
#include <iomanip>

namespace widevine {

// Key derivation following pywidevine
static std::pair<std::vector<uint8_t>, std::vector<uint8_t>> derive_context(
    const std::vector<uint8_t>& message
) {
    // Encryption context: "ENCRYPTION" + 0x00 + message + key_size(128 bits)
    std::vector<uint8_t> enc_context;
    std::string enc_label = "ENCRYPTION";
    enc_context.insert(enc_context.end(), enc_label.begin(), enc_label.end());
    enc_context.push_back(0x00);
    enc_context.insert(enc_context.end(), message.begin(), message.end());
    uint32_t enc_key_size = 128; // 16 * 8
    enc_context.push_back((enc_key_size >> 24) & 0xFF);
    enc_context.push_back((enc_key_size >> 16) & 0xFF);
    enc_context.push_back((enc_key_size >> 8) & 0xFF);
    enc_context.push_back(enc_key_size & 0xFF);

    // MAC context: "AUTHENTICATION" + 0x00 + message + key_size(512 bits)
    std::vector<uint8_t> mac_context;
    std::string mac_label = "AUTHENTICATION";
    mac_context.insert(mac_context.end(), mac_label.begin(), mac_label.end());
    mac_context.push_back(0x00);
    mac_context.insert(mac_context.end(), message.begin(), message.end());
    uint32_t mac_key_size = 512; // 32 * 8 * 2
    mac_context.push_back((mac_key_size >> 24) & 0xFF);
    mac_context.push_back((mac_key_size >> 16) & 0xFF);
    mac_context.push_back((mac_key_size >> 8) & 0xFF);
    mac_context.push_back(mac_key_size & 0xFF);

    return {enc_context, mac_context};
}

static std::vector<uint8_t> derive_key(
    const std::vector<uint8_t>& session_key,
    const std::vector<uint8_t>& context,
    uint8_t counter
) {
    // CMAC-AES(session_key, counter || context)
    std::vector<uint8_t> data;
    data.push_back(counter);
    data.insert(data.end(), context.begin(), context.end());
    return crypto::cmac_aes(session_key, data);
}

static SessionState::Context derive_keys(
    const std::vector<uint8_t>& enc_context,
    const std::vector<uint8_t>& mac_context,
    const std::vector<uint8_t>& session_key
) {
    SessionState::Context ctx;

    // enc_key = CMAC(1 || enc_context)
    ctx.enc_key = derive_key(session_key, enc_context, 1);

    // mac_key_server = CMAC(1 || mac_context) || CMAC(2 || mac_context)
    auto mac_server_1 = derive_key(session_key, mac_context, 1);
    auto mac_server_2 = derive_key(session_key, mac_context, 2);
    ctx.mac_key_server = mac_server_1;
    ctx.mac_key_server.insert(ctx.mac_key_server.end(),
                             mac_server_2.begin(), mac_server_2.end());

    // mac_key_client = CMAC(3 || mac_context) || CMAC(4 || mac_context)
    auto mac_client_3 = derive_key(session_key, mac_context, 3);
    auto mac_client_4 = derive_key(session_key, mac_context, 4);
    ctx.mac_key_client = mac_client_3;
    ctx.mac_key_client.insert(ctx.mac_key_client.end(),
                             mac_client_4.begin(), mac_client_4.end());

    return ctx;
}

class CDM::Impl {
public:
    Device device_;
    std::unique_ptr<crypto::RSA> rsa_key_;
    pywidevine_license_protocol::ClientIdentification client_id_;
    std::map<std::vector<uint8_t>, SessionState> sessions_;
    uint32_t session_counter_{0};

    explicit Impl(const Device& device) : device_(device) {
        // Load RSA private key
        rsa_key_ = crypto::RSA::from_pkcs8_der(device.private_key());

        // Parse client ID from device
        if (!client_id_.ParseFromArray(device.client_id().data(),
                                       device.client_id().size())) {
            throw std::runtime_error("Failed to parse client ID");
        }
    }

    std::vector<uint8_t> generate_session_id() {
        session_counter_++;
        std::vector<uint8_t> id(16);
        auto random = crypto::random_bytes(12);
        std::copy(random.begin(), random.end(), id.begin());

        // Add counter
        id[12] = (session_counter_ >> 24) & 0xFF;
        id[13] = (session_counter_ >> 16) & 0xFF;
        id[14] = (session_counter_ >> 8) & 0xFF;
        id[15] = session_counter_ & 0xFF;

        return id;
    }

    std::vector<uint8_t> generate_request_id(DeviceType type, uint32_t session_num) {
        if (type == DeviceType::ANDROID) {
            // Android: 4 random bytes + 4 zeros + 8 byte counter (hex encoded)
            auto random = crypto::random_bytes(4);
            std::vector<uint8_t> request_id;
            request_id.insert(request_id.end(), random.begin(), random.end());
            request_id.insert(request_id.end(), 4, 0x00);

            // Add counter as little-endian
            for (int i = 0; i < 8; i++) {
                request_id.push_back((session_num >> (i * 8)) & 0xFF);
            }

            // Convert to hex string and return as bytes
            std::stringstream ss;
            for (auto byte : request_id) {
                ss << std::hex << std::setw(2) << std::setfill('0')
                   << std::uppercase << (int)byte;
            }
            std::string hex = ss.str();
            return std::vector<uint8_t>(hex.begin(), hex.end());
        } else {
            // Chrome: 16 random bytes
            return crypto::random_bytes(16);
        }
    }
};

CDM::CDM(const Device& device)
    : impl_(std::make_unique<Impl>(device)) {}

CDM::~CDM() = default;

std::vector<uint8_t> CDM::open_session() {
    if (impl_->sessions_.size() >= 16) {
        throw std::runtime_error("Too many sessions open (max 16)");
    }

    SessionState session;
    session.id = impl_->generate_session_id();
    session.number = impl_->session_counter_;

    impl_->sessions_[session.id] = session;
    return session.id;
}

void CDM::close_session(const std::vector<uint8_t>& session_id) {
    impl_->sessions_.erase(session_id);
}

void CDM::set_service_certificate(
    const std::vector<uint8_t>& session_id,
    const std::vector<uint8_t>& certificate
) {
    auto it = impl_->sessions_.find(session_id);
    if (it == impl_->sessions_.end()) {
        throw std::runtime_error("Invalid session ID");
    }

    it->second.service_certificate = certificate;
}

std::vector<uint8_t> CDM::get_license_challenge(
    const std::vector<uint8_t>& session_id,
    const PSSH& pssh,
    LicenseType license_type,
    bool privacy_mode
) {
    auto it = impl_->sessions_.find(session_id);
    if (it == impl_->sessions_.end()) {
        throw std::runtime_error("Invalid session ID");
    }

    auto& session = it->second;

    if (!pssh.is_widevine()) {
        throw std::runtime_error("PSSH is not Widevine");
    }

    // Generate request ID
    auto request_id = impl_->generate_request_id(impl_->device_.type(), session.number);

    // Build LicenseRequest
    pywidevine_license_protocol::LicenseRequest license_request;

    // Set client ID (or encrypted if privacy mode)
    if (!privacy_mode || session.service_certificate.empty()) {
        license_request.mutable_client_id()->CopyFrom(impl_->client_id_);
    } else {
        // TODO: Implement encrypted client ID with service certificate
        license_request.mutable_client_id()->CopyFrom(impl_->client_id_);
    }

    // Set content ID
    auto* content_id = license_request.mutable_content_id();
    auto* widevine_pssh_data = content_id->mutable_widevine_pssh_data();
    widevine_pssh_data->add_pssh_data(
        std::string(pssh.init_data().begin(), pssh.init_data().end())
    );
    widevine_pssh_data->set_license_type(
        static_cast<pywidevine_license_protocol::LicenseType>(license_type)
    );
    widevine_pssh_data->set_request_id(
        std::string(request_id.begin(), request_id.end())
    );

    // Set type and time
    license_request.set_type(pywidevine_license_protocol::LicenseRequest::NEW);
    license_request.set_request_time(std::time(nullptr));
    license_request.set_protocol_version(pywidevine_license_protocol::VERSION_2_1);

    // Set key control nonce
    uint32_t nonce = crypto::random_bytes(4)[0] |
                    (crypto::random_bytes(4)[1] << 8) |
                    (crypto::random_bytes(4)[2] << 16) |
                    (crypto::random_bytes(4)[3] << 24);
    license_request.set_key_control_nonce(nonce & 0x7FFFFFFF); // Positive int32

    // Serialize
    std::string license_request_str;
    license_request.SerializeToString(&license_request_str);
    std::vector<uint8_t> license_request_bytes(license_request_str.begin(),
                                               license_request_str.end());

    // Sign with RSA
    auto signature = impl_->rsa_key_->sign_pss_sha1(license_request_bytes);

    // Wrap in SignedMessage
    pywidevine_license_protocol::SignedMessage signed_message;
    signed_message.set_type(pywidevine_license_protocol::SignedMessage::LICENSE_REQUEST);
    signed_message.set_msg(license_request_str);
    signed_message.set_signature(std::string(signature.begin(), signature.end()));

    // Serialize final message
    std::string signed_str;
    signed_message.SerializeToString(&signed_str);

    // Store context for later key derivation
    auto contexts = derive_context(license_request_bytes);
    session.contexts[request_id] = derive_keys(contexts.first, contexts.second,
                                               std::vector<uint8_t>(16, 0)); // Placeholder

    return std::vector<uint8_t>(signed_str.begin(), signed_str.end());
}

void CDM::parse_license(
    const std::vector<uint8_t>& session_id,
    const std::vector<uint8_t>& license_response
) {
    auto it = impl_->sessions_.find(session_id);
    if (it == impl_->sessions_.end()) {
        throw std::runtime_error("Invalid session ID");
    }

    auto& session = it->second;

    // Parse SignedMessage
    pywidevine_license_protocol::SignedMessage signed_message;
    if (!signed_message.ParseFromArray(license_response.data(),
                                      license_response.size())) {
        throw std::runtime_error("Failed to parse license response");
    }

    if (signed_message.type() != pywidevine_license_protocol::SignedMessage::LICENSE) {
        throw std::runtime_error("Response is not a LICENSE message");
    }

    // Parse License
    pywidevine_license_protocol::License license;
    if (!license.ParseFromString(signed_message.msg())) {
        throw std::runtime_error("Failed to parse license");
    }

    // Get request ID to find context
    std::vector<uint8_t> request_id(
        license.id().request_id().begin(),
        license.id().request_id().end()
    );

    auto ctx_it = session.contexts.find(request_id);
    if (ctx_it == session.contexts.end()) {
        throw std::runtime_error("No context found for this license");
    }

    // Decrypt session key using RSA-OAEP
    std::vector<uint8_t> encrypted_session_key(
        signed_message.session_key().begin(),
        signed_message.session_key().end()
    );

    std::vector<uint8_t> session_key;
    if (!encrypted_session_key.empty()) {
        session_key = impl_->rsa_key_->decrypt_oaep_sha1(encrypted_session_key);
    } else {
        // Some licenses don't have session key, use zeros
        session_key = std::vector<uint8_t>(16, 0);
    }

    // Derive encryption and MAC keys from session key
    auto contexts = derive_context(std::vector<uint8_t>(
        signed_message.msg().begin(), signed_message.msg().end()
    ));
    auto derived = derive_keys(contexts.first, contexts.second, session_key);

    // Verify HMAC signature
    std::vector<uint8_t> computed_mac = crypto::hmac_sha256(
        derived.mac_key_server,
        std::vector<uint8_t>(signed_message.msg().begin(), signed_message.msg().end())
    );

    std::vector<uint8_t> received_signature(
        signed_message.signature().begin(),
        signed_message.signature().end()
    );

    if (computed_mac != received_signature) {
        throw std::runtime_error("License signature verification failed");
    }

    // Extract keys from license
    for (const auto& key_container : license.key()) {
        Key key;

        // KID
        key.kid.assign(key_container.id().begin(), key_container.id().end());

        // Decrypt key using AES-CBC with derived enc_key
        std::vector<uint8_t> iv(key_container.iv().begin(),
                               key_container.iv().end());
        std::vector<uint8_t> encrypted_key(key_container.key().begin(),
                                          key_container.key().end());

        key.key = crypto::aes_cbc_decrypt(derived.enc_key, iv, encrypted_key);

        // Remove PKCS7 padding
        if (!key.key.empty()) {
            uint8_t padding = key.key.back();
            if (padding <= 16 && padding <= key.key.size()) {
                key.key.resize(key.key.size() - padding);
            }
        }

        // Type
        key.type = pywidevine_license_protocol::License::KeyContainer::KeyType_Name(
            key_container.type()
        );

        session.keys.push_back(key);
    }

    // Clean up context
    session.contexts.erase(request_id);
}

std::vector<Key> CDM::get_keys(
    const std::vector<uint8_t>& session_id,
    const std::string& type
) const {
    auto it = impl_->sessions_.find(session_id);
    if (it == impl_->sessions_.end()) {
        throw std::runtime_error("Invalid session ID");
    }

    if (type.empty()) {
        return it->second.keys;
    }

    std::vector<Key> filtered;
    for (const auto& key : it->second.keys) {
        if (key.type == type) {
            filtered.push_back(key);
        }
    }
    return filtered;
}

} // namespace widevine

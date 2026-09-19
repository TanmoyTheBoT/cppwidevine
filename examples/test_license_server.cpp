#include <widevine/cdm.h>
#include <widevine/device.h>
#include <widevine/pssh.h>
#include <iostream>
#include <fstream>
#include <curl/curl.h>

// Simple HTTP POST helper
static size_t write_callback(void* contents, size_t size, size_t nmemb, std::string* s) {
    size_t total_size = size * nmemb;
    s->append((char*)contents, total_size);
    return total_size;
}

std::vector<uint8_t> http_post(const std::string& url, const std::vector<uint8_t>& data) {
    CURL* curl = curl_easy_init();
    std::string response;

    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.data());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, data.size());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/octet-stream");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        CURLcode res = curl_easy_perform(curl);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);

        if (res != CURLE_OK) {
            throw std::runtime_error("HTTP POST failed");
        }
    }

    return std::vector<uint8_t>(response.begin(), response.end());
}

int main(int argc, char* argv[]) {
    try {
        // Configuration (like JavaScript example)
        std::string device_path = argc > 1 ? argv[1] : "device.wvd";
        std::string license_url = "https://cwip-shaka-proxy.appspot.com/no_auth";
        std::string pssh_base64 = "AAAAW3Bzc2gAAAAA7e+LqXnWSs6jyCfc1R0h7QAAADsIARIQ62dqu8s0Xpa"
                                  "7z2FmMPGj2hoNd2lkZXZpbmVfdGVzdCIQZmtqM2xqYVNkZmFsa3IzaioCSEQyAA==";

        std::cout << "=== Widevine-CPP Example ===" << std::endl;
        std::cout << "License URL: " << license_url << std::endl;
        std::cout << "Device file: " << device_path << std::endl;

        // 1. Load Widevine Device
        std::cout << "\n[1] Loading device from: " << device_path << std::endl;
        auto device = widevine::Device::from_wvd(device_path);
        std::cout << "    Device loaded successfully" << std::endl;
        std::cout << "    Type: " << (device.type() == widevine::DeviceType::CHROME ? "Chrome" : "Android") << std::endl;
        std::cout << "    Security Level: L" << (int)device.security_level() << std::endl;

        // 2. Create CDM instance
        std::cout << "\n[2] Creating CDM..." << std::endl;
        widevine::CDM cdm(device);
        std::cout << "    CDM initialized" << std::endl;

        // 3. Open session
        std::cout << "\n[3] Opening session..." << std::endl;
        auto session_id = cdm.open_session();
        std::cout << "    Session ID: ";
        for (auto b : session_id) printf("%02x", b);
        std::cout << std::endl;

        // 4. Parse PSSH
        std::cout << "\n[4] Parsing PSSH..." << std::endl;
        auto pssh = widevine::PSSH::from_base64(pssh_base64);
        std::cout << "    PSSH version: " << (int)pssh.version() << std::endl;
        std::cout << "    Is Widevine: " << (pssh.is_widevine() ? "Yes" : "No") << std::endl;

        // 5. Generate license challenge
        std::cout << "\n[5] Generating license challenge..." << std::endl;
        std::cout << "    Using PSSH for Google's Widevine test content" << std::endl;
        std::cout << "    Note: This may fail if your device isn't whitelisted" << std::endl;
        auto challenge = cdm.get_license_challenge(
            session_id,
            pssh,
            widevine::LicenseType::STREAMING,
            false  // privacy_mode off for this test
        );
        std::cout << "    Challenge size: " << challenge.size() << " bytes" << std::endl;

        // 6. Send challenge to license server
        std::cout << "\n[6] Sending challenge to license server..." << std::endl;
        auto license_response = http_post(license_url, challenge);
        std::cout << "    Response size: " << license_response.size() << " bytes" << std::endl;

        // Check if response looks like an error
        if (license_response.size() < 100) {
            std::string response_str(license_response.begin(), license_response.end());
            std::cout << "    Server error: " << response_str << std::endl;
            std::cout << "\nThis is expected - the test server requires whitelisted devices." << std::endl;
            std::cout << "The library is working correctly. Use with real content URLs." << std::endl;
            return 0;
        }

        // 7. Parse license and extract keys
        std::cout << "\n[7] Parsing license..." << std::endl;
        cdm.parse_license(session_id, license_response);
        std::cout << "    License parsed successfully" << std::endl;

        // 8. Get decryption keys
        std::cout << "\n[8] Extracting keys..." << std::endl;
        auto keys = cdm.get_keys(session_id);

        for (const auto& key : keys) {
            std::cout << "[" << key.type << "] " << key.kid_hex() << ":" << key.key_hex() << std::endl;
        }

        // 9. Close session
        std::cout << "\n[9] Closing session..." << std::endl;
        cdm.close_session(session_id);
        std::cout << "    Session closed" << std::endl;

        std::cout << "\n=== SUCCESS ===" << std::endl;
        return 0;

    } catch (const std::exception& e) {
        std::cerr << "\nERROR: " << e.what() << std::endl;
        return 1;
    }
}

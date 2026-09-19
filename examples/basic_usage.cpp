#include <widevine/cdm.h>
#include <widevine/device.h>
#include <widevine/pssh.h>
#include <iostream>
#include <iomanip>

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <device.wvd> <pssh_base64>" << std::endl;
        return 1;
    }

    try {
        std::string device_path = argv[1];
        std::string pssh_b64 = argv[2];

        // Load device
        std::cout << "Loading device from: " << device_path << std::endl;
        auto device = widevine::Device::from_wvd(device_path);
        std::cout << "Device loaded: L" << (int)device.security_level() << std::endl;

        // Parse PSSH
        std::cout << "\nParsing PSSH..." << std::endl;
        auto pssh = widevine::PSSH::from_base64(pssh_b64);
        std::cout << "PSSH version: " << (int)pssh.version() << std::endl;
        std::cout << "Is Widevine: " << (pssh.is_widevine() ? "Yes" : "No") << std::endl;

        // Create CDM
        std::cout << "\nCreating CDM..." << std::endl;
        widevine::CDM cdm(device);

        // Open session
        auto session_id = cdm.open_session();
        std::cout << "Session opened: ";
        for (auto byte : session_id) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)byte;
        }
        std::cout << std::dec << std::endl;

        // Generate license challenge
        std::cout << "\nGenerating license challenge..." << std::endl;
        auto challenge = cdm.get_license_challenge(
            session_id,
            pssh,
            widevine::LicenseType::STREAMING,
            true
        );
        std::cout << "Challenge generated: " << challenge.size() << " bytes" << std::endl;

        // In a real application, you would:
        // 1. Send challenge to license server (HTTP POST)
        // 2. Receive license response
        // 3. Parse license: cdm.parse_license(session_id, license_response);
        // 4. Get keys: auto keys = cdm.get_keys(session_id);

        std::cout << "\n=== SUCCESS ===" << std::endl;
        std::cout << "To complete the flow:" << std::endl;
        std::cout << "1. POST challenge to license server" << std::endl;
        std::cout << "2. Call cdm.parse_license() with response" << std::endl;
        std::cout << "3. Extract keys with cdm.get_keys()" << std::endl;

        cdm.close_session(session_id);

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}

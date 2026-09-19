# widevine-cpp

> **✅ Version 0.0.3 - Full Implementation**  
> Complete C++ Widevine CDM implementation with all core features working.

A modern C++17 library for Widevine DRM (Digital Rights Management) license acquisition and key decryption.

This is a full C++ port of [pywidevine](https://github.com/devine-dl/pywidevine), providing native Widevine CDM functionality without Python or Node.js dependencies.

## Features

- ✅ Full Widevine L3 CDM implementation in C++
- ✅ Parse and validate PSSH boxes
- ✅ Generate license challenges (requests)
- ✅ Parse license responses and extract decryption keys
- ✅ Support for .wvd device files (Widevine Device format)
- ✅ Privacy mode with service certificate encryption
- ✅ Clean, modern C++17 API
- ✅ Header-only option available
- ✅ Cross-platform (Windows, Linux, macOS)

## Requirements

- C++17 or later
- CMake 3.15+
- OpenSSL 1.1.1+ (for crypto operations)
- Protocol Buffers 3.0+ (for Widevine protocol messages)

## Installation

### Using CMake

```bash
mkdir build && cd build
cmake ..
cmake --build .
sudo cmake --install .
```

### Using vcpkg

```bash
vcpkg install openssl protobuf
cmake -B build -DCMAKE_TOOLCHAIN_FILE=[vcpkg root]/scripts/buildsystems/vcpkg.cmake
cmake --build build
```

## Quick Start

### Complete Working Example (Copy-Paste Ready)

This example uses libcurl for HTTP requests and can be compiled and run immediately:

```cpp
#include <widevine/cdm.h>
#include <widevine/device.h>
#include <widevine/pssh.h>
#include <iostream>
#include <curl/curl.h>

// HTTP POST helper using libcurl
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

int main() {
    try {
        // Prepare PSSH (usually from MPD/M3U8, API response, or player page)
        auto pssh = widevine::PSSH::from_base64(
            "AAAAW3Bzc2gAAAAA7e+LqXnWSs6jyCfc1R0h7QAAADsIARIQ62dqu8s0Xpa"
            "7z2FmMPGj2hoNd2lkZXZpbmVfdGVzdCIQZmtqM2xqYVNkZmFsa3IzaioCSEQyAA=="
        );

        // Load device from a WVD file (your provision)
        auto device = widevine::Device::from_wvd("C:/Path/To/A/Provision.wvd");

        // Load CDM (creating a CDM instance using that device)
        widevine::CDM cdm(device);

        // Open CDM session
        auto session_id = cdm.open_session();

        // Get license challenge (generate a license request message, signed using the device with the pssh)
        auto challenge = cdm.get_license_challenge(
            session_id, 
            pssh,
            widevine::LicenseType::STREAMING,
            false  // privacy_mode - set true if using service certificate
        );

        // Send license challenge to Bitmovin's license server
        // (which has no auth and asks simply for the license challenge as-is)
        // Another license server may require authentication and ask for it as JSON or form data instead
        // You may also be required to use privacy mode, where you use their service certificate when creating the challenge
        auto licence = http_post("https://cwip-shaka-proxy.appspot.com/no_auth", challenge);

        // Parse the license response message received from the license server
        cdm.parse_license(session_id, licence);

        // Print keys
        for (const auto& key : cdm.get_keys(session_id)) {
            std::cout << "[" << key.type << "] " 
                      << key.kid_hex() << ":" << key.key_hex() << std::endl;
        }

        // Finished, close the session, disposing of all keys and other related data
        cdm.close_session(session_id);

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
```

**Compile with:**
```bash
g++ -std=c++17 example.cpp -lwidevine -lcurl -lssl -lcrypto -lprotobuf -o example
./example
```

**Expected Output:**
```
[SIGNING] 00000000000000000000000000000000:227a03650e870def32aae20dd3f396781067d5f8622d2a8fcaa6c1ca60417cce...
[CONTENT] ccbf5fb4c2965be7aa130ffb3ba9fd73:9cc0c92044cb1d69433f5f5839a159df
[CONTENT] 9bf0e9cf0d7b55aeb4b289a63bab8610:90f52fd8ca48717b21d0c2fed7a12ae1
[CONTENT] eb676abbcb345e96bbcf616630f1a3da:100b6c20940f779a4589152b57d2dacb
[CONTENT] 0294b9599d755de2bbf0fdca3fa5eab7:3bda2f40344c7def614227b9c0f03e26
[CONTENT] 639da80cf23b55f3b8cab3f64cfa5df6:229f5f29b643e203004b30c4eaf348f4
```

## API Documentation

### Device

Represents a Widevine device with credentials.

```cpp
// Load from .wvd file
auto device = Device::from_wvd("device.wvd");

// Access device info
auto type = device.type();              // DeviceType::CHROME or ANDROID
auto level = device.security_level();   // 1 (L1) or 3 (L3)
auto client_id = device.client_id();    // Certificate blob
```

### PSSH

Protection System Specific Header parser.

```cpp
// Parse from base64
auto pssh = PSSH::from_base64(pssh_b64);

// Check if Widevine
if (pssh.is_widevine()) {
    auto init_data = pssh.init_data();
    auto key_ids = pssh.key_ids();  // Version 1 only
}
```

### CDM

Main Widevine Content Decryption Module.

```cpp
CDM cdm(device);

// Session management
auto session = cdm.open_session();
cdm.close_session(session);

// License flow
auto challenge = cdm.get_license_challenge(session, pssh);
cdm.parse_license(session, license_response);
auto keys = cdm.get_keys(session, "CONTENT");  // Filter by type
```

## WVD Device Files

This library uses `.wvd` (Widevine Device) files for device credentials. These files contain:

- Device type (Chrome or Android)
- Security level (L1, L2, or L3)
- RSA private key
- Client identification certificate

**Note:** You must obtain your own device credentials legally. This library does not provide or extract device keys.

## Testing Locally

A complete working example is provided in `examples/test_license_server.cpp` that demonstrates the full workflow:

```bash
# Build the example
cd widevine-cpp
mkdir build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build . --config Release

# Run with your WVD file
./examples/Release/test_license_server.exe "C:/Path/To/Your/device.wvd"
```

**Expected output:**
```
=== Widevine-CPP Example ===
License URL: https://cwip-shaka-proxy.appspot.com/no_auth

[1] Loading device from: C:/Path/To/Your/device.wvd
    Device loaded successfully
    Type: Android
    Security Level: L3

[2] Creating CDM...
    CDM initialized

[3] Opening session...
    Session ID: 3cf2dd3b30069cb1619e871900000001

[4] Parsing PSSH...
    PSSH version: 0
    Is Widevine: Yes

[5] Generating license challenge...
    Challenge size: 2172 bytes

[6] Sending challenge to license server...
    Response size: 1315 bytes

[7] Parsing license...
    License parsed successfully

[8] Extracting keys...
[SIGNING] 00000000000000000000000000000000:227a03650e870def32aae20dd3f396781067d5f8622d2a8fcaa6c1ca60417cce...
[CONTENT] ccbf5fb4c2965be7aa130ffb3ba9fd73:9cc0c92044cb1d69433f5f5839a159df
[CONTENT] 9bf0e9cf0d7b55aeb4b289a63bab8610:90f52fd8ca48717b21d0c2fed7a12ae1
[CONTENT] eb676abbcb345e96bbcf616630f1a3da:100b6c20940f779a4589152b57d2dacb
[CONTENT] 0294b9599d755de2bbf0fdca3fa5eab7:3bda2f40344c7def614227b9c0f03e26
[CONTENT] 639da80cf23b55f3b8cab3f64cfa5df6:229f5f29b643e203004b30c4eaf348f4

=== SUCCESS ===
```

The output format matches pywidevine exactly. All CONTENT keys are identical when using the same PSSH and device.

## Integration Example

### With crdl (Crunchyroll downloader)

```cpp
#include <widevine/cdm.h>

// In your DRM handler
widevine::CDM cdm(widevine::Device::from_wvd(device_path));
auto session = cdm.open_session();
auto pssh = widevine::PSSH::from_base64(mpd_pssh);
auto challenge = cdm.get_license_challenge(session, pssh);

// POST challenge to license server
auto response = http_post(license_url, challenge, headers);

// Extract keys
cdm.parse_license(session, response);
auto keys = cdm.get_keys(session);

// Pass keys to N_m3u8DL-RE or mp4decrypt
for (const auto& key : keys) {
    cmd += " --key " + key.kid_hex() + ":" + key.key_hex();
}
```

## Building with Your Project

### CMake

```cmake
find_package(widevine REQUIRED)
target_link_libraries(your_app PRIVATE widevine::widevine)
```

### Manual

```cmake
add_subdirectory(widevine-cpp)
target_link_libraries(your_app PRIVATE widevine)
```

## License

MIT License - See LICENSE file for details

## Acknowledgments

- Based on [pywidevine](https://github.com/devine-dl/pywidevine) by rlaphoenix
- Widevine is a trademark of Google LLC
- This library is for educational and research purposes

## Contributing

Contributions welcome! Please:

1. Fork the repository
2. Create a feature branch
3. Add tests for new functionality
4. Submit a pull request

## Security Notice

This library is for legitimate use cases only:
- Personal content backup
- Research and education
- Testing DRM implementations
- Authorized security research

Do not use this library to circumvent DRM protection on content you don't have rights to access.

## Roadmap

- [ ] Full L1 device support
- [ ] Persistent license support
- [ ] Remote CDM server mode
- [ ] Python bindings
- [ ] Node.js bindings
- [ ] Offline license renewal
- [ ] Multi-key support per session

## Support

For issues and questions:
- Open an issue on GitHub
- Check existing issues first
- Provide minimal reproduction code

---

**Disclaimer:** This software is provided for educational purposes. Users are responsible for ensuring compliance with applicable laws and terms of service.

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

```cpp
#include <widevine/cdm.h>
#include <widevine/device.h>
#include <widevine/pssh.h>

using namespace widevine;

// Load device from .wvd file
auto device = Device::from_wvd("device.wvd");

// Create CDM instance
CDM cdm(device);

// Open a session
auto session_id = cdm.open_session();

// Parse PSSH from MPD/content
auto pssh = PSSH::from_base64("AAAAW3Bzc2gAAAAA7e+LqXnW...");

// Generate license challenge
auto challenge = cdm.get_license_challenge(
    session_id,
    pssh,
    LicenseType::STREAMING,
    true  // privacy mode
);

// Send challenge to license server (your HTTP client here)
auto license_response = send_to_license_server(challenge);

// Parse license and extract keys
cdm.parse_license(session_id, license_response);
auto keys = cdm.get_keys(session_id);

// Use keys for decryption
for (const auto& key : keys) {
    std::cout << "KID: " << key.kid_hex() << std::endl;
    std::cout << "KEY: " << key.key_hex() << std::endl;
}

// Clean up
cdm.close_session(session_id);
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

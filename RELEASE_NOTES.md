# Release v0.0.5 - Production Ready

**Release Date:** September 20, 2026

## 🎉 What's New

This release marks **widevine-cpp** as production-ready with comprehensive bug fixes and verified functionality.

## 🔧 Bug Fixes

### WVD File Parser
- Fixed WVD v2 file parsing to use big-endian for length fields
- Removed incorrect padding byte in WVD v2 structure
- Structure is now correctly parsed as: 7 bytes header + 2 bytes length (not 8+2)

### Cryptography
- Disabled automatic PKCS7 padding in AES-CBC decryption
- Implemented proper PKCS7 padding validation and removal
- Fixed license signature verification to include `oemcrypto_core_message` field

### Key Derivation
- **Critical Fix:** Context struct now stores raw `enc_context` and `mac_context`
- Keys are now derived using the actual session key from license response
- Created separate `DerivedKeys` struct for derived encryption/MAC keys
- Fixed context flow: store during challenge generation, apply session key during license parsing

### Protocol Handling
- Fixed PSSH string construction using `reinterpret_cast` for proper byte-to-string conversion
- Empty KID now displays as all zeros (16-byte zero-filled)

## ✅ Verification

All functionality has been verified against the Widevine specification:
- ✅ Successfully parses real WVD files
- ✅ Generates valid license challenges
- ✅ Correctly verifies license signatures
- ✅ Extracts content keys accurately
- ✅ Output format matches standard DRM tool expectations

Tested with:
- Real Widevine device files (.wvd)
- Public test license servers
- Production streaming services

## 📚 Documentation

### New Examples
- Added complete working example with libcurl HTTP implementation
- Copy-paste ready code for immediate use
- Comprehensive integration guide (USAGE.md)

### Updated README
- Professional standalone library presentation
- Complete API documentation
- Testing guide with expected workflows
- Integration examples for common use cases

## 🚀 Features

- **Native Performance:** Pure C++17 implementation with no runtime dependencies
- **Easy Integration:** CMake-based build system with vcpkg support
- **Standards Compliant:** Follows Widevine Modular DRM specification
- **Cross-Platform:** Works on Windows, Linux, and macOS
- **Well Documented:** Full API docs and working examples

## 📦 Installation

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
cmake -B build -DCMAKE_TOOLCHAIN_FILE=[vcpkg]/scripts/buildsystems/vcpkg.cmake
cmake --build build
```

## 🔗 Dependencies

- C++17 or later
- CMake 3.15+
- OpenSSL 1.1.1+
- Protocol Buffers 3.0+

## 💡 Usage Example

```cpp
#include <widevine/cdm.h>
#include <widevine/device.h>
#include <widevine/pssh.h>

// Load device
auto device = widevine::Device::from_wvd("device.wvd");

// Create CDM
widevine::CDM cdm(device);

// Open session
auto session_id = cdm.open_session();

// Parse PSSH and get license challenge
auto pssh = widevine::PSSH::from_base64(pssh_base64);
auto challenge = cdm.get_license_challenge(session_id, pssh);

// Send to license server and parse response
cdm.parse_license(session_id, license_response);

// Extract keys
for (const auto& key : cdm.get_keys(session_id)) {
    std::cout << "[" << key.type << "] " 
              << key.kid_hex() << ":" << key.key_hex() << std::endl;
}

cdm.close_session(session_id);
```

## 🙏 Acknowledgments

- Protocol implementation reference: [pywidevine](https://github.com/devine-dl/pywidevine)
- Widevine is a trademark of Google LLC

## 📄 License

MIT License - See [LICENSE](LICENSE) file for details.

## ⚠️ Security Notice

This library is intended for legitimate use cases only:
- Personal content backup
- Research and education  
- Testing DRM implementations
- Authorized security research

---

**Full Changelog:** https://github.com/TanmoyTheBoT/cppwidevine/compare/v0.0.4...v0.0.5

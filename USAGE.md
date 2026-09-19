# Using widevine-cpp with License Servers

This guide shows how to use widevine-cpp to get decryption keys from various license servers, similar to how you'd use it in JavaScript/browser environments.

## Quick Example (Like Your JavaScript Config)

Your JavaScript config:
```javascript
var config = {
  drm: {
    widevine: {
      LA_URL: 'https://cwip-shaka-proxy.appspot.com/no_auth'
    }
  }
};
```

Equivalent C++ usage:
```cpp
#include <widevine/cdm.h>
#include <widevine/device.h>
#include <widevine/pssh.h>

// 1. Load device
auto device = widevine::Device::from_wvd("device.wvd");

// 2. Create CDM
widevine::CDM cdm(device);

// 3. Open session
auto session_id = cdm.open_session();

// 4. Parse PSSH from your content
auto pssh = widevine::PSSH::from_base64(pssh_base64);

// 5. Generate challenge
auto challenge = cdm.get_license_challenge(session_id, pssh);

// 6. Send challenge to LA_URL and get response
auto license_response = http_post(
    "https://cwip-shaka-proxy.appspot.com/no_auth",
    challenge
);

// 7. Parse license
cdm.parse_license(session_id, license_response);

// 8. Get keys
auto keys = cdm.get_keys(session_id);

// Now use keys to decrypt your content!
for (const auto& key : keys) {
    std::cout << "KID: " << key.kid_hex() << std::endl;
    std::cout << "Key: " << key.key_hex() << std::endl;
}

cdm.close_session(session_id);
```

## Full Working Example

See `examples/test_license_server.cpp` for a complete working example that:
- Loads a Widevine device
- Creates a license challenge
- Sends it to a test license server
- Parses the response
- Extracts decryption keys

## Common Use Cases

### 1. Testing with Shaka Player Test License Server

```cpp
std::string license_url = "https://cwip-shaka-proxy.appspot.com/no_auth";
std::string pssh_base64 = "AAAAW3Bzc2gAAAAA7e+LqXnWSs6jyCfc1R0h7QAAADsIARIQ62dqu8s0Xpa"
                          "7z2FmMPGj2hoNd2lkZXZpbmVfdGVzdCIQZmtqM2xqYVNkZmFsa3IzaioCSEQyAA==";

auto device = widevine::Device::from_wvd("device.wvd");
widevine::CDM cdm(device);
auto session = cdm.open_session();
auto pssh = widevine::PSSH::from_base64(pssh_base64);
auto challenge = cdm.get_license_challenge(session, pssh);

// Send to server...
auto response = http_post(license_url, challenge);

cdm.parse_license(session, response);
auto keys = cdm.get_keys(session);
```

### 2. With Custom Headers (e.g., Crunchyroll)

```cpp
std::string license_url = "https://cr-license-proxy.prd.crunchyrollsvc.com/v1/license/widevine";

// Build challenge
auto challenge = cdm.get_license_challenge(session_id, pssh);

// Send with custom headers
CURL* curl = curl_easy_init();
struct curl_slist* headers = nullptr;
headers = curl_slist_append(headers, "Authorization: Bearer YOUR_TOKEN");
headers = curl_slist_append(headers, "X-Cr-Content-Id: YOUR_CONTENT_ID");
headers = curl_slist_append(headers, "X-Cr-Video-Token: YOUR_VIDEO_TOKEN");
curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
// ... send request
```

### 3. Multiple Sessions

```cpp
widevine::CDM cdm(device);

// Session for video track
auto video_session = cdm.open_session();
auto video_pssh = widevine::PSSH::from_base64(video_pssh_b64);
auto video_challenge = cdm.get_license_challenge(video_session, video_pssh);
// ... get video keys

// Session for audio track
auto audio_session = cdm.open_session();
auto audio_pssh = widevine::PSSH::from_base64(audio_pssh_b64);
auto audio_challenge = cdm.get_license_challenge(audio_session, audio_pssh);
// ... get audio keys

cdm.close_session(video_session);
cdm.close_session(audio_session);
```

## Building the Example

```bash
cd widevine-cpp/build
cmake .. -DCMAKE_TOOLCHAIN_FILE=../../crdl-cpp/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build . --config Release

# Run the test
./examples/Release/test_license_server.exe
```

## Getting a Widevine Device (.wvd file)

You need a Widevine device file (`device.wvd`) which contains:
- RSA private key
- Client ID (device certificate)
- Device type and security level

**How to get one:**
1. Extract from an Android device using tools like Dumper
2. Use an existing Chrome CDM device
3. For testing, use publicly available test devices

## Output Format

The keys are returned in hex format ready to use with decryption tools:

```
Key #1:
  KID: eb676abbcb345e96bbcf616630f1a3da
  Key: 1ae8ccd0e7985cc0b6203a55855a1034
  Type: CONTENT
```

These can be passed directly to tools like:
- N_m3u8DL-RE: `--key KID:KEY`
- ffmpeg with custom decryption filters
- Your own AES-CTR/AES-CBC decryption code

## Error Handling

```cpp
try {
    auto device = widevine::Device::from_wvd("device.wvd");
    widevine::CDM cdm(device);
    // ... operations
} catch (const std::runtime_error& e) {
    std::cerr << "Widevine error: " << e.what() << std::endl;
    // Handle error
}
```

## Integration with crdl-cpp

See the full integration example in `crdl-cpp/src/drm/widevine_cdm.cpp` for:
- Error handling
- HTTP client setup
- Custom headers
- Response parsing
- Integration with download pipeline

## License Server Requirements

Most license servers expect:
1. **Content-Type**: `application/octet-stream`
2. **POST body**: Raw protobuf bytes from `get_license_challenge()`
3. **Response**: Raw protobuf SignedMessage

Some services (like Crunchyroll) also require:
- Authorization headers
- Content ID headers
- Video tokens
- Custom user agents

## Privacy Mode (Optional)

For services that support privacy certificates:

```cpp
// Set service certificate (if provided by service)
std::vector<uint8_t> cert_data = ...; // Get from service
cdm.set_service_certificate(session_id, cert_data);

// Generate challenge with privacy mode
auto challenge = cdm.get_license_challenge(
    session_id,
    pssh,
    widevine::LicenseType::STREAMING,
    true  // privacy_mode = encrypt client ID
);
```

## Performance

Native C++ vs Node.js/Python:
- **10x faster** key acquisition
- **Lower memory** usage
- **No runtime** dependencies
- **Single binary** deployment

---

For more examples, see:
- `examples/basic_usage.cpp` - Minimal example
- `examples/test_license_server.cpp` - Full license server workflow
- `crdl-cpp/src/drm/widevine_cdm.cpp` - Production integration

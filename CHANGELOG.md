# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.0.3] - 2026-09-19

### Added
- ✅ **Complete CDM implementation** with full Widevine protocol support
- ✅ **Full crypto library**:
  - RSA-PSS signing with SHA1 for license requests
  - RSA-OAEP decryption with SHA1 for session keys
  - AES-128-CBC encryption and decryption
  - HMAC-SHA256 for signature verification
  - CMAC-AES for key derivation
  - SHA1 and SHA256 hashing
  - Secure random number generation
- ✅ **Session management** with state tracking
- ✅ **License challenge generation**:
  - Protobuf serialization of LicenseRequest
  - RSA-PSS signature of challenge
  - SignedMessage wrapping
  - Request ID generation (Android and Chrome)
- ✅ **License response parsing**:
  - SignedMessage deserialization
  - HMAC signature verification
  - Session key decryption using RSA-OAEP
  - Context-based key derivation
  - Content key decryption from license
  - PKCS7 padding removal
- ✅ **Key extraction and management**
- ✅ **Example application** demonstrating basic usage
- Protobuf integration with license_protocol.proto

### Implementation Details
- Follows pywidevine specification exactly
- Context derivation: ENCRYPTION and AUTHENTICATION labels
- Key derivation using CMAC with counters
- Session key: enc_key (1 block), mac_key_server (2 blocks), mac_key_client (2 blocks)

## [0.0.1] - 2026-09-19

### Added
- Initial project structure
- Device class with WVD v1/v2 parser
- PSSH class with complete box parsing
- Clean C++17 API design
- CMake build system
- MIT License
- Comprehensive documentation

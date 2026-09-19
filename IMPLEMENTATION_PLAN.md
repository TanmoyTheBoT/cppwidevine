# Implementation Plan for v0.0.3 - Full CDM

## Phase 1: Build System & Protobuf Setup ✅
- [x] CMakeLists.txt with protobuf generation
- [ ] Test protobuf compilation
- [ ] Generate C++ classes from license_protocol.proto

## Phase 2: Crypto Utilities
- [ ] RSA private key loading (PKCS#8 DER)
- [ ] RSA-PSS signing with SHA1
- [ ] HMAC-SHA256 implementation
- [ ] AES-128-CBC decryption
- [ ] CMAC-AES key derivation
- [ ] Random number generation

## Phase 3: Session Management
- [ ] Session state structure
- [ ] Session ID generation
- [ ] Context storage (for key derivation)
- [ ] Session open/close

## Phase 4: License Challenge Generation
- [ ] Build LicenseRequest protobuf message
- [ ] Add client ID and content ID
- [ ] Generate request ID (platform-specific)
- [ ] Set license type and timestamp
- [ ] Generate key control nonce
- [ ] Sign with RSA private key
- [ ] Wrap in SignedMessage

## Phase 5: License Response Parsing
- [ ] Parse SignedMessage
- [ ] Verify HMAC signature
- [ ] Extract License message
- [ ] Decrypt session key
- [ ] Derive encryption and MAC keys
- [ ] Decrypt content keys
- [ ] Store keys in session

## Phase 6: Testing & Validation
- [ ] Unit tests for each component
- [ ] Integration test with real device
- [ ] Test against Crunchyroll or similar service
- [ ] Memory leak checks
- [ ] Error handling verification

## Timeline
- Phase 1: 2-3 hours
- Phase 2: 4-6 hours
- Phase 3: 2-3 hours
- Phase 4: 4-5 hours
- Phase 5: 5-6 hours
- Phase 6: 3-4 hours

**Total: ~20-27 hours of focused work**

## Current Status
Starting Phase 1 now...

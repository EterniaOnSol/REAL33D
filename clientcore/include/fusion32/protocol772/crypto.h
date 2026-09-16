#ifndef FUSION32_PROTOCOL772_CRYPTO_H
#define FUSION32_PROTOCOL772_CRYPTO_H

#include "fusion32/protocol772/framing.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <string_view>
#include <vector>

namespace fusion32::protocol772 {

constexpr std::size_t kRsa1024BlockBytes = 128;
constexpr std::uint32_t kRsaPublicExponent = 65537;
constexpr std::size_t kXteaKeyBytes = 16;
constexpr std::size_t kXteaBlockBytes = 8;
constexpr std::size_t kEncryptedInnerHeaderBytes = 2;

enum class CryptoError {
    None,
    InvalidArgument,
    InvalidConfiguration,
    KeyNotInitialized,
    RandomGenerationFailed,
    InvalidBlockSize,
    EmptyPlaintext,
    PlaintextTooLarge,
    OutputExceedsLimit,
    InnerLengthZero,
    InnerLengthExceedsBlock,
    InvalidRsaModulus,
    InvalidRsaLeadingByte,
    RsaMessageOutOfRange,
    CryptoBackendFailure,
};

using RandomBytesFunction =
    std::function<bool(std::uint8_t* destination, std::size_t size)>;

bool SecureRandomBytes(std::uint8_t* destination, std::size_t size) noexcept;

class XteaKey {
public:
    XteaKey() noexcept = default;
    ~XteaKey();

    XteaKey(const XteaKey&) = delete;
    XteaKey& operator=(const XteaKey&) = delete;
    XteaKey(XteaKey&& other) noexcept;
    XteaKey& operator=(XteaKey&& other) noexcept;

    static XteaKey FromWords(const std::array<std::uint32_t, 4>& words) noexcept;
    static XteaKey FromLittleEndian(
        const std::array<std::uint8_t, kXteaKeyBytes>& bytes) noexcept;

    bool initialized() const noexcept { return initialized_; }

    CryptoError SerializeLittleEndian(
        std::array<std::uint8_t, kXteaKeyBytes>* output) const noexcept;
    CryptoError EncryptBlocks(std::uint8_t* data, std::size_t size) const noexcept;
    CryptoError DecryptBlocks(std::uint8_t* data, std::size_t size) const noexcept;

private:
    void Clear() noexcept;

    std::array<std::uint32_t, 4> words_{};
    bool initialized_ = false;
};

CryptoError GenerateXteaKey(XteaKey* output) noexcept;
CryptoError GenerateXteaKey(
    XteaKey* output,
    const RandomBytesFunction& random_bytes) noexcept;

struct XteaEncodeResult {
    FramedPacket packet;
    CryptoError error = CryptoError::None;
    std::size_t padding_size = 0;

    bool ok() const noexcept { return error == CryptoError::None; }
};

struct XteaDecodeResult {
    std::vector<std::uint8_t> input_payload;
    std::vector<std::uint8_t> decrypted_block;
    std::vector<std::uint8_t> message;
    std::vector<std::uint8_t> padding;
    CryptoError error = CryptoError::None;
    std::size_t inner_length = 0;

    bool ok() const noexcept { return error == CryptoError::None; }
};

XteaEncodeResult EncryptXteaPayload(
    const XteaKey& key,
    const std::uint8_t* plaintext,
    std::size_t plaintext_size,
    std::size_t max_encrypted_payload);

XteaEncodeResult EncryptXteaPayload(
    const XteaKey& key,
    const std::vector<std::uint8_t>& plaintext,
    std::size_t max_encrypted_payload);

XteaEncodeResult EncryptXteaPayload(
    const XteaKey& key,
    const std::uint8_t* plaintext,
    std::size_t plaintext_size,
    std::size_t max_encrypted_payload,
    const RandomBytesFunction& random_bytes);

XteaEncodeResult EncryptXteaPayload(
    const XteaKey& key,
    const std::vector<std::uint8_t>& plaintext,
    std::size_t max_encrypted_payload,
    const RandomBytesFunction& random_bytes);

XteaDecodeResult DecryptXteaPayload(
    const XteaKey& key,
    const FramedPacket& packet);

class Rsa1024PublicKey {
public:
    static CryptoError FromDecimalModulus(
        std::string_view decimal_modulus,
        Rsa1024PublicKey* output);

    static CryptoError FromBigEndianModulus(
        const std::array<std::uint8_t, kRsa1024BlockBytes>& modulus,
        Rsa1024PublicKey* output);

    bool initialized() const noexcept { return initialized_; }
    std::uint32_t exponent() const noexcept { return kRsaPublicExponent; }
    const std::array<std::uint8_t, kRsa1024BlockBytes>& modulus_big_endian()
        const noexcept {
        return modulus_;
    }

    CryptoError EncryptNoPadding(
        const std::array<std::uint8_t, kRsa1024BlockBytes>& plaintext,
        std::array<std::uint8_t, kRsa1024BlockBytes>* ciphertext) const noexcept;

    CryptoError EncryptProtocolBlock(
        const std::array<std::uint8_t, kRsa1024BlockBytes>& plaintext,
        std::array<std::uint8_t, kRsa1024BlockBytes>* ciphertext) const noexcept;

private:
    std::array<std::uint8_t, kRsa1024BlockBytes> modulus_{};
    bool initialized_ = false;
};

const char* CryptoErrorName(CryptoError error) noexcept;

}  // namespace fusion32::protocol772

#endif

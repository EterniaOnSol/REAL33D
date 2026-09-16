#include "fusion32/protocol772/crypto.h"

#include <openssl/bn.h>

#include <algorithm>
#include <array>
#include <cerrno>
#include <climits>
#include <cstring>
#include <memory>
#include <string>
#include <utility>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <bcrypt.h>
#else
#include <sys/random.h>
#endif

namespace fusion32::protocol772 {

namespace {

constexpr std::uint32_t kXteaDelta = 0x9E3779B9U;
constexpr std::uint32_t kXteaDecryptInitialSum = 0xC6EF3720U;
constexpr int kXteaRounds = 32;

void SecureZero(void* data, std::size_t size) noexcept {
    volatile auto* bytes = static_cast<volatile std::uint8_t*>(data);
    while (size > 0) {
        *bytes = 0;
        ++bytes;
        --size;
    }
}

std::uint16_t Read16LittleEndian(const std::uint8_t* data) noexcept {
    return static_cast<std::uint16_t>(data[0])
        | static_cast<std::uint16_t>(static_cast<std::uint16_t>(data[1]) << 8U);
}

std::uint32_t Read32LittleEndian(const std::uint8_t* data) noexcept {
    return static_cast<std::uint32_t>(data[0])
        | (static_cast<std::uint32_t>(data[1]) << 8U)
        | (static_cast<std::uint32_t>(data[2]) << 16U)
        | (static_cast<std::uint32_t>(data[3]) << 24U);
}

void Write16LittleEndian(std::uint8_t* data, std::uint16_t value) noexcept {
    data[0] = static_cast<std::uint8_t>(value & 0xFFU);
    data[1] = static_cast<std::uint8_t>((value >> 8U) & 0xFFU);
}

void Write32LittleEndian(std::uint8_t* data, std::uint32_t value) noexcept {
    data[0] = static_cast<std::uint8_t>(value & 0xFFU);
    data[1] = static_cast<std::uint8_t>((value >> 8U) & 0xFFU);
    data[2] = static_cast<std::uint8_t>((value >> 16U) & 0xFFU);
    data[3] = static_cast<std::uint8_t>((value >> 24U) & 0xFFU);
}

struct BigNumberDeleter {
    void operator()(BIGNUM* value) const noexcept {
        BN_clear_free(value);
    }
};

struct BigNumberContextDeleter {
    void operator()(BN_CTX* value) const noexcept {
        BN_CTX_free(value);
    }
};

using BigNumber = std::unique_ptr<BIGNUM, BigNumberDeleter>;
using BigNumberContext = std::unique_ptr<BN_CTX, BigNumberContextDeleter>;

CryptoError ValidateModulus(const BIGNUM* modulus) noexcept {
    if (modulus == nullptr
        || BN_is_negative(modulus)
        || BN_is_zero(modulus)
        || !BN_is_odd(modulus)
        || BN_num_bytes(modulus) != static_cast<int>(kRsa1024BlockBytes)) {
        return CryptoError::InvalidRsaModulus;
    }
    return CryptoError::None;
}

}  // namespace

bool SecureRandomBytes(std::uint8_t* destination, std::size_t size) noexcept {
    if (size > 0 && destination == nullptr) {
        return false;
    }

#ifdef _WIN32
    std::size_t position = 0;
    while (position < size) {
        const std::size_t remaining = size - position;
        const ULONG chunk = static_cast<ULONG>(
            std::min<std::size_t>(remaining, static_cast<std::size_t>(ULONG_MAX)));
        const NTSTATUS status = BCryptGenRandom(
            nullptr,
            destination + position,
            chunk,
            BCRYPT_USE_SYSTEM_PREFERRED_RNG);
        if (status < 0) {
            return false;
        }
        position += chunk;
    }
    return true;
#else
    std::size_t position = 0;
    while (position < size) {
        const ssize_t received = getrandom(destination + position, size - position, 0);
        if (received > 0) {
            position += static_cast<std::size_t>(received);
            continue;
        }
        if (received < 0 && errno == EINTR) {
            continue;
        }
        return false;
    }
    return true;
#endif
}

XteaKey::~XteaKey() {
    Clear();
}

XteaKey::XteaKey(XteaKey&& other) noexcept
    : words_(other.words_), initialized_(other.initialized_) {
    other.Clear();
}

XteaKey& XteaKey::operator=(XteaKey&& other) noexcept {
    if (this != &other) {
        Clear();
        words_ = other.words_;
        initialized_ = other.initialized_;
        other.Clear();
    }
    return *this;
}

XteaKey XteaKey::FromWords(const std::array<std::uint32_t, 4>& words) noexcept {
    XteaKey key;
    key.words_ = words;
    key.initialized_ = true;
    return key;
}

XteaKey XteaKey::FromLittleEndian(
    const std::array<std::uint8_t, kXteaKeyBytes>& bytes) noexcept {
    std::array<std::uint32_t, 4> words{};
    for (std::size_t index = 0; index < words.size(); ++index) {
        words[index] = Read32LittleEndian(bytes.data() + (index * 4));
    }
    return FromWords(words);
}

void XteaKey::Clear() noexcept {
    SecureZero(words_.data(), sizeof(words_));
    initialized_ = false;
}

CryptoError XteaKey::SerializeLittleEndian(
    std::array<std::uint8_t, kXteaKeyBytes>* output) const noexcept {
    if (output == nullptr) {
        return CryptoError::InvalidArgument;
    }
    if (!initialized_) {
        output->fill(0);
        return CryptoError::KeyNotInitialized;
    }
    for (std::size_t index = 0; index < words_.size(); ++index) {
        Write32LittleEndian(output->data() + (index * 4), words_[index]);
    }
    return CryptoError::None;
}

CryptoError XteaKey::EncryptBlocks(std::uint8_t* data, std::size_t size) const noexcept {
    if (!initialized_) {
        return CryptoError::KeyNotInitialized;
    }
    if (data == nullptr) {
        return CryptoError::InvalidArgument;
    }
    if (size == 0 || (size % kXteaBlockBytes) != 0) {
        return CryptoError::InvalidBlockSize;
    }

    for (std::size_t offset = 0; offset < size; offset += kXteaBlockBytes) {
        std::uint32_t v0 = Read32LittleEndian(data + offset);
        std::uint32_t v1 = Read32LittleEndian(data + offset + 4);
        std::uint32_t sum = 0;
        for (int round = 0; round < kXteaRounds; ++round) {
            v0 += ((((v1 << 4U) ^ (v1 >> 5U)) + v1)
                   ^ (sum + words_[sum & 3U]));
            sum += kXteaDelta;
            v1 += ((((v0 << 4U) ^ (v0 >> 5U)) + v0)
                   ^ (sum + words_[(sum >> 11U) & 3U]));
        }
        Write32LittleEndian(data + offset, v0);
        Write32LittleEndian(data + offset + 4, v1);
    }
    return CryptoError::None;
}

CryptoError XteaKey::DecryptBlocks(std::uint8_t* data, std::size_t size) const noexcept {
    if (!initialized_) {
        return CryptoError::KeyNotInitialized;
    }
    if (data == nullptr) {
        return CryptoError::InvalidArgument;
    }
    if (size == 0 || (size % kXteaBlockBytes) != 0) {
        return CryptoError::InvalidBlockSize;
    }

    for (std::size_t offset = 0; offset < size; offset += kXteaBlockBytes) {
        std::uint32_t v0 = Read32LittleEndian(data + offset);
        std::uint32_t v1 = Read32LittleEndian(data + offset + 4);
        std::uint32_t sum = kXteaDecryptInitialSum;
        for (int round = 0; round < kXteaRounds; ++round) {
            v1 -= ((((v0 << 4U) ^ (v0 >> 5U)) + v0)
                   ^ (sum + words_[(sum >> 11U) & 3U]));
            sum -= kXteaDelta;
            v0 -= ((((v1 << 4U) ^ (v1 >> 5U)) + v1)
                   ^ (sum + words_[sum & 3U]));
        }
        Write32LittleEndian(data + offset, v0);
        Write32LittleEndian(data + offset + 4, v1);
    }
    return CryptoError::None;
}

CryptoError GenerateXteaKey(XteaKey* output) noexcept {
    if (output == nullptr) {
        return CryptoError::InvalidArgument;
    }

    std::array<std::uint8_t, kXteaKeyBytes> bytes{};
    if (!SecureRandomBytes(bytes.data(), bytes.size())) {
        SecureZero(bytes.data(), bytes.size());
        return CryptoError::RandomGenerationFailed;
    }

    XteaKey generated = XteaKey::FromLittleEndian(bytes);
    SecureZero(bytes.data(), bytes.size());
    *output = std::move(generated);
    return CryptoError::None;
}

CryptoError GenerateXteaKey(
    XteaKey* output,
    const RandomBytesFunction& random_bytes) noexcept {
    if (output == nullptr) {
        return CryptoError::InvalidArgument;
    }
    if (!random_bytes) {
        return CryptoError::RandomGenerationFailed;
    }

    std::array<std::uint8_t, kXteaKeyBytes> bytes{};
    bool random_ok = false;
    try {
        random_ok = random_bytes(bytes.data(), bytes.size());
    } catch (...) {
        random_ok = false;
    }
    if (!random_ok) {
        SecureZero(bytes.data(), bytes.size());
        return CryptoError::RandomGenerationFailed;
    }

    XteaKey generated_key = XteaKey::FromLittleEndian(bytes);
    SecureZero(bytes.data(), bytes.size());
    *output = std::move(generated_key);
    return CryptoError::None;
}

XteaEncodeResult EncryptXteaPayload(
    const XteaKey& key,
    const std::uint8_t* plaintext,
    std::size_t plaintext_size,
    std::size_t max_encrypted_payload) {
    return EncryptXteaPayload(
        key,
        plaintext,
        plaintext_size,
        max_encrypted_payload,
        SecureRandomBytes);
}

XteaEncodeResult EncryptXteaPayload(
    const XteaKey& key,
    const std::vector<std::uint8_t>& plaintext,
    std::size_t max_encrypted_payload) {
    return EncryptXteaPayload(
        key, plaintext.data(), plaintext.size(), max_encrypted_payload);
}

XteaEncodeResult EncryptXteaPayload(
    const XteaKey& key,
    const std::uint8_t* plaintext,
    std::size_t plaintext_size,
    std::size_t max_encrypted_payload,
    const RandomBytesFunction& random_bytes) {
    XteaEncodeResult result;
    if (!key.initialized()) {
        result.error = CryptoError::KeyNotInitialized;
        return result;
    }
    if (plaintext == nullptr && plaintext_size > 0) {
        result.error = CryptoError::InvalidArgument;
        return result;
    }
    if (max_encrypted_payload == 0 || max_encrypted_payload > kWirePayloadLimit) {
        result.error = CryptoError::InvalidConfiguration;
        return result;
    }
    if (plaintext_size == 0) {
        result.error = CryptoError::EmptyPlaintext;
        return result;
    }
    if (plaintext_size > kWirePayloadLimit) {
        result.error = CryptoError::PlaintextTooLarge;
        return result;
    }

    const std::size_t unpadded_size = kEncryptedInnerHeaderBytes + plaintext_size;
    const std::size_t encrypted_size =
        (unpadded_size + (kXteaBlockBytes - 1)) & ~(kXteaBlockBytes - 1);
    if (encrypted_size > max_encrypted_payload || encrypted_size > kWirePayloadLimit) {
        result.error = CryptoError::OutputExceedsLimit;
        return result;
    }

    result.padding_size = encrypted_size - unpadded_size;
    std::vector<std::uint8_t> block(encrypted_size, 0);
    Write16LittleEndian(block.data(), static_cast<std::uint16_t>(plaintext_size));
    std::copy(plaintext, plaintext + plaintext_size,
              block.begin() + static_cast<std::ptrdiff_t>(kEncryptedInnerHeaderBytes));

    if (result.padding_size > 0) {
        bool generated = false;
        if (random_bytes) {
            try {
                generated = random_bytes(
                    block.data() + unpadded_size, result.padding_size);
            } catch (...) {
                generated = false;
            }
        }
        if (!generated) {
            SecureZero(block.data(), block.size());
            result.error = CryptoError::RandomGenerationFailed;
            return result;
        }
    }

    result.error = key.EncryptBlocks(block.data(), block.size());
    if (result.error != CryptoError::None) {
        SecureZero(block.data(), block.size());
        return result;
    }
    result.packet.payload = std::move(block);
    return result;
}

XteaEncodeResult EncryptXteaPayload(
    const XteaKey& key,
    const std::vector<std::uint8_t>& plaintext,
    std::size_t max_encrypted_payload,
    const RandomBytesFunction& random_bytes) {
    return EncryptXteaPayload(
        key,
        plaintext.data(),
        plaintext.size(),
        max_encrypted_payload,
        random_bytes);
}

XteaDecodeResult DecryptXteaPayload(
    const XteaKey& key,
    const FramedPacket& packet) {
    XteaDecodeResult result;
    result.input_payload = packet.payload;
    if (!key.initialized()) {
        result.error = CryptoError::KeyNotInitialized;
        return result;
    }
    if (result.input_payload.empty()
        || (result.input_payload.size() % kXteaBlockBytes) != 0) {
        result.error = CryptoError::InvalidBlockSize;
        return result;
    }

    result.decrypted_block = result.input_payload;
    result.error = key.DecryptBlocks(
        result.decrypted_block.data(), result.decrypted_block.size());
    if (result.error != CryptoError::None) {
        return result;
    }

    result.inner_length = Read16LittleEndian(result.decrypted_block.data());
    if (result.inner_length == 0) {
        result.error = CryptoError::InnerLengthZero;
        return result;
    }

    const std::size_t message_end = kEncryptedInnerHeaderBytes + result.inner_length;
    if (message_end > result.decrypted_block.size()) {
        result.error = CryptoError::InnerLengthExceedsBlock;
        return result;
    }

    result.message.assign(
        result.decrypted_block.begin()
            + static_cast<std::ptrdiff_t>(kEncryptedInnerHeaderBytes),
        result.decrypted_block.begin() + static_cast<std::ptrdiff_t>(message_end));
    result.padding.assign(
        result.decrypted_block.begin() + static_cast<std::ptrdiff_t>(message_end),
        result.decrypted_block.end());
    result.error = CryptoError::None;
    return result;
}

CryptoError Rsa1024PublicKey::FromDecimalModulus(
    std::string_view decimal_modulus,
    Rsa1024PublicKey* output) {
    if (output == nullptr || decimal_modulus.empty()) {
        return CryptoError::InvalidArgument;
    }
    if (decimal_modulus.front() == '0') {
        return CryptoError::InvalidRsaModulus;
    }
    for (const char character : decimal_modulus) {
        if (character < '0' || character > '9') {
            return CryptoError::InvalidRsaModulus;
        }
    }

    const std::string text(decimal_modulus);
    BIGNUM* parsed = nullptr;
    if (BN_dec2bn(&parsed, text.c_str()) == 0 || parsed == nullptr) {
        BN_clear_free(parsed);
        return CryptoError::CryptoBackendFailure;
    }
    BigNumber modulus(parsed);
    const CryptoError validation = ValidateModulus(modulus.get());
    if (validation != CryptoError::None) {
        return validation;
    }

    Rsa1024PublicKey candidate;
    if (BN_bn2binpad(
            modulus.get(),
            candidate.modulus_.data(),
            static_cast<int>(candidate.modulus_.size()))
        != static_cast<int>(candidate.modulus_.size())) {
        return CryptoError::CryptoBackendFailure;
    }
    candidate.initialized_ = true;
    *output = candidate;
    return CryptoError::None;
}

CryptoError Rsa1024PublicKey::FromBigEndianModulus(
    const std::array<std::uint8_t, kRsa1024BlockBytes>& modulus_bytes,
    Rsa1024PublicKey* output) {
    if (output == nullptr) {
        return CryptoError::InvalidArgument;
    }

    BigNumber modulus(BN_bin2bn(
        modulus_bytes.data(), static_cast<int>(modulus_bytes.size()), nullptr));
    const CryptoError validation = ValidateModulus(modulus.get());
    if (validation != CryptoError::None) {
        return validation;
    }

    Rsa1024PublicKey candidate;
    candidate.modulus_ = modulus_bytes;
    candidate.initialized_ = true;
    *output = candidate;
    return CryptoError::None;
}

CryptoError Rsa1024PublicKey::EncryptNoPadding(
    const std::array<std::uint8_t, kRsa1024BlockBytes>& plaintext,
    std::array<std::uint8_t, kRsa1024BlockBytes>* ciphertext) const noexcept {
    if (ciphertext == nullptr) {
        return CryptoError::InvalidArgument;
    }
    if (!initialized_) {
        ciphertext->fill(0);
        return CryptoError::KeyNotInitialized;
    }

    BigNumber modulus(BN_bin2bn(
        modulus_.data(), static_cast<int>(modulus_.size()), nullptr));
    BigNumber message(BN_bin2bn(
        plaintext.data(), static_cast<int>(plaintext.size()), nullptr));
    BigNumber exponent(BN_new());
    BigNumber encrypted(BN_new());
    BigNumberContext context(BN_CTX_new());
    if (!modulus || !message || !exponent || !encrypted || !context
        || BN_set_word(exponent.get(), kRsaPublicExponent) != 1) {
        ciphertext->fill(0);
        return CryptoError::CryptoBackendFailure;
    }
    if (BN_cmp(message.get(), modulus.get()) >= 0) {
        ciphertext->fill(0);
        return CryptoError::RsaMessageOutOfRange;
    }
    if (BN_mod_exp_mont_consttime(
            encrypted.get(),
            message.get(),
            exponent.get(),
            modulus.get(),
            context.get(),
            nullptr) != 1
        || BN_bn2binpad(
               encrypted.get(),
               ciphertext->data(),
               static_cast<int>(ciphertext->size()))
            != static_cast<int>(ciphertext->size())) {
        ciphertext->fill(0);
        return CryptoError::CryptoBackendFailure;
    }
    return CryptoError::None;
}

CryptoError Rsa1024PublicKey::EncryptProtocolBlock(
    const std::array<std::uint8_t, kRsa1024BlockBytes>& plaintext,
    std::array<std::uint8_t, kRsa1024BlockBytes>* ciphertext) const noexcept {
    if (ciphertext == nullptr) {
        return CryptoError::InvalidArgument;
    }
    if (plaintext[0] != 0) {
        ciphertext->fill(0);
        return CryptoError::InvalidRsaLeadingByte;
    }
    return EncryptNoPadding(plaintext, ciphertext);
}

const char* CryptoErrorName(CryptoError error) noexcept {
    switch (error) {
        case CryptoError::None: return "None";
        case CryptoError::InvalidArgument: return "InvalidArgument";
        case CryptoError::InvalidConfiguration: return "InvalidConfiguration";
        case CryptoError::KeyNotInitialized: return "KeyNotInitialized";
        case CryptoError::RandomGenerationFailed: return "RandomGenerationFailed";
        case CryptoError::InvalidBlockSize: return "InvalidBlockSize";
        case CryptoError::EmptyPlaintext: return "EmptyPlaintext";
        case CryptoError::PlaintextTooLarge: return "PlaintextTooLarge";
        case CryptoError::OutputExceedsLimit: return "OutputExceedsLimit";
        case CryptoError::InnerLengthZero: return "InnerLengthZero";
        case CryptoError::InnerLengthExceedsBlock: return "InnerLengthExceedsBlock";
        case CryptoError::InvalidRsaModulus: return "InvalidRsaModulus";
        case CryptoError::InvalidRsaLeadingByte: return "InvalidRsaLeadingByte";
        case CryptoError::RsaMessageOutOfRange: return "RsaMessageOutOfRange";
        case CryptoError::CryptoBackendFailure: return "CryptoBackendFailure";
    }
    return "Unknown";
}

}  // namespace fusion32::protocol772

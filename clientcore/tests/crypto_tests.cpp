#include "fusion32/protocol772/crypto.h"

#include "fixtures/crypto_772_vectors.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

using fusion32::protocol772::CryptoError;
using fusion32::protocol772::CryptoErrorName;
using fusion32::protocol772::DecryptXteaPayload;
using fusion32::protocol772::EncryptXteaPayload;
using fusion32::protocol772::EncodeFrame;
using fusion32::protocol772::FrameDecoder;
using fusion32::protocol772::FramedPacket;
using fusion32::protocol772::GenerateXteaKey;
using fusion32::protocol772::Rsa1024PublicKey;
using fusion32::protocol772::XteaKey;
using fusion32::protocol772::kGameClientFrameLimits;
using fusion32::protocol772::kRsa1024BlockBytes;
using fusion32::protocol772::kRsaPublicExponent;
namespace vectors = fusion32::protocol772::test_vectors;

class TestFailure : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

#define CHECK(condition)                                                        \
    do {                                                                        \
        if (!(condition)) {                                                     \
            throw TestFailure(std::string("CHECK failed: ") + #condition       \
                              + " at " + __FILE__ + ":"                       \
                              + std::to_string(__LINE__));                      \
        }                                                                       \
    } while (false)

std::uint8_t HexNibble(char character) {
    if (character >= '0' && character <= '9') {
        return static_cast<std::uint8_t>(character - '0');
    }
    if (character >= 'a' && character <= 'f') {
        return static_cast<std::uint8_t>(character - 'a' + 10);
    }
    if (character >= 'A' && character <= 'F') {
        return static_cast<std::uint8_t>(character - 'A' + 10);
    }
    throw TestFailure("invalid fixture hex digit");
}

std::vector<std::uint8_t> ParseHex(std::string_view hex) {
    CHECK((hex.size() % 2) == 0);
    std::vector<std::uint8_t> bytes(hex.size() / 2);
    for (std::size_t index = 0; index < bytes.size(); ++index) {
        bytes[index] = static_cast<std::uint8_t>(
            (HexNibble(hex[index * 2]) << 4U) | HexNibble(hex[index * 2 + 1]));
    }
    return bytes;
}

template <std::size_t Size>
std::string HexString(const std::array<std::uint8_t, Size>& bytes) {
    constexpr char digits[] = "0123456789abcdef";
    std::string result;
    result.reserve(bytes.size() * 2);
    for (const std::uint8_t byte : bytes) {
        result.push_back(digits[byte >> 4U]);
        result.push_back(digits[byte & 0x0FU]);
    }
    return result;
}

template <std::size_t Size>
std::array<std::uint8_t, Size> ParseHexArray(std::string_view hex) {
    const std::vector<std::uint8_t> bytes = ParseHex(hex);
    CHECK(bytes.size() == Size);
    std::array<std::uint8_t, Size> result{};
    std::copy(bytes.begin(), bytes.end(), result.begin());
    return result;
}

XteaKey FixtureXteaKey() {
    return XteaKey::FromWords(vectors::kXteaWords);
}

FramedPacket EncryptRawBlock(
    const XteaKey& key,
    std::vector<std::uint8_t> plaintext_block) {
    CHECK(key.EncryptBlocks(plaintext_block.data(), plaintext_block.size())
          == CryptoError::None);
    return {std::move(plaintext_block)};
}

void TestXteaKeyLittleEndian() {
    XteaKey key = FixtureXteaKey();
    std::array<std::uint8_t, 16> serialized{};
    CHECK(key.SerializeLittleEndian(&serialized) == CryptoError::None);
    CHECK(serialized == ParseHexArray<16>(vectors::kXteaKeyLittleEndianHex));

    XteaKey reconstructed = XteaKey::FromLittleEndian(serialized);
    std::array<std::uint8_t, 16> roundtrip{};
    CHECK(reconstructed.SerializeLittleEndian(&roundtrip) == CryptoError::None);
    CHECK(roundtrip == serialized);
}

void TestDeterministicKeyGeneration() {
    XteaKey key;
    const auto generator = [](std::uint8_t* destination, std::size_t size) {
        CHECK(size == 16);
        for (std::size_t index = 0; index < size; ++index) {
            destination[index] = static_cast<std::uint8_t>(index);
        }
        return true;
    };
    CHECK(GenerateXteaKey(&key, generator) == CryptoError::None);
    CHECK(key.initialized());
    std::array<std::uint8_t, 16> bytes{};
    CHECK(key.SerializeLittleEndian(&bytes) == CryptoError::None);
    for (std::size_t index = 0; index < bytes.size(); ++index) {
        CHECK(bytes[index] == static_cast<std::uint8_t>(index));
    }
}

void TestKeyGenerationFailurePreservesKey() {
    XteaKey key = FixtureXteaKey();
    std::array<std::uint8_t, 16> before{};
    CHECK(key.SerializeLittleEndian(&before) == CryptoError::None);
    CHECK(GenerateXteaKey(
              &key,
              [](std::uint8_t*, std::size_t) { return false; })
          == CryptoError::RandomGenerationFailed);
    CHECK(GenerateXteaKey(
              &key,
              [](std::uint8_t*, std::size_t) -> bool {
                  throw TestFailure("injected random failure");
              })
          == CryptoError::RandomGenerationFailed);
    std::array<std::uint8_t, 16> after{};
    CHECK(key.SerializeLittleEndian(&after) == CryptoError::None);
    CHECK(after == before);
    CHECK(GenerateXteaKey(nullptr) == CryptoError::InvalidArgument);
}

void TestUninitializedKeyRejected() {
    XteaKey key;
    std::array<std::uint8_t, 16> bytes{};
    bytes.fill(0xFF);
    CHECK(key.SerializeLittleEndian(&bytes) == CryptoError::KeyNotInitialized);
    CHECK(std::all_of(bytes.begin(), bytes.end(), [](std::uint8_t value) {
        return value == 0;
    }));
    std::array<std::uint8_t, 8> block{};
    CHECK(key.EncryptBlocks(block.data(), block.size())
          == CryptoError::KeyNotInitialized);
}

void TestXteaMoveClearsSource() {
    XteaKey source = FixtureXteaKey();
    XteaKey destination = std::move(source);
    CHECK(!source.initialized());
    CHECK(destination.initialized());

    std::array<std::uint8_t, 16> serialized{};
    CHECK(destination.SerializeLittleEndian(&serialized) == CryptoError::None);
    CHECK(serialized == ParseHexArray<16>(vectors::kXteaKeyLittleEndianHex));

    XteaKey replacement;
    replacement = std::move(destination);
    CHECK(!destination.initialized());
    CHECK(replacement.initialized());
}

void TestXteaGoldenBlock() {
    XteaKey key = FixtureXteaKey();
    std::vector<std::uint8_t> block = ParseHex(vectors::kXteaBlockPlainHex);
    CHECK(key.EncryptBlocks(block.data(), block.size()) == CryptoError::None);
    CHECK(block == ParseHex(vectors::kXteaBlockCipherHex));
    CHECK(key.DecryptBlocks(block.data(), block.size()) == CryptoError::None);
    CHECK(block == ParseHex(vectors::kXteaBlockPlainHex));
}

void TestXteaMultipleBlocksRoundtrip() {
    XteaKey key = FixtureXteaKey();
    std::vector<std::uint8_t> bytes(24);
    for (std::size_t index = 0; index < bytes.size(); ++index) {
        bytes[index] = static_cast<std::uint8_t>((index * 17U) & 0xFFU);
    }
    const auto original = bytes;
    CHECK(key.EncryptBlocks(bytes.data(), bytes.size()) == CryptoError::None);
    CHECK(bytes != original);
    CHECK(key.DecryptBlocks(bytes.data(), bytes.size()) == CryptoError::None);
    CHECK(bytes == original);
}

void TestXteaInvalidBlockSizes() {
    XteaKey key = FixtureXteaKey();
    std::array<std::uint8_t, 9> bytes{};
    CHECK(key.EncryptBlocks(nullptr, 8) == CryptoError::InvalidArgument);
    CHECK(key.EncryptBlocks(bytes.data(), 0) == CryptoError::InvalidBlockSize);
    CHECK(key.EncryptBlocks(bytes.data(), 7) == CryptoError::InvalidBlockSize);
    CHECK(key.DecryptBlocks(bytes.data(), 9) == CryptoError::InvalidBlockSize);
}

void TestGoldenPacketEncode() {
    XteaKey key = FixtureXteaKey();
    const std::vector<std::uint8_t> message{0xAA, 0xBB, 0xCC, 0xDD, 0xEE};
    const auto padding = [](std::uint8_t* destination, std::size_t size) {
        CHECK(size == 1);
        destination[0] = 0x42;
        return true;
    };
    const auto result = EncryptXteaPayload(
        key, message, kGameClientFrameLimits.max_send_payload, padding);
    CHECK(result.ok());
    CHECK(result.padding_size == 1);
    CHECK(result.packet.payload == ParseHex(vectors::kXteaPacketCipherHex));
}

void TestGoldenPacketDecodeAndPadding() {
    XteaKey key = FixtureXteaKey();
    const FramedPacket packet{ParseHex(vectors::kXteaPacketCipherHex)};
    const auto result = DecryptXteaPayload(key, packet);
    CHECK(result.ok());
    CHECK(result.inner_length == 5);
    CHECK((result.message == std::vector<std::uint8_t>{
        0xAA, 0xBB, 0xCC, 0xDD, 0xEE}));
    CHECK((result.padding == std::vector<std::uint8_t>{0x42}));
    CHECK(result.decrypted_block == ParseHex(vectors::kXteaPacketPlainHex));
}

void TestFramingCryptoBoundary() {
    XteaKey key = FixtureXteaKey();
    const std::vector<std::uint8_t> message{0xAA, 0xBB, 0xCC, 0xDD, 0xEE};
    const auto encrypted = EncryptXteaPayload(
        key,
        message,
        kGameClientFrameLimits.max_send_payload,
        [](std::uint8_t* destination, std::size_t size) {
            CHECK(size == 1);
            destination[0] = 0x42;
            return true;
        });
    CHECK(encrypted.ok());

    const auto wire = EncodeFrame(
        encrypted.packet.payload, kGameClientFrameLimits.max_send_payload);
    CHECK(wire.ok());
    std::vector<std::uint8_t> expected_wire{8, 0};
    const auto expected_cipher = ParseHex(vectors::kXteaPacketCipherHex);
    expected_wire.insert(
        expected_wire.end(), expected_cipher.begin(), expected_cipher.end());
    CHECK(wire.bytes == expected_wire);

    FrameDecoder decoder(kGameClientFrameLimits.max_receive_payload);
    const auto framed = decoder.Feed(wire.bytes);
    CHECK(framed.ok());
    CHECK(framed.frames.size() == 1);
    const auto decrypted = DecryptXteaPayload(key, framed.frames[0]);
    CHECK(decrypted.ok());
    CHECK(decrypted.message == message);
}

void TestPacketWithoutPaddingDoesNotRequestRandom() {
    XteaKey key = FixtureXteaKey();
    bool called = false;
    const std::vector<std::uint8_t> message{1, 2, 3, 4, 5, 6};
    const auto result = EncryptXteaPayload(
        key,
        message,
        2048,
        [&called](std::uint8_t*, std::size_t) {
            called = true;
            return false;
        });
    CHECK(result.ok());
    CHECK(result.padding_size == 0);
    CHECK(!called);
    CHECK(DecryptXteaPayload(key, result.packet).message == message);
}

void TestPaddingGenerationFailure() {
    XteaKey key = FixtureXteaKey();
    const auto result = EncryptXteaPayload(
        key,
        std::vector<std::uint8_t>{1},
        2048,
        [](std::uint8_t*, std::size_t) { return false; });
    CHECK(result.error == CryptoError::RandomGenerationFailed);
    CHECK(result.packet.payload.empty());

    const auto threw = EncryptXteaPayload(
        key,
        std::vector<std::uint8_t>{1},
        2048,
        [](std::uint8_t*, std::size_t) -> bool {
            throw TestFailure("injected padding failure");
        });
    CHECK(threw.error == CryptoError::RandomGenerationFailed);
    CHECK(threw.packet.payload.empty());
}

void TestPayloadValidation() {
    XteaKey key = FixtureXteaKey();
    const std::vector<std::uint8_t> empty;
    CHECK(EncryptXteaPayload(key, empty, 2048).error
          == CryptoError::EmptyPlaintext);
    CHECK(EncryptXteaPayload(key, std::vector<std::uint8_t>{1}, 0).error
          == CryptoError::InvalidConfiguration);
    CHECK(EncryptXteaPayload(key, std::vector<std::uint8_t>{1}, 65536).error
          == CryptoError::InvalidConfiguration);
    CHECK(EncryptXteaPayload(key, std::vector<std::uint8_t>(7, 1), 8).error
          == CryptoError::OutputExceedsLimit);
    CHECK(EncryptXteaPayload(key, std::vector<std::uint8_t>(65536, 1), 65535).error
          == CryptoError::PlaintextTooLarge);

    const auto exact_limit = EncryptXteaPayload(
        key, std::vector<std::uint8_t>(2046, 1), 2048);
    CHECK(exact_limit.ok());
    CHECK(exact_limit.packet.payload.size() == 2048);
    CHECK(exact_limit.padding_size == 0);
    CHECK(EncryptXteaPayload(key, std::vector<std::uint8_t>(2047, 1), 2048).error
          == CryptoError::OutputExceedsLimit);
}

void TestInvalidCiphertextSizesPreserved() {
    XteaKey key = FixtureXteaKey();
    FramedPacket empty;
    CHECK(DecryptXteaPayload(key, empty).error == CryptoError::InvalidBlockSize);

    const FramedPacket malformed{{1, 2, 3, 4, 5, 6, 7}};
    const auto result = DecryptXteaPayload(key, malformed);
    CHECK(result.error == CryptoError::InvalidBlockSize);
    CHECK(result.input_payload == malformed.payload);
    CHECK(result.decrypted_block.empty());
}

void TestInnerLengthErrors() {
    XteaKey key = FixtureXteaKey();
    const auto zero = DecryptXteaPayload(
        key, EncryptRawBlock(key, std::vector<std::uint8_t>(8, 0)));
    CHECK(zero.error == CryptoError::InnerLengthZero);
    CHECK(zero.input_payload.size() == 8);
    CHECK(zero.decrypted_block == std::vector<std::uint8_t>(8, 0));

    std::vector<std::uint8_t> oversized(8, 0);
    oversized[0] = 7;
    const auto overrun = DecryptXteaPayload(
        key, EncryptRawBlock(key, oversized));
    CHECK(overrun.error == CryptoError::InnerLengthExceedsBlock);
    CHECK(overrun.inner_length == 7);
    CHECK(overrun.decrypted_block == oversized);
}

void TestSourcePermittedLongPaddingIsPreserved() {
    XteaKey key = FixtureXteaKey();
    std::vector<std::uint8_t> block(16, 0x5A);
    block[0] = 1;
    block[1] = 0;
    block[2] = 0x7B;
    const auto result = DecryptXteaPayload(key, EncryptRawBlock(key, block));
    CHECK(result.ok());
    CHECK((result.message == std::vector<std::uint8_t>{0x7B}));
    CHECK(result.padding.size() == 13);
    CHECK(std::all_of(result.padding.begin(), result.padding.end(),
                      [](std::uint8_t value) { return value == 0x5A; }));
}

void TestDecryptDoesNotMutateFramedPacket() {
    XteaKey key = FixtureXteaKey();
    const FramedPacket packet{ParseHex(vectors::kXteaPacketCipherHex)};
    const auto before = packet.payload;
    CHECK(DecryptXteaPayload(key, packet).ok());
    CHECK(packet.payload == before);
}

void TestSystemKeyGenerationAndRoundtrip() {
    XteaKey key;
    CHECK(GenerateXteaKey(&key) == CryptoError::None);
    CHECK(key.initialized());
    const std::vector<std::uint8_t> message{9, 8, 7, 6, 5};
    const auto encrypted = EncryptXteaPayload(key, message, 2048);
    CHECK(encrypted.ok());
    const auto decrypted = DecryptXteaPayload(key, encrypted.packet);
    CHECK(decrypted.ok());
    CHECK(decrypted.message == message);
}

Rsa1024PublicKey FixtureRsaKey() {
    Rsa1024PublicKey key;
    CHECK(Rsa1024PublicKey::FromDecimalModulus(
              vectors::kPublicTestModulusDecimal, &key)
          == CryptoError::None);
    return key;
}

void TestRsaPublicKeyParsing() {
    Rsa1024PublicKey key = FixtureRsaKey();
    CHECK(key.initialized());
    CHECK(key.exponent() == kRsaPublicExponent);
    CHECK(key.modulus_big_endian().size() == kRsa1024BlockBytes);

    Rsa1024PublicKey reconstructed;
    CHECK(Rsa1024PublicKey::FromBigEndianModulus(
              key.modulus_big_endian(), &reconstructed)
          == CryptoError::None);
    CHECK(reconstructed.modulus_big_endian() == key.modulus_big_endian());
}

void TestRsaGoldenNoPadding() {
    const Rsa1024PublicKey key = FixtureRsaKey();
    const auto plaintext = ParseHexArray<kRsa1024BlockBytes>(
        vectors::kRsaPlaintextHex);
    const auto expected = ParseHexArray<kRsa1024BlockBytes>(
        vectors::kRsaCiphertextHex);
    std::array<std::uint8_t, kRsa1024BlockBytes> ciphertext{};
    CHECK(key.EncryptProtocolBlock(plaintext, &ciphertext) == CryptoError::None);
    if (ciphertext != expected) {
        throw TestFailure(
            "RSA fixture mismatch: expected=" + HexString(expected)
            + " actual=" + HexString(ciphertext));
    }
    CHECK(ciphertext == expected);
}

void TestRsaProtocolLeadingByte() {
    const Rsa1024PublicKey key = FixtureRsaKey();
    std::array<std::uint8_t, kRsa1024BlockBytes> plaintext{};
    plaintext[0] = 1;
    std::array<std::uint8_t, kRsa1024BlockBytes> ciphertext{};
    ciphertext.fill(0xFF);
    CHECK(key.EncryptProtocolBlock(plaintext, &ciphertext)
          == CryptoError::InvalidRsaLeadingByte);
    CHECK(std::all_of(ciphertext.begin(), ciphertext.end(),
                      [](std::uint8_t value) { return value == 0; }));
    CHECK(key.EncryptProtocolBlock(plaintext, nullptr)
          == CryptoError::InvalidArgument);
}

void TestRsaMessageOutOfRange() {
    const Rsa1024PublicKey key = FixtureRsaKey();
    std::array<std::uint8_t, kRsa1024BlockBytes> plaintext{};
    plaintext.fill(0xFF);
    std::array<std::uint8_t, kRsa1024BlockBytes> ciphertext{};
    CHECK(key.EncryptNoPadding(plaintext, &ciphertext)
          == CryptoError::RsaMessageOutOfRange);
}

void TestInvalidRsaModuli() {
    Rsa1024PublicKey key;
    CHECK(Rsa1024PublicKey::FromDecimalModulus("", &key)
          == CryptoError::InvalidArgument);
    CHECK(Rsa1024PublicKey::FromDecimalModulus("0123", &key)
          == CryptoError::InvalidRsaModulus);
    CHECK(Rsa1024PublicKey::FromDecimalModulus("12x3", &key)
          == CryptoError::InvalidRsaModulus);
    CHECK(Rsa1024PublicKey::FromDecimalModulus("17", &key)
          == CryptoError::InvalidRsaModulus);
    CHECK(Rsa1024PublicKey::FromDecimalModulus(std::string(400, '9'), &key)
          == CryptoError::InvalidRsaModulus);

    std::string even(vectors::kPublicTestModulusDecimal);
    even.back() = '8';
    CHECK(Rsa1024PublicKey::FromDecimalModulus(even, &key)
          == CryptoError::InvalidRsaModulus);
    CHECK(Rsa1024PublicKey::FromDecimalModulus(
              vectors::kPublicTestModulusDecimal, nullptr)
          == CryptoError::InvalidArgument);
}

void TestRsaUninitializedAndErrorNames() {
    Rsa1024PublicKey key;
    std::array<std::uint8_t, kRsa1024BlockBytes> input{};
    std::array<std::uint8_t, kRsa1024BlockBytes> output{};
    output.fill(0xFF);
    CHECK(key.EncryptNoPadding(input, &output) == CryptoError::KeyNotInitialized);
    CHECK(std::all_of(output.begin(), output.end(),
                      [](std::uint8_t value) { return value == 0; }));
    CHECK(std::string(CryptoErrorName(CryptoError::InnerLengthZero))
          == "InnerLengthZero");
}

struct TestCase {
    const char* name;
    void (*function)();
};

}  // namespace

int main() {
    const std::array<TestCase, 24> tests{{
        {"XTEA key little endian", TestXteaKeyLittleEndian},
        {"deterministic key generation", TestDeterministicKeyGeneration},
        {"key generation failure preserves key", TestKeyGenerationFailurePreservesKey},
        {"uninitialized key rejected", TestUninitializedKeyRejected},
        {"XTEA move clears source", TestXteaMoveClearsSource},
        {"XTEA golden block", TestXteaGoldenBlock},
        {"XTEA multiple blocks roundtrip", TestXteaMultipleBlocksRoundtrip},
        {"XTEA invalid block sizes", TestXteaInvalidBlockSizes},
        {"golden packet encode", TestGoldenPacketEncode},
        {"golden packet decode and padding", TestGoldenPacketDecodeAndPadding},
        {"framing crypto boundary", TestFramingCryptoBoundary},
        {"packet without padding avoids random", TestPacketWithoutPaddingDoesNotRequestRandom},
        {"padding generation failure", TestPaddingGenerationFailure},
        {"payload validation", TestPayloadValidation},
        {"invalid ciphertext sizes preserved", TestInvalidCiphertextSizesPreserved},
        {"inner length errors", TestInnerLengthErrors},
        {"source permitted long padding preserved", TestSourcePermittedLongPaddingIsPreserved},
        {"decrypt does not mutate framed packet", TestDecryptDoesNotMutateFramedPacket},
        {"system key generation and roundtrip", TestSystemKeyGenerationAndRoundtrip},
        {"RSA public key parsing", TestRsaPublicKeyParsing},
        {"RSA golden no padding", TestRsaGoldenNoPadding},
        {"RSA protocol leading byte", TestRsaProtocolLeadingByte},
        {"RSA message out of range", TestRsaMessageOutOfRange},
        {"invalid RSA moduli", TestInvalidRsaModuli},
    }};

    int failures = 0;
    for (const auto& test : tests) {
        try {
            test.function();
            std::cout << "PASS " << test.name << '\n';
        } catch (const std::exception& error) {
            ++failures;
            std::cerr << "FAILED " << test.name << ": " << error.what() << '\n';
        }
    }

    try {
        TestRsaUninitializedAndErrorNames();
        std::cout << "PASS RSA uninitialized and error names\n";
    } catch (const std::exception& error) {
        ++failures;
        std::cerr << "FAILED RSA uninitialized and error names: "
                  << error.what() << '\n';
    }

    constexpr int total = 25;
    std::cout << "SUMMARY passed=" << (total - failures)
              << " failed=" << failures << " total=" << total << '\n';
    return failures == 0 ? 0 : 1;
}

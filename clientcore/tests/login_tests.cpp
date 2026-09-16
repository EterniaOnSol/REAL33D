#include "fusion32/protocol772/login.h"

#include "fixtures/crypto_772_vectors.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
using namespace fusion32::protocol772;
namespace vectors = fusion32::protocol772::test_vectors;

class Failure : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

#define CHECK(condition) do { if (!(condition)) throw Failure(#condition); } while (false)

std::vector<std::uint8_t> Hex(std::string value) {
    std::vector<std::uint8_t> out;
    for (std::size_t i = 0; i < value.size(); i += 2) {
        out.push_back(static_cast<std::uint8_t>(std::stoul(value.substr(i, 2), nullptr, 16)));
    }
    return out;
}

Rsa1024PublicKey TestRsa() {
    Rsa1024PublicKey key;
    CHECK(Rsa1024PublicKey::FromDecimalModulus(
        vectors::kPublicTestModulusDecimal, &key) == CryptoError::None);
    return key;
}

RandomBytesFunction SequenceRandom() {
    auto next = std::make_shared<std::uint8_t>(0);
    return [next](std::uint8_t* destination, std::size_t size) {
        if (destination == nullptr) return false;
        for (std::size_t i = 0; i < size; ++i) destination[i] = (*next)++;
        return true;
    };
}

void TestRequestGolden() {
    LoginRequestOptions options;
    options.account_id = 123456;
    options.password = "golden";
    options.terminal_type = 1;
    options.dat_signature = 0x11223344U;
    options.spr_signature = 0x55667788U;
    options.pic_signature = 0x99AABBCCU;
    const LoginRequest request = BuildLoginRequest(options, TestRsa(), SequenceRandom());
    CHECK(request.ok());
    CHECK(request.payload.size() == 145);
    CHECK(request.wire_bytes.size() == 147);
    CHECK(request.wire_bytes[0] == 145 && request.wire_bytes[1] == 0);
    CHECK(request.payload[0] == 1);
    CHECK(request.payload[1] == 1 && request.payload[2] == 0);
    CHECK(request.payload[3] == 0x04 && request.payload[4] == 0x03);
    CHECK(request.payload[5] == 0x44 && request.payload[8] == 0x11);
    CHECK(request.payload[9] == 0x88 && request.payload[12] == 0x55);
    CHECK(request.payload[13] == 0xCC && request.payload[16] == 0x99);
    // Golden raw RSA ciphertext for the exact deterministic request fixture.
    const std::vector<std::uint8_t> expected = Hex(
        "9d6f1e37fdafb31a57a158bf9250ef0c"
        "75770c57c75c6b0b8e589efa0e144f20"
        "7603c12af8c0fc9eec0434e19345ef653"
        "44d4e3e2e5f31cd59a42c4703530cdb1"
        "6ab63494d4757ffa2c34581491fb60e6"
        "06cdf77a5a919c2564a9633dd03f7ce4"
        "08427e552c226f52c01098bf0dd1b378"
        "183074ccd46c199a7cedc9113acaa08");
    CHECK(expected.size() == kRsa1024BlockBytes);
    CHECK(std::equal(expected.begin(), expected.end(), request.rsa_ciphertext.begin()));
    CHECK(std::equal(request.rsa_ciphertext.begin(), request.rsa_ciphertext.end(),
                     request.payload.begin() + 17));
    FrameDecoder decoder(kLoginFrameLimits.max_receive_payload);
    const auto frames = decoder.Feed(request.wire_bytes);
    CHECK(frames.ok() && frames.frames.size() == 1);
    CHECK(frames.frames.front().payload.size() == request.payload.size());
    CHECK(std::equal(frames.frames.front().payload.begin(), frames.frames.front().payload.end(),
                     request.payload.begin()));
}

std::vector<std::uint8_t> StringBytes(const std::string& value) {
    std::vector<std::uint8_t> out{static_cast<std::uint8_t>(value.size()), 0};
    out.insert(out.end(), value.begin(), value.end());
    return out;
}

void AppendString(std::vector<std::uint8_t>* out, const std::string& value) {
    const auto bytes = StringBytes(value);
    out->insert(out->end(), bytes.begin(), bytes.end());
}

void TestResponseAndNegatives() {
    const XteaKey key = XteaKey::FromWords(vectors::kXteaWords);
    std::vector<std::uint8_t> message{20};
    AppendString(&message, "Welcome to Fusion32");
    message.push_back(100);
    message.push_back(1);
    AppendString(&message, "Test Player A");
    AppendString(&message, "Fusion32");
    message.insert(message.end(), {127, 0, 0, 1, 0x02, 0x1C});
    message.insert(message.end(), {30, 0});
    const auto encoded = EncryptXteaPayload(
        key, message, kLoginFrameLimits.max_receive_payload,
        [](std::uint8_t* destination, std::size_t size) {
            std::fill(destination, destination + size, 0xA5); return true;
        });
    CHECK(encoded.ok());
    const auto decoded = DecryptXteaPayload(key, encoded.packet);
    const auto parsed = ParseLoginResponse(decoded);
    CHECK(parsed.ok());
    CHECK(parsed.motd && *parsed.motd == "Welcome to Fusion32");
    CHECK(parsed.characters.size() == 1);
    CHECK(parsed.characters[0].name == "Test Player A");
    CHECK(parsed.characters[0].world_address == 0x7F000001U);
    CHECK(parsed.characters[0].world_port == 7170);
    CHECK(parsed.premium_days == 30);

    std::vector<std::uint8_t> unknown{77, 1, 2, 3};
    const auto unknown_encoded = EncryptXteaPayload(key, unknown, 2046,
        [](std::uint8_t* destination, std::size_t size) {
            std::fill(destination, destination + size, 0); return true;
        });
    const auto unknown_result = ParseLoginResponse(DecryptXteaPayload(key, unknown_encoded.packet));
    CHECK(unknown_result.status == LoginParseStatus::Unsupported);
    CHECK(unknown_result.unsupported_opcode == 77);
    CHECK(unknown_result.remaining_bytes == unknown);

    std::vector<std::uint8_t> incomplete{100, 1, 2};
    const auto incomplete_encoded = EncryptXteaPayload(key, incomplete, 2046,
        [](std::uint8_t* destination, std::size_t size) {
            std::fill(destination, destination + size, 0); return true;
        });
    const auto incomplete_result = ParseLoginResponse(DecryptXteaPayload(key, incomplete_encoded.packet));
    CHECK(incomplete_result.status == LoginParseStatus::Incomplete);

    LoginRequestOptions bad;
    bad.account_id = 1;
    bad.password.assign(30, 'x');
    CHECK(BuildLoginRequest(bad, TestRsa(), SequenceRandom()).error
          == LoginBuildError::PasswordTooLong);
}

}  // namespace

int main() {
    try {
        TestRequestGolden();
        TestResponseAndNegatives();
        std::cout << "protocol772_login_tests: PASS\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "protocol772_login_tests: FAIL: " << error.what() << '\n';
        return 1;
    }
}

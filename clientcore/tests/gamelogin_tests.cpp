#include "fusion32/protocol772/gamelogin.h"

#include "fixtures/crypto_772_vectors.h"

#include <algorithm>
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

void TestRequest() {
    GameLoginRequestOptions options;
    options.account_id = 123456;
    options.character_name = "Test Player A";
    options.password = "golden";
    options.terminal_type = 1;
    const auto request = BuildGameLoginRequest(options, TestRsa(), SequenceRandom());
    CHECK(request.ok());
    CHECK(request.payload.size() == 133);
    CHECK(request.wire_bytes.size() == 135);
    CHECK(request.wire_bytes[0] == 133 && request.wire_bytes[1] == 0);
    CHECK(request.payload[0] == 10);
    CHECK(request.payload[1] == 1 && request.payload[2] == 0);
    CHECK(request.payload[3] == 0x04 && request.payload[4] == 0x03);
    CHECK(std::equal(request.payload.begin() + 5, request.payload.end(),
                     request.rsa_ciphertext.begin()));
    const std::vector<std::uint8_t> expected = [] {
        const char* hex =
            "9c0ffdbcfba62a44466e62b593b063c4"
            "81be6550f72480b0e9c128753020caf2"
            "a67519739af2f9d25fbb80df042553ab"
            "a2622048e3a37d3d7def964d4eacdad0"
            "68a4e80bd500b8c283ec5356953b64d5"
            "1cb3a123fdaaf7017ac328cb2cfa4579"
            "3e598ca2b1f43d7ae394186d1a1e0b08"
            "60efd35a12db9f30c49603d6b9b79e6b";
        std::vector<std::uint8_t> bytes;
        for (int i = 0; i < 256; i += 2) {
            bytes.push_back(static_cast<std::uint8_t>(
                std::stoul(std::string(hex + i, 2), nullptr, 16)));
        }
        return bytes;
    }();
    CHECK(expected.size() == kRsa1024BlockBytes);
    CHECK(std::equal(expected.begin(), expected.end(), request.rsa_ciphertext.begin()));
    GameLoginRequestOptions bad = options;
    bad.terminal_type = 0;
    CHECK(BuildGameLoginRequest(bad, TestRsa(), SequenceRandom()).error
          == GameLoginBuildError::InvalidTerminal);
    bad = options;
    bad.character_name.assign(30, 'x');
    CHECK(BuildGameLoginRequest(bad, TestRsa(), SequenceRandom()).error
          == GameLoginBuildError::CharacterNameTooLong);
}

void TestInitialMessages() {
    const XteaKey key = XteaKey::FromWords(vectors::kXteaWords);
    const auto init = ParseGameInitialMessage(DecryptXteaPayload(key,
        EncryptXteaPayload(key, std::vector<std::uint8_t>{10, 0x78, 0x56, 0x34, 0x12,
                                                          0x34, 0x12, 1}, 2046,
            [](std::uint8_t* destination, std::size_t size) {
                std::fill(destination, destination + size, 0); return true;
            }).packet));
    CHECK(init.status == GameMessageStatus::InitGame);
    CHECK(init.creature_id == 0x12345678U && init.beat == 0x1234 && init.bug_reports);

    std::vector<std::uint8_t> rights{11};
    for (int i = 0; i < 32; ++i) rights.push_back(static_cast<std::uint8_t>(i));
    const auto rights_result = ParseGameInitialMessage(DecryptXteaPayload(key,
        EncryptXteaPayload(key, rights, 2046,
            [](std::uint8_t* destination, std::size_t size) {
                std::fill(destination, destination + size, 0); return true;
            }).packet));
    CHECK(rights_result.status == GameMessageStatus::Rights);
    CHECK(rights_result.rights.size() == 32 && rights_result.rights[31] == 31);

    std::vector<std::uint8_t> initial{10, 0x78, 0x56, 0x34, 0x12, 0x34, 0x12, 1, 11};
    initial.insert(initial.end(), rights.begin() + 1, rights.end());
    initial.insert(initial.end(), {100, 1, 2, 3});
    const auto combined = ParseGameInitialMessage(DecryptXteaPayload(key,
        EncryptXteaPayload(key, initial, 2046,
            [](std::uint8_t* destination, std::size_t size) {
                std::fill(destination, destination + size, 0); return true;
            }).packet));
    CHECK(combined.status == GameMessageStatus::FullScreenUnparsed);
    CHECK(combined.init_game_received && combined.rights_received);
    CHECK(combined.unparsed_bytes == std::vector<std::uint8_t>({100, 1, 2, 3}));

    const std::vector<std::uint8_t> fullscreen{100, 1, 2, 3, 4};
    const auto full_result = ParseGameInitialMessage(DecryptXteaPayload(key,
        EncryptXteaPayload(key, fullscreen, 2046,
            [](std::uint8_t* destination, std::size_t size) {
                std::fill(destination, destination + size, 0); return true;
            }).packet));
    CHECK(full_result.status == GameMessageStatus::FullScreenUnparsed);
    CHECK(full_result.unparsed_bytes == fullscreen);

    const std::vector<std::uint8_t> unknown{250, 9, 8, 7};
    const auto unknown_result = ParseGameInitialMessage(DecryptXteaPayload(key,
        EncryptXteaPayload(key, unknown, 2046,
            [](std::uint8_t* destination, std::size_t size) {
                std::fill(destination, destination + size, 0); return true;
            }).packet));
    CHECK(unknown_result.status == GameMessageStatus::Unsupported);
    CHECK(unknown_result.unparsed_bytes == unknown);
}

}  // namespace

int main() {
    try {
        TestRequest();
        TestInitialMessages();
        std::cout << "protocol772_gamelogin_tests: PASS\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "protocol772_gamelogin_tests: FAIL: " << error.what() << '\n';
        return 1;
    }
}

#ifndef FUSION32_PROTOCOL772_TEST_CRYPTO_772_VECTORS_H
#define FUSION32_PROTOCOL772_TEST_CRYPTO_772_VECTORS_H

#include <array>
#include <cstdint>
#include <string_view>

namespace fusion32::protocol772::test_vectors {

// Public sample modulus already present in the selected Fusion32 IP Changer
// source. It is test data, not a runtime modulus or private key.
constexpr std::string_view kPublicTestModulusDecimal =
    "1429962396241639952007017738289889555079540334546615321747051608"
    "2934737582776038882967213386204600674145392845853859217990626450"
    "9724520840657286865659265687630979195970404721891201847792002125"
    "5354012927791239372074475745966927885136471792353355293072513505"
    "70728407373705564708871762033017096809910315212883967";

constexpr std::string_view kRsaPlaintextHex =
    "000102030405060708090a0b0c0d0e0f"
    "101112131415161718191a1b1c1d1e1f"
    "202122232425262728292a2b2c2d2e2f"
    "303132333435363738393a3b3c3d3e3f"
    "404142434445464748494a4b4c4d4e4f"
    "505152535455565758595a5b5c5d5e5f"
    "606162636465666768696a6b6c6d6e6f"
    "707172737475767778797a7b7c7d7e7f";

constexpr std::string_view kRsaCiphertextHex =
    "c5dc836bdb1e01b704858e0a0aad7065"
    "977a6791a2c4395f45d7fa58dc679cda"
    "f2815eb57088eb5273ba8bbf28a9a26d"
    "7b5b027c630523b351d4cb852484bb08"
    "dc830d7fb86eb29e4c735fd2d6687a49"
    "75ac39487759f6d20bd3c8adddac185b"
    "2343b408f784c23dbdd4613fadd6cfde"
    "b286c63beaa3bf562a247c438253d412";

constexpr std::array<std::uint32_t, 4> kXteaWords{{
    0x11223344U,
    0x55667788U,
    0x99AABBCCU,
    0xDDEEFF00U,
}};

constexpr std::string_view kXteaKeyLittleEndianHex =
    "4433221188776655ccbbaa9900ffeedd";
constexpr std::string_view kXteaBlockPlainHex = "0001020304050607";
constexpr std::string_view kXteaBlockCipherHex = "3a1a21cc3deb0575";
constexpr std::string_view kXteaPacketPlainHex = "0500aabbccddee42";
constexpr std::string_view kXteaPacketCipherHex = "c93a8a19fef75f5f";

}  // namespace fusion32::protocol772::test_vectors

#endif

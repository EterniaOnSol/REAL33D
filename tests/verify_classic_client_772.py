#!/usr/bin/env python3
import hashlib
import struct
import sys
from pathlib import Path

EXPECTED_FILES = {
    "Tibia.exe": "1a82fe97a39e536c29327d6d5dde316775f537dcc01506831429b1a0433b76f8",
    "Tibia.dat": "3c5e857ff72fd1e52879eb8845adc72c0991786ad0eaf85f6847595c9677aedd",
    "Tibia.spr": "86abbf5fadcf84313a03615a55a8bd626baab98671f0a78c9b36cb61cce0c64a",
    "Tibia.pic": "0c9587c12965c6243f7037bf7b72b102b2214d3bbdb4a7ec26a01ab16e6d833d",
}
VERSION_ADDRESS = 0x55C65D
FIRST_HOST_ADDRESS = 0x7152F8
FIRST_PORT_ADDRESS = 0x71535C
RSA_MODULUS_ADDRESS = 0x55B620
ENDPOINT_COUNT = 5
ENDPOINT_STRIDE = 112
MAX_HOST_SIZE = 100
MAX_MODULUS_SIZE = 312
IMAGE_SCN_MEM_WRITE = 0x80000000


def require(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit(f"FAIL: {message}")


def read_c_string(data: bytes, offset: int, maximum: int) -> str:
    raw = data[offset:offset + maximum]
    end = raw.find(b"\0")
    if end < 0:
        raise AssertionError(f"unterminated string at file offset 0x{offset:X}")
    return raw[:end].decode("ascii")


def main() -> int:
    if len(sys.argv) != 2:
        raise SystemExit("usage: verify_classic_client_772.py /path/to/Tibia.exe")

    path = Path(sys.argv[1]).resolve()
    require(path.is_file(), f"client executable not found: {path}")

    observed_hashes = {}
    for filename, expected in EXPECTED_FILES.items():
        artifact = path.with_name(filename)
        require(artifact.is_file(), f"required client artifact not found: {artifact}")
        digest = hashlib.sha256(artifact.read_bytes()).hexdigest()
        require(digest == expected, f"unexpected {filename} SHA-256: {digest}")
        observed_hashes[filename] = digest

    data = path.read_bytes()
    require(data[:2] == b"MZ", "missing DOS MZ signature")

    pe_offset = struct.unpack_from("<I", data, 0x3C)[0]
    require(data[pe_offset:pe_offset + 4] == b"PE\0\0", "missing PE signature")
    coff = pe_offset + 4
    machine, section_count, timestamp = struct.unpack_from("<HHI", data, coff)
    optional_size = struct.unpack_from("<H", data, coff + 16)[0]
    optional = coff + 20
    require(machine == 0x14C, f"expected x86 PE, got 0x{machine:04X}")
    require(struct.unpack_from("<H", data, optional)[0] == 0x10B, "expected PE32 optional header")
    image_base = struct.unpack_from("<I", data, optional + 28)[0]

    sections = []
    section_table = optional + optional_size
    for index in range(section_count):
        entry = section_table + index * 40
        name = data[entry:entry + 8].rstrip(b"\0").decode("ascii", errors="replace")
        virtual_size, virtual_address, raw_size, raw_offset = struct.unpack_from(
            "<IIII", data, entry + 8
        )
        characteristics = struct.unpack_from("<I", data, entry + 36)[0]
        sections.append(
            (name, virtual_address, max(virtual_size, raw_size), raw_size, raw_offset, characteristics)
        )

    def resolve_address(address: int):
        rva = address - image_base
        for name, section_rva, section_size, raw_size, raw_offset, characteristics in sections:
            if section_rva <= rva < section_rva + section_size:
                delta = rva - section_rva
                file_offset = raw_offset + delta if delta < raw_size else None
                return name, characteristics, file_offset
        raise AssertionError(f"address 0x{address:X} is not backed by a PE section")

    version_section, _, version_offset = resolve_address(VERSION_ADDRESS)
    require(version_offset is not None, "version address has no file backing")
    version = read_c_string(data, version_offset, 128)
    require(version.startswith("Version 7.72"), f"unexpected embedded version: {version!r}")

    endpoint_locations = []
    for index in range(ENDPOINT_COUNT):
        host_address = FIRST_HOST_ADDRESS + index * ENDPOINT_STRIDE
        port_address = FIRST_PORT_ADDRESS + index * ENDPOINT_STRIDE
        host_section, host_flags, host_offset = resolve_address(host_address)
        port_section, port_flags, port_offset = resolve_address(port_address)
        host_end_section, _, _ = resolve_address(host_address + MAX_HOST_SIZE - 1)
        port_end_section, _, _ = resolve_address(port_address + 3)
        require(
            host_section == host_end_section == port_section == port_end_section == ".data",
            f"endpoint {index} is not fully contained in .data",
        )
        require(
            host_flags & IMAGE_SCN_MEM_WRITE and port_flags & IMAGE_SCN_MEM_WRITE,
            f"endpoint {index} is not writable",
        )
        require(
            host_offset is None and port_offset is None,
            f"endpoint {index} unexpectedly has file-backed data",
        )
        endpoint_locations.append((host_address, port_address, host_section))

    modulus_section, _, modulus_offset = resolve_address(RSA_MODULUS_ADDRESS)
    require(modulus_offset is not None, "RSA modulus address has no file backing")
    modulus = read_c_string(data, modulus_offset, MAX_MODULUS_SIZE)
    require(modulus.isdecimal(), "embedded RSA modulus is not decimal")
    require(len(modulus) + 1 <= MAX_MODULUS_SIZE, "embedded RSA modulus exceeds source limit")

    for filename, digest in observed_hashes.items():
        print(f"CLIENT_ARTIFACT {filename} sha256={digest.upper()} PASS")
    print(f"PE machine=0x{machine:04X} image_base=0x{image_base:X} timestamp=0x{timestamp:08X} PASS")
    print(
        f"VERSION_ADDRESS 0x{VERSION_ADDRESS:X} section={version_section} "
        f"value={version!r} PASS"
    )
    for index, (host_address, port_address, section) in enumerate(endpoint_locations):
        print(
            f"ENDPOINT_{index} host_address=0x{host_address:X} "
            f"port_address=0x{port_address:X} section={section} runtime_only PASS"
        )
    print(
        "RSA_ADDRESS "
        f"0x{RSA_MODULUS_ADDRESS:X} section={modulus_section} chars={len(modulus)} "
        f"sha256={hashlib.sha256(modulus.encode('ascii')).hexdigest().upper()} PASS"
    )
    print("FUSION32_IPCHANGER_STATIC_ADDRESS_TABLE PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

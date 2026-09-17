#include "fusion32/protocol772/framed_connection.h"
#include "fusion32/protocol772/framing.h"
#include "fusion32/protocol772/tcp_transport.h"

#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <exception>
#include <functional>
#include <future>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <cerrno>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace {

using fusion32::protocol772::ClientFrameLimits;
using fusion32::protocol772::ConnectionState;
using fusion32::protocol772::EncodeFrame;
using fusion32::protocol772::FrameDecoder;
using fusion32::protocol772::FrameError;
using fusion32::protocol772::FramedConnection;
using fusion32::protocol772::FramedPacket;
using fusion32::protocol772::IoStatus;
using fusion32::protocol772::TcpTransport;
using fusion32::protocol772::kGameClientFrameLimits;
using fusion32::protocol772::kLoginClientFrameLimits;

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

#ifdef _WIN32
using TestSocket = SOCKET;
constexpr TestSocket kInvalidTestSocket = INVALID_SOCKET;

class TestSocketRuntime {
public:
    TestSocketRuntime() {
        WSADATA data{};
        const int error = WSAStartup(MAKEWORD(2, 2), &data);
        if (error != 0) {
            throw TestFailure("WSAStartup failed: " + std::to_string(error));
        }
    }
    ~TestSocketRuntime() { WSACleanup(); }
};

int TestSocketError() { return WSAGetLastError(); }
void CloseTestSocket(TestSocket socket) { closesocket(socket); }
int TestShutdownHow() { return SD_BOTH; }
#else
using TestSocket = int;
constexpr TestSocket kInvalidTestSocket = -1;

class TestSocketRuntime {};

int TestSocketError() { return errno; }
void CloseTestSocket(TestSocket socket) { close(socket); }
int TestShutdownHow() { return SHUT_RDWR; }
#endif

void SetTestTimeout(TestSocket socket) {
#ifdef _WIN32
    const DWORD timeout = 3000;
    CHECK(setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO,
                     reinterpret_cast<const char*>(&timeout), sizeof(timeout)) == 0);
    CHECK(setsockopt(socket, SOL_SOCKET, SO_SNDTIMEO,
                     reinterpret_cast<const char*>(&timeout), sizeof(timeout)) == 0);
#else
    timeval timeout{};
    timeout.tv_sec = 3;
    CHECK(setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) == 0);
    CHECK(setsockopt(socket, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) == 0);
#endif
}

void SendAll(TestSocket socket, const std::vector<std::uint8_t>& bytes) {
    std::size_t position = 0;
    while (position < bytes.size()) {
        const std::size_t remaining = bytes.size() - position;
#ifdef _WIN32
        const int sent = send(socket,
                              reinterpret_cast<const char*>(bytes.data() + position),
                              static_cast<int>(remaining), 0);
#else
        const int sent = static_cast<int>(send(socket, bytes.data() + position, remaining,
#ifdef MSG_NOSIGNAL
                                               MSG_NOSIGNAL
#else
                                               0
#endif
                                               ));
#endif
        if (sent <= 0) {
            throw TestFailure("test server send failed: " + std::to_string(TestSocketError()));
        }
        position += static_cast<std::size_t>(sent);
    }
}

std::vector<std::uint8_t> ReceiveExact(TestSocket socket, std::size_t size) {
    std::vector<std::uint8_t> bytes(size);
    std::size_t position = 0;
    while (position < size) {
#ifdef _WIN32
        const int received = recv(socket,
                                  reinterpret_cast<char*>(bytes.data() + position),
                                  static_cast<int>(size - position), 0);
#else
        const int received = static_cast<int>(recv(
            socket, bytes.data() + position, size - position, 0));
#endif
        if (received <= 0) {
            throw TestFailure("test server receive failed or closed");
        }
        position += static_cast<std::size_t>(received);
    }
    return bytes;
}

class LoopbackServer {
public:
    explicit LoopbackServer(std::function<void(TestSocket)> handler)
        : listener_(socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) {
        if (listener_ == kInvalidTestSocket) {
            throw TestFailure("listener socket creation failed");
        }

        int reuse = 1;
        CHECK(setsockopt(listener_, SOL_SOCKET, SO_REUSEADDR,
                         reinterpret_cast<const char*>(&reuse), sizeof(reuse)) == 0);

        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_port = 0;
        address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        CHECK(bind(listener_, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == 0);
        CHECK(listen(listener_, 1) == 0);

#ifdef _WIN32
        int address_size = static_cast<int>(sizeof(address));
#else
        socklen_t address_size = static_cast<socklen_t>(sizeof(address));
#endif
        CHECK(getsockname(listener_, reinterpret_cast<sockaddr*>(&address), &address_size) == 0);
        port_ = ntohs(address.sin_port);

        thread_ = std::thread([this, handler = std::move(handler)]() mutable {
            try {
                sockaddr_in peer{};
#ifdef _WIN32
                int peer_size = static_cast<int>(sizeof(peer));
#else
                socklen_t peer_size = static_cast<socklen_t>(sizeof(peer));
#endif
                TestSocket accepted = accept(
                    listener_, reinterpret_cast<sockaddr*>(&peer), &peer_size);
                if (accepted == kInvalidTestSocket) {
                    throw TestFailure("accept failed: " + std::to_string(TestSocketError()));
                }
                SetTestTimeout(accepted);
                try {
                    handler(accepted);
                } catch (...) {
                    shutdown(accepted, TestShutdownHow());
                    CloseTestSocket(accepted);
                    throw;
                }
                shutdown(accepted, TestShutdownHow());
                CloseTestSocket(accepted);
            } catch (...) {
                error_ = std::current_exception();
            }
        });
    }

    ~LoopbackServer() {
        if (thread_.joinable()) {
            shutdown(listener_, TestShutdownHow());
            CloseTestSocket(listener_);
            listener_ = kInvalidTestSocket;
            thread_.join();
        } else if (listener_ != kInvalidTestSocket) {
            CloseTestSocket(listener_);
        }
    }

    LoopbackServer(const LoopbackServer&) = delete;
    LoopbackServer& operator=(const LoopbackServer&) = delete;

    std::uint16_t port() const noexcept { return port_; }

    void Join() {
        if (thread_.joinable()) {
            thread_.join();
        }
        if (listener_ != kInvalidTestSocket) {
            CloseTestSocket(listener_);
            listener_ = kInvalidTestSocket;
        }
        if (error_) {
            std::rethrow_exception(error_);
        }
    }

private:
    TestSocket listener_ = kInvalidTestSocket;
    std::uint16_t port_ = 0;
    std::thread thread_;
    std::exception_ptr error_;
};

class ReservedUnlistenedPort {
public:
    ReservedUnlistenedPort() : socket_(socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) {
        CHECK(socket_ != kInvalidTestSocket);
        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_port = 0;
        address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        CHECK(bind(socket_, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == 0);
#ifdef _WIN32
        int address_size = static_cast<int>(sizeof(address));
#else
        socklen_t address_size = static_cast<socklen_t>(sizeof(address));
#endif
        CHECK(getsockname(socket_, reinterpret_cast<sockaddr*>(&address), &address_size) == 0);
        port_ = ntohs(address.sin_port);
    }

    ~ReservedUnlistenedPort() { CloseTestSocket(socket_); }
    std::uint16_t port() const noexcept { return port_; }

private:
    TestSocket socket_ = kInvalidTestSocket;
    std::uint16_t port_ = 0;
};

std::vector<std::uint8_t> WireFrame(std::initializer_list<std::uint8_t> payload) {
    const std::vector<std::uint8_t> bytes(payload);
    auto encoded = EncodeFrame(bytes, 2048);
    CHECK(encoded.ok());
    return encoded.bytes;
}

void Append(std::vector<std::uint8_t>* destination, const std::vector<std::uint8_t>& source) {
    destination->insert(destination->end(), source.begin(), source.end());
}

std::vector<FramedPacket> ReadUntilFrames(
    FramedConnection* connection,
    std::size_t expected_count) {
    std::vector<FramedPacket> frames;
    for (int attempt = 0; attempt < 16 && frames.size() < expected_count; ++attempt) {
        auto read = connection->ReadOnce();
        CHECK(read.io_status == IoStatus::Ok);
        CHECK(read.framing.ok());
        for (auto& frame : read.framing.frames) {
            frames.push_back(std::move(frame));
        }
    }
    CHECK(frames.size() == expected_count);
    return frames;
}

void TestPartialHeader() {
    FrameDecoder decoder(2048);
    const std::array<std::uint8_t, 1> first{{3}};
    auto partial = decoder.Feed(first.data(), first.size());
    CHECK(partial.ok());
    CHECK(partial.frames.empty());
    CHECK(partial.needs_more_data);
    CHECK(partial.buffered_bytes == 1);

    const std::array<std::uint8_t, 4> rest{{0, 10, 11, 12}};
    auto complete = decoder.Feed(rest.data(), rest.size());
    CHECK(complete.ok());
    CHECK(complete.frames.size() == 1);
    CHECK((complete.frames[0].payload == std::vector<std::uint8_t>{10, 11, 12}));
    CHECK(complete.buffered_bytes == 0);
}

void TestPartialPayload() {
    FrameDecoder decoder(2048);
    const std::array<std::uint8_t, 4> first{{4, 0, 1, 2}};
    auto partial = decoder.Feed(first.data(), first.size());
    CHECK(partial.ok());
    CHECK(partial.frames.empty());
    CHECK(partial.buffered_bytes == 4);

    const std::array<std::uint8_t, 2> rest{{3, 4}};
    auto complete = decoder.Feed(rest.data(), rest.size());
    CHECK(complete.frames.size() == 1);
    CHECK((complete.frames[0].payload == std::vector<std::uint8_t>{1, 2, 3, 4}));
}

void TestFrameSplitAcrossSeveralReads() {
    FrameDecoder decoder(2048);
    const auto wire = WireFrame({20, 21, 22, 23, 24});
    for (std::size_t i = 0; i + 1 < wire.size(); ++i) {
        auto result = decoder.Feed(&wire[i], 1);
        CHECK(result.ok());
        CHECK(result.frames.empty());
        CHECK(result.needs_more_data);
    }
    auto final = decoder.Feed(&wire.back(), 1);
    CHECK(final.frames.size() == 1);
    CHECK((final.frames[0].payload == std::vector<std::uint8_t>{20, 21, 22, 23, 24}));
}

void TestMultipleFramesAndOrder() {
    FrameDecoder decoder(2048);
    std::vector<std::uint8_t> wire;
    Append(&wire, WireFrame({1}));
    Append(&wire, WireFrame({2, 3}));
    Append(&wire, WireFrame({4, 5, 6}));
    auto result = decoder.Feed(wire);
    CHECK(result.ok());
    CHECK(result.frames.size() == 3);
    CHECK((result.frames[0].payload == std::vector<std::uint8_t>{1}));
    CHECK((result.frames[1].payload == std::vector<std::uint8_t>{2, 3}));
    CHECK((result.frames[2].payload == std::vector<std::uint8_t>{4, 5, 6}));
}

void TestCompleteFrameAndPartialNext() {
    FrameDecoder decoder(2048);
    std::vector<std::uint8_t> first = WireFrame({7, 8});
    const auto second = WireFrame({9, 10, 11});
    first.insert(first.end(), second.begin(), second.begin() + 3);

    auto result = decoder.Feed(first);
    CHECK(result.frames.size() == 1);
    CHECK((result.frames[0].payload == std::vector<std::uint8_t>{7, 8}));
    CHECK(result.needs_more_data);
    CHECK(result.buffered_bytes == 3);

    auto completed = decoder.Feed(second.data() + 3, second.size() - 3);
    CHECK(completed.frames.size() == 1);
    CHECK((completed.frames[0].payload == std::vector<std::uint8_t>{9, 10, 11}));
}

void TestMalformedZeroLengthPreservesInput() {
    FrameDecoder decoder(2048);
    const std::array<std::uint8_t, 4> bytes{{0, 0, 0xAA, 0xBB}};
    auto result = decoder.Feed(bytes.data(), bytes.size());
    CHECK(result.error == FrameError::ZeroLength);
    CHECK(result.input_bytes_consumed == 2);
    CHECK(result.buffered_bytes == 2);
    CHECK((result.unconsumed_bytes == std::vector<std::uint8_t>{0xAA, 0xBB}));
    CHECK((decoder.buffered_data() == std::vector<std::uint8_t>{0, 0}));

    const std::array<std::uint8_t, 1> later{{0xCC}};
    auto failed_again = decoder.Feed(later.data(), later.size());
    CHECK(failed_again.error == FrameError::ZeroLength);
    CHECK((failed_again.unconsumed_bytes == std::vector<std::uint8_t>{0xCC}));
}

void TestUnsupportedFrameSize() {
    FrameDecoder decoder(8);
    const std::array<std::uint8_t, 2> header{{9, 0}};
    auto result = decoder.Feed(header.data(), header.size());
    CHECK(result.error == FrameError::ExceedsConfiguredMaximum);
    CHECK(result.buffered_bytes == 2);
}

void TestBufferPreservedUntilComplete() {
    FrameDecoder decoder(2048);
    const auto wire = WireFrame({31, 32, 33, 34});
    auto first = decoder.Feed(wire.data(), 3);
    CHECK(first.buffered_bytes == 3);
    CHECK(decoder.buffered_data()[2] == 31);
    auto second = decoder.Feed(wire.data() + 3, 1);
    CHECK(second.buffered_bytes == 4);
    CHECK(decoder.buffered_data()[3] == 32);
    auto third = decoder.Feed(wire.data() + 4, wire.size() - 4);
    CHECK(third.frames.size() == 1);
    CHECK(third.buffered_bytes == 0);
}

void TestWriteFraming() {
    const std::vector<std::uint8_t> payload{0xA1, 0xB2, 0xC3};
    auto result = EncodeFrame(payload, 2048);
    CHECK(result.ok());
    CHECK((result.bytes == std::vector<std::uint8_t>{3, 0, 0xA1, 0xB2, 0xC3}));
}

void TestWriteFramingRejectsInvalidLengths() {
    const std::vector<std::uint8_t> empty;
    CHECK(EncodeFrame(empty, 2048).error == FrameError::ZeroLength);
    const std::vector<std::uint8_t> too_large(9, 1);
    CHECK(EncodeFrame(too_large, 8).error == FrameError::ExceedsConfiguredMaximum);
    const std::vector<std::uint8_t> cannot_fit(65536, 1);
    CHECK(EncodeFrame(cannot_fit, 65535).error == FrameError::PayloadTooLargeForHeader);
}

void TestEndpointProfileBoundaries() {
    const std::vector<std::uint8_t> client_payload(
        kGameClientFrameLimits.max_send_payload, 0xA5);
    auto client_frame = EncodeFrame(
        client_payload, kGameClientFrameLimits.max_send_payload);
    CHECK(client_frame.ok());
    CHECK(client_frame.bytes[0] == 0x00);
    CHECK(client_frame.bytes[1] == 0x08);
    CHECK(EncodeFrame(
        std::vector<std::uint8_t>(kGameClientFrameLimits.max_send_payload + 1, 1),
        kGameClientFrameLimits.max_send_payload).error
        == FrameError::ExceedsConfiguredMaximum);

    FrameDecoder login_decoder(kLoginClientFrameLimits.max_receive_payload);
    auto login_max = EncodeFrame(
        std::vector<std::uint8_t>(kLoginClientFrameLimits.max_receive_payload, 2),
        kLoginClientFrameLimits.max_receive_payload);
    CHECK(login_max.ok());
    CHECK(login_decoder.Feed(login_max.bytes).frames.size() == 1);

    FrameDecoder game_decoder(kGameClientFrameLimits.max_receive_payload);
    auto game_max = EncodeFrame(
        std::vector<std::uint8_t>(kGameClientFrameLimits.max_receive_payload, 3),
        kGameClientFrameLimits.max_receive_payload);
    CHECK(game_max.ok());
    CHECK(game_max.bytes[0] == 0x08);
    CHECK(game_max.bytes[1] == 0x40);
    CHECK(game_decoder.Feed(game_max.bytes).frames.size() == 1);
}

void TestFinishDetectsTruncation() {
    FrameDecoder decoder(2048);
    const std::array<std::uint8_t, 3> bytes{{4, 0, 1}};
    CHECK(decoder.Feed(bytes.data(), bytes.size()).needs_more_data);
    auto finished = decoder.Finish();
    CHECK(finished.error == FrameError::TruncatedFrame);
    CHECK(finished.buffered_bytes == 3);

    FrameDecoder complete_decoder(2048);
    CHECK(complete_decoder.Finish().end_of_stream);
    const std::array<std::uint8_t, 1> after_eof{{1}};
    auto rejected = complete_decoder.Feed(after_eof.data(), after_eof.size());
    CHECK(rejected.error == FrameError::StreamFinished);
    CHECK((rejected.unconsumed_bytes == std::vector<std::uint8_t>{1}));
}

void TestSuccessfulLocalConnectionAndCleanDisconnect() {
    std::atomic<bool> saw_eof{false};
    LoopbackServer server([&](TestSocket socket) {
        std::uint8_t byte = 0;
#ifdef _WIN32
        const int received = recv(socket, reinterpret_cast<char*>(&byte), 1, 0);
#else
        const int received = static_cast<int>(recv(socket, &byte, 1, 0));
#endif
        saw_eof = received == 0;
    });

    TcpTransport transport;
    auto connected = transport.Connect("127.0.0.1", server.port());
    CHECK(connected.ok());
    CHECK(transport.state() == ConnectionState::Connected);
    transport.Disconnect();
    CHECK(transport.state() == ConnectionState::Disconnected);
    server.Join();
    CHECK(saw_eof.load());
}

void TestRefusedConnection() {
    ReservedUnlistenedPort reserved;
    TcpTransport transport;
    auto result = transport.Connect("127.0.0.1", reserved.port(), std::chrono::milliseconds(1000));
    CHECK(!result.ok());
    CHECK(result.status == IoStatus::Failed || result.status == IoStatus::TimedOut);
    CHECK(transport.state() == ConnectionState::Failed);
}

void TestReadTimeoutIsSeparateFromConnectTimeout() {
    // A connection that never sends anything. The read must come back on the
    // short timeout it was given, not on the generous one Connect used, which
    // is what lets an interactive caller poll its own outbound queue.
    LoopbackServer server([](TestSocket) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1500));
    });

    TcpTransport transport;
    CHECK(transport.Connect("127.0.0.1", server.port(),
                            std::chrono::milliseconds(3000)).ok());
    CHECK(transport.SetReadTimeout(std::chrono::milliseconds(80)));

    const auto started = std::chrono::steady_clock::now();
    auto read = transport.ReadSome();
    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - started);

    CHECK(read.status == IoStatus::TimedOut);
    // Comfortably under the 3000 ms connect timeout, and generous enough that a
    // loaded scheduler does not turn this into a flaky test.
    CHECK(elapsed < std::chrono::milliseconds(1200));

    transport.Disconnect();
    server.Join();
}

void TestReadTimeoutRejectsUnusableValues() {
    TcpTransport disconnected;
    // Nothing to apply the option to yet.
    CHECK(!disconnected.SetReadTimeout(std::chrono::milliseconds(50)));

    LoopbackServer server([](TestSocket) {});
    TcpTransport transport;
    CHECK(transport.Connect("127.0.0.1", server.port()).ok());
    CHECK(!transport.SetReadTimeout(std::chrono::milliseconds(0)));
    CHECK(!transport.SetReadTimeout(std::chrono::milliseconds(-5)));
    // A rejected value must leave the connection usable.
    CHECK(transport.connected());
    transport.Disconnect();
    server.Join();
}

void TestRemoteDisconnect() {
    LoopbackServer server([](TestSocket) {});
    TcpTransport transport;
    CHECK(transport.Connect("127.0.0.1", server.port()).ok());
    server.Join();
    auto read = transport.ReadSome();
    CHECK(read.status == IoStatus::RemoteClosed);
    CHECK(transport.state() == ConnectionState::RemoteClosed);
}

void TestLoopbackPartialHeaderAndPayload() {
    std::promise<void> sent_header_byte;
    std::promise<void> continue_after_header;
    std::promise<void> sent_payload_part;
    std::promise<void> continue_after_payload;
    auto continue_header = continue_after_header.get_future().share();
    auto continue_payload = continue_after_payload.get_future().share();

    LoopbackServer server([&](TestSocket socket) {
        const auto wire = WireFrame({40, 41, 42, 43});
        SendAll(socket, {wire[0]});
        sent_header_byte.set_value();
        continue_header.wait();
        SendAll(socket, {wire[1], wire[2], wire[3]});
        sent_payload_part.set_value();
        continue_payload.wait();
        SendAll(socket, {wire[4], wire[5]});
    });

    FramedConnection connection(kGameClientFrameLimits, 4096);
    CHECK(connection.Connect("127.0.0.1", server.port()).ok());
    sent_header_byte.get_future().wait();
    auto header = connection.ReadOnce();
    CHECK(header.io_status == IoStatus::Ok);
    CHECK(header.framing.frames.empty());
    CHECK(header.framing.buffered_bytes == 1);

    continue_after_header.set_value();
    sent_payload_part.get_future().wait();
    auto payload_part = connection.ReadOnce();
    CHECK(payload_part.framing.frames.empty());
    CHECK(payload_part.framing.buffered_bytes == 4);

    continue_after_payload.set_value();
    auto complete = ReadUntilFrames(&connection, 1);
    CHECK((complete[0].payload == std::vector<std::uint8_t>{40, 41, 42, 43}));
    connection.Disconnect();
    server.Join();
}

void TestLoopbackMultipleFramesOneWrite() {
    LoopbackServer server([](TestSocket socket) {
        std::vector<std::uint8_t> wire;
        Append(&wire, WireFrame({50}));
        Append(&wire, WireFrame({51, 52}));
        Append(&wire, WireFrame({53, 54, 55}));
        SendAll(socket, wire);
    });

    FramedConnection connection(kGameClientFrameLimits, 4096);
    CHECK(connection.Connect("127.0.0.1", server.port()).ok());
    auto frames = ReadUntilFrames(&connection, 3);
    CHECK((frames[0].payload == std::vector<std::uint8_t>{50}));
    CHECK((frames[1].payload == std::vector<std::uint8_t>{51, 52}));
    CHECK((frames[2].payload == std::vector<std::uint8_t>{53, 54, 55}));
    connection.Disconnect();
    server.Join();
}

void TestLoopbackWriteFraming() {
    std::vector<std::uint8_t> received;
    LoopbackServer server([&](TestSocket socket) {
        received = ReceiveExact(socket, 5);
    });

    FramedConnection connection(ClientFrameLimits{2048, 2048});
    CHECK(connection.Connect("127.0.0.1", server.port()).ok());
    CHECK(connection.SendFrame(std::vector<std::uint8_t>{60, 61, 62}).ok());
    server.Join();
    CHECK((received == std::vector<std::uint8_t>{3, 0, 60, 61, 62}));
    connection.Disconnect();
}

void TestRemoteDisconnectWithPartialFrame() {
    LoopbackServer server([](TestSocket socket) {
        SendAll(socket, {4, 0, 70});
    });

    FramedConnection connection(kGameClientFrameLimits);
    CHECK(connection.Connect("127.0.0.1", server.port()).ok());
    auto partial = connection.ReadOnce();
    CHECK(partial.io_status == IoStatus::Ok);
    CHECK(partial.framing.needs_more_data);
    server.Join();
    auto closed = connection.ReadOnce();
    CHECK(closed.io_status == IoStatus::RemoteClosed);
    CHECK(closed.framing.error == FrameError::TruncatedFrame);
}

void TestReconnectLifecycle() {
    TcpTransport transport;
    {
        LoopbackServer first([](TestSocket socket) {
            static_cast<void>(ReceiveExact(socket, 1));
        });
        CHECK(transport.Connect("127.0.0.1", first.port()).ok());
        CHECK(transport.WriteAll(std::vector<std::uint8_t>{1}).ok());
        transport.Disconnect();
        first.Join();
    }
    CHECK(transport.state() == ConnectionState::Disconnected);

    {
        LoopbackServer second([](TestSocket socket) {
            static_cast<void>(ReceiveExact(socket, 1));
        });
        CHECK(transport.Connect("127.0.0.1", second.port()).ok());
        CHECK(transport.WriteAll(std::vector<std::uint8_t>{2}).ok());
        transport.Disconnect();
        second.Join();
    }
    CHECK(transport.state() == ConnectionState::Disconnected);
}

struct TestCase {
    const char* name;
    void (*function)();
};

}  // namespace

int main() {
    try {
        [[maybe_unused]] TestSocketRuntime socket_runtime;
        const std::array<TestCase, 21> tests{{
            {"partial header", TestPartialHeader},
            {"partial payload", TestPartialPayload},
            {"frame split across several reads", TestFrameSplitAcrossSeveralReads},
            {"multiple frames and order", TestMultipleFramesAndOrder},
            {"complete frame plus partial next", TestCompleteFrameAndPartialNext},
            {"zero length preserves bytes", TestMalformedZeroLengthPreservesInput},
            {"unsupported frame size", TestUnsupportedFrameSize},
            {"incomplete buffer preservation", TestBufferPreservedUntilComplete},
            {"correct write framing", TestWriteFraming},
            {"write framing rejects invalid lengths", TestWriteFramingRejectsInvalidLengths},
            {"endpoint profile boundaries", TestEndpointProfileBoundaries},
            {"finish detects truncation", TestFinishDetectsTruncation},
            {"local connection and clean disconnect", TestSuccessfulLocalConnectionAndCleanDisconnect},
            {"refused connection", TestRefusedConnection},
            {"read timeout separate from connect timeout",
             TestReadTimeoutIsSeparateFromConnectTimeout},
            {"read timeout rejects unusable values", TestReadTimeoutRejectsUnusableValues},
            {"remote disconnect", TestRemoteDisconnect},
            {"loopback partial header and payload", TestLoopbackPartialHeaderAndPayload},
            {"loopback multiple frames", TestLoopbackMultipleFramesOneWrite},
            {"loopback write framing", TestLoopbackWriteFraming},
            {"reconnect lifecycle", TestReconnectLifecycle},
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

        // This test intentionally runs after the table so its two-stage remote
        // close cannot affect another loopback fixture if the platform delays FIN.
        try {
            TestRemoteDisconnectWithPartialFrame();
            std::cout << "PASS remote disconnect with partial frame\n";
        } catch (const std::exception& error) {
            ++failures;
            std::cerr << "FAILED remote disconnect with partial frame: "
                      << error.what() << '\n';
        }

        // Counted, not written down. A hardcoded total silently under-reports
        // the moment a case is added, and these counts are quoted as evidence.
        // The +1 is the case above, which runs outside the table on purpose.
        const int total = static_cast<int>(tests.size()) + 1;
        std::cout << "SUMMARY passed=" << (total - failures)
                  << " failed=" << failures << " total=" << total << '\n';
        return failures == 0 ? 0 : 1;
    } catch (const std::exception& error) {
        std::cerr << "FATAL test setup: " << error.what() << '\n';
        return 2;
    }
}

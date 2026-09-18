#ifndef FUSION32_PROTOCOL772_CHAT_LOG_H
#define FUSION32_PROTOCOL772_CHAT_LOG_H

#include <cstddef>
#include <cstdint>
#include <deque>
#include <string>

namespace fusion32::protocol772 {

// A bounded, ordered transcript of what the player should be able to read.
//
// This is PRESENTATION STATE, deliberately kept out of WorldState. WorldState
// is what the server says the world is; a chat transcript is what this client
// chose to keep on screen, and nothing in Fusion32 stores or replays it. Mixing
// the two would make an authoritative structure carry client UI history.
//
// It lives in ClientCore rather than in the Unreal module for one reason: the
// rules worth getting right here are ordering, bounding and eviction, and those
// are testable without a renderer. Whether the operator can *read* the result is
// a separate question that only a live run can answer.
//
// Not thread-safe by design. One owner, on one thread.
class ChatLog {
public:
    enum class EntryKind : std::uint8_t {
        // Something a creature said. `sender` may be empty: moveuse.cc passes
        // "" for the ANIMAL modes, and the channel form blanks the sender for
        // ANONYMOUS_CHANNELCALL.
        Speech,
        // Something the server told this player directly, via SV_CMD_MESSAGE.
        // Distinct from a protocol diagnostic, which is for the developer and
        // must never reach a player surface.
        ServerMessage,
    };

    struct Entry {
        EntryKind kind = EntryKind::Speech;
        std::string sender;
        // A human-readable mode name, never a protocol number.
        std::string mode;
        std::string text;
        // Receive order, monotonic and never reused. Ordering is by arrival and
        // is deliberately NOT affected by whether a speaker could be resolved to
        // a creature: a message that failed resolution keeps its place.
        std::uint64_t sequence = 0;
    };

    // REAL33D_UI_BEHAVIOUR, not 7.72 parity. Nothing in Fusion32 states how many
    // lines a client should retain, because the server keeps no transcript at
    // all. Sixty is enough to scroll back through an exchange and small enough
    // that a long session cannot grow without limit.
    static constexpr std::size_t kDefaultCapacity = 60;

    explicit ChatLog(std::size_t capacity = kDefaultCapacity);

    void AddSpeech(const std::string& sender,
                   const std::string& mode,
                   const std::string& text);
    void AddServerMessage(const std::string& mode, const std::string& text);

    // Everything is dropped. Used when a session ends: a transcript from a
    // previous connection must not appear to belong to the new one.
    void Clear();

    const std::deque<Entry>& entries() const noexcept { return entries_; }
    std::size_t size() const noexcept { return entries_.size(); }
    bool empty() const noexcept { return entries_.empty(); }
    std::size_t capacity() const noexcept { return capacity_; }
    // Counts everything ever accepted, including what has since been evicted,
    // so a test can prove exactly one entry per event.
    std::uint64_t accepted() const noexcept { return next_sequence_; }

    // One display line. Say omits its own name because almost every line is a
    // Say and repeating it is noise; anything else is labelled, because a yell
    // and a whisper reading identically would misinform the player.
    static std::string Format(const Entry& entry);

private:
    void Push(Entry entry);

    std::deque<Entry> entries_;
    std::size_t capacity_;
    std::uint64_t next_sequence_ = 0;
};

}  // namespace fusion32::protocol772

#endif

#include "fusion32/protocol772/chat_log.h"

namespace fusion32::protocol772 {

ChatLog::ChatLog(std::size_t capacity)
    // A zero-capacity log would silently discard everything, which reads as a
    // broken client rather than as a configuration mistake. One line minimum.
    : capacity_(capacity == 0 ? 1 : capacity) {}

void ChatLog::Push(Entry entry) {
    entry.sequence = next_sequence_++;
    entries_.push_back(std::move(entry));
    // Oldest first, so what the player is reading now survives.
    while (entries_.size() > capacity_) {
        entries_.pop_front();
    }
}

void ChatLog::AddSpeech(const std::string& sender,
                        const std::string& mode,
                        const std::string& text) {
    Entry entry;
    entry.kind = EntryKind::Speech;
    entry.sender = sender;
    entry.mode = mode;
    entry.text = text;
    Push(std::move(entry));
}

void ChatLog::AddServerMessage(const std::string& mode, const std::string& text) {
    Entry entry;
    entry.kind = EntryKind::ServerMessage;
    entry.mode = mode;
    entry.text = text;
    Push(std::move(entry));
}

void ChatLog::AddClientNotice(const std::string& text) {
    Entry entry;
    entry.kind = EntryKind::ClientNotice;
    entry.text = text;
    Push(std::move(entry));
}

void ChatLog::Clear() {
    entries_.clear();
    // The sequence deliberately keeps counting. It identifies an arrival, not a
    // slot, and reusing numbers across a reconnect would make two different
    // messages indistinguishable in evidence.
}

std::string ChatLog::Format(const Entry& entry) {
    if (entry.kind == EntryKind::ServerMessage) {
        return "Server: " + entry.text;
    }
    // Marked as coming from the client, not the server, and not named after a
    // creature either, so it cannot be mistaken for something that was said.
    if (entry.kind == EntryKind::ClientNotice) {
        return "* " + entry.text;
    }

    // An unnamed speaker is a real case rather than an error, so it gets a
    // readable stand-in instead of a stray leading colon.
    std::string line = entry.sender.empty() ? std::string("Someone") : entry.sender;
    if (!entry.mode.empty() && entry.mode != "Say") {
        line += " [" + entry.mode + "]";
    }
    line += ": ";
    line += entry.text;
    return line;
}

}  // namespace fusion32::protocol772

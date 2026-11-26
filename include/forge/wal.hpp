// The write-ahead log: every change, safely on disk before we say "saved".
// (component WAL; issues WAL-1, WAL-2)   The file format is in docs/FORMAT.md.
#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <span>
#include <vector>

namespace forge {

enum class RecordType : std::uint8_t {
    put = 1,  // payload: one chunk, as encode_chunk() makes it
};

struct Record {
    RecordType type;
    std::vector<std::byte> payload;

    bool operator==(const Record&) const = default;
};

/// CRC-32 (the common "IEEE" one, as used by zlib and Ethernet).
/// crc32 of the 9 bytes "123456789" is 0xCBF43926.
std::uint32_t crc32(std::span<const std::byte> data);

/// Appends records to a log file. Owns the open file (closes it when
/// destroyed). Move-only.
class WalWriter {
public:
    /// Opens `file` for appending, creating it if needed. Existing records
    /// are kept: new ones go after them. Throws std::system_error.
    explicit WalWriter(const std::filesystem::path& file);
    ~WalWriter();
    WalWriter(WalWriter&& other) noexcept;
    WalWriter& operator=(WalWriter&& other) noexcept;
    WalWriter(const WalWriter&) = delete;
    WalWriter& operator=(const WalWriter&) = delete;

    /// Writes one record at the end of the file. Payloads over 64 MiB:
    /// std::invalid_argument. Write errors: std::system_error.
    /// This does NOT fsync: call sync() for that.
    void append(RecordType type, std::span<const std::byte> payload);

    /// fsync: everything appended so far is on the disk itself, not just in
    /// the kernel's memory.
    void sync();

    /// The size of the file in bytes (old records included).
    std::uint64_t size() const;

private:
    int fd_ = -1;
    std::uint64_t size_ = 0;
};

/// Everything read_wal() found in a log file.
struct WalContents {
    std::vector<Record> records;   // every complete, undamaged record, in order
    std::uint64_t valid_bytes = 0; // where the last good record ends
    bool damaged_tail = false;     // true if there were bytes after it that aren't a good record
};

/// Reads a log file. Stops at the first record that is cut short, has a
/// wrong CRC, or claims a payload over 64 MiB: that's the "damaged tail" a
/// crash in the middle of append() leaves. A missing file: empty contents.
/// Throws std::system_error only if the file exists but can't be read.
WalContents read_wal(const std::filesystem::path& file);

}  // namespace forge

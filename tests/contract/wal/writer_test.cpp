// Contract tests for WAL-1: crc32 and WalWriter.   (owner: lead)
// Switched on when src/wal/stub_writer.cpp is deleted.
// The record format is in docs/FORMAT.md.
#include <gtest/gtest.h>

#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <type_traits>
#include <vector>

#include "forge/wal.hpp"

using namespace forge;
namespace fs = std::filesystem;

namespace {

std::vector<std::byte> bytes(std::string_view s) {
    std::vector<std::byte> out(s.size());
    std::memcpy(out.data(), s.data(), s.size());
    return out;
}

std::vector<unsigned char> read_file(const fs::path& p) {
    std::ifstream f(p, std::ios::binary);
    return {std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>()};
}

/// A fresh, empty temporary directory for one test.
fs::path temp_dir() {
    fs::path d = fs::temp_directory_path() /
                 ("forge-wal-test-" + std::to_string(::getpid()) + "-" +
                  ::testing::UnitTest::GetInstance()->current_test_info()->name());
    fs::remove_all(d);
    fs::create_directories(d);
    return d;
}

std::size_t open_fd_count() {
    return static_cast<std::size_t>(std::distance(fs::directory_iterator("/proc/self/fd"), fs::directory_iterator{}));
}

/// The bytes one record should have, built by hand from docs/FORMAT.md.
std::vector<unsigned char> expected_record(RecordType type, std::string_view payload) {
    std::vector<std::byte> covered = {static_cast<std::byte>(type)};
    for (char c : payload) covered.push_back(static_cast<std::byte>(c));
    std::uint32_t len = static_cast<std::uint32_t>(payload.size()), crc = crc32(covered);
    std::vector<unsigned char> out;
    for (int i = 0; i < 4; ++i) out.push_back(static_cast<unsigned char>(len >> (8 * i)));
    for (int i = 0; i < 4; ++i) out.push_back(static_cast<unsigned char>(crc >> (8 * i)));
    out.push_back(static_cast<unsigned char>(type));
    out.insert(out.end(), payload.begin(), payload.end());
    return out;
}

}  // namespace

TEST(Crc32, KnownValues) {
    EXPECT_EQ(crc32(bytes("123456789")), 0xCBF43926u) << "the standard check value";
    EXPECT_EQ(crc32(bytes("")), 0u);
    EXPECT_EQ(crc32(bytes("The quick brown fox jumps over the lazy dog")), 0x414FA339u);
}

TEST(WalWriter, IsMoveOnly) {
    static_assert(!std::is_copy_constructible_v<WalWriter>);
    static_assert(std::is_nothrow_move_constructible_v<WalWriter>);
}

TEST(WalWriter, WritesOneRecordExactly) {
    fs::path file = temp_dir() / "log";
    {
        WalWriter w(file);
        EXPECT_EQ(w.size(), 0u);
        w.append(RecordType::put, bytes("hello"));
        EXPECT_EQ(w.size(), 14u) << "9 bytes of header + 5 of payload";
    }
    auto want = expected_record(RecordType::put, "hello");
    EXPECT_EQ(read_file(file), want);
    // Byte by byte: length 5, little-endian; then the CRC; then the type.
    EXPECT_EQ(want[0], 5);
    EXPECT_EQ(want[1], 0);
    EXPECT_EQ(want[8], 1);
}

TEST(WalWriter, RecordsFollowEachOther) {
    fs::path file = temp_dir() / "log";
    std::vector<unsigned char> want;
    {
        WalWriter w(file);
        for (const std::string& p : std::vector<std::string>{"a", "", std::string(1000, 'z'), std::string("bin\0ary", 7)}) {
            w.append(RecordType::put, bytes(p));
            auto rec = expected_record(RecordType::put, p);
            want.insert(want.end(), rec.begin(), rec.end());
        }
        w.sync();
        EXPECT_EQ(w.size(), want.size());
    }
    EXPECT_EQ(read_file(file), want);
}

TEST(WalWriter, ReopeningAppendsAfterTheOldRecords) {
    fs::path file = temp_dir() / "log";
    { WalWriter(file).append(RecordType::put, bytes("first")); }
    {
        WalWriter w(file);
        EXPECT_EQ(w.size(), 14u) << "size() counts the records already in the file";
        w.append(RecordType::put, bytes("second"));
        EXPECT_EQ(w.size(), 29u);
    }
    auto want = expected_record(RecordType::put, "first");
    auto second = expected_record(RecordType::put, "second");
    want.insert(want.end(), second.begin(), second.end());
    EXPECT_EQ(read_file(file), want);
}

TEST(WalWriter, AMovedWriterKeepsWorking) {
    fs::path dir = temp_dir();
    fs::path file = dir / "log";
    {
        WalWriter a(file);
        WalWriter b(std::move(a));
        b.append(RecordType::put, bytes("x"));
        WalWriter c(dir / "other");
        c = std::move(b);
        c.append(RecordType::put, bytes("y"));
        EXPECT_EQ(c.size(), 20u);
    }
    EXPECT_EQ(read_file(file).size(), 20u);
}

TEST(WalWriter, ClosesItsFile) {
    fs::path dir = temp_dir();
    std::size_t before = open_fd_count();
    {
        WalWriter a(dir / "log");
        WalWriter b(std::move(a));
        WalWriter c(dir / "other");
        c = std::move(b);  // c's own file must be closed here
    }
    EXPECT_EQ(open_fd_count(), before) << "a file descriptor leaked";
}

TEST(WalWriter, RefusesPayloadsOver64MiB) {
    WalWriter w(temp_dir() / "log");
    std::vector<std::byte> big(64 * 1024 * 1024 + 1);
    EXPECT_THROW(w.append(RecordType::put, big), std::invalid_argument);
    EXPECT_EQ(w.size(), 0u);
}

TEST(WalWriter, AMissingDirectoryThrows) {
    EXPECT_THROW(WalWriter(temp_dir() / "no" / "such" / "dir" / "log"), std::system_error);
}

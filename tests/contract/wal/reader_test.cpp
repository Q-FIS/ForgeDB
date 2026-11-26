// Contract tests for WAL-2: read_wal.   (owner: lead)
// Switched on when src/wal/stub_reader.cpp is deleted. Needs WAL-1.
#include <gtest/gtest.h>

#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
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

fs::path temp_dir() {
    fs::path d = fs::temp_directory_path() /
                 ("forge-wal-read-" + std::to_string(::getpid()) + "-" +
                  ::testing::UnitTest::GetInstance()->current_test_info()->name());
    fs::remove_all(d);
    fs::create_directories(d);
    return d;
}

const std::vector<std::string> kPayloads = {"first record", "", std::string(5000, 'm'), "last"};

/// Writes kPayloads; returns the file size after each record.
std::vector<std::uint64_t> write_log(const fs::path& file) {
    WalWriter w(file);
    std::vector<std::uint64_t> ends;
    for (const auto& p : kPayloads) {
        w.append(RecordType::put, bytes(p));
        ends.push_back(w.size());
    }
    return ends;
}

std::vector<Record> expected(std::size_t n) {
    std::vector<Record> out;
    for (std::size_t i = 0; i < n; ++i) out.push_back({RecordType::put, bytes(kPayloads[i])});
    return out;
}

void flip_byte(const fs::path& file, std::uint64_t offset) {
    std::fstream f(file, std::ios::in | std::ios::out | std::ios::binary);
    f.seekg(static_cast<std::streamoff>(offset));
    char c;
    f.get(c);
    f.seekp(static_cast<std::streamoff>(offset));
    f.put(static_cast<char>(c ^ 0x55));
}

void write_raw(const fs::path& file, const std::string& data) {
    std::ofstream(file, std::ios::binary) << data;
}

}  // namespace

TEST(ReadWal, AMissingFileIsEmpty) {
    WalContents c = read_wal(temp_dir() / "nothing-here");
    EXPECT_TRUE(c.records.empty());
    EXPECT_EQ(c.valid_bytes, 0u);
    EXPECT_FALSE(c.damaged_tail);
}

TEST(ReadWal, AnEmptyFileIsEmpty) {
    fs::path file = temp_dir() / "log";
    write_raw(file, "");
    WalContents c = read_wal(file);
    EXPECT_TRUE(c.records.empty());
    EXPECT_FALSE(c.damaged_tail);
}

TEST(ReadWal, ReadsBackEveryRecord) {
    fs::path file = temp_dir() / "log";
    auto ends = write_log(file);
    WalContents c = read_wal(file);
    EXPECT_EQ(c.records, expected(4));
    EXPECT_EQ(c.valid_bytes, ends.back());
    EXPECT_FALSE(c.damaged_tail);
}

TEST(ReadWal, ALastRecordCutShortAnywhereIsADamagedTail) {
    // A crash in the middle of append() leaves part of a record. Try every cut.
    fs::path dir = temp_dir();
    auto ends = write_log(dir / "full");
    for (std::uint64_t cut = ends[2] + 1; cut < ends[3]; ++cut) {
        fs::path file = dir / "cut";
        fs::copy_file(dir / "full", file, fs::copy_options::overwrite_existing);
        fs::resize_file(file, cut);
        WalContents c = read_wal(file);
        ASSERT_EQ(c.records, expected(3)) << "cut at byte " << cut;
        EXPECT_EQ(c.valid_bytes, ends[2]) << "cut at byte " << cut;
        EXPECT_TRUE(c.damaged_tail) << "cut at byte " << cut;
    }
}

TEST(ReadWal, AWrongCrcStopsTheReading) {
    fs::path file = temp_dir() / "log";
    auto ends = write_log(file);
    // Record 2 is empty (just its 9-byte header) and ends at ends[1]; record 3
    // starts there. Damage a byte inside record 3's payload (the 5000 m's).
    flip_byte(file, ends[1] + 9 + 100);
    WalContents c = read_wal(file);
    EXPECT_EQ(c.records, expected(2));
    EXPECT_EQ(c.valid_bytes, ends[1]);
    EXPECT_TRUE(c.damaged_tail);
}

TEST(ReadWal, ADamagedTypeByteIsCaughtByTheCrc) {
    fs::path file = temp_dir() / "log";
    auto ends = write_log(file);
    flip_byte(file, 8);  // the type byte of the first record
    WalContents c = read_wal(file);
    EXPECT_TRUE(c.records.empty());
    EXPECT_EQ(c.valid_bytes, 0u);
    EXPECT_TRUE(c.damaged_tail);
}

TEST(ReadWal, AHugeLengthIsDamageNotAnAllocation) {
    fs::path file = temp_dir() / "log";
    write_raw(file, std::string("\xff\xff\xff\xff\0\0\0\0\x01", 9) + "some bytes");
    WalContents c = read_wal(file);
    EXPECT_TRUE(c.records.empty());
    EXPECT_TRUE(c.damaged_tail);
}

TEST(ReadWal, AFewGarbageBytesAreADamagedTail) {
    fs::path file = temp_dir() / "log";
    write_raw(file, "junk!");
    WalContents c = read_wal(file);
    EXPECT_TRUE(c.records.empty());
    EXPECT_EQ(c.valid_bytes, 0u);
    EXPECT_TRUE(c.damaged_tail);
}

TEST(ReadWal, WritingAfterADamagedTailWasCutOff) {
    // What recovery (WAL-4) will do: cut the file at valid_bytes, then append.
    fs::path file = temp_dir() / "log";
    auto ends = write_log(file);
    fs::resize_file(file, ends[3] - 2);
    WalContents c = read_wal(file);
    fs::resize_file(file, c.valid_bytes);
    WalWriter(file).append(RecordType::put, bytes("after"));
    WalContents again = read_wal(file);
    auto want = expected(3);
    want.push_back({RecordType::put, bytes("after")});
    EXPECT_EQ(again.records, want);
    EXPECT_FALSE(again.damaged_tail);
}

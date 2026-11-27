// Contract tests for SNAP-3: write_snapshot and read_snapshot.   (owner: lead)
// Switched on when src/snap/stub_snapshot.cpp is deleted. Needs SNAP-1, SNAP-2 and WAL-1 (crc32).
#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>

#include "forge/store.hpp"

using namespace forge;
namespace fs = std::filesystem;

namespace {

fs::path temp_dir() {
    fs::path d = fs::temp_directory_path() /
                 ("forge-snap-test-" + std::to_string(::getpid()) + "-" +
                  ::testing::UnitTest::GetInstance()->current_test_info()->name());
    fs::remove_all(d);
    fs::create_directories(d);
    return d;
}

std::unique_ptr<Store> store_with(int n, const std::string& tag = "") {
    auto s = make_memory_store();
    for (int i = 0; i < n; ++i)
        s->add(Chunk{"id-" + std::to_string(i), "doc-" + std::to_string(i % 3), "text " + tag + std::to_string(i),
                     {float(i), 1.0f, -0.5f}, {{"n", std::to_string(i)}}});
    return s;
}

void expect_same(const Store& a, const Store& b) {
    ASSERT_EQ(a.size(), b.size());
    for (Row r = 0; r < a.size(); ++r) EXPECT_EQ(a.at(r), b.at(r)) << "row " << r;
    if (a.size() > 0) {
        EXPECT_EQ(b.find(a.at(0).id), Row{0}) << "ids must be findable after loading";
    }
}

}  // namespace

TEST(Snapshot, RoundTrip) {
    fs::path file = temp_dir() / "snapshot";
    auto s = store_with(500);
    write_snapshot(*s, file);
    expect_same(*s, *read_snapshot(file));
}

TEST(Snapshot, AnEmptyStoreRoundTrips) {
    fs::path file = temp_dir() / "snapshot";
    write_snapshot(*make_memory_store(), file);
    EXPECT_EQ(read_snapshot(file)->size(), 0u);
}

TEST(Snapshot, StartsWithTheMagicBytes) {
    fs::path file = temp_dir() / "snapshot";
    write_snapshot(*store_with(1), file);
    std::ifstream f(file, std::ios::binary);
    char magic[8];
    f.read(magic, 8);
    EXPECT_EQ(std::string(magic, 8), "FORGESNP");
}

TEST(Snapshot, ReplacesTheOldFileAndLeavesNoTemporaryFiles) {
    fs::path dir = temp_dir();
    write_snapshot(*store_with(10, "old"), dir / "snapshot");
    auto fresh = store_with(20, "new");
    write_snapshot(*fresh, dir / "snapshot");
    expect_same(*fresh, *read_snapshot(dir / "snapshot"));
    int files = 0;
    for ([[maybe_unused]] auto& e : fs::directory_iterator(dir)) ++files;
    EXPECT_EQ(files, 1) << "the temporary file must be renamed over the old one";
}

TEST(Snapshot, ADamagedFileIsAnError) {
    fs::path dir = temp_dir();
    write_snapshot(*store_with(50), dir / "snapshot");
    auto size = fs::file_size(dir / "snapshot");
    fs::copy_file(dir / "snapshot", dir / "cut");
    fs::resize_file(dir / "cut", size - 3);
    EXPECT_THROW(read_snapshot(dir / "cut"), std::runtime_error) << "cut short";
    fs::copy_file(dir / "snapshot", dir / "flipped");
    {
        std::fstream f(dir / "flipped", std::ios::in | std::ios::out | std::ios::binary);
        f.seekg(static_cast<std::streamoff>(size / 2));
        char c;
        f.get(c);
        f.seekp(static_cast<std::streamoff>(size / 2));
        f.put(static_cast<char>(c ^ 0x55));
    }
    EXPECT_THROW(read_snapshot(dir / "flipped"), std::runtime_error) << "one byte changed: the CRC must catch it";
    EXPECT_THROW(read_snapshot(dir / "missing"), std::runtime_error) << "no file";
}

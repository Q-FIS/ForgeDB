// The contract between Group A (the engine) and Group B (the service), weeks 5-8.
//
// This is the ONLY thing Group B calls from Group A. Changing it needs both
// captains to approve. Each class keeps its insides in a private `Impl`
// ("pimpl"): Group A can change everything inside without changing this file.
//
// Until Group A's real engine is ready, the build uses a STUB (src/stub_engine.cpp)
// that returns simple fixed results, so Group B can build and test the service.
// Errors throw std::system_error (file problems) or std::invalid_argument (bad input).
#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace qfis {

struct Chunk {
    std::uint64_t id;
    std::string doc_id;
    std::uint32_t page;
    std::string text;
};

struct Hit {
    std::uint64_t id;
    float score;
};

// Crash-safe storage of chunks.
class ChunkStore {
   public:
    explicit ChunkStore(const std::string& dir);  // opens or creates; replays the log
    ~ChunkStore();                                // defined in the .cpp, where Impl is complete
    ChunkStore(ChunkStore&&) noexcept;            // movable, not copyable
    ChunkStore& operator=(ChunkStore&&) noexcept;

    void append(const Chunk& chunk);  // durable on disk when it returns
    std::optional<Chunk> get(std::uint64_t id) const;
    std::size_t size() const;

   private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

// Exact search by meaning: dot product over unit vectors.
class VectorIndex {
   public:
    explicit VectorIndex(std::size_t dim);
    ~VectorIndex();
    VectorIndex(VectorIndex&&) noexcept;  // needed: load() returns one by value
    VectorIndex& operator=(VectorIndex&&) noexcept;

    void add(std::uint64_t id, std::span<const float> vec);
    std::vector<Hit> search(std::span<const float> query, std::size_t k, int threads = 1) const;
    void save(const std::string& path) const;
    static VectorIndex load(const std::string& path);

   private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

// Search by keywords: BM25 over an inverted index.
class KeywordIndex {
   public:
    KeywordIndex();
    ~KeywordIndex();
    KeywordIndex(KeywordIndex&&) noexcept;
    KeywordIndex& operator=(KeywordIndex&&) noexcept;

    void add(std::uint64_t id, std::string_view text);
    void finish();  // call once after the last add(): computes lengths and IDF
    std::vector<Hit> search(std::string_view query, std::size_t k) const;

   private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace qfis

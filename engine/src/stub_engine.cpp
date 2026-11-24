// STUB engine: simple fixed behaviour so Group B can build the service in week 5.
// Nothing here is durable, fast or smart. Group A replaces it piece by piece.
//   - ChunkStore keeps chunks in memory (lost on restart).
//   - VectorIndex::search and KeywordIndex::search return the first k ids added, score 0.
#include <map>
#include <stdexcept>

#include "qfis/engine.hpp"

namespace qfis {

// ----- ChunkStore -----
struct ChunkStore::Impl {
    std::map<std::uint64_t, Chunk> chunks;
};

ChunkStore::ChunkStore(const std::string& /*dir*/) : impl_(std::make_unique<Impl>()) {}
ChunkStore::~ChunkStore() = default;
ChunkStore::ChunkStore(ChunkStore&&) noexcept = default;
ChunkStore& ChunkStore::operator=(ChunkStore&&) noexcept = default;

void ChunkStore::append(const Chunk& chunk) { impl_->chunks[chunk.id] = chunk; }

std::optional<Chunk> ChunkStore::get(std::uint64_t id) const {
    auto it = impl_->chunks.find(id);
    if (it == impl_->chunks.end()) return std::nullopt;
    return it->second;
}

std::size_t ChunkStore::size() const { return impl_->chunks.size(); }

// ----- VectorIndex -----
struct VectorIndex::Impl {
    std::size_t dim;
    std::vector<std::uint64_t> ids;
};

VectorIndex::VectorIndex(std::size_t dim) : impl_(std::make_unique<Impl>(Impl{dim, {}})) {}
VectorIndex::~VectorIndex() = default;
VectorIndex::VectorIndex(VectorIndex&&) noexcept = default;
VectorIndex& VectorIndex::operator=(VectorIndex&&) noexcept = default;

void VectorIndex::add(std::uint64_t id, std::span<const float> vec) {
    if (vec.size() != impl_->dim) throw std::invalid_argument("vector has the wrong dimension");
    impl_->ids.push_back(id);
}

std::vector<Hit> VectorIndex::search(std::span<const float> query, std::size_t k,
                                     int /*threads*/) const {
    if (query.size() != impl_->dim) throw std::invalid_argument("query has the wrong dimension");
    std::vector<Hit> hits;
    for (std::size_t i = 0; i < impl_->ids.size() && i < k; ++i) hits.push_back({impl_->ids[i], 0.0f});
    return hits;
}

void VectorIndex::save(const std::string& /*path*/) const {}  // stub: saves nothing
VectorIndex VectorIndex::load(const std::string& /*path*/) { return VectorIndex(384); }

// ----- KeywordIndex -----
struct KeywordIndex::Impl {
    std::vector<std::uint64_t> ids;
};

KeywordIndex::KeywordIndex() : impl_(std::make_unique<Impl>()) {}
KeywordIndex::~KeywordIndex() = default;
KeywordIndex::KeywordIndex(KeywordIndex&&) noexcept = default;
KeywordIndex& KeywordIndex::operator=(KeywordIndex&&) noexcept = default;

void KeywordIndex::add(std::uint64_t id, std::string_view /*text*/) { impl_->ids.push_back(id); }
void KeywordIndex::finish() {}

std::vector<Hit> KeywordIndex::search(std::string_view /*query*/, std::size_t k) const {
    std::vector<Hit> hits;
    for (std::size_t i = 0; i < impl_->ids.size() && i < k; ++i) hits.push_back({impl_->ids[i], 0.0f});
    return hits;
}

}  // namespace qfis

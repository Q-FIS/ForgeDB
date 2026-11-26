// Where chunks live, in memory and on disk.   (component SNAP; issues SNAP-1, SNAP-2, SNAP-3)
#pragma once

#include <cstddef>
#include <filesystem>
#include <memory>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

#include "forge/types.hpp"

namespace forge {

/// Every chunk, by Row and by id. Not thread-safe by itself: the Engine
/// (POOL) holds a lock around it.
class Store {
public:
    virtual ~Store() = default;

    /// Adds a chunk and returns its Row: size() before the call (0, 1, 2...).
    /// Throws AlreadyExists if a chunk with the same id is already here
    /// (and then nothing changes).
    virtual Row add(Chunk chunk) = 0;

    /// The chunk at `row`. Throws std::out_of_range if row >= size().
    virtual const Chunk& at(Row row) const = 0;

    /// The Row of the chunk with this id, or std::nullopt.
    virtual std::optional<Row> find(std::string_view id) const = 0;

    virtual std::size_t size() const = 0;
};

/// An empty in-memory Store (SNAP-1).
std::unique_ptr<Store> make_memory_store();

/// A Chunk as bytes, and back again (SNAP-2): the format is in docs/FORMAT.md.
/// The write-ahead log and the snapshots both store chunks this way.
std::vector<std::byte> encode_chunk(const Chunk& chunk);

/// Throws std::runtime_error if the bytes are not exactly one valid chunk
/// (too short, too long, or a length that points past the end).
Chunk decode_chunk(std::span<const std::byte> bytes);

/// Writes every chunk of `store` to `file`, atomically: after a crash, the
/// file is either the complete old one or the complete new one (SNAP-3).
void write_snapshot(const Store& store, const std::filesystem::path& file);

/// Reads a snapshot back into a new in-memory Store (SNAP-3). Throws
/// std::runtime_error if the file is damaged.
std::unique_ptr<Store> read_snapshot(const std::filesystem::path& file);

}  // namespace forge

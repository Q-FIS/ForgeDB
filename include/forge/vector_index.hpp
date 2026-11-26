// Search by meaning: nearest vectors.   (component VEC; issue VEC-1)
#pragma once

#include <cstddef>
#include <functional>
#include <memory>
#include <span>
#include <vector>

#include "forge/executor.hpp"
#include "forge/types.hpp"

namespace forge {

/// Says whether a row may appear in the results. An empty RowFilter keeps all.
using RowFilter = std::function<bool(Row)>;

class VectorIndex {
public:
    virtual ~VectorIndex() = default;

    virtual std::size_t dim() const = 0;
    virtual std::size_t size() const = 0;

    /// Adds the vector for `row`. Rows arrive in order: row must equal
    /// size(). Throws std::invalid_argument (and changes nothing) if the row
    /// is out of order, the length isn't dim(), or the vector is all zeros.
    virtual void add(Row row, std::span<const float> vector) = 0;

    /// The k rows whose vectors are most similar to `query` (cosine
    /// similarity, from -1 to 1), best first, ties broken by the smaller row
    /// (see `better` in types.hpp). Only rows that `keep` accepts. Fewer than
    /// k if there aren't enough. k == 0 gives an empty list. Throws
    /// std::invalid_argument if the query's length isn't dim() or it's all zeros.
    virtual std::vector<ScoredRow> search(std::span<const float> query, std::size_t k,
                                          const RowFilter& keep = {}) const = 0;
};

/// The flat (brute-force) index: compares the query with every vector.
/// dim must be >= 1 (std::invalid_argument). With a `pool`, search splits the
/// work across its threads (VEC-5); without one, it runs on the caller's thread.
std::unique_ptr<VectorIndex> make_flat_index(std::size_t dim, Executor* pool = nullptr);

}  // namespace forge

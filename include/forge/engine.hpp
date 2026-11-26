// The Engine: the store and both indexes, safe for many threads at once.
// (component POOL; issue POOL-3)
//
// The API calls only the Engine. The Engine checks every request, keeps the
// Store and the two indexes in step, and holds a lock so that many searches
// can run at the same time while puts take turns.
#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <string_view>
#include <vector>

#include "forge/keyword_index.hpp"
#include "forge/store.hpp"
#include "forge/types.hpp"
#include "forge/vector_index.hpp"

namespace forge {

class Engine {
public:
    /// The three must hold the same number of rows (usually 0), else
    /// std::invalid_argument. The Engine's dimension is vectors->dim().
    Engine(std::unique_ptr<Store> store, std::unique_ptr<VectorIndex> vectors,
           std::unique_ptr<KeywordIndex> keywords);

    /// Adds a chunk to the store and both indexes; returns its Row.
    /// std::invalid_argument if: the id is empty, the vector's length isn't
    /// dim(), or the vector is all zeros. AlreadyExists if the id is taken.
    /// On any error nothing changes anywhere.
    Row put(Chunk chunk);

    /// A copy of the chunk with this id, or std::nullopt.
    std::optional<Chunk> get(std::string_view id) const;

    /// Runs a search (see SearchRequest in types.hpp).
    /// std::invalid_argument if: k is not 1..1000; a vector or hybrid search
    /// has a vector whose length isn't dim() (or no vector); a keyword or
    /// hybrid search has empty text. Hybrid: NotImplemented until KW-5.
    std::vector<SearchHit> search(const SearchRequest& request) const;

    std::size_t size() const;
    std::size_t dim() const;

private:
    // POOL-3: your members go here, e.g. the three unique_ptrs and a
    // mutable std::shared_mutex.
};

}  // namespace forge

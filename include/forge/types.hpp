// The data every component shares.   (owner: lead)
//
// Change this file only with the lead's approval: every component codes
// against it. Rules: add, never rename or remove.
#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace forge {

/// A chunk's id, chosen by the client, e.g. "infosys_ar_fy2025:p12:c0003".
using ChunkId = std::string;

/// Inside ForgeDB every chunk also has a Row number: 0 for the first chunk
/// ever added, 1 for the next, and so on. Indexes work with Rows (small,
/// dense integers), not with string ids.
using Row = std::uint32_t;

/// One piece of a document, with its embedding. Sent by the client.
struct Chunk {
    ChunkId id;                               // unique, not empty
    std::string doc_id;                       // which document it came from
    std::string text;                         // the words (searched by keyword)
    std::vector<float> vector;                // the embedding (searched by meaning); length = dim
    std::map<std::string, std::string> meta;  // e.g. {"company": "infosys", "fy": "FY2025"}

    bool operator==(const Chunk&) const = default;
};

/// A search result inside ForgeDB: which row, and how well it matched.
struct ScoredRow {
    Row row;
    float score;

    bool operator==(const ScoredRow&) const = default;
};

/// The order of every result list in ForgeDB: higher score first, and for
/// equal scores the smaller row first (so results never change randomly).
inline bool better(const ScoredRow& a, const ScoredRow& b) {
    if (a.score != b.score) return a.score > b.score;
    return a.row < b.row;
}

enum class SearchMode { vector, keyword, hybrid };

/// Keep only chunks that match. An empty field means "don't filter on it".
struct Filter {
    std::string doc_id;                       // "" = any document
    std::map<std::string, std::string> meta;  // every pair must be in the chunk's meta

    bool empty() const { return doc_id.empty() && meta.empty(); }

    bool matches(const Chunk& c) const {
        if (!doc_id.empty() && c.doc_id != doc_id) return false;
        for (const auto& [key, value] : meta) {
            auto it = c.meta.find(key);
            if (it == c.meta.end() || it->second != value) return false;
        }
        return true;
    }
};

struct SearchRequest {
    SearchMode mode = SearchMode::vector;
    std::vector<float> vector;  // needed for vector and hybrid
    std::string text;           // needed for keyword and hybrid
    std::size_t k = 10;         // how many results: 1 to 1000
    Filter filter;
};

/// A search result as the client sees it.
struct SearchHit {
    ChunkId id;
    std::string doc_id;
    std::string text;
    float score;

    bool operator==(const SearchHit&) const = default;
};

// Errors. The API turns each into an HTTP status code:
//   std::invalid_argument -> 400 (bad input: wrong dimension, empty id, ...)
//   AlreadyExists         -> 409
//   NotImplemented        -> 501
//   anything else         -> 500

/// Adding a chunk whose id is already in ForgeDB.
struct AlreadyExists : std::runtime_error {
    using std::runtime_error::runtime_error;
};

/// A feature whose issue isn't done yet (e.g. hybrid search before KW-5).
struct NotImplemented : std::logic_error {
    using std::logic_error::logic_error;
};

}  // namespace forge

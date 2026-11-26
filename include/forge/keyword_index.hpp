// Search by words.   (component KW; issues KW-1, KW-2, KW-5)
#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "forge/types.hpp"
#include "forge/vector_index.hpp"  // RowFilter

namespace forge {

/// Splits text into lowercase words. The exact rules are in issue KW-1, e.g.
///   "Revenue rose to ₹1,45,000 crore in FY2025." ->
///   {"revenue", "rose", "to", "1,45,000", "crore", "in", "fy2025"}
std::vector<std::string> tokenize(std::string_view text);

class KeywordIndex {
public:
    virtual ~KeywordIndex() = default;

    virtual std::size_t size() const = 0;

    /// Adds the text for `row`. Rows arrive in order: row must equal size(),
    /// else std::invalid_argument.
    virtual void add(Row row, std::string_view text) = 0;

    /// The k best rows for the query's words, best first (see `better` in
    /// types.hpp). A row is returned only if it contains at least one of the
    /// query's words, and `keep` accepts it. Every score is > 0. A query with
    /// no words gives an empty list.
    virtual std::vector<ScoredRow> search(std::string_view query, std::size_t k,
                                          const RowFilter& keep = {}) const = 0;
};

/// An empty keyword index (KW-2: scores by counting; KW-3: BM25).
std::unique_ptr<KeywordIndex> make_keyword_index();

/// Reciprocal Rank Fusion (KW-5): merges two ranked lists into one. A row's
/// score is the sum, over the lists it appears in, of 1 / (c + rank), where
/// rank starts at 1. Returns the best k.
std::vector<ScoredRow> rrf_merge(const std::vector<ScoredRow>& a, const std::vector<ScoredRow>& b,
                                 std::size_t k, double c = 60.0);

}  // namespace forge

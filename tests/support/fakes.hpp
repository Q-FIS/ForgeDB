// Simple stand-ins for the Store and the two indexes, for tests ONLY.
// (owner: lead)
//
// The Engine and the API are tested with these instead of the real
// components, so POOL and APP don't have to wait for SNAP, VEC and KW.
// They're deliberately plain: correct, slow, and not thread-safe.
#pragma once

#include <algorithm>
#include <cctype>
#include <cmath>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>

#include "forge/keyword_index.hpp"
#include "forge/store.hpp"
#include "forge/vector_index.hpp"

namespace forge::testing {

class FakeStore final : public Store {
public:
    bool explode = false;  // set to true to make every call throw std::runtime_error

    Row add(Chunk chunk) override {
        boom();
        if (rows_.count(chunk.id)) throw AlreadyExists(chunk.id);
        rows_.emplace(chunk.id, static_cast<Row>(chunks_.size()));
        chunks_.push_back(std::move(chunk));
        return static_cast<Row>(chunks_.size() - 1);
    }
    const Chunk& at(Row row) const override {
        boom();
        return chunks_.at(row);
    }
    std::optional<Row> find(std::string_view id) const override {
        boom();
        auto it = rows_.find(std::string(id));
        if (it == rows_.end()) return std::nullopt;
        return it->second;
    }
    std::size_t size() const override { return chunks_.size(); }

private:
    void boom() const {
        if (explode) throw std::runtime_error("FakeStore exploded (on purpose)");
    }
    std::vector<Chunk> chunks_;
    std::map<std::string, Row> rows_;
};

class FakeVectorIndex final : public VectorIndex {
public:
    explicit FakeVectorIndex(std::size_t dim) : dim_(dim) {}
    std::size_t dim() const override { return dim_; }
    std::size_t size() const override { return vectors_.size(); }

    void add(Row row, std::span<const float> v) override {
        if (row != vectors_.size()) throw std::invalid_argument("rows must arrive in order");
        if (v.size() != dim_) throw std::invalid_argument("wrong dimension");
        if (norm(v) == 0) throw std::invalid_argument("zero vector");
        vectors_.emplace_back(v.begin(), v.end());
    }

    std::vector<ScoredRow> search(std::span<const float> q, std::size_t k,
                                  const RowFilter& keep) const override {
        if (q.size() != dim_) throw std::invalid_argument("wrong dimension");
        if (norm(q) == 0) throw std::invalid_argument("zero vector");
        std::vector<ScoredRow> all;
        for (Row r = 0; r < vectors_.size(); ++r) {
            if (keep && !keep(r)) continue;
            double dot = 0;
            for (std::size_t i = 0; i < dim_; ++i) dot += double(q[i]) * vectors_[r][i];
            all.push_back({r, static_cast<float>(dot / (norm(q) * norm(vectors_[r])))});
        }
        std::sort(all.begin(), all.end(), better);
        if (all.size() > k) all.resize(k);
        return all;
    }

private:
    static double norm(std::span<const float> v) {
        double s = 0;
        for (float x : v) s += double(x) * x;
        return std::sqrt(s);
    }
    std::size_t dim_;
    std::vector<std::vector<float>> vectors_;
};

/// Words = runs of letters and digits, lowercased. Score = how many of the
/// row's words are query words.
class FakeKeywordIndex final : public KeywordIndex {
public:
    std::size_t size() const override { return docs_.size(); }

    void add(Row row, std::string_view text) override {
        if (row != docs_.size()) throw std::invalid_argument("rows must arrive in order");
        docs_.push_back(words(text));
    }

    std::vector<ScoredRow> search(std::string_view query, std::size_t k,
                                  const RowFilter& keep) const override {
        auto q = words(query);
        std::vector<ScoredRow> all;
        for (Row r = 0; r < docs_.size(); ++r) {
            if (keep && !keep(r)) continue;
            int n = 0;
            for (const auto& w : docs_[r]) n += static_cast<int>(std::count(q.begin(), q.end(), w) > 0);
            if (n > 0) all.push_back({r, static_cast<float>(n)});
        }
        std::sort(all.begin(), all.end(), better);
        if (all.size() > k) all.resize(k);
        return all;
    }

private:
    static std::vector<std::string> words(std::string_view text) {
        std::vector<std::string> out;
        std::string cur;
        for (char c : text) {
            if (std::isalnum(static_cast<unsigned char>(c))) {
                cur += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            } else if (!cur.empty()) {
                out.push_back(cur);
                cur.clear();
            }
        }
        if (!cur.empty()) out.push_back(cur);
        return out;
    }
    std::vector<std::vector<std::string>> docs_;
};

}  // namespace forge::testing

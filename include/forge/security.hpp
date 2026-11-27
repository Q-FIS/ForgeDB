// Protecting the server: rate limits and secret comparison.   (component SEC; issue SEC-1)
//
// You may add private members and helpers. Changing a public signature needs
// the lead's OK: APP calls these.
#pragma once

#include <chrono>
#include <cstddef>
#include <string>
#include <string_view>

namespace forge {

/// A token bucket per key (an API key, or a client's IP address).
///
/// Each key's bucket holds up to `burst` tokens and refills at
/// `rate_per_second`, continuously (half a second refills rate/2 tokens).
/// A new key starts with a full bucket. Each allowed request takes one token.
///
/// Time is passed in, so tests can use a fake clock: give it
/// std::chrono::steady_clock::now() in real code. Thread-safe.
class RateLimiter {
public:
    using Clock = std::chrono::steady_clock;

    /// rate_per_second > 0 and burst >= 1, else std::invalid_argument.
    RateLimiter(double rate_per_second, double burst);

    /// Takes one token from `key`'s bucket if it has at least one, and returns
    /// true; otherwise changes nothing and returns false (the API answers 429).
    bool allow(std::string_view key, Clock::time_point now);

    /// How long until `key` will have a whole token: zero if it has one now.
    /// (The API puts this in the Retry-After header, rounded up to seconds.)
    Clock::duration retry_after(std::string_view key, Clock::time_point now) const;

    /// Forgets keys not seen for at least `idle`, so memory doesn't grow
    /// forever with one bucket per client ever seen. (A forgotten key comes
    /// back with a full bucket, which is what it would have refilled to anyway
    /// if idle >= burst / rate.)
    void forget_idle(Clock::time_point now, Clock::duration idle);

    /// How many keys have a bucket right now.
    std::size_t tracked_keys() const;

private:
    // SEC-1: your members go here, e.g. a mutex and a map from key to
    // {tokens, last refill time, last seen}.
};

/// Compares two secrets in a time that depends only on their lengths, never
/// on where they first differ, so an attacker can't guess a key byte by byte
/// by timing the answers.
bool constant_time_equals(std::string_view a, std::string_view b);

}  // namespace forge

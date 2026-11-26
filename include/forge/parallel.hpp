// Split a loop across an Executor, and wait for all of it.   (component POOL; issue POOL-5)
#pragma once

#include <cstddef>
#include <functional>

#include "forge/executor.hpp"

namespace forge {

/// Splits [0, n) into `pieces` consecutive ranges (pieces >= 1, else
/// std::invalid_argument), calls fn(begin, end) once for each non-empty range,
/// runs them on `executor` at the same time, and returns when all are done.
///
/// The calling thread works on pieces too, instead of just waiting. That
/// matters: a search running on a pool worker may call parallel_for on the
/// same pool, and if every worker only waited, nobody would be left to run
/// the pieces (a deadlock). Pieces the executor refuses run on the caller.
///
/// If any piece throws, parallel_for still waits for the others, then
/// rethrows the first exception.
void parallel_for(Executor& executor, std::size_t n, std::size_t pieces,
                  const std::function<void(std::size_t begin, std::size_t end)>& fn);

}  // namespace forge

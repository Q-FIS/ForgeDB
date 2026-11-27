// Something that runs tasks.   (owner: lead)
//
// The server hands work to an Executor. ThreadPool (thread_pool.hpp) is the
// real one; InlineExecutor below is the simplest possible one (tests use it).
#pragma once

#include <functional>

namespace forge {

class Executor {
public:
    virtual ~Executor() = default;
    /// Runs `task` some time soon. Returns false if it can't take the task
    /// right now (full, or stopped): then the task is NOT run. Never blocks.
    virtual bool submit(std::function<void()> task) = 0;
};

/// Runs each task immediately, on the caller's thread.
class InlineExecutor final : public Executor {
public:
    bool submit(std::function<void()> task) override {
        task();
        return true;
    }
};

}  // namespace forge

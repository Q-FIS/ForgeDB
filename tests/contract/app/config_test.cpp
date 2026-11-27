// Contract tests for APP-1: parse_args.   (owner: lead)
// Switched on when src/app/stub_config.cpp is deleted.
#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "forge/app.hpp"

using namespace forge;
using namespace std::chrono_literals;

namespace {

Config parse(std::vector<std::string_view> args) { return parse_args(args); }

void expect_usage_error(std::vector<std::string_view> args, std::string_view mentions) {
    try {
        parse(args);
        ADD_FAILURE() << "expected a UsageError for: " << (args.empty() ? "(nothing)" : args.back());
    } catch (const UsageError& e) {
        EXPECT_NE(std::string(e.what()).find(mentions), std::string::npos)
            << "the message should mention " << mentions << "; it was: " << e.what();
    }
}

}  // namespace

TEST(Config, DefaultsNeedOnlyTheDimension) {
    Config c = parse({"--dim", "384"});
    EXPECT_EQ(c.dim, 384u);
    EXPECT_EQ(c.host, "127.0.0.1");
    EXPECT_EQ(c.port, 8080);
    EXPECT_EQ(c.data_dir, "forge-data");
    EXPECT_EQ(c.threads, 4u);
    EXPECT_EQ(c.queue, 256u);
    EXPECT_EQ(c.io, IoMode::threads);
    EXPECT_EQ(c.fsync, "batch");
    EXPECT_EQ(c.idle_timeout, 10000ms);
}

TEST(Config, EveryFlag) {
    Config c = parse({"--host", "0.0.0.0", "--port", "9000", "--data-dir", "/tmp/forge", "--dim", "4",
                      "--threads", "8", "--queue", "1000", "--io", "epoll", "--fsync", "always",
                      "--idle-timeout-ms", "2500"});
    EXPECT_EQ(c.host, "0.0.0.0");
    EXPECT_EQ(c.port, 9000);
    EXPECT_EQ(c.data_dir, "/tmp/forge");
    EXPECT_EQ(c.dim, 4u);
    EXPECT_EQ(c.threads, 8u);
    EXPECT_EQ(c.queue, 1000u);
    EXPECT_EQ(c.io, IoMode::epoll);
    EXPECT_EQ(c.fsync, "always");
    EXPECT_EQ(c.idle_timeout, 2500ms);
}

TEST(Config, TheEdgesOfEveryRangeAreAllowed) {
    EXPECT_EQ(parse({"--dim", "1", "--port", "0"}).port, 0);  // 0 = any free port
    EXPECT_EQ(parse({"--dim", "4096", "--port", "65535"}).dim, 4096u);
    EXPECT_EQ(parse({"--dim", "1", "--threads", "256", "--queue", "100000"}).threads, 256u);
    EXPECT_EQ(parse({"--dim", "1", "--threads", "1", "--queue", "1"}).queue, 1u);
    EXPECT_EQ(parse({"--dim", "1", "--fsync", "never"}).fsync, "never");
    EXPECT_EQ(parse({"--dim", "1", "--io", "threads"}).io, IoMode::threads);
    EXPECT_EQ(parse({"--dim", "1", "--idle-timeout-ms", "3600000"}).idle_timeout, 3600000ms);
}

TEST(Config, BadValuesAreUsageErrors) {
    expect_usage_error({}, "--dim");
    expect_usage_error({"--dim", "0"}, "--dim");
    expect_usage_error({"--dim", "4097"}, "--dim");
    expect_usage_error({"--dim", "abc"}, "--dim");
    expect_usage_error({"--dim", "4", "--port", "65536"}, "--port");
    expect_usage_error({"--dim", "4", "--port", "-1"}, "--port");
    expect_usage_error({"--dim", "4", "--port", "80x"}, "--port");
    expect_usage_error({"--dim", "4", "--port", ""}, "--port");
    expect_usage_error({"--dim", "4", "--threads", "0"}, "--threads");
    expect_usage_error({"--dim", "4", "--threads", "257"}, "--threads");
    expect_usage_error({"--dim", "4", "--queue", "0"}, "--queue");
    expect_usage_error({"--dim", "4", "--queue", "100001"}, "--queue");
    expect_usage_error({"--dim", "4", "--io", "fast"}, "--io");
    expect_usage_error({"--dim", "4", "--fsync", "sometimes"}, "--fsync");
    expect_usage_error({"--dim", "4", "--idle-timeout-ms", "0"}, "--idle-timeout-ms");
    expect_usage_error({"--dim", "4", "--idle-timeout-ms", "3600001"}, "--idle-timeout-ms");
    expect_usage_error({"--dim", "4", "--data-dir", ""}, "--data-dir");
    expect_usage_error({"--dim", "4", "--host", ""}, "--host");
}

TEST(Config, BadCommandLinesAreUsageErrors) {
    expect_usage_error({"--dim", "4", "--colour", "red"}, "--colour");  // unknown flag
    expect_usage_error({"--dim", "4", "--port"}, "--port");             // flag without a value
    expect_usage_error({"--dim", "4", "extra"}, "extra");               // not a flag
    expect_usage_error({"--dim", "4", "--dim", "8"}, "--dim");          // given twice
}

TEST(Config, UsageMentionsEveryFlag) {
    std::string u = usage();
    for (const char* flag : {"--host", "--port", "--data-dir", "--dim", "--threads", "--queue", "--io",
                             "--fsync", "--idle-timeout-ms"})
        EXPECT_NE(u.find(flag), std::string::npos) << flag;
}

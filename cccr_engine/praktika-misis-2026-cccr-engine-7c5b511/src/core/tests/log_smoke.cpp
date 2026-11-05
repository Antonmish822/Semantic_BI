#include <cccr/log/log.hpp>

#include <doctest/doctest.h>

#include <memory_resource>
#include <string_view>

TEST_SUITE("cccr::log") {
  TEST_CASE("cccr::log free functions compile and run") {
    using namespace cccr::log;

    SUBCASE("string_view overloads without Context") {
      Debug(std::string_view{"debug smoke"});
      Trace(std::string_view{"trace smoke"});
      Info(std::string_view{"info smoke"});
      Warn(std::string_view{"warn smoke"});
      Error(std::string_view{"error smoke"});
      Critical(std::string_view{"critical smoke"});
      CHECK_EQ(LevelToString(Level::kError), "error");
    }

    SUBCASE("string_view overloads with Context first") {
      std::pmr::monotonic_buffer_resource arena{1024};
      const cccr::Context ctx{.allocator = &arena};

      Debug(ctx, std::string_view{"debug with ctx"});
      Trace(ctx, std::string_view{"trace with ctx"});
      Info(ctx, std::string_view{"info with ctx"});
      Warn(ctx, std::string_view{"warn with ctx"});
      Error(ctx, std::string_view{"error with ctx"});
      Critical(ctx, std::string_view{"critical with ctx"});
      CHECK_EQ(LevelToString(Level::kWarn), "warn");
    }

    SUBCASE("formatted overloads without Context") {
      Debug("debug {}", 1);
      Trace("trace {}", 2);
      Info("info {}", 3);
      Warn("warn {}", 4);
      Error("error {}", 5);
      Critical("critical {}", 6);
      CHECK_EQ(LevelToString(Level::kInfo), "info");
    }

    SUBCASE("formatted overloads with Context first") {
      std::pmr::monotonic_buffer_resource arena{1024};
      const cccr::Context ctx{.allocator = &arena};

      Debug(ctx, "debug {}", 1);
      Trace(ctx, "trace {}", 2);
      Info(ctx, "info {}", 3);
      Warn(ctx, "warn {}", 4);
      Error(ctx, "error {}", 5);
      Critical(ctx, "critical {}", 6);
      CHECK_EQ(LevelToString(Level::kCritical), "critical");
    }
  }
}

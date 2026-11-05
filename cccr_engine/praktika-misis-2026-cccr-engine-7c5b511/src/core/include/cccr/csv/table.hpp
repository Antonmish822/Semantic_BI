#pragma once

#include <cccr/context.hpp>

#include <memory_resource>
#include <string>
#include <vector>

namespace cccr::csv {

/// @brief One CSV data row (cells aligned with @ref Table::headers).
struct Row {
  /// @brief Returns an empty row that allocates via @p ctx.
  /// @param ctx Shared allocator context
  /// @return Empty row bound to @p ctx
  [[nodiscard]] static Row FromContext(Context ctx) noexcept {
    return {.cells = std::pmr::vector<std::pmr::string>{ctx.GetAllocator()}};
  }

  /// @brief Cell values for this row.
  std::pmr::vector<std::pmr::string> cells{std::pmr::get_default_resource()};
};

/// @brief In-memory CSV table: header labels plus data rows.
struct Table {
  /// @brief Returns an empty table that allocates via @p ctx.
  /// @param ctx Shared allocator context
  /// @return Empty table bound to @p ctx
  [[nodiscard]] static Table FromContext(Context ctx) noexcept {
    auto* mr = ctx.GetAllocator();
    return {
        .headers = std::pmr::vector<std::pmr::string>{mr},
        .rows = std::pmr::vector<Row>{mr},
    };
  }

  /// @brief Column header labels (first CSV row).
  std::pmr::vector<std::pmr::string> headers{std::pmr::get_default_resource()};
  /// @brief Data rows following the header.
  std::pmr::vector<Row> rows{std::pmr::get_default_resource()};
};

}  // namespace cccr::csv

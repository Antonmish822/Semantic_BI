#include <pch.hpp>

#include <cccr/context.hpp>
#include <cccr/csv/parse.hpp>
#include <cccr/log/log.hpp>

#include <glaze/csv.hpp>

#include <cstddef>
#include <expected>
#include <memory_resource>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace cccr::csv {

namespace {

constexpr glz::opts_csv kReadOpts{.use_headers = false};

}  // namespace

auto Parse(std::string_view csv, Context ctx) -> CsvResult<Table> {
  auto* mr = ctx.GetAllocator();
  auto table = Table::FromContext(ctx);

  if (csv.empty()) [[unlikely]] {
    log::Error(ctx, "CSV parse failed: input is empty");
    return std::unexpected(CsvError::kParseFailed);
  }

  std::pmr::vector<std::pmr::vector<std::pmr::string>> matrix{mr};
  if (const auto ec = glz::read<kReadOpts>(matrix, csv)) [[unlikely]] {
    const std::string detail = glz::format_error(ec, csv);
    log::Error(ctx, "CSV parse failed: {}", detail);
    return std::unexpected(CsvError::kParseFailed);
  }

  if (matrix.empty() || matrix[0].empty()) [[unlikely]] {
    log::Error(ctx, "CSV parse failed: no header row");
    return std::unexpected(CsvError::kParseFailed);
  }

  const size_t column_count = matrix[0].size();
  table.headers.reserve(column_count);
  for (auto& cell : matrix[0]) {
    table.headers.push_back(std::move(cell));
  }

  table.rows.reserve(matrix.size() - 1);
  for (size_t row_idx = 1; row_idx < matrix.size(); ++row_idx) {
    auto& cells = matrix[row_idx];

    // Skip a completely empty trailing record produced by a final newline.
    if (cells.size() == 1 && cells[0].empty() && row_idx + 1 == matrix.size()) {
      break;
    }

    if (cells.size() > column_count) [[unlikely]] {
      log::Error(ctx,
                 "CSV parse failed: row {} has {} cells, expected at most {}",
                 row_idx, cells.size(), column_count);
      return std::unexpected(CsvError::kParseFailed);
    }

    auto row = Row::FromContext(ctx);
    row.cells.reserve(column_count);
    for (auto& cell : cells) {
      row.cells.push_back(std::move(cell));
    }
    while (row.cells.size() < column_count) {
      row.cells.emplace_back();
    }
    table.rows.push_back(std::move(row));
  }

  return table;
}

}  // namespace cccr::csv

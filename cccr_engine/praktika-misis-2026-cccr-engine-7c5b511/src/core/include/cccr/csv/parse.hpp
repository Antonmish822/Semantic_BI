#pragma once

#include <cccr/context.hpp>
#include <cccr/csv/error.hpp>
#include <cccr/csv/table.hpp>

#include <string_view>

namespace cccr::csv {

/// @brief Parses comma-separated CSV text into a @ref Table.
/// @details Uses glaze (`opts_csv`, headerless 2D read). First row is headers.
///          Short data rows are padded with empty cells; rows with more cells
///          than headers fail with @ref CsvError::kParseFailed. Supports
///          RFC4180-style quoting (`"..."` and `""` escapes).
/// @param csv CSV text (UTF-8)
/// @param ctx Shared allocator context (PMR table storage)
/// @return Parsed table or CsvError
[[nodiscard]] auto Parse(std::string_view csv, Context ctx = {})
    -> CsvResult<Table>;

}  // namespace cccr::csv

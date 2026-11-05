#pragma once

#include <cccr/context.hpp>
#include <cccr/csv/table.hpp>

#include <memory_resource>
#include <string>

namespace cccr::csv {

/// @brief Projects @p table to registry/row/cell XML for XQuery.
/// @details Shape matches `пример.md`: `<registry><row id="N"><cell
/// label="…">…`.
///          Labels and cell text are XML-escaped (`& < > "`).
/// @param table Parsed CSV table
/// @param ctx Shared allocator context
/// @return XML document text
[[nodiscard]] auto ProjectToXml(const Table& table, Context ctx = {})
    -> std::pmr::string;

}  // namespace cccr::csv

#include <pch.hpp>

#include <cccr/context.hpp>
#include <cccr/csv/project.hpp>

#include <algorithm>
#include <format>
#include <iterator>
#include <memory_resource>
#include <string>
#include <string_view>

namespace cccr::csv {

namespace {

void AppendEscaped(std::pmr::string& out, std::string_view text) {
  for (const char ch : text) {
    switch (ch) {
      case '&':
        out += "&amp;";
        break;
      case '<':
        out += "&lt;";
        break;
      case '>':
        out += "&gt;";
        break;
      case '"':
        out += "&quot;";
        break;
      default:
        out.push_back(ch);
        break;
    }
  }
}

}  // namespace

std::pmr::string ProjectToXml(const Table& table, Context ctx) {
  std::pmr::string out{ctx.GetAllocator()};
  out.reserve(256 + table.rows.size() * 64);

  out += R"(<?xml version="1.0" encoding="UTF-8"?>)";
  out += '\n';
  out += "<registry>\n";

  for (size_t row_idx = 0; row_idx < table.rows.size(); ++row_idx) {
    const Row& row = table.rows[row_idx];
    out += "    <row id=\"";
    std::format_to(std::back_inserter(out), "{}", row_idx + 1);
    out += "\">\n";

    const size_t cell_count = std::min(table.headers.size(), row.cells.size());
    for (size_t col = 0; col < cell_count; ++col) {
      out += "        <cell label=\"";
      AppendEscaped(out, table.headers[col]);
      out += "\">";
      AppendEscaped(out, row.cells[col]);
      out += "</cell>\n";
    }

    out += "    </row>\n";
  }

  out += "</registry>\n";
  return out;
}

}  // namespace cccr::csv

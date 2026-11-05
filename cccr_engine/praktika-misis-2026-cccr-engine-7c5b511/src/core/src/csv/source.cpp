#include <pch.hpp>

#include <cccr/csv/parse.hpp>
#include <cccr/csv/project.hpp>
#include <cccr/csv/source.hpp>

#include <expected>
#include <string_view>
#include <utility>

namespace cccr::csv {

auto CsvSource::Load(std::string_view csv) -> CsvResult<void> {
  auto parsed = Parse(csv, ctx_);
  if (!parsed) [[unlikely]] {
    return std::unexpected(parsed.error());
  }
  table_ = std::move(*parsed);

  const auto xml_text = ProjectToXml(table_, ctx_);
  auto loaded = xml_.Load(xml_text);
  if (!loaded) [[unlikely]] {
    return std::unexpected(CsvError::kProjectFailed);
  }

  return {};
}

}  // namespace cccr::csv

#include <pch.hpp>

#include <cccr/validate/literal.hpp>

#include <array>
#include <cctype>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <expected>
#include <limits>
#include <optional>
#include <string_view>
#include <system_error>

namespace cccr::validate {

namespace {

[[nodiscard]] constexpr std::string_view LocalDatatypeName(
    std::string_view datatype) noexcept {
  if (datatype.empty()) {
    return datatype;
  }

  const auto hash = datatype.find_last_of('#');
  if (hash != std::string_view::npos && hash + 1 < datatype.size()) {
    return datatype.substr(hash + 1);
  }

  const auto slash = datatype.find_last_of('/');
  if (slash != std::string_view::npos && slash + 1 < datatype.size()) {
    return datatype.substr(slash + 1);
  }

  // Compact CURIE form, e.g. `xsd:integer`.
  const auto colon = datatype.find(':');
  if (colon != std::string_view::npos && colon + 1 < datatype.size() &&
      !datatype.contains("://")) {
    return datatype.substr(colon + 1);
  }

  return datatype;
}

[[nodiscard]] constexpr bool IsDigitString(std::string_view text) noexcept {
  if (text.empty()) {
    return false;
  }

  size_t idx = 0;
  if (text[0] == '+' || text[0] == '-') {
    if (text.size() == 1) {
      return false;
    }
    ++idx;
  }

  for (; idx < text.size(); ++idx) {
    if (std::isdigit(static_cast<unsigned char>(text[idx])) == 0) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] constexpr auto ParseInteger(std::string_view text)
    -> std::optional<long long> {
  if (!IsDigitString(text)) {
    return std::nullopt;
  }

  long long value = 0;
  const auto* first = text.data();
  const auto* last = text.data() + text.size();
  const auto [ptr, ec] = std::from_chars(first, last, value);
  if (ec != std::errc{} || ptr != last) {
    return std::nullopt;
  }

  return value;
}

[[nodiscard]] constexpr auto ParseDouble(std::string_view text)
    -> std::optional<double> {
  if (text.empty()) {
    return std::nullopt;
  }

  double value = 0;
  const auto* first = text.data();
  const auto* last = text.data() + text.size();
  const auto [ptr, ec] = std::from_chars(first, last, value);
  if (ec != std::errc{} || ptr != last || !std::isfinite(value)) {
    return std::nullopt;
  }

  return value;
}

[[nodiscard]] constexpr bool LooksLikeDate(std::string_view text) noexcept {
  // YYYY-MM-DD
  if (text.size() != 10) {
    return false;
  }

  for (size_t i = 0; i < text.size(); ++i) {
    if (i == 4 || i == 7) {
      if (text[i] != '-') {
        return false;
      }
      continue;
    }

    if (std::isdigit(static_cast<unsigned char>(text[i])) == 0) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] constexpr bool LooksLikeGYear(std::string_view text) noexcept {
  if (text.size() < 4) {
    return false;
  }

  size_t idx = 0;
  if (text[0] == '+' || text[0] == '-') {
    ++idx;
  }

  if (text.size() - idx < 4) {
    return false;
  }

  for (; idx < text.size(); ++idx) {
    if (std::isdigit(static_cast<unsigned char>(text[idx])) == 0) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] constexpr bool LooksLikeTime(std::string_view text) noexcept {
  // hh:mm:ss with optional fractional seconds and Z / ±hh:mm
  if (text.size() < 8) {
    return false;
  }

  size_t i = 0;
  auto take_digits = [&](size_t count) -> bool {
    if (i + count > text.size()) {
      return false;
    }
    for (size_t n = 0; n < count; ++n) {
      if (std::isdigit(static_cast<unsigned char>(text[i + n])) == 0) {
        return false;
      }
    }
    i += count;
    return true;
  };

  if (!take_digits(2) || i >= text.size() || text[i++] != ':' ||
      !take_digits(2) || i >= text.size() || text[i++] != ':' ||
      !take_digits(2)) {
    return false;
  }

  if (i < text.size() && text[i] == '.') {
    ++i;
    if (i >= text.size() ||
        std::isdigit(static_cast<unsigned char>(text[i])) == 0) {
      return false;
    }
    while (i < text.size() &&
           std::isdigit(static_cast<unsigned char>(text[i])) != 0) {
      ++i;
    }
  }

  if (i < text.size()) {
    if (text[i] == 'Z' || text[i] == 'z') {
      ++i;
    } else if (text[i] == '+' || text[i] == '-') {
      ++i;
      if (!take_digits(2) || i >= text.size() || text[i++] != ':' ||
          !take_digits(2)) {
        return false;
      }
    } else {
      return false;
    }
  }

  return i == text.size();
}

[[nodiscard]] constexpr bool LooksLikeDateTime(std::string_view text) noexcept {
  // YYYY-MM-DDThh:mm:ss… (lenient)
  const auto t_pos = text.find('T');
  if (t_pos == std::string_view::npos) {
    return false;
  }
  return LooksLikeDate(text.substr(0, t_pos)) &&
         LooksLikeTime(text.substr(t_pos + 1));
}

[[nodiscard]] constexpr bool LooksLikeAnyUri(std::string_view text) noexcept {
  // Basic absolute IRI: scheme ':' and no whitespace.
  if (text.size() < 3) {
    return false;
  }
  const auto colon = text.find(':');
  if (colon == std::string_view::npos || colon == 0) {
    return false;
  }
  for (size_t i = 0; i < colon; ++i) {
    const char ch = text[i];
    const bool ok = (std::isalnum(static_cast<unsigned char>(ch)) != 0) ||
                    ch == '+' || ch == '-' || ch == '.';
    if (!ok) {
      return false;
    }
  }
  for (const char ch : text) {
    if (std::isspace(static_cast<unsigned char>(ch)) != 0) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] constexpr bool IsIntegerFamily(
    std::string_view local_type) noexcept {
  return local_type == "integer" || local_type == "int" ||
         local_type == "long" || local_type == "positiveInteger" ||
         local_type == "nonNegativeInteger" ||
         local_type == "nonPositiveInteger" || local_type == "unsignedInt" ||
         local_type == "unsignedLong" || local_type == "gYear";
}

[[nodiscard]] constexpr bool MatchesDatatype(std::string_view value,
                                             std::string_view local_type) {
  if (local_type.empty() || local_type == "string") {
    return true;
  }

  if (local_type == "integer" || local_type == "int" || local_type == "long") {
    return ParseInteger(value).has_value();
  }

  if (local_type == "positiveInteger") {
    const auto parsed = ParseInteger(value);
    return parsed.has_value() && *parsed > 0;
  }

  if (local_type == "nonNegativeInteger" || local_type == "unsignedInt" ||
      local_type == "unsignedLong") {
    const auto parsed = ParseInteger(value);
    if (!parsed || *parsed < 0) {
      return false;
    }
    if (local_type == "unsignedInt" &&
        *parsed >
            static_cast<long long>(std::numeric_limits<uint32_t>::max())) {
      return false;
    }
    return true;
  }

  if (local_type == "nonPositiveInteger") {
    const auto parsed = ParseInteger(value);
    return parsed.has_value() && *parsed <= 0;
  }

  if (local_type == "decimal" || local_type == "double" ||
      local_type == "float") {
    return ParseDouble(value).has_value();
  }

  if (local_type == "boolean") {
    return value == "true" || value == "false" || value == "1" || value == "0";
  }

  if (local_type == "date") {
    return LooksLikeDate(value);
  }

  if (local_type == "dateTime") {
    return LooksLikeDateTime(value);
  }

  if (local_type == "time") {
    return LooksLikeTime(value);
  }

  if (local_type == "gYear") {
    return LooksLikeGYear(value);
  }

  if (local_type == "anyURI") {
    return LooksLikeAnyUri(value);
  }

  return false;
}

template <typename T>
[[nodiscard]] constexpr bool InRange(T value, std::optional<T> min_v,
                                     std::optional<T> max_v) noexcept {
  if (min_v && value < *min_v) {
    return false;
  }

  if (max_v && value > *max_v) {
    return false;
  }

  return true;
}

[[nodiscard]] constexpr auto CheckRange(
    std::string_view value, std::string_view local_type,
    std::optional<std::string_view> min_inclusive,
    std::optional<std::string_view> max_inclusive) -> ValidateResult<void> {
  if (!min_inclusive && !max_inclusive) {
    return {};
  }

  if (IsIntegerFamily(local_type)) {
    const auto parsed = ParseInteger(value);
    if (!parsed) [[unlikely]] {
      return std::unexpected(ValidateError::kTypeMismatch);
    }

    std::optional<long long> min_v;
    std::optional<long long> max_v;
    if (min_inclusive) {
      min_v = ParseInteger(*min_inclusive);
      if (!min_v) [[unlikely]] {
        return std::unexpected(ValidateError::kTypeMismatch);
      }
    }

    if (max_inclusive) {
      max_v = ParseInteger(*max_inclusive);
      if (!max_v) [[unlikely]] {
        return std::unexpected(ValidateError::kTypeMismatch);
      }
    }

    if (!InRange(*parsed, min_v, max_v)) [[unlikely]] {
      return std::unexpected(ValidateError::kOutOfRange);
    }
    return {};
  }

  if (local_type == "decimal" || local_type == "double" ||
      local_type == "float") {
    const auto parsed = ParseDouble(value);
    if (!parsed) [[unlikely]] {
      return std::unexpected(ValidateError::kTypeMismatch);
    }

    std::optional<double> min_v;
    std::optional<double> max_v;
    if (min_inclusive) {
      min_v = ParseDouble(*min_inclusive);
      if (!min_v) [[unlikely]] {
        return std::unexpected(ValidateError::kTypeMismatch);
      }
    }

    if (max_inclusive) {
      max_v = ParseDouble(*max_inclusive);
      if (!max_v) [[unlikely]] {
        return std::unexpected(ValidateError::kTypeMismatch);
      }
    }

    if (!InRange(*parsed, min_v, max_v)) [[unlikely]] {
      return std::unexpected(ValidateError::kOutOfRange);
    }
    return {};
  }

  // Lexical compare for string / date / dateTime / time / anyURI.
  if (min_inclusive && value < *min_inclusive) [[unlikely]] {
    return std::unexpected(ValidateError::kOutOfRange);
  }

  if (max_inclusive && value > *max_inclusive) [[unlikely]] {
    return std::unexpected(ValidateError::kOutOfRange);
  }

  return {};
}

[[nodiscard]] constexpr auto SupportedXsdNames() noexcept
    -> std::array<std::string_view, 18> {
  using namespace std::string_view_literals;
  return {
      "string"sv,
      "integer"sv,
      "int"sv,
      "long"sv,
      "positiveInteger"sv,
      "nonNegativeInteger"sv,
      "nonPositiveInteger"sv,
      "unsignedInt"sv,
      "unsignedLong"sv,
      "decimal"sv,
      "double"sv,
      "float"sv,
      "boolean"sv,
      "date"sv,
      "dateTime"sv,
      "time"sv,
      "gYear"sv,
      "anyURI"sv,
  };
}

}  // namespace

bool KnownXsdDatatype(std::string_view datatype) noexcept {
  if (datatype.empty()) {
    return false;
  }

  const auto local = LocalDatatypeName(datatype);
  for (const auto name : SupportedXsdNames()) {
    if (local == name) {
      return true;
    }
  }

  return false;
}

auto CheckLiteral(std::string_view value, std::string_view datatype,
                  std::optional<std::string_view> min_inclusive,
                  std::optional<std::string_view> max_inclusive)
    -> ValidateResult<void> {
  const auto local = LocalDatatypeName(datatype);

  if (!datatype.empty()) {
    if (!KnownXsdDatatype(datatype)) [[unlikely]] {
      return std::unexpected(ValidateError::kUnsupportedDatatype);
    }

    if (!MatchesDatatype(value, local)) [[unlikely]] {
      return std::unexpected(ValidateError::kTypeMismatch);
    }
  }

  return CheckRange(value, local.empty() ? "string" : local, min_inclusive,
                    max_inclusive);
}

}  // namespace cccr::validate

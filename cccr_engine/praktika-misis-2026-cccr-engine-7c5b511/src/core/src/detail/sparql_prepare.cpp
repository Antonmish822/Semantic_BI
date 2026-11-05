#include <pch.hpp>

#include <detail/sparql_prepare.hpp>

#include <cctype>
#include <cstddef>
#include <memory_resource>
#include <string>
#include <string_view>

namespace cccr::detail {

namespace {

[[nodiscard]] constexpr bool StartsWithIgnoreCase(
    std::string_view text, std::string_view prefix) noexcept {
  if (text.size() < prefix.size()) {
    return false;
  }

  for (size_t i = 0; i < prefix.size(); ++i) {
    if (std::tolower(static_cast<unsigned char>(text[i])) !=
        std::tolower(static_cast<unsigned char>(prefix[i]))) {
      return false;
    }
  }

  return true;
}

[[nodiscard]] bool HasPrefixDecl(std::string_view query,
                                 std::string_view prefix_name) {
  size_t pos = 0;
  while (pos < query.size()) {
    auto found = query.find("PREFIX", pos);
    if (found == std::string_view::npos) {
      found = query.find("prefix", pos);
      if (found == std::string_view::npos) {
        return false;
      }
    }

    pos = found;
    size_t i = pos + 6;
    while (i < query.size() &&
           std::isspace(static_cast<unsigned char>(query[i])) != 0) {
      ++i;
    }

    if (StartsWithIgnoreCase(query.substr(i), prefix_name)) {
      return true;
    }
    ++pos;
  }

  return false;
}

[[nodiscard]] auto NormalizePrefixBases(std::string_view text,
                                        std::pmr::memory_resource* allocator)
    -> std::pmr::string {
  std::pmr::string out{allocator};
  out.reserve(text.size() + 8);
  size_t i = 0;
  while (i < text.size()) {
    if (!StartsWithIgnoreCase(text.substr(i), "PREFIX")) {
      out.push_back(text[i]);
      ++i;
      continue;
    }

    out.append(text.substr(i, 6));
    i += 6;

    while (i < text.size() &&
           std::isspace(static_cast<unsigned char>(text[i])) != 0) {
      out.push_back(text[i]);
      ++i;
    }

    while (i < text.size() &&
           std::isspace(static_cast<unsigned char>(text[i])) == 0 &&
           text[i] != '<') {
      out.push_back(text[i]);
      ++i;
    }

    while (i < text.size() &&
           std::isspace(static_cast<unsigned char>(text[i])) != 0) {
      out.push_back(text[i]);
      ++i;
    }

    if (i >= text.size() || text[i] != '<') {
      continue;
    }

    out.push_back('<');
    ++i;
    const size_t iri_start = i;
    while (i < text.size() && text[i] != '>') {
      ++i;
    }

    const auto iri = text.substr(iri_start, i - iri_start);
    out.append(iri);
    if (!iri.empty() && iri.back() != '#' && iri.back() != '/') {
      out.push_back('#');
    }

    if (i < text.size() && text[i] == '>') {
      out.push_back('>');
      ++i;
    }
  }
  return out;
}

}  // namespace

std::pmr::string PrepareSparqlQuery(std::string_view query,
                                    std::pmr::memory_resource* allocator) {
  std::pmr::string out{allocator};
  out.reserve(query.size() + 192);
  if (!HasPrefixDecl(query, "rdf:")) {
    out += "PREFIX rdf: <http://www.w3.org/1999/02/22-rdf-syntax-ns#>\n";
  }

  if (!HasPrefixDecl(query, "rdfs:")) {
    out += "PREFIX rdfs: <http://www.w3.org/2000/01/rdf-schema#>\n";
  }

  if (!HasPrefixDecl(query, "xsd:")) {
    out += "PREFIX xsd: <http://www.w3.org/2001/XMLSchema#>\n";
  }

  out += NormalizePrefixBases(query, allocator);
  return out;
}

}  // namespace cccr::detail

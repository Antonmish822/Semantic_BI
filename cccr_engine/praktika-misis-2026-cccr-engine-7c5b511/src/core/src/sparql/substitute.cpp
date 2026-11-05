#include <pch.hpp>

#include <cccr/context.hpp>
#include <cccr/sparql/substitute.hpp>

#include <string>
#include <string_view>

namespace cccr::sparql {

namespace {

[[nodiscard]] std::pmr::string EscapeSparqlString(std::string_view value,
                                                  Context ctx) {
  std::pmr::string out{ctx.GetAllocator()};
  out.reserve(value.size() + 2);
  out.push_back('"');
  for (const char ch : value) {
    switch (ch) {
      case '\\':
        out += "\\\\";
        break;
      case '"':
        out += "\\\"";
        break;
      case '\n':
        out += "\\n";
        break;
      case '\r':
        out += "\\r";
        break;
      case '\t':
        out += "\\t";
        break;
      default:
        out.push_back(ch);
        break;
    }
  }
  out.push_back('"');
  return out;
}

}  // namespace

std::pmr::string SubstitutePseudos(std::string_view text,
                                   const PseudoMap& variables, Context ctx) {
  auto* const mr = ctx.GetAllocator();
  std::pmr::string out{mr};
  out.reserve(text.size());
  for (size_t i = 0; i < text.size();) {
    if (text[i] == '{' && i + 1 < text.size() && text[i + 1] == '{') {
      const size_t start = i + 2;
      const size_t end = text.find("}}", start);
      if (end == std::string_view::npos) {
        out.append(text.substr(i));
        break;
      }

      const std::pmr::string name{text.substr(start, end - start), mr};
      const auto it = variables.find(name);
      if (it != variables.end()) {
        out += EscapeSparqlString(it->second, ctx);
      } else {
        out.append(text.substr(i, end + 2 - i));
      }
      i = end + 2;
      continue;
    }

    out.push_back(text[i]);
    ++i;
  }

  return out;
}

}  // namespace cccr::sparql

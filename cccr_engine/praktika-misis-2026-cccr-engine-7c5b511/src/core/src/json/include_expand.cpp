#include <pch.hpp>

#include <json/include_expand.hpp>

#include <cccr/log/log.hpp>
#include <cccr/utils/filesystem.hpp>

#include <detail/path.hpp>

#include <glaze/glaze.hpp>

#include <cstddef>
#include <expected>
#include <iterator>
#include <utility>
#include <vector>

namespace cccr::json {

namespace {

using detail::PropertiesWrapDto;
using detail::PropertyDto;
using detail::SchemaDto;

constexpr glz::opts kReadOpts{.null_terminated = false};

[[nodiscard]] auto ParsePropertyFragment(std::string_view text)
    -> JsonResult<std::vector<PropertyDto>> {
  {
    std::vector<PropertyDto> as_array;
    if (!glz::read<kReadOpts>(as_array, text)) {
      return as_array;
    }
  }

  {
    PropertyDto as_property{};
    if (!glz::read<kReadOpts>(as_property, text)) {
      if (as_property.include || as_property.qname || as_property.uri ||
          as_property.literal || as_property.properties ||
          as_property.description) {
        return std::vector<PropertyDto>{std::move(as_property)};
      }
    }
  }

  {
    PropertiesWrapDto as_wrap{};
    if (!glz::read<kReadOpts>(as_wrap, text) && as_wrap.properties) {
      return std::move(*as_wrap.properties);
    }
  }

  {
    SchemaDto as_schema{};
    if (!glz::read<kReadOpts>(as_schema, text) &&
        as_schema.description.properties) {
      return std::move(*as_schema.description.properties);
    }
  }

  log::Error(
      "JSON include fragment is not a valid property, properties array, "
      "or schema properties list");
  return std::unexpected(JsonError::kInvalidSchema);
}

[[nodiscard]] auto ExpandPropertyIncludesImpl(
    std::vector<PropertyDto>& properties,
    const ExpandPropertyIncludesParams& params) -> JsonResult<void> {
  std::vector<std::vector<PropertyDto>*> queue{&properties};
  size_t qi = 0;
  while (qi < queue.size()) {
    auto& list = *queue[qi++];
    size_t cursor = 0;
    while (cursor < list.size()) {
      auto& prop = list[cursor];

      if (!prop.include) {
        if (prop.properties) {
          queue.push_back(&*prop.properties);
        }

        if (prop.description && prop.description->properties) {
          queue.push_back(&*prop.description->properties);
        }
        ++cursor;
        continue;
      }

      if (prop.qname || prop.uri || prop.literal || prop.properties ||
          prop.description || prop.context) [[unlikely]] {
        log::Error(
            params.ctx,
            "JSON include property must not combine include with other fields "
            "(include=\"{}\")",
            *prop.include);
        return std::unexpected(JsonError::kInvalidSchema);
      }

      const auto path =
          cccr::detail::ResolveIncludePath(*prop.include, params.include_base);
      auto file = utils::ReadFileToString(path, params.ctx);
      if (!file) [[unlikely]] {
        log::Error(params.ctx,
                   "Failed to read JSON include \"{}\" (resolved path: {}): {}",
                   *prop.include, path.string(),
                   utils::FileErrorToString(file.error()));
        return std::unexpected(JsonError::kIncludeFailed);
      }

      auto fragment = ParsePropertyFragment(*file);
      if (!fragment) [[unlikely]] {
        log::Error(params.ctx,
                   "Failed to parse JSON include \"{}\" (resolved path: {})",
                   *prop.include, path.string());
        return std::unexpected(fragment.error());
      }

      list.erase(list.begin() + static_cast<std::ptrdiff_t>(cursor));
      list.insert(list.begin() + static_cast<std::ptrdiff_t>(cursor),
                  std::make_move_iterator(fragment->begin()),
                  std::make_move_iterator(fragment->end()));
    }
  }

  return {};
}

}  // namespace

auto ExpandPropertyIncludes(std::vector<detail::PropertyDto>& properties,
                            const ExpandPropertyIncludesParams& params)
    -> JsonResult<void> {
  return ExpandPropertyIncludesImpl(properties, params);
}

}  // namespace cccr::json

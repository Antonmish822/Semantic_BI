#include "context.hpp"

#include "util.hpp"

#include <cccr/bindings/common/error.hpp>
#include <cccr/bindings/common/pipeline.hpp>

#include <filesystem>
#include <format>
#include <functional>
#include <iterator>
#include <memory_resource>
#include <optional>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

namespace cccr::bindings::python {
namespace {

[[nodiscard]] bool EnsureSameContext(ContextState& state,
                                     const ContextStatePtr& expected,
                                     const ContextStatePtr& actual,
                                     std::string_view what) {
  if (expected == actual) [[likely]] {
    return true;
  }

  std::pmr::string msg{state.core.GetAllocator()};
  std::format_to(std::back_inserter(msg), "{} must belong to the same Context",
                 what);
  state.last_error.assign(msg.data(), msg.size());
  return false;
}

}  // namespace

auto Context::LoadSchema(std::string_view json,
                         std::optional<std::string_view> include_base)
    -> std::optional<Schema> {
  const auto base = include_base.has_value()
                        ? std::filesystem::path{*include_base}
                        : std::filesystem::path{};
  auto loaded = cccr::bindings::LoadSchema(state_->core, json, base);
  if (!loaded) [[unlikely]] {
    state_->SetError(loaded.error());
    return std::nullopt;
  }

  state_->ClearError();
  return Schema{state_, std::move(*loaded), base};
}

auto Context::LoadXml(std::string_view xml) -> std::optional<Source> {
  auto loaded = LoadXmlSource(state_->core, xml);
  if (!loaded) [[unlikely]] {
    state_->SetError(loaded.error());
    return std::nullopt;
  }

  state_->ClearError();
  return Source{state_, std::move(*loaded)};
}

auto Context::LoadCsv(std::string_view csv) -> std::optional<Source> {
  auto loaded = LoadCsvSource(state_->core, csv);
  if (!loaded) [[unlikely]] {
    state_->SetError(loaded.error());
    return std::nullopt;
  }

  state_->ClearError();
  return Source{state_, std::move(*loaded)};
}

auto Context::LoadGraph(std::string_view text, std::string_view format)
    -> std::optional<Graph> {
  const auto syntax = ParseGraphFormat(format);
  if (!syntax) [[unlikely]] {
    state_->SetError(syntax.error());
    return std::nullopt;
  }

  auto loaded = cccr::bindings::LoadGraph(state_->core, text, *syntax);
  if (!loaded) [[unlikely]] {
    state_->SetError(loaded.error());
    return std::nullopt;
  }

  state_->ClearError();
  return Graph{state_, std::move(*loaded)};
}

auto Context::Run(const Schema& schema, Source& source, const Graph* knowledge)
    -> std::optional<Graph> {
  auto ref = std::reference_wrapper<Source>{source};
  return RunMany(schema, std::span{&ref, 1}, knowledge);
}

auto Context::RunMany(const Schema& schema,
                      std::span<std::reference_wrapper<Source>> sources,
                      const Graph* knowledge) -> std::optional<Graph> {
  if (sources.empty()) [[unlikely]] {
    state_->SetError(BindingError::kInvalidArg);
    return std::nullopt;
  }

  for (Source& source : sources) {
    if (!EnsureSameContext(*state_, state_, source.State(), "source"))
        [[unlikely]] {
      return std::nullopt;
    }
  }

  if (!EnsureSameContext(*state_, state_, schema.State(), "schema"))
      [[unlikely]] {
    return std::nullopt;
  }

  if (knowledge != nullptr) {
    if (!EnsureSameContext(*state_, state_, knowledge->State(), "knowledge"))
        [[unlikely]] {
      return std::nullopt;
    }
  }

  std::vector<std::reference_wrapper<input::Source>> refs;
  refs.reserve(sources.size());
  for (Source& source : sources) {
    refs.emplace_back(source.Get());
  }

  const rdf::Graph* knowledge_ptr =
      knowledge != nullptr ? &knowledge->Get() : nullptr;

  auto ran = RunEngine(state_->core, schema.Get(), refs, knowledge_ptr,
                       schema.IncludeBase());
  if (!ran) [[unlikely]] {
    state_->SetError(ran.error());
    return std::nullopt;
  }

  state_->ClearError();
  return Graph{state_, std::move(*ran)};
}

}  // namespace cccr::bindings::python

#pragma once

#include <cccr/context.hpp>
#include <cccr/csv/error.hpp>
#include <cccr/csv/table.hpp>
#include <cccr/input/source.hpp>
#include <cccr/xml/source.hpp>

#include <string_view>

namespace cccr::csv {

/// @brief CSV input backend: parses CSV, projects to XML, evaluates via XQuery.
class CsvSource final : public input::Source {
public:
  /// @brief Constructs an unloaded source that allocates via @p ctx.
  /// @param ctx Shared allocator context
  explicit CsvSource(Context ctx = {})
      : ctx_(ctx), table_(Table::FromContext(ctx)), xml_(ctx) {}
  CsvSource(const CsvSource&) = delete;
  CsvSource(CsvSource&&) = delete;

  /// @brief Destroys the source and releases the projected document.
  ~CsvSource() override = default;

  CsvSource& operator=(const CsvSource&) = delete;
  CsvSource& operator=(CsvSource&&) = delete;

  /// @brief Parses @p csv, projects to XML, and replaces any previous document.
  /// @param csv CSV text
  /// @return Empty success or CsvError
  [[nodiscard]] auto Load(std::string_view csv) -> CsvResult<void>;

  /// @brief Evaluates an XQuery expression to a string against the projection.
  /// @param context Context item
  /// @param query XQuery / XPath expression
  /// @return String value or InputError
  [[nodiscard]] auto EvalString(input::NodeRef context, input::Query query)
      -> input::InputResult<std::pmr::string> override {
    return xml_.EvalString(context, query);
  }

  /// @brief Evaluates an XQuery expression to a node sequence.
  /// @param context Context item
  /// @param query XQuery / XPath expression
  /// @return Nodes (valid for this source's lifetime) or InputError
  [[nodiscard]] auto EvalNodes(input::NodeRef context, input::Query query)
      -> input::InputResult<std::pmr::vector<input::NodeRef>> override {
    return xml_.EvalNodes(context, query);
  }

  /// @brief Returns whether no document is loaded.
  /// @return `true` if unloaded
  [[nodiscard]] bool Empty() const noexcept { return xml_.Empty(); }

  /// @brief Returns the projected document root element.
  /// @return Root node reference
  [[nodiscard]] input::NodeRef Root() const noexcept override {
    return xml_.Root();
  }

private:
  Context ctx_;
  Table table_;
  xml::XmlSource xml_;
};

}  // namespace cccr::csv

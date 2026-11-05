#pragma once

#include <cccr/context.hpp>
#include <cccr/input/source.hpp>
#include <cccr/xml/error.hpp>
#include <cccr/xml/node.hpp>
#include <cccr/xml/processor.hpp>

#include <string_view>

namespace cccr::xml {

[[nodiscard]] constexpr input::InputError ToInputError(
    XmlError error) noexcept {
  switch (error) {
    using enum XmlError;
    case kParseFailed:
      return input::InputError::kParseFailed;
    case kEvalFailed:
    default:
      return input::InputError::kEvalFailed;
  }
}

/// @brief XML/HTML input backend: owns a Saxon session and parsed document.
class XmlSource final : public input::Source {
public:
  /// @brief Constructs an unloaded source that allocates via @p ctx.
  /// @param ctx Shared allocator context
  explicit XmlSource(Context ctx = {}) : processor_(ctx) {}
  XmlSource(const XmlSource&) = delete;
  XmlSource(XmlSource&&) = delete;

  /// @brief Destroys the source and releases the Saxon document.
  ~XmlSource() override = default;

  XmlSource& operator=(const XmlSource&) = delete;
  XmlSource& operator=(XmlSource&&) = delete;

  /// @brief Parses @p xml and replaces any previously loaded document.
  /// @param xml XML text
  /// @return Empty success or XmlError
  [[nodiscard]] auto Load(std::string_view xml) -> XmlResult<void>;

  /// @brief Evaluates an XQuery expression to a string.
  /// @param context Context item
  /// @param query XQuery / XPath expression
  /// @return String value or InputError
  [[nodiscard]] auto EvalString(input::NodeRef context, input::Query query)
      -> input::InputResult<std::pmr::string> override;

  /// @brief Evaluates an XQuery expression to a node sequence.
  /// @param context Context item
  /// @param query XQuery / XPath expression
  /// @return Nodes (valid for this source's lifetime) or InputError
  [[nodiscard]] auto EvalNodes(input::NodeRef context, input::Query query)
      -> input::InputResult<std::pmr::vector<input::NodeRef>> override;

  /// @brief Returns whether a document is loaded.
  /// @return `true` if no document is loaded
  [[nodiscard]] bool Empty() const noexcept { return document_.Empty(); }

  /// @brief Returns the document root element.
  /// @return Root node reference
  [[nodiscard]] input::NodeRef Root() const noexcept override {
    return document_.Root();
  }

private:
  Processor processor_;
  Document document_;
};

inline auto XmlSource::Load(std::string_view xml) -> XmlResult<void> {
  auto document = processor_.Parse(xml);
  if (!document) [[unlikely]] {
    return std::unexpected(document.error());
  }
  document_ = std::move(*document);
  return {};
}

inline auto XmlSource::EvalString(input::NodeRef context, input::Query query)
    -> input::InputResult<std::pmr::string> {
  auto result = processor_.EvalString(context, query);
  if (!result) [[unlikely]] {
    return std::unexpected(ToInputError(result.error()));
  }
  return std::move(*result);
}

inline auto XmlSource::EvalNodes(input::NodeRef context, input::Query query)
    -> input::InputResult<std::pmr::vector<input::NodeRef>> {
  auto result = processor_.EvalNodes(context, query);
  if (!result) [[unlikely]] {
    return std::unexpected(ToInputError(result.error()));
  }
  return std::move(*result);
}

}  // namespace cccr::xml

#pragma once

#include <cccr/context.hpp>
#include <cccr/input/query.hpp>
#include <cccr/xml/error.hpp>
#include <cccr/xml/node.hpp>

#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace cccr::xml {

/// @brief Saxon session: parse XML and evaluate XQuery against a context item.
class Processor {
public:
  /// @brief Constructs a processor that allocates via @p ctx.
  /// @param ctx Shared allocator context
  explicit Processor(Context ctx = {});
  Processor(const Processor&) = delete;

  /// @brief Moves the Saxon session from @p other.
  /// @param other Processor to move from
  Processor(Processor&&) noexcept;

  /// @brief Destroys the Saxon session and retained node results.
  ~Processor();

  Processor& operator=(const Processor&) = delete;
  Processor& operator=(Processor&&) noexcept;

  /// @brief Parses an XML document from a UTF-8 string.
  /// @param xml XML text
  /// @return Owned document or XmlError
  [[nodiscard]] auto Parse(std::string_view xml) -> XmlResult<Document>;

  /// @brief Evaluates an XQuery expression and returns a single string value.
  /// @param context Context item (document or element node)
  /// @param query XQuery / XPath expression
  /// @return String result (empty sequence → empty string) or XmlError
  [[nodiscard]] auto EvalString(NodeRef context, input::Query query)
      -> XmlResult<std::pmr::string>;

  /// @brief Evaluates an XQuery expression expected to yield nodes.
  /// @param context Context item
  /// @param query XQuery / XPath expression
  /// @return Node sequence (caller does not own the nodes; valid until
  ///         Processor destruction) or XmlError
  [[nodiscard]] auto EvalNodes(NodeRef context, input::Query query)
      -> XmlResult<std::pmr::vector<NodeRef>>;

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace cccr::xml

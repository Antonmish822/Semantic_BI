#include <pch.hpp>

#include <cccr/xml/processor.hpp>

#include <cccr/context.hpp>
#include <cccr/input/query.hpp>
#include <cccr/log/log.hpp>

#include <saxonc/SaxonApiException.h>
#include <saxonc/SaxonProcessor.h>
#include <saxonc/XQueryProcessor.h>
#include <saxonc/XdmItem.h>
#include <saxonc/XdmNode.h>
#include <saxonc/XdmValue.h>

#include <expected>
#include <memory>
#include <memory_resource>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace cccr::xml {

namespace {

void LogSaxonException(std::string_view operation, SaxonApiException& ex,
                       Context ctx) noexcept {
  const char* message = ex.getMessageWithErrorCode();
  if (message == nullptr || message[0] == '\0') {
    message = ex.getMessage();
  }

  if (message == nullptr || message[0] == '\0') {
    message = "unknown Saxon error";
  }

  const int line = ex.getLineNumber();
  const char* system_id = ex.getSystemId();
  if (line >= 0 && system_id != nullptr && system_id[0] != '\0') {
    log::Error(ctx, "{} failed: {} (line {}, systemId={})", operation, message,
               line, system_id);
    return;
  }

  if (line >= 0) {
    log::Error(ctx, "{} failed: {} (line {})", operation, message, line);
    return;
  }
  log::Error(ctx, "{} failed: {}", operation, message);
}

}  // namespace

struct Processor::Impl {
  std::pmr::memory_resource* allocator = std::pmr::get_default_resource();
  SaxonProcessor processor{/*license=*/false};
  XQueryProcessor* query = nullptr;
  // Retains every EvalNodes result for the Processor lifetime so NodeRefs
  // remain valid across nested subject selection and deferred walks.
  std::pmr::vector<std::unique_ptr<XdmValue>> retained_node_values;
  std::pmr::vector<std::unique_ptr<XdmItem>> retained_node_items;

  explicit Impl(std::pmr::memory_resource* mr)
      : allocator(mr),
        retained_node_values(
            std::pmr::polymorphic_allocator<std::unique_ptr<XdmValue>>{mr}),
        retained_node_items(
            std::pmr::polymorphic_allocator<std::unique_ptr<XdmItem>>{mr}) {
    query = processor.newXQueryProcessor();
  }

  Impl(const Impl&) = delete;
  Impl(Impl&&) noexcept = default;
  ~Impl() {
    retained_node_items.clear();
    retained_node_values.clear();
    delete query;
    query = nullptr;
  }

  Impl& operator=(const Impl&) = delete;
  Impl& operator=(Impl&&) noexcept = default;
};

Processor::Processor(Context ctx)
    : impl_(std::make_unique<Impl>(ctx.GetAllocator())) {}
Processor::Processor(Processor&&) noexcept = default;
Processor::~Processor() = default;

Processor& Processor::operator=(Processor&&) noexcept = default;

auto Processor::Parse(std::string_view xml) -> XmlResult<Document> {
  const Context ctx{.allocator = impl_->allocator};
  try {
    const std::pmr::string owned{xml, impl_->allocator};
    XdmNode* root = impl_->processor.parseXmlFromString(owned.c_str());
    if (root == nullptr) [[unlikely]] {
      log::Error(ctx, "XML parse failed: Saxon returned a null document");
      return std::unexpected(XmlError::kParseFailed);
    }
    return Document{root};
  } catch (SaxonApiException& ex) {
    LogSaxonException("XML parse", ex, ctx);
    return std::unexpected(XmlError::kParseFailed);
  } catch (...) {
    log::Error(ctx, "XML parse failed: unknown exception");
    return std::unexpected(XmlError::kParseFailed);
  }
}

auto Processor::EvalString(NodeRef context, input::Query query)
    -> XmlResult<std::pmr::string> {
  const Context ctx{.allocator = impl_->allocator};
  if (context.Empty() || impl_->query == nullptr) [[unlikely]] {
    log::Error(ctx,
               "XQuery evaluation failed: empty context node or no processor");
    return std::unexpected(XmlError::kEvalFailed);
  }

  try {
    const std::pmr::string owned_query{query.Text(), impl_->allocator};
    impl_->query->clearProperties();
    impl_->query->clearParameters(/*deleteValues=*/false);
    impl_->query->setContextItem(static_cast<XdmItem*>(context.Handle()));
    impl_->query->setQueryContent(owned_query.c_str());

    // Prefer XDM string-value over serialized XML (avoids XML declaration
    // noise).
    std::unique_ptr<XdmValue> value{impl_->query->runQueryToValue()};
    if (value == nullptr) {
      return std::pmr::string{impl_->allocator};
    }

    std::pmr::string out{impl_->allocator};
    const int count = value->size();
    for (int i = 0; i < count; ++i) {
      std::unique_ptr<XdmItem> item{value->itemAt(i)};
      if (item == nullptr) {
        continue;
      }
      const char* piece = item->getStringValue();
      if (piece != nullptr) {
        if (!out.empty()) {
          out.push_back(' ');
        }
        out += piece;
      }
    }
    return out;
  } catch (SaxonApiException& ex) {
    LogSaxonException("XQuery evaluation", ex, ctx);
    return std::unexpected(XmlError::kEvalFailed);
  } catch (...) {
    log::Error(ctx, "XQuery evaluation failed: unknown exception");
    return std::unexpected(XmlError::kEvalFailed);
  }
}

auto Processor::EvalNodes(NodeRef context, input::Query query)
    -> XmlResult<std::pmr::vector<NodeRef>> {
  const Context ctx{.allocator = impl_->allocator};
  if (context.Empty() || impl_->query == nullptr) [[unlikely]] {
    log::Error(ctx,
               "XQuery node evaluation failed: empty context node or no "
               "processor");
    return std::unexpected(XmlError::kEvalFailed);
  }

  try {
    const std::pmr::string owned_query{query.Text(), impl_->allocator};
    impl_->query->clearProperties();
    impl_->query->clearParameters(/*deleteValues=*/false);
    impl_->query->setContextItem(static_cast<XdmItem*>(context.Handle()));
    impl_->query->setQueryContent(owned_query.c_str());
    std::unique_ptr<XdmValue> value{impl_->query->runQueryToValue()};
    if (value == nullptr) {
      return std::pmr::vector<NodeRef>{impl_->allocator};
    }

    std::pmr::vector<NodeRef> nodes{impl_->allocator};
    const int count = value->size();
    nodes.reserve(static_cast<size_t>(count));
    for (int i = 0; i < count; ++i) {
      std::unique_ptr<XdmItem> item{value->itemAt(i)};
      if (item == nullptr) {
        continue;
      }
      nodes.emplace_back(item.get());
      impl_->retained_node_items.push_back(std::move(item));
    }
    impl_->retained_node_values.push_back(std::move(value));
    return nodes;
  } catch (SaxonApiException& ex) {
    LogSaxonException("XQuery node evaluation", ex, ctx);
    return std::unexpected(XmlError::kEvalFailed);
  } catch (...) {
    log::Error(ctx, "XQuery node evaluation failed: unknown exception");
    return std::unexpected(XmlError::kEvalFailed);
  }
}

}  // namespace cccr::xml

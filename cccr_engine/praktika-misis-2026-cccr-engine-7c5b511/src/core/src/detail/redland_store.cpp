#include <pch.hpp>

#include <detail/redland_store.hpp>

#include <cccr/assert.hpp>
#include <cccr/context.hpp>
#include <cccr/log/log.hpp>
#include <cccr/rdf/term.hpp>

#include <redland.h>

#include <cstddef>
#include <expected>
#include <memory>
#include <memory_resource>
#include <optional>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace cccr::detail {

namespace {

/// @brief Routes Redland/raptor/rasqal log messages through @ref cccr::log.
/// @details Replaces the default handler, which can print garbled bytes before
///          the message text on some rasqal warning paths.
[[nodiscard]] int RedlandLogHandler(void* user_data,
                                    librdf_log_message* message) {
  if (message == nullptr) {
    return 1;
  }

  const char* text = librdf_log_message_message(message);
  if (text == nullptr || text[0] == '\0') {
    return 1;
  }

  const Context ctx{.allocator =
                        user_data != nullptr
                            ? static_cast<std::pmr::memory_resource*>(user_data)
                            : std::pmr::get_default_resource()};

  switch (librdf_log_message_level(message)) {
    case LIBRDF_LOG_DEBUG:
      log::Debug(ctx, "redland: {}", text);
      break;
    case LIBRDF_LOG_INFO:
      log::Info(ctx, "redland: {}", text);
      break;
    case LIBRDF_LOG_WARN:
      log::Warn(ctx, "redland: {}", text);
      break;
    case LIBRDF_LOG_ERROR:
    case LIBRDF_LOG_FATAL:
      log::Error(ctx, "redland: {}", text);
      break;
    case LIBRDF_LOG_NONE:
    default:
      break;
  }
  return 1;
}

[[nodiscard]] const unsigned char* AsBytes(std::string_view text) noexcept {
  return reinterpret_cast<const unsigned char*>(text.data());
}

[[nodiscard]] librdf_node* MakeUriNode(librdf_world* world,
                                       std::string_view iri) noexcept {
  return librdf_new_node_from_uri_string(world, AsBytes(iri));
}

[[nodiscard]] librdf_node* MakeLiteralNode(librdf_world* world,
                                           const rdf::Literal& literal) {
  if (literal.datatype) {
    librdf_uri* datatype = librdf_new_uri(world, AsBytes(*literal.datatype));
    if (datatype == nullptr) [[unlikely]] {
      return nullptr;
    }

    librdf_node* node = librdf_new_node_from_typed_literal(
        world, AsBytes(literal.value), /*xml_language=*/nullptr, datatype);
    librdf_free_uri(datatype);
    return node;
  }

  const char* language = nullptr;
  if (literal.language) {
    language = literal.language->c_str();
  }
  return librdf_new_node_from_literal(world, AsBytes(literal.value), language,
                                      /*is_wf_xml=*/0);
}

[[nodiscard]] librdf_node* MakeObjectNode(librdf_world* world,
                                          const rdf::Term& term) {
  if (const auto* iri = std::get_if<rdf::Iri>(&term)) {
    return MakeUriNode(world, iri->value);
  }
  return MakeLiteralNode(world, std::get<rdf::Literal>(term));
}

[[nodiscard]] auto NodeToIri(librdf_node* node,
                             std::pmr::memory_resource* allocator)
    -> std::optional<rdf::Iri> {
  if (node == nullptr || librdf_node_is_resource(node) == 0) {
    return std::nullopt;
  }

  librdf_uri* uri = librdf_node_get_uri(node);
  if (uri == nullptr) [[unlikely]] {
    return std::nullopt;
  }

  const unsigned char* text = librdf_uri_as_string(uri);
  if (text == nullptr) [[unlikely]] {
    return std::nullopt;
  }

  return rdf::Iri::From(
      std::pmr::string{reinterpret_cast<const char*>(text), allocator});
}

[[nodiscard]] auto NodeToTerm(librdf_node* node,
                              std::pmr::memory_resource* allocator)
    -> std::optional<rdf::Term> {
  if (node == nullptr) {
    return std::nullopt;
  }

  if (librdf_node_is_resource(node) != 0) {
    auto iri = NodeToIri(node, allocator);
    if (!iri) {
      return std::nullopt;
    }
    return std::move(*iri);
  }

  if (librdf_node_is_literal(node) == 0) {
    return std::nullopt;
  }

  const unsigned char* value = librdf_node_get_literal_value(node);
  if (value == nullptr) [[unlikely]] {
    return std::nullopt;
  }

  std::optional<std::pmr::string> datatype;
  if (librdf_uri* datatype_uri =
          librdf_node_get_literal_value_datatype_uri(node);
      datatype_uri != nullptr) {
    const unsigned char* datatype_text = librdf_uri_as_string(datatype_uri);
    if (datatype_text == nullptr) [[unlikely]] {
      return std::nullopt;
    }
    datatype = std::pmr::string{reinterpret_cast<const char*>(datatype_text),
                                allocator};
  }

  std::optional<std::pmr::string> language;
  if (const char* lang = librdf_node_get_literal_value_language(node);
      lang != nullptr && lang[0] != '\0') {
    language = std::pmr::string{lang, allocator};
  }

  return rdf::Literal::From(
      std::pmr::string{reinterpret_cast<const char*>(value), allocator},
      std::move(datatype), std::move(language));
}

[[nodiscard]] auto NodeToLexical(librdf_node* node,
                                 std::pmr::memory_resource* allocator)
    -> std::optional<std::pmr::string> {
  if (node == nullptr) {
    return std::nullopt;
  }

  if (librdf_node_is_literal(node) != 0) {
    const unsigned char* value = librdf_node_get_literal_value(node);
    if (value == nullptr) [[unlikely]] {
      return std::nullopt;
    }
    return std::pmr::string{reinterpret_cast<const char*>(value), allocator};
  }

  if (librdf_node_is_resource(node) != 0) {
    librdf_uri* uri = librdf_node_get_uri(node);
    if (uri == nullptr) [[unlikely]] {
      return std::nullopt;
    }

    const unsigned char* text = librdf_uri_as_string(uri);
    if (text == nullptr) [[unlikely]] {
      return std::nullopt;
    }
    return std::pmr::string{reinterpret_cast<const char*>(text), allocator};
  }

  if (librdf_node_is_blank(node) != 0) {
    const unsigned char* id = librdf_node_get_blank_identifier(node);
    if (id == nullptr) [[unlikely]] {
      return std::nullopt;
    }
    return std::pmr::string{reinterpret_cast<const char*>(id), allocator};
  }

  return std::nullopt;
}

}  // namespace

RedlandStore::RedlandStore(librdf_world* world, librdf_storage* storage,
                           librdf_model* model,
                           std::pmr::memory_resource* allocator) noexcept
    : world_(world),
      storage_(storage),
      model_(model),
      allocator_(allocator != nullptr ? allocator
                                      : std::pmr::get_default_resource()) {}

RedlandStore::RedlandStore(RedlandStore&& other) noexcept
    : world_(other.world_),
      storage_(other.storage_),
      model_(other.model_),
      allocator_(other.allocator_) {
  other.world_ = nullptr;
  other.storage_ = nullptr;
  other.model_ = nullptr;
  other.allocator_ = std::pmr::get_default_resource();
}

auto RedlandStore::operator=(RedlandStore&& other) noexcept -> RedlandStore& {
  if (this == &other) [[unlikely]] {
    return *this;
  }

  Reset();
  world_ = std::exchange(other.world_, nullptr);
  storage_ = std::exchange(other.storage_, nullptr);
  model_ = std::exchange(other.model_, nullptr);
  allocator_ =
      std::exchange(other.allocator_, std::pmr::get_default_resource());

  return *this;
}

auto RedlandStore::Create(std::pmr::memory_resource* allocator)
    -> std::optional<RedlandStore> {
  auto* const mr =
      allocator != nullptr ? allocator : std::pmr::get_default_resource();

  librdf_world* world = librdf_new_world();
  if (world == nullptr) [[unlikely]] {
    return std::nullopt;
  }
  librdf_world_open(world);
  librdf_world_set_logger(world, mr, RedlandLogHandler);

  librdf_storage* storage = librdf_new_storage(
      world, "memory", /*name=*/nullptr, /*options=*/nullptr);
  if (storage == nullptr) [[unlikely]] {
    librdf_free_world(world);
    return std::nullopt;
  }

  librdf_model* model = librdf_new_model(world, storage, /*options=*/nullptr);
  if (model == nullptr) [[unlikely]] {
    librdf_free_storage(storage);
    librdf_free_world(world);
    return std::nullopt;
  }

  return RedlandStore{world, storage, model, mr};
}

void RedlandStore::Reset() noexcept {
  if (model_ != nullptr) {
    librdf_free_model(model_);
    model_ = nullptr;
  }

  if (storage_ != nullptr) {
    librdf_free_storage(storage_);
    storage_ = nullptr;
  }

  if (world_ != nullptr) {
    librdf_free_world(world_);
    world_ = nullptr;
  }
}

bool RedlandStore::Add(const rdf::Triple& triple) {
  CCCR_ASSERT(world_ != nullptr && model_ != nullptr,
              "RedlandStore used after move!");

  librdf_node* subject = MakeUriNode(world_, triple.subject.value);
  librdf_node* predicate = MakeUriNode(world_, triple.predicate.value);
  librdf_node* object = MakeObjectNode(world_, triple.object);
  if (subject == nullptr || predicate == nullptr || object == nullptr)
      [[unlikely]] {
    if (subject != nullptr) {
      librdf_free_node(subject);
    }

    if (predicate != nullptr) {
      librdf_free_node(predicate);
    }

    if (object != nullptr) {
      librdf_free_node(object);
    }
    return false;
  }

  // librdf_model_add takes ownership of the three nodes.
  return librdf_model_add(model_, subject, predicate, object) == 0;
}

bool RedlandStore::Parse(std::string_view text, std::string_view parser_name) {
  CCCR_ASSERT(world_ != nullptr && model_ != nullptr,
              "RedlandStore used after move!");
  CCCR_ASSERT(!parser_name.empty(), "RDF parser name must not be empty!");

  // librdf expects null-terminated C strings for name and document.
  const std::pmr::string name{parser_name, allocator_};
  librdf_parser* parser =
      librdf_new_parser(world_, name.c_str(), /*mime_type=*/nullptr,
                        /*type_uri=*/nullptr);
  if (parser == nullptr) [[unlikely]] {
    return false;
  }

  const std::pmr::string owned{text, allocator_};

  // Absolute documents do not need a base; keep a dummy for relative IRIs.
  librdf_uri* base = librdf_new_uri(world_, AsBytes("http://cccr.local/base"));
  const int rc = librdf_parser_parse_string_into_model(parser, AsBytes(owned),
                                                       base, model_);
  if (base != nullptr) {
    librdf_free_uri(base);
  }

  librdf_free_parser(parser);
  return rc == 0;
}

auto RedlandStore::SerializeRdfXml(std::pmr::memory_resource* allocator) const
    -> std::optional<std::pmr::string> {
  CCCR_ASSERT(world_ != nullptr && model_ != nullptr,
              "RedlandStore used after move!");
  auto* const mr = ResolveAllocator(allocator);

  librdf_serializer* serializer = librdf_new_serializer(
      world_, "rdfxml", /*mime_type=*/nullptr, /*type_uri=*/nullptr);
  if (serializer == nullptr) [[unlikely]] {
    return std::nullopt;
  }

  unsigned char* xml = librdf_serializer_serialize_model_to_string(
      serializer, /*base_uri=*/nullptr, model_);
  librdf_free_serializer(serializer);
  if (xml == nullptr) [[unlikely]] {
    return std::nullopt;
  }

  std::pmr::string out{reinterpret_cast<const char*>(xml), mr};
  librdf_free_memory(xml);
  return out;
}

auto RedlandStore::SelectFirst(std::string_view query,
                               std::pmr::memory_resource* allocator) const
    -> std::expected<std::pmr::string, RedlandQueryError> {
  CCCR_ASSERT(world_ != nullptr && model_ != nullptr,
              "RedlandStore used after move!");
  auto* const result_mr = ResolveAllocator(allocator);

  // Null-terminated copy: librdf_new_query expects a C string.
  const std::pmr::string owned{query, allocator_};
  librdf_query* rdf_query = librdf_new_query(
      world_, "sparql", /*uri=*/nullptr, AsBytes(owned), /*base_uri=*/nullptr);
  if (rdf_query == nullptr) [[unlikely]] {
    return std::unexpected(RedlandQueryError::kFailed);
  }

  librdf_query_results* results = librdf_model_query_execute(model_, rdf_query);
  if (results == nullptr) [[unlikely]] {
    librdf_free_query(rdf_query);
    return std::unexpected(RedlandQueryError::kFailed);
  }

  if (librdf_query_results_is_bindings(results) == 0) [[unlikely]] {
    librdf_free_query_results(results);
    librdf_free_query(rdf_query);
    return std::unexpected(RedlandQueryError::kFailed);
  }

  if (librdf_query_results_finished(results) != 0) {
    librdf_free_query_results(results);
    librdf_free_query(rdf_query);
    return std::unexpected(RedlandQueryError::kNoResult);
  }

  librdf_node* node =
      librdf_query_results_get_binding_value(results, /*offset=*/0);
  if (node == nullptr) [[unlikely]] {
    librdf_free_query_results(results);
    librdf_free_query(rdf_query);
    return std::unexpected(RedlandQueryError::kNoResult);
  }

  auto binding = NodeToLexical(node, result_mr);
  librdf_free_node(node);
  librdf_free_query_results(results);
  librdf_free_query(rdf_query);
  if (!binding) [[unlikely]] {
    return std::unexpected(RedlandQueryError::kFailed);
  }

  return std::move(*binding);
}

auto RedlandStore::QueryFormatted(std::string_view query,
                                  std::string_view format_name,
                                  std::pmr::memory_resource* allocator) const
    -> std::expected<std::pmr::string, RedlandQueryError> {
  CCCR_ASSERT(world_ != nullptr && model_ != nullptr,
              "RedlandStore used after move!");
  auto* const mr = ResolveAllocator(allocator);

  const std::pmr::string owned_query{query, allocator_};
  const std::pmr::string owned_format{format_name, allocator_};
  librdf_query* rdf_query =
      librdf_new_query(world_, "sparql", /*uri=*/nullptr, AsBytes(owned_query),
                       /*base_uri=*/nullptr);
  if (rdf_query == nullptr) [[unlikely]] {
    return std::unexpected(RedlandQueryError::kFailed);
  }

  librdf_query_results* results = librdf_model_query_execute(model_, rdf_query);
  if (results == nullptr) [[unlikely]] {
    librdf_free_query(rdf_query);
    return std::unexpected(RedlandQueryError::kFailed);
  }

  unsigned char* text = librdf_query_results_to_string2(
      results, owned_format.c_str(), /*mime_type=*/nullptr,
      /*format_uri=*/nullptr, /*base_uri=*/nullptr);
  librdf_free_query_results(results);
  librdf_free_query(rdf_query);
  if (text == nullptr) [[unlikely]] {
    return std::unexpected(RedlandQueryError::kFailed);
  }

  std::pmr::string out{reinterpret_cast<const char*>(text), mr};
  librdf_free_memory(text);
  return out;
}

auto RedlandStore::MaterializeTriples(std::pmr::memory_resource* allocator)
    const -> std::optional<std::pmr::vector<rdf::Triple>> {
  CCCR_ASSERT(world_ != nullptr && model_ != nullptr,
              "RedlandStore used after move!");
  auto* const mr = ResolveAllocator(allocator);

  librdf_stream* stream = librdf_model_as_stream(model_);
  if (stream == nullptr) [[unlikely]] {
    return std::nullopt;
  }

  std::pmr::vector<rdf::Triple> triples{
      std::pmr::polymorphic_allocator<rdf::Triple>{mr}};

  while (librdf_stream_end(stream) == 0) {
    librdf_statement* statement = librdf_stream_get_object(stream);
    if (statement == nullptr) [[unlikely]] {
      librdf_free_stream(stream);
      return std::nullopt;
    }

    auto subject = NodeToIri(librdf_statement_get_subject(statement), mr);
    auto predicate = NodeToIri(librdf_statement_get_predicate(statement), mr);
    auto object = NodeToTerm(librdf_statement_get_object(statement), mr);
    if (!subject || !predicate || !object) [[unlikely]] {
      librdf_free_stream(stream);
      return std::nullopt;
    }

    triples.push_back(rdf::Triple::From(
        std::move(*subject), std::move(*predicate), std::move(*object)));
    librdf_stream_next(stream);
  }

  librdf_free_stream(stream);
  return triples;
}

}  // namespace cccr::detail

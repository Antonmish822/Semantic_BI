#pragma once

#include <cccr/ast/types.hpp>
#include <cccr/bindings/common/error.hpp>
#include <cccr/context.hpp>
#include <cccr/input/source.hpp>
#include <cccr/rdf/graph.hpp>
#include <cccr/rdf/syntax.hpp>
#include <cccr/sparql/results.hpp>

#include <filesystem>
#include <functional>
#include <memory>
#include <span>
#include <string>
#include <string_view>

namespace cccr::bindings {

/// @brief Loads a CCCR schema from JSON using @p ctx.
/// @param ctx Shared allocator context
/// @param schema_json UTF-8 CCCR JSON schema text
/// @param include_base Directory for resolving relative `include` paths
/// @return Parsed schema, or BindingError
[[nodiscard]] auto LoadSchema(Context ctx, std::string_view schema_json,
                              const std::filesystem::path& include_base = {})
    -> BindingResult<ast::Schema>;

/// @brief Loads an XML/HTML document into a Source owned by the caller.
/// @param ctx Shared allocator context
/// @param xml UTF-8 XML text
/// @return Loaded source, or BindingError
[[nodiscard]] auto LoadXmlSource(Context ctx, std::string_view xml)
    -> BindingResult<std::unique_ptr<input::Source>>;

/// @brief Loads a CSV document into a Source owned by the caller.
/// @param ctx Shared allocator context
/// @param csv UTF-8 CSV text
/// @return Loaded source, or BindingError
[[nodiscard]] auto LoadCsvSource(Context ctx, std::string_view csv)
    -> BindingResult<std::unique_ptr<input::Source>>;

/// @brief Loads an RDF document into a graph.
/// @param ctx Shared allocator context
/// @param text UTF-8 RDF document text
/// @param syntax Serialization of @p text
/// @return Graph, or BindingError
[[nodiscard]] auto LoadGraph(Context ctx, std::string_view text,
                             rdf::RdfSyntax syntax)
    -> BindingResult<rdf::Graph>;

/// @brief Runs @p schema over @p source and returns the output graph.
/// @param ctx Shared allocator context
/// @param schema Parsed CCCR schema
/// @param source Loaded input document
/// @param knowledge Optional seed / knowledge graph; nullptr skips
/// @param include_base Directory for resolving relative `include` paths
/// @return Output RDF graph, or BindingError
[[nodiscard]] auto RunEngine(Context ctx, const ast::Schema& schema,
                             input::Source& source,
                             const rdf::Graph* knowledge = nullptr,
                             const std::filesystem::path& include_base = {})
    -> BindingResult<rdf::Graph>;

/// @brief Runs @p schema over each source in @p sources and merges outputs.
/// @param ctx Shared allocator context
/// @param schema Parsed CCCR schema
/// @param sources Loaded input documents (at least one)
/// @param knowledge Optional seed / knowledge graph; nullptr skips
/// @param include_base Directory for resolving relative `include` paths
/// @return Merged output RDF graph, or BindingError
[[nodiscard]] auto RunEngine(
    Context ctx, const ast::Schema& schema,
    std::span<std::reference_wrapper<input::Source>> sources,
    const rdf::Graph* knowledge = nullptr,
    const std::filesystem::path& include_base = {})
    -> BindingResult<rdf::Graph>;

/// @brief Appends all triples from @p other into @p dest.
/// @param dest Destination graph (mutated)
/// @param other Source graph (must not alias @p dest)
/// @return Empty result on success, or BindingError
[[nodiscard]] auto MergeGraph(rdf::Graph& dest, const rdf::Graph& other)
    -> BindingResult<void>;

/// @brief Serializes @p graph to an owned RDF/XML string.
/// @param graph Graph to serialize
/// @return RDF/XML text, or BindingError
[[nodiscard]] auto SerializeRdfXml(const rdf::Graph& graph)
    -> BindingResult<std::string>;

/// @brief Runs a SPARQL SELECT over @p graph and formats the results.
/// @param ctx Shared allocator context
/// @param graph Knowledge graph
/// @param sparql SPARQL query text
/// @param format Output format
/// @return Formatted result document, or BindingError
[[nodiscard]] auto QueryGraph(Context ctx, const rdf::Graph& graph,
                              std::string_view sparql,
                              sparql::ResultFormat format)
    -> BindingResult<std::string>;

}  // namespace cccr::bindings

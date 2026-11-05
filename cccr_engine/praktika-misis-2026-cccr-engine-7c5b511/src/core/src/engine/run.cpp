#include <pch.hpp>

#include <cccr/engine/run.hpp>

#include <cccr/ast/ast.hpp>
#include <cccr/context.hpp>
#include <cccr/eval/eval.hpp>
#include <cccr/input/input.hpp>
#include <cccr/rdf/rdf.hpp>
#include <cccr/sparql/substitute.hpp>
#include <cccr/validate/validate.hpp>

#include <cctype>
#include <deque>
#include <expected>
#include <filesystem>
#include <functional>
#include <memory_resource>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace cccr::engine {

namespace {

using sparql::PseudoMap;

struct PropertyFrame {
  std::reference_wrapper<const std::pmr::vector<ast::Property>> properties;
  rdf::Iri subject;
  size_t index = 0;
  PseudoMap pseudos{std::pmr::get_default_resource()};
  std::span<const rdf::Iri> default_predicates;
};

struct DescriptionFrame {
  std::reference_wrapper<const ast::Description> description;
  PseudoMap pseudos{std::pmr::get_default_resource()};
  input::NodeRef parent_node;
  std::optional<rdf::Iri> link_subject;
  std::optional<rdf::Iri> link_predicate;
  bool allow_subject_rebind = false;
};

[[nodiscard]] PseudoMap CopyPseudoMap(const PseudoMap& source, Context ctx) {
  PseudoMap out{ctx.GetAllocator()};
  out.reserve(source.size());
  for (const auto& [name, value] : source) {
    out.emplace(std::pmr::string{name, ctx.GetAllocator()},
                std::pmr::string{value, ctx.GetAllocator()});
  }

  return out;
}

void EmitTriple(rdf::Graph& out, rdf::Graph& working, rdf::Triple triple) {
  working.Add(triple);
  out.Add(std::move(triple));
}

[[nodiscard]] auto ApplyContextPseudos(
    const ast::Context& context, input::Source& source, input::NodeRef node,
    const rdf::Graph& knowledge, PseudoMap& pseudos,
    const std::filesystem::path& include_base, Context ctx)
    -> EngineResult<void> {
  const auto eval_params = eval::EvalParams::From(include_base, ctx);
  for (const auto& [name, expr] : context.pseudo_variables) {
    auto value =
        eval::EvalToString(expr, source, node, knowledge, pseudos, eval_params);
    if (!value) [[unlikely]] {
      return std::unexpected(EngineError::kEvalFailed);
    }
    pseudos.insert_or_assign(std::pmr::string{name, ctx.GetAllocator()},
                             std::move(*value));
  }

  return {};
}

[[nodiscard]] auto SplitIriList(std::string_view text, Context ctx)
    -> std::pmr::vector<std::pmr::string> {
  auto* const mr = ctx.GetAllocator();
  std::pmr::vector<std::pmr::string> out{mr};
  size_t idx = 0;
  while (idx < text.size()) {
    while (idx < text.size() &&
           std::isspace(static_cast<unsigned char>(text[idx])) != 0) {
      ++idx;
    }

    if (idx >= text.size()) {
      break;
    }

    const size_t start = idx;
    while (idx < text.size() &&
           std::isspace(static_cast<unsigned char>(text[idx])) == 0) {
      ++idx;
    }
    out.push_back(std::pmr::string{text.substr(start, idx - start), mr});
  }

  return out;
}

[[nodiscard]] auto ResolvePredicates(const ast::PredicatesList& list,
                                     input::Source& source, input::NodeRef node,
                                     const rdf::Graph& knowledge,
                                     const PseudoMap& pseudos,
                                     const std::filesystem::path& include_base,
                                     Context ctx)
    -> EngineResult<std::pmr::vector<rdf::Iri>> {
  auto* const mr = ctx.GetAllocator();
  std::pmr::vector<rdf::Iri> predicates{mr};
  predicates.reserve(list.values.size());
  for (const auto& value : list.values) {
    predicates.push_back(rdf::Iri::From(value, ctx));
  }

  if (list.compute) {
    auto expanded =
        eval::EvalToString(*list.compute, source, node, knowledge, pseudos,
                           eval::EvalParams::From(include_base, ctx));
    if (!expanded) [[unlikely]] {
      return std::unexpected(EngineError::kEvalFailed);
    }

    for (auto& iri : SplitIriList(*expanded, ctx)) {
      predicates.push_back(rdf::Iri::From(std::move(iri)));
    }
  }

  if (predicates.empty()) [[unlikely]] {
    return std::unexpected(EngineError::kSchemaInvalid);
  }

  return predicates;
}

[[nodiscard]] auto ResolvePropertyPredicates(
    const ast::Property& property, input::Source& source, input::NodeRef node,
    const rdf::Graph& knowledge, const PseudoMap& local,
    const std::filesystem::path& include_base, Context ctx,
    std::span<const rdf::Iri> default_predicates)
    -> EngineResult<std::pmr::vector<rdf::Iri>> {
  if (property.context && property.context->predicates) {
    return ResolvePredicates(*property.context->predicates, source, node,
                             knowledge, local, include_base, ctx);
  }

  auto* const mr = ctx.GetAllocator();
  if (property.qname) {
    std::pmr::vector<rdf::Iri> predicates{mr};
    predicates.push_back(
        rdf::Iri::From(eval::QNameToIri(*property.qname, ctx)));
    return predicates;
  }

  if (!default_predicates.empty()) {
    std::pmr::vector<rdf::Iri> predicates{mr};
    predicates.reserve(default_predicates.size());
    for (const auto& predicate : default_predicates) {
      predicates.push_back(rdf::Iri::From(predicate.value, ctx));
    }
    return predicates;
  }

  return std::unexpected(EngineError::kSchemaInvalid);
}

[[nodiscard]] auto EmitTerminalProperty(
    const ast::Property& property, const rdf::Iri& subject,
    const rdf::Iri& predicate, input::Source& source, input::NodeRef node,
    const rdf::Graph& knowledge, const PseudoMap& pseudos,
    const std::filesystem::path& include_base, Context ctx, rdf::Graph& out,
    rdf::Graph& working) -> EngineResult<void> {
  const auto eval_params = eval::EvalParams::From(include_base, ctx);
  auto* const mr = ctx.GetAllocator();

  if (property.uri) {
    auto value = eval::EvalToString(*property.uri, source, node, knowledge,
                                    pseudos, eval_params);
    if (!value) [[unlikely]] {
      return std::unexpected(EngineError::kEvalFailed);
    }

    EmitTriple(out, working,
               rdf::Triple::From(subject, predicate,
                                 rdf::Iri::From(std::move(*value))));
    return {};
  }

  if (property.literal) {
    auto value = eval::EvalToString(*property.literal, source, node, knowledge,
                                    pseudos, eval_params);
    if (!value) [[unlikely]] {
      return std::unexpected(EngineError::kEvalFailed);
    }

    std::optional<std::pmr::string> datatype;
    std::optional<std::pmr::string> language;
    if (property.constraint) {
      const auto& constraint = *property.constraint;
      const auto type = constraint.datatype
                            ? std::string_view{*constraint.datatype}
                            : std::string_view{};
      std::optional<std::string_view> min;
      std::optional<std::string_view> max;
      if (constraint.min_inclusive) {
        min = *constraint.min_inclusive;
      }
      if (constraint.max_inclusive) {
        max = *constraint.max_inclusive;
      }

      auto checked = validate::CheckLiteral(*value, type, min, max);
      if (!checked) [[unlikely]] {
        return std::unexpected(EngineError::kValidationFailed);
      }

      if (constraint.datatype) {
        datatype = std::pmr::string{*constraint.datatype, mr};
      } else if (constraint.language) {
        language = std::pmr::string{*constraint.language, mr};
      }
    }

    EmitTriple(out, working,
               rdf::Triple::From(
                   subject, predicate,
                   rdf::Literal::From(std::move(*value), std::move(datatype),
                                      std::move(language))));
    return {};
  }

  return std::unexpected(EngineError::kSchemaInvalid);
}

[[nodiscard]] auto SelectContextNodes(const ast::ValueExpr& subject_expr,
                                      input::Source& source,
                                      input::NodeRef parent,
                                      const std::filesystem::path& include_base,
                                      Context ctx)
    -> EngineResult<std::pmr::vector<input::NodeRef>> {
  auto nodes = eval::EvalToNodes(subject_expr, source, parent,
                                 eval::EvalParams::From(include_base, ctx));
  if (!nodes) [[unlikely]] {
    return std::unexpected(EngineError::kEvalFailed);
  }

  return std::move(*nodes);
}

[[nodiscard]] auto SelectRootNodes(const ast::Schema& schema,
                                   input::Source& source,
                                   const std::filesystem::path& include_base,
                                   Context ctx)
    -> EngineResult<std::pmr::vector<input::NodeRef>> {
  if (!schema.description.context || !schema.description.context->subject) {
    std::pmr::vector<input::NodeRef> nodes{ctx.GetAllocator()};
    nodes.push_back(source.Root());
    return nodes;
  }

  return SelectContextNodes(*schema.description.context->subject, source,
                            source.Root(), include_base, ctx);
}

[[nodiscard]] auto WalkDescriptions(const ast::Description& root,
                                    input::Source& source,
                                    input::NodeRef root_node,
                                    const std::filesystem::path& include_base,
                                    Context ctx, rdf::Graph& out,
                                    rdf::Graph& working) -> EngineResult<void> {
  auto* const mr = ctx.GetAllocator();
  std::pmr::deque<DescriptionFrame> desc_queue{mr};
  desc_queue.push_back(DescriptionFrame{
      .description = root,
      .pseudos = PseudoMap{mr},
      .parent_node = root_node,
      .link_subject = std::nullopt,
      .link_predicate = std::nullopt,
      .allow_subject_rebind = false,
  });

  while (!desc_queue.empty()) {
    auto desc_frame = std::move(desc_queue.front());
    desc_queue.pop_front();

    const auto& description = desc_frame.description.get();

    std::pmr::vector<input::NodeRef> nodes{mr};
    if (desc_frame.allow_subject_rebind && description.context &&
        description.context->subject) {
      auto selected =
          SelectContextNodes(*description.context->subject, source,
                             desc_frame.parent_node, include_base, ctx);
      if (!selected) [[unlikely]] {
        return std::unexpected(selected.error());
      }
      nodes = std::move(*selected);
    } else {
      nodes.push_back(desc_frame.parent_node);
    }

    for (const input::NodeRef node : nodes) {
      PseudoMap pseudos = CopyPseudoMap(desc_frame.pseudos, ctx);

      if (description.context) {
        auto applied = ApplyContextPseudos(*description.context, source, node,
                                           working, pseudos, include_base, ctx);
        if (!applied) [[unlikely]] {
          return applied;
        }
      }

      if (!description.about) [[unlikely]] {
        return std::unexpected(EngineError::kSchemaInvalid);
      }

      auto about =
          eval::EvalToString(*description.about, source, node, working, pseudos,
                             eval::EvalParams::From(include_base, ctx));
      if (!about) [[unlikely]] {
        return std::unexpected(EngineError::kEvalFailed);
      }

      const auto subject = rdf::Iri::From(std::move(*about));
      if (desc_frame.link_subject && desc_frame.link_predicate) {
        EmitTriple(out, working,
                   rdf::Triple::From(*desc_frame.link_subject,
                                     *desc_frame.link_predicate, subject));
      }

      std::optional<std::pmr::vector<rdf::Iri>> desc_predicates;
      std::span<const rdf::Iri> defaults{};
      if (description.context && description.context->predicates) {
        auto resolved =
            ResolvePredicates(*description.context->predicates, source, node,
                              working, pseudos, include_base, ctx);
        if (!resolved) [[unlikely]] {
          return std::unexpected(resolved.error());
        }
        desc_predicates = std::move(*resolved);
        defaults = *desc_predicates;
      }

      std::pmr::vector<PropertyFrame> prop_stack{mr};
      prop_stack.push_back(PropertyFrame{
          .properties = description.properties,
          .subject = subject,
          .index = 0,
          .pseudos = std::move(pseudos),
          .default_predicates = defaults,
      });

      while (!prop_stack.empty()) {
        auto& frame = prop_stack.back();
        if (frame.index >= frame.properties.get().size()) {
          prop_stack.pop_back();
          continue;
        }

        const auto& property = frame.properties.get()[frame.index++];
        PseudoMap local = CopyPseudoMap(frame.pseudos, ctx);
        if (property.context) {
          auto applied = ApplyContextPseudos(*property.context, source, node,
                                             working, local, include_base, ctx);
          if (!applied) [[unlikely]] {
            return applied;
          }
        }

        if (property.description) {
          auto predicates = ResolvePropertyPredicates(
              property, source, node, working, local, include_base, ctx,
              frame.default_predicates);
          if (!predicates) [[unlikely]] {
            return std::unexpected(predicates.error());
          }

          for (const auto& predicate : *predicates) {
            desc_queue.push_back(DescriptionFrame{
                .description = *property.description,
                .pseudos = CopyPseudoMap(local, ctx),
                .parent_node = node,
                .link_subject = frame.subject,
                .link_predicate = predicate,
                .allow_subject_rebind = true,
            });
          }
          continue;
        }

        if (!property.properties.empty() && !property.uri &&
            !property.literal) {
          prop_stack.push_back(PropertyFrame{
              .properties = property.properties,
              .subject = frame.subject,
              .index = 0,
              .pseudos = std::move(local),
              .default_predicates = frame.default_predicates,
          });
          continue;
        }

        auto predicates = ResolvePropertyPredicates(
            property, source, node, working, local, include_base, ctx,
            frame.default_predicates);
        if (!predicates) [[unlikely]] {
          return std::unexpected(predicates.error());
        }

        for (const auto& predicate : *predicates) {
          auto emitted = EmitTerminalProperty(
              property, frame.subject, predicate, source, node, working, local,
              include_base, ctx, out, working);
          if (!emitted) [[unlikely]] {
            return emitted;
          }
        }
      }
    }
  }

  return {};
}

}  // namespace

auto Run(const ast::Schema& schema, input::Source& source,
         const RunParams& params) -> EngineResult<rdf::Graph> {
  const Context ctx = params.ctx;

  auto subjects = SelectRootNodes(schema, source, params.include_base, ctx);
  if (!subjects) [[unlikely]] {
    return std::unexpected(subjects.error());
  }

  auto out = rdf::Graph::FromContext(ctx);
  auto working = rdf::Graph::FromContext(ctx);
  for (const auto& triple : params.knowledge.Triples()) {
    working.Add(triple);
  }

  for (const input::NodeRef subject_node : *subjects) {
    auto result = WalkDescriptions(schema.description, source, subject_node,
                                   params.include_base, ctx, out, working);
    if (!result) [[unlikely]] {
      return std::unexpected(result.error());
    }
  }

  // Ontology ranges live in the seed; instance types may be in out or seed.
  auto range_ok = validate::CheckRdfsRange(out, params.knowledge, ctx);
  if (!range_ok) [[unlikely]] {
    return std::unexpected(EngineError::kValidationFailed);
  }

  return out;
}

}  // namespace cccr::engine

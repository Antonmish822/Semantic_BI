#include <pch.hpp>

#include <cccr/eval/qname.hpp>

#include <cccr/context.hpp>

#include <detail/iri.hpp>

#include <string>
#include <string_view>

namespace cccr::eval {

std::pmr::string QNameToIri(const ast::QName& qname, Context ctx) {
  std::string_view local = qname.prefixed_name;
  if (const auto pos = local.rfind(':'); pos != std::string_view::npos) {
    local = local.substr(pos + 1);
  }

  return detail::JoinNsLocal(qname.name_space, local, ctx.GetAllocator());
}

}  // namespace cccr::eval

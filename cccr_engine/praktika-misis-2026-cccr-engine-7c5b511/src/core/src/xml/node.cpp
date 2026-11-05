#include <pch.hpp>

#include <cccr/xml/node.hpp>

#include <saxonc/XdmNode.h>

#include <utility>

namespace cccr::xml {

Document& Document::operator=(Document&& other) noexcept {
  if (this == &other) [[unlikely]] {
    return *this;
  }

  delete static_cast<XdmNode*>(root_);
  root_ = std::exchange(other.root_, nullptr);

  return *this;
}

Document::~Document() {
  delete static_cast<XdmNode*>(root_);
  root_ = nullptr;
}

}  // namespace cccr::xml

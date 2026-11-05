#include <pybind11/pybind11.h>

#include "bind.hpp"

#ifndef CCCR_VERSION_INFO
#error "CCCR_VERSION_INFO must be defined by the build"
#endif

namespace py = pybind11;

/// @brief Python extension module for the CCCR engine.
PYBIND11_MODULE(cccr_py, m) {
  m.doc() = "CCCR engine Python bindings";
  m.attr("__version__") = CCCR_VERSION_INFO;

  // Register handle types before Context methods that return them.
  cccr::bindings::python::BindSchema(m);
  cccr::bindings::python::BindSource(m);
  cccr::bindings::python::BindGraph(m);
  cccr::bindings::python::BindContext(m);
}

#pragma once

#include <pybind11/pybind11.h>

namespace cccr::bindings::python {

void BindSchema(pybind11::module_& module);
void BindSource(pybind11::module_& module);
void BindGraph(pybind11::module_& module);
void BindContext(pybind11::module_& module);

}  // namespace cccr::bindings::python

#include "bind.hpp"
#include "schema.hpp"

namespace py = pybind11;

namespace cccr::bindings::python {

void BindSchema(py::module_& module) {
  py::class_<Schema>(module, "Schema", "Loaded CCCR schema bound to a Context");
}

}  // namespace cccr::bindings::python

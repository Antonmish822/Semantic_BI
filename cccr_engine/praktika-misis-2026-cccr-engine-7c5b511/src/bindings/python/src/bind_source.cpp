#include "bind.hpp"
#include "source.hpp"

namespace py = pybind11;

namespace cccr::bindings::python {

void BindSource(py::module_& module) {
  py::class_<Source>(module, "Source",
                     "Loaded input document bound to a Context");
}

}  // namespace cccr::bindings::python

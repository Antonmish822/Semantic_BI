#include "bind.hpp"
#include "context.hpp"
#include "graph.hpp"
#include "schema.hpp"
#include "source.hpp"

#include <pybind11/stl.h>

#include <cstddef>
#include <functional>
#include <optional>
#include <vector>

namespace py = pybind11;

namespace cccr::bindings::python {

void BindContext(py::module_& module) {
  py::class_<Context>(module, "Context",
                      "CCCR runtime context (allocator + arena)")
      .def(py::init<size_t>(), py::arg("arena_size") = 0,
           "Create a context; arena_size 0 selects the default arena.")
      .def_property_readonly("last_error", &Context::LastError,
                             "Last error message (empty when none).")
      .def("clear_error", &Context::ClearError, "Clear the last error message.")
      .def("load_schema", &Context::LoadSchema, py::arg("json"),
           py::arg("include_base") = py::none(),
           py::call_guard<py::gil_scoped_release>(),
           "Load a CCCR schema from JSON text. Returns None on failure.")
      .def("load_xml", &Context::LoadXml, py::arg("xml"),
           py::call_guard<py::gil_scoped_release>(),
           "Load an XML/HTML input document. Returns None on failure.")
      .def("load_csv", &Context::LoadCsv, py::arg("csv"),
           py::call_guard<py::gil_scoped_release>(),
           "Load a CSV input document. Returns None on failure.")
      .def("load_graph", &Context::LoadGraph, py::arg("text"),
           py::arg("format") = "ntriples",
           py::call_guard<py::gil_scoped_release>(),
           "Load an RDF graph from text. Returns None on failure.")
      .def(
          "run",
          [](Context& self, const Schema& schema, Source& source,
             const std::optional<std::reference_wrapper<const Graph>>&
                 knowledge) {
            return self.Run(
                schema, source,
                knowledge.has_value() ? &knowledge->get() : nullptr);
          },
          py::arg("schema"), py::arg("source"),
          py::arg("knowledge") = py::none(),
          py::call_guard<py::gil_scoped_release>(),
          "Run a CCCR schema over a source. Returns None on failure.")
      .def(
          "run_many",
          [](Context& self, const Schema& schema, const py::sequence& sources,
             const std::optional<std::reference_wrapper<const Graph>>&
                 knowledge) {
            std::vector<std::reference_wrapper<Source>> refs;
            refs.reserve(py::len(sources));
            for (py::handle item : sources) {
              refs.emplace_back(py::cast<Source&>(item));
            }
            py::gil_scoped_release release;
            return self.RunMany(
                schema, refs,
                knowledge.has_value() ? &knowledge->get() : nullptr);
          },
          py::arg("schema"), py::arg("sources"),
          py::arg("knowledge") = py::none(),
          "Run a CCCR schema over multiple sources and merge outputs. "
          "Returns None on failure.");
}

}  // namespace cccr::bindings::python

#include "bind.hpp"
#include "graph.hpp"

#include <pybind11/stl.h>

namespace py = pybind11;

namespace cccr::bindings::python {

void BindGraph(py::module_& module) {
  py::class_<Graph>(module, "Graph", "RDF graph bound to a Context")
      .def("to_rdfxml", &Graph::ToRdfXml,
           py::call_guard<py::gil_scoped_release>(),
           "Serialize the graph to RDF/XML. Returns None on failure.")
      .def("query", &Graph::Query, py::arg("sparql"),
           py::arg("result_format") = "json",
           py::call_guard<py::gil_scoped_release>(),
           "Run a SPARQL SELECT. Returns None on failure.")
      .def("merge", &Graph::Merge, py::arg("other"),
           py::call_guard<py::gil_scoped_release>(),
           "Merge another graph into this one. Returns False on failure.");
}

}  // namespace cccr::bindings::python
